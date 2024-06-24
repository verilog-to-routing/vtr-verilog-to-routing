#ifndef VTR_K_ARY_HEAP_H
#define VTR_K_ARY_HEAP_H

#include "heap_type.h"
#include <vector>

/**
 * @brief Abstract class whose children are HeapInterface implementations of a k-ary minheap.
 */
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
    /**
     * @brief The struct which the heap_ vector contains.
     *
     * @details
     * Previously, heap_ was made of only t_heap pointers. This meant that
     * all comparisons required dereferencing to attain the element's cost.
     * Now, the cost is attained by dereferencing only once in add_to_heap().
     * This resulted in a slightly larger memory footprint but a ~1.4% runtime
     * improvement.
     *
     * @param elem_ptr A pointer to the t_heap struct which contains all
     * the node's information.
     * @param cost The cost of the node.
     *
     * @todo
     * We are currently storing the node cost in two places (in elem_ptr->cost and cost). This might be fixed in two ways:<BR>
     * 1. Don't store the cost in t_heap.<BR>
     * 2. Instead of using pointers, use a 32-bit ID. If we do this, we can create a new 8-ary heap, which is likely to be even
     * faster as we can fit more heap_elem on one cache line (currently, we can fit 5 as heap_elem is 12 bytes), even with more
     * comparisons.
     */
    struct heap_elem {
        t_heap* elem_ptr;
        float cost;
    };

    /**
     * @return The number of elements in the heap.
     */
    size_t size() const;

    /**
     * @brief Sift node up until it satisfies minheap property.
     *
     * @details
     * O(lgn) sifting up to maintain heap property after insertion (should sift
     * own when building heap)
     *
     * @param leaf The heap leaf where node currently resides.
     * @param node The node to be sifted up.
     */
    void sift_up(size_t leaf, heap_elem const& node);

    /**
     * @brief Expands heap by 2 times if it is full.
     */
    void expand_heap_if_full();

    /**
     * @brief If the size of the heap is greater than the prune limit, prune the heap.
     *
     * @return Whether the heap was pruned.
     */
    bool check_prune_limit();

    /**
     * @brief Prune the heap.
     */
    void prune_heap();

    /**
     * @brief Make a heap rooted at index hole by **sifting down** in O(lgn) time
     *
     * @param hole
     */
    virtual void sift_down(size_t hole) = 0;

    /**
     * @param i Heap child node.
     *
     * @return Heap parent node.
     */
    virtual size_t parent(size_t i) const = 0;

    HeapStorage storage_;

    /**
     * @details
     * heap_ is indexed from [1..heap_size]; the 0th element is unused. For BinaryHeap, this simplifies
     * arithmetic in left() and parent() functions. Using a heap beginning at index 0 would simplify
     * first_child() and parent() functions in FourAryHeap, but this does not improve runtime.
     *
     * @todo
     * If an 8-ary heap is implemented, experiment with starting at index 0
     */
    std::vector<heap_elem> heap_;

    size_t heap_size_; /* Number of slots in the heap array */
    size_t heap_tail_; /* Index of first unused slot in the heap array */

    size_t max_index_;
    size_t prune_limit_;
};

#endif // VTR_K_ARY_HEAP_H
