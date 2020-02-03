#ifndef _RR_NODE_STORAGE_
#define _RR_NODE_STORAGE_

#include <exception>

#include "rr_node_fwd.h"
#include "rr_graph2.h"
#include "vtr_log.h"

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_* structures.                                            *
 *                                                                           *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for           *
 *       coordinate system) of the ends of this routing resource.            *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of           *
 *       length 1.  These values are used to decide whether or not this      *
 *       node should be added to the expansion heap, based on things         *
 *       like whether it's outside the net bounding box or is moving         *
 *       further away from the target, etc.                                  *
 * type:  What is this routing resource?                                     *
 * ptc_num:  Pin, track or class number, depending on rr_node type.          *
 *           Needed to properly draw.                                        *
 * cost_index: An integer index into the table of routing resource indexed   *
 *             data t_rr_index_data (this indirection allows quick dynamic   *
 *             changes of rr base costs, and some memory storage savings for *
 *             fields that have only a few distinct values).                 *
 * capacity:   Capacity of this node (number of routes that can use it).     *
 * num_edges:  Number of edges exiting this node.  That is, the number       *
 *             of nodes to which it connects.                                *
 * edges[0..num_edges-1]:  Array of indices of the neighbours of this        *
 *                         node.                                             *
 * switches[0..num_edges-1]:  Array of switch indexes for each of the        *
 *                            edges leaving this node.                       *
 *                                                                           *
 * direction: if the node represents a track, this field                     *
 *            indicates the direction of the track. Otherwise                *
 *            the value contained in the field should be                     *
 *            ignored.                                                       *
 * side: The side of a grid location where an IPIN or OPIN is located.       *
 *       This field is valid only for IPINs and OPINs and should be ignored  *
 *       otherwise.                                                          */
struct alignas(16) t_rr_node_data {
    int8_t cost_index_ = -1;
    int16_t rc_index_ = -1;

    int16_t xlow_ = -1;
    int16_t ylow_ = -1;
    int16_t xhigh_ = -1;
    int16_t yhigh_ = -1;

    t_rr_type type_ = NUM_RR_TYPES;
    union {
        e_direction direction; //Valid only for CHANX/CHANY
        e_side side;           //Valid only for IPINs/OPINs
    } dir_side_;

    uint16_t capacity_ = 0;
};

struct t_rr_node_ptc_data {
    union {
        int16_t pin_num;
        int16_t track_num;
        int16_t class_num;
    } ptc_;
};

static_assert(sizeof(t_rr_node_data) == 16, "Check t_rr_node_data size");
static_assert(alignof(t_rr_node_data) == 16, "Check t_rr_node_data size");

template<class T>
struct aligned_allocator {
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    pointer allocate(size_type n, const void* /*hint*/ = 0) {
        void* data;
        int ret = posix_memalign(&data, alignof(T), sizeof(T) * n);
        if (ret != 0) {
            throw std::bad_alloc();
        }
        return static_cast<pointer>(data);
    }

    void deallocate(T* p, size_type /*n*/) {
        free(p);
    }
};

// RR node and edge storage class.
class t_rr_graph_storage {
  public:
    t_rr_graph_storage() {
        clear();
    }

    void reserve(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        storage_.reserve(size);
        ptc_.reserve(size);
    }
    void resize(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        storage_.resize(size);
        ptc_.resize(size);
    }
    size_t size() const {
        return storage_.size();
    }
    bool empty() const {
        return storage_.empty();
    }

    void clear() {
        storage_.clear();
        ptc_.clear();
        first_edge_.clear();
        fan_in_.clear();
        edge_src_node_.clear();
        edge_dest_node_.clear();
        edge_switch_.clear();
        edges_read_ = false;
        partitioned_ = false;
        remapped_edges_ = false;
    }

    void shrink_to_fit() {
        storage_.shrink_to_fit();
        ptc_.shrink_to_fit();
        first_edge_.shrink_to_fit();
        fan_in_.shrink_to_fit();
        edge_src_node_.shrink_to_fit();
        edge_dest_node_.shrink_to_fit();
        edge_switch_.shrink_to_fit();
    }

