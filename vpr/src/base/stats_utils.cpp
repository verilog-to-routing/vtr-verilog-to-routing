/**
 * @file
 * @brief Implementation of routing statistics utility functions.
 *
 * See stats_utils.h for an overview.
 */

#include "stats_utils.h"

#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "globals.h"
#include "physical_types.h"
#include "route_tree.h"
#include "rr_graph_view.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_ndmatrix.h"

#include "crr_common.h"

/******************* Tap utilization helpers (file-local) ********************/

/** @brief Accumulates fanout counts per tap for wire segments. */
struct TapAccumulator {
    std::map<int, int> num_wires;                       ///< wire_length -> count of routed wires
    std::map<int, std::map<int, int>> tap_pin_fanouts;  ///< wire_length -> (tap -> fanouts to pins)
    std::map<int, std::map<int, int>> tap_wire_fanouts; ///< wire_length -> (tap -> fanouts to wires)
};

/**
 * @brief Computes the tap number at which child_node exits from parent_node.
 *
 * A routing wire segment spans a contiguous range of grid points along one
 * axis.  Each grid point is a tap.  Taps are numbered starting from 0 at the
 * driving MUX location (inside the switch block that drives the wire) and
 * increase toward the far end of the wire.
 *
 * The driving MUX sits in a switch block whose position depends on direction:
 *   - INC wire: the MUX is in the switch block at tile (x_low - 1) or
 *     (y_low - 1), i.e. one tile before the wire's start coordinate.
 *   - DEC wire: the MUX is in the switch block at tile x_high or y_high,
 *     i.e. at the same tile as the wire's start coordinate (recall the switch
 *     block on the right side of a tile shares the tile's coordinates).
 *
 * BIDIR wires are rejected upstream in print_tap_utilization().
 *
 * @param rr_graph    The routing-resource graph.
 * @param parent_node A CHANX or CHANY node (the wire whose tap we measure).
 * @param child_node  A child of parent_node in the route tree (the fanout).
 * @return            Tap number (0 = MUX location, increasing toward far end).
 */
static int compute_tap(const RRGraphView& rr_graph, RRNodeId parent_node, RRNodeId child_node) {
    e_rr_type parent_type = rr_graph.node_type(parent_node);
    VTR_ASSERT(parent_type == e_rr_type::CHANX || parent_type == e_rr_type::CHANY);
    e_rr_type child_type = rr_graph.node_type(child_node);
    Direction parent_dir = rr_graph.node_direction(parent_node);
    Direction child_dir = rr_graph.node_direction(child_node);
    bool is_x = (parent_type == e_rr_type::CHANX);

    int parent_low = is_x ? rr_graph.node_xlow(parent_node) : rr_graph.node_ylow(parent_node);
    int parent_high = is_x ? rr_graph.node_xhigh(parent_node) : rr_graph.node_yhigh(parent_node);
    // MUX location: for INC wires the driving MUX is one tile before the wire
    // start; for DEC wires it is at the wire start (same tile as x_high/y_high).
    int mux_coord = (parent_dir == Direction::INC) ? parent_low - 1 : parent_high;

    // Connection coordinate along the parent's axis. The ±1 offsets account
    // for switchboxes sitting at tile boundaries.
    //
    //   – Pins (IPIN/OPIN) and CHANZ: single grid point, use child_low.
    //
    //   – Same-axis wires (CHANX→CHANX or CHANY→CHANY):
    //       INC parent + INC child: child_low - 1
    //       INC parent + DEC child: child_high
    //       DEC parent + INC child: child_low
    //       DEC parent + DEC child: child_high + 1
    //
    //   – Perpendicular wires (CHANX→CHANY or CHANY→CHANX):
    //       child_dir is irrelevant; use child_low (+1 for DEC parent).
    int child_low = is_x ? rr_graph.node_xlow(child_node) : rr_graph.node_ylow(child_node);
    int child_high = is_x ? rr_graph.node_xhigh(child_node) : rr_graph.node_yhigh(child_node);

    int connection_coord;
    if (child_type == e_rr_type::CHANZ || child_type == e_rr_type::IPIN || child_type == e_rr_type::OPIN) {
        connection_coord = child_low;
    } else {
        VTR_ASSERT(child_type == e_rr_type::CHANX || child_type == e_rr_type::CHANY);
        bool same_axis = (parent_type == child_type);
        if (same_axis) {
            connection_coord = (child_dir == Direction::INC) ? child_low - 1 : child_high;
        } else {
            VTR_ASSERT(child_low == child_high); // Perpendicular wires should only span one grid point
            connection_coord = child_low;
        }
    }

    int tap = (parent_dir == Direction::INC)
                  ? connection_coord - mux_coord
                  : mux_coord - connection_coord;

    return tap;
}

