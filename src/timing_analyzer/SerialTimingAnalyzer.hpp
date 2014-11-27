#pragma once

#include "TimingAnalyzer.hpp"
#include "timing_graph_fwd.hpp"

class SerialTimingAnalyzer : public TimingAnalyzer {
    public: 
        virtual void calculate_timing(TimingGraph& timing_graph);
        virtual void reset_timing(TimingGraph& timing_graph);

    protected:
        /*
         * Setup the timing graph.
         *   Includes propogating clock domains and clock skews to clock pins
         */
        virtual void pre_traversal(TimingGraph& timing_graph);

        /*
         * Propogate arrival times
         */
        virtual void forward_traversal(TimingGraph& timing_graph);

        /*
         * Propogate required times
         */
        virtual void backward_traversal(TimingGraph& timing_graph);
        
        //Per node worker functions
        void pre_traverse_node(TimingGraph& tg, NodeId node_id, int level_idx);
        void forward_traverse_node(TimingGraph& tg, NodeId node_id);
        void backward_traverse_node(TimingGraph& tg, NodeId node_id);
};
