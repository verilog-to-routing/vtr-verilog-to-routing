#pragma once
#include <memory>
#include <vector>

#include "tatum_assert.hpp"

#include "GraphVisitor.hpp"
#include "TimingTags.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

/** \file
 * The 'HoldAnalysisMode' class defines the operators needed by a timing analyzer class
 * to perform a hold (min/shortest path) analysis. It extends the BaseAnalysisMode concept class.
 *
 * \see SetupAnalysisMode
 *
 * Hold Analysis Principles
 * ==========================
 *
 * In addition to satisfying setup constraints, data arriving at a Flip-Flop (FF) must stay (i.e.
 * remain stable) for some amount of time *after* the capturing clock edge arrives. This time is
 * referred to as the 'Hold Time' of the Flip-Flop.  If the data changes during the hold window
 * (i.e. less than \f$ t_h \f$ after the capturing clock edge) then the FF may go meta-stable
 * failing to capture the data. This will put the circuit in an invalid state (this is bad).
 *
 * More formally, for correct operation at every cycle we require the following to be satisfied
 * for every path in the circuit:
 *
 * \f[
 *      t_{clk\_insrt}^{(launch)} + t_{cq}^{(min)} + t_{comb}^{(min)} \geq t_{clk\_insrt}^{(capture)} + t_h   (1)
 * \f]
 *
 * where \f$ t_{clk\_insrt}^{(launch)}, t_{clk\_insrt}^{(capture)} \f$ are the up/downstream FF clock insertion
 * delays,  \f$ t_{cq}^{(min)} \f$ is the minimum clock-to-q delay of the upstream FF, \f$ t_{comb}^{(min)} \f$ is
 * the minimum combinational path delay from the upstream to downstream FFs, and \f$ t_h \f$ is the hold
 * constraint of the downstream FF.
 *
 * Note that unlike in setup analysis this behaviour is indepenant of clock period.
 * Intuitively, hold analysis can be viewed as data from the upstream FF trampling the data launched
 * on the previous cycle before it can be captured by the downstream FF.
 */

/**
 * Hold Analysis Implementation
 * ===============================
 * The hold analysis implementation is generally similar to the implementation used for Setup, except
 * that the minumum arrival times (and maximum required times) are propagated through the timing graph.
 */

class HoldAnalysisVisitor {
    public:
        HoldAnalysisVisitor(size_t num_nodes)
            : hold_data_tags_(num_nodes)
            , hold_clock_tags_(num_nodes)
            {}

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        void reset();

        const TimingTags& get_hold_data_tags(const NodeId node_id) const { return hold_data_tags_[size_t(node_id)]; }

        const TimingTags& get_hold_clock_tags(const NodeId node_id) const { return hold_clock_tags_[size_t(node_id)]; }

    private:
        TimingTags& get_hold_data_tags(const NodeId node_id) { return hold_data_tags_[size_t(node_id)]; }
        TimingTags& get_hold_clock_tags(const NodeId node_id) { return hold_clock_tags_[size_t(node_id)]; }

        template<class DelayCalc>
        void do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);
    private:
        std::vector<TimingTags> hold_data_tags_;
        std::vector<TimingTags> hold_clock_tags_;
};

//Short name
using HoldAnalysis = HoldAnalysisVisitor;

/*
 * Pre-traversal
 */
void HoldAnalysisVisitor::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& /*tc*/, const NodeId node_id) {
    //Logical Input
    TATUM_ASSERT_MSG(tg.node_in_edges(node_id).size() == 0, "Logical input has input edges: timing graph not levelized.");

    TN_Type node_type = tg.node_type(node_id);

    //Note that we assume that edge counting has set the effective period constraint assuming a
    //launch edge at time zero.  This means we don't need to do anything special for clocks
    //with rising edges after time zero.
    if(node_type == TN_Type::CONSTANT_GEN_SOURCE) {
        //Pass, we don't propagate any tags from constant generators,
        //since they do not effect the dynamic timing behaviour of the
        //system

    } else if(node_type == TN_Type::CLOCK_SOURCE) {
        TATUM_ASSERT_MSG(get_hold_clock_tags(node_id).num_tags() == 0, "Clock source already has clock tags");

        //Initialize a clock tag with zero arrival, invalid required time
        TimingTag clock_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Add the tag
        get_hold_clock_tags(node_id).add_tag(clock_tag);

    } else {
        TATUM_ASSERT(node_type == TN_Type::INPAD_SOURCE);

        //A standard primary input

        //We assume input delays are on the arc from INPAD_SOURCE to INPAD_OPIN,
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES

        //Initialize a data tag with zero arrival, invalid required time
        TimingTag input_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Figure out if we are an input which defines a clock
        if(tg.node_is_clock_source(node_id)) {
            TATUM_ASSERT_MSG(get_hold_clock_tags(node_id).num_tags() == 0, "Logical input already has clock tags");

            get_hold_clock_tags(node_id).add_tag(input_tag);
        } else {
            TATUM_ASSERT_MSG(get_hold_clock_tags(node_id).num_tags() == 0, "Logical input already has data tags");

            get_hold_data_tags(node_id).add_tag(input_tag);
        }
    }
}

