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
        const TimingTags& get_setup_data_tags(NodeId node_id) const { return get_setup_data_tags_impl(node_id); }
        const TimingTags& get_setup_clock_tags(NodeId node_id) const { return get_setup_clock_tags_impl(node_id); }

    protected:
        virtual const TimingTags& get_setup_data_tags_impl(NodeId node_id) const = 0;
        virtual const TimingTags& get_setup_clock_tags_impl(NodeId node_id) const = 0;

};

} //namepsace
