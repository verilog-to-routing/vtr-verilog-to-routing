#pragma once
#include "TimingGraphWalker.hpp"
#include "TimingGraph.hpp"

namespace tatum {

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

        void do_arrival_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_arrival_traverse_node(tg, tc, dc, node_id);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_required_traverse_node(tg, tc, dc, node_id);
                }
            }
        }
};

} //namepsace
