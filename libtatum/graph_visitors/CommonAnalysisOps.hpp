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
            : node_tags_(num_tags) {}

        CommonAnalysisOps(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps(CommonAnalysisOps&&) = delete;
        CommonAnalysisOps& operator=(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps& operator=(CommonAnalysisOps&&) = delete;

        TimingTags::tag_range get_tags(const NodeId node_id) { 
            return node_tags_[node_id].tags(); 
        }
        TimingTags::tag_range get_data_tags(const NodeId node_id) { 
            return node_tags_[node_id].tags(TagType::DATA); 
        }
        TimingTags::tag_range get_launch_clock_tags(const NodeId node_id) { 
            return node_tags_[node_id].tags(TagType::CLOCK_LAUNCH); 
        }
        TimingTags::tag_range get_capture_clock_tags(const NodeId node_id) { 
            return node_tags_[node_id].tags(TagType::CLOCK_CAPTURE); 
        }

        TimingTags::tag_range get_tags(const NodeId node_id) const { 
            return node_tags_[node_id].tags(); 
        }
        TimingTags::tag_range get_data_tags(const NodeId node_id) const {
            return node_tags_[node_id].tags(TagType::DATA); 
        }
        TimingTags::tag_range get_launch_clock_tags(const NodeId node_id) const { 
            return node_tags_[node_id].tags(TagType::CLOCK_LAUNCH); 
        }
        TimingTags::tag_range get_capture_clock_tags(const NodeId node_id) const { 
            return node_tags_[node_id].tags(TagType::CLOCK_CAPTURE); 
        }

        void add_tag(const NodeId node, const TimingTag& tag) {
            node_tags_[node].add_tag(tag);
        }

        void reset() { 
            node_tags_ = tatum::util::linear_map<NodeId,TimingTags>(node_tags_.size());
        }

    protected:
        tatum::util::linear_map<NodeId,TimingTags> node_tags_;
};

}} //namespace

