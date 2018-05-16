#ifndef TATUM_TIMING_PATH_COLLECTOR_HPP
#define TATUM_TIMING_PATH_COLLECTOR_HPP
#include "tatum/timing_analyzers_fwd.hpp"
#include "TimingPath.hpp"
#include "SkewPath.hpp"

namespace tatum {

    class TimingPathCollector {
        public:
            std::vector<TimingPath> collect_worst_setup_timing_paths(const TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const;
            std::vector<TimingPath> collect_worst_hold_timing_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const;

            std::vector<SkewPath> collect_worst_setup_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const;
            std::vector<SkewPath> collect_worst_hold_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const;
    };

} //namespace

#endif