    void emplace_back() {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        storage_.emplace_back();
        ptc_.emplace_back();
    }

    node_idx_iterator begin() const;

    node_idx_iterator end() const;

    const t_rr_node operator[](size_t idx) const;
    t_rr_node operator[](size_t idx);
    const t_rr_node at(size_t idx) const;
    t_rr_node at(size_t idx);

    const t_rr_node front() const;
    t_rr_node front();
    const t_rr_node back() const;
    t_rr_node back();

    friend class t_rr_node;

    /****************
     * Node methods *
     ****************/

    const char* node_type_string(RRNodeId id) const;
    t_rr_type node_type(RRNodeId id) const;

    /* PTC set methods */
    void set_node_ptc_num(RRNodeId id, short);
    void set_node_pin_num(RRNodeId id, short);   //Same as set_ptc_num() by checks type() is consistent
    void set_node_track_num(RRNodeId id, short); //Same as set_ptc_num() by checks type() is consistent
    void set_node_class_num(RRNodeId id, short); //Same as set_ptc_num() by checks type() is consistent

    /* PTC get methods */
    short node_ptc_num(RRNodeId id) const;
    short node_pin_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent
    short node_track_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    short node_class_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent

    /****************
     * Edge methods *
     ****************/

    // Edge initialization ordering:
    //  1. Use reserve_edges/emplace_back_edge/alloc_and_load_edges to
    //     initialize edges.  All edges must be added prior to calling any
    //     methods that read edge data.
    //
    //     Note: Either arch_switch_inf indicies or rr_switch_inf should be
    //     used with emplace_back_edge and alloc_and_load_edges.  Do not mix
    //     indicies, otherwise things will be break.
    //
    //  2. The following methods read from the edge data, and lock out the
    //     edge mutation methods (e.g. emplace_back_edge/alloc_and_load_edges):
    //       - init_fan_in
    //       - partition_edges
    //       - count_rr_switches
    //       - remap_rr_node_switch_indices
    //       - mark_edges_as_rr_switch_ids
    //
    //  3. If edge_switch values are arch_switch_inf indicies,
    //     remap_rr_node_switch_indices must be called prior to calling
    //     partition_edges.
    //
    //     If edge_switch values are rr_switch_inf indices,
    //     mark_edges_as_rr_switch_ids must be called prior to calling
    //     partition_edges.
    //
    //  4. init_fan_in can be invoked any time after edges have been
    //     initialized.
    //
    //  5. The following methods must only be called after partition_edges
    //     have been invoked:
    //      - edges
    //      - configurable_edges
    //      - non_configurable_edges
    //      - num_edges
    //      - num_configurable_edges
    //      - edge_id
    //      - edge_sink_node
    //      - edge_switch

    /* Edge mutators */

    // Reserve at least num_edges in the edge backing arrays.
    void reserve_edges(size_t num_edges);

