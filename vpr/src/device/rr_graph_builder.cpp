#include "rr_graph_builder.h"

RRGraphBuilder::RRGraphBuilder(t_rr_graph_storage* node_storage,
                               RRSpatialLookup* node_lookup)
    : node_storage_(*node_storage)
    , node_lookup_(*node_lookup) {
}

t_rr_graph_storage& RRGraphBuilder::node_storage() {
    return node_storage_;
}

RRSpatialLookup& RRGraphBuilder::node_lookup() {
    return node_lookup_;
}
