#ifndef SerialTimingAnalyzer_hpp
#define SerialTimingAnalyzer_hpp
#include "TimingAnalyzer.hpp"

class SerialTimingAnalyzer : public TimingAnalyzer {
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
};

#endif
