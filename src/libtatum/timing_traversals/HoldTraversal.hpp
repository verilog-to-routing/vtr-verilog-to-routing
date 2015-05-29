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
        //Internal operations for performing hold analysis
        void initialize_traversal(const TimingGraph& tg);

        void pre_traverse_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalcType>
        void forward_traverse_edge(MemoryPool& tag_pool, const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id);

        void forward_traverse_finalize_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalcType>
        void backward_traverse_edge(const TimingGraph& tg, const DelayCalcType& dc, const NodeId node_id, const EdgeId edge_id);

        //Tag data structure
        std::vector<TimingTags> hold_data_tags_;
        std::vector<TimingTags> hold_clock_tags_;
};

//Implementation
#include "HoldTraversal.tpp"
