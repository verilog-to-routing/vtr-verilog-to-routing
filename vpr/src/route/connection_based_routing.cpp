#include "connection_based_routing.h"

#include "route_timing.h"
#include "route_profiling.h"

// incremental rerouting resources class definitions
Connection_based_routing_resources::Connection_based_routing_resources()
    : current_inet(NO_PREVIOUS)
    , // not routing to a specific net yet (note that NO_PREVIOUS is not unsigned, so will be largest unsigned)
    last_stable_critical_path_delay{0.0f}
    , critical_path_growth_tolerance{1.001f}
    , connection_criticality_tolerance{0.9f}
    , connection_delay_optimality_tolerance{1.1f} {
    /* Initialize the persistent data structures for incremental rerouting
     * this includes rr_sink_node_to_pin, which provides pin lookup given a
     * sink node for a specific net.
     *
     * remaining_targets will reserve enough space to ensure it won't need
     * to grow while storing the sinks that still need routing after pruning
     *
     * reached_rt_sinks will also reserve enough space, but instead of
     * indices, it will store the pointers to route tree nodes */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    // can have as many targets as sink pins (total number of pins - SOURCE pin)
    // supposed to be used as persistent vector growing with push_back and clearing at the start of each net routing iteration
    auto max_sink_pins_per_net = std::max(get_max_pins_per_net() - 1, 0);
    remaining_targets.reserve(max_sink_pins_per_net);
    reached_rt_sinks.reserve(max_sink_pins_per_net);

    size_t routing_num_nets = cluster_ctx.clb_nlist.nets().size();
    rr_sink_node_to_pin.resize(routing_num_nets);
    lower_bound_connection_delay.resize(routing_num_nets);
    forcible_reroute_connection_flag.resize(routing_num_nets);

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        // unordered_map<int,int> net_node_to_pin;
        auto& net_node_to_pin = rr_sink_node_to_pin[net_id];
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[net_id];
        auto& net_forcible_reroute_connection_flag = forcible_reroute_connection_flag[net_id];

        unsigned int num_pins = cluster_ctx.clb_nlist.net_pins(net_id).size();
        net_node_to_pin.reserve(num_pins);                                                         // not looking up on the SOURCE pin
        net_lower_bound_connection_delay.resize(num_pins, std::numeric_limits<float>::infinity()); // will be filled in after the 1st iteration's
        net_forcible_reroute_connection_flag.reserve(num_pins);                                    // all false to begin with

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[net_id][ipin];

            net_node_to_pin.insert({rr_sink_node, ipin});
            net_forcible_reroute_connection_flag.insert({rr_sink_node, false});
        }
    }
}

void Connection_based_routing_resources::convert_sink_nodes_to_net_pins(std::vector<int>& rr_sink_nodes) const {
    /* Turn a vector of device_ctx.rr_nodes indices, assumed to be of sinks for a net *
     * into the pin indices of the same net. */

    VTR_ASSERT(current_inet != ClusterNetId::INVALID()); // not uninitialized

    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (size_t s = 0; s < rr_sink_nodes.size(); ++s) {
        auto mapping = node_to_pin_mapping.find(rr_sink_nodes[s]);
        if (mapping != node_to_pin_mapping.end()) {
            rr_sink_nodes[s] = mapping->second;
        } else {
            VTR_ASSERT_SAFE_MSG(false, "Should always expect it find a pin mapping for its own net");
        }
    }
}

void Connection_based_routing_resources::put_sink_rt_nodes_in_net_pins_lookup(const std::vector<t_rt_node*>& sink_rt_nodes,
                                                                              t_rt_node** rt_node_of_sink) const {
    /* Load rt_node_of_sink (which maps a PIN index to a route tree node)
     * with a vector of route tree sink nodes. */

    VTR_ASSERT(current_inet != ClusterNetId::INVALID());

    // a net specific mapping from node index to pin index
    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (t_rt_node* rt_node : sink_rt_nodes) {
        auto mapping = node_to_pin_mapping.find(rt_node->inode);

        if (mapping != node_to_pin_mapping.end()) {
            rt_node_of_sink[mapping->second] = rt_node;
        } else {
            VTR_ASSERT_SAFE_MSG(false, "element should be able to find itself");
        }
    }
}

