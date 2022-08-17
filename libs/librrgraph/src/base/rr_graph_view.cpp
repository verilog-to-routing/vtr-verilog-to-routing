#include "rr_graph_view.h"
#include "rr_node.h"
#include "physical_types.h"

RRGraphView::RRGraphView(const t_rr_graph_storage& node_storage,
                         const RRSpatialLookup& node_lookup,
                         const MetadataStorage<int>& rr_node_metadata,
                         const MetadataStorage<std::tuple<int, int, short>>& rr_edge_metadata,
                         const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                         const std::vector<t_rr_rc_data>& rr_rc_data,
                         const vtr::vector<RRSegmentId, t_segment_inf>& rr_segments,
                         const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
                         const vtr::vector<RRNodeId, std::vector<RREdgeId>>& node_in_edges)
    : node_storage_(node_storage)
    , node_lookup_(node_lookup)
    , rr_node_metadata_(rr_node_metadata)
    , rr_edge_metadata_(rr_edge_metadata)
    , rr_indexed_data_(rr_indexed_data)
    , rr_rc_data_(rr_rc_data)
    , rr_segments_(rr_segments)
    , rr_switch_inf_(rr_switch_inf) 
    , node_in_edges_(node_in_edges) {
}

std::vector<RREdgeId> RRGraphView::node_in_edges(RRNodeId node) const {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    if (node_in_edges_.empty()) {
        return std::vector<RREdgeId>();
    }
    return node_in_edges_[node];
}

std::vector<RREdgeId> RRGraphView::node_configurable_in_edges(RRNodeId node) const {
    /* Note: This is not efficient in runtime, should sort edges by configurability when allocating the array! */
    VTR_ASSERT(size_t(node) < node_storage_.size());
    std::vector<RREdgeId> ret_edges;
    if (node_in_edges_.empty()) {
        return ret_edges;
    }
    for (const RREdgeId& edge : node_in_edges_[node]) {
        if (rr_switch_inf_[edge_switch(edge)].configurable()) {
            ret_edges.push_back(edge);
        }
    }
    return ret_edges;
}

std::vector<RREdgeId> RRGraphView::node_non_configurable_in_edges(RRNodeId node) const {
    /* Note: This is not efficient in runtime, should sort edges by configurability when allocating the array! */
    VTR_ASSERT(size_t(node) < node_storage_.size());
    std::vector<RREdgeId> ret_edges;
    if (node_in_edges_.empty()) {
        return ret_edges;
    }
    for (const RREdgeId& edge : node_in_edges_[node]) {
        if (!rr_switch_inf_[edge_switch(edge)].configurable()) {
            ret_edges.push_back(edge);
        }
    }
    return ret_edges;
}

std::vector<RREdgeId> RRGraphView::find_edges(const RRNodeId& src_node, const RRNodeId& des_node) const {
    std::vector<RREdgeId> edge_list;
    for (auto iedge : node_out_edges(src_node)) {
        if (edge_sink_node(RREdgeId(iedge)) == des_node) {
            edge_list.push_back(RREdgeId(iedge));
        }
    }
    return edge_list;
}

RRSegmentId RRGraphView::node_segment(RRNodeId node) const {
    RRIndexedDataId cost_index = node_cost_index(node);
    return RRSegmentId(rr_indexed_data_[cost_index].seg_index);
}
