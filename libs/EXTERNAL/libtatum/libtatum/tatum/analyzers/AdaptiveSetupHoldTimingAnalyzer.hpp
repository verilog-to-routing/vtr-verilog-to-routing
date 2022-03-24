#pragma once
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/graph_walkers/SerialWalker.hpp"
#include "tatum/graph_walkers/SerialIncrWalker.hpp"
#include "tatum/SetupHoldAnalysis.hpp"
#include "tatum/analyzers/SetupHoldTimingAnalyzer.hpp"
#include "tatum/base/validate_timing_graph_constraints.hpp"
#include "tatum/graph_walkers/TimingGraphWalker.hpp"

namespace tatum { namespace detail {

/** Threshold for AdaptiveSetupHoldTimingAnalyzer to use full updates. 
* Expressed as fraction of all edges in timing graph. */
constexpr float full_update_threshold = 0.1;

/**
 * A concrete implementation of a SetupHoldTimingAnalyzer.
 *
 * This is an adaptive analyzer: can do incremental updates if the number of invalidated
 * nodes is small, falls back to a full update after a certain threshold to avoid the overhead.
 */
template<class FullWalker=SerialWalker, class IncrWalker=SerialIncrWalker>
class AdaptiveSetupHoldTimingAnalyzer : public SetupHoldTimingAnalyzer {
    public:
        AdaptiveSetupHoldTimingAnalyzer(const TimingGraph& timing_graph, const TimingConstraints& timing_constraints, const DelayCalculator& delay_calculator)
            : SetupHoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints)
            , delay_calculator_(delay_calculator)
            , setup_hold_visitor_(timing_graph_.nodes().size(), timing_graph_.edges().size()) {
            validate_timing_graph_constraints(timing_graph_, timing_constraints_);

            //Initialize profiling data. Use full walker to store data for both
            full_walker_.set_profiling_data("total_analysis_sec", 0.);
            full_walker_.set_profiling_data("analysis_sec", 0.);
            full_walker_.set_profiling_data("num_full_updates", 0.);
            full_walker_.set_profiling_data("num_incr_updates", 0.);

            mode_ = Mode::INCR;
            n_modified_edges_ = 0;
            max_modified_edges_ = timing_graph_.edges().size() * full_update_threshold;
        }

    protected:
        //Update both setup and hold simultaneously (this is more efficient than updating them sequentially)
        virtual void update_timing_impl() override {
            auto start_time = Clock::now();

            if(mode_ == Mode::INCR)
                update_timing_incr_(setup_hold_visitor_);
            else
                update_timing_full_(setup_hold_visitor_);

            clear_timing_incr_();

            double analysis_sec = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();

            //Record profiling data (use full walker to store it) (arbitrary choice)
            double total_analysis_sec = analysis_sec + full_walker_.get_profiling_data("total_analysis_sec");
            full_walker_.set_profiling_data("total_analysis_sec", total_analysis_sec);
            full_walker_.set_profiling_data("analysis_sec", analysis_sec);
            if(mode_ == Mode::INCR)
                full_walker_.set_profiling_data("num_incr_updates", full_walker_.get_profiling_data("num_incr_updates") + 1);
            else
                full_walker_.set_profiling_data("num_full_updates", full_walker_.get_profiling_data("num_full_updates") + 1);

            mode_ = Mode::INCR; /* We did our update, try to use incr until too many edges are modified */
        }

        //Update only setup timing
        virtual void update_setup_timing_impl() override {
            auto& setup_visitor = setup_hold_visitor_.setup_visitor();
            
            if(mode_ == Mode::INCR)
                update_timing_incr_(setup_visitor);
            else
                update_timing_full_(setup_visitor);
        }

        //Update only hold timing
        virtual void update_hold_timing_impl() override {
            auto& hold_visitor = setup_hold_visitor_.hold_visitor();

            if(mode_ == Mode::INCR)
                update_timing_incr_(hold_visitor);
            else
                update_timing_full_(hold_visitor);
        }

        virtual void invalidate_edge_impl(const EdgeId edge) override {
            if(mode_ == Mode::FULL)
                return;
            incr_walker_.invalidate_edge(edge);
            n_modified_edges_++;
            if(n_modified_edges_ > max_modified_edges_)
                mode_ = Mode::FULL;
        }

