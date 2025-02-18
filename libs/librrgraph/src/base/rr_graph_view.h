#ifndef RR_GRAPH_VIEW_H
#define RR_GRAPH_VIEW_H

/**
 * @file
 * @brief The RRGraphView encapsulates a read-only routing resource graph as most 
 * clients (router, timing analyzer, etc.) only need to read a routing-resource graph (RRGraph).
 * 
 * 
 * The RRGraph models the FPGA's programmable routing fabric as a graph consisting of nodes and edges. 
 * Each node and edge is supplemented with additional metadata, such as the physical location within 
 * the chip and electrical properties, to optimize algorithm efficiency, aid in visualizing the chip layout, 
 * and estimate signal delays.
 * 
 * @par  RRGraph nodes
 * Each node represents a routing resource, which can be:
 * 1. A routing track (CHANX or CHANY).
 * 2. An input/output of a logic block (IPIN or OPIN).
 * 3. A virtual source or sink node (SOURCE or SINK).
 * 
 * @par  RRGraph edges
 * Each edge represents a switch between resources, which can be:
 * 1. A multiplexer.
 * 2. A tri-state buffer.
 * 3. A pass gate.
 * 4. A non-configurable buffer.
 * 5. A short (metal connection).
 * 
 * 
 * @note Despite the RRGraph containing millions of edges, there are only a few switch types. Therefore, all switch details, including R and C, are stored using a flyweight pattern (rr_switch_inf) rather than being directly embedded in the edge-related data of the RRGraph. Each edge stores the ID of its associated switch for easy lookup.
 * 
 * 
 * \internal
 * A unified object that includes pointers to:
 * - Node storage
 * - TODO: Edge storage
 * - TODO: Node PTC storage
 * - TODO: Node fan-in storage
 * - RR node indices
 * \endinternal

 *
 * @note The RRGraphView does not own the storage. It provides a virtual 
 * read-only protocol for:
 * - Placer
 * - Router
 * - Timing analyzer
 * - GUI
 *
 * \internal
 * TODO: More compact frame views will be created, such as:
 * - A mini frame view: Contains only nodes and edges, representing the 
 *   connectivity of the graph.
 * - A geometry frame view: An extended mini frame view with node-level 
 *   attributes, particularly geometry information (type, x, y, etc.).
 * \endinternal
 */
#include "rr_graph_builder.h"
#include "rr_node.h"
#include "physical_types.h"

class RRGraphView {
    /* -- Constructors -- */
  public:
    /* See detailed comments about the data structures in the internal data storage section of this file */
    RRGraphView(const t_rr_graph_storage& node_storage,
                const RRSpatialLookup& node_lookup,
                const MetadataStorage<int>& rr_node_metadata,
                const MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata,
                const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                const std::vector<t_rr_rc_data>& rr_rc_data,
                const vtr::vector<RRSegmentId, t_segment_inf>& rr_segments,
                const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf);

    /* Disable copy constructors and copy assignment operator
     * This is to avoid accidental copy because it could be an expensive operation considering that the 
     * memory footprint of the data structure could ~ Gb
     * Using the following syntax, we prohibit accidental 'pass-by-value' which can be immediately caught 
     * by compiler
     */
    RRGraphView(const RRGraphView&) = delete;
    void operator=(const RRGraphView&) = delete;

    /* -- Accessors -- */
    /* TODO: The accessors may be turned into private later if they are replacable by 'questionin' 
     * kind of accessors
     */
  public:
    /**
     * @brief Aggregates for creating range-based loops for nodes.
     *
     * To iterate over the nodes in an RRGraph, using a range-based loop is recommended.
     * 
     * @code
     * 
     * // Strongly suggest using a read-only rr_graph object const RRGraph& rr_graph;
     * for (const RRNodeId& node : rr_graph.nodes()) {
     *     // Do something with each node
     * }
     * @endcode
     */

    inline vtr::StrongIdRange<RRNodeId> nodes() const {
        return vtr::StrongIdRange<RRNodeId>(RRNodeId(0), RRNodeId(num_nodes()));
    }

    /** @brief Return number of routing resource nodes. 
    */
    inline size_t num_nodes() const {
        return node_storage_.size();
    }
    /** @brief check whether the RR graph is currently empty 
    */
    inline bool empty() const {
        return node_storage_.empty();
    }

    /** @brief Return a range of RREdgeIds that belong to a specified node.
     * @note If this range is empty, then the specified node has no edges.*/
    inline vtr::StrongIdRange<RREdgeId> edge_range(RRNodeId id) const {
        return vtr::StrongIdRange<RREdgeId>(node_storage_.first_edge(id), node_storage_.last_edge(id));
    }

