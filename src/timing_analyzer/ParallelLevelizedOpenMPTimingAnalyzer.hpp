#pragma once

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"


class ParallelLevelizedOpenMPTimingAnalyzer : public SerialTimingAnalyzer {
        //Re-use serial calculate timing
        
    private:
        /*
         * Setup the timing graph.
         *   Includes propogating clock domains and clock skews to clock pins
         */
        void pre_traversal(TimingGraph& timing_graph) override;

        /*
         * Propogate arrival times
         */
        void forward_traversal(TimingGraph& timing_graph) override;

        /*
         * Propogate required times
         */
        void backward_traversal(TimingGraph& timing_graph) override;

        //Re-use serial worker functions
};

