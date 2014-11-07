#ifndef TIMINGGRAPH_HPP
#define TIMINGGRAPH_HPP
#include "TimingNode.hpp"


class TimingGraph {
    public:

        TimingNode& node(int id) { return nodes_[id]; }
        const TimingNode& node(int id) const { return nodes_[id]; }
        add_node(const TimingNode& node) {nodes_.push_back(node);}

        int num_levels() const { return node_levels_.size(); }
        const std::vector<int>& level(int level_id) const { return node_levels_[i]; }
        add_level(const int level, const std::vector<int> level_node_ids) {node_levels_[i] = level_node_ids;}

    private:
        std::vector<TimingNode> nodes_;

        std::vector<std::vector<int>> node_levels_;
};

#endif
