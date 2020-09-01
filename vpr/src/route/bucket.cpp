#include "bucket.h"

#include <cmath>
#include "vtr_log.h"
#include "vpr_error.h"

/* Bucket spacing algorithm:
 *
 * The size in cost each bucket consumes is a fixed width determined by
 * conv_factor_.  The bucket index equation is simply:
 *
 *  bucket index = cost * conv_factor_
 *
 * The default conv_factor_ is 1e12, e.g. each bucket is 1 picosecond wide.
 *
 * There two reasons to change conv_factor_:
 *  - The maximum cost item in the bucket would require too many buckets in
 *    heap_, and would cause memory usage to climb higher than desired.
 *  - The front bucket contains too many items, making the pop operation too
 *    cost insenstive.
 *
 * The other consideration is to avoid rescaling the buckets too often, as
 * that operation consumes time without delivering useful work.
 *
 * To prevent rescaling constantly, the bucket heap determines if a rescaling
 * is needed based on two conditions:
 *
 *  - The maximum item cost (max_cost_) would require a bucket index that is
 *    greater than max_buckets_.  When this occurs, a rescaling is done to
 *    make the width of the buckets larger so that the cost index for the
 *    max_cost_ item fits within max_buckets_.
 *
 *     - A larger max_buckets_ results in more memory consumption, but
 *       accomidates a wider range of items without needing to rescale.
 *
 *  - The number of items in the first bucket exceeds kIncreaseFocusLimit. In
 *    this case, the bucket heap will shrink the width of the buckets so that
 *    the number of entries in the first bucket drops below
 *    kIncreaseFocusLimit.
 *
 * In both of the above cases, rescaling is determined by the following
 * (simplified) equation:
 *
 *    conv_factor_ = division_scaling_ / max_cost_
 *
 * The default division_scaling_ is kInitialDivisionScaling (50k).  This
 * can be read as using 50k buckets to evenly divided based on the
 * maximum cost.  For example, if max cost = 100 ns, then each bucket would
 * be 2 picosecond wide.
 *
 * When the number of elements in the first bucket exceeds
 * kIncreaseFocusLimit, division_scaling_ is multiplied by two, effectively
 * halving the bucket size for a given max_cost_.  In addition max_buckets_
 * is also multiplied by two to result in a similiar rescaling rate as
 * max_cost_ increases.
 *
 * This multiply by two logic could result in an unbounded memory consumption
 * in the number of buckets. To limit this, the 2x scaling for
 * division_scaling_ and max_buckets_ is limited such that max_buckets_ never
 * exceeds kMaxMaxBuckets
 *
 */

// Initial bucket scaling.  A larger division scaling results in smaller cost
// range per bucket.
static constexpr float kInitialDivisionScaling = 50000.f;
// Initial maximum number of buckets before bucket rescaling.
static constexpr ssize_t kInitialMaxBuckets = 1000000;
// If the division scaling results in more than kIncreaseFocusLimit elements
// in the first bucket, than division scaling is increased by 2x to try to
// lower the size of the first bucket.
//
// This is an attempt to dynamically scale the bucket widths to prevent the
// bucket heap from being too cost insenstive / imprecise.
//
// When the division scaling is increased by 2x, the maximum number of buckets
// also is increased by 2x to prevent excessive rescaling during runtime.
static constexpr size_t kIncreaseFocusLimit = 2048;
// To prevent unbounded division scaling, the 2x when the first bucket is too
// large is limited by kMaxMaxBuckets.  If increasing the division scaling
// will result in max_buckets_ exceeding kMaxMaxBuckets, then division scaling
// will not be increased again.
static constexpr ssize_t kMaxMaxBuckets = 16000000;

BucketItems::BucketItems() noexcept
    : alloced_items_(0)
    , num_heap_allocated_(0)
    , heap_free_head_(nullptr) {}

Bucket::Bucket() noexcept
    : outstanding_items_(0)
    , seed_(1231)
    , heap_(nullptr)
    , heap_size_(0)
    , heap_head_(std::numeric_limits<size_t>::max())
    , heap_tail_(0)
    , conv_factor_(0.f)
    , division_scaling_(kInitialDivisionScaling)
    , max_buckets_(kInitialMaxBuckets)
    , min_cost_(0.f)
    , max_cost_(0.f)
    , num_items_(0)
    , max_index_(std::numeric_limits<size_t>::max())
    , prune_limit_(std::numeric_limits<size_t>::max())
    , prune_count_(0)
    , front_head_(std::numeric_limits<size_t>::max()) {}

Bucket::~Bucket() {
    free_all_memory();
}

