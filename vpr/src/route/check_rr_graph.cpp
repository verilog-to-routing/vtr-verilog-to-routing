#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph.h"
#include "check_rr_graph.h"

/*********************** Subroutines local to this module *******************/

static bool rr_node_is_global_clb_ipin(int inode);

static void check_unbuffered_edges(int from_node);

static bool has_adjacent_channel(const t_rr_node& node, const DeviceGrid& grid);

static void check_rr_edge(int from_node, int from_edge, int to_node);

/************************ Subroutine definitions ****************************/

class node_edge_sorter {
  public:
    bool operator()(const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) const {
        return lhs.first < rhs.first;
    }

    bool operator()(const std::pair<int, int>& lhs, const int& rhs) const {
        return lhs.first < rhs;
    }

    bool operator()(const int& lhs, const std::pair<int, int>& rhs) const {
        return lhs < rhs.first;
    }

    bool operator()(const int& lhs, const int& rhs) const {
        return lhs < rhs;
    }
};

void check_rr_graph(const t_graph_type graph_type,
                    const DeviceGrid& grid,
                    const std::vector<t_physical_tile_type>& types) {
    e_route_type route_type = DETAILED;
    if (graph_type == GRAPH_GLOBAL) {
        route_type = GLOBAL;
    }

    auto& device_ctx = g_vpr_ctx.device();

    auto total_edges_to_node = std::vector<int>(device_ctx.rr_nodes.size());
    auto switch_types_from_current_to_node = std::vector<unsigned char>(device_ctx.rr_nodes.size());
    const int num_rr_switches = device_ctx.rr_switch_inf.size();

    std::vector<std::pair<int, int>> edges;

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        device_ctx.rr_nodes[inode].validate();

        /* Ignore any uninitialized rr_graph nodes */
        if ((device_ctx.rr_nodes[inode].type() == SOURCE)
            && (device_ctx.rr_nodes[inode].xlow() == 0) && (device_ctx.rr_nodes[inode].ylow() == 0)
            && (device_ctx.rr_nodes[inode].xhigh() == 0) && (device_ctx.rr_nodes[inode].yhigh() == 0)) {
            continue;
        }

        // Virtual clock network sink is special, ignore.
        if (device_ctx.virtual_clock_network_root_idx == int(inode)) {
            continue;
        }

        t_rr_type rr_type = device_ctx.rr_nodes[inode].type();
        int num_edges = device_ctx.rr_nodes[inode].num_edges();

        check_rr_node(inode, route_type, device_ctx);

        /* Check all the connectivity (edges, etc.) information.                    */
        edges.resize(0);
        edges.reserve(num_edges);

        for (int iedge = 0; iedge < num_edges; iedge++) {
            int to_node = device_ctx.rr_nodes[inode].edge_sink_node(iedge);

            if (to_node < 0 || to_node >= (int)device_ctx.rr_nodes.size()) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_graph: node %d has an edge %d.\n"
                                "\tEdge is out of range.\n",
                                inode, to_node);
            }

            check_rr_edge(inode, iedge, to_node);

            edges.emplace_back(to_node, iedge);
            total_edges_to_node[to_node]++;

            auto switch_type = device_ctx.rr_nodes[inode].edge_switch(iedge);

            if (switch_type < 0 || switch_type >= num_rr_switches) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_graph: node %d has a switch type %d.\n"
                                "\tSwitch type is out of range.\n",
                                inode, switch_type);
            }
        } /* End for all edges of node. */

        std::sort(edges.begin(), edges.end(), [](const std::pair<int, int>& lhs, const std::pair<int, int>& rhs) {
            return lhs.first < rhs.first;
        });

        //Check that multiple edges between the same from/to nodes make sense
        for (int iedge = 0; iedge < num_edges; iedge++) {
            int to_node = device_ctx.rr_nodes[inode].edge_sink_node(iedge);

            auto range = std::equal_range(edges.begin(), edges.end(),
                                          to_node, node_edge_sorter());

            size_t num_edges_to_node = std::distance(range.first, range.second);

            if (num_edges_to_node == 1) continue; //Single edges are always OK

            VTR_ASSERT_MSG(num_edges_to_node > 1, "Expect multiple edges");

            t_rr_type to_rr_type = device_ctx.rr_nodes[to_node].type();

            /* Only expect the following cases to have multiple edges
             * - chan <-> chan connections 
             * - IPIN <-> chan connections (unique rr_node for IPIN nodes on multiple sides)
             * - OPIN <-> chan connections (unique rr_node for OPIN nodes on multiple sides)
             */
            if (((to_rr_type != CHANX && to_rr_type != CHANY && rr_type != IPIN)
                 || (rr_type != CHANX && rr_type != CHANY))
                && ((to_rr_type != CHANX && to_rr_type != CHANY)
                    || (rr_type != CHANX && rr_type != CHANY && rr_type != OPIN))) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_graph: node %d (%s) connects to node %d (%s) %zu times - multi-connections only expected for CHAN->CHAN.\n",
                          inode, rr_node_typename[rr_type], to_node, rr_node_typename[to_rr_type], num_edges_to_node);
            }

            //Between two wire segments
            VTR_ASSERT_MSG(to_rr_type == CHANX || to_rr_type == CHANY || to_rr_type == IPIN, "Expect channel type or input pin type");
            VTR_ASSERT_MSG(rr_type == CHANX || rr_type == CHANY || rr_type == OPIN, "Expect channel type or output pin type");

            //While multiple connections between the same wires can be electrically legal,
            //they are redundant if they are of the same switch type.
            //
            //Identify any such edges with identical switches
            std::map<short, int> switch_counts;
            for (const auto& to_edge : vtr::Range<decltype(edges)::const_iterator>(range.first, range.second)) {
                auto edge = to_edge.second;
                auto edge_switch = device_ctx.rr_nodes[inode].edge_switch(edge);

                switch_counts[edge_switch]++;
            }

            //Tell the user about any redundant edges
            for (auto kv : switch_counts) {
                if (kv.second <= 1) continue;

                /* Redundant edges are not allowed for chan <-> chan connections
                 * but allowed for input pin <-> chan or output pin <-> chan connections 
                 */
                if ((to_rr_type == CHANX || to_rr_type == CHANY)
                    && (rr_type == CHANX || rr_type == CHANY)) {
                    auto switch_type = device_ctx.rr_switch_inf[kv.first].type();

                    VPR_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d has %d redundant connections to node %d of switch type %d (%s)",
                              inode, kv.second, to_node, kv.first, SWITCH_TYPE_STRINGS[size_t(switch_type)]);
                }
            }
        }

        /* Slow test could leave commented out most of the time. */
        check_unbuffered_edges(inode);

        //Check that all config/non-config edges are appropriately organized
        for (auto edge : device_ctx.rr_nodes[inode].configurable_edges()) {
            if (!device_ctx.rr_nodes[inode].edge_is_configurable(edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d edge %d is non-configurable, but in configurable edges",
                                inode, edge);
            }
        }

        for (auto edge : device_ctx.rr_nodes[inode].non_configurable_edges()) {
            if (device_ctx.rr_nodes[inode].edge_is_configurable(edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d edge %d is configurable, but in non-configurable edges",
                                inode, edge);
            }
        }

    } /* End for all rr_nodes */

    /* I built a list of how many edges went to everything in the code above -- *
     * now I check that everything is reachable.                                */
    bool is_fringe_warning_sent = false;

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        t_rr_type rr_type = device_ctx.rr_nodes[inode].type();

        if (rr_type != SOURCE) {
            if (total_edges_to_node[inode] < 1 && !rr_node_is_global_clb_ipin(inode)) {
                /* A global CLB input pin will not have any edges, and neither will  *
                 * a SOURCE or the start of a carry-chain.  Anything else is an error.
                 * For simplicity, carry-chain input pin are entirely ignored in this test
                 */
                bool is_chain = false;
                if (rr_type == IPIN) {
                    t_physical_tile_type_ptr type = device_ctx.grid[device_ctx.rr_nodes[inode].xlow()][device_ctx.rr_nodes[inode].ylow()].type;
                    for (const t_fc_specification& fc_spec : types[type->index].fc_specs) {
                        if (fc_spec.fc_value == 0 && fc_spec.seg_index == 0) {
                            is_chain = true;
                        }
                    }
                }

                const auto& node = device_ctx.rr_nodes[inode];

                bool is_fringe = ((device_ctx.rr_nodes[inode].xlow() == 1)
                                  || (device_ctx.rr_nodes[inode].ylow() == 1)
                                  || (device_ctx.rr_nodes[inode].xhigh() == int(grid.width()) - 2)
                                  || (device_ctx.rr_nodes[inode].yhigh() == int(grid.height()) - 2));
                bool is_wire = (device_ctx.rr_nodes[inode].type() == CHANX
                                || device_ctx.rr_nodes[inode].type() == CHANY);

                if (!is_chain && !is_fringe && !is_wire) {
                    if (node.type() == IPIN || node.type() == OPIN) {
                        if (has_adjacent_channel(node, device_ctx.grid)) {
                            auto block_type = device_ctx.grid[node.xlow()][node.ylow()].type;
                            std::string pin_name = block_type_pin_index_to_name(block_type, node.pin_num());
                            VTR_LOG_ERROR("in check_rr_graph: node %d (%s) at (%d,%d) block=%s side=%s pin=%s has no fanin.\n",
                                          inode, node.type_string(), node.xlow(), node.ylow(), block_type->name, node.side_string(), pin_name.c_str());
                        }
                    } else {
                        VTR_LOG_ERROR("in check_rr_graph: node %d (%s) has no fanin.\n",
                                      inode, device_ctx.rr_nodes[inode].type_string());
                    }
                } else if (!is_chain && !is_fringe_warning_sent) {
                    VTR_LOG_WARN(
                        "in check_rr_graph: fringe node %d %s at (%d,%d) has no fanin.\n"
                        "\t This is possible on a fringe node based on low Fc_out, N, and certain lengths.\n",
                        inode, device_ctx.rr_nodes[inode].type_string(), device_ctx.rr_nodes[inode].xlow(), device_ctx.rr_nodes[inode].ylow());
                    is_fringe_warning_sent = true;
                }
            }
        } else { /* SOURCE.  No fanin for now; change if feedthroughs allowed. */
            if (total_edges_to_node[inode] != 0) {
                VTR_LOG_ERROR("in check_rr_graph: SOURCE node %d has a fanin of %d, expected 0.\n",
                              inode, total_edges_to_node[inode]);
            }
        }
    }
}

