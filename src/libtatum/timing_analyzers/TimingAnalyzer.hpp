#pragma once
#include <vector>

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "analysis_types.hpp"

class TimingTags;

struct ta_runtime;

/*
 * TimingAnalyzer represents a interface class for all timing analyzers
 */
template<class AnalysisType, class DelayCalcType>
class TimingAnalyzer : public AnalysisType {
    public:
        typedef DelayCalcType delay_calculator_type;
        typedef AnalysisType analysis_type;

        virtual ~TimingAnalyzer() {};
        virtual void calculate_timing() = 0;
        virtual void reset_timing() = 0;

        virtual const DelayCalcType& delay_calculator() = 0;
        virtual std::map<std::string, double> profiling_data() = 0;
};

struct ta_runtime {
    float pre_traversal;
    float fwd_traversal;
    float bck_traversal;
};
