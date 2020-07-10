#pragma once
#include <algorithm>
#include "tatum/graph_walkers/TimingGraphWalker.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"
#include "tatum/graph_visitors/GraphVisitor.hpp"

namespace tatum {

/**
 * Controls how timing tags are invalidated during the incremental traversal.
 *
 * If TATUM_INCR_BLOCK_INVALIDATION is defined: 
 *      All of a nodes tags associated with an invalidated edge are invalidated.
 *      This is a robust but pessimisitc approach (it invalidates more tags than
 *      strictly required). As a result all nodes processed will report having been
 *      modified, meaning their decendents/predecessors will also be invalidated
 *      even if in reality the recalculated tags are identical to the previous ones
 *      (i.e. nothing has really changed).
 *
 * Ohterwise, the analyzer performs edge invalidation:
 *      Only node tags which are dominanted by an invalidated edge are invalidated.
 *      This is a less pessimistic approach, and means when processed nodes which
 *      don't have any changed tags will report as being unmodified. This significantly
 *      prunes the amount of the timing graph which needs to be updated (as unmodified
 *      nodes don't need to invalidate their decendents/predecessors.
 */
//#define TATUM_INCR_BLOCK_INVALIDATION

/**
 * A serial graph walker which traverses the timing graph in a levelized
 * manner. Unlike SerialWalker it attempts to incrementally (rather than
 * fully) update based on invalidated edges.
 *
 * To performan an incremental traversal, the st of invalidated edges
 * is processed to identify nodes which will need to be re-evaluated for
 * the arrival and/or required traversals.
 *
 * These nodes are then stored in per-level queues for the arrival/required
 * traversals in incr_arr_update_/incr_req_update_, which also stores the
 * level range which needs to be considered.
 *
 * These queues are processed by do_arrival_traversal_impl()/
 * do_required_traversal_impl() to update the analysis result. Importantly,
 * incr_arr_update_/incr_req_update_ may be modified while 
 * do_arrival_traversal_impl()/do_required_traversal_impl() are running, since
 * as nodes are modified their decendents/predessors may need to be updated
 * as well.
 *
 * Note that this graph walker assumes that timing constraints aren't changed
 * and so do_arrival_pre_traversal_impl() / do_required_pre_traversal_impl()
 * should only be called once (on the first analysis)
 */
class SerialIncrWalker : public TimingGraphWalker {
    protected:
        void invalidate_edge_impl(const EdgeId edge) override {
            if (is_invalidated(edge)) return;

            invalidated_edges_.push_back(edge);

            mark_invalidated(edge);
        }

        void clear_invalidated_edges_impl() override {
            invalidated_edges_.clear();
            edge_invalidated_.clear();
        }

        node_range modified_nodes_impl() const override {
            return tatum::util::make_range(nodes_modified_.cbegin(), nodes_modified_.cend());
        }

        void do_arrival_pre_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, GraphVisitor& visitor) override {
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
            prepare_incr_update(tg);

            for(int level_idx = incr_arr_update_.min_level; level_idx <= incr_arr_update_.max_level; ++level_idx) {
                LevelId level(level_idx);

                auto& level_nodes = incr_arr_update_.nodes_to_process[level_idx];

                //Sorting the level nodes tends to help memory locality, since the
                //timing graph is laid out in traversal order
                sort(level_nodes);

                for (NodeId node : level_nodes) {

                    invalidate_node_for_arrival_traversal(node, tg, visitor);

                    bool node_updated = visitor.do_arrival_traverse_node(tg, tc, dc, node);

                    if (node_updated) {
                        //Record that this node was updated, for later efficient slack update
                        enqueue_modified_node(node);

                        //Queue this node's downstream dependencies for updating
                        for (EdgeId edge : tg.node_out_edges(node)) {
                            NodeId snk_node = tg.edge_sink_node(edge);
                            enqueue_arr_node(tg, snk_node, edge);
                        }
                    }
                }
            }
        }

        void do_required_traversal_impl(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, GraphVisitor& visitor) override {

            for(int level_idx = incr_req_update_.max_level; level_idx >= incr_req_update_.min_level; --level_idx) {
                LevelId level(level_idx);

                auto& level_nodes = incr_req_update_.nodes_to_process[level_idx];

                //Sorting the level nodes tends to help memory locality, since the
                //timing graph is laid out in traversal order
                sort(level_nodes);

                for (NodeId node : level_nodes) {
                    invalidate_node_for_required_traversal(node, tg, visitor);
                    bool node_updated = visitor.do_required_traverse_node(tg, tc, dc, node);

                    if (node_updated) {
                        //Record that this node was updated, for later efficient slack update
                        enqueue_modified_node(node);

                        //Queue this node's upstream dependencies for updating
                        for (EdgeId edge : tg.node_in_edges(node)) {
                            NodeId src_node = tg.edge_src_node(edge);

                            enqueue_req_node(tg, src_node, edge);
                        }
                    }
                }
            }
        }

