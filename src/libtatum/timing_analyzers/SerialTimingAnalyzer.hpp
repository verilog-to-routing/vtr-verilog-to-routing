#pragma once

#include "TimingAnalyzer.hpp"
#include "memory_pool.hpp"

template<template<typename> class AnalysisType, class DelayCalcType>
class SerialTimingAnalyzer : public TimingAnalyzer<AnalysisType, DelayCalcType> {
    public:
        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        ta_runtime calculate_timing() override;
        void reset_timing() override;

    protected:
        /*
         * Setup the timing graph.
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

//Implementation
#include "SerialTimingAnalyzer.tpp"

