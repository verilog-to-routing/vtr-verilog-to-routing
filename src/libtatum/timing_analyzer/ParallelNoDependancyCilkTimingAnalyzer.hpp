#pragma once
#include <omp.h>

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"

//This implementation ignores dependancies to get a best case iteration time
class ParallelNoDependancyCilkTimingAnalyzer : public SerialTimingAnalyzer {
    public:
        bool is_correct() override { return false; }
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

};

