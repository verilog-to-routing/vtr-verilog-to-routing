#pragma once
#include <vector>
#include <unordered_map>
#include "route_tree_type.h"
#include "vpr_types.h"
#include "timing_info.h"

/***************** Connection based rerouting **********************/
// encompasses both incremental rerouting through route tree pruning
// and targeted reroute of connections that are critical and suboptimal

// lookup and persistent scratch-space resources used for incremental reroute through
// pruning the route tree of large fanouts. Instead of rerouting to each sink of a congested net,
// reroute only the connections to the ones that did not have a legal connection the previous time
class Connection_based_routing_resources {
    // Incremental reroute resources --------------
    // conceptually works like rr_sink_node_to_pin[inet][sink_rr_node_index] to get the pin index for that net
    // each net maps SINK node index -> PIN index for net
    // only need to be built once at the start since the SINK nodes never change
    // the reverse lookup of route_ctx.net_rr_terminals
    vtr::vector<ClusterNetId, std::unordered_map<int, int>> rr_sink_node_to_pin;

    // a property of each net, but only valid after pruning the previous route tree
    // the "targets" in question can be either rr_node indices or pin indices, the
    // conversion from node to pin being performed by this class
    std::vector<int> remaining_targets;

    // contains rt_nodes representing sinks reached legally while pruning the route tree
    // used to populate rt_node_of_sink after building route tree from traceback
    // order does not matter
    std::vector<t_rt_node*> reached_rt_sinks;

  public:
    Connection_based_routing_resources();
    // adding to the resources when they are reached during pruning
    // mark rr sink node as something that still needs to be reached
    void toreach_rr_sink(int rr_sink_node) { remaining_targets.push_back(rr_sink_node); }
    // mark rt sink node as something that has been legally reached
    void reached_rt_sink(t_rt_node* rt_sink) { reached_rt_sinks.push_back(rt_sink); }

    // get a handle on the resources
    std::vector<int>& get_remaining_targets() { return remaining_targets; }
    std::vector<t_rt_node*>& get_reached_rt_sinks() { return reached_rt_sinks; }

    void convert_sink_nodes_to_net_pins(std::vector<int>& rr_sink_nodes) const;

    void put_sink_rt_nodes_in_net_pins_lookup(const std::vector<t_rt_node*>& sink_rt_nodes,
                                              t_rt_node** rt_node_of_sink) const;

    bool sanity_check_lookup() const;

    void set_connection_criticality_tolerance(float val) { connection_criticality_tolerance = val; }
    void set_connection_delay_tolerance(float val) { connection_delay_optimality_tolerance = val; }

    // Targeted reroute resources --------------
  private:
    // whether or not a connection should be forcibly rerouted the next iteration
    // takes [inet][sink_rr_node_index] and returns whether that connection should be rerouted or not
    /* reroute connection if all of the following are true:
     * 1. current critical path delay grew from the last stable critical path delay significantly
     * 2. the connection is critical enough
     * 3. the connection is suboptimal, in comparison to lower_bound_connection_delay
     */
    vtr::vector<ClusterNetId, std::unordered_map<int, bool>> forcible_reroute_connection_flag;

    // the optimal delay for a connection [inet][ipin] ([0...num_net][1...num_pin])
    // determined after the first routing iteration when only optimizing for timing delay
    vtr::vector<ClusterNetId, std::vector<float>> lower_bound_connection_delay;

    // the current net that's being routed
    ClusterNetId current_inet;

    // the most recent stable critical path delay
    // compared against the current iteration's critical path delay
    // if the growth is too high, some connections will be forcibly ripped up
    // those ones will be highly critical ones that are suboptimal (compared to 1st iteration minimum delay)
    float last_stable_critical_path_delay;

    // modifiers and tolerances that evolves over iterations and decides when forcible reroute should occur (> 1)
    float critical_path_growth_tolerance;
    // what fraction of max criticality is a connection's criticality considered too much (< 1)
    float connection_criticality_tolerance;
    // what fraction of a connection's lower bound delay is considered close enough to optimal (> 1)
    float connection_delay_optimality_tolerance;

  public:
    // after timing analysis of 1st iteration, can set a lower bound on connection delay
    void set_lower_bound_connection_delays(vtr::vector<ClusterNetId, float*>& net_delay);

    // initialize routing resources at the start of routing to a new net
    void prepare_routing_for_net(ClusterNetId inet) {
        current_inet = inet;
        // fresh net with fresh targets
        remaining_targets.clear();
        reached_rt_sinks.clear();
    }

    // get a handle on the resources
    ClusterNetId get_current_inet() const { return current_inet; }
    float get_stable_critical_path_delay() const { return last_stable_critical_path_delay; }

    bool critical_path_delay_grew_significantly(float new_critical_path_delay) const {
        return new_critical_path_delay > last_stable_critical_path_delay * critical_path_growth_tolerance;
    }

    // for updating the last stable path delay
    void set_stable_critical_path_delay(float stable_critical_path_delay) { last_stable_critical_path_delay = stable_critical_path_delay; }

    // get whether the connection to rr_sink_node of current_inet should be forcibly rerouted (can either assign or just read)
    bool should_force_reroute_connection(int rr_sink_node) const {
        auto itr = forcible_reroute_connection_flag[current_inet].find(rr_sink_node);

        if (itr == forcible_reroute_connection_flag[current_inet].end()) {
            return false; //A non-SINK end of a branch
        }
        return itr->second;
    }
    void clear_force_reroute_for_connection(int rr_sink_node);
    void clear_force_reroute_for_net();

    // check each connection of each net to see if any satisfy the criteria described above (for the forcible_reroute_connection_flag data structure)
    // and if so, mark them to be rerouted
    bool forcibly_reroute_connections(float max_criticality,
                                      std::shared_ptr<const SetupTimingInfo> timing_info,
                                      const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                      vtr::vector<ClusterNetId, float*>& net_delay);
};

using CBRR = Connection_based_routing_resources; // shorthand
