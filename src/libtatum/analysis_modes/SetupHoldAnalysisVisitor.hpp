#pragma once
#include "SetupAnalysisVisitor.hpp"
#include "HoldAnalysisVisitor.hpp"

class SetupHoldAnalysisVisitor {
    public:
        SetupHoldAnalysisVisitor(size_t num_tags)
            : setup_visitor_(num_tags)
            , hold_visitor_(num_tags) {}

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) { 
            setup_visitor_.do_arrival_pre_traverse_node(tg, tc, node_id); 
            hold_visitor_.do_arrival_pre_traverse_node(tg, tc, node_id); 
        }

        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) { 
            setup_visitor_.do_required_pre_traverse_node(tg, tc, node_id); 
            hold_visitor_.do_required_pre_traverse_node(tg, tc, node_id); 
        }

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) { 
            setup_visitor_.do_arrival_traverse_node(tg, dc, node_id); 
            hold_visitor_.do_arrival_traverse_node(tg, dc, node_id); 
        }

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const DelayCalc& dc, const NodeId node_id) { 
            setup_visitor_.do_required_traverse_node(tg, dc, node_id); 
            hold_visitor_.do_required_traverse_node(tg, dc, node_id); 
        }

        const TimingTags& get_setup_data_tags(const NodeId node_id) const { 
            return setup_visitor_.get_setup_data_tags(node_id); 
        }

        const TimingTags& get_setup_clock_tags(const NodeId node_id) const { 
            return setup_visitor_.get_setup_clock_tags(node_id); 
        }

        const TimingTags& get_hold_data_tags(const NodeId node_id) const { 
            return hold_visitor_.get_hold_data_tags(node_id); 
        }

        const TimingTags& get_hold_clock_tags(const NodeId node_id) const { 
            return hold_visitor_.get_hold_clock_tags(node_id); 
        }

        void reset() { 
            setup_visitor_.reset(); 
            hold_visitor_.reset(); 
        }

    private:
        SetupAnalysisVisitor setup_visitor_;
        HoldAnalysisVisitor hold_visitor_;
};
