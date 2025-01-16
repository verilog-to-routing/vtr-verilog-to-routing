#ifndef _VTR_D_ARY_HEAP_H
#define _VTR_D_ARY_HEAP_H

#include <vector>

#include "device_grid.h"
#include "heap_type.h"
#include "d_ary_heap.tpp"

/**
 * @brief Min-heap with D child nodes per parent.
 *
 * @note
 * Currently, DAryHeap only has two children, BinaryHeap and FourAryHeap. On small circuits,
 * these heaps have negligible differences in runtime, but on larger heaps, runtime is lower when
 * using FourAryHeap. On Koios large benchmarks, the runtime is ~5% better on FourAryHeap compared
 * to BinaryHeap. This is likely because FourAryHeap has lower tree height, and as we can fit 8
 * heap node (each is 8 bytes) on a cache line (commonly 64 bytes on modern architectures), each
 * heap operation (the comparison among sibling nodes) tends to benefit from the caches.
*/
template<unsigned D>
class DAryHeap : public HeapInterface {
  public:
    using priority_queue = customized_d_ary_priority_queue<D, HeapNode, std::vector<HeapNode>, HeapNodeComparator>;

    DAryHeap() {}

    void init_heap(const DeviceGrid& grid) {
        size_t target_heap_size = (grid.width() - 1) * (grid.height() - 1);
        pq_.reserve(target_heap_size); // reserve the memory for the heap structure
    }

    bool try_pop(HeapNode& heap_node) {
        if (pq_.empty()) {
            return false;
        } else {
            heap_node = pq_.top();
            pq_.pop();
            return true;
        }
    }

    void add_to_heap(const HeapNode& heap_node) {
        pq_.push(heap_node);
    }

    void push_back(const HeapNode& heap_node) {
        pq_.push(heap_node); // FIXME: add to heap without maintaining the heap property
    }

    void build_heap() {
        // FIXME: restore the heap property after pushing back nodes
    }

    bool is_valid() const {
        return true; // FIXME: checking if the heap property is maintained or not
    }

    void empty_heap() {
        pq_.clear();
    }

    bool is_empty_heap() const {
        return (bool)(pq_.empty());
    }

  private:
    priority_queue pq_;
};

using BinaryHeap = DAryHeap<2>;
using FourAryHeap = DAryHeap<4>;

#endif /* _VTR_D_ARY_HEAP_H */
