#ifndef TIMINGGRAPH_HPP
#define TIMINGGRAPH_HPP
#include <vector>
#include <iostream>
#include <iomanip>

#include "TimingNode.hpp"
#include "Time.hpp"


class TimingGraph {
    public:
        typedef int NodeId;

        TimingNode& node(NodeId id) { return nodes_[id]; }
        const TimingNode& node(NodeId id) const { return nodes_[id]; }
        NodeId num_nodes() const { return nodes_.size(); }
        void add_node(const TimingNode& new_node);

        NodeId num_levels() const { return node_levels_.size(); }
        void set_num_levels(const NodeId nlevels) { node_levels_ = std::vector<std::vector<NodeId>>(nlevels); }

        const std::vector<int>& level(NodeId level_id) const { return node_levels_[level_id]; }
        void add_level(const NodeId level_id, const std::vector<int>& level_node_ids) {node_levels_[level_id] = level_node_ids;}

        const std::vector<int>& primary_inputs() const { return node_levels_[0]; }
        const std::vector<int>& primary_outputs() const { return primary_outputs_; }

        void print() {
            //Save io state
            std::ios_base::fmtflags saved_flags = std::cout.flags();
            std::streamsize prec = std::cout.precision();
            std::streamsize width = std::cout.width();

            std::streamsize num_width = 10;
            std::streamsize num_prec = 3;
            std::cout.precision(num_prec);
            std::cout << std::scientific;

            for(decltype(nodes_.size()) i = 0; i < nodes_.size(); i++) {
                std::cout << "Node: " << std::setw(4) << i;
                std::cout << " T_arr: " << std::setw(num_width) << nodes_[i].arrival_time();
                std::cout << " T_req: " << std::setw(num_width) << nodes_[i].required_time();
                std::cout << " Type: " << nodes_[i].type();
                std::cout << std::endl;
                //nodes_[i].print();
            }

/*
 *            for(decltype(node_levels_.size()) i = 0; i < node_levels_.size(); i++) {
 *                std::cout << "Level : " << i << std::endl;
 *
 *                for(decltype(node_levels_[i].size()) j = 0; j < node_levels_[i].size(); j++) {
 *                    NodeId id = node_levels_[i][j];
 *                    std::cout << "  Node: " << id << " Type: " << nodes_[id].type() << std::endl;
 *
 *                }
 *
 *            }
 */
            //Reset I/O format
            std::cout.flags(saved_flags);
            std::cout.precision(prec);
            std::cout.width(width);
        }

    private:
        std::vector<TimingNode> nodes_;

        std::vector<std::vector<NodeId>> node_levels_;
        std::vector<NodeId> primary_outputs_;
};

#endif