void Bucket::init_heap(const DeviceGrid& grid) {
    vtr::free(heap_);
    heap_ = nullptr;

    heap_size_ = (grid.width() - 1) * (grid.height() - 1);
    heap_ = (BucketItem**)vtr::malloc(heap_size_ * sizeof(BucketItem*));
    memset(heap_, 0, heap_size_ * sizeof(t_heap*));

    heap_head_ = std::numeric_limits<size_t>::max();
    front_head_ = std::numeric_limits<size_t>::max();
    heap_tail_ = 0;
    num_items_ = 0;
    prune_count_ = 0;

    conv_factor_ = kDefaultConvFactor;
    division_scaling_ = kInitialDivisionScaling;
    max_buckets_ = kInitialMaxBuckets;

    min_cost_ = std::numeric_limits<float>::max();
    max_cost_ = std::numeric_limits<float>::min();
}

void Bucket::free_all_memory() {
    vtr::free(heap_);
    heap_ = nullptr;

    items_.free();
}

void Bucket::expand(size_t required_number_of_buckets) {
    auto old_size = heap_size_;
    heap_size_ = required_number_of_buckets * 2;

    heap_ = (BucketItem**)vtr::realloc((void*)(heap_),
                                       heap_size_ * sizeof(BucketItem*));
    std::fill(heap_ + old_size, heap_ + heap_size_, nullptr);
}

void Bucket::verify() {
    for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
        for (BucketItem* data = heap_[bucket]; data != nullptr;
             data = data->next_bucket) {
            VTR_ASSERT(data->item.cost >= 0);
            int bucket_idx = cost_to_int(data->item.cost);
            if (bucket_idx != static_cast<ssize_t>(bucket)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Wrong bucket for cost %g bucket_idx %d bucket %zu conv_factor %g",
                                data->item.cost, bucket_idx, bucket, conv_factor_);
            }
        }
    }
}

void Bucket::empty_heap() {
    VTR_ASSERT(outstanding_items_ == 0);

    if (heap_head_ != std::numeric_limits<size_t>::max()) {
        std::fill(heap_ + heap_head_, heap_ + heap_tail_ + 1, nullptr);
    }
    heap_head_ = std::numeric_limits<size_t>::max();
    front_head_ = std::numeric_limits<size_t>::max();
    heap_tail_ = 0;
    num_items_ = 0;
    prune_count_ = 0;
    min_push_cost_.clear();

    // Quickly reset all items to being free'd
    items_.clear();

    conv_factor_ = kDefaultConvFactor;
    division_scaling_ = kInitialDivisionScaling;
    max_buckets_ = kInitialMaxBuckets;

    min_cost_ = std::numeric_limits<float>::max();
    max_cost_ = std::numeric_limits<float>::min();
}

float Bucket::rescale_func() const {
    // Choose a scaling factor that accomidates division_scaling_ buckets
    // between min_cost_ and max_cost_.
    //
    // If min and max are close to each other, assume 3 orders of
    // magnitude between min and max.  The goal is to rescale less often
    // when the larger costs haven't been seen yet.
    //
    // If min and max are at least 3 orders of magnitude apart, scale
    // soley based on max cost.  The goal at this point is to keep the
    // number of buckets between division_scaling_ and division_scaling_*2.
    return division_scaling_ / max_cost_ / std::max(1.f, 1000.f / (max_cost_ / min_cost_));
}

void Bucket::check_conv_factor() const {
    VTR_ASSERT(cost_to_int(min_cost_) >= 0);
    VTR_ASSERT(cost_to_int(max_cost_) >= 0);
    VTR_ASSERT(cost_to_int(max_cost_) < max_buckets_);
}

// Checks if the scaling factor for cost results in a reasonable
// number of buckets based on the maximum cost value seen.
//
// Target number of buckets is between 50k and 100k buckets.
// Default scaling is each bucket is around ~1 ps wide.
//
// Designs with scaled costs less than 100000 (e.g. 100 ns) shouldn't require
// a bucket resize.
void Bucket::check_scaling() {
    float min_cost = min_cost_;
    float max_cost = max_cost_;
    VTR_ASSERT(max_cost != std::numeric_limits<float>::min());
    if (min_cost == std::numeric_limits<float>::max()) {
        min_cost = max_cost;
    }
    auto min_bucket = cost_to_int(min_cost);
    auto max_bucket = cost_to_int(max_cost);

    // If scaling is invalid or more than 100k buckets are needed, rescale.
    if (min_bucket < 0 || max_bucket < 0 || max_bucket > max_buckets_) {
        rescale();
    }
}

