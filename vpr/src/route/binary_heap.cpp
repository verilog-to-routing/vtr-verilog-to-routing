#include "binary_heap.h"
#include "rr_graph_fwd.h"
#include "vtr_log.h"

#if defined(HEAP_DARITY_2)

static inline size_t parent(size_t i) { return i >> 1; }
// child indices of a heap
static inline size_t left(size_t i) { return i << 1; }

static inline size_t right(size_t i) { return (i << 1) + 1; }

#elif defined(HEAP_DARITY_4)

static inline size_t parent(size_t i) { return (i + 2) >> 2; }

static inline size_t first_child(size_t i) { return (i << 2) - 2; }

size_t BinaryHeap::smallest_child(size_t i) {
    // Returns heap_tail_ if i has no children

    size_t child_1 = first_child(i);
    size_t child_2 = child_1 + 1;
    size_t child_3 = child_1 + 2;
    size_t child_4 = child_1 + 3;

    int num_children = (((int) heap_tail_ - (int) child_1) > 4) ? 4 : (int) heap_tail_ - (int) child_1;

#if defined(HEAP_USE_HEAP_ELEM)

    switch (num_children) {
        case 4: {
            size_t minA = (heap_[child_1].cost < heap_[child_2].cost) ? child_1 : child_2;
            size_t minB = (heap_[child_3].cost < heap_[child_4].cost) ? child_3 : child_4;
            return (heap_[minA].cost < heap_[minB].cost) ? minA : minB;
        }
        case 3: {
            size_t minA = (heap_[child_1].cost < heap_[child_2].cost) ? child_1 : child_2;
            return (heap_[minA].cost < heap_[child_3].cost) ? minA : child_3;
        }
        case 2:
            return (heap_[child_1].cost < heap_[child_2].cost) ? child_1 : child_2;
        case 1:
            return child_1;
        default:
            return heap_tail_;
    }

#else

    switch (num_children) {
        case 4: {
            size_t minA = (heap_[child_1].elem_ptr->cost < heap_[child_2].elem_ptr->cost) ? child_1 : child_2;
            size_t minB = (heap_[child_3].elem_ptr->cost < heap_[child_4].elem_ptr->cost) ? child_3 : child_4;
            return (heap_[minA].elem_ptr->cost < heap_[minB].elem_ptr->cost) ? minA : minB;
        }
        case 3: {
            size_t minA = (heap_[child_1].elem_ptr->cost < heap_[child_2].elem_ptr->cost) ? child_1 : child_2;
            return (heap_[minA].elem_ptr->cost < heap_[child_3].elem_ptr->cost) ? minA : child_3;
        }
        case 2:
            return (heap_[child_1].elem_ptr->cost < heap_[child_2].elem_ptr->cost) ? child_1 : child_2;
        case 1:
            return child_1;
        default:
            return heap_tail_;
    }

#endif

}

#endif

BinaryHeap::BinaryHeap()
    : heap_()
    , heap_size_(0)
    , heap_tail_(0)
    , max_index_(std::numeric_limits<size_t>::max())
    , prune_limit_(std::numeric_limits<size_t>::max()) {}

BinaryHeap::~BinaryHeap() {
    free_all_memory();
}

t_heap* BinaryHeap::alloc() {
    return storage_.alloc();
}
void BinaryHeap::free(t_heap* hptr) {
    storage_.free(hptr);
}

