#ifndef VTR_FOUR_ARY_HEAP_H
#define VTR_FOUR_ARY_HEAP_H

#include "k_ary_heap.h"
#include <vector>

class FourAryHeap : public KAryHeap {
  public:
    bool is_valid() const final;
    t_heap* get_heap_head() final;

  private:
    void sift_down(size_t hole) final;
    size_t parent(size_t i) const final;
    size_t smallest_child(size_t i) const;
};

#endif //VTR_FOUR_ARY_HEAP_H
