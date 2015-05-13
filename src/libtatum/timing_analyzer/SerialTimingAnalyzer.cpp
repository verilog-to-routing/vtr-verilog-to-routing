#include <fstream>
#include <algorithm>
#include "SerialTimingAnalyzer.hpp"
#include "TimingGraph.hpp"

#include <iostream>
using std::cout;
using std::endl;

#include "sta_util.hpp"

SerialTimingAnalyzer::SerialTimingAnalyzer()
    : tag_pool_(sizeof(TimingTag)) //Need to give the size of the object to allocate
    {}

ta_runtime SerialTimingAnalyzer::calculate_timing(const TimingGraph& timing_graph) {
    //Pre-allocate data sturctures
    arr_tags_ = std::vector<TimingTags>(timing_graph.num_nodes());
    req_tags_ = std::vector<TimingTags>(timing_graph.num_nodes());

    struct timespec start_times[4];
    struct timespec end_times[4];

#ifdef SAVE_LEVEL_TIMES
    fwd_start_ = std::vector<struct timespec>(timing_graph.num_levels());
    fwd_end_ = std::vector<struct timespec>(timing_graph.num_levels());
    bck_start_ = std::vector<struct timespec>(timing_graph.num_levels());
    bck_end_ = std::vector<struct timespec>(timing_graph.num_levels());
#endif
    clock_gettime(CLOCK_MONOTONIC, &start_times[0]);
    pre_traversal(timing_graph);
    clock_gettime(CLOCK_MONOTONIC, &end_times[0]);

    clock_gettime(CLOCK_MONOTONIC, &start_times[1]);
    forward_traversal(timing_graph);
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
    arr_tags_.clear();
    req_tags_.clear();

    //Release the memory allocated to tags
    tag_pool_.purge_memory();
}

void SerialTimingAnalyzer::pre_traversal(const TimingGraph& timing_graph) {

    /*
     * The pre-traversal sets up the timing graph for propagating arrival
     * and required times.
     * Steps performed include:
     *   - Initialize arrival times on primary inputs
     *   - Propogating clock delay to all clock pins
     *   - Initialize required times on primary outputs
     */
    for(NodeId node_id : timing_graph.primary_inputs()) {
        pre_traverse_node(timing_graph, node_id);
    }

    //TODO: remove primary_ outputs() if not needed?
    for(NodeId node_id : timing_graph.primary_outputs()) {
        pre_traverse_node(timing_graph, node_id);
    }
}

void SerialTimingAnalyzer::forward_traversal(const TimingGraph& timing_graph) {
    //Forward traversal (arrival times)
    for(int level_idx = 1; level_idx < timing_graph.num_levels(); level_idx++) {
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &fwd_start_[level_idx]);
#endif
        for(NodeId node_id : timing_graph.level(level_idx)) {
            forward_traverse_node(timing_graph, node_id);
        }
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &fwd_end_[level_idx]);
#endif
    }
}

void SerialTimingAnalyzer::backward_traversal(const TimingGraph& timing_graph) {
    //Backward traversal (required times)
    for(int level_idx = timing_graph.num_levels() - 2; level_idx >= 0; level_idx--) {
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &bck_start_[level_idx]);
#endif
        for(NodeId node_id : timing_graph.level(level_idx)) {
            backward_traverse_node(timing_graph, node_id);
        }
#ifdef SAVE_LEVEL_TIMES
        clock_gettime(CLOCK_MONOTONIC, &bck_end_[level_idx]);
#endif
    }
}

void SerialTimingAnalyzer::pre_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    if(tg.num_node_in_edges(node_id) == 0) { //Primary Input
        //Initialize with zero arrival time
        //TODO: use real timing constraints!
        if(tg.node_is_clock_source(node_id)) {
            arr_tags_[node_id].add_tag(tag_pool_,
                    TimingTag(Time(0), Time(NAN), tg.node_clock_domain(node_id), node_id, TagType::CLOCK));
        } else {
            arr_tags_[node_id].add_tag(tag_pool_,
                    TimingTag(Time(0), Time(NAN), tg.node_clock_domain(node_id), node_id, TagType::DATA));
        }
    }

    if(tg.num_node_out_edges(node_id) == 0) { //Primary Output

        //Initialize required time
        //FIXME Currently assuming:
        //   * All clocks at fixed frequency

        //FF_SINK's have their required time set by the clock network, so we don't
        //need to set them explicitly here.
        //
        //Everything else gets the standard constraint
        if(tg.node_type(node_id) != TN_Type::FF_SINK) {
            req_tags_[node_id].add_tag(tag_pool_,
                    TimingTag(Time(NAN), Time(DEFAULT_CLOCK_PERIOD), tg.node_clock_domain(node_id), node_id, TagType::DATA));
        }
    }
}

