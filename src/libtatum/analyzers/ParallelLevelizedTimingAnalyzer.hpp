#pragma once

#include "timing_graph_fwd.hpp"
#include "SerialTimingAnalyzer.hpp"

/*
 * The ParallelLevelizedTimingAnalyzer implements the TimingAnalyzer interface, providing
 * a parallel (multi-core) timing analyzer that runs faster than the standard SerialTimingAnalyzer.
 *
 * Like the SerialTimingAnalyzer the timing graph is walked in a levelized manner where each
 * preceeding level is fully evaluated before the next is processed.  However, within each level
 * nodes are processed in parallel.
 *
 * This allows a reasonably parrallelization provided levels are sufficiently wide. If however,
 * levels are too small this can result in an extremely fine-grained parallelization where the
 * parallelization overhead overwhelms any potential speed-up.  This poor behaviour is avoided
 * by evaluating serially levels that not 'sufficiently' large.
 */

template<class AnalysisType, class DelayCalcType, class TagPoolType=MemoryPool>
class ParallelLevelizedTimingAnalyzer : public SerialTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType> {
    public:
        ParallelLevelizedTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        ~ParallelLevelizedTimingAnalyzer() override;
    protected:
        /*
         * Setup the timing graph.
         */
        void pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) override;

        /*
         * Propogate arrival times
         */
        void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) override;

        /*
         * Propogate required times
         */
        void backward_traversal(const TimingGraph& timing_graph) override;

        //We use multiple tag pools to reduce contention between threads
        std::vector<MemoryPool*> tag_pools_;

        //Level width thresholds beyond which to execute a parallel traversal
        //  For very narrow levels the overhead of (finegrained) parallel execution
        //  outweighs any potential speed-up, so it is faster to (explicitly) run serially
        size_t parallel_threshold_fwd_;
        size_t parallel_threshold_bck_;
};

#include "ParallelLevelizedTimingAnalyzer.tpp"