static bool rr_node_is_global_clb_ipin(int inode) {
    /* Returns true if inode refers to a global CLB input pin node.   */

    int ipin;
    t_physical_tile_type_ptr type;

    auto& device_ctx = g_vpr_ctx.device();

    type = device_ctx.grid[device_ctx.rr_nodes[inode].xlow()][device_ctx.rr_nodes[inode].ylow()].type;

    if (device_ctx.rr_nodes[inode].type() != IPIN)
        return (false);

    ipin = device_ctx.rr_nodes[inode].ptc_num();

    return type->is_ignored_pin[ipin];
}

void check_rr_node(int inode, enum e_route_type route_type, const DeviceContext& device_ctx) {
    /* This routine checks that the rr_node is inside the grid and has a valid
     * pin number, etc.
     */

    int xlow, ylow, xhigh, yhigh, ptc_num, capacity;
    t_rr_type rr_type;
    t_physical_tile_type_ptr type;
    int nodes_per_chan, tracks_per_node, num_edges, cost_index;
    float C, R;

    rr_type = device_ctx.rr_nodes[inode].type();
    xlow = device_ctx.rr_nodes[inode].xlow();
    xhigh = device_ctx.rr_nodes[inode].xhigh();
    ylow = device_ctx.rr_nodes[inode].ylow();
    yhigh = device_ctx.rr_nodes[inode].yhigh();
    ptc_num = device_ctx.rr_nodes[inode].ptc_num();
    capacity = device_ctx.rr_nodes[inode].capacity();
    cost_index = device_ctx.rr_nodes[inode].cost_index();
    type = nullptr;

    const auto& grid = device_ctx.grid;
    if (xlow > xhigh || ylow > yhigh) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "in check_rr_node: rr endpoints are (%d,%d) and (%d,%d).\n", xlow, ylow, xhigh, yhigh);
    }

    if (xlow < 0 || xhigh > int(grid.width()) - 1 || ylow < 0 || yhigh > int(grid.height()) - 1) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_rr_node: rr endpoints (%d,%d) and (%d,%d) are out of range.\n", xlow, ylow, xhigh, yhigh);
    }

    if (ptc_num < 0) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
    }

    if (cost_index < 0 || cost_index >= (int)device_ctx.rr_indexed_data.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_rr_node: node %d cost index (%d) is out of range.\n", inode, cost_index);
    }

    /* Check that the segment is within the array and such. */
    type = device_ctx.grid[xlow][ylow].type;

    switch (rr_type) {
        case SOURCE:
        case SINK:
            if (type == nullptr) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: node %d (type %d) is at an illegal clb location (%d, %d).\n", inode, rr_type, xlow, ylow);
            }
            if (xlow != (xhigh - type->width + 1) || ylow != (yhigh - type->height + 1)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: node %d (type %d) has endpoints (%d,%d) and (%d,%d)\n", inode, rr_type, xlow, ylow, xhigh, yhigh);
            }
            break;
        case IPIN:
        case OPIN:
            if (type == nullptr) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: node %d (type %d) is at an illegal clb location (%d, %d).\n", inode, rr_type, xlow, ylow);
            }
            if (xlow != xhigh || ylow != yhigh) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: node %d (type %d) has endpoints (%d,%d) and (%d,%d)\n", inode, rr_type, xlow, ylow, xhigh, yhigh);
            }
            break;

        case CHANX:
            if (xlow < 1 || xhigh > int(grid.width()) - 2 || yhigh > int(grid.height()) - 2 || yhigh != ylow) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: CHANX out of range for endpoints (%d,%d) and (%d,%d)\n", xlow, ylow, xhigh, yhigh);
            }
            if (route_type == GLOBAL && xlow != xhigh) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: node %d spans multiple channel segments (not allowed for global routing).\n", inode);
            }
            break;

        case CHANY:
            if (xhigh > int(grid.width()) - 2 || ylow < 1 || yhigh > int(grid.height()) - 2 || xlow != xhigh) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "Error in check_rr_node: CHANY out of range for endpoints (%d,%d) and (%d,%d)\n", xlow, ylow, xhigh, yhigh);
            }
            if (route_type == GLOBAL && ylow != yhigh) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: node %d spans multiple channel segments (not allowed for global routing).\n", inode);
            }
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_rr_node: Unexpected segment type: %d\n", rr_type);
    }

    /* Check that it's capacities and such make sense. */

    switch (rr_type) {
        case SOURCE:
            if (ptc_num >= (int)type->class_inf.size()
                || type->class_inf[ptc_num].type != DRIVER) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (type->class_inf[ptc_num].num_pins != capacity) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) had a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case SINK:
            if (ptc_num >= (int)type->class_inf.size()
                || type->class_inf[ptc_num].type != RECEIVER) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (type->class_inf[ptc_num].num_pins != capacity) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case OPIN:
            if (ptc_num >= type->num_pins
                || type->class_inf[type->pin_class[ptc_num]].type != DRIVER) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (capacity != 1) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case IPIN:
            if (ptc_num >= type->num_pins
                || type->class_inf[type->pin_class[ptc_num]].type != RECEIVER) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (capacity != 1) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case CHANX:
            if (route_type == DETAILED) {
                nodes_per_chan = device_ctx.chan_width.max;
                tracks_per_node = 1;
            } else {
                nodes_per_chan = 1;
                tracks_per_node = device_ctx.chan_width.x_list[ylow];
            }

            if (ptc_num >= nodes_per_chan) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) has a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }

            if (capacity != tracks_per_node) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case CHANY:
            if (route_type == DETAILED) {
                nodes_per_chan = device_ctx.chan_width.max;
                tracks_per_node = 1;
            } else {
                nodes_per_chan = 1;
                tracks_per_node = device_ctx.chan_width.y_list[xlow];
            }

            if (ptc_num >= nodes_per_chan) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) has a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }

            if (capacity != tracks_per_node) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_rr_node: Unexpected segment type: %d\n", rr_type);
    }

    /* Check that the number of (out) edges is reasonable. */
    num_edges = device_ctx.rr_nodes[inode].num_edges();

    if (rr_type != SINK && rr_type != IPIN) {
        if (num_edges <= 0) {
            /* Just a warning, since a very poorly routable rr-graph could have nodes with no edges.  *
             * If such a node was ever used in a final routing (not just in an rr_graph), other       *
             * error checks in check_routing will catch it.                                           */

            //Don't worry about disconnect PINs which have no adjacent channels (i.e. on the device perimeter)
            bool check_for_out_edges = true;
            if (rr_type == IPIN || rr_type == OPIN) {
                if (!has_adjacent_channel(device_ctx.rr_nodes[inode], device_ctx.grid)) {
                    check_for_out_edges = false;
                }
            }

            if (check_for_out_edges) {
                std::string info = describe_rr_node(inode);
                VTR_LOG_WARN("in check_rr_node: %s has no out-going edges.\n", info.c_str());
            }
        }
    } else if (rr_type == SINK) { /* SINK -- remove this check if feedthroughs allowed */
        if (num_edges != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_rr_node: node %d is a sink, but has %d edges.\n", inode, num_edges);
        }
    }

    /* Check that the capacitance and resistance are reasonable. */
    C = device_ctx.rr_nodes[inode].C();
    R = device_ctx.rr_nodes[inode].R();

    if (rr_type == CHANX || rr_type == CHANY) {
        if (C < 0. || R < 0.) {
            VPR_ERROR(VPR_ERROR_ROUTE,
                      "in check_rr_node: node %d of type %d has R = %g and C = %g.\n", inode, rr_type, R, C);
        }
    } else {
        if (C != 0. || R != 0.) {
            VPR_ERROR(VPR_ERROR_ROUTE,
                      "in check_rr_node: node %d of type %d has R = %g and C = %g.\n", inode, rr_type, R, C);
        }
    }
}

