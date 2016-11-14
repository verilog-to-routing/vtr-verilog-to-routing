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
            : data_tags_(num_tags, NUM_DATA_TAGS_RESERVE)
            , clock_launch_tags_(num_tags, NUM_CLOCK_TAGS_RESERVE)
            , clock_capture_tags_(num_tags, NUM_CLOCK_TAGS_RESERVE) {}

        TimingTags& get_data_tags(const NodeId node_id) { return data_tags_[node_id]; }
        TimingTags& get_launch_clock_tags(const NodeId node_id) { return clock_launch_tags_[node_id]; }
        TimingTags& get_capture_clock_tags(const NodeId node_id) { return clock_capture_tags_[node_id]; }
        const TimingTags& get_data_tags(const NodeId node_id) const { return data_tags_[node_id]; }
        const TimingTags& get_launch_clock_tags(const NodeId node_id) const { return clock_launch_tags_[node_id]; }
        const TimingTags& get_capture_clock_tags(const NodeId node_id) const { return clock_capture_tags_[node_id]; }

        void reset() { 
            data_tags_ = tatum::util::linear_map<NodeId,TimingTags>(data_tags_.size(), NUM_DATA_TAGS_RESERVE);
            clock_launch_tags_ = tatum::util::linear_map<NodeId,TimingTags>(clock_launch_tags_.size(), NUM_CLOCK_TAGS_RESERVE);
            clock_capture_tags_ = tatum::util::linear_map<NodeId,TimingTags>(clock_capture_tags_.size(), NUM_CLOCK_TAGS_RESERVE);
        }

    private:
        constexpr static size_t NUM_DATA_TAGS_RESERVE = 1;
        constexpr static size_t NUM_CLOCK_TAGS_RESERVE = 1;
        tatum::util::linear_map<NodeId,TimingTags> data_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_launch_tags_;
        tatum::util::linear_map<NodeId,TimingTags> clock_capture_tags_;
};

}} //namespace

