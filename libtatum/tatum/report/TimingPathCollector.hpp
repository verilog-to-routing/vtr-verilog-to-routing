#ifndef TATUM_TIMING_PATH_COLLECTOR_HPP
#define TATUM_TIMING_PATH_COLLECTOR_HPP
#include "tatum/timing_analyzers_fwd.hpp"
#include "TimingPath.hpp"

namespace tatum {

    class TimingPathCollector {
        public:
            std::vector<TimingPath> collect_worst_setup_paths(const TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths) const;
            std::vector<TimingPath> collect_worst_hold_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths) const;
    };

} //namespace

#endif
