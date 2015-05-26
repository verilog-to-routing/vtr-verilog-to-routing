#pragma once
#include "assert.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "Traversal.hpp"

template<class Base = Traversal>
class HoldTraversal : public Base {
    public:
        //External tag access
        const TimingTags& hold_data_tags(NodeId node_id) const { return hold_data_tags_[node_id]; }
        const TimingTags& hold_clock_tags(NodeId node_id) const { return hold_clock_tags_[node_id]; }
    protected:
        //Internal operations for performing setup analysis
        void initialize_traversal(const TimingGraph& tg);
        void pre_traverse_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void forward_traverse_edge(MemoryPool& tag_pool, const TimingGraph& tg, const NodeId node_id, const EdgeId edge_id);
        void forward_traverse_finalize_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void backward_traverse_edge(const TimingGraph& tg, const NodeId node_id, const EdgeId edge_id);

        //Tag data structure
        std::vector<TimingTags> hold_data_tags_;
        std::vector<TimingTags> hold_clock_tags_;
};

template<class Base>
void HoldTraversal<Base>::initialize_traversal(const TimingGraph& tg) {
    Base::initialize_traversal(tg);

    hold_data_tags_ = std::vector<TimingTags>(tg.num_nodes());
    hold_clock_tags_ = std::vector<TimingTags>(tg.num_nodes());
}

template<class Base>
void HoldTraversal<Base>::pre_traverse_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    Base::pre_traverse_node(tag_pool, tg, tc, node_id);
}

template<class Base>
void HoldTraversal<Base>::forward_traverse_edge(MemoryPool& tag_pool, const TimingGraph& tg, const NodeId node_id, const EdgeId edge_id) {
    Base::forward_traverse_edge(tag_pool, tg, node_id, edge_id);
}

template<class Base>
void HoldTraversal<Base>::forward_traverse_finalize_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {
    Base::forward_traverse_finalize_node(tag_pool, tg, tc, node_id);
}

template<class Base>
void HoldTraversal<Base>::backward_traverse_edge(const TimingGraph& tg, const NodeId node_id, const EdgeId edge_id) {
    Base::backward_traverse_edge(tg, node_id, edge_id);
}
