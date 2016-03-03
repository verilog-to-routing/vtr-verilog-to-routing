#pragma once
#include <memory>
#include <vector>

#include "assert.hpp"

#include "GraphVisitor.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"

/** \file
 * The 'SetupAnalysisVisitor' class defines the operations needed used by a GraphWalker class
 * to perform a setup (max/longest path) analysis. It satisifes and extends the GraphVisitor 
 * concept class.
 *
 * Setup Analysis Principles
 * ==========================
 * To operate correctly data arriving at a Flip-Flop (FF) must arrive (i.e. be stable) some
 * amount of time BEFORE the capturing clock edge.  This time is referred to as the
 * 'Setup Time' of the Flip-Flop.  If the data arrives during the setup window
 * (i.e. less than \f$ t_s \f$ before the capturing clock edge) then the FF may go meta-stable
 * failing to capture the data. This will put the circuit in an invalid state (this is bad).
 *
 * More formally, for correct operation at every cycle we require the following to be satisfied
 * for every path in the circuit:
 *
 * \f[
 *      t_{clock}^{(launch)} + t_{cq}^{(max)} + t_{comb}^{(max)} \leq t_{clock}^{(capture)} - t_s   (1)
 * \f]
 *
 * where \f$ t_{clock}^{(launch)} \f$ is the clock arrival time at the upstream FF, \f$ t_{cq}^{(max)} \f$ is the
 * maximum clock-to-q delay of the upstream FF, and \f$ t_{comb}^{(max)} \f$ is the maximum combinational
 * path delay from the upstream to downstream FFs, \f$ t_s \f$ is the setup constraint of the downstream
 * FF, and \f$ t_{clock}^{(capture)} \f$ is the clock arrival time at the downstream FF.
 *
 * Typically \f$ t_{clock}^{(launch)} \f$ and \f$ t_{clock}^{(capture)} \f$ have a periodic relationship. To ensure
 * a non-optimistic analysis we need to consider the minimum possible time difference between
 * \f$ t_{clock}^{(capture)} \f$ and \f$ t_{clock}^{(launch)} \f$.  In the case where the launch and capture clocks
 * are the same this *constraint* (\f$ T_{cstr} \f$) value is simply the clock period (\f$ T_{clk} \f$); however, in multi-clock
 * scenarios the closest alignment of clock edges is used, which may be smaller than the clock period
 * of either the launch or capture clock (depending on their period and phase relationship). It is
 * typically assumed that the launch clock arrives at time zero (even if this is not strictly true
 * in an absolute sense, such as if the clock has a rise time > 0, we can achieve this by adjusting
 * the value of \f$ T_{cstr} \f$).
 *
 * Additionally, the arrival times of the launch and capture edges are unlikely to be perfectly
 * aligned in practise, due to clock skew.
 *
 * Formally, we can re-write our condition for correct operation as:
 * \f[
 *      t_{clk\_insrt}^{(launch)} + t_{cq}^{(max)} + t_{comb}^{(max)} \leq t_{clk\_insrt}^{(capture)} - t_s + T_{cstr}    (2)
 * \f]
 *
 * where \f$ t_{clk\_insrt}^{(launch)} \f$ and \f$ t_{clk\_insrt}^{(capture)} \f$ represent the clock insertion delays
 * to the launch/capture nodes, and \f$ T_{cstr} \f$ the ideal constraint (excluding skew).
 *
 * We refer to the left hand side of (2) as the 'arrival time' (when the data actually arrives at a FF capture node),
 * and the right hand side as the 'required time' (when the data is required to arrive for correct operation), so
 * (2) becomes:
 * \f[
 *      t_{arr}^{(max)} \leq t_{req}^{(min)} (3)
 * \f]
 */

 /**
 * Setup Analysis Implementation
 * ===============================
 * When we perform setup analysis we follow the formulation of (2), by performing two key operations: traversing
 * the clock network, and traversing the data paths.
 *
 * Clock Propogation
 * -------------------
 * We traverse the clock network to determine the clock delays (\f$ t_{clk\_insrt}^{(launch)} \f$, \f$ t_{clk\_insrt}^{(capture)} \f$)
 * at each FF clock pin (FF_CLOCK node in the timing graph).  Clock related delay information is stored and
 * propogated as sets of 'clock tags'.
 *
 * Data Propogation
 * ------------------
 * We traverse the data paths in the circuit to determine \f$ t_{arr}^{(max)} \f$ in (2).
 * In particular, at each node in the circuit we track the maximum arrival time to it as a set
 * of 'data_tags'.
 *
 * The timing graph uses separte nodes to represent FF Pins (FF_IPIN, FF_OPIN) and FF Sources/Sinks
 * (FF_SOURCE/FF_SINK). As a result \f$ t_{cq} \f$ delays are actually placed on the edges between FF_SOURCEs
 * and FF_OPINs, \f$ t_s \f$ values are similarily placed as edge delays between FF_IPINs and FF_SINKs.
 *
 * The data launch nodes (e.g. FF_SOURCES) have their, arrival times initialized to the clock insertion
 * delay (\f$ t_{clk\_insrt}^{(launch)} \f$). Then at each downstream node we store the maximum of the upstream
 * arrival time plus the incoming edge delay as the arrival time at each node.  As a result the final
 * arrival time at a capture node (e.g. FF_SINK) is the maximum arival time (\f$ t_{arr}^{(max)} \f$).
 *
 *
 * The required times at sink nodes (Primary Outputs, e.g. FF_SINKs) can be calculated directly after clock propogation,
 * since the value of \f$ T_{cstr} \f$ is determined ahead of time.
 *
 * To facilitate the calculation of slack at each node we also propogate required times back through
 * the timing graph.  This follows a similar procedure to arrival propogation but happens in reverse
 * order (from POs to PIs), with each node taking the minumum of the downstream required time minus
 * the edge delay.
 *
 * Combined Clock & Data Propogation
 * -----------------------------------
 * In practice the clock and data propogation, although sometimes logically useful to think of as separate,
 * are combined into a single traversal for efficiency (minimizing graph walks).  This is enabled by
 * building the timing graph with edges FF_CLOCK and FF_SINK/FF_SOUCE nodes.  On the forward traversal
 * we propogate clock tags from known clock sources, which are converted to data tags (with appropriate
 * *arrival times*) at FF_SOURCE nodes, and data tags (with appropriate *required times*) at FF_SINK nodes.
 *
 * \see HoldAnalysisVisitor
 */

