#pragma once

#include "TimingGraph.hpp"
#include "SerialTimingAnalyzer.hpp"

#if 0
//This implementation ignores dependancies to get a best case iteration time
class ParallelNoDependancyCilkTimingAnalyzer : public SerialTimingAnalyzer {
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
#endif