/** @brief Prints a single tap utilization table for the given category,
 *  with pin and wire fanouts shown as separate columns. */
static void print_tap_stats(const std::string& label, const TapAccumulator& acc) {
    VTR_LOG("\n--- %s ---\n", label.c_str());
    if (acc.num_wires.empty()) {
        VTR_LOG("  (no wires)\n");
        return;
    }

    static const std::map<int, int> empty_map;

    for (const auto& [length, n_wires] : acc.num_wires) {
        auto pin_it = acc.tap_pin_fanouts.find(length);
        auto wire_it = acc.tap_wire_fanouts.find(length);
        const auto& pin_map = (pin_it != acc.tap_pin_fanouts.end()) ? pin_it->second : empty_map;
        const auto& wire_map = (wire_it != acc.tap_wire_fanouts.end()) ? wire_it->second : empty_map;

        int total_pins = 0, total_wires = 0;
        for (const auto& p : pin_map)
            total_pins += p.second;
        for (const auto& p : wire_map)
            total_wires += p.second;

        int max_tap = length;
        if (!pin_map.empty()) max_tap = std::max(max_tap, pin_map.rbegin()->first);
        if (!wire_map.empty()) max_tap = std::max(max_tap, wire_map.rbegin()->first);

        int min_tap = 0;
        if (!pin_map.empty()) min_tap = std::min(min_tap, pin_map.begin()->first);
        if (!wire_map.empty()) min_tap = std::min(min_tap, wire_map.begin()->first);

        VTR_LOG("  Wire length %d (%d wires, %d pin fanouts, %d wire fanouts):\n",
                length, n_wires, total_pins, total_wires);
        for (int tap = min_tap; tap <= max_tap; ++tap) {
            int pin_cnt = pin_map.count(tap) ? pin_map.at(tap) : 0;
            int wire_cnt = wire_map.count(tap) ? wire_map.at(tap) : 0;
            float pin_pct = (total_pins > 0) ? 100.0f * pin_cnt / total_pins : 0.0f;
            float wire_pct = (total_wires > 0) ? 100.0f * wire_cnt / total_wires : 0.0f;
            const char* suffix = (tap > length) ? " [beyond wire end]" : "";
            VTR_LOG("    Tap %2d:  pins %6d (%6.2f%%)  wires %6d (%6.2f%%)%s\n",
                    tap, pin_cnt, pin_pct, wire_cnt, wire_pct, suffix);
        }
    }
}

/************************ Tap utilization (public) **************************/

