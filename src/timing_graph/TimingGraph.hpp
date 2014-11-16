#pragma once

#include <vector>
#include <iostream>
#include <iomanip>

#include "Time.hpp"

#include "timing_graph_fwd.hpp"

#include "TimingNode.hpp"
#include "TimingEdge.hpp"


class TimingGraph {
    public:
        TimingNode& node(NodeId id) { return nodes_[id]; }
        const TimingNode& node(NodeId id) const { return nodes_[id]; }
        NodeId num_nodes() const { return nodes_.size(); }
        NodeId add_node(const TimingNode& new_node);

        TimingEdge& edge(EdgeId id) { return edges_[id]; }
        const TimingEdge& edge(EdgeId id) const { return edges_[id]; }
        EdgeId num_edges() const { return edges_.size(); }
        EdgeId add_edge(const TimingEdge& new_edge);

        NodeId num_levels() const { return node_levels_.size(); }
        void set_num_levels(const NodeId nlevels) { node_levels_ = std::vector<std::vector<NodeId>>(nlevels); }

        const std::vector<NodeId>& level(NodeId level_id) const { return node_levels_[level_id]; }
        void add_level(const NodeId level_id, const std::vector<NodeId>& level_node_ids) {node_levels_[level_id] = level_node_ids;}

        const std::vector<NodeId>& primary_inputs() const { return node_levels_[0]; }
        const std::vector<NodeId>& primary_outputs() const { return primary_outputs_; }

    private:
        std::vector<TimingNode> nodes_;
        std::vector<TimingEdge> edges_;

        std::vector<std::vector<NodeId>> node_levels_;
        std::vector<NodeId> primary_outputs_;
};
