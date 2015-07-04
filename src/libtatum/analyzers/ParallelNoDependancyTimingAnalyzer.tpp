#pragma once
#include <cilk/cilk_api.h>

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelNoDependancyTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {

    //NOTE: ignoring levels, just running in parallel across all nodes
    cilk_for(int node_id = 0; node_id < timing_graph.num_nodes(); node_id++) {
        TagPoolType& tag_pool = *(this->tag_pools_[__cilkrts_get_worker_number()]);
        this->forward_traverse_node(tag_pool, timing_graph, timing_constraints, node_id);
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelNoDependancyTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::backward_traversal(const TimingGraph& timing_graph) {

    //NOTE: ignoring levels, just running in parallel across all nodes
    cilk_for(int node_id = 0; node_id < timing_graph.num_nodes(); node_id++) {
        this->backward_traverse_node(timing_graph, node_id);
    }
}
