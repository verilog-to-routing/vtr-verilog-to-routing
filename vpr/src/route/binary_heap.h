#ifndef VTR_BINARY_HEAP_H
#define VTR_BINARY_HEAP_H

#include "k_ary_heap.h"
#include <vector>

class BinaryHeap : public KAryHeap {
  public:
    bool is_valid() const final;
    t_heap* get_heap_head() final;

  private:
    void sift_down(size_t hole) final;
    size_t parent(size_t i) const final;
};

#endif //VTR_BINARY_HEAP_H