    /** @brief Return the type of a specified node.
    */
    inline t_rr_type node_type(RRNodeId node) const {
        return node_storage_.node_type(node);
    }

    /**
     * @brief Return the name of a specified node.
     * @note If no name is assigned, an empty optional is returned.
     */
    std::optional<const std::string*> node_name(RRNodeId node) const {
        return node_storage_.node_name(node);
    }

    /** @brief Return a string indicating the type of the specified node. 
    */
    inline const char* node_type_string(RRNodeId node) const {
        return node_storage_.node_type_string(node);
    }

    /** @brief Return the capacity of a specified node. 
    */
    inline short node_capacity(RRNodeId node) const {
        return node_storage_.node_capacity(node);
    }

    /**
     * @brief Return the direction of a specified node.
     * 
     * @note Direction Categories:
     * - `Direction::INC`: Wire driver is positioned at the low-coordinate end of the wire.
     * - `Direction::DEC`: Wire driver is positioned at the high-coordinate end of the wire.
     * - `Direction::BIDIR`: Wire has multiple drivers, allowing signals to travel either way along the wire.
     * - `Direction::NONE`: Node does not have a direction, such as IPIN/OPIN.
     */
    inline Direction node_direction(RRNodeId node) const {
        return node_storage_.node_direction(node);
    }

    /** @brief Return a string representing the direction of the specified node.
    */
    inline const std::string& node_direction_string(RRNodeId node) const {
        return node_storage_.node_direction_string(node);
    }

    /** @brief Return the capacitance of a specified node.
    */
    inline float node_C(RRNodeId node) const {
        VTR_ASSERT(node_rc_index(node) < (short)rr_rc_data_.size());
        return rr_rc_data_[node_rc_index(node)].C;
    }

    /** @brief Return the resistance of a specified node.
    */
    inline float node_R(RRNodeId node) const {
        VTR_ASSERT(node_rc_index(node) < (short)rr_rc_data_.size());
        return rr_rc_data_[node_rc_index(node)].R;
    }

    /** @brief Return the rc_index of a specified node.
    */
    inline int16_t node_rc_index(RRNodeId node) const {
        return node_storage_.node_rc_index(node);
    }

    /** @brief Return the fan in count of a specified node.
    */
    inline t_edge_size node_fan_in(RRNodeId node) const {
        return node_storage_.fan_in(node);
    }

    /** @brief Return the minimum x-coordinate of a specified node.
    */
    inline short node_xlow(RRNodeId node) const {
        return node_storage_.node_xlow(node);
    }

    /** @brief Return the maximum x-coordinate of a specified node.
    */
    inline short node_xhigh(RRNodeId node) const {
        return node_storage_.node_xhigh(node);
    }

    /** @brief Return the minimum y-coordinate of a specified node.
    */
    inline short node_ylow(RRNodeId node) const {
        return node_storage_.node_ylow(node);
    }

    /** @brief Return the maximum y-coordinate of a specified node.
    */
    inline short node_yhigh(RRNodeId node) const {
        return node_storage_.node_yhigh(node);
    }

    /** @brief Return the layer num of a specified node.
    */
    inline short node_layer(RRNodeId node) const {
        return node_storage_.node_layer(node);
    }

    /** @brief Return the ptc number twist of a specified node.
    */
    inline short node_ptc_twist(RRNodeId node) const {
        return node_storage_.node_ptc_twist(node);
    }

    /** @brief Return the first outgoing edge of a specified node.
    */
    inline RREdgeId node_first_edge(RRNodeId node) const {
        return node_storage_.first_edge(node);
    }

    /** @brief Return the last outgoing edge of a specified node.
    */
    inline RREdgeId node_last_edge(RRNodeId node) const {
        return node_storage_.last_edge(node);
    }

    /** @brief Return the length (number of grid tile units spanned by the wire, including the endpoints) of a specified node.
     * @note node_length() only applies to CHANX or CHANY and is always a positive number
     */
    inline int node_length(RRNodeId node) const {
        VTR_ASSERT(node_type(node) == CHANX || node_type(node) == CHANY);
        if (node_direction(node) == Direction::NONE) {
            return 0; //length zero wire
        }
        int length = 1 + node_xhigh(node) - node_xlow(node) + node_yhigh(node) - node_ylow(node);
        VTR_ASSERT_SAFE(length > 0);
        return length;
    }

