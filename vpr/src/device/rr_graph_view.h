#ifndef RR_GRAPH_VIEW_H
#define RR_GRAPH_VIEW_H

#include "rr_graph_builder.h"
#include "rr_node.h"
#include "physical_types.h"
/* An read-only routing resource graph
 * which is an unified object including pointors to
 * - node storage
 * - TODO: edge_storage
 * - TODO: node_ptc_storage
 * - TODO: node_fan_in_storage
 * - rr_node_indices
 *
 * Note that the RRGraphView does not own the storage
 * It serves a virtual read-only protocol for
 * - placer
 * - router
 * - timing analyzer
 * - GUI
 *
 * Note that each client of rr_graph may get a frame view of the object
 * The RRGraphView is the complete frame view of the routing resource graph
 * - This helps to reduce the memory footprint for each client
 * - This avoids massive changes for each client on using the APIs
 *   as each frame view provides adhoc APIs for each client
 *
 * TODO: more compact frame views will be created, e.g., 
 * - a mini frame view: contains only node and edges, representing the connectivity of the graph
 * - a geometry frame view: an extended mini frame view with node-level attributes, 
 *                          in particular geometry information (type, x, y etc).
 *
 */
class RRGraphView {
    /* -- Constructors -- */
  public:
    /* See detailed comments about the data structures in the internal data storage section of this file */
    RRGraphView(const t_rr_graph_storage& node_storage,
                const RRSpatialLookup& node_lookup,
                const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                const std::vector<t_segment_inf>& rr_segments);

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
    /** @brief Get the type of a routing resource node. This function is inlined for runtime optimization. */
    inline t_rr_type node_type(RRNodeId node) const {
        return node_storage_.node_type(node);
    }

    /** @brief Get the type string of a routing resource node. This function is inlined for runtime optimization. */
    inline const char* node_type_string(RRNodeId node) const {
        return node_storage_.node_type_string(node);
    }

    /** @brief Get the capacity of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_capacity(RRNodeId node) const {
        return node_storage_.node_capacity(node);
    }

    /** @brief Get the direction of a routing resource node. This function is inlined for runtime optimization.
     * Direction::INC: wire driver is positioned at the low-coordinate end of the wire.
     * Direction::DEC: wire_driver is positioned at the high-coordinate end of the wire.
     * Direction::BIDIR: wire has multiple drivers, so signals can travel either way along the wire
     * Direction::NONE: node does not have a direction, such as IPIN/OPIN
     */
    inline Direction node_direction(RRNodeId node) const {
        return node_storage_.node_direction(node);
    }

    /** @brief Get the direction string of a routing resource node. This function is inlined for runtime optimization. */
    inline const std::string& node_direction_string(RRNodeId node) const {
        return node_storage_.node_direction_string(node);
    }

    /** @brief Get the capacitance of a routing resource node. This function is inlined for runtime optimization. */
    inline float node_C(RRNodeId node) const {
        return node_storage_.node_C(node);
    }

    /** @brief Get the resistance of a routing resource node. This function is inlined for runtime optimization. */
    inline float node_R(RRNodeId node) const {
        return node_storage_.node_R(node);
    }

    /** @brief Get the rc_index of a routing resource node. This function is inlined for runtime optimization. */
    inline int16_t node_rc_index(RRNodeId node) const {
        return node_storage_.node_rc_index(node);
    }

    /** @brief Get the fan in of a routing resource node. This function is inlined for runtime optimization. */
    inline t_edge_size node_fan_in(RRNodeId node) const {
        return node_storage_.fan_in(node);
    }

    /** @brief Get the minimum x-coordinate of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_xlow(RRNodeId node) const {
        return node_storage_.node_xlow(node);
    }

    /** @brief Get the maximum x-coordinate of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_xhigh(RRNodeId node) const {
        return node_storage_.node_xhigh(node);
    }

    /** @brief Get the minimum y-coordinate of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_ylow(RRNodeId node) const {
        return node_storage_.node_ylow(node);
    }

    /** @brief Get the maximum y-coordinate of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_yhigh(RRNodeId node) const {
        return node_storage_.node_yhigh(node);
    }

    /** @brief Get the length (number of grid tile units spanned by the wire, including the endpoints) of a routing resource node.
     * node_length() only applies to CHANX or CHANY and is always a positive number
     * This function is inlined for runtime optimization.
     */
    inline int node_length(RRNodeId node) const {
        VTR_ASSERT(node_type(node) == CHANX || node_type(node) == CHANY);
        int length = 1 + node_xhigh(node) - node_xlow(node) + node_yhigh(node) - node_ylow(node);
        VTR_ASSERT_SAFE(length > 0);
        return length;
    }

    /** @brief Check if routing resource node is initialized. This function is inlined for runtime optimization. */
    inline bool node_is_initialized(RRNodeId node) const {
        return !((node_type(node) == NUM_RR_TYPES)
                 && (node_xlow(node) == -1) && (node_ylow(node) == -1)
                 && (node_xhigh(node) == -1) && (node_yhigh(node) == -1));
    }

    /** @brief Check if two routing resource nodes are adjacent (must be a CHANX and a CHANY). 
     * This function is used for error checking; it checks if two nodes are physically adjacent (could be connected) based on their geometry.
     * It does not check the routing edges to see if they are, in fact, possible to connect in the current routing graph.
     * This function is inlined for runtime optimization. */
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

