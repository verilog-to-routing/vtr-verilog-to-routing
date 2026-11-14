#ifndef TATUM_TIMING_PATH_COLLECTOR_HPP
#define TATUM_TIMING_PATH_COLLECTOR_HPP
#include "tatum/timing_analyzers_fwd.hpp"
#include "TimingPath.hpp"
#include "SkewPath.hpp"
#include "TimingPathCategory.hpp"

namespace tatum {

    /**
     * @brief Collects the worst timing/skew paths.
     *
     * If a category filter is provided only paths in the accepted categories are collected,
     * and npaths limits the number of paths collected after filtering.
     *
     * Note that the analyzers record only the worst arrival time per clock domain pair at
     * each node, so each endpoint has at most one path per clock domain pair. That path's
     * startpoint (and hence category) is the startpoint of the worst path to the endpoint.
     * For example, if inputs and registers are constrained on the same clock, a register
     * endpoint whose worst path starts at an input will only appear as an in2reg path.
     */
    class TimingPathCollector {
        public:
            std::vector<TimingPath> collect_worst_setup_timing_paths(const TimingGraph& timing_graph, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths,
                                                                     const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
            std::vector<TimingPath> collect_worst_hold_timing_paths(const TimingGraph& timing_graph, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths,
                                                                    const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;

            std::vector<SkewPath> collect_worst_setup_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::SetupTimingAnalyzer& setup_analyzer, size_t npaths,
                                                                 const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
            std::vector<SkewPath> collect_worst_hold_skew_paths(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const tatum::HoldTimingAnalyzer& hold_analyzer, size_t npaths,
                                                                const TimingPathCategoryFilter& category_filter=TimingPathCategoryFilter::all()) const;
    };

} //namespace

#endif
