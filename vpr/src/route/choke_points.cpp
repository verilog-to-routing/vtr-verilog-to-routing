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
/**
 * @brief Identify a sink group within one net (see t_net_sink_groups).
 */
typedef vtr::StrongId<net_sink_group_id_tag> NetSinkGroupId;

/**
 * @brief Group a net's sinks by cluster and sub-block for choke point analysis.
 */
struct t_net_sink_groups {
    vtr::vector<NetSinkGroupId, std::vector<RRNodeId>> groups; ///< [group ID] -> SINK nodes in the group.
    std::vector<NetSinkGroupId> pin_group;                     ///< [net pin index] -> group ID; invalid for the source.
};

/**
 * @brief Count group sinks reachable from the pin, returning zero if ref_sink_num is unreachable.
 */
static int get_num_reachable_sinks(t_physical_tile_type_ptr physical_tile,
                                   const int pin_physcial_num,
                                   const int ref_sink_num,
                                   const std::vector<int>& sink_grp) {
    int num_reachable_sinks = 0;

    auto pb_pin = get_pb_pin_from_pin_physical_num(physical_tile,
                                                   pin_physcial_num);
    const auto& connected_sinks = pb_pin->connected_sinks_ptc;

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

/**
 * @brief Return the sink class's pb_graph_node and its ancestors, ending at the cluster root.
 */
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
 * @brief Return whether two pin classes share a sub-block within the cluster.
 *
 * Always returns true when flat routing is disabled.
 */
static bool classes_in_same_block(t_physical_tile_type_ptr physical_tile,
                                  int first_class_ptc_num,
                                  int second_class_ptc_num,
                                  bool is_flat) {
    if (!is_flat) {
        return true;
    }

    // Require a shared sub-block in addition to the cluster root.
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

/**
 * @brief Check classes_in_same_block() for the tile containing blk_id.
 */
static bool classes_in_same_block(ParentBlockId blk_id, int first_class_ptc_num, int second_class_ptc_num, bool is_flat) {
    t_physical_tile_type_ptr physical_tile = physical_tile_type(blk_id, is_flat);
    return classes_in_same_block(physical_tile, first_class_ptc_num, second_class_ptc_num, is_flat);
}

/**
 * @brief Find choke points for sink_ptc_num among the sinks in grp.
 * @return Physical pin numbers mapped to reachable group sink counts, including sink_ptc_num.
 */
static std::map<int, int> get_sink_choking_points(t_physical_tile_type_ptr physical_tile,
                                                  int sink_ptc_num,
                                                  const std::vector<int>& grp) {
    // A choke point must reach multiple sinks in the group.
    VTR_ASSERT(grp.size() > 1);
    std::map<int, int> choking_point;
    const t_sub_tile* sub_tile;
    int sub_tile_rel_cap;
    std::tie(sub_tile, sub_tile_rel_cap) = get_sub_tile_from_class_physical_num(physical_tile, sink_ptc_num);
    auto logical_block = get_logical_block_from_class_physical_num(physical_tile, sink_ptc_num);
    auto sink_hierarchical_parents_node = get_sink_hierarchical_parents(physical_tile, sink_ptc_num);
    for (auto pb_graph_node : sink_hierarchical_parents_node) {
        // Assume the cluster root's crossbar is well connected enough to avoid choke points.
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
        // Count group sinks reachable from each input pin that can reach sink_ptc_num.
        for (int pin_num = pb_pins.low; pin_num <= pb_pins.high; pin_num++) {
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

        // If counts differ, record pins that reach multiple group sinks.
        if (has_unique_val) {
            for (const auto& reachability_it : reachability_inf) {
                choking_point.insert(std::make_pair(reachability_it.first, reachability_it.second));
            }
        }
    }

    return choking_point;
}

/**
 * @brief Group each net's sinks by cluster and sub-block (see classes_in_same_block()).
 *
 * @param net_rr_terminals SOURCE/SINK nodes indexed by net ID and net pin index.
 * @return Sink groups indexed by net ID.
 */
static vtr::vector<ParentNetId, t_net_sink_groups> load_net_sink_groups(const RRGraphView& rr_graph,
                                                                        const Netlist<>& net_list,
                                                                        const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                                                                        bool is_flat) {
    vtr::vector<ParentNetId, t_net_sink_groups> net_sink_groups(net_list.nets().size());

    // Search only groups at the sink's location, in creation order, to keep the first match.
    // Reuse the map's buckets across nets.
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

const std::unordered_map<RRNodeId, int>& t_net_choke_points::sink_choke_points(int pin_index) const {
    static const std::unordered_map<RRNodeId, int> no_choke_points;
    return by_pin.empty() ? no_choke_points : by_pin[pin_index];
}

vtr::vector<ParentNetId, t_net_choke_points> set_nets_choking_spots(const Netlist<>& net_list,
                                                                    bool router_opt_choke_points,
                                                                    bool is_flat) {
    vtr::vector<ParentNetId, t_net_choke_points> choking_spots(net_list.nets().size());

    // Choke point analysis requires flat routing and the choke point option.
    if (!router_opt_choke_points || !is_flat) {
        return choking_spots;
    }

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminal = route_ctx.net_rr_terminals;

    // Keep sink groups local to choke point analysis.
    vtr::vector<ParentNetId, t_net_sink_groups> net_sink_groups = load_net_sink_groups(rr_graph, net_list, net_rr_terminal, is_flat);

    for (ParentNetId net_id : net_list.nets()) {
        // Global nets are not routed.
        if (net_list.net_is_global(net_id)) {
            continue;
        }
        for (ParentPinId pin_id : net_list.net_sinks(net_id)) {
            int pin_index = net_list.pin_net_index(pin_id);
            ParentBlockId block_id = net_list.pin_block(pin_id);
            t_block_loc blk_loc = get_block_loc(block_id, is_flat);
            NetSinkGroupId group_id = net_sink_groups[net_id].pin_group[pin_index];
            // Limit this sink's choke point analysis to its group.
            const std::vector<RRNodeId>& sink_grp_nodes = net_sink_groups[net_id].groups[group_id];
            VTR_ASSERT(sink_grp_nodes.size() >= 1);
            if (sink_grp_nodes.size() == 1) {
                continue;
            } else {
                // Convert SINK node IDs to class PTC numbers for reachability analysis.
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
                auto sink_choking_spots = get_sink_choking_points(physical_type,
                                                                  rr_graph.node_ptc_num(net_rr_terminal[net_id][pin_index]),
                                                                  sink_grp);
                // Store each choke point's RR node ID and reachable group sink count.
                for (const auto& choking_spot : sink_choking_spots) {
                    int pin_physical_num = choking_spot.first;
                    int num_reachable_sinks = choking_spot.second;
                    RRNodeId pin_rr_node_id = get_pin_rr_node_id(rr_graph.node_lookup(),
                                                                 physical_type,
                                                                 grid_loc,
                                                                 pin_physical_num);
                    if (pin_rr_node_id != RRNodeId::INVALID()) {
                        // Allocate per-pin maps only for nets with choke points.
                        std::vector<std::unordered_map<RRNodeId, int>>& by_pin = choking_spots[net_id].by_pin;
                        if (by_pin.empty()) {
                            by_pin.resize(net_list.net_pins(net_id).size());
                        }
                        by_pin[pin_index].insert(std::make_pair(pin_rr_node_id, num_reachable_sinks));
                    }
                }
            }
        }
    }

    return choking_spots;
}
