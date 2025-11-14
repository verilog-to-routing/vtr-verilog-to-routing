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
                         const vtr::vector<RRNodeId, std::vector<RREdgeId>>& node_in_edges,
                         const vtr::vector<RRNodeId, std::vector<short>>& node_tileable_track_nums)
    : node_storage_(node_storage)
    , node_lookup_(node_lookup)
    , rr_node_metadata_(rr_node_metadata)
    , rr_edge_metadata_(rr_edge_metadata)
    , rr_indexed_data_(rr_indexed_data)
    , rr_rc_data_(rr_rc_data)
    , rr_segments_(rr_segments)
    , rr_switch_inf_(rr_switch_inf) 
    , node_in_edges_(node_in_edges)
    , node_tileable_track_nums_(node_tileable_track_nums) {
}

std::vector<RREdgeId> RRGraphView::node_in_edges(RRNodeId node) const {
    VTR_ASSERT(size_t(node) < node_storage_.size());
    if (node_in_edges_.empty()) {
        return std::vector<RREdgeId>();
    }
    return node_in_edges_[node];
}

std::vector<RREdgeId> RRGraphView::node_configurable_in_edges(RRNodeId node) const {
    // Note: Should sort edges by configurability when allocating the array!
    VTR_ASSERT(size_t(node) < node_storage_.size());
    std::vector<RREdgeId> ret_edges;
    if (node_in_edges_.empty()) {
        return ret_edges;
    }
    for (const RREdgeId& edge : node_in_edges_[node]) {
        if (rr_switch_inf_[RRSwitchId(edge_switch(edge))].configurable()) {
            ret_edges.push_back(edge);
        }
    }
    return ret_edges;
}

std::vector<RREdgeId> RRGraphView::node_non_configurable_in_edges(RRNodeId node) const {
    // Note: Should sort edges by configurability when allocating the array!
    VTR_ASSERT(size_t(node) < node_storage_.size());
    std::vector<RREdgeId> ret_edges;
    if (node_in_edges_.empty()) {
        return ret_edges;
    }
    for (const RREdgeId& edge : node_in_edges_[node]) {
        if (!rr_switch_inf_[RRSwitchId(edge_switch(edge))].configurable()) {
            ret_edges.push_back(edge);
        }
    }
    return ret_edges;
}

std::vector<RREdgeId> RRGraphView::find_edges(RRNodeId src_node, RRNodeId des_node) const {
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

size_t RRGraphView::in_edges_count() const {
    size_t edge_count = 0;
    for (const std::vector<RREdgeId>& edge_list : node_in_edges_) {
        edge_count += edge_list.size();
    }
    return edge_count;
}

bool RRGraphView::validate_in_edges() const {
    VTR_ASSERT(node_in_edges_.size() == node_storage_.size());
    size_t num_err = 0;
    // For each edge, validate that
    // - The source node is in the fan-in edge list of the destination node
    // - The sink node is in the fan-out edge list of the source node
    for (RRNodeId curr_node : vtr::StrongIdRange<RRNodeId>(RRNodeId(0), RRNodeId(node_storage_.size()))) {
        // curr_node ---> des_node 
        //           <-?-            check if the incoming edge is correct or not
        for (t_edge_size iedge : node_storage_.edges(curr_node)) {
            RRNodeId des_node = node_storage_.edge_sink_node(node_storage_.edge_id(curr_node, iedge));
            std::vector<RRNodeId> des_fanin_nodes;
            for (RREdgeId next_edge : node_in_edges(des_node)) {
                RRNodeId prev_edge_des_node = node_storage_.edge_source_node(next_edge);
                des_fanin_nodes.push_back(prev_edge_des_node);
            }
            if (des_fanin_nodes.end() == std::find(des_fanin_nodes.begin(), des_fanin_nodes.end(), curr_node)) {
                VTR_LOG_ERROR("Node '%s' does not appear in the fan-in edges of Node '%s', while does drive it in its fan-out list\n",
                              node_coordinate_to_string(curr_node).c_str(), node_coordinate_to_string(des_node).c_str());
                num_err++;
            }
        }
        // src_node -?-> curr_node
        //          <---           check if the fan-out edge is correct or not
        for (RREdgeId iedge : node_in_edges(curr_node)) {
            RRNodeId src_node = node_storage_.edge_source_node(iedge);
            std::vector<RRNodeId> src_fanout_nodes;
            for (t_edge_size prev_edge : node_storage_.edges(src_node)) {
                RRNodeId prev_edge_des_node = node_storage_.edge_sink_node(node_storage_.edge_id(src_node, prev_edge));
                src_fanout_nodes.push_back(prev_edge_des_node);
            }
            if (src_fanout_nodes.end() == std::find(src_fanout_nodes.begin(), src_fanout_nodes.end(), curr_node)) {
                VTR_LOG_ERROR("Node '%s' does not appear in the fan-out edges of Node '%s', while does exist in its fan-in list\n",
                              node_coordinate_to_string(curr_node).c_str(), node_coordinate_to_string(src_node).c_str());
                num_err++;
            }
        }
    }
    if (num_err) {
      VTR_LOG_ERROR("Found %ld errors when validating incoming edges for routing resource graph\n", num_err);
      return false;
    }
    return true;
}
