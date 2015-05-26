#pragma once
#include <vector>

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "analysis_types.hpp"

class TimingTags;

struct ta_runtime;


template<class AnalysisType>
class TimingAnalyzer : public AnalysisType {
    public:
        virtual ta_runtime calculate_timing() = 0;
        virtual void reset_timing() = 0;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
