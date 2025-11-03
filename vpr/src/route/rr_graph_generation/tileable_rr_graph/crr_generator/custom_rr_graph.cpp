#include "custom_rr_graph.h"

#include "vtr_log.h"

namespace crrgenerator {

// RRGraph implementation

void RRGraph::add_node(const RRNode& node) {
    nodes_.push_back(node);
    update_node_index(node.get_id(), nodes_.size() - 1);
}

void RRGraph::add_node(NodeId id, e_rr_type type, int capacity, const std::string& direction, const Location& location, const NodeTiming& timing, const NodeSegmentId& segment_id) {
    add_node(RRNode(id, type, capacity, direction, location, timing, segment_id));
}

const RRNode* RRGraph::get_node(NodeId id) const {
    auto it = node_id_to_index_.find(id);
    return (it != node_id_to_index_.end()) ? &nodes_[it->second] : nullptr;
}

RRNode* RRGraph::get_node(NodeId id) {
    auto it = node_id_to_index_.find(id);
    return (it != node_id_to_index_.end()) ? &nodes_[it->second] : nullptr;
}

void RRGraph::add_edge(const RREdge& edge) { edges_.push_back(edge); }

void RRGraph::add_edge(NodeId src_node, NodeId sink_node, SwitchId switch_id) {
    add_edge(RREdge(src_node, sink_node, switch_id));
}

void RRGraph::add_channel(const Channel& channel) {
    channels_.push_back(channel);
}

void RRGraph::add_channel(int max_width, int x_max, int x_min, int y_max, int y_min) {
    add_channel(Channel(max_width, x_max, x_min, y_max, y_min));
}

void RRGraph::add_xy_list(const XYList& xy_list) {
    xy_lists_.push_back(xy_list);
}

void RRGraph::add_xy_list(XYList::Type type, int index, int info) {
    add_xy_list(XYList(type, index, info));
}

void RRGraph::add_switch(const Switch& switch_obj) {
    switches_.push_back(switch_obj);
    update_switch_index(switch_obj.get_id(), switches_.size() - 1);
}

void RRGraph::add_switch(SwitchId id, const std::string& name, const std::string& type, const Timing& timing, const Sizing& sizing) {
    add_switch(Switch(id, name, type, timing, sizing));
}

const Switch* RRGraph::get_switch(SwitchId id) const {
    auto it = switch_id_to_index_.find(id);
    return (it != switch_id_to_index_.end()) ? &switches_[it->second] : nullptr;
}

Switch* RRGraph::get_switch(SwitchId id) {
    auto it = switch_id_to_index_.find(id);
    return (it != switch_id_to_index_.end()) ? &switches_[it->second] : nullptr;
}

void RRGraph::add_segment(const Segment& segment) {
    segments_.push_back(segment);
    update_segment_index(segment.get_id(), segments_.size() - 1);
}

void RRGraph::add_segment(SegmentId id, const std::string& name, int length, const std::string& res_type) {
    add_segment(Segment(id, name, length, res_type));
}

const Segment* RRGraph::get_segment(SegmentId id) const {
    auto it = segment_id_to_index_.find(id);
    return (it != segment_id_to_index_.end()) ? &segments_[it->second] : nullptr;
}

void RRGraph::add_block_type(const BlockType& block_type) {
    block_types_.push_back(block_type);
}

void RRGraph::add_block_type(int id, const std::string& name, int height, int width, const std::vector<PinClass>& pin_classes) {
    add_block_type(BlockType(id, name, height, width, pin_classes));
}

void RRGraph::add_grid_loc(const GridLoc& grid_loc) {
    grid_locs_.push_back(grid_loc);
}

void RRGraph::add_grid_loc(int type_id, int height_offset, int width_offset, int x, int y, int layer) {
    add_grid_loc(GridLoc(type_id, height_offset, width_offset, x, y, layer));
}

void RRGraph::clear() {
    nodes_.clear();
    edges_.clear();
    switches_.clear();
    segments_.clear();
    node_id_to_index_.clear();
    switch_id_to_index_.clear();
    segment_id_to_index_.clear();
    tool_name_.clear();
    tool_version_.clear();
    tool_comment_.clear();
}

void RRGraph::print_statistics() const {
    VTR_LOG("=== CRR Generator RR Graph Statistics ===\n");
    VTR_LOG("Nodes: %zu\n", nodes_.size());
    VTR_LOG("Edges: %zu\n", edges_.size());
    VTR_LOG("Switches: %zu\n", switches_.size());
    VTR_LOG("Segments: %zu\n", segments_.size());

    // Count nodes by type
    std::map<e_rr_type, size_t> type_counts;
    for (const auto& node : nodes_) {
        type_counts[node.get_type()]++;
    }

    for (const auto& [type, count] : type_counts) {
        VTR_LOG("  %s nodes: %zu\n", rr_node_typename[type], count);
    }

    VTR_LOG("Tool: %s v%s\n", tool_name_.c_str(), tool_version_.c_str());
    if (!tool_comment_.empty()) {
        VTR_LOG("Comment: %s\n", tool_comment_.c_str());
    }
}

std::unordered_set<NodeId> RRGraph::get_source_nodes() const {
    std::unordered_set<NodeId> result;
    for (const auto& node : nodes_) {
        if (node.get_type() == e_rr_type::SOURCE) {
            result.insert(node.get_id());
        }
    }
    return result;
}

std::unordered_set<NodeId> RRGraph::get_sink_nodes() const {
    std::unordered_set<NodeId> result;
    for (const auto& node : nodes_) {
        if (node.get_type() == e_rr_type::SINK) {
            result.insert(node.get_id());
        }
    }
    return result;
}

std::unordered_set<NodeId> RRGraph::get_ipin_nodes() const {
    std::unordered_set<NodeId> result;
    for (const auto& node : nodes_) {
        if (node.get_type() == e_rr_type::IPIN) {
            result.insert(node.get_id());
        }
    }
    return result;
}

std::unordered_set<NodeId> RRGraph::get_opin_nodes() const {
    std::unordered_set<NodeId> result;
    for (const auto& node : nodes_) {
        if (node.get_type() == e_rr_type::OPIN) {
            result.insert(node.get_id());
        }
    }
    return result;
}

void RRGraph::set_tool_info(const std::string& name, const std::string& version, const std::string& comment) {
    tool_name_ = name;
    tool_version_ = version;
    tool_comment_ = comment;
}

void RRGraph::update_node_index(NodeId id, size_t index) {
    node_id_to_index_[id] = index;
}

void RRGraph::update_switch_index(SwitchId id, size_t index) {
    switch_id_to_index_[id] = index;
}

void RRGraph::update_segment_index(SegmentId id, size_t index) {
    segment_id_to_index_[id] = index;
}

} // namespace crrgenerator