/**
 * SetupAnalysisVisitor extends the GraphVisitor concept with operations for performing a setup (max/long-path)
 * analysis.
 */
class SetupAnalysisVisitor {
    public:
        SetupAnalysisVisitor(size_t num_tags) {
            for(size_t i = 0; i < num_tags; ++i) {
                setup_data_tags_.push_back(std::make_shared<TimingTags>());
                setup_clock_tags_.push_back(std::make_shared<TimingTags>());
            }
        }

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        void reset();

        std::shared_ptr<TimingTags> get_setup_data_tags(const NodeId node_id) { return setup_data_tags_[node_id]; }
        std::shared_ptr<TimingTags> get_setup_clock_tags(const NodeId node_id) { return setup_clock_tags_[node_id]; }

    private:

        template<class DelayCalc>
        void do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        std::vector<std::shared_ptr<TimingTags>> setup_data_tags_;
        std::vector<std::shared_ptr<TimingTags>> setup_clock_tags_;
};

void SetupAnalysisVisitor::reset() {
    size_t num_tags = setup_data_tags_.size();

    setup_data_tags_.clear();
    setup_clock_tags_.clear();

    for(size_t i = 0; i < num_tags; ++i) {
        setup_data_tags_.push_back(std::make_shared<TimingTags>());
        setup_clock_tags_.push_back(std::make_shared<TimingTags>());
    }
}

/*
 * Arrival Time Operations
 */

