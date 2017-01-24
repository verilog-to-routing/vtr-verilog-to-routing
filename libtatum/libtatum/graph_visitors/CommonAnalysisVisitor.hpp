#ifndef TATUM_COMMON_ANALYSIS_VISITOR_HPP
#define TATUM_COMMON_ANALYSIS_VISITOR_HPP
#include "tatum_error.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"

//#define LOG_TRAVERSAL_ORDER

#ifdef LOG_TRAVERSAL_ORDER
# include <iostream>
#endif

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
        CommonAnalysisVisitor(size_t num_tags, size_t num_slacks)
            : ops_(num_tags, num_slacks) { }

        void do_reset_node(const NodeId node_id) { ops_.reset_node(node_id); }
        void do_reset_edge(const EdgeId edge_id) { ops_.reset_edge(edge_id); }

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_slack_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node);

    protected:
        AnalysisOps ops_;

    private:
        template<class DelayCalc>
        void do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_slack_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const EdgeId edge);

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
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Logical Input
    TATUM_ASSERT_MSG(tg.node_in_edges(node_id).size() == 0, "Logical input has input edges: timing graph not levelized.");

    NodeType node_type = tg.node_type(node_id);

    if(tc.node_is_constant_generator(node_id)) {
        //We don't propagate any tags from constant generators,
        //since they do not effect the dynamic timing behaviour of the
        //system
        return;
    }

    TATUM_ASSERT(node_type == NodeType::SOURCE);

    if(tc.node_is_clock_source(node_id)) {
        //Generate the appropriate clock tag

        TATUM_ASSERT_MSG(ops_.get_tags(node_id, TagType::CLOCK_LAUNCH).size() == 0, "Uninitialized clock source should have no launch clock tags");
        TATUM_ASSERT_MSG(ops_.get_tags(node_id, TagType::CLOCK_CAPTURE).size() == 0, "Uninitialized clock source should have no capture clock tags");

        //Find it's domain
        DomainId domain_id = tc.node_clock_domain(node_id);
        TATUM_ASSERT(domain_id);

        //Initialize a clock tag with zero arrival, invalid required time
        //
        //Note: we assume that edge counting has set the effective period constraint assuming a
        //launch edge at time zero.  This means we don't need to do anything special for clocks
        //with rising edges after time zero.
        TimingTag launch_tag = TimingTag(Time(0.), domain_id, DomainId::INVALID(), node_id, TagType::CLOCK_LAUNCH);
        TimingTag capture_tag = TimingTag(Time(0.), DomainId::INVALID(), domain_id, node_id, TagType::CLOCK_CAPTURE);

        //Add the tag
        ops_.add_tag(node_id, launch_tag);
        ops_.add_tag(node_id, capture_tag);

    } else {

        //A standard primary input, generate the appropriate data tag

        TATUM_ASSERT_MSG(ops_.get_tags(node_id, TagType::DATA_ARRIVAL).size() == 0, "Primary input already has data tags");

        DomainId domain_id = tc.node_clock_domain(node_id);
        TATUM_ASSERT(domain_id);

        float input_constraint = tc.input_constraint(node_id, domain_id);
        TATUM_ASSERT(!isnan(input_constraint));

        //Initialize a data tag based on input delay constraint, invalid required time
        TimingTag input_tag = TimingTag(Time(input_constraint), domain_id, DomainId::INVALID(), node_id, TagType::DATA_ARRIVAL);

        ops_.add_tag(node_id, input_tag);
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {

    /*
     * Calculate required times
     */

    TATUM_ASSERT(tg.node_type(node_id) == NodeType::SINK);

    //Sinks corresponding to FF sinks will have propagated (capturing) clock tags,
    //while those corresponding to outpads will not. To treat them uniformly, we 
    //initialize outpads with capturing clock tags based on the ouput constraints.
    TimingTags::tag_range node_clock_tags = ops_.get_tags(node_id, TagType::CLOCK_CAPTURE);
    if(node_clock_tags.empty()) {
        //Initialize the clock tags based on the constraints.

        auto output_constraints = tc.output_constraints(node_id);

        if(output_constraints.empty()) {
            //throw tatum::Error("Output unconstrained");
            std::cerr << "Warning: Timing graph " << node_id << " " << tg.node_type(node_id);
            std::cerr << " has no incomming clock tags, and no output constraint. No required time will be calculated\n";
        } else {
            DomainId domain_id = tc.node_clock_domain(node_id);
            TATUM_ASSERT(domain_id);

            float output_constraint = tc.output_constraint(node_id, domain_id);
            TATUM_ASSERT(!isnan(output_constraint));

            for(auto constraint : output_constraints) {
                TimingTag constraint_tag = TimingTag(Time(output_constraint), DomainId::INVALID(), constraint.second.domain, node_id, TagType::CLOCK_CAPTURE);
                ops_.add_tag(node_id, constraint_tag);
            }
        }
    }

    //At this point the sink will have a capturing clock tag defined

    //Determine the required time at this sink
    //
    //We need to generate a required time for each clock domain for which there is a data
    //arrival time at this node, while considering all possible clocks that could drive
    //this node (i.e. take the most restrictive constraint across all clock tags at this
    //node)


    //Note: since we add tags at the current node (and the tags are all stored together),
    //we must *copy* the data arrival and clock capture tags before adding any new tags
    //(since adding new tags may invalidate the old tag references)
    auto data_arr_range = ops_.get_tags(node_id, TagType::DATA_ARRIVAL);
    auto clock_capture_range = ops_.get_tags(node_id, TagType::CLOCK_CAPTURE);
    std::vector<TimingTag> node_data_arr_tags(data_arr_range.begin(), data_arr_range.end());
    std::vector<TimingTag> node_clock_capture_tags(clock_capture_range.begin(), clock_capture_range.end());

    for(const TimingTag& node_data_arr_tag : node_data_arr_tags) {
        for(const TimingTag& node_clock_tag : node_clock_capture_tags) {

            //Should we be analyzing paths between these two domains?
            if(tc.should_analyze(node_data_arr_tag.launch_clock_domain(), node_clock_tag.capture_clock_domain())) {

                //We only set a required time if the source domain actually reaches this sink
                //domain.  This is indicated by having a valid arrival time.
                if(node_data_arr_tag.time().valid()) {
                    float clock_constraint = ops_.clock_constraint(tc, node_data_arr_tag.launch_clock_domain(),
                                                                       node_clock_tag.capture_clock_domain());

                    //Update the required time. This will keep the most restrictive constraint.
                    TimingTag node_data_req_tag(node_clock_tag.time() + Time(clock_constraint), 
                                                node_data_arr_tag.launch_clock_domain(), 
                                                node_clock_tag.capture_clock_domain(), 
                                                node_id, 
                                                TagType::DATA_REQUIRED);
                    ops_.add_tag(node_id, node_data_req_tag);
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
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, NodeId node_id) {

#ifdef LOG_TRAVERSAL_ORDER
    std::cout << "Arrival Traverse " << node_id << "\n";
#endif

    //Pull from upstream sources to current node
    for(EdgeId edge_id : tg.node_in_edges(node_id)) {

        if(tg.edge_disabled(edge_id)) continue;

#ifdef LOG_TRAVERSAL_ORDER
        std::cout << "Arrival Traverse " << edge_id << "\n";
#endif

        do_arrival_traverse_edge(tg, tc, dc, node_id, edge_id);
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    if(should_propagate_clocks(tg, tc, edge_id)) {
        /*
         * Clock tags
         */

        //Propagate the clock tags through the clock network

        //The launch tags
        if(should_propagate_clock_launch_tags(tg, edge_id)) {
            TimingTags::tag_range src_launch_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_LAUNCH);

            if(!src_launch_clk_tags.empty()) {
                const Time& clk_launch_edge_delay = ops_.launch_clock_edge_delay(dc, tg, edge_id);

                for(const TimingTag& src_launch_clk_tag : src_launch_clk_tags) {
                    //Standard propagation through the clock network

                    Time new_arr = src_launch_clk_tag.time() + clk_launch_edge_delay;
                    ops_.merge_arr_tags(node_id, new_arr, src_launch_clk_tag);

                    if(is_clock_data_launch_edge(tg, edge_id)) {
                        //We convert the clock arrival time to a data
                        //arrival time at this node (since the clock's
                        //arrival launches the data).
                        TATUM_ASSERT_SAFE(tg.node_type(node_id) == NodeType::SOURCE);

                        //Make a copy of the tag
                        TimingTag launch_tag = src_launch_clk_tag;

                        //Update the launch node, since the data is
                        //launching from this node
                        launch_tag.set_origin_node(node_id);
                        launch_tag.set_type(TagType::DATA_ARRIVAL);

                        //Mark propagated launch time as a DATA tag
                        ops_.merge_arr_tags(node_id, new_arr, launch_tag);
                    }
                }
            }
        }

        //The capture tags
        if(should_propagate_clock_capture_tags(tg, edge_id)) {
            TimingTags::tag_range src_capture_clk_tags = ops_.get_tags(src_node_id, TagType::CLOCK_CAPTURE);

            if(!src_capture_clk_tags.empty()) {
                const Time& clk_capture_edge_delay = ops_.capture_clock_edge_delay(dc, tg, edge_id);

                for(const TimingTag& src_capture_clk_tag : src_capture_clk_tags) {
                    //Standard propagation through the clock network
                    ops_.merge_arr_tags(node_id, src_capture_clk_tag.time() + clk_capture_edge_delay, src_capture_clk_tag);
                }
            }
        }
    }

    /*
     * Data tags
     */

    TimingTags::tag_range src_data_tags = ops_.get_tags(src_node_id, TagType::DATA_ARRIVAL);

    if(!src_data_tags.empty() && should_propagate_data(tg, edge_id)) {
        const Time& edge_delay = ops_.data_edge_delay(dc, tg, edge_id);
        TATUM_ASSERT_SAFE(edge_delay.valid());

        for(const TimingTag& src_data_tag : src_data_tags) {
            //Standard data-path propagation
            Time new_arr = src_data_tag.time() + edge_delay;
            ops_.merge_arr_tags(node_id, new_arr, src_data_tag);
        }
    }
}

/*
 * Required Time Operations
 */


template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& /*tc*/, const DelayCalc& dc, const NodeId node_id) {
#ifdef LOG_TRAVERSAL_ORDER
    std::cout << "Required Traverse " << node_id << "\n";
#endif

    //Don't propagate required times through the clock network
    if(tg.node_type(node_id) == NodeType::CPIN) return;


    //Pull from downstream sinks to current node
    for(EdgeId edge_id : tg.node_out_edges(node_id)) {

        if(tg.edge_disabled(edge_id)) continue;

#ifdef LOG_TRAVERSAL_ORDER
        std::cout << "Required Traverse " << edge_id << "\n";
#endif

        do_required_traverse_edge(tg, dc, node_id, edge_id);
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {

    //Pulling values from downstream sink node
    NodeId sink_node_id = tg.edge_sink_node(edge_id);

    TimingTags::tag_range sink_data_tags = ops_.get_tags(sink_node_id, TagType::DATA_REQUIRED);

    if(!sink_data_tags.empty()) {
        const Time& edge_delay = ops_.data_edge_delay(dc, tg, edge_id);
        TATUM_ASSERT_SAFE(edge_delay.valid());

        for(const TimingTag& sink_tag : sink_data_tags) {
            //We only propogate the required time if we have a valid matching arrival time
            ops_.merge_req_tags(node_id, sink_tag.time() - edge_delay, sink_tag, true);
        }
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_slack_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node) {
    //Calculate the slack for each edge
    for(const EdgeId edge : tg.node_in_edges(node)) {
        do_slack_traverse_edge(tg, dc, edge);
    }

    //Calculate the slacks at each node
    for(const TimingTag& arr_tag : ops_.get_tags(node, TagType::DATA_ARRIVAL)) {
        for(const TimingTag& req_tag : ops_.get_tags(node, TagType::DATA_REQUIRED)) {
            if(!should_calculate_slack(arr_tag, req_tag)) continue;

            Time slack_value = req_tag.time() - arr_tag.time();

            ops_.merge_slack_tags(node, slack_value, req_tag);
        }
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_slack_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const EdgeId edge) {
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

            ops_.merge_slack_tags(edge, slack_value, sink_req_tag);
        }
    }
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_clocks(const TimingGraph& tg, const TimingConstraints& tc, const EdgeId edge_id) const {
    //We want to propagate clock tags through the arbitrary nodes making up the clock network until 
    //we hit another source node (i.e. a FF's output source).
    //
    //To allow tags to propagte from the original source (i.e. the input clock pin) we also allow
    //propagation from defined clock sources
    NodeId src_node_id = tg.edge_src_node(edge_id);
    NodeType src_node_type = tg.node_type(src_node_id);
    if (src_node_type != NodeType::SOURCE) {
        //Not a source, allow propagation
        return true;
    } else if (tc.node_is_clock_source(src_node_id)) {
        //Base-case: the source is a clock source
        TATUM_ASSERT_MSG(src_node_type == NodeType::SOURCE, "Only SOURCEs can be clock sources");
        TATUM_ASSERT_MSG(tg.node_in_edges(src_node_id).empty(), "Clock sources should have no incoming edges");
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
    return !is_clock_data_launch_edge(tg, edge_id);
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

    return src_tag.launch_clock_domain() == sink_tag.launch_clock_domain();
}

}} //namepsace

#endif
