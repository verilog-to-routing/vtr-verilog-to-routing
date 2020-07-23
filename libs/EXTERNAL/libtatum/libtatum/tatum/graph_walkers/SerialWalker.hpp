#pragma once
#include "tatum/graph_walkers/TimingGraphWalker.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/graph_visitors/GraphVisitor.hpp"

namespace tatum {

/**
 * A simple serial graph walker which traverses the timing graph in a levelized
 * manner.
 */
class SerialWalker : public TimingGraphWalker {
    protected:
        void invalidate_edge_impl(const EdgeId /*edge*/) override {
            //Do nothing, this walker only does full updates
        }

        void clear_invalidated_edges_impl() override {
            //Do nothing, this walker only does full updates
        }

        node_range modified_nodes_impl() const override {
            return tatum::util::make_range(nodes_modified_.cbegin(), nodes_modified_.cend());
        }

        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) override {
            if (nodes_modified_.empty()) {
                //This is a non-incremental updater so all nodes are always updated
                auto nodes = tg.nodes();
                nodes_modified_.reserve(nodes.size());
                for (NodeId node : nodes) {
                    nodes_modified_.push_back(node);
                }
            }

            size_t num_unconstrained = 0;

            LevelId first_level = *tg.levels().begin();
            for(NodeId node_id : tg.level_nodes(first_level)) {
                bool constrained = visitor.do_arrival_pre_traverse_node(tg, tc, node_id);

                if(!constrained) {
                    ++num_unconstrained;
                }
            }

            num_unconstrained_startpoints_ = num_unconstrained;
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) override {
            size_t num_unconstrained = 0;

            for(NodeId node_id : tg.logical_outputs()) {
                bool constrained = visitor.do_required_pre_traverse_node(tg, tc, node_id);

                if(!constrained) {
                    ++num_unconstrained;
                }
            }

            num_unconstrained_endpoints_ = num_unconstrained;
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_arrival_traverse_node(tg, tc, dc, node_id);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
                for(NodeId node_id : tg.level_nodes(level_id)) {
                    visitor.do_required_traverse_node(tg, tc, dc, node_id);
                }
            }
        }

        void do_update_slack_impl(const TimingGraph& tg, const DelayCalculator& dc, GraphVisitor& visitor) override {
            for(NodeId node : tg.nodes()) {
                visitor.do_slack_traverse_node(tg, dc, node);
            }
        }

        void do_reset_impl(const TimingGraph& tg, GraphVisitor& visitor) override {
            for(NodeId node_id : tg.nodes()) {
                visitor.do_reset_node(node_id);
            }
#ifdef TATUM_CALCULATE_EDGE_SLACKS
            for(EdgeId edge_id : tg.edges()) {
                visitor.do_reset_edge(edge_id);
            }
#endif
        }

        size_t num_unconstrained_startpoints_impl() const override { return num_unconstrained_startpoints_; }
        size_t num_unconstrained_endpoints_impl() const override { return num_unconstrained_endpoints_; }
    private:
        size_t num_unconstrained_startpoints_ = 0;
        size_t num_unconstrained_endpoints_ = 0;
        std::vector<NodeId> nodes_modified_;
};

} //namepsace
