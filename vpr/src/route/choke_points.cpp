#include "choke_points.h"

#include <algorithm>
#include <map>
#include <unordered_map>

#include "globals.h"
#include "physical_types_util.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_strong_id.h"

struct net_sink_group_id_tag {};
/// Identifies one group of a net's sinks (see t_net_sink_groups)
typedef vtr::StrongId<net_sink_group_id_tag> NetSinkGroupId;

struct t_net_sink_groups {
    vtr::vector<NetSinkGroupId, std::vector<RRNodeId>> groups; ///< [group] -> SINK nodes of the group's pins
    std::vector<NetSinkGroupId> pin_group;                     ///< [net pin index] -> group of the pin (invalid for the source pin)
};

/**
 * @return Return zero if the ref_sink_num is not reachable by pin_physical_num, otherwise return the number sinks in sink_grp
 * reachable by pin_physical_num
 */
static int get_num_reachable_sinks(t_physical_tile_type_ptr physical_tile,
                                   const int pin_physcial_num,
                                   const int ref_sink_num,
                                   const std::vector<int>& sink_grp) {
    int num_reachable_sinks = 0;

    auto pb_pin = get_pb_pin_from_pin_physical_num(physical_tile,
                                                   pin_physcial_num);
    const auto& connected_sinks = pb_pin->connected_sinks_ptc;

    // If ref_sink_num is not reachable by pin_physical_num return 0
    if (!connected_sinks.contains(ref_sink_num)) {
        return 0;
    }

    for (auto sink_num : sink_grp) {
        if (connected_sinks.contains(sink_num)) {
            num_reachable_sinks++;
        }
    }

    return num_reachable_sinks;
}

static std::vector<const t_pb_graph_node*> get_sink_hierarchical_parents(t_physical_tile_type_ptr physical_tile,
                                                                         int class_physical_num) {
    std::vector<const t_pb_graph_node*> pb_nodes;
    const t_pb_graph_node* curr_pb_node = get_pb_graph_node_from_pin_physical_num(physical_tile,
                                                                                  get_pin_list_from_class_physical_num(physical_tile,
                                                                                                                       class_physical_num)[0]);
    pb_nodes.reserve(curr_pb_node->pb_type->depth + 1);
    VTR_ASSERT(curr_pb_node != nullptr);

    while (curr_pb_node != nullptr) {
        pb_nodes.push_back(curr_pb_node);
        curr_pb_node = curr_pb_node->parent_pb_graph_node;
    }

    return pb_nodes;
}

/**
 * @brief True if the two classes of a tile share at least two levels of the pb-graph hierarchy, i.e. are in the
 *        same sub-block of the cluster; always true when flat routing is off.
 */
static bool classes_in_same_block(t_physical_tile_type_ptr physical_tile,
                                  int first_class_ptc_num,
                                  int second_class_ptc_num,
                                  bool is_flat) {
    // We don't do reachability analysis if flat-routing is not enabled
    if (!is_flat) {
        return true;
    }

    // Two functions are considered to be in the same group if share at least two level of blocks
    constexpr int NUM_SIMILAR_PB_NODE_THRESHOLD = 2;
    std::vector<int> first_class_pin_list = get_pin_list_from_class_physical_num(physical_tile, first_class_ptc_num);
    std::vector<int> second_class_pin_list = get_pin_list_from_class_physical_num(physical_tile, second_class_ptc_num);

    auto first_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, first_class_pin_list[0]);
    auto second_pb_graph_pin = get_pb_pin_from_pin_physical_num(physical_tile, second_class_pin_list[0]);

    std::vector<const t_pb_graph_node*> first_pb_graph_node_chain;
    auto curr_pb_graph_node = first_pb_graph_pin->parent_node;
    while (curr_pb_graph_node != nullptr) {
        first_pb_graph_node_chain.push_back(curr_pb_graph_node);
        curr_pb_graph_node = curr_pb_graph_node->parent_pb_graph_node;
    }

    int num_shared_pb_graph_node = 0;
    curr_pb_graph_node = second_pb_graph_pin->parent_node;
    while (curr_pb_graph_node != nullptr) {
        auto find_res = std::ranges::find(first_pb_graph_node_chain, curr_pb_graph_node);
        if (find_res != first_pb_graph_node_chain.end()) {
            num_shared_pb_graph_node++;
            if (num_shared_pb_graph_node >= NUM_SIMILAR_PB_NODE_THRESHOLD)
                break;
        }
        curr_pb_graph_node = curr_pb_graph_node->parent_pb_graph_node;
    }

    if (num_shared_pb_graph_node < NUM_SIMILAR_PB_NODE_THRESHOLD) {
        return false;
    } else {
        return true;
    }
}

