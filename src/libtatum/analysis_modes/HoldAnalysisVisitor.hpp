#pragma once
#include <memory>
#include <vector>

#include "GraphVisitor.hpp"
#include "TimingTags.hpp"

class HoldAnalysisVisitor : public GraphVisitor {
    public:
        void do_arrival_pre_traverse_node(TimingGraph& tg, NodeId node_id);
        void do_arrival_traverse_node(TimingGraph& tg, NodeId node_id);

        void do_required_pre_traverse_node(TimingGraph& tg, NodeId node_id);
        void do_required_traverse_node(TimingGraph& tg, NodeId node_id);

        void clear() { hold_tags_.clear(); }

        std::shared_ptr<TimingTags> get_hold_tags(NodeId node_id) { return hold_tags_[node_id]; }

    private:
        std::vector<std::shared_ptr<TimingTags>> hold_tags_;
}

