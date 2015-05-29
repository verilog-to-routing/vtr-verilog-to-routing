#pragma once

#include "timing_graph_fwd.hpp"
#include "timing_constraints_fwd.hpp"
#include "memory_pool.hpp"

class Traversal {
    protected:
        void initialize_traversal(const TimingGraph& tg) {}

        void pre_traverse_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {}

        template<class DelayCalc>
        void forward_traverse_edge(MemoryPool& tag_pool, const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {}

        void forward_traverse_finalize_node(MemoryPool& tag_pool, const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) {}

        template<class DelayCalc>
        void backward_traverse_edge(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id, const EdgeId edge_id) {}
};

