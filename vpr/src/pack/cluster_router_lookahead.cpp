#include "cluster_router_lookahead.h"

#include <algorithm>
#include <queue>

/// @brief Most memory the hop tables of one block type may use.
static constexpr size_t MAX_HOP_TABLE_BYTES = 128 * 1024 * 1024;

ClusterRouterLookahead::ClusterRouterLookahead(const LbTypeRRGraph& lb_type_graph)
    : lb_type_graph_(lb_type_graph) {
    zero_hops_.resize(lb_type_graph.num_nodes(), 0);
    hop_table_bytes_ = 0;
}

const ClusterRouterLookahead::t_hop_table& ClusterRouterLookahead::hops_to(LbRRNodeId sink) {
    auto it = hop_tables_.find(sink);
    if (it != hop_tables_.end()) {
        return it->second;
    }

    // Past the memory budget, give no lower bound.
    const size_t bytes_per_hop_table = lb_type_graph_.num_nodes() * sizeof(uint8_t);
    if (hop_table_bytes_ + bytes_per_hop_table > MAX_HOP_TABLE_BYTES) {
        return zero_hops_;
    }

    // Inserts an empty table for sink that build_hop_table_ fills.
    t_hop_table& hops = hop_tables_[sink];
    build_hop_table_(sink, hops);
    hop_table_bytes_ += bytes_per_hop_table;
    return hops;
}

void ClusterRouterLookahead::build_hop_table_(LbRRNodeId sink, t_hop_table& hops) const {
    hops.assign(lb_type_graph_.num_nodes(), UNREACHABLE);
    hops[sink] = 0;

    // Breadth-first search from the sink, following edges backwards. Nodes
    // are visited in order of hop count, so the first visit gives the fewest hops.
    std::queue<LbRRNodeId> to_visit;
    to_visit.push(sink);
    while (!to_visit.empty()) {
        LbRRNodeId inode = to_visit.front();
        to_visit.pop();

        // Counts stop growing one below UNREACHABLE so they stay a lower bound.
        uint8_t next_hops = std::min<int>(hops[inode] + 1, UNREACHABLE - 1);
        for (LbRRNodeId src : lb_type_graph_.fanin_nodes(inode)) {
            if (hops[src] == UNREACHABLE) {
                hops[src] = next_hops;
                to_visit.push(src);
            }
        }
    }
}
