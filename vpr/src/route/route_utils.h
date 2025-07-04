#pragma once

/** @file Utility functions used in the top-level router (route.cpp). */

#include "clustered_netlist_utils.h"
#include "netlist_fwd.h"
#include "router_stats.h"
#include "timing_info.h"
#include "vpr_net_pins_matrix.h"

#include "RoutingDelayCalculator.h"

/** Identifies the two breakpoint types in routing */
typedef enum router_breakpoint_type {
    BP_ROUTE_ITER,
    BP_NET_ID
} bp_router_type;

/** Per-iteration congestion mode for the router: focus more on routability after a certain threshold */
enum class RouterCongestionMode {
    NORMAL,
    CONFLICTED
};

struct RoutingMetrics {
    size_t used_wirelength = 0;

    float sWNS = std::numeric_limits<float>::quiet_NaN();
    float sTNS = std::numeric_limits<float>::quiet_NaN();
    float hWNS = std::numeric_limits<float>::quiet_NaN();
    float hTNS = std::numeric_limits<float>::quiet_NaN();
    tatum::TimingPathInfo critical_path;
};

/** Returns the bounding box of a net's used routing resources */
t_bb calc_current_bb(const RouteTree& tree);

/** Get available wirelength for the current RR graph */
size_t calculate_wirelength_available();

/** Calculate wirelength for the current routing and populate a WirelengthInfo */
WirelengthInfo calculate_wirelength_info(const Netlist<>& net_list, size_t available_wirelength);

/** Checks that the net delays computed incrementally during timing driven
 * routing match those computed from scratch by the net_delay.cpp module. */
bool check_net_delays(const Netlist<>& net_list, NetPinsMatrix<float>& net_delay);

/** Update bounding box for net if existing routing is close to boundary */
void dynamic_update_bounding_boxes(const std::vector<ParentNetId>& rerouted_nets, std::vector<ParentNetId> out_bb_updated_nets);

/** Early exit code for cases where it is obvious that a successful route will not be found
 * Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit */
bool early_exit_heuristic(const t_router_opts& router_opts, const WirelengthInfo& wirelength_info);

/** Give-up on reconvergent routing if the CPD improvement after the
 * first iteration since convergence is small, compared to the best
 * CPD seen so far */
bool early_reconvergence_exit_heuristic(const t_router_opts& router_opts,
                                        int itry_since_last_convergence,
                                        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                                        const RoutingMetrics& best_routing_metrics);

void generate_route_timing_reports(const t_router_opts& router_opts,
                                   const t_analysis_opts& analysis_opts,
                                   const SetupTimingInfo& timing_info,
                                   const RoutingDelayCalculator& delay_calc,
                                   bool is_flat);

/** Get the maximum number of pins used in the netlist (used to allocate things) */
int get_max_pins_per_net(const Netlist<>& net_list);

/** Get the RouteTree associated with the ClusterNetId.
 * Flat routing maps AtomNetIds to RouteTrees instead, so we need to first look up the associated AtomNetId. */
inline const vtr::optional<RouteTree>& get_route_tree_from_cluster_net_id(ClusterNetId net_id) {
    auto& route_ctx = g_vpr_ctx.routing();
    if (!route_ctx.is_flat) {
        return route_ctx.route_trees[ParentNetId(net_id)];
    } else {
        auto& atom_lookup = g_vpr_ctx.atom().lookup();
        AtomNetId atom_id = atom_lookup.atom_net(net_id);
        return route_ctx.route_trees[ParentNetId(atom_id)];
    }
}

/** Initialize net_delay based on best-case delay estimates from the router lookahead. */
void init_net_delay_from_lookahead(const RouterLookahead& router_lookahead,
                                   const Netlist<>& net_list,
                                   const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                                   NetPinsMatrix<float>& net_delay,
                                   const RRGraphView& rr_graph,
                                   bool is_flat);

bool is_better_quality_routing(const vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                               const RoutingMetrics& best_routing_metrics,
                               const WirelengthInfo& wirelength_info,
                               std::shared_ptr<const SetupHoldTimingInfo> timing_info);

