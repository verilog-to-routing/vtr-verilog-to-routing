#ifndef RR_GRAPH_STORAGE
#define RR_GRAPH_STORAGE

#include <exception>
#include <bitset>

#include "vtr_vector.h"
#include "physical_types.h"
#include "rr_node_types.h"
#include "rr_graph_fwd.h"
#include "rr_node_fwd.h"
#include "rr_edge.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_strong_id_range.h"
#include "vtr_array_view.h"
#include "rr_graph_utils.h"

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_* structures.                                            *
 *                                                                           *
 * This structure should **only** contain data used in the inner loop of the *
 * router.  This data is consider "hot" and the router performance is        *
 * sensitive to layout and size of this "hot" data.
 *
 * Cold data should be stored seperately in t_rr_graph_storage.              *
 *                                                                           *
 * xlow, xhigh, ylow, yhigh:  Integer coordinates (see route.c for           *
 *       coordinate system) of the ends of this routing resource.            *
 *       xlow = xhigh and ylow = yhigh for pins or for segments of           *
 *       length 1.  These values are used to decide whether or not this      *
 *       node should be added to the expansion heap, based on things         *
 *       like whether it's outside the net bounding box or is moving         *
 *       further away from the target, etc.                                  *
 * type:  What is this routing resource?                                     *
 * cost_index: An integer index into the table of routing resource indexed   *
 *             data t_rr_index_data (this indirection allows quick dynamic   *
 *             changes of rr base costs, and some memory storage savings for *
 *             fields that have only a few distinct values).                 *
 * rc_index: An integer index into a deduplicated table of R and C values.
 *           For example, two nodes that have identifical R/C values will
 *           have the same rc_index.
 * capacity:   Capacity of this node (number of routes that can use it).     *
 *                                                                           *
 * direction: if the node represents a track, this field                     *
 *            indicates the direction of the track. Otherwise                *
 *            the value contained in the field should be                     *
 *            ignored.                                                       *
 * side: The side of a grid location where an IPIN or OPIN is located.       *
 *       This field is valid only for IPINs and OPINs and should be ignored  *
 *       otherwise.                                                          */
struct alignas(16) t_rr_node_data {
    int16_t cost_index_ = -1;
    int16_t rc_index_ = -1;

    int16_t xlow_ = -1;
    int16_t ylow_ = -1;
    int16_t xhigh_ = -1;
    int16_t yhigh_ = -1;

    t_rr_type type_ = NUM_RR_TYPES;

    /* The character is a hex number which is a 4-bit truth table for node sides
     * The 4-bits in serial represent 4 sides on which a node could appear 
     * It follows a fixed sequence, which is (LEFT, BOTTOM, RIGHT, TOP) whose indices are (3, 2, 1, 0) 
     *   - When a node appears on a given side, it is set to "1"
     *   - When a node does not appear on a given side, it is set to "0"
     * For example,
     *   - '1' means '0001' in hex number, which means the node appears on TOP 
     *   - 'A' means '1100' in hex number, which means the node appears on LEFT and BOTTOM sides, 
     */
    union {
        Direction direction;       //Valid only for CHANX/CHANY
        unsigned char sides = 0x0; //Valid only for IPINs/OPINs
    } dir_side_;

    uint16_t capacity_ = 0;
};

// t_rr_node_data is a key data structure, so fail at compile time if the
// structure gets bigger than expected (16 bytes right now). Developers
// should only expand it after careful consideration and measurement.
static_assert(sizeof(t_rr_node_data) == 16, "Check t_rr_node_data size");
static_assert(alignof(t_rr_node_data) == 16, "Check t_rr_node_data size");

/* t_rr_node_ptc_data is cold data is therefore kept seperate from
 * t_rr_node_data.
 *
 * ptc_num:  Pin, track or class number, depending on rr_node type.          *
 *           Needed to properly draw.                                        */
struct t_rr_node_ptc_data {
    union {
        int16_t pin_num;
        int16_t track_num;
        int16_t class_num;
    } ptc_;
};

class t_rr_graph_view;

