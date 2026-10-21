#pragma once
/**
 * @file
 * @brief Lower bounds on the cost from each node of a logical block type's rr
 *        graph to a target node, used to order the cluster router search.
 */

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "lb_type_rr_graph.h"
#include "vtr_vector.h"

/**
 * @brief Fewest edges from each node to each target of one block type's rr
 *        graph, shared by every cluster of that type.
 *
 * The hop counts to a target are computed over the edges of all modes the
 * first time the router searches for that target, and kept for later searches.
 * Once the kept tables reach a memory budget, new targets get an all-zero
 * table instead, which is a valid but useless bound.
 *
 * The cluster router runs single threaded. hops_to fills the cache on a miss
 * and is not safe to call from several threads at once.
 */
class ClusterRouterLookahead {
  public:
    /// @brief Hop count of a node that has no path to the target.
    static constexpr uint8_t UNREACHABLE = 255;

    /// @brief Hop count of every node, indexed by node.
    typedef vtr::vector<LbRrNodeId, uint8_t> t_hop_table;

    /// @brief Builds the in edges of lb_type_graph. Hop tables are built on first use.
    explicit ClusterRouterLookahead(const LbTypeRrGraph& lb_type_graph);

    /// @brief For each node, the fewest edges on any path to target, or UNREACHABLE.
    const t_hop_table& hops_to(LbRrNodeId target);

    /// @brief Lowest cost of any edge in the graph.
    float min_edge_cost() const { return min_edge_cost_; }

  private:
    /// @brief The nodes with an edge into node, in any mode.
    std::span<const LbRrNodeId> in_edges_(LbRrNodeId node) const {
        int32_t first = in_edge_start_[size_t(node)];
        int32_t last = in_edge_start_[size_t(node) + 1];
        return std::span<const LbRrNodeId>(in_edge_sources_.data() + first, last - first);
    }

    /// @brief Fills hops with the fewest edges from every node to target.
    void build_hop_table_(LbRrNodeId target, t_hop_table& hops) const;

    /// @brief For each node, the index in in_edge_sources_ of its first in edge.
    ///        One extra entry at the end holds the total number of edges.
    std::vector<int32_t> in_edge_start_;

    /// @brief The source node of every edge, grouped by the node the edge drives.
    std::vector<LbRrNodeId> in_edge_sources_;

    /// @brief Lowest cost of any edge in the graph.
    float min_edge_cost_;

    /// @brief Hop tables built so far, keyed by target node.
    std::unordered_map<LbRrNodeId, t_hop_table> hop_tables_;

    /// @brief All-zero table handed out once the memory budget is used up.
    t_hop_table zero_hops_;

    /// @brief Total bytes held by the tables in hop_tables_.
    size_t hop_table_bytes_;
};
