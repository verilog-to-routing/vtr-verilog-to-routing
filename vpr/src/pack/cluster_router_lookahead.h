#pragma once
/**
 * @file
 * @brief Lower bounds on the cost from each node of a logical block type's RR
 *        graph to a sink node.
 */

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <unordered_map>

#include "lb_type_rr_graph.h"
#include "vpr_types.h"
#include "vtr_vector.h"

/**
 * @brief Estimate of the remaining cost from a node to the sink of the current
 *        cluster router search, shared by every cluster of one block type.
 */
class ClusterRouterLookahead {
  public:
    /// @brief Remaining cost of a node that has no path to the sink.
    static constexpr float UNREACHABLE = std::numeric_limits<float>::infinity();

    virtual ~ClusterRouterLookahead() = default;

    /// @brief Makes sink the sink that remaining_cost refers to.
    /// The current sink is shared by every router of this block type, so each
    /// search must call set_sink before it calls remaining_cost.
    virtual void set_sink(LbRRNodeId sink) = 0;

    /// @brief Lower bound on the cost from node to the current sink, or UNREACHABLE.
    virtual float remaining_cost(LbRRNodeId node) const = 0;
};

/// @brief Gives no estimate.
class ClusterRouterNoOpLookahead final : public ClusterRouterLookahead {
  public:
    void set_sink(LbRRNodeId /*sink*/) override {}

    float remaining_cost(LbRRNodeId /*node*/) const override { return 0.0f; }
};

/**
 * @brief Fewest edges from each node to each sink of one block type's RR
 *        graph, shared by every cluster of that type.
 *
 * The hop counts to a sink are computed over the edges of all modes the
 * first time the router searches for that sink, and kept for later searches.
 * Once the kept tables reach a memory budget, new sinks get an all-zero
 * table instead, which is a valid but useless bound.
 *
 * The cluster router runs single threaded. set_sink fills the cache on a miss
 * and is not safe to call from several threads at once.
 */
class ClusterRouterHopsLookahead final : public ClusterRouterLookahead {
  public:
    /// @brief Hop count of a node that has no path to the sink.
    static constexpr uint8_t UNREACHABLE_HOPS = 255;

    /// @brief Lower bound on the cost from every node to one sink, indexed by node.
    /// The cost is currently a hop count.
    /// TODO: Explore other costs:
    ///   - The sum of the edge costs on the path.
    ///   - A fanout factor per hop that depends on the net and node fanout.
    ///   - A delay cost.
    ///   - A cost for scarce pins.
    typedef vtr::vector<LbRRNodeId, uint8_t> t_cost_to_sink;

    /// @brief Hop tables are built on first use. lb_type_graph must outlive this object.
    /// @param min_cost_per_hop Lower bound on the cost the router adds for one edge.
    ClusterRouterHopsLookahead(const LbTypeRRGraph& lb_type_graph, float min_cost_per_hop);

    void set_sink(LbRRNodeId sink) override;

    /// @brief The fewest edges on any path from node to the current sink times
    ///        the lowest cost of one edge, or UNREACHABLE.
    float remaining_cost(LbRRNodeId node) const override;

  private:
    /// @brief Fills hops with the fewest edges from every node to sink.
    void build_hop_table_(LbRRNodeId sink, t_cost_to_sink& hops) const;

    /// @brief The graph the hop tables are computed over.
    const LbTypeRRGraph& lb_type_graph_;

    /// @brief Lower bound on the cost of one edge. A node h hops from the
    ///        sink is at least min_cost_per_hop_ * h away from it.
    float min_cost_per_hop_;

    /// @brief Hop tables built so far, keyed by sink node.
    std::unordered_map<LbRRNodeId, t_cost_to_sink> hop_tables_;

    /// @brief All-zero table handed out once the memory budget is used up.
    t_cost_to_sink zero_hops_;

    /// @brief Total bytes held by the tables in hop_tables_.
    size_t hop_table_bytes_;

    /// @brief Hop table of the current sink. The all-zero table until set_sink is called.
    const t_cost_to_sink* sink_hops_;
};

/// @brief Creates the lookahead of the given kind for one block type's RR graph.
std::unique_ptr<ClusterRouterLookahead> make_cluster_router_lookahead(e_cluster_router_lookahead lookahead_type,
                                                                      const LbTypeRRGraph& lb_type_graph);
