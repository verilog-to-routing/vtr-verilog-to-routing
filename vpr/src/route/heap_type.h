#ifndef _HEAP_TYPE_H
#define _HEAP_TYPE_H

#include "physical_types.h"
#include "device_grid.h"
#include "vtr_memory.h"
#include "vtr_array_view.h"

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
        unsigned int edge;
    };

    t_heap* next_heap_item() const {
        return u.next;
    }

    void set_next_heap_item(t_heap* next) {
        u.next = next;
    }

    int prev_node() const {
        return u.prev.node;
    }

    void set_prev_node(int prev_node) {
        u.prev.node = prev_node;
    }

    RREdgeId prev_edge() const {
        return RREdgeId(u.prev.edge);
    }

    void set_prev_edge(RREdgeId edge) {
        u.prev.edge = (size_t)edge;
    }

  private:
    union {
        t_heap* next = nullptr;
        t_prev prev;
    } u;
};

// t_heap object pool, useful for implementing heaps that conform to
// HeapInterface.
class HeapStorage {
  public:
    HeapStorage();

    // Allocate a heap item.
    t_heap* alloc();

    // Free a heap item.
    void free(t_heap* hptr);
    void free_all_memory();

  private:
    /* For keeping track of the sudo malloc memory for the heap*/
    vtr::t_chunk heap_ch_;

    t_heap* heap_free_head_;
    size_t num_heap_allocated_;
};

// Interface to heap used for router optimization.
//
// Note: Objects used in instances of HeapInterface must always be allocated
// and free'd using the HeapInterface::alloc and HeapInterface::free methods
// of that instance.  Object pools are likely in use.
//
// As a general rule, any t_heap objects returned from this interface,
// **must** be HeapInterface::free'd before destroying the HeapInterface
// instance.  This ensure that no leaks are present in the users of the heap.
// Violating this assumption may result in a assertion violation.
class HeapInterface {
  public:
    virtual ~HeapInterface() {}

    // Allocate a heap item.
    //
    // This transfers ownership of the t_heap object from HeapInterface to the
    // caller.
    virtual t_heap* alloc() = 0;

    // Free a heap item.
    //
    // HeapInterface::free can be called on objects returned from either
    // HeapInterface::alloc or HeapInterface::get_heap_head.
    virtual void free(t_heap* hptr) = 0;

    // Initializes heap storage based on the size of the device.
    //
    // Note: this method **must** be invoked at least once prior to the
    // following methods being called:
    //  - add_to_heap
    //  - push_back
    //  - get_heap_head
    //  - is_empty_heap
    //  - empty_heap
    //  - build_heap
    virtual void init_heap(const DeviceGrid& grid) = 0;

    // Add t_heap to heap, preserving heap property.
    //
    // This transfers ownership of the t_heap object to HeapInterface from the
    // called.
    virtual void add_to_heap(t_heap* hptr) = 0;

    // Add t_heap to heap, however does not preserve heap property.
    //
    // This is useful if multiple t_heap's are being added in bulk.  Once
    // all t_heap's have been added, HeapInterface::build_heap can be invoked
    // to restore the heap property in an efficient way.
    //
    // This transfers ownership of the t_heap object to HeapInterface from the
    // called.
    virtual void push_back(t_heap* const hptr) = 0;

    // Restore the heap property.
    //
    // This is useful in conjunction with HeapInterface::push_back when adding
    // multiple heap elements.
    virtual void build_heap() = 0;

    // Pop the head (smallest element) of the heap, and return it.
    //
    // This transfers ownership of the t_heap object from HeapInterface to the
    // caller.
    virtual t_heap* get_heap_head() = 0;

    // Is the heap empty?
    virtual bool is_empty_heap() const = 0;

    // Is the heap valid?
    virtual bool is_valid() const = 0;

    // Empty all items from the heap.
    virtual void empty_heap() = 0;

    // marks all the heap entries consisting of sink_node, where it was
    // reached via ipin_node, as invalid (open).  used only by the
    // breadth_first router and even then only in rare circumstances.
    //
    // This function enables forcing the breadth-first router to route to a
    // sink more than once, using multiple ipins, which is useful in some
    // architectures.
    virtual void invalidate_heap_entries(int sink_node, int ipin_node) = 0;

    // Free all storage used by the heap.
    //
    // This returns all memory allocated by the HeapInterface instance. Only
    // call this if the heap is no longer being used.
    //
    // Note: Only invoke this method if all objects returned from this
    // HeapInterface instace have been free'd.
    virtual void free_all_memory() = 0;
};

#endif /* _HEAP_TYPE_H */
