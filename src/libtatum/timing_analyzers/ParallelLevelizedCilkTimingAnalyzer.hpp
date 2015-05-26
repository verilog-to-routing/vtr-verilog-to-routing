#pragma once
#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"


#if 0
class ParallelLevelizedCilkTimingAnalyzer : public SerialTimingAnalyzer {
        //Use the same basic functions for the different traversals
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

        //Use the same per-node worker functions as the serial case
        void pre_traverse_node(TimingGraph& timing_graph, NodeId node_id);

};

#endif