/// Same as above for the classes of blk_id's tile
static bool classes_in_same_block(ParentBlockId blk_id, int first_class_ptc_num, int second_class_ptc_num, bool is_flat) {
    t_physical_tile_type_ptr physical_tile = physical_tile_type(blk_id, is_flat);
    return classes_in_same_block(physical_tile, first_class_ptc_num, second_class_ptc_num, is_flat);
}

/**
 * @brief Given the sink group, identify the pins which can reach both sink_ptc_num and at least one of the sinks,
 * in the grp.
 * @return Key is the pin number and value is the number of sinks, including sink_ptc_num, in the grp reachable by the pin
 */
static std::map<int, int> get_sink_choking_points(t_physical_tile_type_ptr physical_tile,
                                                  int sink_ptc_num,
                                                  const std::vector<int>& grp) {
    // There may be a choking point if there are at least two sinks in the group!
    VTR_ASSERT(grp.size() > 1);
    std::map<int, int> choking_point;
    const t_sub_tile* sub_tile;
    int sub_tile_rel_cap;
    std::tie(sub_tile, sub_tile_rel_cap) = get_sub_tile_from_class_physical_num(physical_tile, sink_ptc_num);
    auto logical_block = get_logical_block_from_class_physical_num(physical_tile, sink_ptc_num);
    auto sink_hierarchical_parents_node = get_sink_hierarchical_parents(physical_tile, sink_ptc_num);
    for (auto pb_graph_node : sink_hierarchical_parents_node) {
        // We assume that choke points do not occur on the root-level pb_graph_node due to having a relatively populated crossbar
        if (pb_graph_node->is_root()) {
            continue;
        }

        auto pb_pins = get_pb_graph_node_pins(physical_tile,
                                              sub_tile,
                                              logical_block,
                                              sub_tile_rel_cap,
                                              pb_graph_node);
        std::unordered_map<int, int> reachability_inf;
        bool has_unique_val = false;
        int prev_val = -1;
        // Iterate over all of the block's IPINs, and check how many sinks in the group are reachable by each pin.
        // If there are pins that the number of reachable sinks are different, we consider those pins as choke points
        for (int pin_num = pb_pins.low; pin_num <= pb_pins.high; pin_num++) {
            // Only choke points on IPINs are analyzed
            if (get_pin_type_from_pin_physical_num(physical_tile, pin_num) == e_pin_type::RECEIVER) {
                int num_reachable_sinks = get_num_reachable_sinks(physical_tile,
                                                                  pin_num,
                                                                  sink_ptc_num,
                                                                  grp);

                if (num_reachable_sinks != 0) {
                    if (prev_val == -1) {
                        prev_val = num_reachable_sinks;
                    } else {
                        if (prev_val != num_reachable_sinks) {
                            has_unique_val = true;
                        }
                    }
                    if (num_reachable_sinks > 1) {
                        reachability_inf.insert(std::make_pair(pin_num, num_reachable_sinks));
                    }
                }
            }
        }

        // Choke point happens if there is a discrepancy between the number of sinks reachable by the pins
        if (has_unique_val) {
            for (const auto& reachability_it : reachability_inf) {
                choking_point.insert(std::make_pair(reachability_it.first, reachability_it.second));
            }
        }
    }

    return choking_point;
}

/**
 * @brief Group the sinks of each net that are in the same cluster and in the same sub-block of it
 *        (see classes_in_same_block()).
 *
 * @param net_rr_terminals [net][net pin index] -> SOURCE/SINK node of the pin
 * @return [net] -> the net's sink groups
 */
