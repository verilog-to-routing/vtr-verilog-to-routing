#pragma once

#include <string>
#include "TimingAnalyzer.hpp"
#include "timing_graph_fwd.hpp"


#define DEFAULT_CLOCK_PERIOD 1.0e-9

//#define SAVE_LEVEL_TIMES

class SerialTimingAnalyzer : public TimingAnalyzer {
    public: 
        virtual ta_runtime calculate_timing(TimingGraph& timing_graph);
        virtual void reset_timing(TimingGraph& timing_graph);
        virtual void save_level_times(TimingGraph& timing_graph, std::string filename);

#ifdef SAVE_LEVEL_TIMES
    protected:
        std::vector<struct timespec> fwd_start_;
        std::vector<struct timespec> fwd_end_;
        std::vector<struct timespec> bck_start_;
        std::vector<struct timespec> bck_end_;
#endif
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
        void pre_traverse_node(TimingGraph& tg, NodeId node_id);
        void forward_traverse_node(TimingGraph& tg, NodeId node_id);
        void backward_traverse_node(TimingGraph& tg, NodeId node_id);

};

