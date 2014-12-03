#pragma once
#include <vector>

class TimingGraph; //Forward declaration

struct ta_runtime;

class TimingAnalyzer {
    public:
        virtual ta_runtime calculate_timing(TimingGraph& timing_graph) = 0;
        virtual void reset_timing(TimingGraph& timing_graph) = 0;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
