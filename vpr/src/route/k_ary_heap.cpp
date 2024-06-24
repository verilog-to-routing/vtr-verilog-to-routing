#include "k_ary_heap.h"
#include "rr_graph_fwd.h"
#include "vtr_log.h"

KAryHeap::KAryHeap()
    : heap_()
    , heap_size_(0)
    , heap_tail_(0)
    , max_index_(std::numeric_limits<size_t>::max())
    , prune_limit_(std::numeric_limits<size_t>::max()) {}

KAryHeap::~KAryHeap() {
    free_all_memory();
}

t_heap* KAryHeap::alloc() {
    return storage_.alloc();
}
void KAryHeap::free(t_heap* hptr) {
    storage_.free(hptr);
}

void KAryHeap::init_heap(const DeviceGrid& grid) {
    size_t target_heap_size = (grid.width() - 1) * (grid.height() - 1);
    if (heap_.empty() || heap_size_ < target_heap_size) {
        if (!heap_.empty()) {
            // coverity[offset_free : Intentional]
            heap_.clear();
        }
        heap_size_ = (grid.width() - 1) * (grid.height() - 1);
        heap_.resize(heap_size_ + 1); /* heap_size_ + 1 because heap stores from [1..heap_size] */
    }
    heap_tail_ = 1;
}

void KAryHeap::add_to_heap(t_heap* hptr) {
    expand_heap_if_full();
    // start with undefined hole
    ++heap_tail_;
    heap_elem new_elem = {hptr, hptr->cost};
    sift_up(heap_tail_ - 1, new_elem);

    // If we have pruned, rebuild the heap now.
    if (check_prune_limit()) {
        build_heap();
    }
}

bool KAryHeap::is_empty_heap() const {
    return (bool)(heap_tail_ == 1);
}

void KAryHeap::empty_heap() {
    for (size_t i = 1; i < heap_tail_; i++)
        free(heap_[i].elem_ptr);

    heap_tail_ = 1;
}

size_t KAryHeap::size() const { return heap_tail_ - 1; } // heap[0] is not valid element

// runs in O(n) time by sifting down; the least work is done on the most elements: 1 swap for bottom layer, 2 swap for 2nd, ... lgn swap for top
// 1*(n/k^1) + 2*(n/k^2) + 3*(n/k^3) + ... + lgn*1 = k*n (sum of i/k^i)
void KAryHeap::build_heap() {
    for (size_t i = parent(heap_tail_); i != 0; --i)
        sift_down(i);
}

void KAryHeap::set_prune_limit(size_t max_index, size_t prune_limit) {
    if (prune_limit != std::numeric_limits<size_t>::max()) {
        VTR_ASSERT(max_index < prune_limit);
    }
    max_index_ = max_index;
    prune_limit_ = prune_limit;
}

void KAryHeap::sift_up(size_t leaf, heap_elem const& node) {
    while ((leaf > 1) && (node.cost < heap_[parent(leaf)].cost)) {
        // sift hole up
        heap_[leaf] = heap_[parent(leaf)];
        leaf = parent(leaf);
    }

    heap_[leaf] = node;
}

void KAryHeap::expand_heap_if_full() {
    if (heap_tail_ >= heap_size_) { /* Heap is full */
        heap_size_ *= 2;
        heap_.resize(heap_size_ + 1);
    }
}

// adds an element to the back of heap and expand if necessary, but does not maintain heap property
void KAryHeap::push_back(t_heap* const hptr) {
    expand_heap_if_full();

    heap_elem new_elem = {hptr, hptr->cost};
    heap_[heap_tail_] = new_elem;
    ++heap_tail_;

    check_prune_limit();
}

void KAryHeap::free_all_memory() {
    if (!heap_.empty()) {
        empty_heap();
        // coverity[offset_free : Intentional]
        heap_.clear();
    }

    //  heap_ = nullptr; /* Defensive coding:  crash hard if I use these. */
    storage_.free_all_memory();
}

bool KAryHeap::check_prune_limit() {
    if (heap_tail_ > prune_limit_) {
        prune_heap();
        return true;
    }

    return false;
}

void KAryHeap::prune_heap() {
    VTR_ASSERT(max_index_ < prune_limit_);

    heap_elem blank_elem = {nullptr, 0.0};
    std::vector<heap_elem> best_heap_item(max_index_, blank_elem);

    // Find the cheapest instance of each index and store it.
    for (size_t i = 1; i < heap_tail_; i++) {
        if (heap_[i].elem_ptr == nullptr) {
            continue;
        }

        if (!heap_[i].elem_ptr->index.is_valid()) {
            free(heap_[i].elem_ptr);
            heap_[i].elem_ptr = nullptr;
            continue;
        }

        auto idx = size_t(heap_[i].elem_ptr->index);

        VTR_ASSERT(idx < max_index_);

        if (best_heap_item[idx].elem_ptr == nullptr || best_heap_item[idx].cost > heap_[i].cost) {
            best_heap_item[idx] = heap_[i];
        }
    }

    // Free unused nodes.
    for (size_t i = 1; i < heap_tail_; i++) {
        if (heap_[i].elem_ptr == nullptr) {
            continue;
        }

        auto idx = size_t(heap_[i].elem_ptr->index);

        if (best_heap_item[idx].elem_ptr != heap_[i].elem_ptr) {
            free(heap_[i].elem_ptr);
            heap_[i].elem_ptr = nullptr;
        }
    }

    heap_tail_ = 1;

    for (size_t i = 0; i < max_index_; ++i) {
        if (best_heap_item[i].elem_ptr != nullptr) {
            heap_[heap_tail_++] = best_heap_item[i];
        }
    }
}
