#include "bucket.h"

#include <cmath>
#include "vtr_log.h"

BucketItems::BucketItems() noexcept
    : alloced_items_(0)
    , num_heap_allocated_(0)
    , heap_free_head_(nullptr) {}

Bucket::Bucket() noexcept
    : seed_(1231)
    , heap_(nullptr)
    , heap_size_(0)
    , heap_head_(std::numeric_limits<size_t>::max())
    , heap_tail_(0)
    , conv_factor_(0.f)
    , min_cost_(0.f)
    , max_cost_(0.f) {}

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
            VTR_ASSERT(data->item.cost > 0 && ((size_t)cost_to_int(data->item.cost)) == bucket);
        }
    }
}

void Bucket::empty_heap() {
    if (heap_head_ != std::numeric_limits<size_t>::max()) {
        std::fill(heap_ + heap_head_, heap_ + heap_tail_ + 1, nullptr);
    }
    heap_head_ = std::numeric_limits<size_t>::max();
    heap_tail_ = 0;

    // Quickly reset all items to being free'd
    items_.clear();
}

void Bucket::check_scaling() {
    float min_cost = min_cost_;
    float max_cost = max_cost_;
    VTR_ASSERT(max_cost != std::numeric_limits<float>::min());
    if (min_cost == std::numeric_limits<float>::max()) {
        min_cost = max_cost;
    }
    auto min_bucket = cost_to_int(min_cost);
    auto max_bucket = cost_to_int(max_cost);

    if (min_bucket < 0 || max_bucket < 0 || max_bucket > 1000000) {
        // If min and max are close to each other, assume 3 orders of
        // magnitude between min and max.
        //
        // If min and max are at least 3 orders of magnitude apart, scale
        // soley based on max cost.
        conv_factor_ = 50000.f / max_cost_ / std::max(1.f, 1000.f / (max_cost_ / min_cost_));

        VTR_ASSERT(cost_to_int(min_cost_) >= 0);
        VTR_ASSERT(cost_to_int(max_cost_) >= 0);
        VTR_ASSERT(cost_to_int(max_cost_) < 1000000);

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
                push_back(&item->item);
            }
        }
    }
}

void Bucket::push_back(t_heap* hptr) {
    float cost = hptr->cost;
    if (!std::isfinite(cost)) {
        return;
    }

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
