#pragma once
#include <chrono>
#include "cilk_safe.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

/**
 * \file
 * We use TimingGraphWalker's to encapsulate the process of traversing the timing graph.
 */

/**
 * A basic no-op implementation of the TimingGraphWalker concept.
 *
 * All walkers should re-define these various operations.
 *
 * Note that we do not use virtual inheritance, as this class is intended to
 * be specialized (i.e. used as a template parameter).
 *
 * \tparam V The visitor to apply
 */
template<class V, class DelayCalc>
class TimingGraphWalker {
    public:
        void do_arrival_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            auto start_time = Clock::now();

            do_arrival_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["arrival_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        void do_required_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            auto start_time = Clock::now();

            do_required_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["required_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        void do_arrival_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {
            auto start_time = Clock::now();

            do_arrival_traversal_impl(tg, dc, visitor);

            profiling_data_["arrival_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        void do_required_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {
            auto start_time = Clock::now();

            do_required_traversal_impl(tg, dc, visitor);

            profiling_data_["required_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        double get_profiling_data(std::string key) { return (profiling_data_.count(key)) ? profiling_data_[key] : std::numeric_limits<double>::quiet_NaN(); }

    protected:
        virtual void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) = 0;
        virtual void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) = 0;

        virtual void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) = 0;

        virtual void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) = 0;

    private:
        std::map<std::string, double> profiling_data_;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;

};

/**
 * A simple serial graph walker which traverses the timing graph in a levelized
 * manner.
 */
template<class V, class DelayCalc>
class SerialWalker : public TimingGraphWalker<V, DelayCalc> {
    protected:
        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) override {
            for(NodeId node_id : tg.primary_inputs()) {
                visitor.do_arrival_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) override {
            for(NodeId node_id : tg.primary_outputs()) {
                visitor.do_required_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) override {
            for(LevelId level_id = 1; level_id < tg.num_levels(); level_id++) {
                for(NodeId node_id : tg.level(level_id)) {
                    visitor.do_arrival_traverse_node(tg, dc, node_id);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) override {
            for(LevelId level_id = tg.num_levels() - 2; level_id >= 0; level_id--) {
                for(NodeId node_id : tg.level(level_id)) {
                    visitor.do_required_traverse_node(tg, dc, node_id);
                }
            }
        }
};

/**
 * A parallel timing analyzer which traveres the timing graph in a levelized
 * manner.  However nodes within each level are processed in parallel using
 * Cilk Plus. If Cilk Plus is not available it operates serially and is 
 * equivalent to the SerialWalker
 *
 * \tparam V The visitor used is responsible for avoiding race conditions.
 */
template<class V, class DelayCalc>
class ParallelLevelizedCilkWalker : public TimingGraphWalker<V, DelayCalc> {
    public:
        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) override {
            const auto& pi = tg.primary_inputs();
            cilk_for(auto iter = pi.begin(); iter != pi.end(); ++iter) {
                visitor.do_arrival_pre_traverse_node(tg, tc, *iter);
            }
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) override {
            const auto& po = tg.primary_outputs();
            cilk_for(auto iter = po.begin(); iter != po.end(); ++iter) {
                visitor.do_required_pre_traverse_node(tg, tc, *iter);
            }
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) override {
            for(LevelId level_id = 1; level_id < tg.num_levels(); level_id++) {
                const auto& level = tg.level(level_id);
                cilk_for(auto iter = level.begin(); iter != level.end(); ++iter) {
                    visitor.do_arrival_traverse_node(tg, dc, *iter);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, V& visitor) override {
            for(LevelId level_id = tg.num_levels() - 2; level_id >= 0; level_id--) {
                const auto& level = tg.level(level_id);
                cilk_for(auto iter = level.begin(); iter != level.end(); ++iter) {
                    visitor.do_required_traverse_node(tg, dc, *iter);
                }
            }
        }
};