    /** @brief Check if node is within bounding box. 
     * To return true, the RRNode must be completely contained within the specified bounding box, with the edges of the bounding box being inclusive.
     *This function is inlined for runtime optimization. */
    inline bool node_is_inside_bounding_box(RRNodeId node, vtr::Rect<int> bounding_box) const {
        return (node_xhigh(node) <= bounding_box.xmax()
                && node_xlow(node) >= bounding_box.xmin()
                && node_yhigh(node) <= bounding_box.ymax()
                && node_ylow(node) >= bounding_box.ymin());
    }

    /** @brief Check if x is within x-range spanned by the node, inclusive of its endpoints. This function is inlined for runtime optimization. */
    inline bool x_in_node_range(int x, RRNodeId node) const {
        return !(x < node_xlow(node) || x > node_xhigh(node));
    }

    /** @brief Check if y is within y-range spanned by the node, inclusive of its endpoints. This function is inlined for runtime optimization. */
    inline bool y_in_node_range(int y, RRNodeId node) const {
        return !(y < node_ylow(node) || y > node_yhigh(node));
    }

    /** @brief Get string of information about routing resource node. The string will contain the following information.
     * type, side, x_low, x_high, y_low, y_high, length, direction, segment_name
     * This function is inlined for runtime optimization.
     */
    inline const std::string node_coordinate_to_string(RRNodeId node) const {
        std::string start_x;                                           //start x-coordinate
        std::string start_y;                                           //start y-coordinate
        std::string end_x;                                             //end x-coordinate
        std::string end_y;                                             //end y-coordinate
        std::string arrow;                                             //direction arrow
        std::string coordinate_string = node_type_string(node);        //write the component's type as a routing resource node
        coordinate_string += ":" + std::to_string(size_t(node)) + " "; //add the index of the routing resource node
        if (node_type(node) == OPIN || node_type(node) == IPIN) {
            coordinate_string += "side: ("; //add the side of the routing resource node
            for (const e_side& node_side : SIDES) {
                if (!is_node_on_specific_side(node, node_side)) {
                    continue;
                }
                coordinate_string += std::string(SIDE_STRING[node_side]) + ","; //add the side of the routing resource node
            }
            coordinate_string += ")"; //add the side of the routing resource node
            // For OPINs and IPINs the starting and ending coordinate are identical, so we can just arbitrarily assign the start to larger values
            // and the end to the lower coordinate
            start_x = " (" + std::to_string(node_xhigh(node)) + ","; //start and end coordinates are the same for OPINs and IPINs
            start_y = std::to_string(node_yhigh(node)) + ")";
            end_x = "";
            end_y = "";
            arrow = "";
        }
        if (node_type(node) == CHANX || node_type(node) == CHANY) { //for channels, we would like to describe the component with segment specific information
            RRIndexedDataId cost_index = node_cost_index(node);
            int seg_index = rr_indexed_data_[cost_index].seg_index;
            coordinate_string += rr_segments_[seg_index].name;                   //Write the segment name
            coordinate_string += " length:" + std::to_string(node_length(node)); //add the length of the segment
            //Figure out the starting and ending coordinate of the segment depending on the direction

            arrow = "->"; //we will point the coordinates from start to finish, left to right

            if (node_direction(node) == Direction::DEC) {                //signal travels along decreasing direction
                start_x = " (" + std::to_string(node_xhigh(node)) + ","; //start coordinates have large value
                start_y = std::to_string(node_yhigh(node)) + ")";
                end_x = "(" + std::to_string(node_xlow(node)) + ","; //end coordinates have smaller value
                end_y = std::to_string(node_ylow(node)) + ")";
            }

            else {                                                      // signal travels in increasing direction, stays at same point, or can travel both directions
                start_x = " (" + std::to_string(node_xlow(node)) + ","; //start coordinates have smaller value
                start_y = std::to_string(node_ylow(node)) + ")";
                end_x = "(" + std::to_string(node_xhigh(node)) + ","; //end coordinates have larger value
                end_y = std::to_string(node_yhigh(node)) + ")";
                if (node_direction(node) == Direction::BIDIR) {
                    arrow = "<->"; //indicate that signal can travel both direction
                }
            }
        }

        coordinate_string += start_x + start_y; //Write the starting coordinates
        coordinate_string += arrow;             //Indicate the direction
        coordinate_string += end_x + end_y;     //Write the end coordinates
        return coordinate_string;
    }

    /** @brief Check whether a routing node is on a specific side. This function is inlined for runtime optimization. */
    inline bool is_node_on_specific_side(RRNodeId node, e_side side) const {
        return node_storage_.is_node_on_specific_side(node, side);
    }

    /** @brief Get the cost index of a routing resource node. This function is inlined for runtime optimization. */
    RRIndexedDataId node_cost_index(RRNodeId node) const {
        return node_storage_.node_cost_index(node);
    }

    /** @brief Return the fast look-up data structure for queries from client functions */
    const RRSpatialLookup& node_lookup() const {
        return node_lookup_;
    }

    /* -- Internal data storage -- */
    /* Note: only read-only object or data structures are allowed!!! */
  private:
    /* node-level storage including edge storages */
    const t_rr_graph_storage& node_storage_;
    /* Fast look-up for rr nodes */
    const RRSpatialLookup& node_lookup_;

    /* rr_indexed_data_ and rr_segments_ are needed to lookup the segment information in  node_coordinate_to_string() */
    const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data_;

    /* Segment info for rr nodes */
    const std::vector<t_segment_inf>& rr_segments_;
};

#endif
