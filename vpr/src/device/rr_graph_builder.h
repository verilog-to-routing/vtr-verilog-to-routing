#ifndef RR_GRAPH_BUILDER_H
#define RR_GRAPH_BUILDER_H

#include "rr_graph_storage.h"
#include "rr_spatial_lookup.h"

/* A data structure allows data modification on a routing resource graph 
 *
 * Note that the builder does not own the storage
 * It serves a virtual protocol for
 * - node_storage: store the node list 
 * - node_lookup: store a fast look-up for the nodes
 *
 * Note:
 * - This is the only data structre allowed to modify a routing resource graph
 *
 */
class RRGraphBuilder {
    /****************
     * Constructors
     ****************/
  public:
    RRGraphBuilder(t_rr_graph_storage& node_storage,
                   RRSpatialLookup& node_lookup);

    /****************
     * Mutators
     ****************/
  public:
    /* Return a writable object for rr_nodes */
    t_rr_graph_storage& node_storage();
    /* Return a writable object for update the fast look-up of rr_node */
    RRSpatialLookup& node_lookup();

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
