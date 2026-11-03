#include <cstdio>

#include "vpr_error.h"

#include "globals.h"
#include "net_delay.h"
#include "vtr_assert.h"
#include "vtr_math.h"

/* This module keeps track of the time delays for signals to arrive at       *
 * each pin in every net after timing-driven routing is complete. It         *
 * achieves this by first constructing the skeleton route tree               *
 * from the traceback. Next, to obtain the completed route tree, it          *
 * calls functions to calculate the resistance, capacitance, and time        *
 * delays associated with each node. Then, it recursively traverses          *
 * the tree, copying the time delay from each node into the                  *
 * net_delay data structure. Essentially, the delays are calculated          *
 * by building the completed route tree from scratch, and is used            *
 * to check against the time delays computed incrementally during           *
 * timing-driven routing.                                                    */

/********************** Variables local to this module ***********************/

/* Unordered map below stores the pair whose key is the pin index (ranging   *
 * from 1 to net fan-out) that corresponds to the rt_node, and whose value   *
 * is the time delay associated with that node. The map will be used to      *
 * store delays while traversing the nodes of the route tree in              *
 * load_one_net_delay_recurr.      */

static std::unordered_map<int, float> ipin_to_Tdel_map;

/*********************** Subroutines local to this module ********************/

static const RouteTree& get_net_route_tree(ParentNetId net_id);

static void load_one_net_delay(const Netlist<>& net_list,
                               const RouteTree& tree,
                               NetPinsMatrix<float>& net_delay,
                               ParentNetId net_id);

static RouteTree make_route_tree_with_timing(const RouteTree& tree, DelayType delay_type);

#ifdef VTR_ASSERT_SAFE_ENABLED
static void check_net_min_max_delays(const Netlist<>& net_list,
                                     const NetPinsMatrix<float>& net_delay_min,
                                     const NetPinsMatrix<float>& net_delay_max);
#endif

static void load_one_net_delay_recurr(const RouteTreeNode& node, ParentNetId net_id);

static void load_one_constant_net_delay(const Netlist<>& net_list,
                                        NetPinsMatrix<float>& net_delay,
                                        ParentNetId net_id,
                                        float delay_value);

/*************************** Subroutine definitions **************************/
void load_net_delay_from_routing(const Netlist<>& net_list, NetPinsMatrix<float>& net_delay) {
    /* This routine loads net_delay[0..nets.size()-1][1..num_pins-1].  Each entry   *
     * is the Elmore delay from the net source to the appropriate sink. Both       *
     * the rr_graph and the routing traceback must be completely constructed        *
     * before this routine is called, and the net_delay array must have been        *
     * allocated.                                                                   */

    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id)) {
            load_one_constant_net_delay(net_list, net_delay, net_id, 0.);
        } else {
            load_one_net_delay(net_list, get_net_route_tree(net_id), net_delay, net_id);
        }
    }
}

void load_net_delay_from_routing(const Netlist<>& net_list,
                                 NetPinsMatrix<float>& net_delay_min,
                                 NetPinsMatrix<float>& net_delay_max) {
    /* Same as above, but also loads the min delays into net_delay_min. */

    load_net_delay_from_routing(net_list, net_delay_max);

    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id)) {
            load_one_constant_net_delay(net_list, net_delay_min, net_id, 0.);
        } else {
            // The route tree holds the max delays computed by the router, so the min delays
            // are loaded from a copy of it with its timing reloaded with the min delays.
            RouteTree min_tree = make_route_tree_with_timing(get_net_route_tree(net_id), DelayType::MIN);
            load_one_net_delay(net_list, min_tree, net_delay_min, net_id);
        }
    }

#ifdef VTR_ASSERT_SAFE_ENABLED
    check_net_min_max_delays(net_list, net_delay_min, net_delay_max);
#endif
}

static const RouteTree& get_net_route_tree(ParentNetId net_id) {
    /* Returns the route tree of a net, which must exist. */
    const auto& route_ctx = g_vpr_ctx.routing();

    if (!route_ctx.route_trees[net_id]) {
        VPR_FATAL_ERROR(VPR_ERROR_TIMING,
                        "in load_one_net_delay: Route tree for net %lu does not exist.\n", size_t(net_id));
    }

    return route_ctx.route_trees[net_id].value();
}

