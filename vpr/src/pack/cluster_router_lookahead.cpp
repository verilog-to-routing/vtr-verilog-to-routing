#include "cluster_router_lookahead.h"

#include <algorithm>
#include <vector>

/// @brief Most memory the hop tables of one block type may use.
static constexpr size_t MAX_HOP_TABLE_BYTES = 128 * 1024 * 1024;

ClusterRouterLookahead::ClusterRouterLookahead(const LbTypeRRGraph& lb_type_graph)
    : lb_type_graph_(lb_type_graph) {
    zero_hops_.resize(lb_type_graph.num_nodes(), 0);
    hop_table_bytes_ = 0;
}

const ClusterRouterLookahead::t_hop_table& ClusterRouterLookahead::hops_to(LbRRNodeId target) {
    auto it = hop_tables_.find(target);
    if (it != hop_tables_.end()) {
        return it->second;
    }

    // Past the memory budget, give no lower bound.
    const size_t table_bytes = zero_hops_.size() * sizeof(uint8_t);
    if (hop_table_bytes_ + table_bytes > MAX_HOP_TABLE_BYTES) {
        return zero_hops_;
    }

    t_hop_table& hops = hop_tables_[target];
    build_hop_table_(target, hops);
    hop_table_bytes_ += table_bytes;
    return hops;
}

void ClusterRouterLookahead::build_hop_table_(LbRRNodeId target, t_hop_table& hops) const {
    hops.assign(zero_hops_.size(), UNREACHABLE);
    hops[target] = 0;

    // Visit the nodes in order of their hop count, following edges backwards
    // from the target. The first visit of a node gives its fewest hops.
    std::vector<LbRRNodeId> visit_order;
    visit_order.push_back(target);
    for (size_t ivisit = 0; ivisit < visit_order.size(); ivisit++) {
        LbRRNodeId inode = visit_order[ivisit];

        // Counts stop growing one below UNREACHABLE so they stay a lower bound.
        uint8_t next_hops = std::min<int>(hops[inode] + 1, UNREACHABLE - 1);
        for (LbRRNodeId src : lb_type_graph_.fanin_nodes(inode)) {
            if (hops[src] == UNREACHABLE) {
                hops[src] = next_hops;
                visit_order.push_back(src);
            }
        }
    }
}
