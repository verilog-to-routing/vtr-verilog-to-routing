#ifndef RR_GRAPH_VIEW_H
#define RR_GRAPH_VIEW_H

#include "rr_graph_builder.h"

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
                const RRSpatialLookup& node_lookup);

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
    /* Get the type of a routing resource node. This function is inlined for runtime optimization. */
    inline t_rr_type node_type(RRNodeId node) const {
        return node_storage_.node_type(node);
    }

    /* Get the capacity of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_capacity(RRNodeId node) const {
        return node_storage_.node_capacity(node);
    }

    /* Get the direction of a routing resource node. This function is inlined for runtime optimization.
     * Direction::INC: wire driver is positioned at the low-coordinate end of the wire.
     * Direction::DEC: wire_driver is positioned at the high-coordinate end of the wire.
     * Direction::BIDIR: wire has multiple drivers, so signals can travel either way along the wire
     * Direction::NONE: node does not have a direction, such as IPIN/OPIN
     */
    inline Direction node_direction(RRNodeId node) const {
        return node_storage_.node_direction(node);
    }

    /* Get the direction string of a routing resource node. This function is inlined for runtime optimization. */
    inline const std::string& node_direction_string(RRNodeId node) const {
        return node_storage_.node_direction_string(node);
    }

    /* Get the capacitance of a routing resource node. This function is inlined for runtime optimization. */
    inline float node_C(RRNodeId node) const {
        return node_storage_.node_C(node);
    }

    /* Get the resistance of a routing resource node. This function is inlined for runtime optimization. */
    inline float node_R(RRNodeId node) const {
        return node_storage_.node_R(node);
    }

    /* Get the rc_index of a routing resource node. This function is inlined for runtime optimization. */
    inline int16_t node_rc_index(RRNodeId node) const {
        return node_storage_.node_rc_index(node);
    }

    /* Get the fan in of a routing resource node. This function is inlined for runtime optimization. */
    inline t_edge_size node_fan_in(RRNodeId node) const {
        return node_storage_.fan_in(node);
    }

    /* Get the xlow of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_xlow(RRNodeId node) const {
        return node_storage_.node_xlow(node);
    }

    /* Get the xhigh of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_xhigh(RRNodeId node) const {
        return node_storage_.node_xhigh(node);
    }

    /* Get the ylow of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_ylow(RRNodeId node) const {
        return node_storage_.node_ylow(node);
    }

    /* Get the yhigh of a routing resource node. This function is inlined for runtime optimization. */
    inline short node_yhigh(RRNodeId node) const {
        return node_storage_.node_yhigh(node);
    }

    /* Get the length of a routing resource node. This function is inlined for runtime optimization. */
    /* node_length() only applies to CHANX or CHANY and is always a positive number*/
    inline int node_length(RRNodeId node) const {
        VTR_ASSERT(node_type(node) == CHANX || node_type(node) == CHANY);
        int length = 1 + node_xhigh(node) - node_xlow(node) + node_yhigh(node) - node_ylow(node);
        VTR_ASSERT_SAFE(length > 0);
        return length;
    }

    /* Check if routing resource node is initialized. This function is inlined for runtime optimization. */
    inline bool node_is_initialized(RRNodeId node) const {
        return !((node_type(node) == NUM_RR_TYPES)
                 && (node_xlow(node) == -1) && (node_ylow(node) == -1)
                 && (node_xhigh(node) == -1) && (node_yhigh(node) == -1));
    }

    /* Check if two routing resource nodes are adjacent (must be a CHANX and a CHANY). This function is inlined for runtime optimization. */
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

    /* Check if node is within bounding box. This function is inlined for runtime optimization. */
    inline bool node_is_inside_bounding_box(RRNodeId node, vtr::Rect<int> bounding_box) const {
        return (node_xhigh(node) <= bounding_box.xmax()
                && node_xlow(node) >= bounding_box.xmin()
                && node_yhigh(node) <= bounding_box.ymax()
                && node_ylow(node) >= bounding_box.ymin());
    }

    /* Check if x is within node x range. This function is inlined for runtime optimization. */
    inline bool x_in_node_range(int x, RRNodeId node) const {
        return !(x < node_xlow(node) || x > node_xhigh(node));
    }

    /* Check if y is within node y range. This function is inlined for runtime optimization. */
    inline bool y_in_node_range(int y, RRNodeId node) const {
        return !(y < node_ylow(node) || y > node_yhigh(node));
    }

    /* Return the fast look-up data structure for queries from client functions */
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
};

#endif
