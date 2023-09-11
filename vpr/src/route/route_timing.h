#pragma once

#include <unordered_map>
#include <vector>

#include "connection_based_routing.h"
#include "connection_router_interface.h"
#include "heap_type.h"
#include "netlist.h"
#include "route_budgets.h"
#include "router_stats.h"
#include "router_lookahead.h"
#include "routing_predictor.h"
#include "rr_graph_type.h"
#include "spatial_route_tree_lookup.h"
#include "timing_info_fwd.h"
#include "vpr_types.h"
#include "vpr_utils.h"

#include "NetPinTimingInvalidator.h"

extern bool f_router_debug;

/** TODO: remove timing_driven_route_structs together with this fn */
int get_max_pins_per_net(const Netlist<>& net_list);

/** Types and defines common to timing_driven and parallel routers */

#define CONGESTED_SLOPE_VAL -0.04

/** Per-iteration congestion mode for the router: focus more on routability after a certain threshold */
enum class RouterCongestionMode {
    NORMAL,
    CONFLICTED
};

/** Identifies the two breakpoint types in routing */
typedef enum router_breakpoint_type {
    BP_ROUTE_ITER,
    BP_NET_ID
} bp_router_type;

/** Results from attempting to route a net.
 * success: Could we route it?
 * was_rerouted: Is the routing different from the last one? (set by try_* functions)
 * retry_with_full_bb: Should we retry this net with a full-device bounding box? (used in the parallel router)
 *
 * I'm fine with returning 3 bytes from a fn: consider an enum class if this becomes too big */
struct NetResultFlags {
    bool success = false;
    bool was_rerouted = false;
    bool retry_with_full_bb = false;
};

struct RoutingMetrics {
    size_t used_wirelength = 0;

    float sWNS = std::numeric_limits<float>::quiet_NaN();
    float sTNS = std::numeric_limits<float>::quiet_NaN();
    float hWNS = std::numeric_limits<float>::quiet_NaN();
    float hTNS = std::numeric_limits<float>::quiet_NaN();
    tatum::TimingPathInfo critical_path;
};

/* Data while timing driven route is active */
class timing_driven_route_structs {
  public:
    std::vector<float> pin_criticality; /* [1..max_pins_per_net-1] */

    timing_driven_route_structs(const Netlist<>& net_list) {
        int max_sinks = std::max(get_max_pins_per_net(net_list) - 1, 0);
        pin_criticality.resize(max_sinks + 1);

        /* Set element 0 to invalid values */
        pin_criticality[0] = std::numeric_limits<float>::quiet_NaN();
    }
};

/** Returns the bounding box of a net's used routing resources */
t_bb calc_current_bb(const RouteTree& tree);

/** Get available wirelength for the current RR graph */
size_t calculate_wirelength_available();

/** Calculate wirelength for the current routing and populate a WirelengthInfo */
WirelengthInfo calculate_wirelength_info(const Netlist<>& net_list, size_t available_wirelength);

size_t dynamic_update_bounding_boxes(const std::vector<ParentNetId>& updated_nets,
                                     const Netlist<>& net_list,
                                     int high_fanout_threshold);

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

void enable_router_debug(const t_router_opts& router_opts, ParentNetId net, RRNodeId sink_rr, int router_iteration, ConnectionRouterInterface* router);

void generate_route_timing_reports(const t_router_opts& router_opts,
                                   const t_analysis_opts& analysis_opts,
                                   const SetupTimingInfo& timing_info,
                                   const RoutingDelayCalculator& delay_calc,
                                   bool is_flat);

/** Initialize net_delay based on best-case delay estimates from the router lookahead. */
void init_net_delay_from_lookahead(const RouterLookahead& router_lookahead,
                                   const Netlist<>& net_list,
                                   const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                                   NetPinsMatrix<float>& net_delay,
                                   const RRGraphView& rr_graph,
                                   bool is_flat);

void init_router_stats(RouterStats& router_stats);

bool is_better_quality_routing(const vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                               const RoutingMetrics& best_routing_metrics,
                               const WirelengthInfo& wirelength_info,
                               std::shared_ptr<const SetupHoldTimingInfo> timing_info);

bool is_iteration_complete(bool routing_is_feasible, const t_router_opts& router_opts, int itry, std::shared_ptr<const SetupHoldTimingInfo> timing_info, bool rcv_finished);

/** Print the index of this routing failure */
void print_overused_nodes_status(const t_router_opts& router_opts, const OveruseInfo& overuse_info);

void print_route_status_header();

void print_route_status(int itry,
                        double elapsed_sec,
                        float pres_fac,
                        int num_bb_updated,
                        const RouterStats& router_stats,
                        const OveruseInfo& overuse_info,
                        const WirelengthInfo& wirelength_info,
                        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                        float est_success_iteration);

void print_router_criticality_histogram(const Netlist<>& net_list,
                                        const SetupTimingInfo& timing_info,
                                        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                        bool is_flat);

/** If a route is ripped up during routing, non-configurable sets are left
 * behind. As a result, the final routing may have stubs at
 * non-configurable sets. This function tracks non-configurable set usage,
 * and if the sets are unused, prunes them. */
void prune_unused_non_configurable_nets(CBRR& connections_inf,
                                        const Netlist<>& net_list);

