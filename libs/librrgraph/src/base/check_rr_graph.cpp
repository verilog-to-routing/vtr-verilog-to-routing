#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "vpr_error.h"
#include "check_rr_graph.h"

#include "rr_node.h"
#include "physical_types_util.h"

#include "describe_rr_node.h"

/*********************** Subroutines local to this module *******************/

static bool rr_node_is_global_clb_ipin(const RRGraphView& rr_graph, const DeviceGrid& grid, RRNodeId inode);

static void check_unbuffered_edges(const RRGraphView& rr_graph, int from_node);

static bool has_adjacent_channel(const RRGraphView& rr_graph, const DeviceGrid& grid, const t_rr_node& node);

static void check_rr_edge(const RRGraphView& rr_graph,
                          const DeviceGrid& grid,
                          const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                          int from_node,
                          int from_edge,
                          int to_node,
                          bool is_flat);

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

void check_rr_graph(const RRGraphView& rr_graph,
                    const std::vector<t_physical_tile_type>& types,
                    const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                    const DeviceGrid& grid,
                    const t_chan_width& chan_width,
                    const t_graph_type graph_type,
                    const int virtual_clock_network_root_idx,
                    bool is_flat) {
    e_route_type route_type = DETAILED;
    if (graph_type == GRAPH_GLOBAL) {
        route_type = GLOBAL;
    }

    auto total_edges_to_node = std::vector<int>(rr_graph.num_nodes());
    auto switch_types_from_current_to_node = std::vector<unsigned char>(rr_graph.num_nodes());
    const int num_rr_switches = rr_graph.num_rr_switches();

    std::vector<std::pair<int, int>> edges;

    for (const RRNodeId& rr_node : rr_graph.nodes()) {
        size_t inode = (size_t)rr_node;
        rr_graph.validate_node(rr_node);

        /* Ignore any uninitialized rr_graph nodes */
        if (!rr_graph.node_is_initialized(rr_node)) {
            continue;
        }

        // Virtual clock network sink is special, ignore.
        if (virtual_clock_network_root_idx == int(inode)) {
            continue;
        }

        t_rr_type rr_type = rr_graph.node_type(rr_node);
        int num_edges = rr_graph.num_edges(RRNodeId(inode));

        check_rr_node(rr_graph, rr_indexed_data, grid, chan_width, route_type, inode, is_flat);

        /* Check all the connectivity (edges, etc.) information.                    */
        edges.resize(0);
        edges.reserve(num_edges);

        for (int iedge = 0; iedge < num_edges; iedge++) {
            int to_node = size_t(rr_graph.edge_sink_node(rr_node, iedge));

            if (to_node < 0 || to_node >= (int)rr_graph.num_nodes()) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_graph: node %d has an edge %d.\n"
                                "\tEdge is out of range.\n",
                                inode, to_node);
            }

            check_rr_edge(rr_graph,
                          grid,
                          rr_indexed_data,
                          inode,
                          iedge,
                          to_node,
                          is_flat);

            edges.emplace_back(to_node, iedge);
            total_edges_to_node[to_node]++;

            auto switch_type = rr_graph.edge_switch(rr_node, iedge);

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
            int to_node = size_t(rr_graph.edge_sink_node(rr_node, iedge));

            auto range = std::equal_range(edges.begin(), edges.end(),
                                          to_node, node_edge_sorter());

            size_t num_edges_to_node = std::distance(range.first, range.second);

            if (num_edges_to_node == 1) continue; //Single edges are always OK

            VTR_ASSERT_MSG(num_edges_to_node > 1, "Expect multiple edges");

            t_rr_type to_rr_type = rr_graph.node_type(RRNodeId(to_node));

            /* It is unusual to have more than one programmable switch (in the same direction) between a from_node and a to_node,
             * as the duplicate switch doesn't add more routing flexibility.
             *
             * However, such duplicate switches can occur for some types of nodes, which we allow below.
             * Reasons one could have duplicate switches between two nodes include:
             *      - The two switches have different electrical characteristics.
             *      - Wires near the edges of an FPGA are often cut off, and the stubs connected together.
             *        A regular switch pattern could then result in one physical wire connecting multiple
             *        times to other wires, IPINs or OPINs.
             *
             * Only expect the following cases to have multiple edges
             * - CHAN <-> CHAN connections
             * - CHAN  -> IPIN connections (unique rr_node for IPIN nodes on multiple sides)
             * - OPIN  -> CHAN connections (unique rr_node for OPIN nodes on multiple sides)
             */
            bool is_chan_to_chan = (rr_type == CHANX || rr_type == CHANY) && (to_rr_type == CHANY || to_rr_type == CHANX);
            bool is_chan_to_ipin = (rr_type == CHANX || rr_type == CHANY) && to_rr_type == IPIN;
            bool is_opin_to_chan = rr_type == OPIN && (to_rr_type == CHANX || to_rr_type == CHANY);
            bool is_internal_edge = false;
            if (is_flat) {
                is_internal_edge = (rr_type == IPIN && to_rr_type == IPIN) || (rr_type == OPIN && to_rr_type == OPIN);
            }
            if (!(is_chan_to_chan || is_chan_to_ipin || is_opin_to_chan || is_internal_edge)) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_graph: node %d (%s) connects to node %d (%s) %zu times - multi-connections only expected for CHAN<->CHAN, CHAN->IPIN, OPIN->CHAN.\n",
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
                auto edge_switch = rr_graph.edge_switch(rr_node, edge);

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
                    auto switch_type = rr_graph.rr_switch_inf(RRSwitchId(kv.first)).type();

                    VPR_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d has %d redundant connections to node %d of switch type %d (%s)",
                              inode, kv.second, to_node, kv.first, SWITCH_TYPE_STRINGS[size_t(switch_type)]);
                }
            }
        }

        /* Slow test could leave commented out most of the time. */
        check_unbuffered_edges(rr_graph, inode);

        //Check that all config/non-config edges are appropriately organized
        for (auto edge : rr_graph.configurable_edges(RRNodeId(inode))) {
            if (!rr_graph.edge_is_configurable(RRNodeId(inode), edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d edge %d is non-configurable, but in configurable edges",
                                inode, edge);
            }
        }

        for (auto edge : rr_graph.non_configurable_edges(RRNodeId(inode))) {
            if (rr_graph.edge_is_configurable(RRNodeId(inode), edge)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "in check_rr_graph: node %d edge %d is configurable, but in non-configurable edges",
                                inode, edge);
            }
        }

    } /* End for all rr_nodes */

    // AM: For the time being, if is_flat is enabled, we don't have proper tests to check whether a node should have an incoming
    // edge or not
    if(is_flat) {
        return;
    }

    /* I built a list of how many edges went to everything in the code above -- *
     * now I check that everything is reachable.                                */
    bool is_fringe_warning_sent = false;

    for (const RRNodeId& rr_node : rr_graph.nodes()) {
        size_t inode = (size_t)rr_node;
        t_rr_type rr_type = rr_graph.node_type(rr_node);
        int ptc_num = rr_graph.node_ptc_num(rr_node);
        int layer_num = rr_graph.node_layer(rr_node);
        int xlow = rr_graph.node_xlow(rr_node);
        int ylow = rr_graph.node_ylow(rr_node);

        t_physical_tile_type_ptr type = grid.get_physical_type({xlow, ylow, layer_num});

        if (rr_type == IPIN || rr_type == OPIN) {
            // #TODO: No edges are added for internal pins. However, they need to be checked somehow!
            if (ptc_num >= type->num_pins) {
                VTR_LOG_ERROR("in check_rr_graph: node %d (%s) type: %s is internal node.\n",
                              inode, rr_graph.node_type_string(rr_node), rr_node_typename[rr_type]);
            }
        }

        if (rr_type != SOURCE) {
            if (total_edges_to_node[inode] < 1 && !rr_node_is_global_clb_ipin(rr_graph, grid, rr_node)) {
                /* A global CLB input pin will not have any edges, and neither will  *
                 * a SOURCE or the start of a carry-chain.  Anything else is an error.
                 * For simplicity, carry-chain input pin are entirely ignored in this test
                 */
                bool is_chain = false;
                if (rr_type == IPIN) {
                    for (const t_fc_specification& fc_spec : types[type->index].fc_specs) {
                        if (fc_spec.fc_value == 0 && fc_spec.seg_index == 0) {
                            is_chain = true;
                        }
                    }
                }

                const auto& node = rr_graph.rr_nodes()[inode];

                bool is_fringe = ((rr_graph.node_xlow(rr_node) == 1)
                                  || (rr_graph.node_ylow(rr_node) == 1)
                                  || (rr_graph.node_xhigh(rr_node) == int(grid.width()) - 2)
                                  || (rr_graph.node_yhigh(rr_node) == int(grid.height()) - 2));
                bool is_wire = (rr_graph.node_type(rr_node) == CHANX
                                || rr_graph.node_type(rr_node) == CHANY);

                if (!is_chain && !is_fringe && !is_wire) {
                    if (rr_graph.node_type(rr_node) == IPIN || rr_graph.node_type(rr_node) == OPIN) {
                        if (has_adjacent_channel(rr_graph, grid, node)) {
                            auto block_type = grid.get_physical_type({rr_graph.node_xlow(rr_node),
                                                                      rr_graph.node_ylow(rr_node),
                                                                      rr_graph.node_layer(rr_node)});
                            std::string pin_name = block_type_pin_index_to_name(block_type, rr_graph.node_pin_num(rr_node), is_flat);
                            /* Print error messages for all the sides that a node may appear */
                            for (const e_side& node_side : SIDES) {
                                if (!rr_graph.is_node_on_specific_side(rr_node, node_side)) {
                                    continue;
                                }
                                VTR_LOG_ERROR("in check_rr_graph: node %d (%s) at (%d,%d) block=%s side=%s pin=%s has no fanin.\n",
                                              inode, rr_graph.node_type_string(rr_node), rr_graph.node_xlow(rr_node), rr_graph.node_ylow(rr_node), block_type->name, SIDE_STRING[node_side], pin_name.c_str());
                            }
                        }
                    } else {
                        VTR_LOG_ERROR("in check_rr_graph: node %d (%s) has no fanin.\n",
                                      inode, rr_graph.node_type_string(rr_node));
                    }
                } else if (!is_chain && !is_fringe_warning_sent) {
                    VTR_LOG_WARN(
                        "in check_rr_graph: fringe node %d %s at (%d,%d) has no fanin.\n"
                        "\t This is possible on a fringe node based on low Fc_out, N, and certain lengths.\n",
                        inode, rr_graph.node_type_string(rr_node), rr_graph.node_xlow(rr_node), rr_graph.node_ylow(rr_node));
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

static bool rr_node_is_global_clb_ipin(const RRGraphView& rr_graph, const DeviceGrid& grid, RRNodeId inode) {
    /* Returns true if inode refers to a global CLB input pin node.   */

    int ipin;
    t_physical_tile_type_ptr type;

    type = grid.get_physical_type({rr_graph.node_xlow(inode),
                                   rr_graph.node_ylow(inode),
                                   rr_graph.node_layer(inode)});

    if (rr_graph.node_type(inode) != IPIN)
        return (false);

    ipin = rr_graph.node_pin_num(inode);

    return type->is_ignored_pin[ipin];
}

void check_rr_node(const RRGraphView& rr_graph,
                   const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                   const DeviceGrid& grid,
                   const t_chan_width& chan_width,
                   const enum e_route_type route_type, 
                   const int inode,
                   bool is_flat) {
    /* This routine checks that the rr_node is inside the grid and has a valid
     * pin number, etc.
     */

    //Make sure over-flow doesn't happen
    VTR_ASSERT(inode >= 0);
    int xlow, ylow, xhigh, yhigh, layer_num, ptc_num, capacity;
    t_rr_type rr_type;
    t_physical_tile_type_ptr type;
    int nodes_per_chan, tracks_per_node;
    RRIndexedDataId cost_index;
    float C, R;
    RRNodeId rr_node = RRNodeId(inode);

    rr_type = rr_graph.node_type(rr_node);
    xlow = rr_graph.node_xlow(rr_node);
    xhigh = rr_graph.node_xhigh(rr_node);
    ylow = rr_graph.node_ylow(rr_node);
    yhigh = rr_graph.node_yhigh(rr_node);
    layer_num = rr_graph.node_layer(rr_node);
    ptc_num = rr_graph.node_ptc_num(rr_node);
    capacity = rr_graph.node_capacity(rr_node);
    cost_index = rr_graph.node_cost_index(rr_node);
    type = nullptr;

    if (xlow > xhigh || ylow > yhigh) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "in check_rr_node: rr endpoints are (%d,%d) and (%d,%d).\n", xlow, ylow, xhigh, yhigh);
    }

    if (xlow < 0 || xhigh > int(grid.width()) - 1 || ylow < 0 || yhigh > int(grid.height()) - 1) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_rr_node: rr endpoints (%d,%d) and (%d,%d) are out of range.\n", xlow, ylow, xhigh, yhigh);
    }

    if (layer_num < 0 || layer_num > int(grid.get_num_layers()) - 1) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_rr_node: rr endpoints layer_num (%d) is out of range.\n", layer_num);
    }

    if (ptc_num < 0) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
    }

    if (!cost_index || (size_t)cost_index >= (size_t)rr_indexed_data.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_rr_node: node %d cost index (%d) is out of range.\n", inode, cost_index);
    }

    /* Check that the segment is within the array and such. */
    type = grid.get_physical_type({xlow, ylow, layer_num});

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

    int class_max_ptc = get_tile_class_max_ptc(type, is_flat);
    int pin_max_ptc = get_tile_pin_max_ptc(type, is_flat);
    e_pin_type class_type = OPEN;
    int class_num_pins = -1;
    switch (rr_type) {
        case SOURCE:
        case SINK:
            class_type = get_class_type_from_class_physical_num(type, ptc_num);
            class_num_pins = get_class_num_pins_from_class_physical_num(type, ptc_num);
            if (ptc_num >= class_max_ptc
                || class_type != ((rr_type == SOURCE) ? DRIVER : RECEIVER)) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (class_num_pins != capacity) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) had a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case OPIN:
        case IPIN:
            class_type = get_pin_type_from_pin_physical_num(type, ptc_num);
            if (ptc_num >= pin_max_ptc
                || class_type != ((rr_type == OPIN) ? DRIVER : RECEIVER)) {
                VPR_ERROR(VPR_ERROR_ROUTE,
                          "in check_rr_node: inode %d (type %d) had a ptc_num of %d.\n", inode, rr_type, ptc_num);
            }
            if (capacity != 1) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_rr_node: inode %d (type %d) has a capacity of %d.\n", inode, rr_type, capacity);
            }
            break;

        case CHANX:
        case CHANY:
            if (route_type == DETAILED) {
                nodes_per_chan = chan_width.max;
                tracks_per_node = 1;
            } else {
                nodes_per_chan = 1;
                tracks_per_node = ((rr_type == CHANX) ? chan_width.x_list[ylow] : chan_width.y_list[xlow]);
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

    /* Check that the capacitance and resistance are reasonable. */
    C = rr_graph.node_C(rr_node);
    R = rr_graph.node_R(rr_node);

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

static void check_unbuffered_edges(const RRGraphView& rr_graph, int from_node) {
    /* This routine checks that all pass transistors in the routing truly are  *
     * bidirectional.  It may be a slow check, so don't use it all the time.   */

    int from_edge, to_node, to_edge, from_num_edges, to_num_edges;
    t_rr_type from_rr_type, to_rr_type;
    short from_switch_type;
    bool trans_matched;

    from_rr_type = rr_graph.node_type(RRNodeId(from_node));
    if (from_rr_type != CHANX && from_rr_type != CHANY)
        return;

    from_num_edges = rr_graph.num_edges(RRNodeId(from_node));

    for (from_edge = 0; from_edge < from_num_edges; from_edge++) {
        to_node = size_t(rr_graph.edge_sink_node(RRNodeId(from_node), from_edge));
        to_rr_type = rr_graph.node_type(RRNodeId(to_node));

        if (to_rr_type != CHANX && to_rr_type != CHANY)
            continue;

        from_switch_type = rr_graph.edge_switch(RRNodeId(from_node), from_edge);

        if (rr_graph.rr_switch_inf(RRSwitchId(from_switch_type)).buffered())
            continue;

        /* We know that we have a pass transistor from from_node to to_node. Now *
         * check that there is a corresponding edge from to_node back to         *
         * from_node.                                                            */

        to_num_edges = rr_graph.num_edges(RRNodeId(to_node));
        trans_matched = false;

        for (to_edge = 0; to_edge < to_num_edges; to_edge++) {
            if (size_t(rr_graph.edge_sink_node(RRNodeId(to_node), to_edge)) == size_t(from_node)
                && rr_graph.edge_switch(RRNodeId(to_node), to_edge) == from_switch_type) {
                trans_matched = true;
                break;
            }
        }

        if (trans_matched == false) {
            VPR_ERROR(VPR_ERROR_ROUTE,
                      "in check_unbuffered_edges:\n"
                      "connection from node %d to node %d uses an unbuffered switch (switch type %d '%s')\n"
                      "but there is no corresponding unbuffered switch edge in the other direction.\n",
                      from_node, to_node, from_switch_type, rr_graph.rr_switch_inf(RRSwitchId(from_switch_type)).name.c_str());
        }

    } /* End for all from_node edges */
}

static bool has_adjacent_channel(const RRGraphView& rr_graph, const DeviceGrid& grid, const t_rr_node& node) {
    /* TODO: this function should be reworked later to adapt RRGraphView interface 
     *       once xlow(), ylow(), side() APIs are implemented
     */
    VTR_ASSERT(rr_graph.node_type(node.id()) == IPIN || rr_graph.node_type(node.id()) == OPIN);

    if ((rr_graph.node_xlow(node.id()) == 0 && !rr_graph.is_node_on_specific_side(node.id(), RIGHT))                          //left device edge connects only along block's right side
        || (rr_graph.node_ylow(node.id()) == int(grid.height() - 1) && !rr_graph.is_node_on_specific_side(node.id(), BOTTOM)) //top device edge connects only along block's bottom side
        || (rr_graph.node_xlow(node.id()) == int(grid.width() - 1) && !rr_graph.is_node_on_specific_side(node.id(), LEFT))    //right deivce edge connects only along block's left side
        || (rr_graph.node_ylow(node.id()) == 0 && !rr_graph.is_node_on_specific_side(node.id(), TOP))                         //bottom deivce edge connects only along block's top side
    ) {
        return false;
    }
    return true; //All other blocks will be surrounded on all sides by channels
}


static void check_rr_edge(const RRGraphView& rr_graph,
                          const DeviceGrid& grid,
                          const vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                          int from_node,
                          int iedge,
                          int to_node,
                          bool is_flat) {

    //Check that to to_node's fan-in is correct, given the switch type
    int iswitch = rr_graph.edge_switch(RRNodeId(from_node), iedge);
    auto switch_type = rr_graph.rr_switch_inf(RRSwitchId(iswitch)).type();

    int to_fanin = rr_graph.node_fan_in(RRNodeId(to_node));
    switch (switch_type) {
        case SwitchType::BUFFER:
            //Buffer switches are non-configurable, and uni-directional -- they must have only one driver
            if (to_fanin != 1) {
                std::string msg = "Non-configurable BUFFER type switch must have only one driver. ";
                msg += vtr::string_fmt(" Actual fan-in was %d (expected 1).\n", to_fanin);
                msg += "  Possible cause is complex block output pins connecting to:\n";
                msg += "    " + describe_rr_node(rr_graph, grid, rr_indexed_data, RRNodeId(to_node), is_flat);
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
