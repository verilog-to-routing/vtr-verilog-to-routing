#ifndef TATUM_COMMON_ANALYSIS_VISITOR_HPP
#define TATUM_COMMON_ANALYSIS_VISITOR_HPP
#include "tatum/error.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/tags/TimingTags.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/graph_visitors/GraphVisitor.hpp"

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
class CommonAnalysisVisitor : public GraphVisitor {
    public:
        CommonAnalysisVisitor(size_t num_tags, size_t num_slacks)
            : ops_(num_tags, num_slacks) { }

        void do_reset_node(const NodeId node_id) override { ops_.reset_node(node_id); }
#ifdef TATUM_CALCULATE_EDGE_SLACKS
        void do_reset_edge(const EdgeId edge_id) override { ops_.reset_edge(edge_id); }
#endif

        void do_reset_node_arrival_tags(const NodeId node_id) override;
        void do_reset_node_required_tags(const NodeId node_id) override;
        void do_reset_node_slack_tags(const NodeId node_id) override;

        void do_reset_node_arrival_tags_from_origin(const NodeId node_id, const NodeId origin) override;
        void do_reset_node_required_tags_from_origin(const NodeId node_id, const NodeId origin) override;

        bool do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) override;

        bool do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) override;

        bool do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id) override;

        bool do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id) override;

        bool do_slack_traverse_node(const TimingGraph& tg, const DelayCalculator& dc, const NodeId node) override;

    protected:
        AnalysisOps ops_;

    private:
        bool do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id, const EdgeId edge_id);

        bool do_required_traverse_edge(const TimingGraph& tg, const DelayCalculator& dc, const NodeId node_id, const EdgeId edge_id);

        bool do_slack_traverse_edge(const TimingGraph& tg, const DelayCalculator& dc, const EdgeId edge);

        bool mark_sink_required_times(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node);

        bool should_propagate_clocks(const TimingGraph& tg, const TimingConstraints& tc, const EdgeId edge_id) const;
        bool should_propagate_clock_launch_tags(const TimingGraph& tg, const EdgeId edge_id) const;
        bool should_propagate_clock_capture_tags(const TimingGraph& tg, const EdgeId edge_id) const;
        bool should_propagate_data(const TimingGraph& tg, const EdgeId edge_id) const;
        bool should_calculate_slack(const TimingTag& src_tag, const TimingTag& sink_tag) const;

        bool is_clock_data_launch_edge(const TimingGraph& tg, const EdgeId edge_id) const;
        bool is_clock_data_capture_edge(const TimingGraph& tg, const EdgeId edge_id) const;
};

