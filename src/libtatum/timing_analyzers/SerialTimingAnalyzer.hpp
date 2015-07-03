#pragma once
#include <chrono>
#include <string>

#include "TimingAnalyzer.hpp"
#include "memory_pool.hpp"

template<class AnalysisType, class DelayCalcType, class TagPoolType=MemoryPool>
class SerialTimingAnalyzer : public TimingAnalyzer<AnalysisType, DelayCalcType> {
    public:
        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalcType& delay_calculator);
        void calculate_timing() override;
        void reset_timing() override;
        const DelayCalcType& delay_calculator() override { return dc_; }
        std::map<std::string, double> profiling_data() override { return perf_data_; }
    protected:
        /*
         * Setup the timing graph.
         */
        virtual void pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propagate arrival times
         */
        virtual void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propagate required times
         */
        virtual void backward_traversal(const TimingGraph& timing_graph);

        //Per node worker functions
        void forward_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void backward_traverse_node(const TimingGraph& tg, const NodeId node_id);


        //Data members
        const TimingGraph& tg_;
        const TimingConstraints& tc_;
        const DelayCalcType& dc_;
        TagPoolType tag_pool_; //Memory pool for allocating tags

        //Profiling info
        std::map<std::string, double> perf_data_;
};

//Implementation
#include "SerialTimingAnalyzer.tpp"

