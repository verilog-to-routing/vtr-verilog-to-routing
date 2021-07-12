#ifndef RR_GRAPH_VIEW_H
#define RR_GRAPH_VIEW_H

#include "rr_graph_storage.h"
#include "rr_spatial_lookup.h"

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
