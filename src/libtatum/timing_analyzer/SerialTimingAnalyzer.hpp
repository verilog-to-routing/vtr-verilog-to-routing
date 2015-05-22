#pragma once

#include <string>

#include "memory_pool.hpp"
#include "TimingAnalyzer.hpp"
#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "TimingTags.hpp"

#define DEFAULT_CLOCK_PERIOD 1.0e-9

//#define SAVE_LEVEL_TIMES

class SerialTimingAnalyzer : public TimingAnalyzer {
    public:
        SerialTimingAnalyzer();
        ta_runtime calculate_timing(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) override;
        void reset_timing();

        virtual bool is_correct() { return true; }

        const TimingTags& data_tags(const NodeId node_id) const override;
        const TimingTags& clock_tags(const NodeId node_id) const override;

    protected:
        /*
         * Setup the timing graph.
         *   Includes propogating clock domains and clock skews to clock pins
         */
        void pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propogate arrival times
         */
        void forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);

        /*
         * Propogate required times
         */
        void backward_traversal(const TimingGraph& timing_graph);

        //Per node worker functions
        void pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void forward_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void backward_traverse_node(const TimingGraph& tg, const NodeId node_id);

        //Tag updaters
        void update_req_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay);
        void update_arr_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay);
        void update_req_tag(TimingTag& tag, const TimingTag& base_tag, const Time& edge_delay);

        std::vector<TimingTags> data_tags_;
        std::vector<TimingTags> clock_tags_;

        MemoryPool tag_pool_; //Memory pool for allocating tags
};

