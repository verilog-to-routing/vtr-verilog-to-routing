#include "rr_graph_view.h"

RRGraphView::RRGraphView(const t_rr_graph_storage& node_storage,
                         const RRSpatialLookup& node_lookup)
    : node_storage_(node_storage)
    , node_lookup_(node_lookup) {
}

t_rr_type RRGraphView::node_type(RRNodeId node) const {
    return node_storage_.node_type(node);
}

const RRSpatialLookup& RRGraphView::node_lookup() const {
    return node_lookup_;
}
