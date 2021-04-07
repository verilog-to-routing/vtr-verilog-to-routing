/***************************************
 * Methods for Object RRGraphView
 ***************************************/
#include "rr_graph_view.h"

/****************************
 * Constructors
 ****************************/
RRGraphView::RRGraphView(t_rr_graph_storage& node_storage,
                         RRSpatialLookup& node_lookup)
    : node_storage_(node_storage), node_lookup_(node_lookup) {
}

/****************************
 * Accessors
 ****************************/
t_rr_type RRGraphView::node_type(const RRNodeId& node) const {
    return node_storage_.node_type(node);
}

const RRSpatialLookup& RRGraphView::node_lookup() const {
  /* Return a constant object rather than a writable one */
  return const_cast<const RRSpatialLookup&>(node_lookup_);
}
