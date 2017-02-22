#pragma once
#include "tatum/tags/TimingTags.hpp"
#include "tatum/TimingGraphFwd.hpp"
#include "tatum/util/tatum_linear_map.hpp"

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
        CommonAnalysisOps(size_t num_nodes, size_t num_edges) 
            : node_tags_(num_nodes)
            , edge_slacks_(num_edges)
            , node_slacks_(num_nodes) {}

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
            node_slacks_[node].clear();
        }

        void merge_slack_tags(const EdgeId edge, const Time time, TimingTag ref_tag) { 
            ref_tag.set_type(TagType::SLACK);
            edge_slacks_[edge].min(time, ref_tag.origin_node(), ref_tag); 
        }

        void merge_slack_tags(const NodeId node, const Time time, TimingTag ref_tag) { 
            ref_tag.set_type(TagType::SLACK);
            node_slacks_[node].min(time, ref_tag.origin_node(), ref_tag); 
        }

        TimingTags::tag_range get_edge_slacks(const EdgeId edge) const {
            return edge_slacks_[edge].tags(TagType::SLACK);
        }

        TimingTags::tag_range get_node_slacks(const NodeId node) const {
            return node_slacks_[node].tags(TagType::SLACK);
        }

        void reset_edge(const EdgeId edge) { 
            edge_slacks_[edge].clear();
        }


    protected:
        tatum::util::linear_map<NodeId,TimingTags> node_tags_;

        tatum::util::linear_map<EdgeId,TimingTags> edge_slacks_;
        tatum::util::linear_map<NodeId,TimingTags> node_slacks_;
};

}} //namespace

