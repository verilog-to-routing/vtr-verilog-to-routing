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

SerialTimingAnalyzer::SerialTimingAnalyzer()
    : tag_pool_(sizeof(TimingTag)) //Need to give the size of the object to allocate
    {}

ta_runtime SerialTimingAnalyzer::calculate_timing(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    //Pre-allocate data sturctures
    data_tags_ = std::vector<TimingTags>(timing_graph.num_nodes());
    clock_tags_ = std::vector<TimingTags>(timing_graph.num_nodes());

    struct timespec start_times[4];
    struct timespec end_times[4];

    clock_gettime(CLOCK_MONOTONIC, &start_times[0]);
    pre_traversal(timing_graph, timing_constraints);
    clock_gettime(CLOCK_MONOTONIC, &end_times[0]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[1]);
    forward_traversal(timing_graph, timing_constraints);
    clock_gettime(CLOCK_MONOTONIC, &end_times[1]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[2]);
    backward_traversal(timing_graph);
    clock_gettime(CLOCK_MONOTONIC, &end_times[2]);

    ta_runtime traversal_times;
    traversal_times.pre_traversal = time_sec(start_times[0], end_times[0]);
    traversal_times.fwd_traversal = time_sec(start_times[1], end_times[1]);
    traversal_times.bck_traversal = time_sec(start_times[2], end_times[2]);

    return traversal_times;
}

void SerialTimingAnalyzer::reset_timing() {
    //Drop references to the tags
    data_tags_.clear();
    clock_tags_.clear();

    //Release the memory allocated to tags
    tag_pool_.purge_memory();
}

void SerialTimingAnalyzer::pre_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {

    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     */
    for(NodeId node_id : timing_graph.primary_inputs()) {
        pre_traverse_node(timing_graph, timing_constraints, node_id);
    }
}

void SerialTimingAnalyzer::forward_traversal(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints) {
    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            forward_traverse_node(timing_graph, timing_constraints, node_id);
        }
    }
}

void SerialTimingAnalyzer::backward_traversal(const TimingGraph& timing_graph) {
    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
        for(NodeId node_id : timing_graph.level(level_idx)) {
            backward_traverse_node(timing_graph, node_id);
        }
    }
}

void SerialTimingAnalyzer::pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Primary Input
    ASSERT(tg.num_node_in_edges(node_id) == 0);

    TN_Type node_type = tg.node_type(node_id);

    if(node_type == TN_Type::CONSTANT_GEN_SOURCE) {
        //Pass, we don't propagate any tags from constant generators,
        //since they do not effect they dynamic timing behaviour of the
        //system

    } else if(node_type == TN_Type::CLOCK_SOURCE) {
        //TODO: use real rise edge time!
        ASSERT_MSG(clock_tags_[node_id].num_tags() == 0, "Clock source already has clock tags");
        clock_tags_[node_id].add_tag(tag_pool_,
                TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));

    } else {
        ASSERT(node_type == TN_Type::INPAD_SOURCE);

        //A standard primary input

        //VPR applies input delays to the arc from INPAD_SOURCE to INPAD_OPIN
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES
        //
        //TODO: The initial arrival should correspond to the rising edge of the clock associated
        //      with the input

        //Figure out if we are an input which defines a clock
        if(tg.node_is_clock_source(node_id)) {
            ASSERT_MSG(clock_tags_[node_id].num_tags() == 0, "Primary input already has clock tags");
            clock_tags_[node_id].add_tag(tag_pool_,
                    TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));
        } else {
            ASSERT_MSG(clock_tags_[node_id].num_tags() == 0, "Primary input already has data tags");
            data_tags_[node_id].add_tag(tag_pool_,
                    TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id));

        }
    }
}

void SerialTimingAnalyzer::forward_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
#ifdef FWD_TRAVERSE_DEBUG
    cout << "FWD Traversing Node: " << node_id << " (" << tg.node_type(node_id) << ")" << endl;
#endif

    //Pull from upstream sources to current node
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = data_tags_[node_id];
    TimingTags& node_clock_tags = clock_tags_[node_id];

    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        int src_node_id = tg.edge_src_node(edge_id);

        const Time& edge_delay = tg.edge_delay(edge_id);

#ifdef FWD_TRAVERSE_DEBUG
            cout << "\tSRC Node: " << src_node_id << endl;;
