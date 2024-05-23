#ifndef _BINARY_HEAP_H
#define _BINARY_HEAP_H

// Only use one of these two DARITYs at a time
#define HEAP_DARITY_2
//#define HEAP_DARITY_4

#define HEAP_USE_HEAP_ELEM
//#define HEAP_USE_MEMORY_ALIGNMENT

#if defined(HEAP_USE_MEMORY_ALIGNMENT)
#define MEMORY_ALIGNMENT_FACTOR 8

#include <boost/align/aligned_allocator.hpp>

#endif

#include "heap_type.h"
#include <vector>

class BinaryHeap : public HeapInterface {
  public:
    BinaryHeap();
    ~BinaryHeap();

#if defined(HEAP_USE_HEAP_ELEM)
    struct heap_elem {
        t_heap* elem_ptr;
        float cost;
#if defined(HEAP_USE_MEMORY_ALIGNMENT)
        } __attribute__((aligned(MEMORY_ALIGNMENT_FACTOR)));
#else
    };
#endif
#else
    struct heap_elem {
        t_heap* elem_ptr;
#if defined(HEAP_USE_MEMORY_ALIGNMENT)
    } __attribute__((aligned(MEMORY_ALIGNMENT_FACTOR)));
#else
    };
#endif
#endif

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
    void set_prune_limit(size_t max_index, size_t prune_limit) final;

    void free_all_memory() final;

  private:
    size_t size() const;

    void sift_up(size_t leaf, heap_elem const& node);
    void sift_down(size_t hole);
    void expand_heap_if_full();
    bool check_prune_limit();
    void prune_heap();

#if defined(HEAP_DARITY_4)

    size_t smallest_child(size_t i);

#endif

    HeapStorage storage_;

    /* heap_ is indexed from [1..heap_size] */
#if defined(HEAP_USE_MEMORY_ALIGNMENT)
    std::vector<heap_elem, boost::alignment::aligned_allocator<heap_elem, MEMORY_ALIGNMENT_FACTOR>> heap_;
#else
    std::vector<heap_elem> heap_;
#endif

    size_t heap_size_;          /* Number of slots in the heap array */
    size_t heap_tail_;          /* Index of first unused slot in the heap array */

    size_t max_index_;
    size_t prune_limit_;
};

#endif /* _BINARY_HEAP_H */
