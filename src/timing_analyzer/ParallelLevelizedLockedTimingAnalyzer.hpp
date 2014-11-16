#pragma once

#include <omp.h>

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"


class ParallelLevelizedLockedTimingAnalyzer : public SerialTimingAnalyzer {
    public: 
        void calculate_timing(TimingGraph& timing_graph);

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

        void create_locks(TimingGraph& tg);
        void init_locks();
        void cleanup_locks();
        std::vector<omp_lock_t> node_locks_;
};
