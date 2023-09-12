#pragma once
#include <vector>
#include <unordered_map>
#include "route_tree_fwd.h"
#include "vpr_types.h"
#include "timing_info.h"
#include "vpr_net_pins_matrix.h"

/***************** Connection based rerouting **********************/
// encompasses both incremental rerouting through route tree pruning
// and targeted reroute of connections that are critical and suboptimal

// lookup and persistent scratch-space resources used for incremental reroute through
// pruning the route tree of large fanouts. Instead of rerouting to each sink of a congested net,
// reroute only the connections to the ones that did not have a legal connection the previous time
class Connection_based_routing_resources {
  public:
    Connection_based_routing_resources(const Netlist<>& net_list,
                                       const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_terminals,
                                       bool is_flat);

    bool sanity_check_lookup() const;

    void set_connection_criticality_tolerance(float val) { connection_criticality_tolerance = val; }
    void set_connection_delay_tolerance(float val) { connection_delay_optimality_tolerance = val; }

    // Targeted reroute resources --------------
  private:
    const Netlist<>& net_list_;
    const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_terminals_;
    bool is_flat_;
    // whether or not a connection should be forcibly rerouted the next iteration
    // takes [inet][sink_rr_node_index] and returns whether that connection should be rerouted or not
    /* reroute connection if all of the following are true:
     * 1. current critical path delay grew from the last stable critical path delay significantly
     * 2. the connection is critical enough
     * 3. the connection is suboptimal, in comparison to lower_bound_connection_delay
     */
    vtr::vector<ParentNetId, std::unordered_map<RRNodeId, bool>> forcible_reroute_connection_flag;

    // the optimal delay for a connection [inet][ipin] ([0...num_net][1...num_pin])
    // determined after the first routing iteration when only optimizing for timing delay
    vtr::vector<ParentNetId, std::vector<float>> lower_bound_connection_delay;

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
    void set_lower_bound_connection_delays(NetPinsMatrix<float>& net_delay);

    //Updates the connection delay lower bound (if less than current best found)
    void update_lower_bound_connection_delay(ParentNetId net, int ipin, float delay);

    // get a handle on the resources
    float get_stable_critical_path_delay() const { return last_stable_critical_path_delay; }

    bool critical_path_delay_grew_significantly(float new_critical_path_delay) const {
        return new_critical_path_delay > last_stable_critical_path_delay * critical_path_growth_tolerance;
    }

    // for updating the last stable path delay
    void set_stable_critical_path_delay(float stable_critical_path_delay) { last_stable_critical_path_delay = stable_critical_path_delay; }

    // get whether the connection to rr_sink_node of net_id should be forcibly rerouted (can either assign or just read)
    bool should_force_reroute_connection(ParentNetId net_id, RRNodeId rr_sink_node) const {
        auto itr = forcible_reroute_connection_flag[net_id].find(rr_sink_node);

        if (itr == forcible_reroute_connection_flag[net_id].end()) {
            return false; //A non-SINK end of a branch
        }
        return itr->second;
    }
    void clear_force_reroute_for_connection(ParentNetId net_id, RRNodeId rr_sink_node);
    void clear_force_reroute_for_net(ParentNetId net_id);

    // check each connection of each net to see if any satisfy the criteria described above (for the forcible_reroute_connection_flag data structure)
    // and if so, mark them to be rerouted
    bool forcibly_reroute_connections(float max_criticality,
                                      std::shared_ptr<const SetupTimingInfo> timing_info,
                                      const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                      NetPinsMatrix<float>& net_delay);
};

using CBRR = Connection_based_routing_resources; // shorthand