// RR node and edge storage class.
//
// Description:
//
// This class stores the detailed routing graph.  Each node within the graph is
// identified by a RRNodeId.  Each edge within the graph is identified by a
// RREdgeId.
//
// Each node contains data about the node itself, for example look at the
// comment t_rr_node_data. Each node also has a set of RREdgeId's that all have
// RRNodeId as the source node.
//
// Each edge is defined by the source node, destination node, and the switch
// index that connects the source to the destination node.
//
// NOTE: The switch index represents either an index into arch_switch_inf
// **or** rr_switch_inf.  During rr graph construction, the switch index is
// always is an index into arch_switch_inf.  Once the graph is completed, the
// RR graph construction code coverts all arch_switch_inf indicies
// into rr_switch_inf indicies via the remap_rr_node_switch_indices method.
//
// Usage notes and assumptions:
//
// This class broadly speak is used by two types of code:
//  - Code that writes to the rr graph
//  - Code that reads from the rr graph
//
// Within VPR, there are two locations that the rr graph is expected to be
// modified, either:
//  - During the building of the rr graph in rr_graph.cpp
//  - During the reading of a static rr graph from a file in rr_graph_reader
//  / rr_graph_uxsdcxx_serializer.
//
// It is expected and assume that once the graph is completed, the graph is
// fixed until the entire graph is cleared.  This object enforces this
// assumption with state flags.  In particular RR graph edges are assumed
// to be write only during construction of the RR graph, and read only
// otherwise.  See the description of the "Edge methods" for details.
//
// Broadly speaking there are two sets of methods.  Methods for reading and
// writing RR nodes, and methods for reading and writing RR edges. The node
// methods can be found underneath the header "Node methods" and the edge
// methods can be found underneath the header "Edge methods".
//
class t_rr_graph_storage {
  public:
    t_rr_graph_storage() {
        clear();
    }

    // Prefer to use t_rr_graph_view over directly accessing
    // t_rr_graph_storage, unless mutating RR graph.
    //
    // t_rr_graph_view is a value object, and should be passed around by value
    // and stored.
    t_rr_graph_view view() const;

    /****************
     * Node methods *
     ****************/

    t_rr_type node_type(RRNodeId id) const {
        return node_storage_[id].type_;
    }
    const char* node_type_string(RRNodeId id) const;

    int16_t node_rc_index(RRNodeId id) const {
        return node_storage_[id].rc_index_;
    }

    short node_xlow(RRNodeId id) const {
        return node_storage_[id].xlow_;
    }
    short node_ylow(RRNodeId id) const {
        return node_storage_[id].ylow_;
    }
    short node_xhigh(RRNodeId id) const {
        return node_storage_[id].xhigh_;
    }
    short node_yhigh(RRNodeId id) const {
        return node_storage_[id].yhigh_;
    }

    short node_capacity(RRNodeId id) const {
        return node_storage_[id].capacity_;
    }
    RRIndexedDataId node_cost_index(RRNodeId id) const {
        return RRIndexedDataId(node_storage_[id].cost_index_);
    }

    Direction node_direction(RRNodeId id) const {
        return get_node_direction(
            vtr::array_view_id<RRNodeId, const t_rr_node_data>(
                node_storage_.data(), node_storage_.size()),
            id);
    }
    const std::string& node_direction_string(RRNodeId id) const;

    /* Find if the given node appears on a specific side */
    bool is_node_on_specific_side(RRNodeId id, e_side side) const {
        return is_node_on_specific_side(
            vtr::array_view_id<RRNodeId, const t_rr_node_data>(
                node_storage_.data(), node_storage_.size()),
            id, side);
    }

    /* FIXME: This function should be DEPRECATED!
     * Developers can easily use the following codes with more flexibility
     *
     *   if (rr_graph.is_node_on_specific_side(id, side)) {
     *       const char* side_string = SIDE_STRING[side];
     *   }
     */
    const char* node_side_string(RRNodeId id) const;

    /* PTC get methods */
    short node_ptc_num(RRNodeId id) const;
    short node_pin_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent
    short node_track_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    short node_class_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent

    /* Retrieve fan_in for RRNodeId, init_fan_in must have been called first. */
    t_edge_size fan_in(RRNodeId id) const {
        return node_fan_in_[id];
    }

    // This prefetechs hot RR node data required for optimization.
    //
    // Note: This is optional, but may lower time spent on memory stalls in
    // some circumstances.
    inline void prefetch_node(RRNodeId id) const {
        VTR_PREFETCH(&node_storage_[id], 0, 0);
    }

