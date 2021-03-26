#ifndef _RR_GRAPH_BUILDER_VIEW_
#define _RR_GRAPH_BUILDER_VIEW_

#include <exception>

#include "rr_graph_fwd.h"
#include "rr_node_fwd.h"
#include "rr_graph2.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vpr_utils.h"
#include "vtr_strong_id_range.h"
#include "vtr_array_view.h"
#include "rr_graph_storage.h"
#include "rr_graph_view.h"

/* An builder of routing resource graph which extends 
 * the read-only frame view RRGraphView
 *
 * Note that the builder does not own the storage
 * It serves a virtual protocol for
 * - rr_graph builder, which requires both mutators and accessors
 *
 * Note:
 * - This is the only data structre allowed to modify a routing resource graph
 *
 */
class RRGraphBuilderView : public RRGraphView {
    /****************
     * Constructors
     ****************/
  public:
    RRGraphBuilderView();

    /****************
     * Accessors all come from RRGraph View
     ****************/

    /****************
     * Mutators
     ****************/
  public:
    /* Register a node in the fast look-up */
    void add_node_to_fast_lookup(const RRNodeId& node,
                                 const int& x,
                                 const int& y,
                                 const t_rr_type& type,
                                 const int& ptc,
                                 const e_side& side);

    /****************
     * internal data
     ****************/
  protected:
    /* node-level storage including edge storages */
    t_rr_graph_storage* node_storage_;
    /* Fast look-up */
    t_rr_node_indices* rr_node_indices_;
};

#endif
