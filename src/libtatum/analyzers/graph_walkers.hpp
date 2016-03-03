#pragma once
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
template<class V>
class TimingGraphWalker {
    public:
        void do_arrival_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {}
        void do_required_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {}

        template<class DelayCalc>
        void do_arrival_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {}

        template<class DelayCalc>
        void do_required_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {}
};

/**
 * A simple serial graph walker which traverses the timing graph in a levelized
 * manner.
 */
template<class V>
class SerialWalker {
    public:
        void do_arrival_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            for(NodeId node_id : tg.primary_inputs()) {
                visitor.do_arrival_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_required_pre_traversal(const TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            for(NodeId node_id : tg.primary_outputs()) {
                visitor.do_required_pre_traverse_node(tg, tc, node_id);
            }
        }

        template<class DelayCalc>
        void do_arrival_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {
            for(LevelId level_id = 1; level_id < tg.num_levels(); level_id++) {
                for(NodeId node_id : tg.level(level_id)) {
                    visitor.do_arrival_traverse_node(tg, dc, node_id);
                }
            }
        }

        template<class DelayCalc>
        void do_required_traversal(const TimingGraph& tg, const DelayCalc& dc, V& visitor) {
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
template<class V>
class ParallelLevelizedCilkWalker {
    public:
        void do_arrival_pre_traversal(TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            cilk_for(NodeId node_id : tg.primary_inputs()) {
                visitor.do_arrival_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_required_pre_traversal(TimingGraph& tg, const TimingConstraints& tc, V& visitor) {
            cilk_for(NodeId node_id : tg.primary_outputs()) {
                visitor.do_required_pre_traverse_node(tg, tc, node_id);
            }
        }

        template<class DelayCalc>
        void do_arrival_traversal(TimingGraph& tg, const DelayCalc& dc, V& visitor) {
            for(LevelId level_id = 1; level_id < tg.num_levels(); level_id++) {
                cilk_for(NodeId node_id : tg.level(level_id)) {
                    visitor.do_visit(tg, dc, node_id);
                }
            }
        }

        template<class DelayCalc>
        void do_required_traversal(TimingGraph& tg, const DelayCalc& dc, V& visitor) {
            for(LevelId level_id = tg.num_levels() - 2; level_id >= 0; level_id--) {
                cilk_for(NodeId node_id : tg.level(level_id)) {
                    visitor.do_visit(tg, dc, node_id);
                }
            }
        }
}