static void check_unbuffered_edges(int from_node) {
    /* This routine checks that all pass transistors in the routing truly are  *
     * bidirectional.  It may be a slow check, so don't use it all the time.   */

    int from_edge, to_node, to_edge, from_num_edges, to_num_edges;
    t_rr_type from_rr_type, to_rr_type;
    short from_switch_type;
    bool trans_matched;

    auto& device_ctx = g_vpr_ctx.device();

    from_rr_type = device_ctx.rr_nodes[from_node].type();
    if (from_rr_type != CHANX && from_rr_type != CHANY)
        return;

    from_num_edges = device_ctx.rr_nodes[from_node].num_edges();

    for (from_edge = 0; from_edge < from_num_edges; from_edge++) {
        to_node = device_ctx.rr_nodes[from_node].edge_sink_node(from_edge);
        to_rr_type = device_ctx.rr_nodes[to_node].type();

        if (to_rr_type != CHANX && to_rr_type != CHANY)
            continue;

        from_switch_type = device_ctx.rr_nodes[from_node].edge_switch(from_edge);

        if (device_ctx.rr_switch_inf[from_switch_type].buffered())
            continue;

        /* We know that we have a pass transistor from from_node to to_node. Now *
         * check that there is a corresponding edge from to_node back to         *
         * from_node.                                                            */

        to_num_edges = device_ctx.rr_nodes[to_node].num_edges();
        trans_matched = false;

        for (to_edge = 0; to_edge < to_num_edges; to_edge++) {
            if (device_ctx.rr_nodes[to_node].edge_sink_node(to_edge) == from_node
                && device_ctx.rr_nodes[to_node].edge_switch(to_edge) == from_switch_type) {
                trans_matched = true;
                break;
            }
        }

        if (trans_matched == false) {
            VPR_ERROR(VPR_ERROR_ROUTE,
                      "in check_unbuffered_edges:\n"
                      "connection from node %d to node %d uses an unbuffered switch (switch type %d '%s')\n"
                      "but there is no corresponding unbuffered switch edge in the other direction.\n",
                      from_node, to_node, from_switch_type, device_ctx.rr_switch_inf[from_switch_type].name);
        }

    } /* End for all from_node edges */
}

