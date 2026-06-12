#include "interposer_routing_stats.h"

#include <algorithm>
#include <vector>

#include "device_grid.h"
#include "globals.h"
#include "interposer_types.h"
#include "route_tree.h"
#include "rr_node_types.h"
#include "vpr_context.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"

/// @brief Returns true if the RR node spans different dice and therefore crosses an interposer cut.
static bool rr_node_crosses_interposer(const RRGraphView& rr_graph,
                                       const DeviceGrid& grid,
                                       RRNodeId node) {
    const e_rr_type rr_type = rr_graph.node_type(node);
    if (!is_chanxy(rr_type)) {
        return false;
    }

    if (rr_graph.node_layer_low(node) != rr_graph.node_layer_high(node)) {
        return false;
    }

    const t_physical_tile_loc side_high = {rr_graph.node_xhigh(node),
                                           rr_graph.node_yhigh(node),
                                           rr_graph.node_layer_high(node)};
    const t_physical_tile_loc side_low = {rr_graph.node_xlow(node),
                                          rr_graph.node_ylow(node),
                                          rr_graph.node_layer_low(node)};

    return !grid.are_locs_on_same_die(side_high, side_low);
}

/// @brief Crossing direction relative to the net source: INC if the source is on the low side of the cut, DEC otherwise.
static Direction get_crossing_direction_relative_to_source(e_interposer_cut_type cut_type,
                                                           int cut_loc,
                                                           int source_x,
                                                           int source_y) {
    if (cut_type == e_interposer_cut_type::VERT) {
        return (source_x <= cut_loc) ? Direction::INC : Direction::DEC;
    }
    return (source_y <= cut_loc) ? Direction::INC : Direction::DEC;
}

static void count_net_crossings_for_cut_type(const Netlist<>& net_list,
                                             const RRGraphView& rr_graph,
                                             const DeviceGrid& grid,
                                             e_interposer_cut_type cut_type,
                                             const std::vector<std::vector<int>>& cut_locs,
                                             std::vector<std::vector<t_interposer_cut_routing_stats>>& cut_stats) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();
    const size_t num_layers = grid.get_num_layers();

    size_t max_cuts = 0;
    for (size_t layer = 0; layer < num_layers; layer++) {
        max_cuts = std::max(max_cuts, cut_locs[layer].size());
    }

    // Per-net crossing state: Direction::NONE means the net does not cross that cut;
    // Direction::INC/DEC record the crossing direction relative to the net source.
    vtr::NdMatrix<Direction, 2> net_crossing_direction({num_layers, max_cuts}, Direction::NONE);

    for (ParentNetId net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id) || net_list.net_sinks(net_id).empty()) {
            continue;
        }

        const vtr::optional<RouteTree>& tree = route_ctx.route_trees[net_id];
        if (!tree) {
            continue;
        }

        const RRNodeId source_rr = tree->root().inode;
        const int source_x = rr_graph.node_xlow(source_rr);
        const int source_y = rr_graph.node_ylow(source_rr);

        net_crossing_direction.fill(Direction::NONE);

        for (const RouteTreeNode& rt_node : tree->all_nodes()) {
            const RRNodeId inode = rt_node.inode;
            if (!rr_node_crosses_interposer(rr_graph, grid, inode)) {
                continue;
            }

            const int layer = rr_graph.node_layer_low(inode);
            const int xlow = rr_graph.node_xlow(inode);
            const int xhigh = rr_graph.node_xhigh(inode);
            const int ylow = rr_graph.node_ylow(inode);
            const int yhigh = rr_graph.node_yhigh(inode);

            const std::vector<int>& layer_cuts = cut_locs[layer];
            for (size_t cut_idx = 0; cut_idx < layer_cuts.size(); cut_idx++) {
                const int cut_loc = layer_cuts[cut_idx];
                bool crosses_this_cut = false;

                if (cut_type == e_interposer_cut_type::VERT) {
                    crosses_this_cut = (xlow <= cut_loc && cut_loc < xhigh);
                } else {
                    crosses_this_cut = (ylow <= cut_loc && cut_loc < yhigh);
                }

                if (!crosses_this_cut) {
                    continue;
                }

                const Direction wire_dir = rr_graph.node_direction(inode);

                t_interposer_cut_routing_stats& stats = cut_stats[layer][cut_idx];
                stats.num_used_wires++;
                if (wire_dir == Direction::INC) {
                    stats.num_used_wires_inc++;
                } else if (wire_dir == Direction::DEC) {
                    stats.num_used_wires_dec++;
                } else if (wire_dir == Direction::BIDIR) {
                    stats.num_used_wires_bidir++;
                }

                if (net_crossing_direction[layer][cut_idx] == Direction::NONE) {
                    const Direction crossing_dir = get_crossing_direction_relative_to_source(cut_type,
                                                                                             cut_loc,
                                                                                             source_x,
                                                                                             source_y);
                    net_crossing_direction[layer][cut_idx] = crossing_dir;
                }
            }
        }

        for (size_t layer = 0; layer < num_layers; layer++) {
            for (size_t cut_idx = 0; cut_idx < cut_locs[layer].size(); cut_idx++) {
                if (net_crossing_direction[layer][cut_idx] == Direction::NONE) {
                    continue;
                }

                t_interposer_cut_routing_stats& stats = cut_stats[layer][cut_idx];
                stats.num_nets_crossing++;

                const Direction crossing_dir = net_crossing_direction[layer][cut_idx];
                if (crossing_dir == Direction::INC) {
                    stats.num_nets_crossing_inc++;
                } else {
                    stats.num_nets_crossing_dec++;
                }
            }
        }
    }
}

