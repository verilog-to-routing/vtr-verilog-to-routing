#ifndef TIMINGNODE_HPP
#define TIMINGNODE_HPP

#include "TimingEdge.hpp"
#include "Time.hpp"

class TimingNode {
    public:
        int num_out_edges() const { return out_edges_.size(); }

        TimingEdge& out_edge(int idx) { return out_edges_[i]; }
        const TimingEdge& out_edge(int idx) const { return out_edges_[i]; }

    private:
        std::vector<TimingEdge> out_edges_; //Timing edges driven by this node

        Time T_arr_; //Data arrival time at this node
        Time T_req_; //Data required arrival time at this node
};

#endif
