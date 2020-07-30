#include "bucket.h"

#include <cmath>
#include "vtr_log.h"
#include "vpr_error.h"

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
    , min_cost_(0.f)
    , max_cost_(0.f)
    , num_items_(0)
    , max_index_(std::numeric_limits<size_t>::max())
    , prune_limit_(std::numeric_limits<size_t>::max()) {}

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
    heap_tail_ = 0;
    num_items_ = 0;

    conv_factor_ = kDefaultConvFactor;

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
    heap_tail_ = 0;
    num_items_ = 0;

    // Quickly reset all items to being free'd
    items_.clear();
}

float Bucket::rescale_func() const {
    return 50000.f / max_cost_ / std::max(1.f, 1000.f / (max_cost_ / min_cost_));
}

void Bucket::check_conv_factor() const {
    VTR_ASSERT(cost_to_int(min_cost_) >= 0);
    VTR_ASSERT(cost_to_int(max_cost_) >= 0);
    VTR_ASSERT(cost_to_int(max_cost_) < 1000000);
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
    if (min_bucket < 0 || max_bucket < 0 || max_bucket > 1000000) {
        // Choose a scaling factor that accomidates 50k buckets between
        // min_cost_ and max_cost_.
        //
        // If min and max are close to each other, assume 3 orders of
        // magnitude between min and max.  The goal is to rescale less often
        // when the larger costs haven't been seen yet.
        //
        // If min and max are at least 3 orders of magnitude apart, scale
        // soley based on max cost.  The goal at this point is to keep the
        // number of buckets between 50k and 100k.
        //
        // NOTE:  The precision loss of the bucket approximation grows as the
        // maximum cost increases.  The underlying assumption of this scaling
        // algorithm is that the maximum cost will not result in a poor
        // scaling factor such that all precision is lost.
        conv_factor_ = rescale_func();
        check_conv_factor();

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
}

void Bucket::push_back(t_heap* hptr) {
    VTR_ASSERT(outstanding_items_ > 0);
    outstanding_items_ -= 1;

    float cost = hptr->cost;
    if (!std::isfinite(cost)) {
        return;
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

    item->next_bucket = prev;
    heap_[uint_cost] = item;

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

    // Find first non-empty bucket

    // Randomly remove element
    size_t count = fast_rand() % 4;

    BucketItem* prev = nullptr;
    BucketItem* next = heap[heap_head];
    for (size_t i = 0; i < count && next->next_bucket != nullptr; ++i) {
        prev = next;
        next = prev->next_bucket;
    }

    if (prev == nullptr) {
        heap[heap_head] = next->next_bucket;
    } else {
        prev->next_bucket = next->next_bucket;
    }

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
    }

    outstanding_items_ += 1;
    num_items_ -= 1;
    return &next->item;
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
    heap_tail_ = 0;
    num_items_ = 0;

    // Re-heap the pruned elements.
    for (BucketItem* item : best_heap_item) {
        if (item == nullptr) {
            continue;
        }

        outstanding_items_ += 1;
        push_back(&item->item);
    }

    verify();
}