static vtr::vector<ParentNetId, t_net_sink_groups> load_net_sink_groups(const RRGraphView& rr_graph,
                                                                        const Netlist<>& net_list,
                                                                        const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                                                                        bool is_flat) {
    vtr::vector<ParentNetId, t_net_sink_groups> net_sink_groups(net_list.nets().size());

    // A sink joins the first group (in creation order) whose block location equals its own and whose first
    // sink is in the same sub-block. Only the groups at the sink's own location can match, so the groups of
    // the current net are indexed by location; the table is reused across nets to avoid reallocating it.
    std::unordered_map<t_pl_loc, std::vector<NetSinkGroupId>> groups_at_loc;

    for (ParentNetId net_id : net_list.nets()) {
        t_net_sink_groups& sink_groups = net_sink_groups[net_id];
        size_t num_pins = net_list.net_pins(net_id).size();
        sink_groups.groups.reserve(num_pins);
        sink_groups.pin_group.resize(num_pins, NetSinkGroupId::INVALID());
        groups_at_loc.clear();
        for (ParentPinId pin_id : net_list.net_sinks(net_id)) {
            int pin_index = net_list.pin_net_index(pin_id);
            RRNodeId rr_node_num = net_rr_terminals[net_id][pin_index];
            ParentBlockId block_id = net_list.pin_block(pin_id);
            int sink_ptc_num = rr_graph.node_ptc_num(rr_node_num);

            std::vector<NetSinkGroupId>& candidate_groups = groups_at_loc[get_block_loc(block_id, is_flat).loc];
            NetSinkGroupId group_id = NetSinkGroupId::INVALID();
            for (NetSinkGroupId curr_grp_id : candidate_groups) {
                const std::vector<RRNodeId>& curr_grp = sink_groups.groups[curr_grp_id];
                if (classes_in_same_block(block_id,
                                          rr_graph.node_ptc_num(curr_grp[0]),
                                          sink_ptc_num,
                                          is_flat)) {
                    group_id = curr_grp_id;
                    break;
                }
            }

            if (!group_id.is_valid()) {
                NetSinkGroupId new_group_id(sink_groups.groups.size());
                sink_groups.groups.push_back({rr_node_num});
                sink_groups.pin_group[pin_index] = new_group_id;
                candidate_groups.push_back(new_group_id);
            } else {
                sink_groups.groups[group_id].push_back(rr_node_num);
                sink_groups.pin_group[pin_index] = group_id;
            }
        }
        sink_groups.groups.shrink_to_fit();
    }

    return net_sink_groups;
}

vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> set_nets_choking_spots(const Netlist<>& net_list,
                                                                                                bool router_opt_choke_points,
                                                                                                bool is_flat) {
    vtr::vector<ParentNetId, std::vector<std::unordered_map<RRNodeId, int>>> choking_spots(net_list.nets().size());
    for (ParentNetId net_id : net_list.nets()) {
        choking_spots[net_id].resize(net_list.net_pins(net_id).size());
    }

    // Return if the architecture doesn't have any potential choke points or flat router is not enabled
    if (!router_opt_choke_points || !is_flat) {
        return choking_spots;
    }

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminal = route_ctx.net_rr_terminals;

    // The sink groups are only needed here, to find each sink's choke points
    vtr::vector<ParentNetId, t_net_sink_groups> net_sink_groups = load_net_sink_groups(rr_graph, net_list, net_rr_terminal, is_flat);

    for (ParentNetId net_id : net_list.nets()) {
        // Global nets are not routed, thus we don't consider them.
        if (net_list.net_is_global(net_id)) {
            continue;
        }
        for (ParentPinId pin_id : net_list.net_sinks(net_id)) {
            int pin_index = net_list.pin_net_index(pin_id);
            ParentBlockId block_id = net_list.pin_block(pin_id);
            t_block_loc blk_loc = get_block_loc(block_id, is_flat);
            NetSinkGroupId group_id = net_sink_groups[net_id].pin_group[pin_index];
            // This is a group of sinks, including the current pin_id, which share a specific number of parent blocks.
            // To determine the choke points of the current sink, pin_id, we only consider the sinks in this group for the
            // run-time purpose
            const std::vector<RRNodeId>& sink_grp_nodes = net_sink_groups[net_id].groups[group_id];
            VTR_ASSERT(sink_grp_nodes.size() >= 1);
            if (sink_grp_nodes.size() == 1) {
                continue;
            } else {
                // ptc numbers of the sinks in the group (what get_sink_choking_points() works with)
                std::vector<int> sink_grp;
                sink_grp.reserve(sink_grp_nodes.size());
                for (RRNodeId sink_rr_node : sink_grp_nodes) {
                    sink_grp.push_back(rr_graph.node_ptc_num(sink_rr_node));
                }

                t_physical_tile_loc grid_loc;
                grid_loc.x = blk_loc.loc.x;
                grid_loc.y = blk_loc.loc.y;
                grid_loc.layer_num = blk_loc.loc.layer;
                t_physical_tile_type_ptr physical_type = device_ctx.grid.get_physical_type(grid_loc);
                // Get the choke points of the sink corresponds to pin_id given the sink group
                auto sink_choking_spots = get_sink_choking_points(physical_type,
                                                                  rr_graph.node_ptc_num(net_rr_terminal[net_id][pin_index]),
                                                                  sink_grp);
                // Store choke points rr_node_id and the number reachable sinks
                for (const auto& choking_spot : sink_choking_spots) {
                    int pin_physical_num = choking_spot.first;
                    int num_reachable_sinks = choking_spot.second;
                    RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph.node_lookup(),
                                                                 physical_type,
                                                                 grid_loc,
                                                                 pin_physical_num);
                    if (pin_rr_node_id != RRNodeId::INVALID()) {
                        choking_spots[net_id][pin_index].insert(std::make_pair(pin_rr_node_id, num_reachable_sinks));
                    }
                }
            }
        }
    }

    return choking_spots;
}
