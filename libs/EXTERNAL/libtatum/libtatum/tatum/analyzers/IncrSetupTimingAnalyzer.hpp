#pragma once
#include "tatum/graph_walkers/SerialWalker.hpp"
#include "tatum/SetupAnalysis.hpp"
#include "tatum/analyzers/SetupTimingAnalyzer.hpp"
#include "tatum/base/validate_timing_graph_constraints.hpp"

namespace tatum { namespace detail {

/**
 * A concrete implementation of a SetupTimingAnalyzer.
 *
 * This is an incremental analyzer, which will incrementally
 * update the timing graph based on edges which have been marked
 * as invalidated.
 */
template<class GraphWalker=SerialIncrWalker>
class IncrSetupTimingAnalyzer : public SetupTimingAnalyzer {
    public:
        IncrSetupTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalculator& delay_calculator)
            : SetupTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , setup_visitor_(timing_graph_.nodes().size(), timing_graph_.edges().size()) {
            validate_timing_graph_constraints(timing_graph_, timing_constraints_);

            //Initialize profiling data
            graph_walker_.set_profiling_data("total_analysis_sec", 0.);
            graph_walker_.set_profiling_data("analysis_sec", 0.);
            graph_walker_.set_profiling_data("num_full_updates", 0.);
            graph_walker_.set_profiling_data("num_incr_updates", 0.);
        }

    protected:
        //Update both setup and hold simultaneously (this is more efficient than updating them sequentially)
        virtual void update_timing_impl() override {
            update_setup_timing_impl();
        }
        virtual void update_setup_timing_impl() override {
            auto start_time = Clock::now();

            if (never_updated_) {
                //Invalidate all edges
                for (EdgeId edge : timing_graph_.edges()) {
                    graph_walker_.invalidate_edge(edge);
                }

                //Only need to pre-traverse the first update
                graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_visitor_);            
            }

            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, delay_calculator_, setup_visitor_);            

            if (never_updated_) {
                //Only need to pre-traverse the first update
                graph_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, setup_visitor_);            
            }

            graph_walker_.do_required_traversal(timing_graph_, timing_constraints_, delay_calculator_, setup_visitor_);            

            graph_walker_.do_update_slack(timing_graph_, delay_calculator_, setup_visitor_);

            double analysis_sec = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();

            graph_walker_.clear_invalidated_edges();

            //Record profiling data
            double total_analysis_sec = analysis_sec + graph_walker_.get_profiling_data("total_analysis_sec");
            graph_walker_.set_profiling_data("total_analysis_sec", total_analysis_sec);
            graph_walker_.set_profiling_data("analysis_sec", analysis_sec);
            graph_walker_.set_profiling_data("num_incr_updates", graph_walker_.get_profiling_data("num_incr_updates") + 1);

            never_updated_ = false;
        }

        virtual void invalidate_edge_impl(const EdgeId edge) override {
            graph_walker_.invalidate_edge(edge);
        }

        virtual node_range modified_nodes_impl() const override {
            return graph_walker_.modified_nodes();
        }

        double get_profiling_data_impl(std::string key) const override { return graph_walker_.get_profiling_data(key); }
        size_t num_unconstrained_startpoints_impl() const override { return graph_walker_.num_unconstrained_startpoints(); }
        size_t num_unconstrained_endpoints_impl() const override { return graph_walker_.num_unconstrained_endpoints(); }

        TimingTags::tag_range setup_tags_impl(NodeId node_id) const override { return setup_visitor_.setup_tags(node_id); }
        TimingTags::tag_range setup_tags_impl(NodeId node_id, TagType type) const override { return setup_visitor_.setup_tags(node_id, type); }
#ifdef TATUM_CALCULATE_EDGE_SLACKS
        TimingTags::tag_range setup_edge_slacks_impl(EdgeId edge_id) const override { return setup_visitor_.setup_edge_slacks(edge_id); }
#endif
        TimingTags::tag_range setup_node_slacks_impl(NodeId node_id) const override { return setup_visitor_.setup_node_slacks(node_id); }
    private:
        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalculator& delay_calculator_;
        SetupAnalysis setup_visitor_;
        GraphWalker graph_walker_;

        bool never_updated_ = true;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

}} //namepsace