void SerialTimingAnalyzer::forward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    //Pull from upstream sources to current node
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& arr_tags = arr_tags_[node_id];

    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        int src_node_id = tg.edge_src_node(edge_id);

        const Time& edge_delay = tg.edge_delay(edge_id);

        const TimingTags& src_arr_tags = arr_tags_[src_node_id];

        for(const TimingTag& src_tag : src_arr_tags) {

            if(tg.node_type(node_id) == TN_Type::FF_SOURCE) {
                //This is a clock to data launch edge
                //Mark the data arrival (launch) time at this launch FF

                //The only input to a FF_SOURCE should be the the clock
                //network, so we expect only CLOCK tags
                ASSERT(src_tag.type() == TagType::CLOCK);

                //FF Launch edge, begin data propagation
                //
                //We convert the clock arrival time to a
                //data arrival time at this node (since the clock arrival
                //launches the data)
                TimingTag launch_tag = src_tag;
                launch_tag.set_type(TagType::DATA);
                launch_tag.set_launch_node(src_node_id);

                //Mark propagated launch time
                arr_tags.max_arr(tag_pool_, launch_tag.arr_time() + edge_delay, launch_tag);

            } else if (tg.node_type(node_id) == TN_Type::FF_SINK && src_tag.type() == TagType::CLOCK) {
                //This is a clock to data capture edge
                //Mark the propagated required time if this is a capture FF

                //Must be a clock tag
                ASSERT(src_tag.type() == TagType::CLOCK);

                //Capture Edge
                TimingTag capture_tag = src_tag;
                capture_tag.set_type(TagType::DATA);
                capture_tag.set_launch_node(node_id);
                capture_tag.set_arr_time(Time(NAN));
                capture_tag.set_req_time(src_tag.arr_time() + Time(DEFAULT_CLOCK_PERIOD));

                //Mark propogated required time
                TimingTags& req_tag = req_tags_[node_id];
                //TODO: use real timing constraints!
                req_tag.add_tag(tag_pool_, capture_tag);
            } else {
                //Standard propogation
                arr_tags.max_arr(tag_pool_, src_tag.arr_time() + edge_delay, src_tag);
            }
        }
    }
}

void SerialTimingAnalyzer::backward_traverse_node(const TimingGraph& tg, const NodeId node_id) {
    //Pull from downstream sinks to current node

    TN_Type node_type = tg.node_type(node_id);

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    if(node_type == TN_Type::FF_CLOCK) {
        return;
    }

    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& req_tags = req_tags_[node_id];

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        int sink_node_id = tg.edge_sink_node(edge_id);

        const Time& edge_delay = tg.edge_delay(edge_id);

        const TimingTags& sink_req_tags = req_tags_[sink_node_id];

        for(const TimingTag& sink_tag : sink_req_tags) {
            req_tags.min_req(tag_pool_, sink_tag.req_time() - edge_delay, sink_tag);
        }
    }
}

void SerialTimingAnalyzer::save_level_times(const TimingGraph& timing_graph, std::string filename) {
#ifdef SAVE_LEVEL_TIMES
    std::ofstream f(filename.c_str());
    f << "Level," << "Width," << "Fwd_Time," << "Bck_Time" << std::endl;
    for(int i = 0; i < timing_graph.num_levels(); i++) {
        f << i << "," << timing_graph.level(i).size() << "," << time_sec(fwd_start_[i], fwd_end_[i]) << "," << time_sec(bck_start_[i], bck_end_[i]) << std::endl;
    }
#endif
}

const TimingTags& SerialTimingAnalyzer::arrival_tags(NodeId node_id) const {
    return arr_tags_[node_id];
}

const TimingTags& SerialTimingAnalyzer::required_tags(NodeId node_id) const {
    return req_tags_[node_id];
}
