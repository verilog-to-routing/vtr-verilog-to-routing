#pragma once
/**
 * @file
 * @brief Lower bounds on the cost from each node of a logical block type's RR
 *        graph to a sink node.
 */

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "lb_type_rr_graph.h"
#include "vtr_vector.h"

/**
 * @brief Fewest edges from each node to each sink of one block type's RR
 *        graph, shared by every cluster of that type.
 *
 * The hop counts to a sink are computed over the edges of all modes the
 * first time the router searches for that sink, and kept for later searches.
 * Once the kept tables reach a memory budget, new sinks get an all-zero
 * table instead, which is a valid but useless bound.
 *
 * The cluster router runs single threaded. hops_to fills the cache on a miss
 * and is not safe to call from several threads at once.
 */
class ClusterRouterLookahead {
  public:
    /// @brief Hop count of a node that has no path to the sink.
    static constexpr uint8_t UNREACHABLE = 255;

    /// @brief Lower bound on the cost from every node to one sink, indexed by node.
    /// The cost is currently a hop count.
    /// TODO: Explore other costs:
    ///   - The sum of the edge costs on the path.
    ///   - A fanout factor per hop that depends on the net and node fanout.
    ///   - A delay cost.
    ///   - A cost for scarce pins.
    typedef vtr::vector<LbRRNodeId, uint8_t> t_cost_to_sink;

    /// @brief Hop tables are built on first use. lb_type_graph must outlive this object.
    explicit ClusterRouterLookahead(const LbTypeRRGraph& lb_type_graph);

    /// @brief For each node, the fewest edges on any path to sink, or UNREACHABLE.
    const t_cost_to_sink& hops_to(LbRRNodeId sink);

  private:
    /// @brief Fills hops with the fewest edges from every node to sink.
    void build_hop_table_(LbRRNodeId sink, t_cost_to_sink& hops) const;

    /// @brief The graph the hop tables are computed over.
    const LbTypeRRGraph& lb_type_graph_;

    /// @brief Hop tables built so far, keyed by sink node.
    std::unordered_map<LbRRNodeId, t_cost_to_sink> hop_tables_;

    /// @brief All-zero table handed out once the memory budget is used up.
    t_cost_to_sink zero_hops_;

    /// @brief Total bytes held by the tables in hop_tables_.
    size_t hop_table_bytes_;
};
