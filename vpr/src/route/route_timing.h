#pragma once
#include <unordered_map>
#include <vector>
#include "connection_based_routing.h"
#include "vpr_types.h"

#include "vpr_utils.h"
#include "timing_info_fwd.h"
#include "route_budgets.h"
#include "route_common.h"
#include "router_stats.h"
#include "router_lookahead.h"
#include "spatial_route_tree_lookup.h"

int get_max_pins_per_net();

// This class encapsolates the timing driven connection router.
//
// In particular, this class routes from some initial set of sources to a
// particular sink.
//
// When the ConnectionRouter is used, it mutates the provided
// rr_node_route_inf.  The routed path can be found by tracing from the sink
// node (which is returned) through the rr_node_route_inf.  See
// update_traceback as an example of this tracing.
class ConnectionRouter {
  public:
    ConnectionRouter(
        const RouterLookahead& router_lookahead,
        const t_rr_graph_storage& rr_nodes,
        const std::vector<t_rr_rc_data>& rr_rc_data,
        const std::vector<t_rr_switch_inf>& rr_switch_inf,
        std::vector<t_rr_node_route_inf>& rr_node_route_inf)
        : router_lookahead_(router_lookahead)
        , rr_nodes_(&rr_nodes)
        , rr_rc_data_(rr_rc_data.data())
        , rr_switch_inf_(rr_switch_inf.data())
        , rr_node_route_inf_(rr_node_route_inf.data()) {}

    // Clear's the modified list.  Should be called after reset_path_costs
    // have been called.
    void clear_modified_rr_node_info() {
        modified_rr_node_inf_.clear();
    }

    // Reset modified data in rr_node_route_inf based on modified_rr_node_inf.
    void reset_path_costs() {
        ::reset_path_costs(modified_rr_node_inf_);
    }

    // Finds a path from the route tree rooted at rt_root to sink_node
    //
    // This is used when you want to allow previous routing of the same net to
    // serve as valid start locations for the current connection.
    //
    // Returns either the last element of the path, or nullptr if no path is
    // found
    t_heap* timing_driven_route_connection_from_route_tree(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats);

    // Finds a path from the route tree rooted at rt_root to sink_node for a
    // high fanout net.
    //
    // Unlike timing_driven_route_connection_from_route_tree(), only part of
    // the route tree which is spatially close to the sink is added to the heap.
    t_heap* timing_driven_route_connection_from_route_tree_high_fanout(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        const SpatialRouteTreeLookup& spatial_rt_lookup,
        RouterStats& router_stats);

    // Finds a path from the route tree rooted at rt_root to all sinks
    // available.
    //
    // Each element of the returned vector is a reachable sink.
    std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(
        t_rt_node* rt_root,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        RouterStats& router_stats);

  private:
    // Mark that inode has been modified, and needs to be reset in
    // reset_path_costs.
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

