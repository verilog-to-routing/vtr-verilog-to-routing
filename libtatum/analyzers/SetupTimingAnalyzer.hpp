#pragma once

#include "TimingAnalyzer.hpp"
#include "TimingTags.hpp"
#include "timing_graph_fwd.hpp"

namespace tatum {

/**
 * SetupTimingAnalyzer represents an abstract interface for all timing analyzers
 * performing setup (i.e. long-path) analysis.
 *
 * Note the use of virtual inheritance to avoid duplicating the base class
 */
class SetupTimingAnalyzer : public virtual TimingAnalyzer {
    public:
        TimingTags::tag_range setup_tags(NodeId node_id) const { return setup_tags_impl(node_id); }
        TimingTags::tag_range setup_tags(NodeId node_id, TagType type) const { return setup_tags_impl(node_id, type); }
        TimingTags::tag_range setup_slacks(EdgeId edge_id) const { return setup_slacks_impl(edge_id); }

    protected:
        virtual TimingTags::tag_range setup_tags_impl(NodeId node_id) const = 0;
        virtual TimingTags::tag_range setup_tags_impl(NodeId node_id, TagType type) const = 0;
        virtual TimingTags::tag_range setup_slacks_impl(EdgeId edge_id) const = 0;

};

} //namepsace
