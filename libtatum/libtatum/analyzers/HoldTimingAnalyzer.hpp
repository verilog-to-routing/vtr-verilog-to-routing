#pragma once

#include "TimingAnalyzer.hpp"
#include "TimingTags.hpp"
#include "timing_graph_fwd.hpp"

namespace tatum {

/**
 * HoldTimingAnalyzer represents an abstract interface for all timing analyzers
 * performing hold (i.e. short-path) analysis.
 *
 * Note the use of virtual inheritance to avoid duplicating the base class
 */
class HoldTimingAnalyzer : public virtual TimingAnalyzer {
    public:
        TimingTags::tag_range hold_tags(NodeId node_id) const { return hold_tags_impl(node_id); }
        TimingTags::tag_range hold_tags(NodeId node_id, TagType type) const { return hold_tags_impl(node_id, type); }
        TimingTags::tag_range hold_slacks(EdgeId edge_id) const { return hold_edge_slacks_impl(edge_id); }
        TimingTags::tag_range hold_slacks(NodeId node_id) const { return hold_node_slacks_impl(node_id); }

    protected:
        virtual TimingTags::tag_range hold_tags_impl(NodeId node_id) const = 0;
        virtual TimingTags::tag_range hold_tags_impl(NodeId node_id, TagType type) const = 0;
        virtual TimingTags::tag_range hold_edge_slacks_impl(EdgeId edge_id) const = 0;
        virtual TimingTags::tag_range hold_node_slacks_impl(NodeId node_id) const = 0;
};

} //namepsace
