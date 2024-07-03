#include "binary_heap.h"
#include "vtr_log.h"

// child indices of a heap
static inline size_t left(size_t i) { return i << 1; }
static inline size_t right(size_t i) { return (i << 1) + 1; }

inline size_t BinaryHeap::parent(size_t i) const { return i >> 1; }

bool BinaryHeap::is_valid() const {
    if (heap_.empty()) {
        return false;
    }

    for (size_t i = 1; i <= heap_tail_ >> 1; ++i) {
        if (left(i) < heap_tail_ && heap_[left(i)].cost < heap_[i].cost) return false;
        if (right(i) < heap_tail_ && heap_[right(i)].cost < heap_[i].cost) return false;
    }

    return true;
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
        child = 2;

        --heap_tail_;

        while (child < heap_tail_) {
            if (heap_[child + 1].cost < heap_[child].cost)
                ++child; // become right child

            heap_[hole] = heap_[child];
            hole = child;
            child = left(child);
        }

        sift_up(hole, heap_[heap_tail_]);
    } while (!cheapest->index.is_valid()); /* Get another one if invalid entry. */

    return (cheapest);
}

// make a heap rooted at index hole by **sifting down** in O(lgn) time
void BinaryHeap::sift_down(size_t hole) {
    heap_elem head{heap_[hole]};
    size_t child{left(hole)};

    while (child < heap_tail_) {
        if (child + 1 < heap_tail_ && heap_[child + 1].cost < heap_[child].cost)
            ++child;

        if (heap_[child].cost < head.cost) {
            heap_[hole] = heap_[child];
            hole = child;
            child = left(child);
        } else
            break;
    }

    heap_[hole] = head;
}