    /** @brief Check if a routing resource node is initialized.
     */
    inline bool node_is_initialized(RRNodeId node) const {
        return !((node_type(node) == NUM_RR_TYPES)
                 && (node_xlow(node) == -1) && (node_ylow(node) == -1)
                 && (node_xhigh(node) == -1) && (node_yhigh(node) == -1));
    }

    /** @brief Check if two routing resource nodes are adjacent (must be a CHANX and a CHANY). 
     * @note This function performs error checking by determining whether two nodes are physically adjacent based on their geometry. It does not verify the routing edges to confirm if a connection is feasible within the current routing graph.
     */
    inline bool nodes_are_adjacent(RRNodeId chanx_node, RRNodeId chany_node) const {
        VTR_ASSERT(node_type(chanx_node) == CHANX && node_type(chany_node) == CHANY);
        if (node_ylow(chany_node) > node_ylow(chanx_node) + 1 || // verifies that chany_node is not more than one unit above chanx_node
            node_yhigh(chany_node) < node_ylow(chanx_node))      // verifies that chany_node is not more than one unit beneath chanx_node
            return false;
        if (node_xlow(chanx_node) > node_xlow(chany_node) + 1 || // verifies that chany_node is not more than one unit to the left of chanx_node
            node_xhigh(chanx_node) < node_xlow(chany_node))      // verifies that chany_node is not more than one unit to the right of chanx_node
            return false;
        return true;
    }

    /**
     * @brief Check if the node is within the bounding box.
     * 
     * @param node the id of the node.
     * @param bounding_box a 2D rectangle that defines the bounding area.
     * @note To return true, the RRNode must be completely contained within the specified bounding box,
     * with the edges of the bounding box being inclusive.
     */

    inline bool node_is_inside_bounding_box(RRNodeId node, vtr::Rect<int> bounding_box) const {
        return (node_xhigh(node) <= bounding_box.xmax()
                && node_xlow(node) >= bounding_box.xmin()
                && node_yhigh(node) <= bounding_box.ymax()
                && node_ylow(node) >= bounding_box.ymin());
    }

    /** @brief Check if x is within x-range spanned by a specified node, inclusive of its endpoints. 
    */
    inline bool x_in_node_range(int x, RRNodeId node) const {
        return !(x < node_xlow(node) || x > node_xhigh(node));
    }

    /** @brief Check if y is within y-range spanned by a specified node, inclusive of its endpoints. 
    */
    inline bool y_in_node_range(int y, RRNodeId node) const {
        return !(y < node_ylow(node) || y > node_yhigh(node));
    }