void Bucket::rescale() {
    conv_factor_ = rescale_func();
    check_conv_factor();
    front_head_ = std::numeric_limits<size_t>::max();

    // Reheap after adjusting scaling.
    if (heap_head_ != std::numeric_limits<size_t>::max()) {
        std::vector<BucketItem*> reheap;
        for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
            for (BucketItem* item = heap_[bucket]; item != nullptr; item = item->next_bucket) {
                reheap.push_back(item);
            }
        }

        std::fill(heap_ + heap_head_, heap_ + heap_tail_ + 1, nullptr);
        heap_head_ = std::numeric_limits<size_t>::max();
        heap_tail_ = 0;

        for (BucketItem* item : reheap) {
            outstanding_items_ += 1;
            push_back(&item->item);
        }
    }
}

void Bucket::push_back(t_heap* hptr) {
    VTR_ASSERT(outstanding_items_ > 0);
    outstanding_items_ -= 1;

    float cost = hptr->cost;
    if (!std::isfinite(cost)) {
        BucketItem* item = reinterpret_cast<BucketItem*>(hptr);
        items_.free_item(item);
        return;
    }

    if (!min_push_cost_.empty()) {
        if (hptr->cost > min_push_cost_[hptr->index]) {
            BucketItem* item = reinterpret_cast<BucketItem*>(hptr);
            items_.free_item(item);
            return;
        }

        min_push_cost_[hptr->index] = hptr->cost;
    }

    // Check to see if the range of costs observed by the heap has changed.
    bool check_scale = false;

    // Exclude 0 cost from min_cost to provide useful scaling factor.
    if (cost < min_cost_ && cost > 0) {
        min_cost_ = cost;
        check_scale = true;
    }
    if (cost > max_cost_) {
        max_cost_ = cost;
        check_scale = true;
    }

    // Rescale the number and size of buckets if needed based on the new
    // cost range.
    if (check_scale) {
        check_scaling();
    }

    // Which bucket should this go into?
    auto int_cost = cost_to_int(cost);

    if (int_cost < 0) {
        VTR_LOG_WARN("Cost is negative? cost = %g, bucket = %d\n", cost, int_cost);
        int_cost = 0;
    }

    size_t uint_cost = int_cost;

    // Is that bucket allocated?
    if (uint_cost >= heap_size_) {
        // Not enough buckets!
        expand(uint_cost);
    }

    // Insert into bucket
    auto* prev = heap_[uint_cost];

    // Static assert ensures that BucketItem::item is at offset 0,
    // so this cast is safe.
    BucketItem* item = reinterpret_cast<BucketItem*>(hptr);

    if (front_head_ == uint_cost) {
        VTR_ASSERT(prev != nullptr);
        front_list_.back()->next_bucket = item;
        item->next_bucket = nullptr;
        front_list_.push_back(item);
    } else {
        // Otherwise just add to front list.
        item->next_bucket = prev;
        heap_[uint_cost] = item;
    }

    if (uint_cost < heap_head_) {
        heap_head_ = uint_cost;
    }
    if (uint_cost > heap_tail_) {
        heap_tail_ = uint_cost;
    }

    num_items_ += 1;
    if (num_items_ > prune_limit_) {
        prune_heap();
    }
}

t_heap* Bucket::get_heap_head() {
    auto heap_head = heap_head_;
    auto heap_tail = heap_tail_;
    BucketItem** heap = heap_;

    // Check empty
    if (heap_head == std::numeric_limits<size_t>::max()) {
        return nullptr;
    }

    if (front_head_ != heap_head) {
        front_list_.clear();
        for (BucketItem* item = heap[heap_head]; item != nullptr; item = item->next_bucket) {
            front_list_.push_back(item);
            VTR_ASSERT(front_list_.size() <= num_items_);
        }

        // If the front bucket is more than kIncreaseFocusLimit, then change
        // the division scaling to attempt to shrink the front bucket size.
        //
        // kMaxMaxBuckets prevents this scaling from continuing without limit.
        if (front_list_.size() > kIncreaseFocusLimit && max_buckets_ < kMaxMaxBuckets) {
            division_scaling_ *= 2;
            max_buckets_ *= 2;
            rescale();
            return get_heap_head();
        }
        VTR_ASSERT(!front_list_.empty());
        front_head_ = heap_head;
        VTR_ASSERT_DEBUG(check_front_list());
    }

    // Find first non-empty bucket

    // Randomly remove element
    size_t count = fast_rand() % front_list_.size();
    BucketItem* item = front_list_[count];

    // If the element is the back of the list, just remove it.
    if (count + 1 == front_list_.size()) {
        if (front_list_.size() > 1) {
            // Stitch into list.
            front_list_[count - 1]->next_bucket = nullptr;
        } else {
            // List is now empty.
            heap[heap_head] = nullptr;
        }
    } else {
        // This is not the back element, so swap the element we are popping
        // with the back element, then remove it.
        BucketItem* swap = front_list_.back();
        if (front_list_.size() > 2) {
            front_list_[front_list_.size() - 2]->next_bucket = nullptr;
        }

        // Update the front_list_
        front_list_[count] = swap;

        if (count == 0) {
            // Swap this element to the front of the list.
            heap[heap_head] = swap;
        } else {
            // Stitch this element back into the list
            front_list_[count - 1]->next_bucket = swap;
        }

        swap->next_bucket = item->next_bucket;
    }

    front_list_.pop_back();

    VTR_ASSERT_DEBUG(check_front_list());

    // Update first non-empty bucket if bucket is now empty
    if (heap[heap_head] == nullptr) {
        heap_head += 1;
        while (heap_head <= heap_tail && heap[heap_head] == nullptr) {
            heap_head += 1;
        }

        if (heap_head > heap_tail) {
            heap_head = std::numeric_limits<size_t>::max();
        }

        heap_head_ = heap_head;
        front_head_ = std::numeric_limits<size_t>::max();
    }

    outstanding_items_ += 1;
    num_items_ -= 1;
    return &item->item;
}

