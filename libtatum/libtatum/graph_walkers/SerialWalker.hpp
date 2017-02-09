#pragma once
#include "TimingGraphWalker.hpp"
#include "TimingGraph.hpp"

//#define LOG_TRAVERSAL_LEVELS

#include <iostream>

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
#ifdef LOG_TRAVERSAL_LEVELS
            std::cout << "Arrival Pre-traversal\n";
#endif
            for(NodeId node_id : tg.primary_inputs()) {
                visitor.do_arrival_pre_traverse_node(tg, tc, node_id);
            }
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, Visitor& visitor) override {
#ifdef LOG_TRAVERSAL_LEVELS
            std::cout << "Required Pre-traversal\n";
#endif
            size_t num_unconstrained = 0;
            for(NodeId node_id : tg.logical_outputs()) {
                bool constrained = visitor.do_required_pre_traverse_node(tg, tc, node_id);

                if(!constrained) ++num_unconstrained;
            }

            if(num_unconstrained  0) {
                std::cerr << "Warning: " << num_unconstrained << " timing sinks were not constrained\n";
            }
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
#ifdef LOG_TRAVERSAL_LEVELS
                std::cout << "Arrival " << level_id << "\n";
#endif
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_arrival_traverse_node(tg, tc, dc, node_id);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, Visitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
#ifdef LOG_TRAVERSAL_LEVELS
                std::cout << "Required " << level_id << "\n";
#endif
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_required_traverse_node(tg, tc, dc, node_id);
                }
            }
        }

        void do_update_slack_impl(const TimingGraph& tg, const DelayCalc& dc, Visitor& visitor) override {
            for(NodeId node : tg.nodes()) {
                visitor.do_slack_traverse_node(tg, dc, node);
            }
        }

        void do_reset_impl(const TimingGraph& tg, Visitor& visitor) override {
            for(NodeId node_id : tg.nodes()) {
                visitor.do_reset_node(node_id);
            }
            for(EdgeId edge_id : tg.edges()) {
                visitor.do_reset_edge(edge_id);
            }
        }
};

} //namepsace