void HoldAnalysisVisitor::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    TN_Type node_type = tg.node_type(node_id);

    TimingTags& node_data_tags = get_hold_data_tags(node_id);
    TimingTags& node_clock_tags = get_hold_clock_tags(node_id);

    /*
     * Calculate required times
     */
    if(node_type == TN_Type::OUTPAD_SINK) {
        //Determine the required time for outputs
        DomainId node_domain = tg.node_clock_domain(node_id);
        for(const TimingTag& data_tag : node_data_tags) {
            //Should we be analyzing paths between these two domains?
            if(tc.should_analyze(data_tag.clock_domain(), node_domain)) {
                //These clock domains should be analyzed

                float clock_constraint = tc.hold_clock_constraint(data_tag.clock_domain(), node_domain);

                //The output delay is assumed to be on the edge from the OUTPAD_IPIN to OUTPAD_SINK
                //so we do not need to account for it here
                node_data_tags.max_req(Time(clock_constraint), data_tag);
            }
        }
    } else if (node_type == TN_Type::FF_SINK) {
        //Determine the required time at this FF
        //
        //We need to generate a required time for each clock domain for which there is a data
        //arrival time at this node, while considering all possible clocks that could drive
        //this node (i.e. take the most restrictive constraint accross all clock tags at this
        //node)

        for(TimingTag& node_data_tag : node_data_tags) {
            for(const TimingTag& node_clock_tag : node_clock_tags) {
                //Should we be analyzing paths between these two domains?
                if(tc.should_analyze(node_data_tag.clock_domain(), node_clock_tag.clock_domain())) {

                    //We only set a required time if the source domain actually reaches this sink
                    //domain.  This is indicated by having a valid arrival time.
                    if(node_data_tag.arr_time().valid()) {
                        float clock_constraint = tc.hold_clock_constraint(node_data_tag.clock_domain(),
                                                                          node_clock_tag.clock_domain());

                        node_data_tag.max_req(node_clock_tag.arr_time() + Time(clock_constraint), node_data_tag);
                    }
                }
            }
        }
    }
}

/*
 * Arrival Time Operations
 */

template<class DelayCalc>
void HoldAnalysisVisitor::do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) {
    //Pull from upstream sources to current node
    for(EdgeId edge_id : tg.node_in_edges(node_id)) {
        do_arrival_traverse_edge(tg, dc, node_id, edge_id);
    }
}

template<class DelayCalc>
void HoldAnalysisVisitor::do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = get_hold_data_tags(node_id);
    TimingTags& node_clock_tags = get_hold_clock_tags(node_id);

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    const Time& edge_delay = dc.min_edge_delay(tg, edge_id);

    /*
     * Clock tags
     */
    if(tg.node_type(src_node_id) != TN_Type::FF_SOURCE) {
        //Do not propagate clock tags from an FF Source,
        //the clock arrival there will have already been converted to a
        //data tag when the previuos level was processed
        const TimingTags& src_clk_tags = get_hold_clock_tags(src_node_id);
        for(const TimingTag& src_clk_tag : src_clk_tags) {
            //Standard propagation through the clock network
            node_clock_tags.min_arr(src_clk_tag.arr_time() + edge_delay, src_clk_tag);

            if(tg.node_type(node_id) == TN_Type::FF_SOURCE) {
                //This is a clock to data launch edge
                //
                //We convert the clock arrival time to a
                //data arrival time at this node (since the clock arrival
                //launches the data)

                //Make a copy of the tag
                TimingTag launch_tag = src_clk_tag;

                //Update the launch node, since the data is
                //launching from this node
                launch_tag.set_launch_node(src_node_id);

                //Mark propagated launch time as a DATA tag
                node_data_tags.min_arr(launch_tag.arr_time() + edge_delay, launch_tag);
            }
        }
    }

    /*
     * Data tags
     */

    const TimingTags& src_data_tags = get_hold_data_tags(src_node_id);

    for(const TimingTag& src_data_tag : src_data_tags) {
        //Standard data-path propagation
        node_data_tags.min_arr(src_data_tag.arr_time() + edge_delay, src_data_tag);
    }
}

/*
 * Required Time Operations
 */
template<class DelayCalc>
void HoldAnalysisVisitor::do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) {
    //Pull from downstream sinks to current node

    //We don't propagate required times past FF_CLOCK nodes,
    //since anything upstream is part of the clock network
    //
    //TODO: if performing optimization on a clock network this may actually be useful
    if(tg.node_type(node_id) == TN_Type::FF_CLOCK) {
        return;
    }

    //Each back-edge from down stream node
    for(EdgeId edge_id : tg.node_out_edges(node_id)) {
        do_required_traverse_edge(tg, dc, node_id, edge_id);
    }
}

template<class DelayCalc>
void HoldAnalysisVisitor::do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = get_hold_data_tags(node_id);

    //Pulling values from downstream sink node
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    const Time& edge_delay = dc.min_edge_delay(tg, edge_id);

    const TimingTags& sink_data_tags = get_hold_data_tags(sink_node_id);

    for(const TimingTag& sink_tag : sink_data_tags) {
        //We only propagate the tag if we have a valid arrival time
        auto matched_tag_iter = node_data_tags.find_tag_by_clock_domain(sink_tag.clock_domain());
        if(matched_tag_iter != node_data_tags.end() && matched_tag_iter->arr_time().valid()) {
            //Valid arrival, update required
            matched_tag_iter->max_req(sink_tag.req_time() - edge_delay, sink_tag);
        }
    }
}

/*
 * Utility
 */

void HoldAnalysisVisitor::reset() {
    size_t num_tags = hold_data_tags_.size();

    hold_data_tags_ = std::vector<TimingTags>(num_tags);
    hold_clock_tags_ = std::vector<TimingTags>(num_tags);
}

