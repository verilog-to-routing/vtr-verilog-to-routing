#pragma once

#include <string>

#include "memory_pool.hpp"
#include "SetupTimingAnalyzer.hpp"
#include "TimingGraph.hpp"
#include "timing_constraints_fwd.hpp"
#include "TimingTags.hpp"

#define DEFAULT_CLOCK_PERIOD 1.0e-9

//#define SAVE_LEVEL_TIMES

template<class TraversalType>
class SerialTimingAnalyzer : public SetupTimingAnalyzer,
                             public TraversalType {
    public:
        SerialTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints);
        ta_runtime calculate_timing() override;
        void reset_timing() override;

        const TimingTags& setup_data_tags(const NodeId node_id) const override;
        const TimingTags& setup_clock_tags(const NodeId node_id) const override;

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
        void update_arr_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay);
        void update_req_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay);
        //TODO: Find a cleaner way to handle these special case required time updates
        void update_req_outpad_sink(TimingTags& node_tags, const Time& req_time, const TimingTag& info_tag);
        void update_req_tag_ff_sink(TimingTag& tag, const TimingTag& base_tag, const Time& constraint, const TimingTag& info_tag);

        MemoryPool tag_pool_; //Memory pool for allocating tags
};

#include <fstream>
#include <algorithm>
#include "SerialTimingAnalyzer.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.cpp"

#include <iostream>
using std::cout;
using std::endl;

#include "sta_util.hpp"

//#define FWD_TRAVERSE_DEBUG
//#define BCK_TRAVERSE_DEBUG

template<class TraversalType>
SerialTimingAnalyzer<TraversalType>::SerialTimingAnalyzer(const TimingGraph& tg, const TimingConstraints& tc)
    : SetupTimingAnalyzer(tg, tc)
    //Need to give the size of the object to allocate
    , tag_pool_(sizeof(TimingTag)) {
}

template<class TraversalType>
ta_runtime SerialTimingAnalyzer<TraversalType>::calculate_timing() {
    //No incremental support yet!
    reset_timing();

    struct timespec start_times[4];
    struct timespec end_times[4];

    clock_gettime(CLOCK_MONOTONIC, &start_times[0]);
    pre_traversal(tg_, tc_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[0]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[1]);
    forward_traversal(tg_, tc_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[1]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[2]);
    backward_traversal(tg_);
    clock_gettime(CLOCK_MONOTONIC, &end_times[2]);

    ta_runtime traversal_times;
    traversal_times.pre_traversal = time_sec(start_times[0], end_times[0]);
    traversal_times.fwd_traversal = time_sec(start_times[1], end_times[1]);
    traversal_times.bck_traversal = time_sec(start_times[2], end_times[2]);

    return traversal_times;
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::reset_timing() {
    //Release the memory allocated to tags
    tag_pool_.purge_memory();

    TraversalType::initialize_traversal(tg_);
/*
 *
 *    //Re-allocate tags
 *    data_tags_ = std::vector<TimingTags>(tg_.num_nodes());
 *    clock_tags_ = std::vector<TimingTags>(tg_.num_nodes());
 */
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     */
    for(NodeId node_id : timing_graph.primary_inputs()) {
        TraversalType::pre_traverse_node(tag_pool_, timing_graph, timing_constraints, node_id);
    }
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            forward_traverse_node(timing_graph, timing_constraints, node_id);
        }
    }
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::backward_traversal(const TimingGraph& timing_graph) {
    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            backward_traverse_node(timing_graph, node_id);
        }
    }
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::forward_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
#ifdef FWD_TRAVERSE_DEBUG
    cout << "FWD Traversing Node: " << node_id << " (" << tg.node_type(node_id) << ")" << endl;
#endif

    //Pull from upstream sources to current node
    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        TraversalType::forward_traverse_edge(tag_pool_, tg, node_id, edge_id);
    }

    TraversalType::forward_traverse_finalize_node(tag_pool_, tg, tc, node_id);

#ifdef FWD_TRAVERSE_DEBUG
    cout << "\tResulting Tags:" << endl;
    for(const auto& node_clk_tag : node_clock_tags) {
        cout << "\t\t";
        cout << "CLOCK_TAG -";
        cout << " CLK: " << node_clk_tag.clock_domain();
        cout << " Arr: " << node_clk_tag.arr_time();
        cout << " Req: " << node_clk_tag.req_time();
        cout << endl;
    }
    for(const auto& node_data_tag : node_data_tags) {
        cout << "\t\t";
        cout << "DATA_TAG -";
        cout << " CLK: " << node_data_tag.clock_domain();
        cout << " Arr: " << node_data_tag.arr_time();
        cout << " Req: " << node_data_tag.req_time();
        cout << endl;
    }
#endif
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::backward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    //Pull from downstream sinks to current node

#ifdef BCK_TRAVERSE_DEBUG
    cout << "BCK Traversing Node: " << node_id << " (" << tg.node_type(node_id) << ")" << endl;
#endif

    TN_Type node_type = tg.node_type(node_id);

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    if(node_type == TN_Type::FF_CLOCK) {
        return;
    }

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        TraversalType::backward_traverse_edge(tg, node_id, edge_id);
    }

#ifdef BCK_TRAVERSE_DEBUG
    const TimingTags& node_clock_tags = clock_tags_[node_id];
    cout << "\tResulting Tags:" << endl;
    for(const auto& node_data_tag : node_data_tags) {
        cout << "\t\t";
        cout << "DATA_TAG -";
        cout << " CLK: " << node_data_tag.clock_domain();
        cout << " Arr: " << node_data_tag.arr_time();
        cout << " Req: " << node_data_tag.req_time();
        cout << endl;
    }
    for(const auto& node_clk_tag : node_clock_tags) {
        cout << "\t\t";
        cout << "CLOCK_TAG -";
        cout << " CLK: " << node_clk_tag.clock_domain();
        cout << " Arr: " << node_clk_tag.arr_time();
        cout << " Req: " << node_clk_tag.req_time();
        cout << endl;
    }
#endif
}

template<class TraversalType>
const TimingTags& SerialTimingAnalyzer<TraversalType>::setup_data_tags(const NodeId node_id) const {
    return TraversalType::get_setup_data_tag(node_id);
}

template<class TraversalType>
const TimingTags& SerialTimingAnalyzer<TraversalType>::setup_clock_tags(const NodeId node_id) const {
    return TraversalType::get_setup_clock_tag(node_id);
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::update_arr_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay) {
    //TODO: this currently performs setup analaysis, it should really be implemented in a
    //dervied analyzers, e.g. SerialSetupTimingAnalyzer
    node_tags.max_arr(tag_pool_, base_tag.arr_time() + edge_delay, base_tag);
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::update_req_tags(TimingTags& node_tags, const TimingTag& base_tag, const Time& edge_delay) {
    //TODO: this currently performs setup analaysis, it should really be implemented in a
    //dervied analyzers, e.g. SerialSetupTimingAnalyzer
    node_tags.min_req(tag_pool_, base_tag.req_time() - edge_delay, base_tag);
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::update_req_outpad_sink(TimingTags& node_tags, const Time& req_time, const TimingTag& info_tag) {
    node_tags.min_req(tag_pool_, req_time, info_tag);
}

template<class TraversalType>
void SerialTimingAnalyzer<TraversalType>::update_req_tag_ff_sink(TimingTag& tag, const TimingTag& base_tag, const Time& constraint, const TimingTag& info_tag) {
    tag.min_req(base_tag.arr_time() + constraint, info_tag);
}
