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
 * \see SetupAnalysisOps
 * \see CommonAnalysisVisitor
 */
class CommonAnalysisOps {
    public:
        CommonAnalysisOps(size_t num_tags, size_t num_slacks) 
            : node_tags_(num_tags)
            , edge_slacks_(num_slacks) {}

        CommonAnalysisOps(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps(CommonAnalysisOps&&) = delete;
        CommonAnalysisOps& operator=(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps& operator=(CommonAnalysisOps&&) = delete;

        TimingTags::tag_range get_tags(const NodeId node_id) { 
            return node_tags_[node_id].tags(); 
        }
        TimingTags::tag_range get_tags(const NodeId node_id, TagType type) { 
            return node_tags_[node_id].tags(type); 
        }

        TimingTags::tag_range get_tags(const NodeId node_id) const { 
            return node_tags_[node_id].tags(); 
        }
        TimingTags::tag_range get_tags(const NodeId node_id, TagType type) const {
            return node_tags_[node_id].tags(type); 
        }

        void add_tag(const NodeId node, const TimingTag& tag) {
            node_tags_[node].add_tag(tag);
        }

        void reset_node(const NodeId node) { 
            node_tags_[node].clear();
        }

        void add_slack(const EdgeId edge, const TimingTag& slack_tag) {
            TATUM_ASSERT(slack_tag.type() == TagType::SLACK);

            edge_slacks_[edge].add_tag(slack_tag); 
        }

        TimingTags::tag_range get_slacks(const EdgeId edge) const {
            return edge_slacks_[edge].tags(TagType::SLACK);
        }

        void reset_edge(const EdgeId edge) { 
            edge_slacks_[edge].clear();
        }


    protected:
        tatum::util::linear_map<NodeId,TimingTags> node_tags_;
        tatum::util::linear_map<EdgeId,TimingTags> edge_slacks_;
};

}} //namespace

