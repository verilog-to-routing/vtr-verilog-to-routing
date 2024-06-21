#ifndef VTR_K_ARY_HEAP_H
#define VTR_K_ARY_HEAP_H

#include "heap_type.h"
#include <vector>

class KAryHeap : public HeapInterface {
  public:
    KAryHeap();
    ~KAryHeap();

    t_heap* alloc() final;
    void free(t_heap* hptr) final;

    void init_heap(const DeviceGrid& grid) final;
    void add_to_heap(t_heap* hptr) final;
    void push_back(t_heap* const hptr) final;
    bool is_empty_heap() const final;
    void empty_heap() final;
    void build_heap() final;
    void set_prune_limit(size_t max_index, size_t prune_limit) final;
    void free_all_memory() final;

    virtual bool is_valid() const = 0;
    virtual t_heap* get_heap_head() = 0;

  protected:
    struct heap_elem {
        t_heap* elem_ptr;
        float cost;
    };

    size_t size() const;
    void sift_up(size_t leaf, heap_elem const& node);
    void expand_heap_if_full();
    bool check_prune_limit();
    void prune_heap();

    virtual void sift_down(size_t hole) = 0;

    HeapStorage storage_;

    /* heap_ is indexed from [1..heap_size] */
    std::vector<heap_elem> heap_;

    size_t heap_size_; /* Number of slots in the heap array */
    size_t heap_tail_; /* Index of first unused slot in the heap array */

    size_t max_index_;
    size_t prune_limit_;
};

#endif // VTR_K_ARY_HEAP_H