        route_inf->prev_node = cheapest->u.prev.node;
        route_inf->prev_edge = cheapest->u.prev.edge;
        route_inf->path_cost = cheapest->cost;
        route_inf->backward_path_cost = cheapest->backward_path_cost;
    }

    // Common logic from timing_driven_route_connection_from_route_tree and
    // timing_driven_route_connection_from_route_tree_high_fanout for running
    // connection router.
    t_heap* timing_driven_route_connection_from_route_tree(
        t_rt_node* rt_root,
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Finds a path to sink_node, starting from the elements currently in the
    // heap.
    //
    // This is the core maze routing routine.
    //
    // Returns either the last element of the path, or nullptr if no path is
    // found
    t_heap* timing_driven_route_connection_from_heap(
        int sink_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    // Expand this current node if it is the cheapest path to this node.
    void timing_driven_expand_cheapest(
        t_heap* cheapest,
        int target_node,
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    void timing_driven_expand_neighbours(
        t_heap* current,
        const t_conn_cost_params cost_params,
        t_bb bounding_box,
        int target_node);
    void timing_driven_expand_neighbour(
        t_heap* current,
        const int from_node,
        const RREdgeId from_edge,
        const t_edge_size from_edge_int,
        const int to_node,
        const t_conn_cost_params cost_params,
        const t_bb bounding_box,
        int target_node,
        const t_bb target_bb);
    void timing_driven_add_to_heap(
        const t_conn_cost_params cost_params,
        const t_heap* current,
        const int from_node,
        const int to_node,
        const RREdgeId from_edge,
        const int iconn,
        const int target_node);
    void evaluate_timing_driven_node_costs(
        t_heap* to,
        const t_conn_cost_params cost_params,
        const int from_node,
        const int to_node,
        const RREdgeId from_edge,
        const int target_node);
    std::vector<t_heap> timing_driven_find_all_shortest_paths_from_heap(
        const t_conn_cost_params cost_params,
        t_bb bounding_box);

    const RouterLookahead& router_lookahead_;
    const t_rr_graph_storage* rr_nodes_;
    const t_rr_rc_data* rr_rc_data_;
    const t_rr_switch_inf* rr_switch_inf_;
    t_rr_node_route_inf* rr_node_route_inf_;
    std::vector<int> modified_rr_node_inf_;
    RouterStats* router_stats_;
};

bool try_timing_driven_route(const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             const std::vector<t_segment_inf>& segment_inf,
                             vtr::vector<ClusterNetId, float*>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority);

bool try_timing_driven_route_net(ConnectionRouter& router,
                                 ClusterNetId net_id,
                                 int itry,
                                 float pres_fac,
                                 const t_router_opts& router_opts,
                                 CBRR& connections_inf,
                                 RouterStats& connections_routed,
                                 float* pin_criticality,
                                 t_rt_node** rt_node_of_sink,
                                 vtr::vector<ClusterNetId, float*>& net_delay,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 std::shared_ptr<SetupTimingInfo> timing_info,
                                 route_budgets& budgeting_inf,
                                 bool& was_rerouted);

bool timing_driven_route_net(ConnectionRouter& router,
                             ClusterNetId net_id,
                             int itry,
                             float pres_fac,
                             const t_router_opts& router_opts,
                             CBRR& connections_inf,
                             RouterStats& connections_routed,
                             float* pin_criticality,
                             t_rt_node** rt_node_of_sink,
                             float* net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<const SetupTimingInfo> timing_info,
                             route_budgets& budgeting_inf);

void alloc_timing_driven_route_structs(float** pin_criticality_ptr,
                                       int** sink_order_ptr,
                                       t_rt_node*** rt_node_of_sink_ptr);
void free_timing_driven_route_structs(float* pin_criticality, int* sink_order, t_rt_node** rt_node_of_sink);

void enable_router_debug(const t_router_opts& router_opts, ClusterNetId net, int sink_rr, int router_iteration);

//Delay budget information for a specific connection
struct t_conn_delay_budget {
    float short_path_criticality; //Hold criticality

    float min_delay;    //Minimum legal connection delay
    float target_delay; //Target/goal connection delay
    float max_delay;    //Maximum legal connection delay
};

struct t_conn_cost_params {
    float criticality = 1.;
    float astar_fac = 1.2;
    float bend_cost = 1.;
    const t_conn_delay_budget* delay_budget = nullptr;

    //TODO: Eventually once delay budgets are working, t_conn_delay_budget
    //should be factoured out, and the delay budget parameters integrated
    //into this struct instead. For now left as a pointer to control whether
    //budgets are enabled.
};

std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(t_rt_node* rt_root,
                                                                          const t_conn_cost_params cost_params,
                                                                          t_bb bounding_box,
                                                                          std::vector<int>& modified_rr_node_inf,
                                                                          RouterStats& router_stats);

struct timing_driven_route_structs {
    // data while timing driven route is active
    float* pin_criticality;      /* [1..max_pins_per_net-1] */
    int* sink_order;             /* [1..max_pins_per_net-1] */
    t_rt_node** rt_node_of_sink; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs();
    ~timing_driven_route_structs();
};

void update_rr_base_costs(int fanout);