void Bucket::invalidate_heap_entries(int /*sink_node*/, int /*ipin_node*/) {
    throw std::runtime_error("invalidate_heap_entries not implemented for Bucket");
}

void Bucket::print() {
    for (size_t i = heap_head_; i < heap_tail_; ++i) {
        if (heap_[heap_head_] != nullptr) {
            VTR_LOG("B:%d ", i);
            for (auto* item = heap_[i]; item != nullptr; item = item->next_bucket) {
                VTR_LOG(" %e", item->item.cost);
            }
        }
    }
    VTR_LOG("\n");
}

void Bucket::set_prune_limit(size_t max_index, size_t prune_limit) {
    if (prune_limit != std::numeric_limits<size_t>::max()) {
        VTR_ASSERT(max_index < prune_limit);
    }
    max_index_ = max_index;
    prune_limit_ = prune_limit;
}

void Bucket::prune_heap() {
    std::vector<BucketItem*> best_heap_item(max_index_, nullptr);

    for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
        for (BucketItem* item = heap_[bucket]; item != nullptr; item = item->next_bucket) {
            VTR_ASSERT(static_cast<size_t>(item->item.index) < max_index_);
            if (best_heap_item[item->item.index] == nullptr || best_heap_item[item->item.index]->item.cost > item->item.cost) {
                best_heap_item[item->item.index] = item;
            }
        }
    }

    min_cost_ = std::numeric_limits<float>::max();
    max_cost_ = std::numeric_limits<float>::min();
    for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
        BucketItem* item = heap_[bucket];
        while (item != nullptr) {
            BucketItem* next_item = item->next_bucket;

            if (best_heap_item[item->item.index] != item) {
                // This item isn't the cheapest, return it to the free list.
                items_.free_item(item);
            } else {
                // Update min_cost_ and max_cost_
                if (min_cost_ > item->item.cost) {
                    min_cost_ = item->item.cost;
                }
                if (max_cost_ < item->item.cost) {
                    max_cost_ = item->item.cost;
                }
            }

            item = next_item;
        }
    }

    // Rescale heap after pruning.
    conv_factor_ = rescale_func();
    check_conv_factor();

    std::fill(heap_, heap_ + heap_size_, nullptr);
    heap_head_ = std::numeric_limits<size_t>::max();
    front_head_ = std::numeric_limits<size_t>::max();
    front_list_.clear();
    heap_tail_ = 0;
    num_items_ = 0;
    prune_count_ += 1;

    // Re-heap the pruned elements.
    for (BucketItem* item : best_heap_item) {
        if (item == nullptr) {
            continue;
        }

        outstanding_items_ += 1;
        push_back(&item->item);
    }

    verify();

    if (prune_count_ >= 1) {
        // If pruning is happening repeatedly, start pruning at entry.
        min_push_cost_.resize(max_index_, std::numeric_limits<float>::infinity());
    }
}

bool Bucket::check_front_list() const {
    VTR_ASSERT(heap_head_ == front_head_);
    size_t i = 0;
    BucketItem* item = heap_[heap_head_];
    while (item != nullptr) {
        if (front_list_.at(i) != item) {
            VTR_LOG(
                "front_list_ (%p size %zu) [%zu] %p != item %p\n",
                front_list_.data(), front_list_.size(), i, front_list_[i], item);
            VTR_ASSERT(front_list_[i] == item);
        }
        i += 1;
        item = item->next_bucket;
    }
    return false;
}
