#pragma once
#include "SetupAnalysis.hpp"
#include "HoldAnalysis.hpp"

namespace tatum {

/** \class SetupHoldAnalysis
 *
 * The SetupHoldAnalysis class defines the operations needed by a timing analyzer
 * to perform a combinded setup (max/long path) and hold (min/shortest path) analysis.
 *
 * Performing both analysis simultaneously tends to be more efficient than performing
 * them sperately due to cache locality.
 *
 * \see SetupAnalysis
 * \see HoldAnalysis
 * \see TimingAnalyzer
 */
class SetupHoldAnalysis {
    public:
        SetupHoldAnalysis(size_t num_tags)
            : setup_visitor_(num_tags)
            , hold_visitor_(num_tags) {}

        void do_reset_node(const NodeId node_id) { 
            setup_visitor_.do_reset_node(node_id); 
            hold_visitor_.do_reset_node(node_id); 
        }

        void do_arrival_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) { 
            setup_visitor_.do_arrival_pre_traverse_node(tg, tc, node_id); 
            hold_visitor_.do_arrival_pre_traverse_node(tg, tc, node_id); 
        }

        void do_required_pre_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const NodeId node_id) { 
            setup_visitor_.do_required_pre_traverse_node(tg, tc, node_id); 
            hold_visitor_.do_required_pre_traverse_node(tg, tc, node_id); 
        }

        template<class DelayCalc>
        void do_arrival_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id) { 
            setup_visitor_.do_arrival_traverse_node(tg, tc, dc, node_id); 
            hold_visitor_.do_arrival_traverse_node(tg, tc, dc, node_id); 
        }

        template<class DelayCalc>
        void do_required_traverse_node(const TimingGraph& tg, const TimingConstraints& tc, const DelayCalc& dc, const NodeId node_id) { 
            setup_visitor_.do_required_traverse_node(tg, tc, dc, node_id); 
            hold_visitor_.do_required_traverse_node(tg, tc, dc, node_id); 
        }

        TimingTags::tag_range get_setup_data_tags(const NodeId node_id) const { return setup_visitor_.get_setup_data_tags(node_id); }
        TimingTags::tag_range get_setup_launch_clock_tags(const NodeId node_id) const { return setup_visitor_.get_setup_launch_clock_tags(node_id); }
        TimingTags::tag_range get_setup_capture_clock_tags(const NodeId node_id) const { return setup_visitor_.get_setup_capture_clock_tags(node_id); }

        TimingTags::tag_range get_hold_data_tags(const NodeId node_id) const { return hold_visitor_.get_hold_data_tags(node_id); }
        TimingTags::tag_range get_hold_launch_clock_tags(const NodeId node_id) const { return hold_visitor_.get_hold_launch_clock_tags(node_id); }
        TimingTags::tag_range get_hold_capture_clock_tags(const NodeId node_id) const { return hold_visitor_.get_hold_capture_clock_tags(node_id); }


    private:
        SetupAnalysis setup_visitor_;
        HoldAnalysis hold_visitor_;
};

} //namepsace