static void print_cut_stats(const char* cut_orientation,
                            size_t layer,
                            size_t cut_idx,
                            int cut_loc,
                            const t_interposer_cut_routing_stats& stats) {
    VTR_LOG("  Layer %zu, %s cut %zu at %s=%d:\n",
            layer, cut_orientation, cut_idx,
            (cut_orientation[0] == 'h') ? "y" : "x", cut_loc);
    VTR_LOG("\tNets crossing: %d\n", stats.num_nets_crossing);
    VTR_LOG("\tUsed wires crossing: %d\n", stats.num_used_wires);
    VTR_LOG("\tNets crossing (INC): %d\n", stats.num_nets_crossing_inc);
    VTR_LOG("\tNets crossing (DEC): %d\n", stats.num_nets_crossing_dec);
    VTR_LOG("\tUsed wires crossing (INC): %d\n", stats.num_used_wires_inc);
    VTR_LOG("\tUsed wires crossing (DEC): %d\n", stats.num_used_wires_dec);
    VTR_LOG("\tUsed wires crossing (BIDIR): %d\n", stats.num_used_wires_bidir);
}

void print_interposer_routing_stats(const Netlist<>& net_list) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const DeviceGrid& grid = device_ctx.grid;

    if (!grid.has_interposer_cuts()) {
        return;
    }

    const RRGraphView& rr_graph = device_ctx.rr_graph;
    const std::vector<std::vector<int>>& horizontal_cuts = grid.get_horizontal_interposer_cuts();
    const std::vector<std::vector<int>>& vertical_cuts = grid.get_vertical_interposer_cuts();
    const size_t num_layers = grid.get_num_layers();

    std::vector<std::vector<t_interposer_cut_routing_stats>> horz_cut_stats(num_layers);
    std::vector<std::vector<t_interposer_cut_routing_stats>> vert_cut_stats(num_layers);

    for (size_t layer = 0; layer < num_layers; layer++) {
        horz_cut_stats[layer].resize(horizontal_cuts[layer].size());
        vert_cut_stats[layer].resize(vertical_cuts[layer].size());
    }

    count_net_crossings_for_cut_type(net_list,
                                     rr_graph,
                                     grid,
                                     e_interposer_cut_type::HORZ,
                                     horizontal_cuts,
                                     horz_cut_stats);
    count_net_crossings_for_cut_type(net_list,
                                     rr_graph,
                                     grid,
                                     e_interposer_cut_type::VERT,
                                     vertical_cuts,
                                     vert_cut_stats);

    VTR_LOG("\nInterposer routing statistics:\n");

    for (size_t layer = 0; layer < num_layers; layer++) {
        for (size_t cut_idx = 0; cut_idx < horizontal_cuts[layer].size(); cut_idx++) {
            print_cut_stats("horizontal", layer, cut_idx, horizontal_cuts[layer][cut_idx], horz_cut_stats[layer][cut_idx]);
        }
        for (size_t cut_idx = 0; cut_idx < vertical_cuts[layer].size(); cut_idx++) {
            print_cut_stats("vertical", layer, cut_idx, vertical_cuts[layer][cut_idx], vert_cut_stats[layer][cut_idx]);
        }
    }
}
