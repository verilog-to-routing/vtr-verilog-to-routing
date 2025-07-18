#pragma once

#include <bitset>

#include "vtr_vector.h"
#include "physical_types.h"
#include "rr_graph_storage_utils.h"
#include "rr_node_types.h"
#include "rr_graph_fwd.h"
#include "rr_node_fwd.h"
#include "rr_edge.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_strong_id_range.h"
#include "vtr_array_view.h"
#include <optional>
#include <cstdint>

/* Main structure describing one routing resource node.  Everything in       *
 * this structure should describe the graph -- information needed only       *
 * to store algorithm-specific data should be stored in one of the           *
 * parallel rr_node_* structures.                                            *
 *                                                                           *
 * This structure should **only** contain data used in the inner loop of the *
 * router.  This data is considered "hot" and the router performance is      *
 * sensitive to layout and size of this "hot" data.
 *
 * Cold data should be stored separately in t_rr_graph_storage.              *
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

    e_rr_type type_ = e_rr_type::NUM_RR_TYPES;

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
        int pin_num;
        int track_num;
        int class_num;
        int mux_num;
    } ptc_;
};

class t_rr_graph_view;

/**
 * @file
 * @brief RR node and edge storage class.
 *
 * @details
 * This class stores the detailed routing graph. Each node within the graph is
 * identified by a RRNodeId. Each edge within the graph is identified by a
 * RREdgeId.
 *
 * Each node contains data about the node itself; refer to the comment for t_rr_node_data.
 * Each node also has a set of RREdgeId's, all with RRNodeId as the source node.
 *
 * Each edge is defined by the source node, destination node, and the switch
 * index that connects the source to the destination node.
 *
 * NOTE: The switch index represents either an index into arch_switch_inf
 * **or** rr_switch_inf. During rr graph construction, the switch index is
 * always an index into arch_switch_inf. Once the graph is completed, the
 * RR graph construction code converts all arch_switch_inf indices
 * into rr_switch_inf indices via the remap_rr_node_switch_indices method.
 *
 * @par Usage notes and assumptions:
 * This class is broadly used by two types of code:
 *   - Code that writes to the rr graph
 *   - Code that reads from the rr graph
 *
 * Within VPR, there are two locations where the rr graph is expected to be
 * modified:
 *   - During the building of the rr graph in rr_graph.cpp
 *   - During the reading of a static rr graph from a file in rr_graph_reader
 *     / rr_graph_uxsdcxx_serializer.
 *
 * It is expected and assumed that once the graph is completed, the graph is
 * fixed until the entire graph is cleared. This object enforces this
 * assumption with state flags. In particular, RR graph edges are assumed
 * to be write-only during the construction of the RR graph and read-only
 * otherwise. See the description of the "Edge methods" for details.
 *
 * Broadly speaking, there are two sets of methods: methods for reading and
 * writing RR nodes, and methods for reading and writing RR edges. The node
 * methods can be found underneath the header "Node methods," and the edge
 * methods can be found underneath the header "Edge methods."
 */
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

    e_rr_type node_type(RRNodeId id) const {
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

    void set_tileable(bool is_tileable) {
        is_tileable_ = is_tileable;
    }

    short node_bend_start(RRNodeId id) const {
        return node_bend_start_[id];
    }
    
    short node_bend_end(RRNodeId id) const {
        return node_bend_end_[id];
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

    /** @brief Find if the given node appears on a specific side */
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
     *       const char* side_string = TOTAL_2D_SIDE_STRINGS[side];
     *   }
     */
    const char* node_side_string(RRNodeId id) const;

    /** @brief PTC get methods */
    int node_ptc_num(RRNodeId id) const;
    int node_pin_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent
    int node_track_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    int node_class_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    int node_mux_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent

    /** @brief Retrieve fan_in for RRNodeId, init_fan_in must have been called first. */
    t_edge_size fan_in(RRNodeId id) const {
        return node_fan_in_[id];
    }

    /** @brief Find the layer number that RRNodeId is located at.
     * it is zero if the FPGA only has one die.
     * The layer number start from the base die (base die: 0, the die above it: 1, etc.)
     */
    short node_layer(RRNodeId id) const{
        return node_layer_[id];
    }
    
    /**
     * @brief Retrieve the name assigned to a given node ID.
     *
     * If no name is assigned, an empty optional is returned.
     *
     * @param id The id of the node.
     * @return An optional pointer to the string representing the name if found,
     *         otherwise an empty optional.
     */
    std::optional<const std::string*> node_name(RRNodeId id) const{
        auto it = node_name_.find(id);
        if (it != node_name_.end()) {
            return &it->second;  // Return the value if key is found
        }
        return std::nullopt;  // Return an empty optional if key is not found
    }

    /**
     * @brief Returns the node ID of the virtual sink for the specified clock network name.
     *
     * If the clock network name is not found, the function returns INVALID RRNodeId.
     *
     * @param clock_network_name The name of the clock network.
     * @return The node ID of the virtual sink associated with the provided clock network name,
     *         or INVALID RRNodeID if the clock network name is not found.
     */
    RRNodeId virtual_clock_network_root_idx(const char* clock_network_name) const {
        // Convert the input char* to a C++ string
        std::string clock_network_name_str(clock_network_name);
        
        // Check if the clock network name exists in the list of virtual sink entries
        auto it = virtual_clock_network_root_idx_.find(clock_network_name);
        
        if (it != virtual_clock_network_root_idx_.end()) {
            // Return the node ID for the virtual sink of the given clock network name
            return it->second;
        }

        // Return INVALID RRNodeID if the clock network name is not found
        return RRNodeId::INVALID();
    }

    /**
     * @brief Checks if the specified RRNode ID is a virtual sink for a clock network.
     *
     * A virtual sink/root is a node with the type SINK to which all clock network drive points are 
     * connected in the routing graph.
     * When the two-stage router routes a net through a clock network, it first routes to the virtual sink 
     * in the first stage and then proceeds from there to the destination in the second stage.
     *
     * @param id The ID of an RRNode.
     * @return True if the node with the given ID is a virtual sink for a clock network, false otherwise.
     */
    bool is_virtual_clock_network_root(RRNodeId id) const{
        for (const auto& pair : virtual_clock_network_root_idx_) {
            if (pair.second == id) {
                return true;
            }
        }
        return false;
    }

    /** @brief This prefetechs hot RR node data required for optimization.
     *
     * Note: This is optional, but may lower time spent on memory stalls in
     * some circumstances.
     */
     inline void prefetch_node(RRNodeId id) const {
        VTR_PREFETCH(&node_storage_[id], 0, 0);
     }

    /** @brief Edge accessors
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
     * - edge_source_node(RRNodeId, t_edge_size)
     * - edge_switch(RRNodeId, t_edge_size)
     *
     * Only call these methods after partition_edges has been invoked.
     */
    edge_idx_range edges(const RRNodeId id) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id)));
    }

    edge_idx_range configurable_edges(const RRNodeId id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id, rr_switches)));
    }
    edge_idx_range non_configurable_edges(const RRNodeId id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const {
        return vtr::make_range(edge_idx_iterator(num_edges(id) - num_non_configurable_edges(id, rr_switches)), edge_idx_iterator(num_edges(id)));
    }

    t_edge_size num_edges(const RRNodeId id) const {
        return size_t(last_edge(id)) - size_t(first_edge(id));
    }
    bool edge_is_configurable(RREdgeId edge, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    bool edge_is_configurable(RRNodeId id, t_edge_size iedge, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    t_edge_size num_configurable_edges(RRNodeId node, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    t_edge_size num_non_configurable_edges(RRNodeId node, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;

    /** @brief Get the first and last RREdgeId for the specified RRNodeId.
     *
     * The edges belonging to RRNodeId is [first_edge, last_edge), excluding
     * last_edge.
     *
     * If first_edge == last_edge, then a RRNodeId has no edges.
     */
     RREdgeId first_edge(const RRNodeId id) const {
        return node_first_edge_[id];
    }

    /** @brief the first_edge of the next rr_node, which is one past the edge id range for the node we care about.
     * This implies we have one dummy rr_node at the end of first_edge_, and
     * we always allocate that dummy node. We also assume that the edges have
     * been sorted by rr_node, which is true after partition_edges().
     */
    RREdgeId last_edge(const RRNodeId id) const {
        return (&node_first_edge_[id])[1];
    }

    /** @brief Returns a range of RREdgeId's belonging to RRNodeId id.
     *
     * If this range is empty, then RRNodeId id has no edges.
     */
    vtr::StrongIdRange<RREdgeId> edge_range(const RRNodeId id) const {
        return vtr::StrongIdRange<RREdgeId>(first_edge(id), last_edge(id));
    }

    /** @brief Retrieve the RREdgeId for iedge'th edge in RRNodeId.
     *
     * This method should generally not be used, and instead first_edge and
     * last_edge should be used.
     */
    RREdgeId edge_id(RRNodeId id, t_edge_size iedge) const {
        RREdgeId first_edge = this->first_edge(id);
        RREdgeId ret(size_t(first_edge) + iedge);
        VTR_ASSERT_SAFE(ret < last_edge(id));
        return ret;
    }

    /**
     * @brief Retrieve the RREdgeId that connects the given source and sink nodes.
     *        If the given source/sink nodes are not connected, RREdgeId::INVALID() is returned.
     */
    RREdgeId edge_id(RRNodeId src, RRNodeId sink) const {
        for (RREdgeId outgoing_edge_id : edge_range(src)) {
            if (edge_sink_node(outgoing_edge_id) == sink) {
                return outgoing_edge_id;
            }
        }

        return RREdgeId::INVALID();
    }

    /** 
     * @brief Get the source node for the specified edge. 
     */
    RRNodeId edge_src_node(const RREdgeId edge) const {
        VTR_ASSERT_DEBUG(edge.is_valid());
        return edge_src_node_[edge];
    }

    /** 
     * @brief Get the destination node for the specified edge. 
     */
    RRNodeId edge_sink_node(const RREdgeId edge) const {
        VTR_ASSERT_DEBUG(edge.is_valid());
        return edge_dest_node_[edge];
    }

    /** 
     * @brief Get the source node for the specified edge. 
     */
    RRNodeId edge_source_node(const RREdgeId edge) const {
        return edge_src_node_[edge];
    }

    /** 
     * @brief Call the `apply` function with the edge id, source, and sink nodes of every edge. 
     */
    void for_each_edge(std::function<void(RREdgeId, RRNodeId, RRNodeId)> apply) const {
        for (size_t i = 0; i < edge_dest_node_.size(); i++) {
            RREdgeId edge(i);
            apply(edge, edge_src_node_[edge], edge_dest_node_[edge]);
        }
    }

    /** 
     * @brief Get the destination node for the iedge'th edge from specified RRNodeId.
     *
     * This method should generally not be used, and instead first_edge and
     * last_edge should be used.
     */
    RRNodeId edge_sink_node(const RRNodeId id, t_edge_size iedge) const {
        return edge_sink_node(edge_id(id, iedge));
    }

    /** 
     * @brief Get the source node for the iedge'th edge from specified RRNodeId.
     */
    RRNodeId edge_source_node(const RRNodeId id, t_edge_size iedge) const {
        return edge_source_node(edge_id(id, iedge));
    }

    /** 
     * @brief Get the switch used for the specified edge. 
     */
    short edge_switch(const RREdgeId edge) const {
        return edge_switch_[edge];
    }

    /** 
     * @brief Get the switch used for the iedge'th edge from specified RRNodeId.
     *
     * This method should generally not be used, and instead first_edge and
     * last_edge should be used.
     */
    short edge_switch(const RRNodeId id, t_edge_size iedge) const {
        return edge_switch(edge_id(id, iedge));
    }

    /** 
     * @brief
     * Node proxy methods
     *
     * The following methods implement an interface that appears to be
     * equivalent to the interface exposed by std::vector<t_rr_node>.
     * This was done for backwards compatibility. See t_rr_node for more details.
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
     */
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

    /** @brief
     * Makes room in storage for RRNodeId in amortized O(1) fashion.
     * This results in an allocation pattern similar to what would happen
     * if push_back(x) / emplace_back() were used if underlying storage
     * was not pre-allocated.
     */
    void make_room_for_node(RRNodeId elem_position) {
        make_room_in_vector(&node_storage_, size_t(elem_position));

        // Reserve the capacity based on node_storage_. The capacity is determined in 
        // make_room_in_vector(), which uses a power-of-two growth pattern to avoid 
        // growing the vector one element at a time.
        node_ptc_.reserve(node_storage_.capacity());
        node_ptc_.resize(node_storage_.size());

        node_layer_.reserve(node_storage_.capacity());
        node_layer_.resize(node_storage_.size());

        if (is_tileable_) {
            node_bend_start_.reserve(node_storage_.capacity());
            node_bend_start_.resize(node_storage_.size());

            node_bend_end_.reserve(node_storage_.capacity());
            node_bend_end_.resize(node_storage_.size());
        }
    }

    /** @brief  Reserve storage for RR nodes. */
    void reserve(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.reserve(size);
        node_ptc_.reserve(size);
        node_layer_.reserve(size);
        if (is_tileable_) {
            node_bend_start_.reserve(size);
            node_bend_end_.reserve(size);
        }
    }

    /** @brief  Resize node storage to accomidate size RR nodes. */
    void resize(size_t size) {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.resize(size);
        node_ptc_.resize(size);
        node_layer_.resize(size);
        if (is_tileable_) {
            node_bend_start_.resize(size);
            node_bend_end_.resize(size);
        }
    }

    /** @brief Number of RR nodes that can be accessed. */
    size_t size() const {
        return node_storage_.size();
    }

    /** @brief Is the RR graph currently empty? */
    bool empty() const {
        return node_storage_.empty();
    }

    /** @brief Remove all nodes and edges from the RR graph.
     * This method re-enables graph mutation if the graph was read-only.
     */
    void clear() {
        node_storage_.clear();
        node_ptc_.clear();
        node_first_edge_.clear();
        node_fan_in_.clear();
        node_layer_.clear();
        node_bend_start_.clear();
        node_bend_end_.clear();
        node_name_.clear();
        virtual_clock_network_root_idx_.clear();
        edge_src_node_.clear();
        edge_dest_node_.clear();
        edge_switch_.clear();
        edge_remapped_.clear();
        edges_read_ = false;
        partitioned_ = false;
        remapped_edges_ = false;
        is_tileable_ = false;
    }

    /** @brief
     * Clear the data structures that are mainly used during RR graph construction.
     * After RR Graph is build, we no longer need these data structures.
     */
    void clear_temp_storage() {
        edge_remapped_.clear();
    }

    /** @brief Clear edge_remap data structure, and then initialize it with the given value */
    void init_edge_remap(bool val) {
        edge_remapped_.clear();
        edge_remapped_.resize(edge_switch_.size(), val);
    }

    /** @brief Shrink memory usage of the RR graph storage.
     * Note that this will temporarily increase the amount of storage required
     * to allocate the small array and copy the data.
     */
    void shrink_to_fit() {
        node_storage_.shrink_to_fit();
        node_ptc_.shrink_to_fit();
        node_first_edge_.shrink_to_fit();
        node_fan_in_.shrink_to_fit();
        node_layer_.shrink_to_fit();
        node_bend_start_.shrink_to_fit();
        node_bend_end_.shrink_to_fit();

        edge_src_node_.shrink_to_fit();
        edge_dest_node_.shrink_to_fit();
        edge_switch_.shrink_to_fit();
        edge_remapped_.shrink_to_fit();
    }

    /** @brief Append 1 more RR node to the RR graph.*/
    void emplace_back() {
        // No edges can be assigned if mutating the rr node array.
        VTR_ASSERT(!edges_read_);
        node_storage_.emplace_back();
        node_ptc_.emplace_back();
        node_layer_.emplace_back();
        if (is_tileable_) {
            node_bend_start_.emplace_back();
            node_bend_end_.emplace_back();
        }
    }

    /** @brief Given `order`, a vector mapping each RRNodeId to a new one (old -> new),
     * and `inverse_order`, its inverse (new -> old), update the t_rr_graph_storage
     * data structure to an isomorphic graph using the new RRNodeId's.
     * NOTE: Re-ordering will invalidate any external references, so this
     *       should generally be called before creating such references.
     */
    void reorder(const vtr::vector<RRNodeId, RRNodeId>& order,
                 const vtr::vector<RRNodeId, RRNodeId>& inverse_order);

    /** @brief PTC set methods */
    void set_node_ptc_num(RRNodeId id, int);
    void set_node_pin_num(RRNodeId id, int);   //Same as set_ptc_num() by checks type() is consistent
    void set_node_track_num(RRNodeId id, int); //Same as set_ptc_num() by checks type() is consistent
    void set_node_class_num(RRNodeId id, int); //Same as set_ptc_num() by checks type() is consistent
    void set_node_mux_num(RRNodeId id, int); //Same as set_ptc_num() by checks type() is consistent

    void set_node_type(RRNodeId id, e_rr_type new_type);
    void set_node_name(RRNodeId id, const std::string& new_name);
    void set_node_coordinates(RRNodeId id, short x1, short y1, short x2, short y2);
    void set_node_layer(RRNodeId id, short layer);
    void set_node_cost_index(RRNodeId, RRIndexedDataId new_cost_index);
    void set_node_bend_start(RRNodeId id, size_t bend_start);
    void set_node_bend_end(RRNodeId id, size_t bend_end);
    void set_node_rc_index(RRNodeId, NodeRCIndex new_rc_index);
    void set_node_capacity(RRNodeId, short new_capacity);
    void set_node_direction(RRNodeId, Direction new_direction);

    /** @brief
     * Add a side to the node attributes
     * This is the function to use when you just add a new side WITHOUT resetting side attributes
     */
    void add_node_side(RRNodeId, e_side new_side);

    /**
     * @brief Set the node ID of the virtual sink for the clock network.
     *
     * @param virtual_clock_network_root_idx The node ID of the virtual sink for the clock network.
     */
    void set_virtual_clock_network_root_idx(RRNodeId virtual_clock_network_root_idx);

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

    /** @brief Reserve at least num_edges in the edge backing arrays. */
    void reserve_edges(size_t num_edges);

    /***
     * @brief Add one edge.  This method is efficient if reserve_edges was called with
     * the number of edges present in the graph.  This method is still
     * amortized O(1), like std::vector::emplace_back, but both runtime and
     * peak memory usage will be higher if reallocation is required.
     * @param remapped This is used later in remap_rr_node_switch_indices to check whether an
     * edge needs its switch ID remapped from the arch_sw_idx to rr_sw_idx.
     * The difference between these two ids is because some switch delays depend on the fan-in
     * of the node. Also, the information about switches is fly-weighted and are accessible with IDs. Thus,
     * the number of rr switch types can be higher than the number of arch switch types.
     */
    void emplace_back_edge(RRNodeId src, RRNodeId dest, short edge_switch, bool remapped);

    /** @brief Adds a batch of edges.*/
    void alloc_and_load_edges(const t_rr_edge_info_set* rr_edges_to_create);

    /* Edge finalization methods */

    /** @brief Counts the number of rr switches needed based on fan in to support mux
     * size dependent switch delays.
     *
     * init_fan_in does not need to be invoked before this method.
     */
     size_t count_rr_switches(const std::vector<t_arch_switch_inf>& arch_switch_inf,
                             t_arch_switch_fanin& arch_switch_fanins);

    /** @brief Maps arch_switch_inf indicies to rr_switch_inf indicies.
     *
     * This must be called before partition_edges if edges were created with
     * arch_switch_inf indicies.
     */
    void remap_rr_node_switch_indices(const t_arch_switch_fanin& switch_fanin);

    /** @brief Marks that edge switch values are rr switch indicies.
     *
     * This must be called before partition_edges if edges were created with
     * rr_switch_inf indicies.
     */
    void mark_edges_as_rr_switch_ids();

    /** @brief
     * Sorts edge data such that configurable edges appears before
     * non-configurable edges.
     */
    void partition_edges(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches);

    /** @brief Overrides the associated switch for a given edge by
     *         updating the edge to use the passed in switch. */
    void override_edge_switch(RREdgeId edge_id, RRSwitchId switch_id);

    /** @brief Validate that edge data is partitioned correctly.*/
    bool validate_node(RRNodeId node_id, const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;
    bool validate(const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switches) const;

    /******************
     * Fan-in methods *
     ******************/

    /** @brief Init per node fan-in data.
     * Should only be called after all edges have been allocated
     * This is an expensive, O(N), operation so it should be called once you
     * have a complete rr-graph and not called often.*/
    void init_fan_in();

    static inline Direction get_node_direction(
        vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        RRNodeId id) {
        return node_storage[id].dir_side_.direction;
    }

    /**
     * @brief Find if the given node appears on a specific side.
     *
     * @param node_storage Array view containing data for the nodes.
     * @param id The RRNodeId to check for.
     * @param side The specific side to check for the node.
     * @return true if the node appears on the specified side, false otherwise.
     */
    static inline bool is_node_on_specific_side(
        vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        const RRNodeId id,
        const e_side side) {
        auto& node_data = node_storage[id];
        if (node_data.type_ != e_rr_type::IPIN && node_data.type_ != e_rr_type::OPIN) {
            VTR_LOG_ERROR("Attempted to access RR node 'side' for non-IPIN/OPIN type '%s'",
                          rr_node_typename[node_data.type_]);
        }
        // Return a vector showing only the sides that the node appears
        std::bitset<NUM_2D_SIDES> side_tt = node_storage[id].dir_side_.sides;
        return side_tt[size_t(side)];
    }

    inline void clear_node_first_edge() {
        node_first_edge_.clear();
    }

  private:
    friend struct edge_swapper;
    friend class edge_sort_iterator;
    friend class edge_compare_dest_node;
    friend class edge_compare_src_node_and_configurable_first;

    /** @brief
     * Take allocated edges in edge_src_node_/ edge_dest_node_ / edge_switch_
     * sort, and assign the first edge for each
     */
    void assign_first_edges();

    /** @brief Verify that first_edge_ array correctly partitions rr edge data. */
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

    /** @brief
     * storage_ stores the core RR node data used by the router and is **very**
     * hot.
     */
    vtr::vector<RRNodeId, t_rr_node_data, vtr::aligned_allocator<t_rr_node_data>> node_storage_;

    /** @brief
     * The PTC data is cold data, and is generally not used during the inner
     * loop of either the placer or router.
     */
    vtr::vector<RRNodeId, t_rr_node_ptc_data> node_ptc_;

    /** @brief
     * This array stores the first edge of each RRNodeId.  Not that the length
     * of this vector is always storage_.size() + 1, where the last value is
     * always equal to the number of edges in the final graph.
     */
    vtr::vector<RRNodeId, RREdgeId> node_first_edge_;

    /** @brief Fan in counts for each RR node. */
    vtr::vector<RRNodeId, t_edge_size> node_fan_in_;

    /** @brief
     * Layer number that each RR node is located at
     * Layer number refers to the die that the node belongs to. The layer number of base die is zero and die above it one, etc.
     * This data is also considered as a hot data since it is used in inner loop of router, but since it didn't fit nicely into t_rr_node_data due to alignment issues, we had to store it
     *in a separate vector.
     */
    vtr::vector<RRNodeId, short> node_layer_;

    /**
     * @brief Stores the assigned names for the RRNode IDs.
     *
     * We use a hash table for efficiency because most RRNodes do not have names.
     * An RRNode can be given a name (e.g., example_name) by reading in an rr-graph
     * in which the optional attribute <name=example_name> is set.
     * Currently, the only use case for names is with global clock networks, where
     * the name specifies the clock network to which an RRNode belongs.
     */
    std::unordered_map<RRNodeId, std::string> node_name_;

    /**
     * @brief A map that uses the name for each clock network as the key and stores 
     * the rr_node index for the virtual sink that connects to all the nodes that 
     * are clock network entry points.
     *
     * This map is particularly useful for two-stage clock routing.
     */
    std::unordered_map<std::string, RRNodeId> virtual_clock_network_root_idx_;

    /** @brief Edge storage */
    vtr::vector<RREdgeId, RRNodeId> edge_src_node_;
    vtr::vector<RREdgeId, RRNodeId> edge_dest_node_;
    vtr::vector<RREdgeId, short> edge_switch_;

    /** @brief
     * The delay of certain switches specified in the architecture file depends on the number of inputs of the edge's sink node (pins or tracks).
     * For example, in the case of a MUX switch, the delay increases as the number of inputs increases.
     * During the construction of the RR Graph, switch IDs are assigned to the edges according to the order specified in the architecture file.
     * These switch IDs are later used to retrieve information such as delay for each edge.
     * This allows for effective fly-weighting of edge information.
     *
     * After building the RR Graph, we iterate over the nodes once more to store their fan-in.
     * If a switch's characteristics depend on the fan-in of a node, a new switch ID is generated and assigned to the corresponding edge.
     * This process is known as remapping.
     * In this vector, we store information about which edges have undergone remapping.
     * It is necessary to store this information, especially when flat-router is enabled.
     * Remapping occurs when constructing global resources after placement and when adding intra-cluster resources after placement.
     * Without storing this information, during subsequent remappings, it would be unclear whether the stored switch ID
     * corresponds to the architecture ID or the RR Graph switch ID for an edge.
    */
    vtr::vector<RREdgeId, bool> edge_remapped_;

    /** @brief
     * The following data structures are only used for tileable routing resource graph.
     * The tileable flag is set to true by tileable routing resource graph builder.
     * Bend start and end are used to store the bend information for each node.
     * Bend start and end are only used for CHANX and CHANY nodes.
     * Bend start and end are only used for tileable routing resource graph.
     */
    bool is_tileable_ = false;
    vtr::vector<RRNodeId, int16_t> node_bend_start_;
    vtr::vector<RRNodeId, int16_t> node_bend_end_;

    /***************
     * State flags *
     ***************/
  public: /* Since rr_node_storage is an internal data of RRGraphView and RRGraphBuilder, expose these flags as public */

    /** @brief Has any edges been read?
     *
     * Any method that mutates edge storage will be locked out after this
     * variable is set.
     *
     * Reading any of the following members should set this flag:
     *  - edge_src_node_
     *  - edge_dest_node_
     *  - edge_switch_
     */
    mutable bool edges_read_;

    /** @brief Set after either remap_rr_node_switch_indices or mark_edges_as_rr_switch_ids
     * has been called.
     *
     * remap_rr_node_switch_indices converts indices to arch_switch_inf into
     * indices to rr_switch_inf.
     */
    bool remapped_edges_;

    /** @brief Set after partition_edges has been called. */
    bool partitioned_;
};

/**
 * @brief t_rr_graph_view is a read-only version of t_rr_graph_storage that only
 * uses pointers and sizes to RR graph data.
 *
 * @details
 * t_rr_graph_view enables efficient access to RR graph storage without owning
 * the underlying storage.
 *
 * Because t_rr_graph_view only uses pointers and sizes, it is suitable for
 * use with purely mmap'd data.
 */
class t_rr_graph_view {
  public:
    t_rr_graph_view();
    t_rr_graph_view(
        const vtr::array_view_id<RRNodeId, const t_rr_node_data> node_storage,
        const vtr::array_view_id<RRNodeId, const t_rr_node_ptc_data> node_ptc,
        const vtr::array_view_id<RRNodeId, const RREdgeId> node_first_edge,
        const vtr::array_view_id<RRNodeId, const t_edge_size> node_fan_in,
        const vtr::array_view_id<RRNodeId, const short> node_layer,
        const std::unordered_map<RRNodeId, std::string>& node_name,
        const vtr::array_view_id<RREdgeId, const RRNodeId> edge_src_node,
        const vtr::array_view_id<RREdgeId, const RRNodeId> edge_dest_node,
        const vtr::array_view_id<RREdgeId, const short> edge_switch,
        const std::unordered_map<std::string, RRNodeId>& virtual_clock_network_root_idx,
        const vtr::array_view_id<RRNodeId, const int16_t> node_bend_start,
        const vtr::array_view_id<RRNodeId, const int16_t> node_bend_end)
        : node_storage_(node_storage)
        , node_ptc_(node_ptc)
        , node_first_edge_(node_first_edge)
        , node_fan_in_(node_fan_in)
        , node_layer_(node_layer)
        , node_name_(node_name)
        , edge_src_node_(edge_src_node)
        , edge_dest_node_(edge_dest_node)
        , edge_switch_(edge_switch)
        , virtual_clock_network_root_idx_(virtual_clock_network_root_idx)
        , node_bend_start_(node_bend_start)
        , node_bend_end_(node_bend_end) {}

    /****************
     * Node methods *
     ****************/

    size_t size() const {
        return node_storage_.size();
    }

    e_rr_type node_type(RRNodeId id) const {
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
    int node_ptc_num(RRNodeId id) const;
    int node_pin_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent
    int node_track_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    int node_class_num(RRNodeId id) const; //Same as ptc_num() but checks that type() is consistent
    int node_mux_num(RRNodeId id) const;   //Same as ptc_num() but checks that type() is consistent

    /**
    * @brief Retrieve the fan-in for a given RRNodeId.
    *
    * @param id The RRNodeId for which to retrieve the fan-in.
    * @return The fan-in value.
    */
    t_edge_size fan_in(RRNodeId id) const {
        return node_fan_in_[id];
    }

    /**
     * @brief Retrieve the layer (die) number where the given RRNodeId is located.
     *
     * @param id The RRNodeId for which to retrieve the layer number.
     * @return The layer number (die) where the RRNodeId is located.
     */
    short node_layer(RRNodeId id) const{
        return node_layer_[id];
    }

    /**
     * @brief Retrieve the name assigned to a given node ID.
     *
     * If no name is assigned, an empty optional is returned.
     *
     * @param id The id of the node.
     * @return An optional pointer to the string representing the name if found,
     *         otherwise an empty optional.
     */
    std::optional<const std::string*> node_name(RRNodeId id) const{
        auto it = node_name_.find(id);
        if (it != node_name_.end()) {
            return &it->second;  // Return the value if key is found
        }
        return std::nullopt;  // Return an empty optional if key is not found
    }

    /**
     * @brief Prefetches hot RR node data required for optimization.
     *
     * @details
     * This is optional but may lower time spent on memory stalls in some circumstances.
     *
     * @param id The RRNodeId for which to prefetch hot node data.
     */
    inline void prefetch_node(RRNodeId id) const {
        VTR_PREFETCH(&node_storage_[id], 0, 0);
    }

    /**
     * @brief Returns the node ID of the virtual sink for the specified clock network name.
     *
     * If the clock network name is not found, the function returns INVALID RRNodeId.
     *
     * @param clock_network_name The name of the clock network.
     * @return The node ID of the virtual sink associated with the provided clock network name,
     *         or INVALID RRNodeID if the clock network name is not found.
     */
    RRNodeId virtual_clock_network_root_idx(const char* clock_network_name) const {
        // Convert the input char* to a C++ string
        std::string clock_network_name_str(clock_network_name);
        
        // Check if the clock network name exists in the list of virtual sink entries
        auto it = virtual_clock_network_root_idx_.find(clock_network_name_str);
        
        if (it != virtual_clock_network_root_idx_.end()) {
            // Return the node ID for the virtual sink of the given clock network name
            return it->second;
        }
        
        // Return INVALID RRNodeID if the clock network name is not found
        return RRNodeId::INVALID();
    }

    
    /**
     * @brief Checks if the specified RRNode ID is a virtual sink for a clock network.
     *
     * A virtual sink is a node with the type SINK to which all clock network drive points are 
     * connected in the routing graph.
     * When the two-stage router routes a net through a clock network, it first routes to the virtual sink 
     * in the first stage and then proceeds from there to the destination in the second stage.
     *
     * @param id The ID of an RRNode.
     * @return True if the node with the given ID is a virtual sink for a clock network, false otherwise.
     */
    bool is_virtual_clock_network_root(RRNodeId id) const {
        for (const auto& pair : virtual_clock_network_root_idx_) {
            if (pair.second == id) {
                return true;
            }
        }
        return false;
    }

    /* Edge accessors */

    /**
     * @brief Returns a range of RREdgeId's belonging to RRNodeId id.
     *
     * @details
     * If this range is empty, then RRNodeId id has no edges.
     *
     * @param id The RRNodeId for which to retrieve the edge range.
     * @return A range of RREdgeId's belonging to the specified RRNodeId.
     */
    vtr::StrongIdRange<RREdgeId> edge_range(RRNodeId id) const {
        return vtr::StrongIdRange<RREdgeId>(first_edge(id), last_edge(id));
    }

    /**
     * @brief Get the destination node for the specified edge.
     *
     * @details
     * This function retrieves the RRNodeId representing the destination node for the specified edge.
     *
     * @param edge The RREdgeId for which to retrieve the destination node.
     * @return The RRNodeId representing the destination node for the specified edge.
     */
    RRNodeId edge_sink_node(RREdgeId edge) const {
        return edge_dest_node_[edge];
    }

    /**
     * @brief Get the switch used for the specified edge.
     *
     * @details
     * This function retrieves the switch used for the specified edge.
     *
     * @param edge The RREdgeId for which to retrieve the switch.
     * @return The switch index used for the specified edge.
     */
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
    vtr::array_view_id<RRNodeId, const short> node_layer_;
    const std::unordered_map<RRNodeId, std::string>& node_name_;
    vtr::array_view_id<RREdgeId, const RRNodeId> edge_src_node_;
    vtr::array_view_id<RREdgeId, const RRNodeId> edge_dest_node_;
    vtr::array_view_id<RREdgeId, const short> edge_switch_;
    const std::unordered_map<std::string, RRNodeId>& virtual_clock_network_root_idx_;

    vtr::array_view_id<RRNodeId, const int16_t> node_bend_start_;
    vtr::array_view_id<RRNodeId, const int16_t> node_bend_end_;

};