    // Adds ones edge.  This method is efficient if reserve_edges was called with
    // the number of edges present in the graph.  This method is still
    // amortized O(1), like std::vector::emplace_back, but both runtime and
    // peak memory usage will be higher if reallocation is required.
    void emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch);

    // Adds a batch of edges.
    void alloc_and_load_edges(const t_rr_edge_info_set* rr_edges_to_create);

    /* Edge finalization methods */

    // Counts the number of rr switches needed based on fan in.
    //
    // init_fan_in does not need to be invoked before this method.
    size_t count_rr_switches(
        size_t num_arch_switches,
        t_arch_switch_inf* arch_switch_inf,
        t_arch_switch_fanin& arch_switch_fanins) const;

    // Maps arch_switch_inf indicies to rr_switch_inf indicies.
    //
    // This must be called before partition_edges if edges were created with
    // arch_switch_inf indicies.
    void remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin);

    // Marks that edge switch values are rr switch indicies.
    //
    // This must be called before partition_edges if edges were created with
    // rr_switch_inf indicies.
    void mark_edges_as_rr_switch_ids();

    // Sorts edge data such that configurable edges appears before
    // non-configurable edges.
    void partition_edges();

    // Validate that edge data is partitioned correctly.
    bool validate() const;

    /* Edge accessors
     *
     * Only call these methods after partition_edges has been invoked. */
    edge_idx_range edges(const RRNodeId& id) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id)));
    }
    edge_idx_range configurable_edges(const RRNodeId& id) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id)));
    }
    edge_idx_range non_configurable_edges(const RRNodeId& id) const {
        return vtr::make_range(edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id)), edge_idx_iterator(num_edges(id)));
    }

    t_edge_size num_edges(const RRNodeId& id) const {
        RREdgeId first_id = first_edge_[id];
        RREdgeId last_id = (&first_edge_[id])[1];
        return size_t(last_id) - size_t(first_id);
    }

    t_edge_size num_configurable_edges(const RRNodeId& id) const;
    t_edge_size num_non_configurable_edges(const RRNodeId& id) const;

    RREdgeId edge_id(const RRNodeId& id, t_edge_size iedge) const {
        RREdgeId first_edge = first_edge_[id];
        RREdgeId ret(size_t(first_edge) + iedge);
        VTR_ASSERT_SAFE(ret < (&first_edge_[id])[1]);
        return ret;
    }
    RRNodeId edge_sink_node(const RREdgeId& edge) const {
        return edge_dest_node_[edge];
    }
    RRNodeId edge_sink_node(const RRNodeId& id, t_edge_size iedge) const {
        return edge_sink_node(edge_id(id, iedge));
    }
    short edge_switch(const RREdgeId& edge) const {
        return edge_switch_[edge];
    }
    short edge_switch(const RRNodeId& id, t_edge_size iedge) const {
        return edge_switch(edge_id(id, iedge));
    }

    /******************
     * Fan-in methods *
     ******************/

    /* Init per node fan-in data.  Should only be called after all edges have
     * been allocated */
    void init_fan_in();

    /* Retrieve fan_in for RRNodeId, init_fan_in must have been called first. */
    t_edge_size fan_in(RRNodeId id) {
        return fan_in_[id];
    }

  private:
    friend struct edge_swapper;
    friend class edge_sort_iterator;
    friend class edge_compare_src_node;
    friend class edge_compare_src_node_and_configurable_first;

    t_rr_node_data& get(const RRNodeId& id) {
        return storage_[id];
    }
    const t_rr_node_data& get(const RRNodeId& id) const {
        return storage_[id];
    }

    // Take allocated edges in edge_src_node_/ edge_dest_node_ / edge_switch_
    // sort, and assign the first edge for each
    void assign_first_edges();

    // Verify that first_edge_ array correctly partitions rr edge data.
    bool verify_first_edges() const;

    vtr::vector<RRNodeId, t_rr_node_data, aligned_allocator<t_rr_node_data>> storage_;
    vtr::vector<RRNodeId, t_rr_node_ptc_data> ptc_;
    vtr::vector<RRNodeId, RREdgeId> first_edge_;
    vtr::vector<RRNodeId, t_edge_size> fan_in_;

    vtr::vector<RREdgeId, RRNodeId> edge_src_node_;
    vtr::vector<RREdgeId, RRNodeId> edge_dest_node_;
    vtr::vector<RREdgeId, short> edge_switch_;

    // Has any edges been read?
    //
    // Any method that mutates edge storage will be locked out after this
    // variable is set.
    //
    // Reading any of the following members should set this flag:
    //  - edge_src_node_
    //  - edge_dest_node_
    //  - edge_switch_
    mutable bool edges_read_;

    // Set after either remap_rr_node_switch_indices or mark_edges_as_rr_switch_ids
    // has been called.
    //
    // remap_rr_node_switch_indices converts indices to arch_switch_inf into
    // indices to rr_switch_inf.
    bool remapped_edges_;

    // Set after partition_edges has been called.
    bool partitioned_;
};

#endif /* _RR_NODE_STORAGE_ */
