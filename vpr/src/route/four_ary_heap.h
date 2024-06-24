#ifndef VTR_FOUR_ARY_HEAP_H
#define VTR_FOUR_ARY_HEAP_H

#include "k_ary_heap.h"
#include <vector>

/**
 * @brief Minheap with 4 child nodes per parent.
 *
 * @note
 * Currently, KAryHeap's two children are BinaryHeap and FourAryHeap. On small circuits, these
 * heaps have negligible differences in runtime, but on larger heaps, runtime is lower when
 * using FourAryHeap. On titan benchmarks, the runtime is ~1.8% better on FourAryHeap compared
 * to BinaryHeap. This is likely because FourAryHeap is more cache friendly, as we can fit 5
 * heap_elem on a cache line.
*/
class FourAryHeap : public KAryHeap {
  public:
    bool is_valid() const final;
    t_heap* get_heap_head() final;

  private:
    void sift_down(size_t hole) final;
    size_t parent(size_t i) const final;

    /**
     * @param i The parent node.
     *
     * @return The child node of i with the smallest cost. Returns the first (smallest index) child of i
     * if i has no children.
     */
    size_t smallest_child(size_t i) const;
};

#endif //VTR_FOUR_ARY_HEAP_H
