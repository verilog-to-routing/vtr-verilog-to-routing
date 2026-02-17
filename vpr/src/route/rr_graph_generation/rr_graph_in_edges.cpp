#include "rr_graph_in_edges.h"

#include "vtr_assert.h"
#include "vtr_log.h"

void RRGraphInEdges::init(const RRGraphView& rr_graph) {
    node_in_edges_.clear();
    node_in_edges_.resize(rr_graph.num_nodes());

    rr_graph.rr_nodes().for_each_edge(
        [&](RREdgeId edge, RRNodeId /*src*/, RRNodeId sink) {
            node_in_edges_[sink].push_back(edge);
        });

    is_built_ = true;
}

const std::vector<RREdgeId>& RRGraphInEdges::node_in_edges(RRNodeId node) const {
    VTR_ASSERT(is_built_);
    return node_in_edges_[node];
}

void RRGraphInEdges::clear() {
    node_in_edges_.clear();
    is_built_ = false;
}
