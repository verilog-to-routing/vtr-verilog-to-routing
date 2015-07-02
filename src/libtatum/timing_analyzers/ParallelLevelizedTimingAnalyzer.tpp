#pragma once
#include "cilk_safe.hpp"
#include <cilk/cilk_api.h>

#include "ParallelLevelizedTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

template<class AnalysisType, class DelayCalcType, class TagPoolType>
ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::ParallelLevelizedTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator)
    : SerialTimingAnalyzer<AnalysisType,DelayCalcType, TagPoolType>(timing_graph, timing_constraints, delay_calculator) {

    int nworkers = __cilkrts_get_nworkers();

    for(int i = 0; i < nworkers; i++) {
        tag_pools_.push_back(new TagPoolType(sizeof(TimingTag)));
    }
}
template<class AnalysisType, class DelayCalcType, class TagPoolType>
ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::~ParallelLevelizedTimingAnalyzer() {
    for(size_t i = 0; i < tag_pools_.size(); i++) {
        delete tag_pools_[i];
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     */
    const std::vector<NodeId>& primary_inputs = timing_graph.primary_inputs();

    //To little work to overcome parallel overhead
    for(int node_idx = 0; node_idx < (int) primary_inputs.size(); node_idx++) {
        NodeId node_id = primary_inputs[node_idx];
        AnalysisType::pre_traverse_node(*(this->tag_pools_[0]), timing_graph, timing_constraints, node_id);
    }

}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    using namespace std::chrono;

    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        auto fwd_level_start = high_resolution_clock::now();

        const std::vector<NodeId>& level = timing_graph.level(level_idx);

        cilk_for(int node_idx = 0; node_idx < level.size(); node_idx++) {
            NodeId node_id = level[node_idx];
            TagPoolType& tag_pool = *tag_pools_[__cilkrts_get_worker_number()];
            this->forward_traverse_node(tag_pool, timing_graph, timing_constraints, node_id);
        }

        auto fwd_level_end = high_resolution_clock::now();
        std::stringstream msg;
        msg << "fwd_level_" << level_idx;
        this->perf_data_[msg.str()] = duration_cast<duration<double>>(fwd_level_end - fwd_level_start).count();
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::backward_traversal(const TimingGraph& timing_graph) {
    using namespace std::chrono;

    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        auto bck_level_start = high_resolution_clock::now();

        const std::vector<NodeId>& level = timing_graph.level(level_idx);

        cilk_for(int node_idx = 0; node_idx < level.size(); node_idx++) {
            NodeId node_id = level[node_idx];
            this->backward_traverse_node(timing_graph, node_id);
        }

        auto bck_level_end = high_resolution_clock::now();
        std::stringstream msg;
        msg << "bck_level_" << level_idx;
        this->perf_data_[msg.str()] = duration_cast<duration<double>>(bck_level_end - bck_level_start).count();
    }
}
