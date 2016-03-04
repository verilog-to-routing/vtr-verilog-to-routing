#pragma once
#include <memory>
#include <vector>

#include "GraphVisitor.hpp"
#include "TimingTags.hpp"

class HoldAnalysisVisitor {
    public:
        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);
        void do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_pre_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id);

        void reset();

        TimingTags& get_hold_data_tags(NodeId node_id) { return hold_data_tags_[node_id]; }
        const TimingTags& get_hold_data_tags(NodeId node_id) const { return hold_data_tags_[node_id]; }

        TimingTags& get_hold_clock_tags(NodeId node_id) { return hold_clock_tags_[node_id]; }
        const TimingTags& get_hold_clock_tags(NodeId node_id) const { return hold_clock_tags_[node_id]; }

    private:
        std::vector<TimingTags> hold_data_tags_;
        std::vector<TimingTags> hold_clock_tags_;
};

