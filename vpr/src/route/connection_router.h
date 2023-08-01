#ifndef _CONNECTION_ROUTER_H
#define _CONNECTION_ROUTER_H

#include "connection_router_interface.h"
#include "rr_graph_storage.h"
#include "route_common.h"
#include "router_lookahead.h"
#include "route_tree.h"
#include "rr_rc_data.h"
#include "router_stats.h"
#include "spatial_route_tree_lookup.h"

// Prune the heap when it contains 4x the number of nodes in the RR graph.
constexpr size_t kHeapPruneFactor = 4;

// This class encapsolates the timing driven connection router. This class
// routes from some initial set of sources (via the input rt tree) to a
// particular sink.
//
// When the ConnectionRouter is used, it mutates the provided
// rr_node_route_inf.  The routed path can be found by tracing from the sink
// node (which is returned) through the rr_node_route_inf.  See
// update_traceback as an example of this tracing.
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
        , router_debug_(false) {
        heap_.init_heap(grid);
        heap_.set_prune_limit(rr_nodes_.size(), kHeapPruneFactor * rr_nodes_.size());
    }

    // Clear's the modified list.  Should be called after reset_path_costs
    // have been called.
    void clear_modified_rr_node_info() final {
        modified_rr_node_inf_.clear();
    }

    // Reset modified data in rr_node_route_inf based on modified_rr_node_inf.
    void reset_path_costs() final {
        ::reset_path_costs(modified_rr_node_inf_);
    }

    /** Finds a path from the route tree rooted at rt_root to sink_node.
     * This is used when you want to allow previous routing of the same net to
     * serve as valid start locations for the current connection.
     *
     * Returns a tuple of:
     * bool: path exists? (hard failure, rr graph disconnected)
     * bool: should retry with full bounding box? (only used in parallel routing)
     * t_heap: heap element of cheapest path */
    std::tuple<bool, bool, t_heap> timing_driven_route_connection_from_route_tree(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params,
        bool can_grow_bb) final;

    /** Finds a path from the route tree rooted at rt_root to sink_node for a
     * high fanout net.
     *
     * Unlike timing_driven_route_connection_from_route_tree(), only part of
     * the route tree which is spatially close to the sink is added to the heap.
     *
     * Returns a tuple of:
     * bool: path exists? (hard failure, rr graph disconnected)
     * bool: should retry with full bounding box? (only used in parallel routing)
     * t_heap: heap element of cheapest path */
    std::tuple<bool, bool, t_heap> timing_driven_route_connection_from_route_tree_high_fanout(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params cost_params,
        t_bb net_bounding_box,
        const SpatialRouteTreeLookup& spatial_rt_lookup,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params,
        bool can_grow_bb) final;

    // Finds a path from the route tree rooted at rt_root to all sinks
    // available.
    //
    // Each element of the returned vector is a reachable sink.
    //
    // If cost_params.astar_fac is set to 0, this effectively becomes
    // Dijkstra's algorithm with a modified exit condition (runs until heap is
    // empty).  When using cost_params.astar_fac = 0, for efficiency the
    // RouterLookahead used should be the NoOpLookahead.
    vtr::vector<RRNodeId, t_heap> timing_driven_find_all_shortest_paths_from_route_tree(
        const RouteTreeNode& rt_root,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats,
        const ConnectionParameters& conn_params) final;

    void set_router_debug(bool router_debug) final {
        router_debug_ = router_debug;
    }

    // Empty the route tree set used for RCV node detection
    // Will return if RCV is disabled
    // Called after each net is finished routing to flush the set
    void empty_rcv_route_tree_set() final;

    // Enable or disable RCV in connection router
    // Enabling this will utilize extra path structures, as well as the RCV cost function
    //
    // Ensure route budgets have been calculated before enabling this
    void set_rcv_enabled(bool enable) final;

  private:
    // Mark that data associated with rr_node "inode" has been modified, and
    // needs to be reset in reset_path_costs.
    void add_to_mod_list(RRNodeId inode) {
        if (std::isinf(rr_node_route_inf_[inode].path_cost)) {
            modified_rr_node_inf_.push_back(inode);
        }
    }

    // Update the route path to the node pointed to by cheapest.
    inline void update_cheapest(t_heap* cheapest) {
        update_cheapest(cheapest, &rr_node_route_inf_[cheapest->index]);
    }

    inline void update_cheapest(t_heap* cheapest, t_rr_node_route_inf* route_inf) {
        //Record final link to target
        add_to_mod_list(cheapest->index);

        route_inf->prev_node = cheapest->prev_node();
        route_inf->prev_edge = cheapest->prev_edge();
        route_inf->path_cost = cheapest->cost;
        route_inf->backward_path_cost = cheapest->backward_path_cost;
    }

    /** Common logic from timing_driven_route_connection_from_route_tree and
     * timing_driven_route_connection_from_route_tree_high_fanout for running
     * the connection router.
     * @param[in] rt_root RouteTreeNode describing the current routing state
     * @param[in] sink_node Sink node ID to route to
     * @param[in] cost_params
     * @param[in] bounding_box Keep search confined to this bounding box
     * @param[in] can_grow_bb Can this fn grow the given bounding box? 
     * @return bool Signal to retry this connection with a full-device bounding box,
     * @return t_heap* Heap element describing the path found. */
    std::tuple<bool, t_heap*> timing_driven_route_connection_common_setup(
        const RouteTreeNode& rt_root,
        RRNodeId sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        bool can_grow_bb);

    // Finds a path to sink_node, starting from the elements currently in the
    // heap.
    //
    // This is the core maze routing routine.
    //
    // Note: For understanding the connection router, start here.
    //
    // Returns either the last element of the path, or nullptr if no path is
    // found
    t_heap* timing_driven_route_connection_from_heap(
        RRNodeId sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Expand this current node if it is a cheaper path.
    void timing_driven_expand_cheapest(
        t_heap* cheapest,
        RRNodeId target_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Expand each neighbor of the current node.
    void timing_driven_expand_neighbours(
        t_heap* current,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RRNodeId target_node);

    // Conditionally adds to_node to the router heap (via path from from_node
    // via from_edge).
    //
    // RR nodes outside bounding box specified in bounding_box are not added
    // to the heap.
    void timing_driven_expand_neighbour(
        t_heap* current,
        RRNodeId from_node,
        RREdgeId from_edge,
        RRNodeId to_node,
        const t_conn_cost_params cost_params,
        const t_bb bounding_box,
        RRNodeId target_node,
        const t_bb target_bb);

    // Add to_node to the heap, and also add any nodes which are connected by
    // non-configurable edges
    void timing_driven_add_to_heap(
        const t_conn_cost_params cost_params,
        const t_heap* current,
        RRNodeId from_node,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node);

    // Calculates the cost of reaching to_node
    void evaluate_timing_driven_node_costs(
        t_heap* to,
        const t_conn_cost_params cost_params,
        RRNodeId from_node,
        RRNodeId to_node,
        RREdgeId from_edge,
        RRNodeId target_node);

    // Find paths from current heap to all nodes in the RR graph
    vtr::vector<RRNodeId, t_heap> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    void empty_heap_annotating_node_route_inf();

    //Adds the route tree rooted at rt_node to the heap, preparing it to be
    //used as branch-points for further routing.
    void add_route_tree_to_heap(const RouteTreeNode& rt_node,
                                RRNodeId target_node,
                                const t_conn_cost_params cost_params,
                                bool from_high_fanout);

    // Evaluate node costs using the RCV algorith
    float compute_node_cost_using_rcv(const t_conn_cost_params cost_params,
                                      RRNodeId to_node,
                                      RRNodeId target_node,
                                      float backwards_delay,
                                      float backwards_cong,
                                      float R_upstream);

    //Unconditionally adds rt_node to the heap
    //
    //Note that if you want to respect rt_node->re_expand that is the caller's
    //responsibility.
    void add_route_tree_node_to_heap(
        const RouteTreeNode& rt_node,
        RRNodeId target_node,
        const t_conn_cost_params cost_params,
        bool is_high_fanout);

    t_bb add_high_fanout_route_tree_to_heap(
        const RouteTreeNode& rt_root,
        RRNodeId target_node,
        const t_conn_cost_params cost_params,
        const SpatialRouteTreeLookup& spatial_route_tree_lookup,
        t_bb net_bounding_box);

    const DeviceGrid& grid_;
    const RouterLookahead& router_lookahead_;
    const t_rr_graph_view rr_nodes_;
    const RRGraphView* rr_graph_;
    vtr::array_view<const t_rr_rc_data> rr_rc_data_;
    vtr::array_view<const t_rr_switch_inf> rr_switch_inf_;
    const vtr::vector<ParentNetId, std::vector<std::vector<int>>>& net_terminal_groups;
    const vtr::vector<ParentNetId, std::vector<int>>& net_terminal_group_num;
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf_;
    bool is_flat_;
    std::vector<RRNodeId> modified_rr_node_inf_;
    RouterStats* router_stats_;
    const ConnectionParameters* conn_params_;
    HeapImplementation heap_;
    bool router_debug_;

    // The path manager for RCV, keeps track of the route tree as a set, also manages the allocation of the heap types
    PathManager rcv_path_manager;
};

/** Construct a connection router that uses the specified heap type.
 * This function is not used, but removing it will result in "undefined reference"
 * errors since heap type specializations won't get emitted from connection_router.cpp
 * without it.
 * The alternative is moving all ConnectionRouter fn implementations into the header. */
std::unique_ptr<ConnectionRouterInterface> make_connection_router(
    e_heap_type heap_type,
    const DeviceGrid& grid,
    const RouterLookahead& router_lookahead,
    const t_rr_graph_storage& rr_nodes,
    const RRGraphView* rr_graph,
    const std::vector<t_rr_rc_data>& rr_rc_data,
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
    vtr::vector<RRNodeId, t_rr_node_route_inf>& rr_node_route_inf,
    bool is_flat);

#endif /* _CONNECTION_ROUTER_H */
