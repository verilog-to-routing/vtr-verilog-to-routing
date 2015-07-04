#pragma once
#include "assert.hpp"
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "TimingTags.hpp"
#include "Traversal.hpp"

template<class Base = Traversal>
class SetupTraversal : public Base {
    public:
        //External tag access
        const TimingTags& setup_data_tags(NodeId node_id) const { return setup_data_tags_[node_id]; }
        const TimingTags& setup_clock_tags(NodeId node_id) const { return setup_clock_tags_[node_id]; }
    protected:
        //Internal operations for performing setup analysis
        void initialize_traversal(const TimingGraph& tg);

        template<class TagPoolType>
        void pre_traverse_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class TagPoolType, class DelayCalc>
        void forward_traverse_edge(TagPoolType& tag_pool, const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        template<class TagPoolType>
        void forward_traverse_finalize_node(TagPoolType& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void backward_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id);

        //Tag data structure
        std::vector<TimingTags> setup_data_tags_;
        std::vector<TimingTags> setup_clock_tags_;
};

//Implementation
#include "SetupTraversal.tpp"
