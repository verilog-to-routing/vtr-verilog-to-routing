#pragma once
#include <vector>

#include "timing_graph_fwd.hpp"

class TimingTags;

struct ta_runtime;

class TimingAnalyzer {
    public:
        virtual ta_runtime calculate_timing(const TimingGraph& timing_graph) = 0;

        virtual const TimingTags& arrival_tags(NodeId node_id) const = 0;
        virtual const TimingTags& required_tags(NodeId node_id) const = 0;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