void print_tap_utilization(const Netlist<>& net_list, const std::vector<t_segment_inf>& segment_inf) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const auto& route_ctx = g_vpr_ctx.routing();

    TapAccumulator overall, chanx_all, chany_all, chanx_inc, chanx_dec, chany_inc, chany_dec;

    for (ParentNetId net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0)
            continue;

        const vtr::optional<RouteTree>& tree = route_ctx.route_trees[net_id];
        if (!tree)
            continue;

        for (const RouteTreeNode& rt_node : tree.value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            e_rr_type ntype = rr_graph.node_type(inode);
            if (ntype != e_rr_type::CHANX && ntype != e_rr_type::CHANY)
                continue;

            RRIndexedDataId cost_index = rr_graph.node_cost_index(inode);
            int seg_type = device_ctx.rr_indexed_data[cost_index].seg_index;
            int wire_length = segment_inf[seg_type].length;
            Direction dir = rr_graph.node_direction(inode);

            if (dir == Direction::BIDIR) {
                VTR_LOG_WARN("Tap utilization assumes a unidirectional architecture "
                             "but a BIDIR wire was encountered.  Skipping tap utilization.\n");
                return;
            }

            // Select the three accumulators this wire contributes to
            TapAccumulator& axis_all = (ntype == e_rr_type::CHANX) ? chanx_all : chany_all;
            TapAccumulator& axis_dir = (ntype == e_rr_type::CHANX)
                                           ? (dir == Direction::INC ? chanx_inc : chanx_dec)
                                           : (dir == Direction::INC ? chany_inc : chany_dec);

            overall.num_wires[wire_length]++;
            axis_all.num_wires[wire_length]++;
            axis_dir.num_wires[wire_length]++;

            // Tally each fanout into pins (IPIN/OPIN) and wires (CHANX/CHANY/CHANZ)
            for (const RouteTreeNode& child : rt_node.child_nodes()) {
                int tap = compute_tap(rr_graph, inode, child.inode);
                e_rr_type child_type = rr_graph.node_type(child.inode);
                bool is_pin = (child_type == e_rr_type::IPIN || child_type == e_rr_type::OPIN);
                VTR_ASSERT(is_pin || child_type == e_rr_type::CHANX || child_type == e_rr_type::CHANY || child_type == e_rr_type::CHANZ);

                for (TapAccumulator* acc : {&overall, &axis_all, &axis_dir}) {
                    (is_pin ? acc->tap_pin_fanouts : acc->tap_wire_fanouts)[wire_length][tap]++;
                }
            }
        }
    }

    VTR_LOG("\n=== Tap Utilization Statistics ===\n");
    print_tap_stats("Overall", overall);
    print_tap_stats("CHANX", chanx_all);
    print_tap_stats("CHANY", chany_all);
    print_tap_stats("INC CHANX", chanx_inc);
    print_tap_stats("DEC CHANX", chanx_dec);
    print_tap_stats("INC CHANY", chany_inc);
    print_tap_stats("DEC CHANY", chany_dec);
}

/****************** Switch block CSV helpers (public) ***********************/

SBKeyParts parse_sb_key(const std::string& key) {
    SBKeyParts parts;

    // Find the last two underscores
    size_t last_underscore = key.rfind('_');
    size_t second_last_underscore = key.rfind('_', last_underscore - 1);

    parts.filename = key.substr(0, second_last_underscore);
    parts.row = std::stoi(key.substr(second_last_underscore + 1, last_underscore - second_last_underscore - 1));
    parts.col = std::stoi(key.substr(last_underscore + 1));

    return parts;
}

std::vector<std::vector<std::string>> read_and_trim_csv(const std::string& filepath) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        VTR_LOG_ERROR("Failed to open file: %s\n", filepath.c_str());
        return data;
    }

    std::string line;
    int row_count = 0;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream ss(line);
        std::string cell;
        int col_count = 0;

        if (row_count < crrgenerator::NUM_EMPTY_ROWS) {
            // Keep entire row for first NUM_EMPTY_ROWS rows
            while (std::getline(ss, cell, ',')) {
                row.push_back(cell);
            }
        } else {
            // Keep only first NUM_EMPTY_COLS columns for other rows
            while (std::getline(ss, cell, ',') && col_count < crrgenerator::NUM_EMPTY_COLS) {
                row.push_back(cell);
                col_count++;
            }
        }

        data.push_back(row);
        row_count++;
    }

    file.close();
    return data;
}

void write_csv(const std::string& filepath, const std::vector<std::vector<std::string>>& data) {
    std::ofstream file(filepath);

    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            file << data[i][j];
            if (j < data[i].size() - 1) {
                file << ",";
            }
        }
        file << "\n";
    }

    file.close();
}

