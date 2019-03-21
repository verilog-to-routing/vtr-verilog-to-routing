#pragma once
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

namespace tatum {

class GraphVisitor {
    public:
        virtual ~GraphVisitor() {}
        virtual void do_reset_node(const NodeId node_id) = 0;
        virtual void do_reset_edge(const EdgeId edge_id) = 0;

        virtual bool do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) = 0;
        virtual bool do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) = 0;

        virtual void do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id) = 0;
        virtual void do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalculator& dc, const NodeId node_id) = 0;

        virtual void do_slack_traverse_node(const TimingGraph& tg, const DelayCalculator& dc, const NodeId node) = 0;
};

}
