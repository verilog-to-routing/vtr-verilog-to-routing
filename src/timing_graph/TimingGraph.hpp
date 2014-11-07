#ifndef TIMINGGRAPH_HPP
#define TIMINGGRAPH_HPP
#include <vector>

#include "TimingNode.hpp"


class TimingGraph {
    public:

        TimingNode& node(int id) { return nodes_[id]; }
        const TimingNode& node(int id) const { return nodes_[id]; }
        int num_nodes() const { return nodes_.size(); }
        void add_node(const TimingNode& new_node) {nodes_.push_back(new_node);}

        int num_levels() const { return node_levels_.size(); }
        void set_num_levels(const int nlevels) { node_levels_ = std::vector<std::vector<int>>(nlevels); }

        const std::vector<int>& level(int level_id) const { return node_levels_[level_id]; }
        void add_level(const int level_id, const std::vector<int>& level_node_ids) {node_levels_[level_id] = level_node_ids;}

        void print() {
            for(int i = 0; i < nodes_.size(); i++) {
                printf("Node: %d\n", i);
                nodes_[i].print();
            }

            for(int i = 0; i < node_levels_.size(); i++) {
                printf("Level: %d\n", i);
                for(int j = 0; j < node_levels_[i].size(); j++) {
                    printf("    %d\n", node_levels_[i][j]);
                }
            }
        }

    private:
        std::vector<TimingNode> nodes_;

        std::vector<std::vector<int>> node_levels_;
};

#endif