/**
 * If flat_routing and has_choking_spot are true, there are some choke points inside the cluster which would increase the convergence time of routing.
 * To address this issue, the congestion cost of those choke points needs to decrease. This function identify those choke points for each net,
 * and since the amount of congestion reduction is dependant on the number sinks reachable from that choke point, it also store the number of reachable sinks
 * for each choke point.
 * @param net_list
 * @param net_terminal_groups [Net_id][group_id] -> rr_node_id of the pins in the group
 * @param net_terminal_group_num [Net_id][pin_id] -> group_id
 * @param has_choking_spot is true if the given architecture has choking spots inside the cluster
 * @param is_flat is true if flat_routing is enabled
 * @return [Net_id][pin_id] -> [choke_point_rr_node_id, number of sinks reachable by this choke point]
 */

vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> set_nets_choking_spots(const Netlist<>& net_list,
                                                                                                const vtr::vector<ParentNetId,
                                                                                                                  std::vector<std::vector<int>>>& net_terminal_groups,
                                                                                                const vtr::vector<ParentNetId,
                                                                                                                  std::vector<int>>& net_terminal_group_num,
                                                                                                bool has_choking_spot,
                                                                                                bool is_flat);

/** Detect if net should be routed or not */
bool should_route_net(ParentNetId net_id,
                      CBRR& connections_inf,
                      bool if_force_reroute);

bool should_setup_lower_bound_connection_delays(int itry, const t_router_opts& router_opts);

bool timing_driven_check_net_delays(const Netlist<>& net_list,
                                    NetPinsMatrix<float>& net_delay);

bool try_timing_driven_route(const Netlist<>& net_list,
                             const t_det_routing_arch& det_routing_arch,
                             const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             const std::vector<t_segment_inf>& segment_inf,
                             NetPinsMatrix<float>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority,
                             bool is_flat);

/** Attempt to route a single net.
 *
 * @param router The ConnectionRouter instance 
 * @param net_list Input netlist
 * @param net_id
 * @param itry # of iteration
 * @param pres_fac
 * @param router_opts
 * @param connections_inf
 * @param router_stats
 * @param pin_criticality
 * @param rt_node_of_sink Lookup from target_pin-like indices (indicating SINK nodes) to RouteTreeNodes
 * @param net_delay
 * @param netlist_pin_lookup
 * @param timing_info
 * @param pin_timing_invalidator
 * @param budgeting_inf
 * @param worst_neg_slack
 * @param routing_predictor
 * @param choking_spots
 * @param is_flat
 * @return NetResultFlags for this net. success = false means the RR graph is disconnected and the caller can give up */
template<typename ConnectionRouter>
NetResultFlags timing_driven_route_net(ConnectionRouter& router,
                                       const Netlist<>& net_list,
                                       ParentNetId net_id,
                                       int itry,
                                       float pres_fac,
                                       const t_router_opts& router_opts,
                                       CBRR& connections_inf,
                                       RouterStats& router_stats,
                                       std::vector<float>& pin_criticality,
                                       float* net_delay,
                                       const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                       std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                       NetPinTimingInvalidator* pin_timing_invalidator,
                                       route_budgets& budgeting_inf,
                                       float worst_neg_slack,
                                       const RoutingPredictor& routing_predictor,
                                       const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                                       bool is_flat);

template<typename ConnectionRouter>
NetResultFlags try_timing_driven_route_net(ConnectionRouter& router,
                                           const Netlist<>& net_list,
                                           const ParentNetId& net_id,
                                           int itry,
                                           float pres_fac,
                                           const t_router_opts& router_opts,
                                           CBRR& connections_inf,
                                           RouterStats& router_stats,
                                           std::vector<float>& pin_criticality,
                                           NetPinsMatrix<float>& net_delay,
                                           const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                           std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                           NetPinTimingInvalidator* pin_timing_invalidator,
                                           route_budgets& budgeting_inf,
                                           float worst_negative_slack,
                                           const RoutingPredictor& routing_predictor,
                                           const std::vector<std::unordered_map<RRNodeId, int>>& choking_spots,
                                           bool is_flat);

/** Update net_delay value for a single sink in a RouteTree. */
inline void update_net_delay_from_isink(float* net_delay,
                                        const RouteTree& tree,
                                        int isink,
                                        const Netlist<>& net_list,
                                        ParentNetId inet,
                                        TimingInfo* timing_info,
                                        NetPinTimingInvalidator* pin_timing_invalidator) {
    float new_delay = tree.find_by_isink(isink)->Tdel;

    if (pin_timing_invalidator && new_delay != net_delay[isink]) {
        //Delay changed, invalidate for incremental timing update
        VTR_ASSERT_SAFE(timing_info);
        ParentPinId pin = net_list.net_pin(inet, isink);
        pin_timing_invalidator->invalidate_connection(pin, timing_info);
    }

    net_delay[isink] = new_delay;
}

void update_router_stats(RouterStats& router_stats, RouterStats& router_iteration_stats);

#ifndef NO_GRAPHICS
void update_router_info_and_check_bp(bp_router_type type, int net_id);
#endif

void update_rr_base_costs(int fanout);

/** Traverses down a route tree and updates rr_node_inf for all nodes
 * to reflect that these nodes have already been routed to */
void update_rr_route_inf_from_tree(const RouteTreeNode& rt_node);
