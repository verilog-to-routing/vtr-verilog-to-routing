#pragma once
#include <omp.h>

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"


class ParallelLevelizedCilkTimingAnalyzer : public SerialTimingAnalyzer {
    public: 
        void calculate_timing(TimingGraph& timing_graph) override;

    private:
        /*
         * Setup the timing graph.
         *   Includes propogating clock domains and clock skews to clock pins
         */
        void pre_traversal(TimingGraph& timing_graph);

        /*
         * Propogate arrival times
         */
        void forward_traversal(TimingGraph& timing_graph);

        /*
         * Propogate required times
         */
        void backward_traversal(TimingGraph& timing_graph);

        //Parallel worker functions
        void pre_traverse_node(TimingGraph& tg, NodeId node_id, int level_idx);
        void forward_traverse_node(TimingGraph& tg, NodeId node_id);
        void backward_traverse_node(TimingGraph& tg, NodeId node_id);

};

