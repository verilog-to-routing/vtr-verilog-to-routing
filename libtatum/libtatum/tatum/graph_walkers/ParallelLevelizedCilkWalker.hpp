#pragma once
#include "tatum/base/cilk_safe.hpp"
#include "tatum/graph_walkers/TimingGraphWalker.hpp"
#include "tatum/graph_visitors/GraphVisitor.hpp"
#include "tatum/TimingGraph.hpp"

#ifdef __cilk
# include <cilk/reducer_opadd.h>
#endif


namespace tatum {

/**
 * A parallel timing analyzer which traveres the timing graph in a levelized
 * manner.  However nodes within each level are processed in parallel using
 * Cilk Plus. If Cilk Plus is not available it operates serially and is 
 * equivalent to the SerialWalker
 */
class ParallelLevelizedCilkWalker : public TimingGraphWalker {
    public:
        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) override {
#ifdef __cilk
            //Use reducer for thread-safe sum
            cilk::reducer<cilk::op_add<size_t>> unconstrained_reducer(0);
#endif

            LevelId first_level = *tg.levels().begin();
            auto nodes = tg.level_nodes(first_level);
            cilk_for(auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
                bool constrained = visitor.do_arrival_pre_traverse_node(tg, tc, *iter);

                if(!constrained) {
#ifdef __cilk
                    *unconstrained_reducer += 1;
#else
                    num_unconstrained_startpoints_ += 1;
#endif
                }
            }

#ifdef __cilk
            num_unconstrained_startpoints_ = unconstrained_reducer.get_value();
#endif
        }

        void do_required_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) override {

#ifdef __cilk
            //Use reducer for thread-safe sum
            cilk::reducer<cilk::op_add<size_t>> unconstrained_reducer(0);
#endif

            const auto& po = tg.logical_outputs();
            cilk_for(auto iter = po.begin(); iter != po.end(); ++iter) {
                bool constrained = visitor.do_required_pre_traverse_node(tg, tc, *iter);

                if(!constrained) {
#ifdef __cilk
                    *unconstrained_reducer += 1;
#else
                    num_unconstrained_endpoints_ += 1;
#endif
                }
            }

#ifdef __cilk
            num_unconstrained_endpoints_ = unconstrained_reducer.get_value();
#endif
        }

        void do_arrival_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) override {
            for(LevelId level_id : tg.levels()) {
                auto level_nodes = tg.level_nodes(level_id);
                cilk_for(auto iter = level_nodes.begin(); iter != level_nodes.end(); ++iter) {
                    visitor.do_arrival_traverse_node(tg, tc, dc, *iter);
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) override {
            for(LevelId level_id : tg.reversed_levels()) {
                auto level_nodes = tg.level_nodes(level_id);
                cilk_for(auto iter = level_nodes.begin(); iter != level_nodes.end(); ++iter) {
                    visitor.do_required_traverse_node(tg, tc, dc, *iter);
                }
            }
        }

        void do_update_slack_impl(const TimingGraph& tg, const DelayCalculator& dc, GraphVisitor& visitor) override {
            auto nodes = tg.nodes();
            cilk_for(auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
                visitor.do_slack_traverse_node(tg, dc, *iter);
            }
        }

        void do_reset_impl(const TimingGraph& tg, GraphVisitor& visitor) override {
            auto nodes = tg.nodes();
            cilk_for(auto node_iter = nodes.begin(); node_iter != nodes.end(); ++node_iter) {
                visitor.do_reset_node(*node_iter);
            }

            auto edges = tg.edges();
            cilk_for(auto edge_iter = edges.begin(); edge_iter != edges.end(); ++edge_iter) {
                visitor.do_reset_edge(*edge_iter);
            }
        }

        size_t num_unconstrained_startpoints_impl() const override { return num_unconstrained_startpoints_; }
        size_t num_unconstrained_endpoints_impl() const override { return num_unconstrained_endpoints_; }
    private:
        size_t num_unconstrained_startpoints_ = 0;
        size_t num_unconstrained_endpoints_ = 0;
};

} //namepsace
