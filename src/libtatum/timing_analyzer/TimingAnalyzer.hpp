#pragma once
#include <vector>

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"

class TimingTags;

struct ta_runtime;

class TimingAnalyzer {
    public:
        TimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints)
            : tg_(timing_graph)
            , tc_(timing_constraints) {}

        virtual ta_runtime calculate_timing() = 0;
        virtual void reset_timing() = 0;

    protected:
        const TimingGraph& tg_;
        const TimingConstraints& tc_;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
