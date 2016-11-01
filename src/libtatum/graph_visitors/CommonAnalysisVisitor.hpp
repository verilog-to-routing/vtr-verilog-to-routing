#ifndef TATUM_COMMON_ANALYSIS_VISITOR_HPP
#define TATUM_COMMON_ANALYSIS_VISITOR_HPP
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"

namespace tatum { namespace detail {

/** \file
 *
 * Common analysis functionality for both setup and hold analysis.
 */

/** \class CommonAnalysisVisitor
 *
 * A class satisfying the GraphVisitor concept, which contains common
 * node and edge processing code used by both setup and hold analysis.
 *
 * \see GraphVisitor
 *
 * \tparam AnalysisOps a class defining the setup/hold specific operations
 * \see SetupAnalysisOps
 * \see HoldAnalysisOps
 */
template<class AnalysisOps>
class CommonAnalysisVisitor {
    public:
        CommonAnalysisVisitor(size_t num_tags)
            : ops_(num_tags) { }

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        void reset() { ops_.reset(); }

    protected:
        AnalysisOps ops_;

    private:
        template<class DelayCalc>
        void do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

};

/*
 * Pre-traversal
 */

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& /*tc*/, const NodeId node_id) {
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
        TATUM_ASSERT_MSG(ops_.get_clock_tags(node_id).num_tags() == 0, "Clock source already has clock tags");

        //Initialize a clock tag with zero arrival, invalid required time
        TimingTag clock_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Add the tag
        ops_.get_clock_tags(node_id).add_tag(clock_tag);

    } else {
        TATUM_ASSERT(node_type == TN_Type::INPAD_SOURCE);

        //A standard primary input

        //We assume input delays are on the arc from INPAD_SOURCE to INPAD_OPIN,
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES

        //Initialize a data tag with zero arrival, invalid required time
        TimingTag input_tag = TimingTag(Time(0.), Time(NAN), tg.node_clock_domain(node_id), node_id);

        //Figure out if we are an input which defines a clock
        if(tg.node_is_clock_source(node_id)) {
            TATUM_ASSERT_MSG(ops_.get_clock_tags(node_id).num_tags() == 0, "Logical input already has clock tags");

            ops_.get_clock_tags(node_id).add_tag(input_tag);
        } else {
            TATUM_ASSERT_MSG(ops_.get_data_tags(node_id).num_tags() == 0, "Logical input already has data tags");

            ops_.get_data_tags(node_id).add_tag(input_tag);
        }
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    TN_Type node_type = tg.node_type(node_id);

    TimingTags& node_data_tags = ops_.get_data_tags(node_id);
    TimingTags& node_clock_tags = ops_.get_clock_tags(node_id);

    /*
     * Calculate required times
     */
    if(node_type == TN_Type::OUTPAD_SINK) {
        //Determine the required time for outputs.
        //
        //We assume any output delay is on the OUTPAT_IPIN to OUTPAD_SINK edge,
        //so we only need set the constraint on the OUTPAD_SINK
        DomainId node_domain = tg.node_clock_domain(node_id);
        for(const TimingTag& data_tag : node_data_tags) {
            //Should we be analyzing paths between these two domains?
            if(tc.should_analyze(data_tag.clock_domain(), node_domain)) {
                //These clock domains should be analyzed

                float clock_constraint = ops_.clock_constraint(tc, data_tag.clock_domain(), node_domain);

                //Set the required time on the sink.
                ops_.merge_req_tags(node_data_tags, Time(clock_constraint), data_tag);
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
                        float clock_constraint = ops_.clock_constraint(tc, node_data_tag.clock_domain(),
                                                                      node_clock_tag.clock_domain());

                        //Update the required time. This will keep the most restrictive constraint.
                        ops_.merge_req_tag(node_data_tag, node_clock_tag.arr_time() + Time(clock_constraint), node_data_tag);
                    }
                }
            }
        }
    }
}

/*
 * Arrival Time Operations
 */

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, NodeId node_id) {
    //Pull from upstream sources to current node
    for(EdgeId edge_id : tg.node_in_edges(node_id)) {
        do_arrival_traverse_edge(tg, dc, node_id, edge_id);
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = ops_.get_data_tags(node_id);
    TimingTags& node_clock_tags = ops_.get_clock_tags(node_id);

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    const Time& edge_delay = ops_.edge_delay(dc, tg, edge_id);

    /*
     * Clock tags
     */
    if(tg.node_type(src_node_id) != TN_Type::FF_SOURCE) {
        //We do not propagate clock tags from an FF_SOURCE.
        //The clock arrival will have already been converted to a
        //data tag when the previous level was traversed.

        const TimingTags& src_clk_tags = ops_.get_clock_tags(src_node_id);
        for(const TimingTag& src_clk_tag : src_clk_tags) {
            //Standard propagation through the clock network
            ops_.merge_arr_tags(node_clock_tags, src_clk_tag.arr_time() + edge_delay, src_clk_tag);

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

                //Mark propagated launch time as a DATA tag
                ops_.merge_arr_tags(node_data_tags, launch_tag.arr_time() + edge_delay, launch_tag);
            }
        }
    }

    /*
     * Data tags
     */
    const TimingTags& src_data_tags = ops_.get_data_tags(src_node_id);

    for(const TimingTag& src_data_tag : src_data_tags) {
        //Standard data-path propagation
        ops_.merge_arr_tags(node_data_tags, src_data_tag.arr_time() + edge_delay, src_data_tag);
    }
}

/*
 * Required Time Operations
 */


template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) {
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

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = ops_.get_data_tags(node_id);

    //Pulling values from downstream sink node
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    const Time& edge_delay = ops_.edge_delay(dc, tg, edge_id);

    const TimingTags& sink_data_tags = ops_.get_data_tags(sink_node_id);

    for(const TimingTag& sink_tag : sink_data_tags) {
        //We only propogate the required time if we have a valid arrival time
        auto matched_tag_iter = node_data_tags.find_tag_by_clock_domain(sink_tag.clock_domain());
        if(matched_tag_iter != node_data_tags.end() && matched_tag_iter->arr_time().valid()) {
            //Valid arrival, update required
            ops_.merge_req_tag(*matched_tag_iter, sink_tag.req_time() - edge_delay, sink_tag);
        }
    }
}

}} //namepsace

#endif
