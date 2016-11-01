#pragma once
#include <memory>
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "timing_analyzer_interfaces.hpp"
#include "graph_walkers.hpp"

#include "SetupAnalysis.hpp"
#include "HoldAnalysis.hpp"
#include "SetupHoldAnalysis.hpp"

namespace tatum {

/** \file
 * This file defines concrete implementations of the TimingAnalyzer interfaces.
 *
 * In particular these concrete analyzers are 'full' (i.e. non-incremental) timing analyzers,
 * ever call to update_timing() fully re-analyze the timing graph.
 *
 * Note that at this time only 'full' analyzers are supported.
 */

template<class DelayCalc,
         template<class V, class D> class GraphWalker=SerialWalker>
class SetupFullTimingAnalyzer : public SetupTimingAnalyzer {
    public:
        SetupFullTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalc& delay_calculator)
            : SetupTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , setup_visitor_(timing_graph_.nodes().size())
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, delay_calculator_, setup_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, setup_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, setup_visitor_);            
        }

        virtual void reset_timing_impl() override { setup_visitor_.reset(); }

        double get_profiling_data_impl(std::string key) override { return graph_walker_.get_profiling_data(key); }

        const TimingTags& get_setup_data_tags_impl(NodeId node_id) const override { return setup_visitor_.get_setup_data_tags(node_id); }
        const TimingTags& get_setup_clock_tags_impl(NodeId node_id) const override { return setup_visitor_.get_setup_clock_tags(node_id); }


    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalc& delay_calculator_;
        SetupAnalysis setup_visitor_;
        GraphWalker<SetupAnalysis, DelayCalc> graph_walker_;

        std::map<std::string,double> profiling_data_;
};

template<class DelayCalc,
         template<class V, class D> class GraphWalker=SerialWalker>
class HoldFullTimingAnalyzer : public HoldTimingAnalyzer {
    public:
        HoldFullTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalc& delay_calculator)
            : HoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , hold_visitor_(timing_graph_.nodes().size())
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, delay_calculator_, hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, hold_visitor_);            
        }

        virtual void reset_timing_impl() override { hold_visitor_.reset(); }

        double get_profiling_data_impl(std::string key) override { return graph_walker_.get_profiling_data(key); }

        const TimingTags& get_hold_data_tags_impl(NodeId node_id) const override { return hold_visitor_.get_hold_data_tags(node_id); }
        const TimingTags& get_hold_clock_tags_impl(NodeId node_id) const override { return hold_visitor_.get_hold_clock_tags(node_id); }

    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalc& delay_calculator_;
        HoldAnalysis hold_visitor_;
        GraphWalker<HoldAnalysis, DelayCalc> graph_walker_;
};

template<class DelayCalc,
         template<class V, class D> class GraphWalker=SerialWalker>
class SetupHoldFullTimingAnalyzer : public SetupHoldTimingAnalyzer {
    public:
        SetupHoldFullTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalc& delay_calculator)
            : SetupHoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , setup_hold_visitor_(timing_graph_.nodes().size())
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, delay_calculator_, setup_hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, setup_hold_visitor_);            
        }

        virtual void reset_timing_impl() override { setup_hold_visitor_.reset(); }

        double get_profiling_data_impl(std::string key) override { return graph_walker_.get_profiling_data(key); }

        const TimingTags& get_setup_data_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.get_setup_data_tags(node_id); }
        const TimingTags& get_setup_clock_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.get_setup_clock_tags(node_id); }
        const TimingTags& get_hold_data_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.get_hold_data_tags(node_id); }
        const TimingTags& get_hold_clock_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.get_hold_clock_tags(node_id); }

    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalc& delay_calculator_;
        SetupHoldAnalysis setup_hold_visitor_;
        GraphWalker<SetupHoldAnalysis, DelayCalc> graph_walker_;
};

} //namepsace
