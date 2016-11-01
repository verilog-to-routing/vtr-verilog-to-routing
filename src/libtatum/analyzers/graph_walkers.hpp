#pragma once
#include <chrono>
#include "cilk_safe.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"

namespace tatum {

/**
 * \file
 * Defines the TimingGraphWalkers used to encapsulate the process of traversing the timing graph.
 */

/**
 * The abstract base class for all TimingGraphWalkers.
 *
 * TimingGraphWalker encapsulates the process of traversing the timing graph, exposing
 * only the do_*_traversal() methods, which can be called by TimingAnalyzers
 *
 * Internally the do_*_traversal() methods measure record performance related information 
 * and delegate to concrete sub-classes via the do_*_traversal_impl() virtual methods.
 *
 * \tparam Visitor The GraphVisitor to apply during traversals
 * \tparam DelayCalc The DelayCalculator used to calculate edge delays
 *
 * \see GraphVisitor
 * \see DelayCalculator
 * \see TimingAnalyzer
 */
template<class Visitor, class DelayCalc>
class TimingGraphWalker {
    public:
        ///Performs the arrival time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        void do_arrival_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) {
            auto start_time = Clock::now();

            do_arrival_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["arrival_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the required time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        void do_required_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) {
            auto start_time = Clock::now();

            do_required_pre_traversal_impl(tg, tc, visitor);

            profiling_data_["required_pre_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the arrival time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        void do_arrival_traversal(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) {
            auto start_time = Clock::now();

            do_arrival_traversal_impl(tg, dc, visitor);

            profiling_data_["arrival_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        void do_required_traversal(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) {
            auto start_time = Clock::now();

            do_required_traversal_impl(tg, dc, visitor);

            profiling_data_["required_traversal_sec"] = std::chrono::duration_cast<dsec>(Clock::now() - start_time).count();
        }

        ///Retrieve profiling information
        ///\param key The profiling key
        ///\returns The profiling value for the given key, or NaN if the key is not found
        double get_profiling_data(std::string key) { 
            if(profiling_data_.count(key)) {
                return profiling_data_[key];
            } else {
                return std::numeric_limits<double>::quiet_NaN();
            }
        }

    protected:
        ///Sub-class defined arrival time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        virtual void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) = 0;

        ///Sub-class defined required time pre-traversal
        ///\param tg The timing graph
        ///\param tc The timing constraints
        ///\param visitor The visitor to apply during the traversal
        virtual void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) = 0;

        ///Sub-class defined arrival time traversal
        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        virtual void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) = 0;

        ///Sub-class defined required time traversal
        ///Performs the required time traversal
        ///\param tg The timing graph
        ///\param dc The edge delay calculator
        ///\param visitor The visitor to apply during the traversal
        virtual void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) = 0;

    private:
        std::map<std::string, double> profiling_data_;

        typedef std::chrono::duration<double> dsec;
        typedef std::chrono::high_resolution_clock Clock;
};

/**
 * A simple serial graph walker which traverses the timing graph in a levelized
 * manner.
 *
 * \tparam Visitor The visitor to apply during traversals
 * \tparam DelayCalc The delay calculator to use
 */
template<class Visitor, class DelayCalc>
class SerialWalker : public TimingGraphWalker<Visitor, DelayCalc> {
    protected:
        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) override {
            for(NodeId node_id : tg.primary_inputs()) {
                visitor.do_arrival_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) override {
            for(NodeId node_id : tg.primary_outputs()) {
                visitor.do_required_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_arrival_traverse_node(tg, dc, node_id);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
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
 * \tparam Visitor The visitor to apply during traversals
 * \tparam DelayCalc The delay calculator to use
 */
template<class Visitor, class DelayCalc>
class ParallelLevelizedCilkWalker : public TimingGraphWalker<Visitor, DelayCalc> {
    public:
        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) override {
            const auto& pi = tg.primary_inputs();
            cilk_for(auto iter = pi.begin(); iter != pi.end(); ++iter) {
                visitor.do_arrival_pre_traverse_node(tg, tc, *iter);
            }
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) override {
            const auto& po = tg.primary_outputs();
            cilk_for(auto iter = po.begin(); iter != po.end(); ++iter) {
                visitor.do_required_pre_traverse_node(tg, tc, *iter);
            }
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
                auto level_nodes = tg.level_nodes(level_id);
                cilk_for(auto iter = level_nodes.begin(); iter != level_nodes.end(); ++iter) {
                    visitor.do_arrival_traverse_node(tg, dc, *iter);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
                auto level_nodes = tg.level_nodes(level_id);
                cilk_for(auto iter = level_nodes.begin(); iter != level_nodes.end(); ++iter) {
                    visitor.do_required_traverse_node(tg, dc, *iter);
                }
            }
        }
};

///The default parallel graph walker
template<class Visitor, class DelayCalc>
using ParallelWalker = ParallelLevelizedCilkWalker<Visitor, DelayCalc>;

} //namepsace
