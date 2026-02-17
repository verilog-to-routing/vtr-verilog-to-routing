/**
 * @file
 * @brief Implementation of tap utilization statistics for routed wire segments.
 *
 * See tap_util_stats.h for an overview.
 */

#include "tap_util_stats.h"

#include <map>
#include <string>
#include <vector>

#include "globals.h"
#include "physical_types.h"
#include "route_tree.h"
#include "rr_graph_view.h"
#include "vtr_assert.h"
#include "vtr_log.h"

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
