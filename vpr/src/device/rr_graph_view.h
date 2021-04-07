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
    /****************
     * Constructors
     ****************/
  public:
    RRGraphView(t_rr_graph_storage& node_storage,
                RRSpatialLookup& node_lookup);

    /****************
     * Accessors
     ****************/
  public:
    /* Get the type of a routing resource node */
    t_rr_type node_type(const RRNodeId& node) const;

    /* Return a read-only object for performing fast look-up in rr_node */
    const RRSpatialLookup& node_lookup() const;

    /****************
     * internal data storage
     ****************/
  private:
    /* node-level storage including edge storages */
    t_rr_graph_storage& node_storage_;
    /* Fast look-up for rr nodes */
    RRSpatialLookup& node_lookup_;
};

#endif
