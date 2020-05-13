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
#ifdef TATUM_CALCULATE_EDGE_SLACKS
            , edge_slacks_(num_edges)
#else
#endif
            , node_slacks_(num_nodes) {
            static_cast<void>(num_edges); //Avoid unused param warning
        }

        CommonAnalysisOps(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps(CommonAnalysisOps&&) = delete;
        CommonAnalysisOps& operator=(const CommonAnalysisOps&) = delete;
        CommonAnalysisOps& operator=(CommonAnalysisOps&&) = delete;

        TimingTags::mutable_tag_range get_mutable_tags(const NodeId node_id) { 
            return node_tags_[node_id].mutable_tags(); 
        }

        TimingTags::mutable_tag_range get_mutable_tags(const NodeId node_id, TagType type) { 
            return node_tags_[node_id].mutable_tags(type); 
        }

        TimingTags::mutable_tag_range get_mutable_slack_tags(const NodeId node_id) { 
            return node_slacks_[node_id].mutable_tags(); 
        }

        TimingTags::tag_range get_tags(const NodeId node_id) const { 
            return node_tags_[node_id].tags(); 
        }
        TimingTags::tag_range get_tags(const NodeId node_id, TagType type) const {
            return node_tags_[node_id].tags(type); 
        }

        bool set_tag(const NodeId node, const TimingTag& tag) {
            return node_tags_[node].set_tag(tag);
        }

        void reset_node(const NodeId node) { 
            node_tags_[node].clear();
            node_slacks_[node].clear();
        }

        bool merge_slack_tags(const NodeId node, const Time time, TimingTag ref_tag) { 
            ref_tag.set_type(TagType::SLACK);
            return node_slacks_[node].min(time, ref_tag.origin_node(), ref_tag); 
        }

        TimingTags::tag_range get_node_slacks(const NodeId node) const {
            return node_slacks_[node].tags(TagType::SLACK);
        }

#ifdef TATUM_CALCULATE_EDGE_SLACKS
        bool merge_slack_tags(const EdgeId edge, const Time time, TimingTag ref_tag) { 
            ref_tag.set_type(TagType::SLACK);
            return edge_slacks_[edge].min(time, ref_tag.origin_node(), ref_tag); 
        }

        TimingTags::tag_range get_edge_slacks(const EdgeId edge) const {
            return edge_slacks_[edge].tags(TagType::SLACK);
        }

        void reset_edge(const EdgeId edge) { 
            edge_slacks_[edge].clear();
        }
#endif

        Time invalid_slack_time() const {
            return Time(std::numeric_limits<float>::infinity());
        }


    protected:
        tatum::util::linear_map<NodeId,TimingTags> node_tags_;

#ifdef TATUM_CALCULATE_EDGE_SLACKS
        tatum::util::linear_map<EdgeId,TimingTags> edge_slacks_;
#endif

        tatum::util::linear_map<NodeId,TimingTags> node_slacks_;
};

}} //namespace

