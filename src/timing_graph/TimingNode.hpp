#ifndef TIMINGNODE_HPP
#define TIMINGNODE_HPP
#include <vector>

#include "TimingEdge.hpp"
#include "Time.hpp"

class TimingNode {
    public:
        int num_out_edges() const { return out_edges_.size(); }

        TimingEdge& out_edge(int idx) { return out_edges_[idx]; }
        const TimingEdge& out_edge(int idx) const { return out_edges_[idx]; }
        void add_out_edge(const TimingEdge& edge) { out_edges_.push_back(edge); }

        void print() {
            for(auto& edge : out_edges_) {
                edge.print();
            }
        }
    private:
        std::vector<TimingEdge> out_edges_; //Timing edges driven by this node

        Time T_arr_; //Data arrival time at this node
        Time T_req_; //Data required arrival time at this node
};

#endif