static bool has_adjacent_channel(const t_rr_node& node, const DeviceGrid& grid) {
    VTR_ASSERT(node.type() == IPIN || node.type() == OPIN);

    if ((node.xlow() == 0 && node.side() != RIGHT)                          //left device edge connects only along block's right side
        || (node.ylow() == int(grid.height() - 1) && node.side() != BOTTOM) //top device edge connects only along block's bottom side
        || (node.xlow() == int(grid.width() - 1) && node.side() != LEFT)    //right deivce edge connects only along block's left side
        || (node.ylow() == 0 && node.side() != TOP)                         //bottom deivce edge connects only along block's top side
    ) {
        return false;
    }
    return true; //All other blocks will be surrounded on all sides by channels
}

static void check_rr_edge(int from_node, int iedge, int to_node) {
    auto& device_ctx = g_vpr_ctx.device();

    //Check that to to_node's fan-in is correct, given the switch type
    int iswitch = device_ctx.rr_nodes[from_node].edge_switch(iedge);
    auto switch_type = device_ctx.rr_switch_inf[iswitch].type();

    int to_fanin = device_ctx.rr_nodes[to_node].fan_in();
    switch (switch_type) {
        case SwitchType::BUFFER:
            //Buffer switches are non-configurable, and uni-directional -- they must have only one driver
            if (to_fanin != 1) {
                std::string msg = "Non-configurable BUFFER type switch must have only one driver. ";
                msg += vtr::string_fmt(" Actual fan-in was %d (expected 1).\n", to_fanin);
                msg += "  Possible cause is complex block output pins connecting to:\n";
                msg += "    " + describe_rr_node(to_node);

                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
            }
            break;
        case SwitchType::TRISTATE:  //Fallthrough
        case SwitchType::MUX:       //Fallthrough
        case SwitchType::PASS_GATE: //Fallthrough
        case SwitchType::SHORT:     //Fallthrough
            break;                  //pass
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid switch type %d", switch_type);
    }
}