    /** @brief Return a string containing information about the specified node.
     * @note The returned string can contain the following information:
     * type, side, x_low, x_high, y_low, y_high, length, direction, segment_name, layer num
     */
    inline const std::string node_coordinate_to_string(RRNodeId node) const {
        std::string start_x;                                           //start x-coordinate
        std::string start_y;                                           //start y-coordinate
        std::string end_x;                                             //end x-coordinate
        std::string end_y;                                             //end y-coordinate
        std::string start_layer_str;                                   //layer number
        std::string end_layer_str;                                     //layer number
        std::string arrow;                                             //direction arrow
        std::string coordinate_string = node_type_string(node);        //write the component's type as a routing resource node
        coordinate_string += ":" + std::to_string(size_t(node)) + " "; //add the index of the routing resource node

        int node_layer_num = node_layer(node);
        if (node_type(node) == OPIN || node_type(node) == IPIN) {
            coordinate_string += "side: ("; //add the side of the routing resource node
            for (const e_side& node_side : TOTAL_2D_SIDES) {
                if (!is_node_on_specific_side(node, node_side)) {
                    continue;
                }
                coordinate_string += std::string(TOTAL_2D_SIDE_STRINGS[node_side]) + ","; //add the side of the routing resource node
            }
            coordinate_string += ")"; //add the side of the routing resource node
            // For OPINs and IPINs the starting and ending coordinate are identical, so we can just arbitrarily assign the start to larger values
            // and the end to the lower coordinate
            start_x = " (" + std::to_string(node_xhigh(node)) + ","; //start and end coordinates are the same for OPINs and IPINs
            start_y = std::to_string(node_yhigh(node)) + ",";
            start_layer_str = std::to_string(node_layer_num) + ")";
        } else if (node_type(node) == SOURCE || node_type(node) == SINK) {
            // For SOURCE and SINK the starting and ending coordinate are identical, so just use start
            start_x = " (" + std::to_string(node_xhigh(node)) + ",";
            start_y = std::to_string(node_yhigh(node)) + ",";
            start_layer_str = std::to_string(node_layer_num) + ")";
        } else if (node_type(node) == CHANX || node_type(node) == CHANY) { //for channels, we would like to describe the component with segment specific information
            RRIndexedDataId cost_index = node_cost_index(node);
            int seg_index = rr_indexed_data_[cost_index].seg_index;
            coordinate_string += rr_segments(RRSegmentId(seg_index)).name;       //Write the segment name
            coordinate_string += " length:" + std::to_string(node_length(node)); //add the length of the segment
            //Figure out the starting and ending coordinate of the segment depending on the direction

            arrow = " ->"; //we will point the coordinates from start to finish, left to right

            if (node_direction(node) == Direction::DEC) { //signal travels along decreasing direction

                start_x = " (" + std::to_string(node_xhigh(node)) + ","; //start coordinates have large value
                start_y = std::to_string(node_yhigh(node)) + ",";
                start_layer_str = std::to_string(node_layer_num);
                end_x = " (" + std::to_string(node_xlow(node)) + ","; //end coordinates have smaller value
                end_y = std::to_string(node_ylow(node)) + ",";
                end_layer_str = std::to_string(node_layer_num) + ")";
            }

            else {                                                      // signal travels in increasing direction, stays at same point, or can travel both directions
                start_x = " (" + std::to_string(node_xlow(node)) + ","; //start coordinates have smaller value
                start_y = std::to_string(node_ylow(node)) + ",";
                start_layer_str = std::to_string(node_layer_num);
                end_x = " (" + std::to_string(node_xhigh(node)) + ","; //end coordinates have larger value
                end_y = std::to_string(node_yhigh(node)) + ",";
                end_layer_str = std::to_string(node_layer_num) + ")"; //layer number
                if (node_direction(node) == Direction::BIDIR) {
                    arrow = " <->"; //indicate that signal can travel both direction
                }
            }
        }

        coordinate_string += start_x + start_y + start_layer_str; //Write the starting coordinates
        coordinate_string += arrow;                               //Indicate the direction
        coordinate_string += end_x + end_y + end_layer_str;       //Write the end coordinates
        return coordinate_string;
    }

    /** @brief Check whether a routing resource node is on a specific side. 
     */
    inline bool is_node_on_specific_side(RRNodeId node, e_side side) const {
        return node_storage_.is_node_on_specific_side(node, side);
    }

    /** @brief Return a string representing the side of a routing resource node. 
     */
    inline const char* node_side_string(RRNodeId node) const {
        return node_storage_.node_side_string(node);
    }

    /** @brief Return the node id of the clock network virtual sink 
    */
    inline RRNodeId virtual_clock_network_root_idx(const char* clock_network_name) const {
        return node_storage_.virtual_clock_network_root_idx(clock_network_name);
    }

    /**
     * @brief Checks if the specified node is a virtual sink for a clock network.
     */
    inline bool is_virtual_clock_network_root(RRNodeId id) const {
        return node_storage_.is_virtual_clock_network_root(id);
    }

    /** @brief Return the switch id that represents the iedge'th outgoing edge from a specific node.
     * @param id the id of the node.
     * @param iedge the outgoing edge index of the node.
     * @return the id of the switch used for the specified edge. 
     * \internal 
     * @todo We may need to revisit this API and think about higher level APIs, like ``switch_delay()``.
     * \endinternal
    */
    inline short edge_switch(RRNodeId id, t_edge_size iedge) const {
        return node_storage_.edge_switch(id, iedge);
    }

    /** @brief Return the source node for the specified edge. 
    */
    inline RRNodeId edge_src_node(const RREdgeId edge_id) const {
        return node_storage_.edge_src_node(edge_id);
    }

    /** @brief Return the destination node for the iedge'th edge from specified RRNodeId.
     *  @param id the id of the node
     *  @param iedge the iedge'th edge of the node
     *  @return destination node id of the specified edge 
     *  @note This method should generally not be used, and instead first_edge and 
     * last_edge should be used.*/
    inline RRNodeId edge_sink_node(RRNodeId id, t_edge_size iedge) const {
        return node_storage_.edge_sink_node(id, iedge);
    }

    /** @brief Check if the edge is a configurable edge 
     * @note A configurable edge represents a programmable switch between routing resources, which could be 
     *  - a multiplexer
     *  - a tri-state buffer
     *  - a pass gate 
    */
    inline bool edge_is_configurable(RRNodeId id, t_edge_size iedge) const {
        return node_storage_.edge_is_configurable(id, iedge, rr_switch_inf_);
    }

