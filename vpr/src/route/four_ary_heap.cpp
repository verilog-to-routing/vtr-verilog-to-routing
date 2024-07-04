#include "four_ary_heap.h"
#include "vtr_log.h"

// The leftmost/smallest-index child of node i
static inline size_t first_child(size_t i) { return (i << 2) - 2; }

inline size_t FourAryHeap::parent(size_t i) const { return (i + 2) >> 2; }

inline size_t FourAryHeap::smallest_child(size_t i) const {
    // This function could be a simple loop to find the min cost child. However,
    // using switch-case is 3% faster, which is worthwhile as this function is
    // called very frequently.

    const size_t child_1 = first_child(i);
    const size_t child_2 = child_1 + 1;
    const size_t child_3 = child_1 + 2;
    const size_t child_4 = child_1 + 3;

    size_t num_children = std::max(std::min(4, (int)heap_tail_ - (int)child_1), 0);

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
        default:
            return child_1;
    }
}

bool FourAryHeap::is_valid() const {
    if (heap_.empty()) {
        return false;
    }

    for (size_t i = 1; i <= parent(heap_tail_); ++i) {
        size_t leftmost_child = first_child(i);

        for (size_t j = 0; j < 4; ++j) {
            if (leftmost_child + j >= heap_tail_)
                break;
            else if (heap_[leftmost_child + j].cost < heap_[i].cost)
                return false;
        }
    }

    return true;
}

t_heap* FourAryHeap::get_heap_head() {
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
        child = smallest_child(hole);

        --heap_tail_;

        while (child < heap_tail_) {
            child = smallest_child(hole);

            heap_[hole] = heap_[child];
            hole = child;
            child = first_child(hole);
        }

        sift_up(hole, heap_[heap_tail_]);
    } while (!cheapest->index.is_valid()); /* Get another one if invalid entry. */

    return (cheapest);
}

// make a heap rooted at index hole by **sifting down** in O(lgn) time
void FourAryHeap::sift_down(size_t hole) {
    heap_elem head{heap_[hole]};
    size_t child{smallest_child(hole)};

    while (child < heap_tail_) {
        if (heap_[child].cost < head.cost) {
            heap_[hole] = heap_[child];
            hole = child;
            child = smallest_child(hole);
        } else
            break;
    }

    heap_[hole] = head;
}