#pragma once
#include "timing_graph_fwd.hpp"

class TimingConstraints {
    void add_clock_constraint(DomainId src_domain, DomainId sink_domain, float constraint) {}
    void add_input_constraint(NodeId node_id, float constraint) {}
    void add_output_constraint(NodeId node_id, float constraint) {}
};