    /** @brief Return the number of configurable edges. 
    */
    inline t_edge_size num_configurable_edges(RRNodeId node) const {
        return node_storage_.num_configurable_edges(node, rr_switch_inf_);
    }

    /** @brief Return the number of non-configurable edges. 
     */
    inline t_edge_size num_non_configurable_edges(RRNodeId node) const {
        return node_storage_.num_non_configurable_edges(node, rr_switch_inf_);
    }

    /** @brief Return ID range for configurable edges.  
     */
    inline edge_idx_range configurable_edges(RRNodeId node) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(node_storage_.num_edges(node) - num_non_configurable_edges(node)));
    }

    /** @brief Return ID range for non-configurable edges. 
     * @note A non-configurable edge represents a hard-wired connection between routing resources, which could be 
     *  - a non-configurable buffer that can not be turned off
     *  - a short metal connection that can not be turned off
     */
    inline edge_idx_range non_configurable_edges(RRNodeId node) const {
        return vtr::make_range(edge_idx_iterator(node_storage_.num_edges(node) - num_non_configurable_edges(node)), edge_idx_iterator(num_edges(node)));
    }

    /**
     * @brief Retrieve the outgoing edges for a specified node. This API is designed to facilitate range-based loops for traversing the outgoing edges of a node.
     * @param id the id of the node
     * @return the range of outgoing edges for the given node
     * @example
     * RRGraphView rr_graph; // A dummy rr_graph for a short example
     * RRNodeId node; // A dummy node for a short example
     * for (RREdgeId edge : rr_graph.edges(node)) {
     *     // Do something with the edge
     * }
     */
    inline edge_idx_range edges(const RRNodeId& id) const {
        return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges(id)));
    }

    /** @brief Return the number of edges.
     */
    inline t_edge_size num_edges(RRNodeId node) const {
        return node_storage_.num_edges(node);
    }

    /**
     * @brief Retrieve the `ptc_num` of a routing resource node.
     * @note ptc_num (Pin, Track, or Class Number) allows for distinguishing overlapping routing elements that occupy the same (x, y) coordinate, ensuring they can be uniquely identified and managed without confusion. For instance, several routing wires or pins might overlap physically, but their ptc_num differentiates them.
     * @note The meaning of `ptc_num` varies depending on the node type (relevant to VPR RRG, may not apply to custom RRGs):
     *  - **CHANX/CHANY**: Represents the track ID in routing channels.
     *  - **OPIN/IPIN**: Refers to the pin index within the logic block data structure.
     *  - **SOURCE/SINK**: Denotes the class ID of a pin, indicating logic equivalence in the logic block data structure.
     * 
     * @warning This API is highly versatile and should only be used when necessary (e.g., when the node type is unknown).
     *          If the node type is known, prefer using more specific APIs such as `node_pin_num()`, `node_track_num()`, 
     *          or `node_class_num()` based on the node type.
     */
    inline int node_ptc_num(RRNodeId node) const {
        return node_storage_.node_ptc_num(node);
    }

    /** @brief Return the pin num of a routing resource node.
     *  @note This function is intended for logic blocks and should only be used with IPIN or OPIN nodes.
    */
    inline int node_pin_num(RRNodeId node) const {
        return node_storage_.node_pin_num(node);
    }

    /** @brief Return the track num of a routing resource node. 
     * @note This function should only be used with CHANX or CHANY nodes.
     */
    inline int node_track_num(RRNodeId node) const {
        return node_storage_.node_track_num(node);
    }

    /** @brief Return the class num of a routing resource node. 
     * @note This function should only be used with SOURCE or SINK nodes.
    */
    inline int node_class_num(RRNodeId node) const {
        return node_storage_.node_class_num(node);
    }

    /** @brief Return the cost index of a routing resource node. 
    */
    RRIndexedDataId node_cost_index(RRNodeId node) const {
        return node_storage_.node_cost_index(node);
    }

    /** @brief Return detailed routing segment information of a specified segment
     * @note The routing segments here may not be exactly same as those defined in architecture file. They have been
     * adapted to fit the context of routing resource graphs.
     */
    inline const t_segment_inf& rr_segments(RRSegmentId seg_id) const {
        return rr_segments_[seg_id];
    }
    /** @brief Return the number of rr_segments in the routing resource graph */
    inline size_t num_rr_segments() const {
        return rr_segments_.size();
    }
    /** @brief Return a read-only list of rr_segments for queries from client functions  */
    inline const vtr::vector<RRSegmentId, t_segment_inf>& rr_segments() const {
        return rr_segments_;
    }

    /** @brief  Return the switch information that is categorized in the rr_switch_inf with a given id.  
     * @note rr_switch_inf is created to minimize memory footprint of RRGraph class.
     * While the RRG could contain millions (even much larger) of edges, there are only
     * a limited number of types of switches.
     * Hence, we use a flyweight pattern to store switch-related information that differs
     * only for types of switches (switch type, drive strength, R, C, etc.).
     * Each edge stores the ids of the switch that implements it so this additional information
     * can be easily looked up.
     *
     * @note All the switch-related information, such as R, C, should be placed in rr_switch_inf
     * but NOT directly in the edge-related data of RRGraph.
     * If you wish to create a new data structure to represent switches between routing resources,
     * please follow the flyweight pattern by linking your switch ids to edges only!
     */

    inline const t_rr_switch_inf& rr_switch_inf(RRSwitchId switch_id) const {
        return rr_switch_inf_[switch_id];
    }
    /** @brief Return the number of rr_segments in the routing resource graph */
    inline size_t num_rr_switches() const {
        return rr_switch_inf_.size();
    }
    /** @brief Return the rr_switch_inf_ structure for queries from client functions */
    inline const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch() const {
        return rr_switch_inf_;
    }

    /** @brief Return the fast look-up data structure for queries from client functions */
    const RRSpatialLookup& node_lookup() const {
        return node_lookup_;
    }
    /** @brief Return the node-level storage structure for queries from client functions */
    inline const t_rr_graph_storage& rr_nodes() const {
        return node_storage_;
    }

    /** @brief Return the metadata of rr nodes
     * @warning The Metadata should stay as an independent data structure than rest of the internal data,
     *  e.g., node_lookup! */
    MetadataStorage<int> rr_node_metadata_data() const {
        return rr_node_metadata_;
    }
    /** @brief Return the metadata of rr edges 
    */
    MetadataStorage<std::tuple<int, int, short>> rr_edge_metadata_data() const {
        return rr_edge_metadata_;
    }

  public: /* Validators */
    /** @brief Validate that edge data is partitioned correctly
     * @note This function is used to validate the correctness of the routing resource graph in terms
     * of graph attributes. Strongly recommend to call it when you finish the building a routing resource
     * graph. If you need more advance checks, which are related to architecture features, you should
     * consider to use the check_rr_graph() function or build your own check_rr_graph() function. */
    inline bool validate_node(RRNodeId node_id) const {
        return node_storage_.validate_node(node_id, rr_switch_inf_);
    }

    /* -- Internal data storage -- */
    /* Note: only read-only object or data structures are allowed!!! */
  private:
    /// node-level storage including edge storages
    const t_rr_graph_storage& node_storage_;
    /// Fast look-up for rr nodes
    const RRSpatialLookup& node_lookup_;

    /**
     * @brief Attributes for each rr_node.
     *
     *  - key:     rr_node index
     *  - value:   map of <attribute_name, attribute_value>
     * @warning    The Metadata should stay as an independent data structure than rest of the internal data,
     *  e.g., node_lookup!
     * @note Metadata is an extra data on rr-nodes and edges, respectively, that is not used by vpr
     * but simply passed through the flow so that it can be used by downstream tools.
     * The main (perhaps only) current use of this metadata is the fasm tool of symbiflow,
     * which needs extra metadata on which programming bits control which switch in order to produce a bitstream.*/
    const MetadataStorage<int>& rr_node_metadata_;
    /**
     * @brief  Attributes for each rr_edge
     *
     *  - key:     <source rr_node_index, sink rr_node_index, iswitch>
     *  - iswitch: Index of the switch type used to go from this rr_node to
     *          the next one in the routing.  OPEN if there is no next node
     *          (i.e. this node is the last one (a SINK) in a branch of the
     *          net's routing).
     *  - value:   map of <attribute_name, attribute_value>
     */
    const MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata_;
    /// rr_indexed_data_ and rr_segments_ are needed to lookup the segment information in  node_coordinate_to_string()
    const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data_;

    /// RC data for nodes. This is a flyweight data
    const std::vector<t_rr_rc_data>& rr_rc_data_;

    /// Segment info for rr nodes
    const vtr::vector<RRSegmentId, t_segment_inf>& rr_segments_;
    /// switch info for rr nodes
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf_;
};

#endif
