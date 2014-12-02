#pragma once
#include <vector>

class TimingGraph; //Forward declaration

class TimingAnalyzer {
    public:
        virtual std::vector<float> calculate_timing(TimingGraph& timing_graph) = 0;
        virtual void reset_timing(TimingGraph& timing_graph) = 0;
};
