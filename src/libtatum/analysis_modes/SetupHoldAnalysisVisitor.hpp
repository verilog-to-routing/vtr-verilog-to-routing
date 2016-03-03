#pragma once
#include "SetupAnalysisVisitor.hpp"
#include "HoldAnalysisVisitor.hpp"

class SetupHoldAnalysisVisitor {
    public:
        void arrival_pre_traverse_node(TimingGraph& tg, NodeId node_id) { setup_visitor_.arrival_pre_traverse_node(tg, node_id); hold_visitor.arrival_pre_traverse_node(tg, node_id); }
        void arrival_traverse_node(TimingGraph& tg, NodeId node_id) { setup_visitor_.arrival_traverse_node(tg, node_id); hold_visitor.arrival_traverse_node(tg, node_id); }

        void required_pre_traverse_node(TimingGraph& tg, NodeId node_id) { setup_visitor_.required_pre_traverse_node(tg, node_id); hold_visitor.required_pre_traverse_node(tg, node_id); }
        void required_traverse_node(TimingGraph& tg, NodeId node_id) { setup_visitor_.required_traverse_node(tg, node_id); hold_visitor.required_traverse_node(tg, node_id); }

        std::shared_ptr<TimingTags> get_setup_tags(NodeId node_id) { return setup_visitor_.get_setup_tags(node_id); }
        std::shared_ptr<TimingTags> get_hold_tags(NodeId node_id) { return hold_visitor_.get_hold_tags(node_id); }

        void clear() { setup_visitor_.clear(); hold_visitor_.clear(); }

    private:
        HoldAnalysisVisitor hold_visitor_;
        SetupAnalysisVisitor setup_visitor_;
}