        virtual node_range modified_nodes_impl() const override {
            if(mode_ == Mode::FULL)
                return full_walker_.modified_nodes();
            else
                return incr_walker_.modified_nodes();
        }

        double get_profiling_data_impl(std::string key) const override {
            return full_walker_.get_profiling_data(key);
        }

        size_t num_unconstrained_startpoints_impl() const override {
            if(mode_ == Mode::FULL)
                return full_walker_.num_unconstrained_startpoints();
            else
                return incr_walker_.num_unconstrained_startpoints();
        }

        size_t num_unconstrained_endpoints_impl() const override {
            if(mode_ == Mode::FULL)
                return full_walker_.num_unconstrained_endpoints();
            else
                return incr_walker_.num_unconstrained_endpoints();
        }

        TimingTags::tag_range setup_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.setup_tags(node_id); }
        TimingTags::tag_range setup_tags_impl(NodeId node_id, TagType type) const override { return setup_hold_visitor_.setup_tags(node_id, type); }
#ifdef TATUM_CALCULATE_EDGE_SLACKS
        TimingTags::tag_range setup_edge_slacks_impl(EdgeId edge_id) const override { return setup_hold_visitor_.setup_edge_slacks(edge_id); }
#endif
        TimingTags::tag_range setup_node_slacks_impl(NodeId node_id) const override { return setup_hold_visitor_.setup_node_slacks(node_id); }

        TimingTags::tag_range hold_tags_impl(NodeId node_id) const override { return setup_hold_visitor_.hold_tags(node_id); }
        TimingTags::tag_range hold_tags_impl(NodeId node_id, TagType type) const override { return setup_hold_visitor_.hold_tags(node_id, type); }
#ifdef TATUM_CALCULATE_EDGE_SLACKS
        TimingTags::tag_range hold_edge_slacks_impl(EdgeId edge_id) const override { return setup_hold_visitor_.hold_edge_slacks(edge_id); }
#endif
        TimingTags::tag_range hold_node_slacks_impl(NodeId node_id) const override { return setup_hold_visitor_.hold_node_slacks(node_id); }

    private:
        /** Update using the full walker */
        void update_timing_full_(GraphVisitor& visitor){
            full_walker_.do_reset(timing_graph_, visitor);

            full_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, visitor);            
            full_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, delay_calculator_, visitor);            

            full_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, visitor);            
            full_walker_.do_required_traversal(timing_graph_, timing_constraints_, delay_calculator_, visitor);            

            full_walker_.do_update_slack(timing_graph_, delay_calculator_, visitor);
        }

        /** Update using the incremental walker */
        void update_timing_incr_(GraphVisitor& visitor){
            if (never_updated_incr_) {
                //Invalidate all edges
                for (EdgeId edge : timing_graph_.edges()) {
                    incr_walker_.invalidate_edge(edge);
                }

                //Only need to pre-traverse the first update
                incr_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, visitor);            
            }

            incr_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, delay_calculator_, visitor);            

            if (never_updated_incr_) {
                //Only need to pre-traverse the first update
                incr_walker_.do_required_pre_traversal(timing_graph_, timing_constraints_, visitor);            
            }

            incr_walker_.do_required_traversal(timing_graph_, timing_constraints_, delay_calculator_, visitor);            

            incr_walker_.do_update_slack(timing_graph_, delay_calculator_, visitor);
        }

        /* Clear incremental timing info */
        void clear_timing_incr_(){
            incr_walker_.clear_invalidated_edges();

            n_modified_edges_ = 0;
            never_updated_incr_ = false;
        }

        const TimingGraph& timing_graph_;
        const TimingConstraints& timing_constraints_;
        const DelayCalculator& delay_calculator_;
        SetupHoldAnalysis setup_hold_visitor_;

        FullWalker full_walker_;
        IncrWalker incr_walker_;
        enum class Mode { FULL, INCR };
        Mode mode_;

        bool never_updated_incr_ = true;
        size_t max_modified_edges_;
        std::atomic_size_t n_modified_edges_ = 0;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

}} //namepsace
