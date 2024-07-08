#ifndef _HEAP_TYPE_H
#define _HEAP_TYPE_H

#include "physical_types.h"
#include "device_grid.h"
#include "vtr_memory.h"
#include "vtr_array_view.h"
#include "rr_graph_fwd.h"
#include "route_path_manager.h"

/**
 * @brief Used by the heap as its fundamental data structure. Each heap
 * element represents a partial route.
 */
struct t_heap {
    ///@brief The cost used to sort heap. For the timing-driven router this is the backward_path_cost + expected cost to the target.
    float cost = 0.;
    ///@brief The "known" cost of the path up to and including this node. Used only by the timing-driven router. In this case, the
    ///.cost member contains not only the known backward cost but also an expected cost to the target.
    float backward_path_cost = 0.;
    ///@brief Used only by the timing-driven router. Stores the upstream resistance to ground from this node, including the resistance
    /// of the node itself (device_ctx.rr_nodes[index].R).
    float R_upstream = 0.;
    ///@brief The RR node index associated with the costs/R_upstream values.
    RRNodeId index = RRNodeId::INVALID();
    ///@brief Structure to handle extra RCV structures. Managed by PathManager class.
    t_heap_path* path_data;

    /**
     * @brief Get the next t_heap item in the linked list.
     */
    t_heap* next_heap_item() const {
        return u.next;
    }

    /**
     * @brief Set the next t_heap item in the linked list.
     */
    void set_next_heap_item(t_heap* next) {
        u.next = next;
    }

    /**
     * @brief Get the edge from the previous node used to reach the current node.
     *
     * @note
     * Be careful: will return 0 (a valid id!) if uninitialized.
     */
    constexpr RREdgeId prev_edge() const {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        return RREdgeId(u.prev_edge);
    }

    /**
     * @brief Set the edge from the previous node used to reach the current node..
     */
    inline void set_prev_edge(RREdgeId edge) {
        static_assert(sizeof(uint32_t) == sizeof(RREdgeId));
        u.prev_edge = size_t(edge);
    }

  private:
    union {
        ///@brief Pointer to the next t_heap structure in the free linked list.
        t_heap* next = nullptr;

        /**
         * @brief The edge from the previous node used to reach the current. Not used when on the heap.
         *
         * @note
         * The previous edge is not a StrongId for performance & brevity
         * reasons: StrongIds can't be trivially placed into an anonymous
         * union.
         */
        uint32_t prev_edge;
    } u;
};

/**
 * @brief t_heap object pool, useful for implementing heaps that conform to
 * HeapInterface.
 */
class HeapStorage {
  public:
    HeapStorage();

    /**
     * @brief Allocate a heap item.
     *
     * @return The allocated item.
     */
    t_heap* alloc();

    /**
     * @brief Free a heap item.
     */
    void free(t_heap* hptr);

    /**
     * @brief Free all heap items.
     */
    void free_all_memory();

  private:
    /* For keeping track of the sudo malloc memory for the heap*/
    vtr::t_chunk heap_ch_;

    t_heap* heap_free_head_;
    size_t num_heap_allocated_;
};

/**
 * @brief Interface to heap used for router optimization.
 *
 * @note
 * Objects used in instances of HeapInterface must always be allocated
 * and free'd using the HeapInterface::alloc and HeapInterface::free methods
 * of that instance. Object pools are likely in use.
 *
 * @details
 * As a general rule, any t_heap objects returned from this interface,
 * **must** be HeapInterface::free'd before destroying the HeapInterface
 * instance. This ensure that no leaks are present in the users of the heap.
 * Violating this assumption may result in an assertion violation.
 */
class HeapInterface {
  public:
    virtual ~HeapInterface() {}

    /**
     * @brief Allocate a heap item.
     *
     * @details
     * This transfers ownership of the t_heap object from HeapInterface to the
     * caller.
     */
    virtual t_heap* alloc() = 0;

