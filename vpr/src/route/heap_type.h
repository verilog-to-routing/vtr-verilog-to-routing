#ifndef _HEAP_TYPE_H
#define _HEAP_TYPE_H

#include "physical_types.h"
#include "device_grid.h"
#include "vtr_memory.h"

/* Used by the heap as its fundamental data structure.
 * Each heap element represents a partial route.
 *
 * cost:    The cost used to sort heap.
 *          For the timing-driven router this is the backward_path_cost +
 *          expected cost to the target.
 *          For the breadth-first router it is the node cost to reach this
 *          point.
 *
 * backward_path_cost:  Used only by the timing-driven router.  The "known"
 *                      cost of the path up to and including this node.
 *                      In this case, the .cost member contains not only
 *                      the known backward cost but also an expected cost
 *                      to the target.
 *
 * R_upstream: Used only by the timing-driven router.  Stores the upstream 
 *             resistance to ground from this node, including the
 *             resistance of the node itself (device_ctx.rr_nodes[index].R).
 *
 * index: The RR node index associated with the costs/R_upstream values
 *
 * u.prev.node: The previous node used to reach the current 'index' node
 * u.prev.next: The edge from u.prev.node used to reach the current 'index' node
 *
 * u.next:  pointer to the next s_heap structure in the free
 *          linked list.  Not used when on the heap.
 *
 */
struct t_heap {
    float cost = 0.;
    float backward_path_cost = 0.;
    float R_upstream = 0.;

    int index = OPEN;

    struct t_prev {
        int node;
        int edge;
    };

    union {
        t_heap* next;
        t_prev prev;
    } u;
};

class HeapStorage {
  public:
    HeapStorage();
    t_heap* alloc();
    void free(t_heap* hptr);
    void free_all_memory();

  private:
    /* For keeping track of the sudo malloc memory for the heap*/
    vtr::t_chunk heap_ch_;

    t_heap* heap_free_head_;
    size_t num_heap_allocated_;
};

class HeapInterface {
  public:
    virtual ~HeapInterface() {}

    virtual t_heap* alloc() = 0;
    virtual void free(t_heap* hptr) = 0;

    virtual void init_heap(const DeviceGrid& grid) = 0;

    virtual void add_to_heap(t_heap* hptr) = 0;
    virtual void push_back(t_heap* const hptr) = 0;

    virtual t_heap* get_heap_head() = 0;

    virtual bool is_empty_heap() const = 0;
    virtual bool is_valid() const = 0;

    virtual void empty_heap() = 0;
    virtual void build_heap() = 0;

    virtual void invalidate_heap_entries(int sink_node, int ipin_node) = 0;

    virtual void free_all_memory() = 0;
};

#endif /* _HEAP_TYPE_H */
