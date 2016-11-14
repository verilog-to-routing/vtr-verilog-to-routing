#pragma once
#include "SerialWalker.hpp"
#include "HoldAnalysis.hpp"
#include "HoldTimingAnalyzer.hpp"

namespace tatum { namespace detail {

/**
 * A concrete implementation of a HoldTimingAnalyzer.
 *
 * This is a full (i.e. non-incremental) analyzer, which fully
 * re-analyzes the timing graph whenever update_timing_impl() is 
 * called.
 */
template<class DelayCalc,
         template<class V, class D> class GraphWalker=SerialWalker>
class FullHoldTimingAnalyzer : public HoldTimingAnalyzer {
    public:
        FullHoldTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalc& delay_calculator)
            : HoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , hold_visitor_(timing_graph_.nodes().size())
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, delay_calculator_, hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, timing_constraints_, delay_calculator_, hold_visitor_);            
        }

        virtual void reset_timing_impl() override { hold_visitor_.reset(); }

        double get_profiling_data_impl(std::string key) override { return graph_walker_.get_profiling_data(key); }

        const TimingTags& get_hold_data_tags_impl(NodeId node_id) const override { return hold_visitor_.get_hold_data_tags(node_id); }
        const TimingTags& get_hold_launch_clock_tags_impl(NodeId node_id) const override { return hold_visitor_.get_hold_launch_clock_tags(node_id); }
        const TimingTags& get_hold_capture_clock_tags_impl(NodeId node_id) const override { return hold_visitor_.get_hold_capture_clock_tags(node_id); }

    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalc& delay_calculator_;
        HoldAnalysis hold_visitor_;
        GraphWalker<HoldAnalysis, DelayCalc> graph_walker_;
};

}} //namepsace