/****************** Channel occupancy helpers (file-local) ******************/

/**
 * @brief Loads the two arrays passed in with the total occupancy at each of the
 *        channel segments in the FPGA.
 */
static void load_channel_occupancies(const Netlist<>& net_list,
                                     vtr::NdMatrix<int, 3>& chanx_occ,
                                     vtr::NdMatrix<int, 3>& chany_occ) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    // First set the occupancy of everything to zero.
    chanx_occ.fill(0);
    chany_occ.fill(0);

    // Now go through each net and count the tracks and pins used everywhere
    for (ParentNetId net_id : net_list.nets()) {
        // Skip global and empty nets.
        if (net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0) {
            continue;
        }

        const vtr::optional<RouteTree>& tree = route_ctx.route_trees[net_id];
        if (!tree) {
            continue;
        }

        for (const RouteTreeNode& rt_node : tree.value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            e_rr_type rr_type = rr_graph.node_type(inode);

            if (rr_type == e_rr_type::CHANX) {
                int j = rr_graph.node_ylow(inode);
                int layer = rr_graph.node_layer_low(inode);
                for (int i = rr_graph.node_xlow(inode); i <= rr_graph.node_xhigh(inode); i++)
                    chanx_occ[layer][i][j]++;
            } else if (rr_type == e_rr_type::CHANY) {
                int i = rr_graph.node_xlow(inode);
                int layer = rr_graph.node_layer_low(inode);
                for (int j = rr_graph.node_ylow(inode); j <= rr_graph.node_yhigh(inode); j++)
                    chany_occ[layer][i][j]++;
            }
        }
    }
}

/**
 * @brief Writes channel occupancy data to a file.
 *
 * Each row contains:
 *   - (layer, x, y) coordinate
 *   - Occupancy count
 *   - Occupancy percentage (occupancy / capacity)
 *   - Channel capacity
 *
 * @param filename      Output file path.
 * @param occupancy     Matrix of occupancy counts.
 * @param capacity      Channel capacities.
 */
static void write_channel_occupancy_table(std::string_view filename,
                                          const vtr::NdMatrix<int, 3>& occupancy,
                                          const vtr::NdMatrix<int, 3>& capacity) {
    constexpr int w_coord = 6;
    constexpr int w_value = 12;
    constexpr int w_percent = 12;

    std::ofstream file(filename.data());
    if (!file.is_open()) {
        VTR_LOG_WARN("Failed to open %s for writing.\n", filename.data());
        return;
    }

    file << std::setw(w_coord) << "layer"
         << std::setw(w_coord) << "x"
         << std::setw(w_coord) << "y"
         << std::setw(w_value) << "occupancy"
         << std::setw(w_percent) << "%"
         << std::setw(w_value) << "capacity"
         << "\n";

    for (size_t layer = 0; layer < occupancy.dim_size(0); ++layer) {
        for (size_t x = 0; x < occupancy.dim_size(1); ++x) {
            for (size_t y = 0; y < occupancy.dim_size(2); ++y) {
                int occ = occupancy[layer][x][y];
                int cap = capacity[layer][x][y];
                float percent = (cap > 0) ? static_cast<float>(occ) / cap * 100.0f : 0.0f;

                file << std::setw(w_coord) << layer
                     << std::setw(w_coord) << x
                     << std::setw(w_coord) << y
                     << std::setw(w_value) << occ
                     << std::setw(w_percent) << std::fixed << std::setprecision(3) << percent
                     << std::setw(w_value) << cap
                     << "\n";
            }
        }
    }

    file.close();
}

/****************** Channel occupancy stats (public) ************************/

