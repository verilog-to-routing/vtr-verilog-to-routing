#pragma once
#include "cilk_safe.hpp"
#include "TimingGraphWalker.hpp"
#include "TimingGraph.hpp"


namespace tatum {

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

} //namepsace