/** This function checks if a routing iteration has completed.
 * When VPR is run normally, we check if routing_budgets_algorithm is disabled, and if the routing is legal
 * With the introduction of yoyo budgeting algorithm, we must check if there are no hold violations
 * in addition to routing being legal and the correct budgeting algorithm being set. */
bool is_iteration_complete(bool routing_is_feasible, const t_router_opts& router_opts, int itry, std::shared_ptr<const SetupHoldTimingInfo> timing_info, bool rcv_finished);

void print_overused_nodes_status(const t_router_opts& router_opts, const OveruseInfo& overuse_info);

void print_route_status(int itry,
                        double elapsed_sec,
                        float pres_fac,
                        int num_bb_updated,
                        const RouterStats& router_stats,
                        const OveruseInfo& overuse_info,
                        const WirelengthInfo& wirelength_info,
                        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                        float est_success_iteration);

void print_route_status_header();

void print_router_criticality_histogram(const Netlist<>& net_list,
                                        const SetupTimingInfo& timing_info,
                                        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                        bool is_flat);

/** Prune stubs of non-config nodes from route_ctx.route_trees.
 * If a route is ripped up during routing, non-configurable sets are left
 * behind. As a result, the final routing may have stubs at
 * non-configurable sets. This function tracks non-configurable set usage,
 * and if the sets are unused, prunes them. */
void prune_unused_non_configurable_nets(CBRR& connections_inf,
                                        const Netlist<>& net_list);

/** If flat_routing and router_opt_choke_points are true, there are some choke points inside the cluster which would increase the convergence time of routing.
 * To address this issue, the congestion cost of those choke points needs to decrease. This function identify those choke points for each net,
 * and since the amount of congestion reduction is dependant on the number sinks reachable from that choke point, it also store the number of reachable sinks
 * for each choke point.
 * @param net_list
 * @param net_terminal_groups [Net_id][group_id] -> rr_node_id of the pins in the group
 * @param net_terminal_group_num [Net_id][pin_id] -> group_id
 * @param router_opt_choke_points is true if the given architecture has choking spots inside the cluster
 * @param is_flat is true if flat_routing is enabled
 * @return [Net_id][pin_id] -> [choke_point_rr_node_id, number of sinks reachable by this choke point] */
vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> set_nets_choking_spots(const Netlist<>& net_list,
                                                                                                const vtr::vector<ParentNetId,
                                                                                                                  std::vector<std::vector<int>>>& net_terminal_groups,
                                                                                                const vtr::vector<ParentNetId,
                                                                                                                  std::vector<int>>& net_terminal_group_num,
                                                                                                bool router_opt_choke_points,
                                                                                                bool is_flat);

/** Wrapper for create_rr_graph() with extra checks */
void try_graph(int width_fac,
               const t_router_opts& router_opts,
               t_det_routing_arch& det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_chan_width_dist chan_width_dist,
               const std::vector<t_direct_inf>& directs,
               bool is_flat);

/* This routine updates the pres_fac used by the drawing functions */
void update_draw_pres_fac(const float new_pres_fac);

#ifndef NO_GRAPHICS
/** Updates router iteration information and checks for router iteration and net id breakpoints
 * Stops after the specified router iteration or net id is encountered */
void update_router_info_and_check_bp(bp_router_type type, int net_id);
#endif

/**
 * @brief Checks whether a given net has been routed.
 *
 * This function determines if the specified net (identified by `net_id`)
 * has routing information associated with it in the current routing context.
 *
 * @param net_id The identifier of the net to check.
 * @return true if the net is routed; false otherwise.
 */
bool is_net_routed(ParentNetId net_id);

/**
 * @brief Checks whether a given net is fully absorbed within sink nodes.
 *
 * This function inspects the route tree of the specified net and determines
 * whether it is fully absorbed into non-routing resources (i.e., it does not
 * occupy any routing channels such as CHANX or CHANY).
 *
 * A net is considered fully absorbed if all its route tree nodes are of types
 * other than CHANX or CHANY (e.g., IPIN, SINK, OPIN).
 *
 * @param net_id The identifier of the net to be checked.
 * @return true if the net is fully absorbed (uses no routing channels); false otherwise.
 */
bool is_net_fully_absorbed(ParentNetId net_id);
