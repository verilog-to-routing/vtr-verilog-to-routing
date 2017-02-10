#pragma once
#include "SerialWalker.hpp"
#include "SetupHoldAnalysis.hpp"
#include "SetupHoldTimingAnalyzer.hpp"
#include "validate_timing_graph_constraints.hpp"

namespace tatum { namespace detail {

/**
 * A concrete implementation of a SetupHoldTimingAnalyzer.
 *
 * This is a full (i.e. non-incremental) analyzer, which fully
 * re-analyzes the timing graph whenever update_timing_impl() is 
 * called.
 */
template<class DelayCalc,
         template<class V, class D> class GraphWalker=SerialWalker>
class FullSetupHoldTimingAnalyzer : public SetupHoldTimingAnalyzer {
    public:
        FullSetupHoldTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalc& delay_calculator)
            : SetupHoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , setup_hold_visitor_(timing_graph_.nodes().size(), timing_graph_.edges().size()) {
            validate_timing_graph_constraints(timing_graph_, timing_constraints_);

            //Initialize profiling data
            graph_walker_.set_profiling_data("total_analysis_sec", 0.);
            graph_walker_.set_profiling_data("analysis_sec", 0.);
            graph_walker_.set_profiling_data("num_full_updates", 0.);
        }

    protected:
        virtual void update_timing_impl() override {
            auto start_time = Clock::now();

            graph_walker_.do_reset(timing_graph_, setup_hold_visitor_);

            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, delay_calculator_, setup_hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, timing_constraints_, delay_calculator_, setup_hold_visitor_);            

            double analysis_sec = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();

            //Record profiling data
            double total_analysis_sec = analysis_sec + graph_walker_.get_profiling_data("total_analysis_sec");
            graph_walker_.set_profiling_data("total_analysis_sec", total_analysis_sec);
            graph_walker_.set_profiling_data("analysis_sec", analysis_sec);
            graph_walker_.set_profiling_data("num_full_updates", graph_walker_.get_profiling_data("num_full_updates") + 1);
        }

        double get_profiling_data_impl(std::string key) const override { return graph_walker_.get_profiling_data(key); }

        TimingTags::tag_range setup_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.setup_tags(node_id); }
        TimingTags::tag_range setup_tags_impl(NodeId node_id, TagType type) const override { return setup_hold_visitor_.setup_tags(node_id, type); }
        TimingTags::tag_range setup_edge_slacks_impl(EdgeId edge_id) const override { return setup_hold_visitor_.setup_edge_slacks(edge_id); }
        TimingTags::tag_range setup_node_slacks_impl(NodeId node_id) const override { return setup_hold_visitor_.setup_node_slacks(node_id); }

        TimingTags::tag_range hold_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.hold_tags(node_id); }
        TimingTags::tag_range hold_tags_impl(NodeId node_id, TagType type) const override { return setup_hold_visitor_.hold_tags(node_id, type); }
        TimingTags::tag_range hold_edge_slacks_impl(EdgeId edge_id) const override { return setup_hold_visitor_.hold_edge_slacks(edge_id); }
        TimingTags::tag_range hold_node_slacks_impl(NodeId node_id) const override { return setup_hold_visitor_.hold_node_slacks(node_id); }

    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalc& delay_calculator_;
        SetupHoldAnalysis setup_hold_visitor_;
        GraphWalker<SetupHoldAnalysis, DelayCalc> graph_walker_;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

}} //namepsace