#endif
        /*
         * Clock tags
         */
        if(tg.node_type(src_node_id) != TN_Type::FF_SOURCE) {
            //Do not propagate clock tags from an FF Source,
            //the clock arrival there will have already been converted to a
            //data tag
            const TimingTags& src_clk_tags = clock_tags_[src_node_id];
            for(const TimingTag& src_clk_tag : src_clk_tags) {
#ifdef FWD_TRAVERSE_DEBUG
                cout << "\t\tCLOCK_TAG -";
                cout << " CLK: " << src_clk_tag.clock_domain();
                cout << " Arr: " << src_clk_tag.arr_time();
                cout << " Edge_Delay: " << edge_delay;
                cout << " Edge_Arrival: " << src_clk_tag.arr_time() + edge_delay;
                cout << endl;
#endif
                //Standard propagation through the clock network
                update_arr_tags(node_clock_tags, edge_delay, src_clk_tag);

                if(tg.node_type(node_id) == TN_Type::FF_SOURCE) {
                    //This is a clock to data launch edge
                    //Mark the data arrival (launch) time at this FF

                    //FF Launch edge, begin data propagation
                    //
                    //We convert the clock arrival time to a
                    //data arrival time at this node (since the clock arrival
                    //launches the data)
                    TimingTag launch_tag = src_clk_tag;
                    launch_tag.set_launch_node(src_node_id);
                    ASSERT(launch_tag.next() == nullptr);

                    //Mark propagated launch time as a DATA tag
                    update_arr_tags(node_data_tags, edge_delay, launch_tag);
                }
            }
        }

        /*
         * Data tags
         */

        const TimingTags& src_data_tags = data_tags_[src_node_id];

        for(const TimingTag& src_data_tag : src_data_tags) {
#ifdef FWD_TRAVERSE_DEBUG
            cout << "\t\tDATA_TAG -";
            cout << " CLK: " << src_data_tag.clock_domain();
            cout << " Arr: " << src_data_tag.arr_time();
            cout << " Edge_Delay: " << edge_delay;
            cout << " Edge_Arrival: " << src_data_tag.arr_time() + edge_delay;
            cout << endl;
#endif
            //Standard data-path propagation
            update_arr_tags(node_data_tags, edge_delay, src_data_tag);
        }

        /*
         * Calculate required times
         */
        if(tg.node_type(node_id) == TN_Type::OUTPAD_SINK) {
            //Determine the required time for outputs
            DomainId node_domain = tg.node_clock_domain(node_id);
            for(const TimingTag& data_tag : node_data_tags) {
                //Should we be analyzing paths between these two domains?
                if(tc.should_analyze(data_tag.clock_domain(), node_domain)) {
                    //Only some clock domain paths should be analyzed

                    float clock_constraint = tc.clock_constraint(data_tag.clock_domain(), node_domain);

                    //The output delay is assumed to be on the edge from the OUTPAD_IPIN to OUTPAD_SINK
                    //so we do not need to account for it here
                    node_data_tags.min_req(tag_pool_, Time(clock_constraint), data_tag);
                }
            }
        } else if (tg.node_type(node_id) == TN_Type::FF_SINK) {
            //Determine the required time at this FF
            //
            //We need to generate a required time for each clock domain for which there is a data
            //arrival time at this node, while considering all possible clocks that could drive
            //this node (i.e. take the most restrictive constraint accross all clock tags at this
            //node)

            //FIXME Only need to generate req tags for clocks with known arrival times
            for(TimingTag& node_data_tag : node_data_tags) {
                for(const TimingTag& node_clock_tag : node_clock_tags) {
                    //Should we be analyzing paths between these two domains?
                    if(tc.should_analyze(node_data_tag.clock_domain(), node_clock_tag.clock_domain())) {

                        //We only set a required time if the source domain actually reaches this sink
                        //domain.  This is indicated by having a valid arrival time.
                        if(node_data_tag.arr_time().valid()) {
                            float clock_constraint = tc.clock_constraint(node_data_tag.clock_domain(),
                                                                         node_clock_tag.clock_domain());

                            node_data_tags.min_req_tag(node_data_tag,
                                                       node_clock_tag.arr_time() + Time(clock_constraint),
                                                       node_data_tag);
                        }
                    }
                }
            }
        }
    }

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

void SerialTimingAnalyzer::backward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
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

    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = data_tags_[node_id];

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        int sink_node_id = tg.edge_sink_node(edge_id);

        const Time& edge_delay = tg.edge_delay(edge_id);

        const TimingTags& sink_data_tags = data_tags_[sink_node_id];

#ifdef BCK_TRAVERSE_DEBUG
            cout << "\tSINK Node: " << sink_node_id << endl;;
#endif

        for(const TimingTag& sink_tag : sink_data_tags) {
#ifdef BCK_TRAVERSE_DEBUG
            cout << "\t\t";
            cout << "DATA_TAG -";
            cout << " CLK: " << sink_tag.clock_domain();
            cout << " Req: " << sink_tag.req_time();
            cout << " Edge_Delay: " << edge_delay;
            cout << " Edge_Required: " << sink_tag.arr_time() - edge_delay;
            cout << endl;
#endif
            //We only take the min if we have a valid arrival time
            //TODO: avoid double searching!
            TimingTagIterator matched_tag_iter = node_data_tags.find_tag_by_clock_domain(sink_tag.clock_domain());
            if(matched_tag_iter != node_data_tags.end() && matched_tag_iter->arr_time().valid()) {
                node_data_tags.min_req_tag(*matched_tag_iter, sink_tag.req_time() - edge_delay, sink_tag);
            }
        }

#ifdef BCK_TRAVERSE_DEBUG
        const TimingTags& sink_clock_tags = clock_tags_[sink_node_id];
        for(const TimingTag& sink_tag : sink_clock_tags) {
            cout << "\t\t";
            cout << "CLOCK_TAG -";
            cout << " CLK: " << sink_tag.clock_domain();
            cout << " Req: " << sink_tag.req_time();
            //cout << " Edge_Delay: " << edge_delay;
            //cout << " Edge_Required: " << sink_tag.arr_time() - edge_delay;
            cout << endl;

        }
#endif
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

const TimingTags& SerialTimingAnalyzer::data_tags(const NodeId node_id) const {
    return data_tags_[node_id];
}
const TimingTags& SerialTimingAnalyzer::clock_tags(const NodeId node_id) const {
    return clock_tags_[node_id];
}

void SerialTimingAnalyzer::update_arr_tags(TimingTags& node_tags, const Time& edge_delay, const TimingTag& base_tag) {
    //TODO: this currently performs setup analaysis, it should really be implemented in a
    //dervied analyzers, e.g. SerialSetupTimingAnalyzer
    node_tags.max_arr(tag_pool_, base_tag.arr_time() + edge_delay, base_tag);
}

void SerialTimingAnalyzer::update_req_tags(TimingTags& node_tags, const Time& edge_delay, const TimingTag& base_tag) {
    //TODO: this currently performs setup analaysis, it should really be implemented in a
    //dervied analyzers, e.g. SerialSetupTimingAnalyzer
    node_tags.min_req(tag_pool_, base_tag.req_time() - edge_delay, base_tag);
}
