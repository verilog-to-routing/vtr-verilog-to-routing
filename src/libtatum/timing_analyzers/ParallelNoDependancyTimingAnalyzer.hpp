#pragma once

#include "timing_graph_fwd.hpp"
#include "ParallelLevelizedTimingAnalyzer.hpp"

template<class AnalysisType, class DelayCalcType, class TagPoolType=MemoryPool>
class ParallelNoDependancyTimingAnalyzer : public ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType> {
    public:
        ParallelNoDependancyTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator): ParallelLevelizedTimingAnalyzer<AnalysisType,DelayCalcType,TagPoolType>(timing_graph, timing_constraints, delay_calculator) {}
    protected:
        /*
         * Propogate arrival times
         */
        void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) override;

        /*
         * Propogate required times
         */
        void backward_traversal(const TimingGraph& timing_graph) override;
};

#include "ParallelNoDependancyTimingAnalyzer.tpp"