    /* Edge accessors
     *
     * Preferred access methods:
     * - first_edge(RRNodeId)
     * - last_edge(RRNodeId)
     * - edge_range(RRNodeId)
     * - edge_sink_node(RREdgeId)
     * - edge_switch(RREdgeId)
     *
     * Legacy/deprecated access methods:
     * - edges(RRNodeId)
     * - configurable_edges(RRNodeId)
     * - non_configurable_edges(RRNodeId)
     * - num_edges(RRNodeId)
     * - num_configurable_edges(RRNodeId)
     * - num_non_configurable_edges(RRNodeId)
     * - edge_id(RRNodeId, t_edge_size)
     * - edge_sink_node(RRNodeId, t_edge_size)
     * - edge_switch(RRNodeId, t_edge_size)
     *
     * Only call these methods after partition_edges has been invoked. */
    edge_idx_range edges(const RRNodeId& id) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id)));
    }

    edge_idx_range configurable_edges(const RRNodeId& id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id, rr_switches)));
    }
    edge_idx_range non_configurable_edges(const RRNodeId& id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
        return vtr::make_range(edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id, rr_switches)), edge_idx_iterator(num_edges(id)));
    }

    t_edge_size num_edges(const RRNodeId& id) const {
        return size_t(last_edge(id)) - size_t(first_edge(id));
    }
    bool edge_is_configurable(RRNodeId id, t_edge_size iedge, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    t_edge_size num_configurable_edges(RRNodeId node, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    t_edge_size num_non_configurable_edges(RRNodeId node, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;

    // Get the first and last RREdgeId for the specified RRNodeId.
    //
    // The edges belonging to RRNodeId is [first_edge, last_edge), excluding
    // last_edge.
    //
    // If first_edge == last_edge, then a RRNodeId has no edges.
    RREdgeId first_edge(const RRNodeId& id) const {
        return node_first_edge_[id];
    }

    // Return the first_edge of the next rr_node, which is one past the edge
    // id range for the node we care about.
    //
    // This implies we have one dummy rr_node at the end of first_edge_, and
    // we always allocate that dummy node. We also assume that the edges have
    // been sorted by rr_node, which is true after partition_edges().
    RREdgeId last_edge(const RRNodeId& id) const {
        return (&node_first_edge_[id])[1];
    }

    // Returns a range of RREdgeId's belonging to RRNodeId id.
    //
    // If this range is empty, then RRNodeId id has no edges.
    vtr::StrongIdRange<RREdgeId> edge_range(const RRNodeId id) const {
        return vtr::StrongIdRange<RREdgeId>(first_edge(id), last_edge(id));
    }

    // Retrieve the RREdgeId for iedge'th edge in RRNodeId.
    //
    // This method should generally not be used, and instead first_edge and
    // last_edge should be used.
    RREdgeId edge_id(const RRNodeId& id, t_edge_size iedge) const {
        RREdgeId first_edge = this->first_edge(id);
        RREdgeId ret(size_t(first_edge) + iedge);
        VTR_ASSERT_SAFE(ret < last_edge(id));
        return ret;
    }

    // Get the destination node for the specified edge.
    RRNodeId edge_sink_node(const RREdgeId& edge) const {
        return edge_dest_node_[edge];
    }

    // Call the `apply` function with the edge id, source, and sink nodes of every edge.
    void for_each_edge(std::function<void(RREdgeId, RRNodeId, RRNodeId)> apply) const {
        for (size_t i = 0; i < edge_dest_node_.size(); i++) {
            RREdgeId edge(i);
            apply(edge, edge_src_node_[edge], edge_dest_node_[edge]);
        }
    }

    // Get the destination node for the iedge'th edge from specified RRNodeId.
    //
    // This method should generally not be used, and instead first_edge and
    // last_edge should be used.
    RRNodeId edge_sink_node(const RRNodeId& id, t_edge_size iedge) const {
        return edge_sink_node(edge_id(id, iedge));
    }

    // Get the switch used for the specified edge.
    short edge_switch(const RREdgeId& edge) const {
        return edge_switch_[edge];
    }

    // Get the switch used for the iedge'th edge from specified RRNodeId.
    //
    // This method should generally not be used, and instead first_edge and
    // last_edge should be used.
    short edge_switch(const RRNodeId& id, t_edge_size iedge) const {
        return edge_switch(edge_id(id, iedge));
    }

    /*
     * Node proxy methods
     *
     * The following methods implement an interface that appears to be
     * equivalent to the interface exposed by std::vector<t_rr_node>.
     * This was done for backwards compability. See t_rr_node for more details.
     *
     * Proxy methods:
     *
     * - begin()
     * - end()
     * - operator[]
     * - at()
     * - front
     * - back
     *
     * These methods should not be used by new VPR code, and instead access
     * methods that use RRNodeId and RREdgeId should be used.
     *
     **********************/

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

    /***************************
     * Node allocation methods *
     ***************************/

    // Makes room in storage for RRNodeId in amoritized O(1) fashion.
    //
    // This results in an allocation pattern similiar to what would happen
    // if push_back(x) / emplace_back() were used if underlying storage
    // was not preallocated.
    void make_room_for_node(RRNodeId elem_position) {
        make_room_in_vector(&node_storage_, size_t(elem_position));
        node_ptc_.reserve(node_storage_.capacity());
        node_ptc_.resize(node_storage_.size());
    }

    // Reserve storage for RR nodes.
    void reserve(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.reserve(size);
        node_ptc_.reserve(size);
    }

    // Resize node storage to accomidate size RR nodes.
    void resize(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.resize(size);
        node_ptc_.resize(size);
    }

    // Number of RR nodes that can be accessed.
    size_t size() const {
        return node_storage_.size();
    }

    // Is the RR graph currently empty?
    bool empty() const {
        return node_storage_.empty();
    }

    // Remove all nodes and edges from the RR graph.
    //
    // This method re-enables graph mutation if the graph was read-only.
    void clear() {
        node_storage_.clear();
        node_ptc_.clear();
        node_first_edge_.clear();
        node_fan_in_.clear();
        edge_src_node_.clear();
        edge_dest_node_.clear();
        edge_switch_.clear();
        edges_read_ = false;
        partitioned_ = false;
        remapped_edges_ = false;
    }

    // Shrink memory usage of the RR graph storage.
    //
    // Note that this will temporarily increase the amount of storage required
    // to allocate the small array and copy the data.
    void shrink_to_fit() {
        node_storage_.shrink_to_fit();
        node_ptc_.shrink_to_fit();
        node_first_edge_.shrink_to_fit();
        node_fan_in_.shrink_to_fit();
        edge_src_node_.shrink_to_fit();
        edge_dest_node_.shrink_to_fit();
        edge_switch_.shrink_to_fit();
    }

    // Append 1 more RR node to the RR graph.
    void emplace_back() {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.emplace_back();
        node_ptc_.emplace_back();
    }

    // Given `order`, a vector mapping each RRNodeId to a new one (old -> new),
    // and `inverse_order`, its inverse (new -> old), update the t_rr_graph_storage
    // data structure to an isomorphic graph using the new RRNodeId's.
    // NOTE: Re-ordering will invalidate any external references, so this
    //       should generally be called before creating such references.
    void reorder(const vtr::vector<RRNodeId, RRNodeId>& order,
                 const vtr::vector<RRNodeId, RRNodeId>& inverse_order);

    /* PTC set methods */
    void set_node_ptc_num(RRNodeId id, short);
    void set_node_pin_num(RRNodeId id, short);   //Same as set_ptc_num() by checks type() is consistent
    void set_node_track_num(RRNodeId id, short); //Same as set_ptc_num() by checks type() is consistent
    void set_node_class_num(RRNodeId id, short); //Same as set_ptc_num() by checks type() is consistent

    void set_node_type(RRNodeId id, t_rr_type new_type);
    void set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2);
    void set_node_cost_index(RRNodeId, RRIndexedDataId new_cost_index);
    void set_node_rc_index(RRNodeId, NodeRCIndex new_rc_index);
    void set_node_capacity(RRNodeId, short new_capacity);
    void set_node_direction(RRNodeId, Direction new_direction);

    /* Add a side to the node abbributes
     * This is the function to use when you just add a new side WITHOUT reseting side attributes
     */
    void add_node_side(RRNodeId, e_side new_side);

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
    //     The rr_switch_inf switches are remapped versions of the
    //     arch_switch_inf switch indices that are used when we have
    //     different delays and hence different indices based on the fanout
    //     of a switch.  Because fanout of the switch can only be computed
    //     after the graph is built, the graph is initially built using
    //     arch_switch_inf indicies, and then remapped once fanout is
    //     determined.
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

    // Add one edge.  This method is efficient if reserve_edges was called with
    // the number of edges present in the graph.  This method is still
    // amortized O(1), like std::vector::emplace_back, but both runtime and
    // peak memory usage will be higher if reallocation is required.
    void emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch);

    // Adds a batch of edges.
    void alloc_and_load_edges(const t_rr_edge_info_set* rr_edges_to_create);

    /* Edge finalization methods */

    // Counts the number of rr switches needed based on fan in to support mux
    // size dependent switch delays.
    //
    // init_fan_in does not need to be invoked before this method.
    size_t count_rr_switches(
        size_t num_arch_switches,
        t_arch_switch_inf* arch_switch_inf,
        t_arch_switch_fanin& arch_switch_fanins);

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
    void partition_edges(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches);

    // Validate that edge data is partitioned correctly.
    bool validate_node(RRNodeId node_id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    bool validate(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;

    /******************
     * Fan-in methods *
     ******************/

    /* Init per node fan-in data.  Should only be called after all edges have
     * been allocated
     *
     * This is an expensive, O(N), operation so it should be called once you
     * have a complete rr-graph and not called often.*/
    void init_fan_in();

    static inline Direction get_node_direction(
        vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        RRNodeId id) {
        auto& node_data = node_storage[id];
        if (node_data.type_ != CHANX && node_data.type_ != CHANY) {
            VTR_LOG_ERROR("Attempted to access RR node 'direction' for non-channel type '%s'",
                          rr_node_typename[node_data.type_]);
        }
        return node_storage[id].dir_side_.direction;
    }

    /* Find if the given node appears on a specific side */
    static inline bool is_node_on_specific_side(
        vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        const RRNodeId& id,
        const e_side& side) {
        auto& node_data = node_storage[id];
        if (node_data.type_ != IPIN && node_data.type_ != OPIN) {
            VTR_LOG_ERROR("Attempted to access RR node 'side' for non-IPIN/OPIN type '%s'",
                          rr_node_typename[node_data.type_]);
        }
        // Return a vector showing only the sides that the node appears
        std::bitset<NUM_SIDES> side_tt = node_storage[id].dir_side_.sides;
        return side_tt[size_t(side)];
    }

  private:
    friend struct edge_swapper;
    friend class edge_sort_iterator;
    friend class edge_compare_dest_node;
    friend class edge_compare_src_node_and_configurable_first;

    // Take allocated edges in edge_src_node_/ edge_dest_node_ / edge_switch_
    // sort, and assign the first edge for each
    void assign_first_edges();

    // Verify that first_edge_ array correctly partitions rr edge data.
    bool verify_first_edges() const;

    /*****************
     * Graph storage
     *
     * RR graph storage generally speaking includes two types of data:
     *  - **Hot** data is used in the core place and route algorithm.
     *    The layout and size of data stored in **hot** storage will likely
     *    have a significant affect on router performance.  Any changes to
     *    hot data should carefully considered and measured, see
     *    README.developers.md.
     *
     *  - **Cold** data is not used in the core place and route algorithms,
     *    so it isn't as critical to be measure the affects, but it **is**
     *    important to watch the overall memory usage of this data.
     *
     *****************/

    // storage_ stores the core RR node data used by the router and is **very**
    // hot.
    vtr::vector<RRNodeId, t_rr_node_data, vtr::aligned_allocator<t_rr_node_data>> node_storage_;

    // The PTC data is cold data, and is generally not used during the inner
    // loop of either the placer or router.
    vtr::vector<RRNodeId, t_rr_node_ptc_data> node_ptc_;

    // This array stores the first edge of each RRNodeId.  Not that the length
    // of this vector is always storage_.size() + 1, where the last value is
    // always equal to the number of edges in the final graph.
    vtr::vector<RRNodeId, RREdgeId> node_first_edge_;

    // Fan in counts for each RR node.
    vtr::vector<RRNodeId, t_edge_size> node_fan_in_;

    // Edge storage.
    vtr::vector<RREdgeId, RRNodeId> edge_src_node_;
    vtr::vector<RREdgeId, RRNodeId> edge_dest_node_;
    vtr::vector<RREdgeId, short> edge_switch_;

    /***************
     * State flags *
     ***************/
  public: /* Since rr_node_storage is an internal data of RRGraphView and RRGraphBuilder, expose these flags as public */

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

// t_rr_graph_view is a read-only version of t_rr_graph_storage that only
// uses pointers and sizes to rr graph data.
//
// t_rr_graph_view enables efficient access to RR graph storage without owning
// the underlying storage.
//
// Because t_rr_graph_view only uses pointers and sizes, it is suitable for
// use with purely mmap'd data.
class t_rr_graph_view {
  public:
    t_rr_graph_view();
    t_rr_graph_view(
        const vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        const vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
        const vtr::array_view_id<RRNodeId, const RREdgeId> node_first_edge,
        const vtr::array_view_id<RRNodeId, const t_edge_size> node_fan_in,
        const vtr::array_view_id<RREdgeId, const RRNodeId> edge_src_node,
        const vtr::array_view_id<RREdgeId, const RRNodeId> edge_dest_node,
        const vtr::array_view_id<RREdgeId, const short> edge_switch)
        : node_storage_(node_storage)
        , node_ptc_(node_ptc)
        , node_first_edge_(node_first_edge)
        , node_fan_in_(node_fan_in)
        , edge_src_node_(edge_src_node)
        , edge_dest_node_(edge_dest_node)
        , edge_switch_(edge_switch) {}

    /****************
     * Node methods *
     ****************/

    size_t size() const {
        return node_storage_.size();
    }

    t_rr_type node_type(RRNodeId id) const {
        return node_storage_[id].type_;
    }
    const char* node_type_string(RRNodeId id) const;

    int16_t node_rc_index(RRNodeId id) const {
        return node_storage_[id].rc_index_;
    }

    short node_xlow(RRNodeId id) const {
        return node_storage_[id].xlow_;
    }
    short node_ylow(RRNodeId id) const {
        return node_storage_[id].ylow_;
    }
    short node_xhigh(RRNodeId id) const {
        return node_storage_[id].xhigh_;
    }
    short node_yhigh(RRNodeId id) const {
        return node_storage_[id].yhigh_;
    }

    short node_capacity(RRNodeId id) const {
        return node_storage_[id].capacity_;
    }
    RRIndexedDataId node_cost_index(RRNodeId id) const {
        return RRIndexedDataId(node_storage_[id].cost_index_);
    }

    Direction node_direction(RRNodeId id) const {
        return t_rr_graph_storage::get_node_direction(node_storage_, id);
    }

    /* PTC get methods */
    short node_ptc_num(RRNodeId id) const;
    short node_pin_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent
    short node_track_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    short node_class_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent

    /* Retrieve fan_in for RRNodeId. */
    t_edge_size fan_in(RRNodeId id) const {
        return node_fan_in_[id];
    }

    // This prefetechs hot RR node data required for optimization.
    //
    // Note: This is optional, but may lower time spent on memory stalls in
    // some circumstances.
    inline void prefetch_node(RRNodeId id) const {
        VTR_PREFETCH(&node_storage_[id], 0, 0);
    }

    /* Edge accessors */

    // Returns a range of RREdgeId's belonging to RRNodeId id.
    //
    // If this range is empty, then RRNodeId id has no edges.
    vtr::StrongIdRange<RREdgeId> edge_range(RRNodeId id) const {
        return vtr::StrongIdRange<RREdgeId>(first_edge(id), last_edge(id));
    }

    // Get the destination node for the specified edge.
    RRNodeId edge_sink_node(RREdgeId edge) const {
        return edge_dest_node_[edge];
    }

    // Get the switch used for the specified edge.
    short edge_switch(RREdgeId edge) const {
        return edge_switch_[edge];
    }

  private:
    RREdgeId first_edge(RRNodeId id) const {
        return node_first_edge_[id];
    }

    RREdgeId last_edge(RRNodeId id) const {
        return (&node_first_edge_[id])[1];
    }

    vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage_;
    vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc_;
    vtr::array_view_id<RRNodeId, const RREdgeId> node_first_edge_;
    vtr::array_view_id<RRNodeId, const t_edge_size> node_fan_in_;
    vtr::array_view_id<RREdgeId, const RRNodeId> edge_src_node_;
    vtr::array_view_id<RREdgeId, const RRNodeId> edge_dest_node_;
    vtr::array_view_id<RREdgeId, const short> edge_switch_;
};

#endif /* _RR_GRAPH_STORAGE_ */