void BinaryHeap::init_heap(const DeviceGrid& grid) {
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

void BinaryHeap::add_to_heap(t_heap* hptr) {
    expand_heap_if_full();
    // start with undefined hole
    ++heap_tail_;

#if defined(HEAP_USE_HEAP_ELEM)
    heap_elem new_elem = {hptr, hptr->cost};
#else
    heap_elem new_elem = {hptr};
#endif

    sift_up(heap_tail_ - 1, new_elem);

    // If we have pruned, rebuild the heap now.
    if (check_prune_limit()) {
        build_heap();
    }
}

bool BinaryHeap::is_empty_heap() const {
    return (bool)(heap_tail_ == 1);
}

t_heap* BinaryHeap::get_heap_head() {
    /* Returns a pointer to the smallest element on the heap, or NULL if the     *
     * heap is empty.  Invalid (index == OPEN) entries on the heap are never     *
     * returned -- they are just skipped over.                                   */

    t_heap* cheapest;
    size_t hole, child;

    do {
        if (heap_tail_ == 1) { /* Empty heap. */
            VTR_LOG_WARN("Empty heap occurred in get_heap_head.\n");
            return (nullptr);
        }

        cheapest = heap_[1].elem_ptr;

        hole = 1;

#if defined(HEAP_DARITY_2)
        child = 2;
#elif defined(HEAP_DARITY_4)
        child = smallest_child(hole);
#endif

        --heap_tail_;


        while (child < heap_tail_) {

#if defined(HEAP_DARITY_2)
#if defined(HEAP_USE_HEAP_ELEM)
            if (heap_[child + 1].cost < heap_[child].cost)
                ++child; // become right child
#else
            if (heap_[child + 1].elem_ptr->cost < heap_[child].elem_ptr->cost)
                ++child; // become right child
#endif
#endif

            heap_[hole] = heap_[child];
            hole = child;

#if defined(HEAP_DARITY_2)
            child = left(child);
#elif defined(HEAP_DARITY_4)
            child = smallest_child(child);
#endif

        }

        sift_up(hole, heap_[heap_tail_]);
    } while (!cheapest->index.is_valid()); /* Get another one if invalid entry. */

    return (cheapest);
}

void BinaryHeap::empty_heap() {
    for (size_t i = 1; i < heap_tail_; i++)
        free(heap_[i].elem_ptr);

    heap_tail_ = 1;
}

size_t BinaryHeap::size() const { return heap_tail_ - 1; } // heap[0] is not valid element

// make a heap rooted at index hole by **sifting down** in O(lgn) time
void BinaryHeap::sift_down(size_t hole) {
    heap_elem head{heap_[hole]};

#if defined(HEAP_DARITY_2)
    size_t child{left(hole)};
#else
    size_t child{smallest_child(hole)};
#endif

    while (child < heap_tail_) {

#if defined(HEAP_DARITY_2)
#if defined(HEAP_USE_HEAP_ELEM)
        if (child + 1 < heap_tail_ && heap_[child + 1].cost < heap_[child].cost)
            ++child;
#else
        if (child + 1 < heap_tail_ && heap_[child + 1].elem_ptr->cost < heap_[child].elem_ptr->cost)
            ++child;
#endif
#endif

#if defined(HEAP_USE_HEAP_ELEM)
        if (heap_[child].cost < head.cost) {
#else
            if (heap_[child].elem_ptr->cost < head.elem_ptr->cost) {
#endif
            heap_[hole] = heap_[child];
            hole = child;

#if defined(HEAP_DARITY_2)
            child = left(child);
#else
            child = smallest_child(child);
#endif

        } else
            break;
    }

    heap_[hole] = head;
}

// runs in O(n) time by sifting down; the least work is done on the most elements: 1 swap for bottom layer, 2 swap for 2nd, ... lgn swap for top
// 1*(n/2) + 2*(n/4) + 3*(n/8) + ... + lgn*1 = 2n (sum of i/2^i)
void BinaryHeap::build_heap() {
    // second half of heap are leaves
    for (size_t i = parent(heap_tail_); i != 0; --i)
        sift_down(i);
}

void BinaryHeap::set_prune_limit(size_t max_index, size_t prune_limit) {
    if (prune_limit != std::numeric_limits<size_t>::max()) {
        VTR_ASSERT(max_index < prune_limit);
    }
    max_index_ = max_index;
    prune_limit_ = prune_limit;
}

// O(lgn) sifting up to maintain heap property after insertion (should sift down when building heap)
void BinaryHeap::sift_up(size_t leaf, heap_elem const& node) {

#if defined(HEAP_USE_HEAP_ELEM)
    while ((leaf > 1) && (node.cost < heap_[parent(leaf)].cost)) {
#else
        while ((leaf > 1) && (node.elem_ptr->cost < heap_[parent(leaf)].elem_ptr->cost)) {
#endif

        // sift hole up
        heap_[leaf] = heap_[parent(leaf)];
        leaf = parent(leaf);
    }

    heap_[leaf] = node;
}

//expands heap by "realloc"
void BinaryHeap::expand_heap_if_full() {
    if (heap_tail_ > heap_size_) { /* Heap is full */
        heap_size_ *= 2;
        heap_.resize(heap_size_ + 1);
    }
}

// adds an element to the back of heap and expand if necessary, but does not maintain heap property
void BinaryHeap::push_back(t_heap* const hptr) {
    expand_heap_if_full();

#if defined(HEAP_USE_HEAP_ELEM)
    heap_elem new_elem = {hptr, hptr->cost};
#else
    heap_elem new_elem = {hptr};
#endif

    heap_[heap_tail_] = new_elem;
    ++heap_tail_;

    check_prune_limit();
}

bool BinaryHeap::is_valid() const {
    if (heap_.empty()) {
        return false;
    }

#if defined(HEAP_DARITY_2)

    for (size_t i = 1; i <= heap_tail_ >> 1; ++i) {
#if defined(HEAP_USE_HEAP_ELEM)
        if (left(i) < heap_tail_ && heap_[left(i)].cost < heap_[i].cost) return false;
        if (right(i) < heap_tail_ && heap_[right(i)].cost < heap_[i].cost) return false;
#else
        if (left(i) < heap_tail_ && heap_[left(i)].elem_ptr->cost < heap_[i].elem_ptr->cost) return false;
        if (right(i) < heap_tail_ && heap_[right(i)].elem_ptr->cost < heap_[i].elem_ptr->cost) return false;
#endif
    }

#elif defined(HEAP_DARITY_4)

    for (size_t i = 1; i <= parent(heap_tail_); ++i) {
        size_t leftmost_child = first_child(i);

        for (size_t j = 0; j < 4; ++j) {
            if (leftmost_child + j >= heap_tail_)
                break;

#if defined(HEAP_USE_HEAP_ELEM)
                else if (heap_[leftmost_child + j].cost < heap_[i].cost)
                    return false;
#else
            else if (heap_[leftmost_child + j].elem_ptr->cost < heap_[i].elem_ptr->cost)
                return false;
#endif
        }
    }

#endif

    return true;
}

void BinaryHeap::free_all_memory() {
    if (!heap_.empty()) {
        empty_heap();

        // coverity[offset_free : Intentional]
        heap_.clear();
    }

    //  heap_ = nullptr; /* Defensive coding:  crash hard if I use these. */

    storage_.free_all_memory();
}

bool BinaryHeap::check_prune_limit() {
    if (heap_tail_ > prune_limit_) {
        prune_heap();
        return true;
    }

    return false;
}

void BinaryHeap::prune_heap() {
    VTR_ASSERT(max_index_ < prune_limit_);

#if defined(HEAP_USE_HEAP_ELEM)
    heap_elem blank_elem = {nullptr, 0.0};
#else
    heap_elem blank_elem = {nullptr};
#endif

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

#if defined(HEAP_USE_HEAP_ELEM)
        if (best_heap_item[idx].elem_ptr == nullptr || best_heap_item[idx].cost > heap_[i].cost) {
#else
            if (best_heap_item[idx].elem_ptr == nullptr || best_heap_item[idx].elem_ptr->cost > heap_[i].elem_ptr->cost) {
#endif
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