bool Connection_based_routing_resources::sanity_check_lookup() const {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        const auto& net_node_to_pin = rr_sink_node_to_pin[net_id];

        for (auto mapping : net_node_to_pin) {
            auto sanity = net_node_to_pin.find(mapping.first);
            if (sanity == net_node_to_pin.end()) {
                VTR_LOG("%d cannot find itself (net %lu)\n", mapping.first, size_t(net_id));
                return false;
            }
            VTR_ASSERT(route_ctx.net_rr_terminals[net_id][mapping.second] == mapping.first);
        }
    }
    return true;
}

void Connection_based_routing_resources::set_lower_bound_connection_delays(vtr::vector<ClusterNetId, float*>& net_delay) {
    /* Set the lower bound connection delays after first iteration, which only optimizes for timing delay.
     * This will be used later to judge the optimality of a connection, with suboptimal ones being candidates
     * for forced reroute */

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[net_id];

        for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            net_lower_bound_connection_delay[ipin] = net_delay[net_id][ipin];
        }
    }
}

void Connection_based_routing_resources::update_lower_bound_connection_delay(ClusterNetId net, int ipin, float delay) {
    if (lower_bound_connection_delay[net][ipin] > delay) {
        //VTR_LOG("Found better connection delay for Net %zu Pin %d (was: %g, now: %g)\n", size_t(net), ipin, lower_bound_connection_delay[net][ipin], delay);
        lower_bound_connection_delay[net][ipin] = delay;
    }
}

/* Run through all non-congested connections of all nets and see if any need to be forcibly rerouted.
 * The connection must satisfy all following criteria:
 * 1. the connection is critical enough
 * 2. the connection is suboptimal, in comparison to lower_bound_connection_delay  */
bool Connection_based_routing_resources::forcibly_reroute_connections(float max_criticality,
                                                                      std::shared_ptr<const SetupTimingInfo> timing_info,
                                                                      const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                                      vtr::vector<ClusterNetId, float*>& net_delay) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    bool any_connection_rerouted = false; // true if any connection has been marked for rerouting

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[net_id][ipin];

            //Clear any forced re-routing from the previous iteration
            forcible_reroute_connection_flag[net_id][rr_sink_node] = false;

            // skip if connection is internal to a block such that SOURCE->OPIN->IPIN->SINK directly, which would have 0 time delay
            if (lower_bound_connection_delay[net_id][ipin - 1] == 0)
                continue;

            // update if more optimal connection found
            if (net_delay[net_id][ipin] < lower_bound_connection_delay[net_id][ipin - 1]) {
                lower_bound_connection_delay[net_id][ipin - 1] = net_delay[net_id][ipin];
                continue;
            }

            // skip if connection criticality is too low (not a problem connection)
            float pin_criticality = calculate_clb_net_pin_criticality(*timing_info, netlist_pin_lookup, pin_id);
            if (pin_criticality < (max_criticality * connection_criticality_tolerance))
                continue;

            // skip if connection's delay is close to optimal
            if (net_delay[net_id][ipin] < (lower_bound_connection_delay[net_id][ipin - 1] * connection_delay_optimality_tolerance))
                continue;

            forcible_reroute_connection_flag[net_id][rr_sink_node] = true;
            // note that we don't set forcible_reroute_connection_flag to false when the converse is true
            // resetting back to false will be done during tree pruning, after the sink has been legally reached
            any_connection_rerouted = true;

            profiling::mark_for_forced_reroute();
        }
    }

    // non-stable configuration if any connection has to be rerouted, otherwise stable
    return !any_connection_rerouted;
}

void Connection_based_routing_resources::clear_force_reroute_for_connection(int rr_sink_node) {
    forcible_reroute_connection_flag[current_inet][rr_sink_node] = false;
    profiling::perform_forced_reroute();
}

void Connection_based_routing_resources::clear_force_reroute_for_net() {
    VTR_ASSERT(current_inet != ClusterNetId::INVALID());

    auto& net_flags = forcible_reroute_connection_flag[current_inet];
    for (auto& force_reroute_flag : net_flags) {
        if (force_reroute_flag.second) {
            force_reroute_flag.second = false;
            profiling::perform_forced_reroute();
        }
    }
}
