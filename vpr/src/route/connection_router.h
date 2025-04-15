#ifndef _CONNECTION_ROUTER_H
#define _CONNECTION_ROUTER_H

/**
 * @file
 * @brief This file defines the ConnectionRouter class.
 *
 * Overview
 * ========
 * The ConnectionRouter represents the timing-driven connection routers, which
 * route from some initial set of sources (via the input rt tree) to a particular
 * sink. VPR supports two timing-driven connection routers, including the serial
 * connection router and the MultiQueue-based parallel connection router. This
 * class defines the interface for the two connection routers and encapsulates
 * the common member variables and helper functions for them.
 *
 * @note
 * When the ConnectionRouter is used, it mutates the provided rr_node_route_inf.
 * The routed path can be found by tracing from the sink node (which is returned)
 * through the rr_node_route_inf. See update_traceback as an example of this tracing.
 *
 */

#include "connection_router_interface.h"
#include "rr_graph_storage.h"
#include "route_common.h"
#include "router_lookahead.h"
#include "route_tree.h"
#include "rr_rc_data.h"
#include "router_stats.h"
#include "spatial_route_tree_lookup.h"

/**
 * @class ConnectionRouter defines the interface for the serial and parallel connection
 * routers and encapsulates the common variables and helper functions for the two routers
 */
template<typename HeapImplementation>
class ConnectionRouter : public ConnectionRouterInterface {
  public:
    ConnectionRouter(
        const DeviceGrid& grid,
        const RouterLookahead& router_lookahead,
        const t_rr_graph_storage& rr_nodes,
        const RRGraphView* rr_graph,
        const std::vector<t_rr_rc_data>& rr_rc_data,
        const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
        vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
        bool is_flat)
        : grid_(grid)
        , router_lookahead_(router_lookahead)
        , rr_nodes_(rr_nodes.view())
        , rr_graph_(rr_graph)
        , rr_rc_data_(rr_rc_data.data(), rr_rc_data.size())
        , rr_switch_inf_(rr_switch_inf.data(), rr_switch_inf.size())
        , net_terminal_groups(g_vpr_ctx.routing().net_terminal_groups)
        , net_terminal_group_num(g_vpr_ctx.routing().net_terminal_group_num)
        , rr_node_route_inf_(rr_node_route_inf)
        , is_flat_(is_flat)
        , router_stats_(nullptr)
        , router_debug_(false)
        , path_search_cumulative_time(0) {
        heap_.init_heap(grid);
    }

    virtual ~ConnectionRouter() {}

    /**
     * @brief Clears the modified list
     * @note Should be called after reset_path_costs have been called
     */
    virtual void clear_modified_rr_node_info() = 0;

    /**
     * @brief Resets modified data in rr_node_route_inf based on modified_rr_node_inf
     */
    virtual void reset_path_costs() = 0;

