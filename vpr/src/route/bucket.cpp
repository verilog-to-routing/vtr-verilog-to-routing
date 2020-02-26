#include "bucket.h"

std::vector<t_heap*> BucketItems::heap_items_;
size_t BucketItems::alloced_items_ = 0;
int BucketItems::num_heap_allocated_ = 0;
t_heap* BucketItems::heap_free_head_ = nullptr;
vtr::t_chunk BucketItems::heap_ch_;

void Bucket::init(const DeviceGrid& grid) {
    vtr::free(heap_);
    heap_ = nullptr;

    heap_size_ = (grid.width() - 1) * (grid.height() - 1);
    heap_ = (t_heap**)vtr::malloc(heap_size_ * sizeof(t_heap*));
    memset(heap_, 0, heap_size_ * sizeof(t_heap*));

    heap_head_ = std::numeric_limits<size_t>::max();
    heap_tail_ = 0;

    conv_factor_ = kDefaultConvFactor;

    min_cost_ = std::numeric_limits<float>::max();
    max_cost_ = std::numeric_limits<float>::min();
}

void Bucket::free() {
    vtr::free(heap_);
    heap_ = nullptr;
}

void Bucket::expand(size_t required_number_of_buckets) {
    auto old_size = heap_size_;
    heap_size_ = required_number_of_buckets * 2;

    heap_ = (t_heap**)vtr::realloc((void*)(heap_),
                                   heap_size_ * sizeof(t_heap*));
    std::fill(heap_ + old_size, heap_ + heap_size_, nullptr);
}

void Bucket::verify() {
    for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
        for (t_heap* data = heap_[bucket]; data != nullptr;
             data = data->next_bucket) {
            VTR_ASSERT(data->cost > 0 && ((size_t)cost_to_int(data->cost)) == bucket);
        }
    }
}

size_t Bucket::seed_ = 1231;
t_heap** Bucket::heap_ = nullptr;
size_t Bucket::heap_size_ = 0;
size_t Bucket::heap_head_ = std::numeric_limits<size_t>::max();
size_t Bucket::heap_tail_ = 0;
float Bucket::min_cost_ = 0.f;
float Bucket::max_cost_ = 0.f;
float Bucket::conv_factor_ = 0.f;

void Bucket::clear() {
    if (heap_head_ != std::numeric_limits<size_t>::max()) {
        std::fill(heap_ + heap_head_, heap_ + heap_tail_ + 1, nullptr);
    }
    heap_head_ = std::numeric_limits<size_t>::max();
    heap_tail_ = 0;
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
            std::vector<t_heap*> reheap;
            for (size_t bucket = heap_head_; bucket <= heap_tail_; ++bucket) {
                for (t_heap* item = heap_[bucket]; item != nullptr; item = item->next_bucket) {
                    reheap.push_back(item);
                }
            }

            std::fill(heap_ + heap_head_, heap_ + heap_tail_ + 1, nullptr);
            heap_head_ = std::numeric_limits<size_t>::max();
            heap_tail_ = 0;

            for (t_heap* item : reheap) {
                push(item);
            }
        }
    }
}

void Bucket::push(t_heap* hptr) {
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
    hptr->next_bucket = prev;
    heap_[uint_cost] = hptr;

    if (uint_cost < heap_head_) {
        heap_head_ = uint_cost;
    }
    if (uint_cost > heap_tail_) {
        heap_tail_ = uint_cost;
    }
}

t_heap* Bucket::pop() {
    auto heap_head = heap_head_;
    auto heap_tail = heap_tail_;
    t_heap** heap = heap_;

    // Check empty
    if (heap_head == std::numeric_limits<size_t>::max()) {
        return nullptr;
    }

    // Find first non-empty bucket

    // Randomly remove element
    size_t count = fast_rand() % 4;

    t_heap* prev = nullptr;
    t_heap* next = heap[heap_head];
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

    return next;
}

void Bucket::print() {
    for (size_t i = heap_head_; i < heap_tail_; ++i) {
        if (heap_[heap_head_] != nullptr) {
            VTR_LOG("B:%d ", i);
            for (auto* item = heap_[i]; item != nullptr; item = item->next_bucket) {
                VTR_LOG(" %e", item->cost);
            }
        }
    }
    VTR_LOG("\n");
}
