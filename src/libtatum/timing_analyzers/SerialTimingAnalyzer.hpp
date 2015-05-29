#pragma once

#include "TimingAnalyzer.hpp"
#include "memory_pool.hpp"

template<template<typename> class AnalysisType, class DelayCalcType>
class SerialTimingAnalyzer : public TimingAnalyzer<AnalysisType<DelayCalcType>> {
    public:
        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        ta_runtime calculate_timing() override;
        void reset_timing() override;

    protected:
        /*
         * Setup the timing graph.
         *   Includes propogating clock domains and clock skews to clock pins
         */
        void pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propogate arrival times
         */
        void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propogate required times
         */
        void backward_traversal(const TimingGraph& timing_graph);

        //Per node worker functions
        void pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void forward_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void backward_traverse_node(const TimingGraph& tg, const NodeId node_id);


        const TimingGraph& tg_;
        const TimingConstraints& tc_;
        const DelayCalcType& dc_;
        MemoryPool tag_pool_; //Memory pool for allocating tags
};


template<template<typename> class AnalysisType, class DelayCalcType>
SerialTimingAnalyzer<AnalysisType,DelayCalcType>::SerialTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalcType& dc)
    : tg_(tg)
    , tc_(tc)
    , dc_(dc)
    //Need to give the size of the object to allocate
    , tag_pool_(sizeof(TimingTag))
    {}

template<template<typename> class AnalysisType, class DelayCalcType>
ta_runtime SerialTimingAnalyzer<AnalysisType,DelayCalcType>::calculate_timing() {
    //No incremental support yet!
    reset_timing();

    struct timespec start_times[4];
    struct timespec end_times[4];

    clock_gettime(CLOCK_MONOTONIC, &start_times[0]);
    pre_traversal(tg_, tc_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[0]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[1]);
    forward_traversal(tg_, tc_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[1]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[2]);
    backward_traversal(tg_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[2]);

    ta_runtime traversal_times;
    traversal_times.pre_traversal = time_sec(start_times[0], end_times[0]);
    traversal_times.fwd_traversal = time_sec(start_times[1], end_times[1]);
    traversal_times.bck_traversal = time_sec(start_times[2], end_times[2]);

    return traversal_times;
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::reset_timing() {
    //Release the memory allocated to tags
    tag_pool_.purge_memory();

    AnalysisType<DelayCalcType>::initialize_traversal(tg_);
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     */
    for(NodeId node_id : timing_graph.primary_inputs()) {
        AnalysisType<DelayCalcType>::pre_traverse_node(tag_pool_, timing_graph, timing_constraints, node_id);
    }
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            forward_traverse_node(timing_graph, timing_constraints, node_id);
        }
    }
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::backward_traversal(const TimingGraph& timing_graph) {
    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            backward_traverse_node(timing_graph, node_id);
        }
    }
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::forward_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Pull from upstream sources to current node
    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        AnalysisType<DelayCalcType>::forward_traverse_edge(tag_pool_, tg, dc_, node_id, edge_id);
    }

    AnalysisType<DelayCalcType>::forward_traverse_finalize_node(tag_pool_, tg, tc, node_id);
}

template<template<typename> class AnalysisType, class DelayCalcType>
void SerialTimingAnalyzer<AnalysisType,DelayCalcType>::backward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    //Pull from downstream sinks to current node

    TN_Type node_type = tg.node_type(node_id);

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    //
    //TODO: if performing optimization on a clock network this may actually be useful
    if(node_type == TN_Type::FF_CLOCK) {
        return;
    }

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        AnalysisType<DelayCalcType>::backward_traverse_edge(tg, dc_, node_id, edge_id);
    }
}

