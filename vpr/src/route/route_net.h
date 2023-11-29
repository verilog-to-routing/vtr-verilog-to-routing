#pragma once

/** @file Net and sink routing functions, and other utils used by them. */

#include <unordered_map>
#include <vector>

#include "connection_based_routing.h"
#include "connection_router_interface.h"
#include "heap_type.h"
#include "netlist.h"
#include "route_budgets.h"
#include "route_utils.h"
#include "router_stats.h"
#include "router_lookahead.h"
#include "routing_predictor.h"
#include "rr_graph_type.h"
#include "spatial_route_tree_lookup.h"
#include "timing_info_fwd.h"
#include "vpr_types.h"
#include "vpr_utils.h"

#include "NetPinTimingInvalidator.h"

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

/** When RCV is enabled, it's necessary to be able to completely ripup high fanout nets
 * if there is still negative hold slack. Normally the router will prune the illegal branches
 * of high fanout nets, this will bypass that */
bool check_hold(const t_router_opts& router_opts, float worst_neg_slack);

/** Return a full-device bounding box */
inline t_bb full_device_bb(void) {
    const auto& grid = g_vpr_ctx.device().grid;
    return {0, (int)grid.width() - 1, 0, (int)grid.height() - 1, 0, (int)grid.get_num_layers() - 1};
}

/** Get criticality of \p pin_id in net \p net_id from 0 to 1 */
float get_net_pin_criticality(const SetupHoldTimingInfo* timing_info,
                              const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                              float max_criticality,
                              float criticality_exp,
                              ParentNetId net_id,
                              ParentPinId pin_id,
                              bool is_flat);

/** Returns true if the specified net fanout is classified as high fanout */
constexpr bool is_high_fanout(int fanout, int fanout_threshold) {
    if (fanout_threshold < 0 || fanout < fanout_threshold)
        return false;
    return true;
}

/** Build a partial route tree in global context for \p net_id from the legal
 * connections from last iteration.
 * Along the way do:
 * - update pathfinder costs to be accurate to the partial route tree
 * - mark the rr_node sinks as targets to be reached. */
void setup_routing_resources(int itry,
                             ParentNetId net_id,
                             const Netlist<>& net_list,
                             unsigned num_sinks,
                             int min_incremental_reroute_fanout,
                             CBRR& connections_inf,
                             const t_router_opts& router_opts,
                             bool ripup_high_fanout_nets);

/** Detect if net should be routed or not */
bool should_route_net(ParentNetId net_id,
                      CBRR& connections_inf,
                      bool if_force_reroute);

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

/** Goes through all the sinks of this net and copies their delay values from
 * the route_tree to the net_delay array. */
void update_net_delays_from_route_tree(float* net_delay,
                                       const Netlist<>& net_list,
                                       ParentNetId inet,
                                       TimingInfo* timing_info,
                                       NetPinTimingInvalidator* pin_timing_invalidator);

/** Change the base costs of rr_nodes globally according to # of fanouts
 * TODO: is this even thread safe? */
void update_rr_base_costs(int fanout);

/** Traverses down a route tree and updates rr_node_inf for all nodes
 * to reflect that these nodes have already been routed to */
void update_rr_route_inf_from_tree(const RouteTreeNode& rt_node);

#include "route_net.tpp"