void SetupAnalysisVisitor::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Logical Input
    ASSERT_MSG(tg.num_node_in_edges(node_id) == 0, "Logical input has input edges: timing graph not levelized.");

    TN_Type node_type = tg.node_type(node_id);

    //Note that we assume that edge counting has set the effective period constraint assuming a
    //launch edge at time zero.  This means we don't need to do anything special for clocks
    //with rising edges after time zero.
    if(node_type == TN_Type::CONSTANT_GEN_SOURCE) {
        //Pass, we don't propagate any tags from constant generators,
        //since they do not effect the dynamic timing behaviour of the
        //system

    } else if(node_type == TN_Type::CLOCK_SOURCE) {
        ASSERT_MSG(get_setup_clock_tags(node_id)->num_tags() == 0, "Clock source already has clock tags");

        //Initialize a clock tag with zero arrival, invalid required time
        TimingTag clock_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Add the tag
        get_setup_data_tags(node_id)->add_tag(clock_tag);

    } else {
        ASSERT(node_type == TN_Type::INPAD_SOURCE);

        //A standard primary input

        //We assume input delays are on the arc from INPAD_SOURCE to INPAD_OPIN,
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES

        //Initialize a data tag with zero arrival, invalid required time
        TimingTag input_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Figure out if we are an input which defines a clock
        if(tg.node_is_clock_source(node_id)) {
            ASSERT_MSG(get_setup_clock_tags(node_id)->num_tags() == 0, "Logical input already has clock tags");

            get_setup_clock_tags(node_id)->add_tag(input_tag);
        } else {
            ASSERT_MSG(get_setup_clock_tags(node_id)->num_tags() == 0, "Logical input already has data tags");

            get_setup_data_tags(node_id)->add_tag(input_tag);
        }
    }
}


template<class DelayCalc>
void SetupAnalysisVisitor::do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, NodeId node_id) {
    //Pull from upstream sources to current node
    for(int edge_idx = 0; edge_idx < tg.num_node_in_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_in_edge(node_id, edge_idx);

        do_arrival_traverse_edge(tg, dc, node_id, edge_id);
    }
}


template<class DelayCalc>
void SetupAnalysisVisitor::do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    std::shared_ptr<TimingTags> node_data_tags = get_setup_data_tags(node_id);
    std::shared_ptr<TimingTags> node_clock_tags = get_setup_clock_tags(node_id);

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    const Time& edge_delay = dc.max_edge_delay(tg, edge_id);

    /*
     * Clock tags
     */
    if(tg.node_type(src_node_id) != TN_Type::FF_SOURCE) {
        //We do not propagate clock tags from an FF_SOURCE.
        //The clock arrival will have already been converted to a
        //data tag when the previous level was traversed.

        const std::shared_ptr<TimingTags> src_clk_tags = get_setup_clock_tags(src_node_id);
        for(const TimingTag& src_clk_tag : *src_clk_tags) {
            //Standard propagation through the clock network
            node_clock_tags->max_arr(src_clk_tag.arr_time() + edge_delay, src_clk_tag);

            if(tg.node_type(node_id) == TN_Type::FF_SOURCE) {
                //We are traversing a clock to data launch edge.
                //
                //We convert the clock arrival time to a data
                //arrival time at this node (since the clock
                //arrival launches the data).

                //Make a copy of the tag
                TimingTag launch_tag = src_clk_tag;

                //Update the launch node, since the data is
                //launching from this node
                launch_tag.set_launch_node(node_id);
                ASSERT(launch_tag.next() == nullptr);

                //Mark propagated launch time as a DATA tag
                node_data_tags->max_arr(launch_tag.arr_time() + edge_delay, launch_tag);
            }
        }
    }

    /*
     * Data tags
     */
    const std::shared_ptr<TimingTags> src_data_tags = get_setup_data_tags(src_node_id);

    for(const TimingTag& src_data_tag : *src_data_tags) {
        //Standard data-path propagation
        node_data_tags->max_arr(src_data_tag.arr_time() + edge_delay, src_data_tag);
    }
}