    /**
     * @brief Free a heap item.
     *
     * @details
     * HeapInterface::free can be called on objects returned from either
     * HeapInterface::alloc or HeapInterface::get_heap_head.
     *
     * @param hptr The element to free.
     */
    virtual void free(t_heap* hptr) = 0;

    /**
     * @brief Initializes heap storage based on the size of the device.
     *
     * @note
     * This method **must** be invoked at least once prior to the
     * following methods being called:<BR>
     *  - add_to_heap<BR>
     *  - push_back<BR>
     *  - get_heap_head<BR>
     *  - is_empty_heap<BR>
     *  - empty_heap<BR>
     *  - build_heap<BR>
     *
     *  @param grid The FPGA device grid
     */
    virtual void init_heap(const DeviceGrid& grid) = 0;

    /**
     * @brief Add t_heap to heap, preserving heap property.
     *
     * @details
     * This transfers ownership of the t_heap object to HeapInterface from the
     * called.
     *
     * @param hptr The element to add.
     */
    virtual void add_to_heap(t_heap* hptr) = 0;

    /**
     * @brief Add t_heap to heap, however does not preserve heap property.
     *
     * @details
     * This is useful if multiple t_heap's are being added in bulk. Once
     * all t_heap's have been added, HeapInterface::build_heap can be invoked
     * to restore the heap property in an efficient way.<BR><BR>
     * This transfers ownership of the t_heap object to HeapInterface from the
     * called.
     *
     * @param hptr The element to insert.
     */
    virtual void push_back(t_heap* const hptr) = 0;

    /**
     * @brief Restore the heap property.
     *
     * @details
     * This is useful in conjunction with HeapInterface::push_back when adding
     * multiple heap elements.
     */
    virtual void build_heap() = 0;

    /**
     * @brief Pop the head (smallest element) of the heap, and return it.
     *
     * @details
     * This transfers ownership of the t_heap object from HeapInterface to the
     * caller.
     */
    virtual t_heap* get_heap_head() = 0;

    /**
     * @brief Is the heap empty?
     */
    virtual bool is_empty_heap() const = 0;

    /**
     * @brief Is the heap valid?
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Empty all items from the heap.
     */
    virtual void empty_heap() = 0;

    /**
     * @brief Free all storage used by the heap.
     *
     * @details
     * This returns all memory allocated by the HeapInterface instance. Only
     * call this if the heap is no longer being used.
     *
     * @note
     * Only invoke this method if all objects returned from this
     * HeapInterface instance have been free'd.
     */
    virtual void free_all_memory() = 0;

    /**
     * @brief Set maximum number of elements that the heap should contain
     * (the prune_limit). If the prune limit is hit, then the heap should
     * kick out duplicate index entries.
     *
     * @details
     * The prune limit exists to provide a maximum bound on memory usage in
     * the heap. In some pathological cases, the router may explore
     * incrementally better paths, resulting in many duplicate entries for
     * RR nodes. To handle this edge case, if the number of heap items
     * exceeds the prune_limit, then the heap will compacts itself.<BR><BR>
     * The heap compaction process simply means taking the lowest cost entry
     * for each index (e.g. RR node).  All nodes with higher costs can safely
     * be dropped.<BR><BR>
     * The pruning process is intended to bound the memory usage the heap can
     * consume based on the prune_limit, which is expected to be a function of
     * the graph size.
     *
     * @param max_index The highest index possible in the heap.
     * @param prune_limit The maximum number of heap entries before pruning should
     * take place. This should always be higher than max_index, likely by a
     * significant amount. The pruning process has some overhead, so prune_limit
     * should be ~2-4x the max_index to prevent excess pruning when not required.
     */
    virtual void set_prune_limit(size_t max_index, size_t prune_limit) = 0;
};

enum class e_heap_type {
    INVALID_HEAP = 0,
    BINARY_HEAP,
    FOUR_ARY_HEAP,
    BUCKET_HEAP_APPROXIMATION,
};

/**
 * @brief Heap factory.
 */
std::unique_ptr<HeapInterface> make_heap(e_heap_type);

#endif /* _HEAP_TYPE_H */
