#ifndef TATUM_COMMON_ANALYSIS_VISITOR_HPP
#define TATUM_COMMON_ANALYSIS_VISITOR_HPP
#include "tatum_error.hpp"
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
        void do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id);

        void reset() { ops_.reset(); }

    protected:
        AnalysisOps ops_;

    private:
        template<class DelayCalc>
        void do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class DelayCalc>
        void do_required_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        bool should_propagate_clock_arr(const TimingGraph& tg, const TimingConstraints& tc, const EdgeId edge_id) const;
        bool is_clock_data_edge(const TimingGraph& tg, const EdgeId edge_id) const;

};

/*
 * Pre-traversal
 */

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    //Logical Input
    TATUM_ASSERT_MSG(tg.node_in_edges(node_id).size() == 0, "Logical input has input edges: timing graph not levelized.");

    NodeType node_type = tg.node_type(node_id);

    //We don't propagate any tags from constant generators,
    //since they do not effect the dynamic timing behaviour of the
    //system
    if(tc.node_is_constant_generator(node_id)) return;

    if(tc.node_is_clock_source(node_id)) {
        //Generate the appropriate clock tag

        //Note that we assume that edge counting has set the effective period constraint assuming a
        //launch edge at time zero.  This means we don't need to do anything special for clocks
        //with rising edges after time zero.
        TATUM_ASSERT_MSG(ops_.get_clock_tags(node_id).num_tags() == 0, "Clock source already has clock tags");

        //Find it's domain
        DomainId domain_id = tc.node_clock_domain(node_id);
        TATUM_ASSERT(domain_id);

        //Initialize a clock tag with zero arrival, invalid required time
        TimingTag clock_tag = TimingTag(Time(0.), Time(NAN), domain_id, node_id);

        //Add the tag
        ops_.get_clock_tags(node_id).add_tag(clock_tag);

    } else {
        TATUM_ASSERT(node_type == NodeType::SOURCE);

        //A standard primary input, generate the appropriate data tag

        //We assume input delays are on the arc from INPAD_SOURCE to INPAD_OPIN,
        //so we do not need to account for it directly in the arrival time of INPAD_SOURCES

        TATUM_ASSERT_MSG(ops_.get_data_tags(node_id).num_tags() == 0, "Primary input already has data tags");

        DomainId domain_id = tc.node_clock_domain(node_id);
        TATUM_ASSERT(domain_id);

        //Initialize a data tag with zero arrival, invalid required time
        TimingTag input_tag = TimingTag(Time(0.), Time(NAN), domain_id, node_id);

        ops_.get_data_tags(node_id).add_tag(input_tag);
    }
}

template<class AnalysisOps>
void CommonAnalysisVisitor<AnalysisOps>::do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {

    TimingTags& node_data_tags = ops_.get_data_tags(node_id);
    TimingTags& node_clock_tags = ops_.get_clock_tags(node_id);

    /*
     * Calculate required times
     */
    auto node_type = tg.node_type(node_id);
    TATUM_ASSERT(node_type == NodeType::SINK);

    //Sinks corresponding to FF sinks will have propagated clock tags,
    //while those corresponding to outpads will not.
    if(node_clock_tags.empty()) {
        //Initialize the outpad's clock tags based on the specified constraints.


        auto output_constraints = tc.output_constraints(node_id);

        if(output_constraints.empty()) {
            //throw tatum::Error("Output unconstrained");
            std::cerr << "Warning: Timing graph " << node_id << " " << node_type << " has no incomming clock tags, and no output constraint. No required time will be calculated\n";

#if 1
            //Debug trace-back

            //TODO: remove debug code!
            if(node_type == NodeType::FF_SINK) {
                std::cerr << "\tClock path:\n";
                int i = 0;
                NodeId curr_node = node_id;
                while(tg.node_type(curr_node) != NodeType::INPAD_SOURCE &&
                      tg.node_type(curr_node) != NodeType::CLOCK_SOURCE &&
                      tg.node_type(curr_node) != NodeType::CONSTANT_GEN_SOURCE &&
                      i < 100) {
                    
                    //Look throught the fanin for a clock or other node to follow
                    //
                    //Typically the first hop from node_id will be to either the IPIN or CLOCK pin
                    //the following preferentially prefers the CLOCK pin to show the clock path
                    EdgeId best_edge;
                    for(auto edge_id : tg.node_in_edges(curr_node)) {
                        if(!best_edge) {
                            best_edge = edge_id;
                        }

                        NodeId src_node = tg.edge_src_node(edge_id);
                        auto src_node_type = tg.node_type(src_node);
                        if(src_node_type == NodeType::FF_CLOCK) {
                            best_edge = edge_id;
                        }
                    }

                    //Step back
                    curr_node = tg.edge_src_node(best_edge);
                    auto curr_node_type = tg.node_type(curr_node);


                    std::cerr << "\tNode " << curr_node << " Type: " << curr_node_type << "\n";
                    if(++i >= 100) {
                        std::cerr << "\tStopping backtrace\n";
                        
                    }
                }
            }
#endif

        } else {
            for(auto constraint : output_constraints) {
                //TODO: use real constraint value when output delay no-longer on edges
                TimingTag constraint_tag = TimingTag(Time(0.), Time(NAN), constraint.second.domain, node_id);
                node_clock_tags.add_tag(constraint_tag);
            }
        }
    }

    //At this stage both FF and outpad sinks now have the relevant clock 
    //tags and we can process them equivalently

    //Determine the required time at this sink
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

/*
 * Arrival Time Operations
 */

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, NodeId node_id) {
    //Do not propagate arrival tags through constant generators
    if(tc.node_is_constant_generator(node_id)) return;

    //Pull from upstream sources to current node
    for(EdgeId edge_id : tg.node_in_edges(node_id)) {
        do_arrival_traverse_edge(tg, tc, dc, node_id, edge_id);
    }
}