/*
 * Required Time Operations
 */

void SetupAnalysisVisitor::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Take tags by reference so they are updated in-place
    std::shared_ptr<TimingTags> node_data_tags = get_setup_data_tags(node_id);
    std::shared_ptr<TimingTags> node_clock_tags = get_setup_clock_tags(node_id);

    /*
     * Calculate required times
     */
    if(tg.node_type(node_id) == TN_Type::OUTPAD_SINK) {
        //Determine the required time for outputs.
        //
        //We assume any output delay is on the OUTPAT_IPIN to OUTPAD_SINK edge,
        //so we only need set the constraint on the OUTPAD_SINK
        DomainId node_domain = tg.node_clock_domain(node_id);
        for(const TimingTag& data_tag : *node_data_tags) {
            //Should we be analyzing paths between these two domains?
            if(tc.should_analyze(data_tag.clock_domain(), node_domain)) {
                //These clock domains should be analyzed

                float clock_constraint = tc.setup_clock_constraint(data_tag.clock_domain(), node_domain);

                //Set the required time on the sink.
                node_data_tags->min_req(Time(clock_constraint), data_tag);
            }
        }
    } else if (tg.node_type(node_id) == TN_Type::FF_SINK) {
        //Determine the required time at this FF
        //
        //We need to generate a required time for each clock domain for which there is a data
        //arrival time at this node, while considering all possible clocks that could drive
        //this node (i.e. take the most restrictive constraint accross all clock tags at this
        //node)

        for(TimingTag& node_data_tag : *node_data_tags) {
            for(const TimingTag& node_clock_tag : *node_clock_tags) {
                //Should we be analyzing paths between these two domains?
                if(tc.should_analyze(node_data_tag.clock_domain(), node_clock_tag.clock_domain())) {

                    //We only set a required time if the source domain actually reaches this sink
                    //domain.  This is indicated by having a valid arrival time.
                    if(node_data_tag.arr_time().valid()) {
                        float clock_constraint = tc.setup_clock_constraint(node_data_tag.clock_domain(),
                                                                           node_clock_tag.clock_domain());

                        //Update the required time. This will keep the most restrictive constraint.
                        node_data_tag.min_req(node_clock_tag.arr_time() + Time(clock_constraint), node_data_tag);
                    }
                }
            }
        }
    }
}

template<class DelayCalc>
void SetupAnalysisVisitor::do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) {
    //Pull from downstream sinks to current node

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    //
    //TODO: if performing optimization on a clock network this may actually be useful
    if(tg.node_type(node_id) == TN_Type::FF_CLOCK) {
        return;
    }

    //Each back-edge from down stream node
    for(int edge_idx = 0; edge_idx < tg.num_node_out_edges(node_id); edge_idx++) {
        EdgeId edge_id = tg.node_out_edge(node_id, edge_idx);

        do_required_traverse_edge(tg, dc, node_id, edge_id);
    }
}

template<class DelayCalc>
void SetupAnalysisVisitor::do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    std::shared_ptr<TimingTags> node_data_tags = get_setup_data_tags(node_id);

    //Pulling values from downstream sink node
    int sink_node_id = tg.edge_sink_node(edge_id);

    const Time& edge_delay = dc.max_edge_delay(tg, edge_id);

    const std::shared_ptr<TimingTags> sink_data_tags = get_setup_data_tags(sink_node_id);

    for(const TimingTag& sink_tag : *sink_data_tags) {
        //We only propogate the required time if we have a valid arrival time
        TimingTagIterator matched_tag_iter = node_data_tags->find_tag_by_clock_domain(sink_tag.clock_domain());
        if(matched_tag_iter != node_data_tags->end() && matched_tag_iter->arr_time().valid()) {
            //Valid arrival, update required
            matched_tag_iter->min_req(sink_tag.req_time() - edge_delay, sink_tag);
        }
    }
}