/*
 * Pre-traversal
 */

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_reset_node_arrival_tags(const NodeId node_id) {
    for (TagType type : {TagType::CLOCK_LAUNCH, TagType::CLOCK_CAPTURE, TagType::DATA_ARRIVAL}) {
        for (TimingTag& tag : ops_.get_mutable_tags(node_id, type)) {
            tag.set_origin_node(NodeId::INVALID());        
            tag.set_time(ops_.invalid_arrival_time());
        }
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_reset_node_required_tags(const NodeId node_id) {
    for (TagType type : {TagType::DATA_REQUIRED}) {
        for (TimingTag& tag : ops_.get_mutable_tags(node_id, type)) {
            tag.set_origin_node(NodeId::INVALID());        
            tag.set_time(ops_.invalid_required_time());
        }
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_reset_node_slack_tags(const NodeId node_id) {
    for (TimingTag& tag : ops_.get_mutable_slack_tags(node_id)) {
        tag.set_origin_node(NodeId::INVALID());        
        tag.set_time(ops_.invalid_slack_time());
    }
}


template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_reset_node_arrival_tags_from_origin(const NodeId node_id, const NodeId origin) {
    for (TagType type : {TagType::CLOCK_LAUNCH, TagType::CLOCK_CAPTURE, TagType::DATA_ARRIVAL}) {
        for (TimingTag& tag : ops_.get_mutable_tags(node_id, type)) {
            if (tag.origin_node() == origin) {
                tag.set_origin_node(NodeId::INVALID());        
                tag.set_time(ops_.invalid_arrival_time());
            }
        }
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_reset_node_required_tags_from_origin(const NodeId node_id, const NodeId origin) {
    for (TagType type : {TagType::DATA_REQUIRED}) {
        for (TimingTag& tag : ops_.get_mutable_tags(node_id, type)) {
            if (tag.origin_node() == origin) {
                tag.set_origin_node(NodeId::INVALID());        
                tag.set_time(ops_.invalid_required_time());
            }
        }
    }
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Logical Input

    //We expect this function to only be called on nodes in the first level of the timing graph
    //These nodes must have no un-disabled input edges (else they shouldn't be in the first level).
    //In the normal case (primary input) there are no incoming edges. However if set_disable_timing
    //was used it may be that the edges were explicitly disabled. We therefore verify that there are
    //no un-disabled edges in the fanin of the current node.
    TATUM_ASSERT_MSG(tg.node_num_active_in_edges(node_id) == 0, "Logical input has non-disabled input edges: timing graph not levelized.");

    //
    //We now generate the various clock/data launch tags associated with the arrival time traversal
    //

    NodeType node_type = tg.node_type(node_id);

    bool node_constrained = false;

    if(tc.node_is_constant_generator(node_id)) {
        //We progpagate the tags from constant generators to ensure any sinks driven 
        //only by constant generators are recorded as constrained.
        //
        //We use a special tag to initialize constant generators which gets overritten
        //by any non-constant tag at downstream nodes

        TimingTag const_gen_tag = ops_.const_gen_tag();
        ops_.set_tag(node_id, const_gen_tag);

        node_constrained = true;
    } else {

        TATUM_ASSERT(node_type == NodeType::SOURCE);

        if(tc.node_is_clock_source(node_id)) {
            //Generate the appropriate clock tag

            //Find the domain of this node (since it is a source)
            DomainId domain_id = tc.node_clock_domain(node_id);
            TATUM_ASSERT(domain_id);

            //Initialize a clock launch tag from this to any capture domain
            //
            //Note: we assume that edge counting has set the effective period constraint assuming a
            //launch edge at time zero + source latency.  This means we don't need to do anything 
            //special for clocks with rising edges after time zero.
            Time launch_source_latency = ops_.launch_source_latency(tc, domain_id);
            TimingTag launch_tag = TimingTag(launch_source_latency, 
                                             domain_id,
                                             DomainId::INVALID(), //Any capture
                                             NodeId::INVALID(), //Origin
                                             TagType::CLOCK_LAUNCH);
            //Add the launch tag
            ops_.set_tag(node_id, launch_tag);

            //Initialize the clock capture tags from any valid launch domain to this domain
            //
            //Note that we enumerate all pairs of valid launch domains for the current domain 
            //(which is now treated as the capture domain), since each pair may have different constraints
            for(DomainId launch_domain_id : tc.clock_domains()) {
                if(tc.should_analyze(launch_domain_id, domain_id)) {

                    //Initialize the clock capture tag with the constraint, including the effect of any source latency
                    //
                    //Note: We assume that this period constraint has been resolved by edge counting for this
                    //domain pair. Note that it does not include the effect of clock uncertainty, which is handled
                    //when the caputre tag is converted into a data-arrival tag.
                    //
                    //Also note that this is the default clock constraint. If there is a different per capture node
                    //constraint this is also handled when setting the required time.
                    Time clock_constraint = ops_.clock_constraint(tc, launch_domain_id, domain_id);

                    Time capture_source_latency = ops_.capture_source_latency(tc, domain_id);

                    TimingTag capture_tag = TimingTag(Time(capture_source_latency) + Time(clock_constraint), 
                                                      launch_domain_id,
                                                      domain_id,
                                                      NodeId::INVALID(), //Origin
                                                      TagType::CLOCK_CAPTURE);

                    ops_.set_tag(node_id, capture_tag);

                    node_constrained = true;
                }
            }
        } else {
            //A standard primary input, generate the appropriate data tags

            //No longer true for an incremental analyzer
            //TATUM_ASSERT_MSG(ops_.get_tags(node_id, TagType::DATA_ARRIVAL).size() == 0, "Primary input already has data tags");

            auto input_constraints = ops_.input_constraints(tc, node_id);
            if(!input_constraints.empty()) { //Some inputs may be unconstrained, so do not create tags for them

                DomainId domain_id = tc.node_clock_domain(node_id);
                TATUM_ASSERT(domain_id);

                //The external clock may have latency
                Time launch_source_latency = ops_.launch_source_latency(tc, domain_id);

                //An input constraint means there is 'input_constraint' delay from when an external 
                //signal is launched by its clock (external to the chip) until it arrives at the
                //primary input
                Time input_constraint = ops_.input_constraint(tc, node_id, domain_id);
                TATUM_ASSERT(input_constraint.valid());

                //Initialize a data tag based on input delay constraint
                TimingTag input_tag = TimingTag(launch_source_latency + input_constraint, 
                                                domain_id, 
                                                DomainId::INVALID(), 
                                                NodeId::INVALID(), //Origin
                                                TagType::DATA_ARRIVAL);

                ops_.merge_arr_tags(node_id, input_tag);

                node_constrained = true;
            }
        }
    }

    return node_constrained;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& /*tc*/, const NodeId node_id) {

    NodeType node_type = tg.node_type(node_id);
    TATUM_ASSERT(node_type == NodeType::SINK);

    return is_constrained(node_type, ops_.get_tags(node_id));
}

/*
 * Arrival Time Operations
 */

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, NodeId node_id) {

    bool node_modified = false;

    //Pull from upstream sources to current node
    for(EdgeId edge_id : tg.node_in_edges(node_id)) {

        if(tg.edge_disabled(edge_id)) continue;

        node_modified |= do_arrival_traverse_edge(tg, tc, dc, node_id, edge_id);
    }

    if(tg.node_type(node_id) == NodeType::SINK) {
        node_modified |= mark_sink_required_times(tg, tc, dc, node_id);
    }

    return node_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id, const EdgeId edge_id) {
    bool timing_modified = false;

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    if(should_propagate_clocks(tg, tc, edge_id)) {
        /*
         * Clock tags
         */

        //Propagate the clock tags through the clock network

        //The launch tags
        if(should_propagate_clock_launch_tags(tg, edge_id)) {
            TimingTags::tag_range src_launch_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_LAUNCH);

            if(!src_launch_clk_tags.empty()) {
                const Time clk_launch_edge_delay = ops_.launch_clock_edge_delay(dc, tg, edge_id);

                for(const TimingTag& src_launch_clk_tag : src_launch_clk_tags) {
                    //Standard propagation through the clock network
                    Time new_arr = src_launch_clk_tag.time() + clk_launch_edge_delay;
                    timing_modified |= ops_.merge_arr_tags(node_id, new_arr, src_node_id, src_launch_clk_tag);

                }
            }
        }

        //The capture tags
        if(should_propagate_clock_capture_tags(tg, edge_id)) {
            TimingTags::tag_range src_capture_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_CAPTURE);

            if(!src_capture_clk_tags.empty()) {
                const Time clk_capture_edge_delay = ops_.capture_clock_edge_delay(dc, tg, edge_id);

                for(const TimingTag& src_capture_clk_tag : src_capture_clk_tags) {
                    //Standard propagation through the clock network

                   /*
                    * Each source capture tag may be related to clock domain of one of the two types:
                    * 1. clock domain created for netlist clock, meant to be used with cells clocked at the rising edge
                    * 2. inverted virtual clock domain based on existing netlist clock.
                    *    It is 180 degree phase shifted in relation to netlist clock
                    *    and it is used in timing analysis for cells clocked at falling edge.
                    *
                    * The triggering edges of FFs should be taken into account when propagating tags
                    * to CPIN nodes. Tags of types which are incompatible with triggering edges
                    * of the FF clock inputs shouldn't be propagated because having those wouldn't allow
                    * timing analysis to calculate correct timings for transfers between cells
                    * clocked rising and falling edges of the same clock.
                    */
                    if ( tg.node_type(sink_node_id) == NodeType::CPIN ) {
                        if ((tg.trigg_edge(sink_node_id) == TriggeringEdge::FALLING_EDGE && !tc.clock_domain_inverted(src_capture_clk_tag.launch_clock_domain())) ||
                            (tg.trigg_edge(sink_node_id) == TriggeringEdge::RISING_EDGE &&  tc.clock_domain_inverted(src_capture_clk_tag.launch_clock_domain()))) {
                            //Skip propagation of timings derived from incompatible constraints
                            continue;
                        }
                    }

                    timing_modified |= ops_.merge_arr_tags(node_id, src_capture_clk_tag.time() + clk_capture_edge_delay, src_node_id, src_capture_clk_tag);
                }
            }
        }
    }

    /*
     * Data Arrival tags
     */

    if(is_clock_data_launch_edge(tg, edge_id)) {
        //Convert the launch clock into a data arrival

        //We convert the clock arrival time at the upstream node into a data
        //arrival time at this node (since the clock's arrival launches the data).
        TATUM_ASSERT_SAFE(tg.node_type(node_id) == NodeType::SOURCE);

        TimingTags::tag_range src_launch_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_LAUNCH);
        if(!src_launch_clk_tags.empty()) {
            const Time launch_edge_delay = ops_.launch_clock_edge_delay(dc, tg, edge_id);

            for(const TimingTag& src_launch_clk_tag : src_launch_clk_tags) {
                //Convert clock launch into data arrival
                TimingTag data_arr_tag = src_launch_clk_tag;
                data_arr_tag.set_type(TagType::DATA_ARRIVAL);
                Time arr_time = src_launch_clk_tag.time() + launch_edge_delay;


                //Mark propagated launch time as a DATA tag
                timing_modified |= ops_.merge_arr_tags(node_id, 
                                    arr_time, 
                                    NodeId::INVALID(), //Origin
                                    data_arr_tag);
            }
        }
    }

    if(should_propagate_data(tg, edge_id)) {
        //Standard data path propagation

        TimingTags::tag_range src_data_tags = ops_.get_tags(src_node_id, TagType::DATA_ARRIVAL);
        if(!src_data_tags.empty()) {
            const Time edge_delay = ops_.data_edge_delay(dc, tg, edge_id);
            TATUM_ASSERT_SAFE(edge_delay.valid());

            for(const TimingTag& src_data_tag : src_data_tags) {
                Time new_arr = src_data_tag.time() + edge_delay;
                timing_modified |= ops_.merge_arr_tags(node_id, new_arr, src_node_id, src_data_tag);
            }
        }
    }

    //NOTE: we do not handle clock caputure edges (which create required times) here, but in
    //      mark_sink_required_times(), after all edges have been processed (i.e. all arrival times
    //      set)
    //
    //To calulate the required times at a node we must know all the arrival times at the node.
    //Since we don't know the order the incomming edges are processed, we don't know whether all
    //arrival times have been set until *after* all edges have been processed. 
    //
    //As a result we set the required times only after all the edges have been processed

    //Return whether this edge changed the current nodes timing
    return timing_modified;
}

/*
 * Required Time Operations
 */


template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& /*tc*/, const DelayCalculator& dc, const NodeId node_id) {
    bool node_modified = false; //For now, always assume modified

    //Don't propagate required times through the clock network
    if(tg.node_type(node_id) == NodeType::CPIN) return node_modified;


    //Pull from downstream sinks to current node
    for(EdgeId edge_id : tg.node_out_edges(node_id)) {

        if(tg.edge_disabled(edge_id)) continue;

        node_modified |= do_required_traverse_edge(tg, dc, node_id, edge_id);
    }

    return node_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_edge(const TimingGraph& tg, const DelayCalculator& dc, const NodeId node_id, const EdgeId edge_id) {
    bool timing_modified = false;

    //Pulling values from downstream sink node
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    TimingTags::tag_range sink_data_tags = ops_.get_tags(sink_node_id, TagType::DATA_REQUIRED);

    if(!sink_data_tags.empty()) {
        const Time& edge_delay = ops_.data_edge_delay(dc, tg, edge_id);
        TATUM_ASSERT_SAFE(edge_delay.valid());

        for(const TimingTag& sink_tag : sink_data_tags) {
            //We only propogate the required time if we have a valid matching arrival time
            timing_modified |= ops_.merge_req_tags(node_id, sink_tag.time() - edge_delay, sink_node_id, sink_tag, true);
        }
    }

    return timing_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_slack_traverse_node(const TimingGraph& tg, const DelayCalculator& dc, const NodeId node) {
    bool timing_modified = false;

    //Calculate the slack for each edge
#ifdef TATUM_CALCULATE_EDGE_SLACKS
    for(const EdgeId edge : tg.node_in_edges(node)) {
        timing_modified |= do_slack_traverse_edge(tg, dc, edge);
    }
#else
    //Avoid unused param warnings
    static_cast<void>(dc);
    static_cast<void>(tg);
#endif

    //Calculate the slacks at each node
    for(const TimingTag& arr_tag : ops_.get_tags(node, TagType::DATA_ARRIVAL)) {
        for(const TimingTag& req_tag : ops_.get_tags(node, TagType::DATA_REQUIRED)) {
            if(!should_calculate_slack(arr_tag, req_tag)) continue;

            Time slack_value = ops_.calculate_slack(req_tag.time(), arr_tag.time());

            timing_modified |= ops_.merge_slack_tags(node, slack_value, req_tag);
        }
    }

    return timing_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::do_slack_traverse_edge(const TimingGraph& tg, const DelayCalculator& dc, const EdgeId edge) {
    bool timing_modified = false;

    NodeId src_node = tg.edge_src_node(edge);
    NodeId sink_node = tg.edge_sink_node(edge);

    auto src_arr_tags = ops_.get_tags(src_node, TagType::DATA_ARRIVAL);
    auto sink_req_tags = ops_.get_tags(sink_node, TagType::DATA_REQUIRED);

    Time edge_delay;
    if(is_clock_data_launch_edge(tg, edge)) {
        edge_delay = ops_.launch_clock_edge_delay(dc, tg, edge);
    } else if(is_clock_data_capture_edge(tg, edge)) {
        edge_delay = ops_.capture_clock_edge_delay(dc, tg, edge);
    } else {
        edge_delay = ops_.data_edge_delay(dc, tg, edge);
    }

    for(const tatum::TimingTag& src_arr_tag : src_arr_tags) {

        for(const tatum::TimingTag& sink_req_tag : sink_req_tags) {
            if(!should_calculate_slack(src_arr_tag, sink_req_tag)) continue;

            Time slack_value = sink_req_tag.time() - src_arr_tag.time() - edge_delay;

            timing_modified |= ops_.merge_slack_tags(edge, slack_value, sink_req_tag);
        }
    }

    return timing_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::mark_sink_required_times(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id) {
    //Mark the required times of the current sink node
    TATUM_ASSERT(tg.node_type(node_id) == NodeType::SINK);
    bool timing_modified = false;

    //Note: since we add tags at the current node (and the tags are all stored together),
    //we must *copy* the data arrival tags before adding any new tags (since adding new 
    //tags may invalidate the old tag references)
    auto data_arr_range = ops_.get_tags(node_id, TagType::DATA_ARRIVAL);
    std::vector<TimingTag> node_data_arr_tags(data_arr_range.begin(), data_arr_range.end());

    EdgeId clock_capture_edge = tg.node_clock_capture_edge(node_id);

    if(clock_capture_edge) {
        //Required time at sink FF
        NodeId src_node_id = tg.edge_src_node(clock_capture_edge);
        TimingTags::tag_range src_capture_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_CAPTURE);

        const Time capture_edge_delay = ops_.capture_clock_edge_delay(dc, tg, clock_capture_edge);

        for(const TimingTag& src_capture_clk_tag : src_capture_clk_tags) {
            DomainId clock_launch_domain = src_capture_clk_tag.launch_clock_domain();
            DomainId clock_capture_domain = src_capture_clk_tag.capture_clock_domain();
            for(const TimingTag& node_data_arr_tag : node_data_arr_tags) {
                DomainId data_launch_domain = node_data_arr_tag.launch_clock_domain();

                if(is_const_gen_tag(node_data_arr_tag)) {
                    //A constant generator tag. Required time is not terribly well defined,
                    //so just use the capture clock tag values since we nevery look at these
                    //tags except to check that any downstream nodes have been constrained
                    data_launch_domain = src_capture_clk_tag.capture_clock_domain();
                }

                //We produce a fully specified capture clock tags (both launch and capture) so we only want 
                //to consider the capture clock tag which matches the data launch domain
                bool same_launch_domain = (data_launch_domain == clock_launch_domain);

                //We only want to analyze paths between domains where a valid constraint has been specified
                bool valid_launch_capture_pair = tc.should_analyze(data_launch_domain, clock_capture_domain, node_id);

                if(same_launch_domain && valid_launch_capture_pair) {

                    //We only set a required time if the source domain actually reaches this sink
                    //domain.  This is indicated by the presence of an arrival tag (which should have
                    //a valid arrival time).
                    TATUM_ASSERT(node_data_arr_tag.time().valid());

                    //If there is a per-sink override for the clock constraint we need to adjust the clock
                    //arrival time from the default
                    Time clock_constraint_offset =   ops_.clock_constraint(tc, data_launch_domain, clock_capture_domain, node_id)
                                                   - ops_.clock_constraint(tc, data_launch_domain, clock_capture_domain);

                    //We apply the clock uncertainty to the generated required time tag
                    Time clock_uncertainty = ops_.clock_uncertainty(tc, data_launch_domain, clock_capture_domain);

                    Time req_time =   src_capture_clk_tag.time() //Latency + propagated clock network delay to CPIN
                                    + clock_constraint_offset    //Period constraint adjustment
                                    + capture_edge_delay         //CPIN to sink delay (Thld, or Tsu)
                                    + Time(clock_uncertainty);   //Clock period uncertainty

                    TimingTag node_data_req_tag(req_time, 
                                                data_launch_domain, 
                                                clock_capture_domain, 
                                                NodeId::INVALID(), //Origin
                                                TagType::DATA_REQUIRED);

                    timing_modified |= ops_.merge_req_tags(node_id, node_data_req_tag);
                }
            }
        }
    } else {
        //Must be a primary-output sink, need to set required tags based on output constraints

        DomainId io_capture_domain = tc.node_clock_domain(node_id);

        //Any constrained primary outputs should have a specified clock domain
        // Note that some outputs may not be constrained and hence should not get required times
        if(io_capture_domain) {

            //An output constraint means there is output_constraint delay outside the chip,
            //as a result signals need to reach the primary-output at least output_constraint
            //before the capture clock.
            //
            //Hence we use a negative output constraint value to subtract the output constraint 
            //from the target clock constraint
            Time output_constraint = -ops_.output_constraint(tc, node_id, io_capture_domain);
            if (output_constraint.valid()) {

                //Since there is no propagated clock tag to primary outputs, we need to account for
                //the capture source clock latency
                Time capture_clock_source_latency = ops_.capture_source_latency(tc, io_capture_domain);

                for(const TimingTag& node_data_arr_tag : node_data_arr_tags) {
                    DomainId data_launch_domain = node_data_arr_tag.launch_clock_domain();

                    if(is_const_gen_tag(node_data_arr_tag)) {
                        //A constant generator tag. Required time is not terribly well defined,
                        //so just use the inter-domain values since we nevery look at these
                        //tags except to check that any downstream nodes have been constrained

                        data_launch_domain = io_capture_domain;
                    }

                    //Should we be analyzing paths between these two domains?
                    if(tc.should_analyze(data_launch_domain, io_capture_domain, node_id)) {

                        //We only set a required time if the source domain actually reaches this sink
                        //domain.  This is indicated by the presence of an arrival tag (which should have
                        //a valid arrival time).
                        TATUM_ASSERT(node_data_arr_tag.time().valid());

                        Time constraint = ops_.clock_constraint(tc, data_launch_domain, io_capture_domain, node_id);

                        Time clock_uncertainty = ops_.clock_uncertainty(tc, data_launch_domain, io_capture_domain);

                        //Calulate the required time
                        Time req_time =   Time(constraint)                   //Period constraint
                                        + Time(capture_clock_source_latency) //Latency from true clock source to def'n point
                                        + Time(output_constraint)            //Output delay
                                        + Time(clock_uncertainty);           //Clock period uncertainty

                        TimingTag node_data_req_tag(req_time, 
                                                    data_launch_domain, 
                                                    io_capture_domain, 
                                                    NodeId::INVALID(), //Origin
                                                    TagType::DATA_REQUIRED);

                        timing_modified |= ops_.merge_req_tags(node_id, node_data_req_tag);
                    }
                }
            }
        }
    }

    return timing_modified;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_clocks(const TimingGraph& tg, const TimingConstraints& tc, const EdgeId edge_id) const {
    //We want to propagate clock tags through the arbitrary nodes making up the clock network until 
    //we hit another source node (i.e. a FF's output source).
    //
    //To allow tags to propagte from the original source (i.e. the input clock pin) we also allow
    //propagation from defined clock sources
    NodeId src_node_id = tg.edge_src_node(edge_id);
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    if (tg.node_type(sink_node_id) != NodeType::SOURCE) {
        //Not a source, allow propagation

        if (tc.node_is_clock_source(src_node_id)) {
            //The source is a clock source
            TATUM_ASSERT_MSG(tg.node_type(src_node_id) == NodeType::SOURCE, "Only SOURCEs can be clock sources");
            TATUM_ASSERT_MSG(tg.node_in_edges(src_node_id).empty(), "Clock sources should have no incoming edges");
        }
        return true;
    }
    return false;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_clock_launch_tags(const TimingGraph& tg, const EdgeId edge_id) const {
    return !is_clock_data_capture_edge(tg, edge_id);
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_clock_capture_tags(const TimingGraph& tg, const EdgeId edge_id) const {
    NodeId sink_node = tg.edge_sink_node(edge_id);
    return tg.node_type(sink_node) != NodeType::SINK;
}


template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::is_clock_data_launch_edge(const TimingGraph& tg, const EdgeId edge_id) const {
    NodeId edge_src_node = tg.edge_src_node(edge_id);
    NodeId edge_sink_node = tg.edge_sink_node(edge_id);

    return (tg.node_type(edge_src_node) == NodeType::CPIN) && (tg.node_type(edge_sink_node) == NodeType::SOURCE);
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::is_clock_data_capture_edge(const TimingGraph& tg, const EdgeId edge_id) const {
    NodeId edge_src_node = tg.edge_src_node(edge_id);
    NodeId edge_sink_node = tg.edge_sink_node(edge_id);

    return (tg.node_type(edge_src_node) == NodeType::CPIN) && (tg.node_type(edge_sink_node) == NodeType::SINK);
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_data(const TimingGraph& tg, const EdgeId edge_id) const {
    //We want to propagate data tags unless then re-enter the clock network
    NodeId src_node_id = tg.edge_src_node(edge_id);
    NodeType src_node_type = tg.node_type(src_node_id);
    if (src_node_type != NodeType::CPIN) {
        //Do not allow data tags to propagate through clock pins
        return true;
    }
    return false;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_calculate_slack(const TimingTag& src_tag, const TimingTag& sink_tag) const {
    TATUM_ASSERT_SAFE(src_tag.type() == TagType::DATA_ARRIVAL && sink_tag.type() == TagType::DATA_REQUIRED);

    //NOTE: we do not need to check the constraints to determine whether this domain pair should be analyzed,
    //      this check has already been done when we created the DATA_REQUIRED tags (i.e. sink tags in this context),
    //      ensuring we only calculate slack for valid domain pairs (otherwise the DATA_REQUIREd tag would note exist)


    return src_tag.launch_clock_domain() == sink_tag.launch_clock_domain();

}

}} //namepsace

#endif