        void do_update_slack_impl(const TimingGraph& tg, const DelayCalculator& dc, GraphVisitor& visitor) override {
            sort(nodes_modified_);

            //std::cout << "Processing slack updates for " << nodes_modified_.size() << " nodes\n";

            for(NodeId node : nodes_modified_) {
                //std::cout << "  Processing slack " << node << "\n";

#ifdef TATUM_CALCULATE_EDGE_SLACKS
                for (EdgeId edge : tg.node_in_edges(node)) {
                    visitor.do_reset_edge(edge);
                }
#endif
                visitor.do_reset_node_slack_tags(node);

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

        bool is_invalidated(EdgeId edge) const {
            if (edge_invalidated_.size() > size_t(edge)) {
                return edge_invalidated_[edge];
            }
            return false; //Not yet marked, so not invalid
        }

        bool not_invalidated(EdgeId edge) const {
            return !is_invalidated(edge);
        }

        void mark_invalidated(EdgeId edge) {
            if (edge_invalidated_.size() <= size_t(edge)) {
                edge_invalidated_.resize(size_t(edge)+1, false);
            }
            edge_invalidated_[edge] = true;
        }

        bool is_modified(NodeId node) const {
            if (node_is_modified_.size() > size_t(node)) {
                return node_is_modified_[node];
            }
            return false; //Not yet marked, so not queued
        }

        void mark_modified(NodeId node) {
            if (node_is_modified_.size() <= size_t(node)) {
                node_is_modified_.resize(size_t(node)+1, false);
            }
            node_is_modified_[node] = true;
        }

        void clear_modified() {
            nodes_modified_.clear();
            node_is_modified_.clear();
        }

        void sort(std::vector<NodeId>& nodes) {
            std::sort(nodes.begin(), nodes.end());
        }

        void prepare_incr_update(const TimingGraph& tg) {
            //Reset incremental traversal tracking data
            clear_modified();
            resize_incr_update_levels(tg);
            incr_arr_update_.clear(tg);
            incr_req_update_.clear(tg);

            incr_arr_update_.min_level = size_t(*(tg.levels().end() - 1));
            incr_arr_update_.max_level = size_t(*tg.levels().begin());
            incr_req_update_.min_level = size_t(*(tg.levels().end() - 1));
            incr_req_update_.max_level = size_t(*tg.levels().begin());

            //Process the externally invalidated edges to prepare for the incremental traversal
            for (EdgeId edge : invalidated_edges_) {
                NodeId snk_node = tg.edge_sink_node(edge);
                enqueue_arr_node(tg, snk_node, edge);

                NodeId src_node = tg.edge_src_node(edge);
                enqueue_req_node(tg, src_node, edge);
            }
        }

        //Enqueues a node for arrival time processing which was invalidated by invalidated_edge
        void enqueue_arr_node(const TimingGraph& tg, NodeId node, EdgeId invalidated_edge) {
            invalidate_edge_impl(invalidated_edge);
            incr_arr_update_.enqueue_node(tg, node);
        }

        //Enqueues a node for required time processing which was invalidated by invalidated_edge
        void enqueue_req_node(const TimingGraph& tg, NodeId node, EdgeId invalidated_edge) {
            invalidate_edge_impl(invalidated_edge);
            incr_req_update_.enqueue_node(tg, node);
        }

        //Record the specified node as having been modified
        void enqueue_modified_node(const NodeId node) {
            if (is_modified(node)) return;

            mark_modified(node);

            nodes_modified_.push_back(node); 
        }

        void invalidate_node_for_arrival_traversal(const NodeId node, const TimingGraph& tg, GraphVisitor& visitor) {

#ifdef TATUM_INCR_BLOCK_INVALIDATION
            //Block invalidation
            //
            //Invalidates the entire node (all the nodes arrival tags)
            //As a result, when processed the node will be re-computed from scratch
            visitor.do_reset_node_arrival_tags(node);
#else
            //Edge invalidation
            for (EdgeId edge : tg.node_in_edges(node)) {
                if (not_invalidated(edge)) continue;
                //Data arrival tags track their associated origin node (i.e. dominant
                //edge which determines the tag value). Rather than invalidate all
                //tags to ensure the correctly updated tag when the node is re-traversed,
                //we can get away with only invalidating the tag when the dominant edge
                //is invalidated.
                //
                //This ensures that cases where non-dominate edges change delay value,
                //but not in ways which effect the tag value, we detect that the tag
                //is 'unchanged', which helps keep the number of updated nodes small.
                NodeId src_node = tg.edge_src_node(edge);
                visitor.do_reset_node_arrival_tags_from_origin(node, /*origin=*/src_node);

                //At SOURCE/SINK nodes clock launch/capture tags are converted into
                //data arrival/required tags, so we also need to carefully reset those
                //tags as well (since they don't track origin nodes this must be done
                //seperately).
                EdgeType edge_type = tg.edge_type(edge);
                if (edge_type == EdgeType::PRIMITIVE_CLOCK_CAPTURE) {
                    //We mark required times on sinks based the clock capture time during
                    //the arrival traversal.
                    //
                    //Therefore we need to invalidate the data required times (so they are
                    //re-calculated correctly) when the clock caputre edge has been 
                    //invalidated. Note that we don't need to enqueue the sink node for 
                    //required time traversal, since the required time will be updated during
                    //the arrival traversasl.
                    visitor.do_reset_node_required_tags(node);

                    //However, since the required time traversal doesn't update the sink node,
                    //it's dependent nodes won't be enqueued for processing, even if the required
                    //times changed, so we explicitly do that here
                    for (EdgeId sink_in_edge : tg.node_in_edges(node)) {
                        NodeId sink_src_node = tg.edge_src_node(sink_in_edge);
                        enqueue_req_node(tg, sink_src_node, sink_in_edge);
                    }
                } else if (edge_type == EdgeType::PRIMITIVE_CLOCK_LAUNCH) {
                    //On propagating to a SOURCE node, CLOCK_LAUNCH becomes DATA_ARRIVAL
                    //we therefore invalidate any outstanding arrival tags if the clock launch
                    //edge has been invalidated.
                    //
                    //This ensures the correct data arrivaltags are generated
                    visitor.do_reset_node_arrival_tags(node);
                }
            }
#endif
        }

        void invalidate_node_for_required_traversal(const NodeId node, const TimingGraph& tg, GraphVisitor& visitor) {
#ifdef TATUM_INCR_BLOCK_INVALIDATION
            //Block invalidation
            //
            //Invalidates the entire node (all the nodes arrival tags)
            //As a result, when processed the node will be re-computed from scratch
            visitor.do_reset_node_required_tags(node);
#else
            //Edge invalidation
            for (EdgeId edge : tg.node_out_edges(node)) {
                if (not_invalidated(edge)) continue;

                NodeId snk_node = tg.edge_sink_node(edge);
                visitor.do_reset_node_required_tags_from_origin(node, /*origin=*/snk_node);
            }
#endif
        }

        void resize_incr_update_levels(const TimingGraph& tg) {
            incr_arr_update_.nodes_to_process.resize(tg.levels().size());
            incr_req_update_.nodes_to_process.resize(tg.levels().size());
        }

        /*
         * Helper struct to record incremental traversal information
         */
        struct t_incr_traversal_update {
            public:

                //The nodes per-level which need to be updated/processed
                std::vector<std::vector<NodeId>> nodes_to_process;

                //The range of levels which need to be updated
                int min_level = 0;
                int max_level = 0;

                void enqueue_node(const TimingGraph& tg, NodeId node) {
                    if (is_enqueued(node)) return;

                    mark_enqueued(node);

                    int level = size_t(tg.node_level(node));

                    nodes_to_process[level].push_back(node);
                    min_level = std::min(min_level, level);
                    max_level = std::max(max_level, level);
                }

                size_t total_nodes_to_process() const {
                    size_t cnt = 0;
                    for (int level = min_level; level <= max_level; ++level) {
                        cnt += nodes_to_process[level].size();
                    }
                    return cnt;
                }

                size_t total_levels_to_process() const {
                    return size_t(max_level) - size_t(min_level) + 1;
                }

                bool is_enqueued(NodeId node) const {
                    if (node_is_enqueued.size() > size_t(node)) {
                        return node_is_enqueued[node];
                    }
                    return false; //Not yet marked, so not queued
                }

                void clear(const TimingGraph& tg) {
                    for (int level = min_level; level <= max_level; ++level) {
                        nodes_to_process[level].clear();
                    }
                    node_is_enqueued.clear();

                    min_level = size_t(*(tg.levels().end() - 1));
                    max_level = size_t(*tg.levels().begin());
                }
            private:
                void mark_enqueued(NodeId node) {
                    if (node_is_enqueued.size() <= size_t(node)) {
                        node_is_enqueued.resize(size_t(node)+1, false);
                    }
                    node_is_enqueued[node] = true;
                }

                //Bitset to record whether a node has already been enqueued
                tatum::util::linear_map<NodeId,bool> node_is_enqueued;

        };

        //State info about the incremental arr/req updates
        t_incr_traversal_update incr_arr_update_;
        t_incr_traversal_update incr_req_update_;

        //Set of invalidated edges, and bitset for membership
        std::vector<EdgeId> invalidated_edges_;
        tatum::util::linear_map<EdgeId,bool> edge_invalidated_;

        //Nodes which have been modified during timing update, and bitset for membership
        std::vector<NodeId> nodes_modified_; 
        tatum::util::linear_map<NodeId,bool> node_is_modified_;

        size_t num_unconstrained_startpoints_ = 0;
        size_t num_unconstrained_endpoints_ = 0;
};

} //namepsace
