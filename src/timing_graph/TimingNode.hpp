#ifndef TIMINGNODE_HPP
#define TIMINGNODE_HPP

#include "TimingEdge.hpp"
#include "Time.hpp"

class TimingNode {
    private:
        std::vector<TimingEdge> out_edges_; //Timing edges driven by this node

        PropInfo arr_; //Arrival time at this node
        PropInfo req_; //Required arrival time at this node
};

#endif
