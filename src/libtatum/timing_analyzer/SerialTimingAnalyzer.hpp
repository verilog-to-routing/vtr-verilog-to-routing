#pragma once

#include <string>

#include "memory_pool.hpp"
#include "TimingAnalyzer.hpp"
#include "timing_graph_fwd.hpp"
#include "TimingTags.hpp"

#define DEFAULT_CLOCK_PERIOD 1.0e-9

//#define SAVE_LEVEL_TIMES

class SerialTimingAnalyzer : public TimingAnalyzer {
    public:
        SerialTimingAnalyzer();
        ta_runtime calculate_timing(const TimingGraph& timing_graph) override;
        void reset_timing();
        virtual void save_level_times(const TimingGraph& timing_graph, std::string filename);

        virtual bool is_correct() { return true; }

        const TimingTags& tags(NodeId node_id) const override;

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
        virtual void pre_traversal(const TimingGraph& timing_graph);

        /*
         * Propogate arrival times
         */
        virtual void forward_traversal(const TimingGraph& timing_graph);

        /*
         * Propogate required times
         */
        virtual void backward_traversal(const TimingGraph& timing_graph);

        //Per node worker functions
        void pre_traverse_node(const TimingGraph& tg, const NodeId node_id);
        void forward_traverse_node(const TimingGraph& tg, const NodeId node_id);
        void backward_traverse_node(const TimingGraph& tg, const NodeId node_id);

        std::vector<TimingTags> tags_;

        MemoryPool tag_pool_; //Memory pool for allocating tags
};

