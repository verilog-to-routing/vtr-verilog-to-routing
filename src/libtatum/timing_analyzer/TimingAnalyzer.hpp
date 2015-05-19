#pragma once
#include <vector>

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"

class TimingTags;

struct ta_runtime;

class TimingAnalyzer {
    public:
        virtual ta_runtime calculate_timing(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) = 0;

        virtual const TimingTags& data_tags(NodeId node_id) const = 0;
        virtual const TimingTags& clock_tags(NodeId node_id) const = 0;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
