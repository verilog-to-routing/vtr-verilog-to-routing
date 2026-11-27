#include "cluster_router_lookahead.h"

#include <algorithm>
#include <queue>

#include "cluster_router.h"
#include "vpr_error.h"

/// @brief Most memory the hop tables of one block type may use.
static constexpr size_t MAX_HOP_TABLE_BYTES = 128 * 1024 * 1024;

std::unique_ptr<ClusterRouterLookahead> make_cluster_router_lookahead(e_cluster_router_lookahead lookahead_type,
                                                                      const LbTypeRRGraph& lb_type_graph) {
    switch (lookahead_type) {
        case e_cluster_router_lookahead::NONE:
            return std::make_unique<ClusterRouterNoOpLookahead>();
        case e_cluster_router_lookahead::HOPS:
            return std::make_unique<ClusterRouterHopsLookahead>(lb_type_graph, cluster_router_min_cost_per_edge(lb_type_graph));
        default:
            VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized cluster router lookahead type");
    }

    return nullptr;
}

ClusterRouterHopsLookahead::ClusterRouterHopsLookahead(const LbTypeRRGraph& lb_type_graph, float min_cost_per_hop)
    : lb_type_graph_(lb_type_graph)
    , min_cost_per_hop_(min_cost_per_hop) {
    zero_hops_.resize(lb_type_graph.num_nodes(), 0);
    hop_table_bytes_ = 0;
    sink_hops_ = &zero_hops_;
}

void ClusterRouterHopsLookahead::set_sink(LbRRNodeId sink) {
    auto it = hop_tables_.find(sink);
    if (it != hop_tables_.end()) {
        sink_hops_ = &it->second;
        return;
    }

    // Past the memory budget, give no lower bound.
    const size_t bytes_per_hop_table = lb_type_graph_.num_nodes() * sizeof(uint8_t);
    if (hop_table_bytes_ + bytes_per_hop_table > MAX_HOP_TABLE_BYTES) {
        sink_hops_ = &zero_hops_;
        return;
    }

    // Inserts an empty table for sink that build_hop_table_ fills.
    vtr::vector<LbRRNodeId, uint8_t>& hops = hop_tables_[sink];
    build_hop_table_(sink, hops);
    hop_table_bytes_ += bytes_per_hop_table;
    sink_hops_ = &hops;
}

float ClusterRouterHopsLookahead::remaining_cost(LbRRNodeId node) const {
    uint8_t hops = (*sink_hops_)[node];
    if (hops == UNREACHABLE_HOPS) {
        return UNREACHABLE;
    }
    return min_cost_per_hop_ * hops;
}

void ClusterRouterHopsLookahead::reset() {
    // The current sink's table is about to be freed, so point at the all-zero table.
    sink_hops_ = &zero_hops_;

    hop_tables_.clear();
    hop_table_bytes_ = 0;
}

void ClusterRouterHopsLookahead::build_hop_table_(LbRRNodeId sink, vtr::vector<LbRRNodeId, uint8_t>& hops) const {
    hops.assign(lb_type_graph_.num_nodes(), UNREACHABLE_HOPS);
    hops[sink] = 0;

    // Breadth-first search from the sink, following edges backwards. Nodes
    // are visited in order of hop count, so the first visit gives the fewest hops.
    std::queue<LbRRNodeId> to_visit;
    to_visit.push(sink);
    while (!to_visit.empty()) {
        LbRRNodeId inode = to_visit.front();
        to_visit.pop();

        // Counts stop growing one below UNREACHABLE_HOPS so they stay a lower bound.
        uint8_t next_hops = std::min<int>(hops[inode] + 1, UNREACHABLE_HOPS - 1);
        for (LbRRNodeId src : lb_type_graph_.fanin_nodes(inode)) {
            if (hops[src] == UNREACHABLE_HOPS) {
                hops[src] = next_hops;
                to_visit.push(src);
            }
        }
    }
}