    /**
     * @brief Finds a path from the route tree rooted at rt_root to sink_node
     * @note This is used when you want to allow previous routing of the same
     * net to serve as valid start locations for the current connection.
     * @param rt_root RouteTreeNode describing the current routing state
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param router_stats Update router statistics
     * @param conn_params Parameters to guide the routing of the given connection
     * @return A tuple of:
     * - bool: path exists? (hard failure, rr graph disconnected)
     * - bool: should retry with full bounding box? (only used in parallel routing)
     * - RTExploredNode: the explored sink node, from which the cheapest path can be found via back-tracing
     */
    std::tuple<bool, bool, RTExploredNode> timing_driven_route_connection_from_route_tree(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    /**
     * @brief Finds a path from the route tree rooted at rt_root to sink_node for a high fanout net
     * @note Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree which
     * is spatially close to the sink is added to the heap.
     * @param rt_root RouteTreeNode describing the current routing state
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param net_bounding_box Keep search confined to this bounding box
     * @param spatial_rt_lookup Route tree spatial lookup
     * @param router_stats Update router statistics
     * @param conn_params Parameters to guide the routing of the given connection
     * @return A tuple of:
     * - bool: path exists? (hard failure, rr graph disconnected)
     * - bool: should retry with full bounding box? (only used in parallel routing)
     * - RTExploredNode: the explored sink node, from which the cheapest path can be found via back-tracing
     */
    std::tuple<bool, bool, RTExploredNode> timing_driven_route_connection_from_route_tree_high_fanout(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bounding_box,
        const SpatialRouteTreeLookup& spatial_rt_lookup,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    /**
     * @brief Finds shortest paths from the route tree rooted at rt_root to all sinks available
     * @note Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree which
     * is spatially close to the sink is added to the heap.
     * @note If cost_params.astar_fac is set to 0, this effectively becomes Dijkstra's algorithm with a
     * modified exit condition (runs until heap is empty). When using cost_params.astar_fac = 0, for
     * efficiency the RouterLookahead used should be the NoOpLookahead.
     * @note This routine is currently used only to generate information that may be helpful in debugging
     * an architecture.
     * @param rt_root RouteTreeNode describing the current routing state
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param router_stats Update router statistics
     * @param conn_params Parameters to guide the routing of the given connection
     * @return A vector where each element is a reachable sink
     */
    virtual vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode& rt_root,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) = 0;

    /**
     * @brief Sets router debug option
     * @param router_debug Router debug option
     */
    void set_router_debug(bool router_debug) final {
        router_debug_ = router_debug;
    }

    /**
     * @brief Empties the route tree set used for RCV node detection
     * @note Will immediately return if RCV is disabled. Called after
     * each net is finished routing to flush the set.
     */
    void empty_rcv_route_tree_set() final {
        rcv_path_manager.empty_route_tree_nodes();
    }

    /**
     * @brief Enables or disables RCV in connection router
     * @note Enabling this will utilize extra path structures, as well as
     * the RCV cost function. Ensure route budgets have been calculated
     * before enabling this.
     * @param enable Whether enabling RCV or not
     */
    virtual void set_rcv_enabled(bool enable) = 0;

  protected:
    /**
     * @brief Common logic from timing_driven_route_connection_from_route_tree and
     * timing_driven_route_connection_from_route_tree_high_fanout for running
     * the connection router.
     * @param rt_root RouteTreeNode describing the current routing state
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @return bool signal to retry this connection with a full-device bounding box
     */
    bool timing_driven_route_connection_common_setup(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    /**
     * @brief Finds a path to sink_node, starting from the elements currently in the heap
     * @note If the path is not found, which means that the path_cost of sink_node in RR
     * node route info has never been updated, `rr_node_route_inf_[sink_node].path_cost`
     * will be the initial value (i.e., float infinity). This case can be detected by
     * `std::isinf(rr_node_route_inf_[sink_node].path_cost)`.
     * @note This is the core maze routing routine. For understanding the connection
     * router, start here.
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     */
    void timing_driven_route_connection_from_heap(
        RRNodeId sink_node,
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box);

    /**
     * @brief Finds the single shortest path from current heap to the sink node in the RR graph
     * @param sink_node Sink node ID to route to
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @param target_bb Prune IPINs that lead to blocks other than the target block
     */
    virtual void timing_driven_find_single_shortest_path_from_heap(RRNodeId sink_node,
                                                                   const t_conn_cost_params& cost_params,
                                                                   const t_bb& bounding_box,
                                                                   const t_bb& target_bb) = 0;

    /**
     * @brief Finds shortest paths from current heap to all nodes in the RR graph
     * @param cost_params Cost function parameters
     * @param bounding_box Keep search confined to this bounding box
     * @return A vector where each element contains the shortest route to a specific sink node
     */
    virtual vtr::vector<RRNodeId, RTExploredNode> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params& cost_params,
        const t_bb& bounding_box) = 0;

    /**
     * @brief Unconditionally adds rt_node to the heap
     * @note If you want to respect rt_node->re_expand that is the caller's responsibility.
     * @todo Consider moving this function into the ConnectionRouter class after checking
     * the different prune functions of the serial and parallel connection routers.
     * @param rt_node RouteTreeNode to be added to the heap
     * @param target_node Target node ID to route to
     * @param cost_params Cost function parameters
     * @param net_bb Do not push to heap if not in bounding box
     */
    virtual void add_route_tree_node_to_heap(
        const RouteTreeNode& rt_node,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const t_bb& net_bb) = 0;

    /**
     * @brief Calculates the cost of reaching to_node
     * @param to Neighbor node to calculate costs before being expanded
     * @param cost_params Cost function parameters
     * @param from_node Current node ID being explored
     * @param target_node Target node ID to route to
     */
    void evaluate_timing_driven_node_costs(
        RTExploredNode* to,
        const t_conn_cost_params& cost_params,
        RRNodeId from_node,
        RRNodeId target_node);

    /**
     * @brief Evaluate node costs using the RCV algorithm
     * @param cost_params Cost function parameters
     * @param to_node Neighbor node to calculate costs before being expanded
     * @param target_node Target node ID to route to
     * @param backwards_delay "Known" delay up to and including to_node
     * @param backwards_cong "Known" congestion up to and including to_node
     * @param R_upstream Upstream resistance to ground from to_node
     * @return Node cost using RCV
     */
    float compute_node_cost_using_rcv(const t_conn_cost_params cost_params,
                                      RRNodeId to_node,
                                      RRNodeId target_node,
                                      float backwards_delay,
                                      float backwards_cong,
                                      float R_upstream);

    /**
     * @brief Adds the route tree rooted at rt_node to the heap, preparing
     * it to be used as branch-points for further routing
     * @param rt_node RouteTreeNode to be added to the heap
     * @param target_node Target node ID to route to
     * @param cost_params Cost function parameters
     * @param net_bb Do not push to heap if not in bounding box
     */
    void add_route_tree_to_heap(const RouteTreeNode& rt_node,
                                RRNodeId target_node,
                                const t_conn_cost_params& cost_params,
                                const t_bb& net_bb);
    /**
     * @brief For high fanout nets, adds only route tree nodes which are
     * spatially close to the sink
     * @param rt_root RouteTreeNode to be added to the heap
     * @param target_node Target node ID to route to
     * @param cost_params Cost function parameters
     * @param spatial_route_tree_lookup Route tree spatial lookup
     * @param net_bounding_box Do not push to heap if not in bounding box
     */
    t_bb add_high_fanout_route_tree_to_heap(
        const RouteTreeNode& rt_root,
        RRNodeId target_node,
        const t_conn_cost_params& cost_params,
        const SpatialRouteTreeLookup& spatial_route_tree_lookup,
        const t_bb& net_bounding_box);

    /** Device grid */
    const DeviceGrid& grid_;

    /** Router lookahead */
    const RouterLookahead& router_lookahead_;

    /** RR node data */
    const t_rr_graph_view rr_nodes_;

    /** RR graph */
    const RRGraphView* rr_graph_;

    /** RR node resistance/capacitance data */
    vtr::array_view<const t_rr_rc_data> rr_rc_data_;

    /** RR switch data */
    vtr::array_view<const t_rr_switch_inf> rr_switch_inf_;

    //@{
    /** Net terminal groups */
    const vtr::vector<ParentNetId, std::vector<std::vector<int>>>& net_terminal_groups;
    const vtr::vector<ParentNetId, std::vector<int>>& net_terminal_group_num;
    //@}

    /** RR node extra information needed during routing */
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf_;

    /** Is flat router enabled or not? */
    bool is_flat_;

    /** Router statistics (e.g., heap push/pop counts) */
    RouterStats* router_stats_;

    /** Parameters to guide the routing of the given connection */
    const ConnectionParameters* conn_params_;

    /** Templated heap instance (e.g., binary heap, 4-ary heap, MultiQueue-based parallel heap) */
    HeapImplementation heap_;

    /** Router debug option */
    bool router_debug_;

    /** Cumulative time spent in the path search part of the connection router */
    std::chrono::microseconds path_search_cumulative_time;

    //@{
    /** The path manager for RCV, keeps track of the route tree as a set, also
     * manages the allocation of `rcv_path_data`. */
    PathManager rcv_path_manager;
    vtr::vector<RRNodeId, t_heap_path*> rcv_path_data;
    //@}
};

#include "connection_router.tpp"

#endif /* _CONNECTION_ROUTER_H */
