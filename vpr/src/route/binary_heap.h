#ifndef _BINARY_HEAP_H
#define _BINARY_HEAP_H

#include "heap_type.h"

class BinaryHeap : public HeapInterface {
  public:
    BinaryHeap();
    ~BinaryHeap();

    t_heap* alloc() final;
    void free(t_heap* hptr) final;

    void init_heap(const DeviceGrid& grid) final;
    void add_to_heap(t_heap* hptr) final;
    void push_back(t_heap* const hptr) final;
    bool is_empty_heap() const final;
    bool is_valid() const final;
    void empty_heap() final;
    t_heap* get_heap_head() final;
    void build_heap() final;

    void invalidate_heap_entries(int sink_node, int ipin_node) final;

    void free_all_memory() final;

  private:
    size_t size() const;
    void sift_up(size_t leaf, t_heap* const node);
    void sift_down(size_t hole);
    void expand_heap_if_full();

    HeapStorage storage_;
    t_heap** heap_; /* Indexed from [1..heap_size] */
    int heap_size_; /* Number of slots in the heap array */
    int heap_tail_; /* Index of first unused slot in the heap array */
};

#endif /* _BINARY_HEAP_H */