void get_channel_occupancy_stats(const Netlist<>& net_list) {
    const auto& device_ctx = g_vpr_ctx.device();

    auto chanx_occ = vtr::NdMatrix<int, 3>({{
                                               device_ctx.grid.get_num_layers(),
                                               device_ctx.grid.width(),     // Length of each x channel
                                               device_ctx.grid.height() - 1 // Total number of x channels. There is no CHANX above the top row.
                                           }},
                                           0);

    auto chany_occ = vtr::NdMatrix<int, 3>({{
                                               device_ctx.grid.get_num_layers(),
                                               device_ctx.grid.width() - 1, // Total number of y channels. There is no CHANY to the right of the most right column.
                                               device_ctx.grid.height()     // Length of each y channel.
                                           }},
                                           0);

    load_channel_occupancies(net_list, chanx_occ, chany_occ);

    write_channel_occupancy_table("chanx_occupancy.txt", chanx_occ, device_ctx.rr_chan_segment_width.x);
    write_channel_occupancy_table("chany_occupancy.txt", chany_occ, device_ctx.rr_chan_segment_width.y);

    int total_cap_x = 0;
    int total_used_x = 0;
    int total_cap_y = 0;
    int total_used_y = 0;

    VTR_LOG("\n");
    VTR_LOG("X - Directed channels: layer   y   max occ   ave occ   ave cap\n");
    VTR_LOG("                        ----- ---- -------- -------- --------\n");

    for (size_t layer = 0; layer < device_ctx.grid.get_num_layers(); ++layer) {
        for (size_t y = 0; y < device_ctx.grid.height() - 1; y++) {
            float ave_occ = 0.0f;
            float ave_cap = 0.0f;
            int max_occ = -1;

            // It is assumed that there is no CHANX at x=0
            for (size_t x = 1; x < device_ctx.grid.width(); x++) {
                max_occ = std::max(chanx_occ[layer][x][y], max_occ);
                ave_occ += chanx_occ[layer][x][y];
                ave_cap += device_ctx.rr_chan_segment_width.x[layer][x][y];

                total_cap_x += chanx_occ[layer][x][y];
                total_used_x += chanx_occ[layer][x][y];
            }
            ave_occ /= device_ctx.grid.width() - 2;
            ave_cap /= device_ctx.grid.width() - 2;
            VTR_LOG("                        %5zu %4zu %8d %8.3f %8.0f\n",
                    layer, y, max_occ, ave_occ, ave_cap);
        }
    }

    VTR_LOG("Y - Directed channels: layer   x   max occ   ave occ   ave cap\n");
    VTR_LOG("                        ----- ---- -------- -------- --------\n");

    for (size_t layer = 0; layer < device_ctx.grid.get_num_layers(); ++layer) {
        for (size_t x = 0; x < device_ctx.grid.width() - 1; x++) {
            float ave_occ = 0.0;
            float ave_cap = 0.0;
            int max_occ = -1;

            // It is assumed that there is no CHANY at y=0
            for (size_t y = 1; y < device_ctx.grid.height(); y++) {
                max_occ = std::max(chany_occ[layer][x][y], max_occ);
                ave_occ += chany_occ[layer][x][y];
                ave_cap += device_ctx.rr_chan_segment_width.y[layer][x][y];

                total_cap_y += chany_occ[layer][x][y];
                total_used_y += chany_occ[layer][x][y];
            }
            ave_occ /= device_ctx.grid.height() - 2;
            ave_cap /= device_ctx.grid.height() - 2;
            VTR_LOG("                        %5zu %4zu %8d %8.3f %8.0f\n",
                    layer, x, max_occ, ave_occ, ave_cap);
        }
    }

    VTR_LOG("\n");

    VTR_LOG("Total existing wires segments: CHANX %d, CHANY %d, ALL %d\n",
            total_cap_x, total_cap_y, total_cap_x + total_cap_y);
    VTR_LOG("Total used wires segments:     CHANX %d, CHANY %d, ALL %d\n",
            total_used_x, total_used_y, total_used_x + total_used_y);
    VTR_LOG("Usage percentage:               CHANX %d%%, CHANY %d%%, ALL %d%%\n",
            (float)total_used_x / total_cap_x, (float)total_used_y / total_cap_y, (float)(total_used_x + total_used_y) / (total_cap_x + total_cap_y));

    VTR_LOG("\n");
}

/****************** Bends and length (public) *******************************/

