#pragma once
#include "cilk_safe.hpp"
#include <cilk/cilk_api.h>

#include "ParallelLevelizedTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

template<class AnalysisType, class DelayCalcType, class TagPoolType>
ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::ParallelLevelizedTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator)
    : SerialTimingAnalyzer<AnalysisType,DelayCalcType, TagPoolType>(timing_graph, timing_constraints, delay_calculator)
    //These thresholds control how fine-grained we allow the parallelization
    //
    //The ideal values will vary on different machines, however default value
    // is not set too aggressivley, and should be reasonable on other systems.
    //
    //These values were set experimentally on an Intel E5-1620 CPU,
    // and yeilded the following self-relative parallel speed-ups
    // compared to a value of 0 (i.e. always run parrallel).
    //
    //                  0           400
    // ====================================
    // bitcoin          3.17x       3.23x       (+2%)
    // gaussianblur     2.25x       2.83x       (+25%)
    //
    //NOTE:
    //   The gaussinablur benchmark has a very tall and narrow timing
    //   graph (i.e. many small levels which parallelize poorly). In
    //   contrast, bitcoin is relatively short and wide (i.e. parallelizes well)
    //
    //The threshold value 400 was choosen to avoid the poor parallelization on small levels,
    //while not decreasing the amount of useful parallelism on larger levels.
    , parallel_threshold_fwd_(400)
    , parallel_threshold_bck_(400) {

    //How many parallel workers (threads) are there?
    int nworkers = __cilkrts_get_nworkers();

    //To avoid contention during tag allocation, we give each worker
    //a separate tag pool
    for(int i = 0; i < nworkers; i++) {
        //XXX: Need to dynamically allocate since move constructor is broken
        //     in boost.pool
        tag_pools_.push_back(new TagPoolType(sizeof(TimingTag)));
    }
}
template<class AnalysisType, class DelayCalcType, class TagPoolType>
ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::~ParallelLevelizedTimingAnalyzer() {
    //Clean-up dynamically alloated tag pools
    for(size_t i = 0; i < tag_pools_.size(); i++) {
        //Pool destructor frees any allocated memory
        delete tag_pools_[i];
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    const std::vector<NodeId>& primary_inputs = timing_graph.primary_inputs();

    //To little work to overcome parallel overhead, just run serially
    for(int node_idx = 0; node_idx < (int) primary_inputs.size(); node_idx++) {
        NodeId node_id = primary_inputs[node_idx];
        AnalysisType::pre_traverse_node(*(this->tag_pools_[0]), timing_graph, timing_constraints, node_id);
    }

}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    using namespace std::chrono;

    //Forward traversal (arrival times)
    for(LevelId level_id = 1; level_id < timing_graph.num_levels(); level_id++) {
        auto fwd_level_start = high_resolution_clock::now();

        const std::vector<NodeId>& level = timing_graph.level(level_id);

        if(level.size() > parallel_threshold_fwd_) {
            //Parallel
            cilk_for(int node_idx = 0; node_idx < level.size(); node_idx++) {
                NodeId node_id = level[node_idx];
                TagPoolType& tag_pool = *tag_pools_[__cilkrts_get_worker_number()];
                this->forward_traverse_node(tag_pool, timing_graph, timing_constraints, node_id);
            }
        } else {
            //Serial
            for(int node_idx = 0; node_idx < (int) level.size(); node_idx++) {
                NodeId node_id = level[node_idx];
                TagPoolType& tag_pool = *tag_pools_[__cilkrts_get_worker_number()];
                this->forward_traverse_node(tag_pool, timing_graph, timing_constraints, node_id);
            }

        }

        auto fwd_level_end = high_resolution_clock::now();
        std::string key = std::string("fwd_level_") + std::to_string(level_id);
        this->perf_data_[key] = duration_cast<duration<double>>(fwd_level_end - fwd_level_start).count();
    }
}

template<class AnalysisType, class DelayCalcType, class TagPoolType>
void ParallelLevelizedTimingAnalyzer<AnalysisType, DelayCalcType, TagPoolType>::backward_traversal(const TimingGraph& timing_graph) {
    using namespace std::chrono;

    //Backward traversal (required times)
    for(LevelId level_id = timing_graph.num_levels() - 2; level_id >= 0; level_id--) {
        auto bck_level_start = high_resolution_clock::now();

        const std::vector<NodeId>& level = timing_graph.level(level_id);

        if(level.size() > parallel_threshold_bck_) {
            //Parallel
            cilk_for(int node_idx = 0; node_idx < level.size(); node_idx++) {
                NodeId node_id = level[node_idx];
                this->backward_traverse_node(timing_graph, node_id);
            }
        } else {
            //Serial
            for(int node_idx = 0; node_idx < (NodeId) level.size(); node_idx++) {
                NodeId node_id = level[node_idx];
                this->backward_traverse_node(timing_graph, node_id);
            }
        }

        auto bck_level_end = high_resolution_clock::now();
        std::string key = std::string("bck_level_") + std::to_string(level_id);
        this->perf_data_[key] = duration_cast<duration<double>>(bck_level_end - bck_level_start).count();
    }
}