template<class AnalysisOps>
template<class DelayCalc>
void CommonAnalysisVisitor<AnalysisOps>::do_arrival_traverse_edge(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {
    //We must use the tags by reference so we don't accidentally wipe-out any
    //existing tags
    TimingTags& node_data_tags = ops_.get_data_tags(node_id);
    TimingTags& node_clock_tags = ops_.get_clock_tags(node_id);

    //Pulling values from upstream source node
    NodeId src_node_id = tg.edge_src_node(edge_id);

    const Time& edge_delay = ops_.edge_delay(dc, tg, edge_id);

    const TimingTags& src_clk_tags = ops_.get_clock_tags(src_node_id);
    const TimingTags& src_data_tags = ops_.get_data_tags(src_node_id);

    /*
     * Clock tags
     */

    if(should_propagate_clock_arr(tg, tc, edge_id)) {
        //Propagate the clock tags through the clock network

        for(const TimingTag& src_clk_tag : src_clk_tags) {
            //Standard propagation through the clock network
            ops_.merge_arr_tags(node_clock_tags, src_clk_tag.arr_time() + edge_delay, src_clk_tag);

            if(is_clock_data_edge(tg, edge_id)) {
                //We convert the clock arrival time to a data
                //arrival time at this node (since the clock's
                //arrival launches the data).
                TATUM_ASSERT(tg.node_type(node_id) == NodeType::SOURCE);

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
void CommonAnalysisVisitor<AnalysisOps>::do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id) {
    //Do not propagate required tags through constant generators
    if(tc.node_is_constant_generator(node_id)) return;

    //Pull from downstream sinks to current node
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

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::should_propagate_clock_arr(const TimingGraph& tg, const TimingConstraints& tc, const EdgeId edge_id) const {
    NodeId src_node_id = tg.edge_src_node(edge_id);

    //We want to propagate clock tags through the arbitrary nodes making up the clock network until 
    //we hit another source node (i.e. a FF's output source).
    //
    //To allow tags to propagte from the original source (i.e. the input clock pin) we also allow
    //propagation from defined clock sources
    NodeType src_node_type = tg.node_type(src_node_id);
    if (src_node_type != NodeType::SOURCE) {
        //Not a source, allow propagation
        return true;
    } else if (tc.node_is_clock_source(src_node_id)) {
        //Base-case: the source is a clock source
        TATUM_ASSERT_MSG(node_type == NodeType::SOURCE, "Only SOURCEs can be clock sources");
        TATUM_ASSERT_MSG(tg.node_in_edges(src_node_id).empty(), "Clock sources should have no incoming edges");
        return true;
    }
    return false;
}

template<class AnalysisOps>
bool CommonAnalysisVisitor<AnalysisOps>::is_clock_data_edge(const TimingGraph& tg, const EdgeId edge_id) const {
    NodeId edge_src_node = tg.edge_src_node(edge_id);
    NodeId edge_sink_node = tg.edge_sink_node(edge_id);

    return (tg.node_type(edge_src_node) == NodeType::SINK) && (tg.node_type(edge_sink_node) == NodeType::SOURCE);
}

}} //namepsace

#endif
