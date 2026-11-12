#include "clock_domain_groups.h"

#include <algorithm>
#include <numeric>
#include <vector>

#include "tatum/TimingGraph.hpp"
#include "tatum/TimingConstraints.hpp"
#include "tatum/tags/TimingTags.hpp"
#include "vtr_assert.h"

tatum::util::linear_map<tatum::DomainId, ClockDomainGroupId> group_synchronous_clock_domains(const tatum::TimingGraph& timing_graph,
                                                                                             const tatum::TimingConstraints& timing_constraints,
                                                                                             const tatum::SetupTimingAnalyzer& setup_analyzer,
                                                                                             const tatum::HoldTimingAnalyzer& hold_analyzer) {
    size_t num_domains = timing_constraints.clock_domains().size();

    // Start with every domain in its own group, labeled by its own index.
    std::vector<size_t> label(num_domains);
    std::iota(label.begin(), label.end(), 0);

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
                VTR_ASSERT_MSG(size_t(launch_domain) < num_domains && size_t(capture_domain) < num_domains,
                               "Slack tag domains must be clock domains of the timing constraints");

                size_t keep = std::min(label[size_t(launch_domain)], label[size_t(capture_domain)]);
                size_t drop = std::max(label[size_t(launch_domain)], label[size_t(capture_domain)]);
                if (keep != drop)
                    std::replace(label.begin(), label.end(), drop, keep);
            }
        }
    }

    // Convert the labels into contiguous group IDs, in order of first appearance.
    std::vector<ClockDomainGroupId> label_to_group(num_domains, ClockDomainGroupId::INVALID());
    tatum::util::linear_map<tatum::DomainId, ClockDomainGroupId> domain_group(num_domains, ClockDomainGroupId::INVALID());
    size_t num_groups = 0;
    for (tatum::DomainId domain_id : timing_constraints.clock_domains()) {
        VTR_ASSERT(size_t(domain_id) < num_domains);
        size_t domain_label = label[size_t(domain_id)];
        if (!label_to_group[domain_label]) {
            label_to_group[domain_label] = ClockDomainGroupId(num_groups);
            num_groups++;
        }
        domain_group[domain_id] = label_to_group[domain_label];
    }

    return domain_group;
}
