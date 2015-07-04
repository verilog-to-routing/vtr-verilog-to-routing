#pragma once

#include <vector>

#include "timing_graph_fwd.hpp"
#include "Time.hpp"

class TimingEdge {
    public:
        TimingEdge(Time val, NodeId new_from_node, NodeId new_to_node)
            : delay_(val)
            , from_node_(new_from_node) 
            , to_node_(new_to_node) {}

        int to_node_id() const {return to_node_;}
        void set_to_node_id(int node) {to_node_ = node;}

        int from_node_id() const {return from_node_;}
        void set_from_node_id(int node) {from_node_ = node;}

        Time delay() const {return delay_;}
        void set_delay(Time delay_val) {delay_ = delay_val;}

    private:
        Time delay_;
        int from_node_;
        int to_node_;
};

