#include "clock_domain_groups.h"

#include <algorithm>

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/tags/TimingTags.hpp"

ClockDomainGroups group_synchronous_clock_domains(const tatum::TimingGraph& timing_graph,
                                                  const tatum::TimingConstraints& timing_constraints,
                                                  const tatum::SetupTimingAnalyzer& setup_analyzer,
                                                  const tatum::HoldTimingAnalyzer& hold_analyzer) {
    size_t num_domains = timing_constraints.clock_domains().size();

    // Start with every domain in its own group, labeled by its own ID.
    tatum::util::linear_map<tatum::DomainId, tatum::DomainId> label(num_domains);
    for (tatum::DomainId domain_id : timing_constraints.clock_domains())
        label[domain_id] = domain_id;

    // Tatum produces a slack tag at a timing sink for every launch/capture domain pair which is
    // analyzed (i.e. not cut by the constraints) and has a timing path to that sink. Merging the
    // groups of the domains of these tags therefore groups exactly the domains with analyzed
    // transfers. A merge moves every domain of the higher label into the lower label; there are
    // few clock domains, so this is cheap.
    for (tatum::NodeId sink_node : timing_graph.logical_outputs()) {
        for (tatum::TimingTags::tag_range slack_tags : {setup_analyzer.setup_slacks(sink_node), hold_analyzer.hold_slacks(sink_node)}) {
            for (const tatum::TimingTag& tag : slack_tags) {
                tatum::DomainId launch_domain = tag.launch_clock_domain();
                tatum::DomainId capture_domain = tag.capture_clock_domain();
                // Tags from constant generators have no launch domain.
                if (!launch_domain || !capture_domain)
                    continue;

                tatum::DomainId keep = std::min(label[launch_domain], label[capture_domain]);
                tatum::DomainId drop = std::max(label[launch_domain], label[capture_domain]);
                if (keep != drop)
                    std::replace(label.begin(), label.end(), drop, keep);
            }
        }
    }

    // Convert the labels into contiguous group IDs, in order of first appearance.
    ClockDomainGroups groups;
    groups.domain_group = tatum::util::linear_map<tatum::DomainId, ClockDomainGroupId>(num_domains, ClockDomainGroupId::INVALID());
    tatum::util::linear_map<tatum::DomainId, ClockDomainGroupId> label_group(num_domains, ClockDomainGroupId::INVALID());
    for (tatum::DomainId domain_id : timing_constraints.clock_domains()) {
        tatum::DomainId domain_label = label[domain_id];
        if (!label_group[domain_label]) {
            label_group[domain_label] = ClockDomainGroupId(groups.num_groups);
            groups.num_groups++;
        }
        groups.domain_group[domain_id] = label_group[domain_label];
    }

    return groups;
}
