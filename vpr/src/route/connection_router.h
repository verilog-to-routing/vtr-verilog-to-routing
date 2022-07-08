#ifndef _CONNECTION_ROUTER_H
#define _CONNECTION_ROUTER_H

#include "connection_router_interface.h"
#include "rr_graph_storage.h"
#include "route_common.h"
#include "router_lookahead.h"
#include "route_tree_type.h"
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
        std::vector<t_rr_node_route_inf>& rr_node_route_inf)
        : grid_(grid)
        , router_lookahead_(router_lookahead)
        , rr_nodes_(rr_nodes.view())
        , rr_graph_(rr_graph)
        , rr_rc_data_(rr_rc_data.data(), rr_rc_data.size())
        , rr_switch_inf_(rr_switch_inf.data(), rr_switch_inf.size())
        , rr_node_route_inf_(rr_node_route_inf.data(), rr_node_route_inf.size())
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

    // Finds a path from the route tree rooted at rt_root to sink_node
    //
    // This is used when you want to allow previous routing of the same net to
    // serve as valid start locations for the current connection.
    //
    // Returns either the last element of the path, or nullptr if no path is
    // found
    std::pair<bool, t_heap> timing_driven_route_connection_from_route_tree(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats) final;

    // Finds a path from the route tree rooted at rt_root to sink_node for a
    // high fanout net.
    //
    // Unlike timing_driven_route_connection_from_route_tree(), only part of
    // the route tree which is spatially close to the sink is added to the heap.
    std::pair<bool, t_heap> timing_driven_route_connection_from_route_tree_high_fanout(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        const SpatialRouteTreeLookup& spatial_rt_lookup,
        RouterStats& router_stats) final;

    // Finds a path from the route tree rooted at rt_root to all sinks
    // available.
    //
    // Each element of the returned vector is a reachable sink.
    //
    // If cost_params.astar_fac is set to 0, this effectively becomes
    // Dijkstra's algorithm with a modified exit condition (runs until heap is
    // empty).  When using cost_params.astar_fac = 0, for efficiency the
    // RouterLookahead used should be the NoOpLookahead.
    std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(
        t_rt_node* rt_root,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats) final;

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
    void add_to_mod_list(int inode) {
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

    // Common logic from timing_driven_route_connection_from_route_tree and
    // timing_driven_route_connection_from_route_tree_high_fanout for running
    // connection router.
    t_heap* timing_driven_route_connection_common_setup(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

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
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Expand this current node if it is a cheaper path.
    void timing_driven_expand_cheapest(
        t_heap* cheapest,
        int target_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Expand each neighbor of the current node.
    void timing_driven_expand_neighbours(
        t_heap* current,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        int target_node);

    // Conditionally adds to_node to the router heap (via path from from_node
    // via from_edge).
    //
    // RR nodes outside bounding box specified in bounding_box are not added
    // to the heap.
    void timing_driven_expand_neighbour(
        t_heap* current,
        const int from_node,
        const RREdgeId from_edge,
        const int to_node,
        const t_conn_cost_params cost_params,
        const t_bb bounding_box,
        int target_node,
        const t_bb target_bb);

    // Add to_node to the heap, and also add any nodes which are connected by
    // non-configurable edges
    void timing_driven_add_to_heap(
        const t_conn_cost_params cost_params,
        const t_heap* current,
        const int from_node,
        const int to_node,
        const RREdgeId from_edge,
        const int target_node);

    // Calculates the cost of reaching to_node
    void evaluate_timing_driven_node_costs(
        t_heap* to,
        const t_conn_cost_params cost_params,
        const int from_node,
        const int to_node,
        const RREdgeId from_edge,
        const int target_node);

    // Find paths from current heap to all nodes in the RR graph
    std::vector<t_heap> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    void empty_heap_annotating_node_route_inf();

    //Adds the route tree rooted at rt_node to the heap, preparing it to be
    //used as branch-points for further routing.
    void add_route_tree_to_heap(t_rt_node* rt_node,
                                int target_node,
                                const t_conn_cost_params cost_params);

    // Evaluate node costs using the RCV algorith
    float compute_node_cost_using_rcv(const t_conn_cost_params cost_params,
                                      const int to_node,
                                      const int target_node,
                                      const float backwards_delay,
                                      const float backwards_cong,
                                      const float R_upstream);

    //Unconditionally adds rt_node to the heap
    //
    //Note that if you want to respect rt_node->re_expand that is the caller's
    //responsibility.
    void add_route_tree_node_to_heap(
        t_rt_node* rt_node,
        int target_node,
        const t_conn_cost_params cost_params);

    t_bb add_high_fanout_route_tree_to_heap(
        t_rt_node* rt_root,
        int target_node,
        const t_conn_cost_params cost_params,
        const SpatialRouteTreeLookup& spatial_route_tree_lookup,
        t_bb net_bounding_box);

    const DeviceGrid& grid_;
    const RouterLookahead& router_lookahead_;
    const t_rr_graph_view rr_nodes_;
    const RRGraphView* rr_graph_;
    vtr::array_view<const t_rr_rc_data> rr_rc_data_;
    vtr::array_view<const t_rr_switch_inf> rr_switch_inf_;
    vtr::array_view<t_rr_node_route_inf> rr_node_route_inf_;
    std::vector<int> modified_rr_node_inf_;
    RouterStats* router_stats_;
    HeapImplementation heap_;
    bool router_debug_;

    // The path manager for RCV, keeps track of the route tree as a set, also manages the allocation of the heap types
    PathManager rcv_path_manager;
};

// Construct a connection router that uses the specified heap type.
std::unique_ptr<ConnectionRouterInterface> make_connection_router(
    e_heap_type heap_type,
    const DeviceGrid& grid,
    const RouterLookahead& router_lookahead,
    const t_rr_graph_storage& rr_nodes,
    const RRGraphView* rr_graph,
    const std::vector<t_rr_rc_data>& rr_rc_data,
    const vtr::vector<RRSwitchId, t_rr_switch_inf>& rr_switch_inf,
    std::vector<t_rr_node_route_inf>& rr_node_route_inf);

#endif /* _CONNECTION_ROUTER_H */
