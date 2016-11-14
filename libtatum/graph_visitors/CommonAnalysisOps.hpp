#pragma once
#include "TimingTags.hpp"
#include "timing_graph_fwd.hpp"
#include "tatum_linear_map.hpp"

namespace tatum { namespace detail {

/** \class CommonAnalysisOps
 *
 * The operations for CommonAnalysisVisitor to perform setup analysis.
 * The setup analysis operations define that maximum edge delays are used, and that the 
 * maixmum arrival time (and minimum required times) are propagated through the timing graph.
 *
 * \see HoldAnalysisOps
 * \see CommonAnalysisVisitor
 */
class CommonAnalysisOps {
    public:
        CommonAnalysisOps(size_t num_tags)
            : data_tags_(num_tags)
            , clock_launch_tags_(num_tags)
            , clock_capture_tags_(num_tags) {}

        TimingTags& get_data_tags(const NodeId node_id) { return data_tags_[node_id]; }
        TimingTags& get_launch_clock_tags(const NodeId node_id) { return clock_launch_tags_[node_id]; }
        TimingTags& get_capture_clock_tags(const NodeId node_id) { return clock_capture_tags_[node_id]; }
        const TimingTags& get_data_tags(const NodeId node_id) const { return data_tags_[node_id]; }
        const TimingTags& get_launch_clock_tags(const NodeId node_id) const { return clock_launch_tags_[node_id]; }
        const TimingTags& get_capture_clock_tags(const NodeId node_id) const { return clock_capture_tags_[node_id]; }

        void reset() { 
            data_tags_.clear(); 
            clock_launch_tags_.clear(); 
            clock_capture_tags_.clear(); 
        }

    private:
        tatum::util::linear_map<NodeId,TimingTags> data_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_launch_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_capture_tags_;
};

}} //namespace