void get_num_bends_and_length(ParentNetId inet, int* bends_ptr, int* len_ptr, int* segments_ptr, bool* is_absorbed_ptr) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int bends = 0;
    int length = 0;
    int segments = 0;

    const vtr::optional<RouteTree>& tree = route_ctx.route_trees[inet];
    if (!tree) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "in get_num_bends_and_length: net #%lu has no routing.\n", size_t(inet));
    }

    e_rr_type prev_type = rr_graph.node_type(tree->root().inode);
    RouteTree::iterator it = tree->all_nodes().begin();
    RouteTree::iterator end = tree->all_nodes().end();
    ++it; /* start from the next node after source */

    for (; it != end; ++it) {
        const RouteTreeNode& rt_node = *it;
        RRNodeId inode = rt_node.inode;
        e_rr_type curr_type = rr_graph.node_type(inode);

        if (curr_type == e_rr_type::CHANX || curr_type == e_rr_type::CHANY) {
            segments++;
            length += rr_graph.node_length(inode);

            if (curr_type != prev_type && (prev_type == e_rr_type::CHANX || prev_type == e_rr_type::CHANY))
                bends++;
        }

        // The all_nodes iterator walks all nodes in the tree. If we are at a leaf and going back to the top, prev_type is invalid: just set it to SINK
        prev_type = rt_node.is_leaf() ? e_rr_type::SINK : curr_type;
    }

    *bends_ptr = bends;
    *len_ptr = length;
    *segments_ptr = segments;
    *is_absorbed_ptr = (segments == 0);
}

void length_and_bends_stats(const Netlist<>& net_list, bool is_flat) {
    int max_bends = 0;
    int total_bends = 0;
    int max_length = 0;
    int total_length = 0;
    int max_segments = 0;
    int total_segments = 0;
    int num_global_nets = 0;
    int num_clb_opins_reserved = 0;
    int num_absorbed_nets = 0;

    for (ParentNetId net_id : net_list.nets()) {
        if (!net_list.net_is_ignored(net_id) && net_list.net_sinks(net_id).size() != 0) { /* Globals don't count. */
            int bends, length, segments;
            bool is_absorbed;
            get_num_bends_and_length(net_id, &bends, &length, &segments, &is_absorbed);

            total_bends += bends;
            max_bends = std::max(bends, max_bends);

            total_length += length;
            max_length = std::max(length, max_length);

            total_segments += segments;
            max_segments = std::max(segments, max_segments);

            if (is_absorbed) {
                num_absorbed_nets++;
            }
        } else if (net_list.net_is_ignored(net_id)) {
            num_global_nets++;
        } else if (!is_flat) {
            /* If flat_routing is enabled, we don't need to count the number of reserved opins*/
            num_clb_opins_reserved++;
        }
    }

    float av_bends = (float)total_bends / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("\n");
    VTR_LOG("Average number of bends per net: %#g  Maximum # of bends: %d\n", av_bends, max_bends);
    VTR_LOG("\n");

    float av_length = (float)total_length / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Number of global nets: %d\n", num_global_nets);
    VTR_LOG("Number of routed nets (nonglobal): %d\n", (int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Wire length results (in units of 1 clb segments)...\n");
    VTR_LOG("\tTotal wirelength: %d, average net length: %#g\n", total_length, av_length);
    VTR_LOG("\tMaximum net length: %d\n", max_length);
    VTR_LOG("\n");

    float av_segments = (float)total_segments / (float)((int)net_list.nets().size() - num_global_nets);
    VTR_LOG("Wire length results in terms of physical segments...\n");
    VTR_LOG("\tTotal wiring segments used: %d, average wire segments per net: %#g\n", total_segments, av_segments);
    VTR_LOG("\tMaximum segments used by a net: %d\n", max_segments);
    VTR_LOG("\tTotal local nets with reserved CLB opins: %d\n", num_clb_opins_reserved);

    VTR_LOG("Total number of nets absorbed: %d\n", num_absorbed_nets);
}
