#ifndef _HEAP_TYPE_H
#define _HEAP_TYPE_H

#include <cstdint>
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
    ///@brief The delay of the partial path plus the path from route tree to source. Needed by RCV. Set to infinity if RCV is disabled
    float rcv_path_backward_delay = std::numeric_limits<float>::infinity();

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

using HeapNodePriority = float;
using HeapNodeId = RRNodeId;
// Ensure that the heap node structure occupies only 64 bits to make the heap cache-friendly
// and achieve high performance.
static_assert(sizeof(RRNodeId) == sizeof(uint32_t));

/**
 * @brief Used by the heap as its fundamental data structure. Each heap
 * node contains only the heap priority value (i.e., the cost of the RR node)
 * and the index of the RR node. The size of each heap node is minimized to
 * ensure that the heap is cache-friendly and to make the initialization and
 * copying of heap nodes efficient.
 */
struct HeapNode {
    ///@brief The priority value or cost used to sort heap. For the timing-driven router
    /// this is the total_cost (i.e., backward_path_cost + expected cost to the target).
    HeapNodePriority prio;
    ///@brief The RR node index associated with the cost.
    HeapNodeId node;
};

/**
 * @brief The comparison function object used to sort heap, following the STL style.
 */
struct HeapNodeComparator {
    bool operator()(const HeapNode& u, const HeapNode& v) {
        return u.prio > v.prio;
    }
};

/**
 * @brief Interface to heap used for router optimization.
 */
class HeapInterface {
  public:
    virtual ~HeapInterface() {}

    /**
     * @brief Initializes heap storage based on the size of the device.
     *
     * @note
     * This method **must** be invoked at least once prior to the
     * following methods being called:<BR>
     *  - try_pop<BR>
     *  - add_to_heap<BR>
     *  - push_back<BR>
     *  - build_heap<BR>
     *  - empty_heap<BR>
     *  - is_empty_heap<BR>
     *
     *  @param grid The FPGA device grid
     */
    virtual void init_heap(const DeviceGrid& grid) = 0;

    /**
     * @brief Pop the head (smallest element) of the heap. Return true if the pop
     * succeeds; otherwise (if the heap is empty), return false.
     *
     * @param heap_node The reference to a location to store the popped heap node.
     */
    virtual bool try_pop(HeapNode& heap_node) = 0;

    /**
     * @brief Add HeapNode to heap, preserving heap property.
     *
     * @param heap_node The element to add.
     */
    virtual void add_to_heap(const HeapNode& heap_node) = 0;

    /**
     * @brief Add HeapNode to heap, however does not preserve heap property.
     *
     * @param hptr The element to insert.
     */
    virtual void push_back(const HeapNode& heap_node) = 0;

    /**
     * @brief Restore the heap property.
     *
     * @details
     * This is useful in conjunction with HeapInterface::push_back when adding
     * multiple heap elements.
     */
    virtual void build_heap() = 0;

    /**
     * @brief Is the heap valid?
     */
    virtual bool is_valid() const = 0;

    /**
     * @brief Empty all items from the heap.
     */
    virtual void empty_heap() = 0;

    /**
     * @brief Is the heap empty?
     */
    virtual bool is_empty_heap() const = 0;
};

enum class e_heap_type {
    INVALID_HEAP = 0,
    BINARY_HEAP,
    FOUR_ARY_HEAP,
};

/**
 * @brief Heap factory.
 */
std::unique_ptr<HeapInterface> make_heap(e_heap_type);

#endif /* _HEAP_TYPE_H */
