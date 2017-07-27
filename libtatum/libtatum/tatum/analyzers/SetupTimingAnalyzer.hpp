#pragma once

#include "tatum/analyzers/TimingAnalyzer.hpp"
#include "tatum/tags/TimingTags.hpp"
#include "tatum/TimingGraphFwd.hpp"

namespace tatum {

/**
 * SetupTimingAnalyzer represents an abstract interface for all timing analyzers
 * performing setup (i.e. long-path) analysis.
 *
 * Note the use of virtual inheritance to avoid duplicating the base class
 */
class SetupTimingAnalyzer : public virtual TimingAnalyzer {
    public:
        ///Update only the setup timing related arrival/required tags
        void update_setup_timing() { update_setup_timing_impl(); }

        TimingTags::tag_range setup_tags(NodeId node_id) const { return setup_tags_impl(node_id); }
        TimingTags::tag_range setup_tags(NodeId node_id, TagType type) const { return setup_tags_impl(node_id, type); }
        TimingTags::tag_range setup_slacks(EdgeId edge_id) const { return setup_edge_slacks_impl(edge_id); }
        TimingTags::tag_range setup_slacks(NodeId node_id) const { return setup_node_slacks_impl(node_id); }

    protected:
        virtual void update_setup_timing_impl() = 0;
        virtual TimingTags::tag_range setup_tags_impl(NodeId node_id) const = 0;
        virtual TimingTags::tag_range setup_tags_impl(NodeId node_id, TagType type) const = 0;
        virtual TimingTags::tag_range setup_edge_slacks_impl(EdgeId edge_id) const = 0;
        virtual TimingTags::tag_range setup_node_slacks_impl(NodeId node_id) const = 0;
};

} //namepsace
