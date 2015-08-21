#pragma once
#include <cilk/cilk_api.h>

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelNoDependancyTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::forward_traversal() {

    //NOTE: ignoring levels, just running in parallel across all nodes
    cilk_for(int node_id = 0; node_id < this->tg_.num_nodes(); node_id++) {
        TagPoolType& tag_pool = *(this->tag_pools_[__cilkrts_get_worker_number()]);
        this->forward_traverse_node(tag_pool, node_id);
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelNoDependancyTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::backward_traversal() {

    //NOTE: ignoring levels, just running in parallel across all nodes
    cilk_for(int node_id = 0; node_id < this->tg_.num_nodes(); node_id++) {
        TagPoolType& tag_pool = *(this->tag_pools_[__cilkrts_get_worker_number()]);
        this->backward_traverse_node(tag_pool, node_id);
    }
}