static void load_one_net_delay(const Netlist<>& net_list,
                               const RouteTree& tree,
                               NetPinsMatrix<float>& net_delay,
                               ParentNetId net_id) {
    /* This routine loads delay values for one net in                            *
     * net_delay[net_id][1..num_pins-1] from its route tree.                     *
     * It walks the route tree recursively, storing the time delays for          *
     * each sink into the map ipin_to_Tdel. Then, while looping through the      *
     * net_delay array we search for the pin index in the map, and               *
     * correspondingly update the entry in net_delay. Finally, it clears the     *
     * ipin_to_Tdel_map associated with that net.                                */

    load_one_net_delay_recurr(tree.root(), net_id); // recursively traverse the tree and load entries into the ipin_to_Tdel map

    for (unsigned int ipin = 1; ipin < net_list.net_pins(net_id).size(); ipin++) {
        auto itr = ipin_to_Tdel_map.find(ipin);
        if (itr == ipin_to_Tdel_map.end())
            VPR_FATAL_ERROR(VPR_ERROR_TIMING, "Could not find ipin in ipin_to_Tdel_map.");

        net_delay[net_id][ipin] = itr->second; // search for the value of Tdel in the ipin map and load into net_delay
    }
    ipin_to_Tdel_map.clear(); // clear the map
}

static RouteTree make_route_tree_with_timing(const RouteTree& tree, DelayType delay_type) {
    /* Returns a copy of tree with its timing reloaded for delay_type. */
    RouteTree tree_copy(tree);
    tree_copy.reload_timing(vtr::nullopt, delay_type);
    return tree_copy;
}

#ifdef VTR_ASSERT_SAFE_ENABLED
static void check_net_min_max_delays(const Netlist<>& net_list,
                                     const NetPinsMatrix<float>& net_delay_min,
                                     const NetPinsMatrix<float>& net_delay_max) {
    /* Debug check of the min delays loaded from a copy of each route tree:     *
     * - reloading the max delays of a copy must give the router's max delays,  *
     *   which checks that reloading a copy gives the same delays as the router *
     * - the min delay must not be larger than the max delay.                   *
     * A relative tolerance is used, since the router updates the delays        *
     * incrementally, which can round differently from reloading from scratch.  */
    constexpr float ERROR_TOL = 0.0001;

    auto reloaded_net_delay_max = make_net_pins_matrix<float>(net_list);

    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id)) continue;

        RouteTree max_tree = make_route_tree_with_timing(get_net_route_tree(net_id), DelayType::MAX);
        load_one_net_delay(net_list, max_tree, reloaded_net_delay_max, net_id);

        for (unsigned int ipin = 1; ipin < net_list.net_pins(net_id).size(); ipin++) {
            float delay_min = net_delay_min[net_id][ipin];
            float delay_max = net_delay_max[net_id][ipin];
            float reloaded_delay_max = reloaded_net_delay_max[net_id][ipin];

            VTR_ASSERT_SAFE_MSG(vtr::isclose(reloaded_delay_max, delay_max, ERROR_TOL, 0.f),
                                "Max net delay of a reloaded route tree copy does not match the router's max net delay");
            VTR_ASSERT_SAFE_MSG(delay_min <= delay_max || vtr::isclose(delay_min, delay_max, ERROR_TOL, 0.f),
                                "Min net delay is larger than the max net delay");
        }
    }
}
#endif

static void load_one_net_delay_recurr(const RouteTreeNode& rt_node, ParentNetId net_id) {
    /* This routine recursively traverses the route tree, and copies the Tdel of the sink_type nodes *
     * into the map.                                                                                 */
    if (rt_node.net_pin_index != UNDEFINED) {                   // value of UNDEFINED indicates a non-SINK
        ipin_to_Tdel_map[rt_node.net_pin_index] = rt_node.Tdel; // add to the map, process current sink-type node
    }

    for (auto& child : rt_node.child_nodes()) { // process children
        load_one_net_delay_recurr(child, net_id);
    }
}

static void load_one_constant_net_delay(const Netlist<>& net_list,
                                        NetPinsMatrix<float>& net_delay,
                                        ParentNetId net_id,
                                        float delay_value) {
    /* Sets each entry of the net_delay array for net inet to delay_value.     */

    for (unsigned int ipin = 1; ipin < net_list.net_pins(net_id).size(); ipin++)
        net_delay[net_id][ipin] = delay_value;
}
