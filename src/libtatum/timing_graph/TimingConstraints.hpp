#pragma once
#include "timing_graph_fwd.hpp"

#include <iostream>

class TimingConstraints {
    public:
        void add_clock_constraint(DomainId src_domain, DomainId sink_domain, float constraint) { std::cout << "SRC: " << src_domain << " SINK: " << sink_domain << " Constraint: " << constraint << std::endl; }
        void add_input_constraint(NodeId node_id, float constraint) { std:: cout << "Node: " << node_id << " Input_Constraint: " << constraint << std::endl; }
        void add_output_constraint(NodeId node_id, float constraint) { std::cout << "Node: " << node_id << " Output_Constraint: " << constraint << std::endl; }
};
