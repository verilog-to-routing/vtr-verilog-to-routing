#include <cstdio>

#include "route_common.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"
#include "vtr_time.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_export.h"
#include "check_route.h"
#include "rr_graph.h"
#include "check_rr_graph.h"
#include "read_xml_arch_file.h"
#include "route_tree.h"

/******************** Subroutines local to this module **********************/
static void check_node_and_range(RRNodeId inode,
                                 enum e_route_type route_type,
                                 bool is_flat);
static void check_source(const Netlist<>& net_list,
                         RRNodeId inode,
                         ParentNetId net_id,
                         bool is_flat);
static void check_sink(const Netlist<>& net_list,
                       RRNodeId inode,
                       int net_pin_index,
                       ParentNetId net_id,
                       bool* pin_done);

static void check_switch(const RouteTreeNode& rt_node, size_t num_switch);
static bool check_adjacent(RRNodeId from_node, RRNodeId to_node, bool is_flat);
static int chanx_chany_adjacent(RRNodeId chanx_node, RRNodeId chany_node);

static void check_locally_used_clb_opins(const t_clb_opins_used& clb_opins_used_locally,
                                         enum e_route_type route_type,
                                         bool is_flat);

static void check_all_non_configurable_edges(const Netlist<>& net_list, bool is_flat);
static bool check_non_configurable_edges(const Netlist<>& net_list,
                                         ParentNetId net,
                                         const t_non_configurable_rr_sets& non_configurable_rr_sets,
                                         bool is_flat);
static void check_net_for_stubs(const Netlist<>& net_list,
                                ParentNetId net,
                                bool is_flat);

/************************ Subroutine definitions ****************************/

void check_route(const Netlist<>& net_list,
                 enum e_route_type route_type,
                 e_check_route_option check_route_option,
                 bool is_flat) {
    /* This routine checks that a routing:  (1) Describes a properly         *
     * connected path for each net, (2) this path connects all the           *
     * pins spanned by that net, and (3) that no routing resources are       *
     * oversubscribed (the occupancy of everything is recomputed from        *
     * scratch).                                                             */

    if (check_route_option == e_check_route_option::OFF) {
        VTR_LOG_WARN("The user disabled the check route step.");
        return;
    }

    int max_pins;
    unsigned int ipin;
    bool valid, connects;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    const size_t num_switches = rr_graph.num_rr_switches();

    VTR_LOG("\n");
    VTR_LOG("Checking to ensure routing is legal...\n");

    /* Recompute the occupancy from scratch and check for overuse of routing *
     * resources.  This was already checked in order to determine that this  *
     * is a successful routing, but I want to double check it here.          */

    recompute_occupancy_from_scratch(net_list, is_flat);
    valid = feasible_routing();
    if (valid == false) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "Error in check_route -- routing resources are overused.\n");
    }

    if (!is_flat) {
        check_locally_used_clb_opins(route_ctx.clb_opins_used_locally,
                                     route_type,
                                     is_flat);
    }

    max_pins = 0;
    for (auto net_id : net_list.nets())
        max_pins = std::max(max_pins, (int)net_list.net_pins(net_id).size());

    auto pin_done = std::make_unique<bool[]>(max_pins);

    /* Now check that all nets are indeed connected. */
    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id) || net_list.net_sinks(net_id).size() == 0) /* Skip ignored nets. */
            continue;

        std::fill_n(pin_done.get(), net_list.net_pins(net_id).size(), false);

        if (!route_ctx.route_trees[net_id]) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_route: net %d has no routing.\n", size_t(net_id));
        }

        /* Check the SOURCE of the net. */
        RRNodeId source_inode = route_ctx.route_trees[net_id].value().root().inode;
        check_node_and_range(source_inode, route_type, is_flat);
        check_source(net_list, source_inode, net_id, is_flat);

        pin_done[0] = true;

        /* Check the rest of the net */
        size_t num_sinks = 0;
        for (auto& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            int net_pin_index = rt_node.net_pin_index;
            check_node_and_range(inode, route_type, is_flat);
            check_switch(rt_node, num_switches);

            if (rt_node.parent()) {
                connects = check_adjacent(rt_node.parent()->inode, rt_node.inode, is_flat);
                if (!connects) {
                    VPR_ERROR(VPR_ERROR_ROUTE,
                              "in check_route: found non-adjacent segments in traceback while checking net %d:\n"
                              "  %s\n"
                              "  %s\n",
                              size_t(net_id),
                              describe_rr_node(rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, rt_node.parent()->inode, is_flat).c_str(),
                              describe_rr_node(rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat).c_str());
                }
            }

            if (rr_graph.node_type(inode) == SINK) {
                check_sink(net_list, inode, net_pin_index, net_id, pin_done.get());
                num_sinks += 1;
            }
        }

        if (num_sinks != net_list.net_sinks(net_id).size()) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_route: net %zu (%s) has %zu SINKs (expected %zu).\n",
                            size_t(net_id), net_list.net_name(net_id).c_str(),
                            num_sinks, net_list.net_sinks(net_id).size());
        }

        for (ipin = 0; ipin < net_list.net_pins(net_id).size(); ipin++) {
            if (pin_done[ipin] == false) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_route: net %zu does not connect to pin %d.\n", size_t(net_id), ipin);
            }
        }

        check_net_for_stubs(net_list, net_id, is_flat);
    } /* End for each net */

    if (check_route_option == e_check_route_option::FULL) {
        check_all_non_configurable_edges(net_list, is_flat);
    } else {
        VTR_ASSERT(check_route_option == e_check_route_option::QUICK);
    }

    VTR_LOG("Completed routing consistency check successfully.\n");
    VTR_LOG("\n");
}

/* Checks that this SINK node is one of the terminals of inet, and marks   *
 * the appropriate pin as being reached.                                   */
static void check_sink(const Netlist<>& net_list,
                       RRNodeId inode,
                       int net_pin_index,
                       ParentNetId net_id,
                       bool* pin_done) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_ASSERT(rr_graph.node_type(inode) == SINK);

    if (net_pin_index == OPEN) { /* If there is no legal net pin index associated with this sink node */
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_sink: node %d does not connect to any terminal of net %s #%lu.\n"
                        "This error is usually caused by incorrectly specified logical equivalence in your architecture file.\n"
                        "You should try to respecify what pins are equivalent or turn logical equivalence off.\n",
                        inode, net_list.net_name(net_id).c_str(), size_t(net_id));
    }

    VTR_ASSERT(!pin_done[net_pin_index]); /* Should not have found a routed cnnection to it before */
    pin_done[net_pin_index] = true;
}

/* Checks that the node passed in is a valid source for this net. */
static void check_source(const Netlist<>& net_list,
                         RRNodeId inode,
                         ParentNetId net_id,
                         bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    t_rr_type rr_type = rr_graph.node_type(inode);
    if (rr_type != SOURCE) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net %d begins with a node of type %d.\n", size_t(net_id), rr_type);
    }

    int i = rr_graph.node_xlow(inode);
    int j = rr_graph.node_ylow(inode);
    /* for sinks and sources, ptc_num is class */
    int ptc_num = rr_graph.node_class_num(inode);
    /* First node_block for net is the source */
    ParentBlockId blk_id = net_list.net_driver_block(net_id);

    t_block_loc blk_loc;
    blk_loc = get_block_loc(blk_id, is_flat);
    if (blk_loc.loc.x != i || blk_loc.loc.y != j) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net SOURCE is in wrong location (%d,%d).\n", i, j);
    }

    //Get the driver pin's index in the block
    int iclass = get_block_pin_class_num(blk_id, net_list.net_pin(net_id, 0), is_flat);

    if (ptc_num != iclass) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net SOURCE is of wrong class (%d).\n", ptc_num);
    }
}

static void check_switch(const RouteTreeNode& rt_node, size_t num_switch) {
    /* Checks that the switch leading to this rt_node is a legal switch type. */
    if (!rt_node.parent())
        return;

    if (size_t(rt_node.parent_switch) >= num_switch) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_switch: rr_node %d left via switch type %d.\n"
                        "\tSwitch type is out of range.\n",
                        size_t(rt_node.inode), size_t(rt_node.parent_switch));
    }
}

static bool check_adjacent(RRNodeId from_node, RRNodeId to_node, bool is_flat) {
    /* This routine checks if the rr_node to_node is reachable from from_node.   *
     * It returns true if is reachable and false if it is not.  Check_node has   *
     * already been used to verify that both nodes are valid rr_nodes, so only   *
     * adjacency is checked here.
     * Special case: direct OPIN to IPIN connections need not be adjacent.  These
     * represent specially-crafted connections such as carry-chains or more advanced
     * blocks where adjacency is overridden by the architect */

    int from_layer, from_xlow, from_ylow, to_layer, to_xlow, to_ylow, from_ptc, to_ptc, iclass;
    int num_adj, to_xhigh, to_yhigh, from_xhigh, from_yhigh;
    bool reached;
    t_rr_type from_type, to_type;
    t_physical_tile_type_ptr from_grid_type, to_grid_type;

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    reached = false;

    for (t_edge_size iconn = 0; iconn < rr_graph.num_edges(RRNodeId(from_node)); iconn++) {
        if (size_t(rr_graph.edge_sink_node(from_node, iconn)) == size_t(to_node)) {
            reached = true;
            break;
        }
    }

    if (!reached)
        return (false);

    /* Now we know the rr graph says these two nodes are adjacent.  Double  *
     * check that this makes sense, to verify the rr graph.                 */
    VTR_ASSERT(reached);

    num_adj = 0;

    auto from_rr = RRNodeId(from_node);
    auto to_rr = RRNodeId(to_node);
    from_type = rr_graph.node_type(from_rr);
    from_layer = rr_graph.node_layer(from_rr);
    from_xlow = rr_graph.node_xlow(from_rr);
    from_ylow = rr_graph.node_ylow(from_rr);
    from_xhigh = rr_graph.node_xhigh(from_rr);
    from_yhigh = rr_graph.node_yhigh(from_rr);
    from_ptc = rr_graph.node_ptc_num(from_rr);
    to_type = rr_graph.node_type(to_rr);
    to_layer = rr_graph.node_layer(to_rr);
    to_xlow = rr_graph.node_xlow(to_rr);
    to_ylow = rr_graph.node_ylow(to_rr);
    to_xhigh = rr_graph.node_xhigh(to_rr);
    to_yhigh = rr_graph.node_yhigh(to_rr);
    to_ptc = rr_graph.node_ptc_num(to_rr);

    // Layer numbers are should not be more than one layer apart for connected nodes
    VTR_ASSERT(abs(from_layer - to_layer) <= 1);
    switch (from_type) {
        case SOURCE:
            VTR_ASSERT(to_type == OPIN);

            //The OPIN should be contained within the bounding box of it's connected source
            if (from_xlow <= to_xlow
                && from_ylow <= to_ylow
                && from_xhigh >= to_xhigh
                && from_yhigh >= to_yhigh) {
                from_grid_type = device_ctx.grid.get_physical_type({from_xlow, from_ylow, from_layer});
                to_grid_type = device_ctx.grid.get_physical_type({to_xlow, to_ylow, to_layer});
                VTR_ASSERT(from_grid_type == to_grid_type);

                iclass = get_class_num_from_pin_physical_num(to_grid_type, to_ptc);
                if (iclass == from_ptc)
                    num_adj++;
            }
            break;

        case SINK:
            /* SINKS are adjacent to not connected */
            break;

        case OPIN:
            from_grid_type = device_ctx.grid.get_physical_type({from_xlow, from_ylow, from_layer});
            if (to_type == CHANX || to_type == CHANY) {
                num_adj += 1; //adjacent
            } else if (is_flat) {
                VTR_ASSERT(to_type == OPIN || to_type == IPIN); // If pin is located inside a cluster
                return true;
            } else {
                VTR_ASSERT(to_type == IPIN); /* direct OPIN to IPIN connections not necessarily adjacent */
                return true;                 /* Special case, direct OPIN to IPIN connections need not be adjacent */
            }

            break;

        case IPIN:
            from_grid_type = device_ctx.grid.get_physical_type({from_xlow, from_ylow, from_layer});
            if (is_flat) {
                VTR_ASSERT(to_type == OPIN || to_type == IPIN || to_type == SINK);
            } else {
                VTR_ASSERT(to_type == SINK);
            }

            //An IPIN should be contained within the bounding box of it's connected sink
            if (to_type == SINK) {
                if (from_xlow >= to_xlow
                    && from_ylow >= to_ylow
                    && from_xhigh <= to_xhigh
                    && from_yhigh <= to_yhigh) {
                    from_grid_type = device_ctx.grid.get_physical_type({from_xlow, from_ylow, from_layer});
                    to_grid_type = device_ctx.grid.get_physical_type({to_xlow, to_ylow, to_layer});
                    VTR_ASSERT(from_grid_type == to_grid_type);
                    iclass = get_class_num_from_pin_physical_num(from_grid_type, from_ptc);
                    if (iclass == to_ptc)
                        num_adj++;
                }
            } else {
                from_grid_type = device_ctx.grid.get_physical_type({from_xlow, from_ylow, from_layer});
                to_grid_type = device_ctx.grid.get_physical_type({to_xlow, to_ylow, to_layer});
                VTR_ASSERT(from_grid_type == to_grid_type);
                int from_root_x = from_xlow - device_ctx.grid.get_width_offset({from_xlow, from_ylow, from_layer});
                int from_root_y = from_ylow - device_ctx.grid.get_height_offset({from_xlow, from_ylow, from_layer});
                int to_root_x = to_xlow - device_ctx.grid.get_width_offset({to_xlow, to_ylow, to_layer});
                int to_root_y = to_ylow - device_ctx.grid.get_height_offset({to_xlow, to_ylow, to_layer});

                if (from_root_x == to_root_x && from_root_y == to_root_y) {
                    num_adj++;
                }
            }
            break;

        case CHANX:
            if (to_type == IPIN) {
                num_adj += 1; //adjacent
            } else if (to_type == CHANX) {
                from_xhigh = rr_graph.node_xhigh(from_node);
                to_xhigh = rr_graph.node_xhigh(to_node);
                if (from_ylow == to_ylow) {
                    /* UDSD Modification by WMF Begin */
                    /*For Fs > 3, can connect to overlapping wire segment */
                    if (to_xhigh == from_xlow - 1 || from_xhigh == to_xlow - 1) {
                        num_adj++;
                    }
                    /* Overlapping */
                    else {
                        int i;

                        for (i = from_xlow; i <= from_xhigh; i++) {
                            if (i >= to_xlow && i <= to_xhigh) {
                                num_adj++;
                                break;
                            }
                        }
                    }
                    /* UDSD Modification by WMF End */
                }
            } else if (to_type == CHANY) {
                num_adj += chanx_chany_adjacent(from_node, to_node);
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_adjacent: %d and %d are not adjacent", from_node, to_node);
            }
            break;

        case CHANY:
            if (to_type == IPIN) {
                num_adj += 1; //adjacent
            } else if (to_type == CHANY) {
                from_yhigh = rr_graph.node_yhigh(from_node);
                to_yhigh = rr_graph.node_yhigh(to_node);
                if (from_xlow == to_xlow) {
                    /* UDSD Modification by WMF Begin */
                    if (to_yhigh == from_ylow - 1 || from_yhigh == to_ylow - 1) {
                        num_adj++;
                    }
                    /* Overlapping */
                    else {
                        int j;

                        for (j = from_ylow; j <= from_yhigh; j++) {
                            if (j >= to_ylow && j <= to_yhigh) {
                                num_adj++;
                                break;
                            }
                        }
                    }
                    /* UDSD Modification by WMF End */
                }
            } else if (to_type == CHANX) {
                num_adj += chanx_chany_adjacent(to_node, from_node);
            } else {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_adjacent: %d and %d are not adjacent", from_node, to_node);
            }
            break;

        default:
            break;
    }

    if (num_adj == 1)
        return (true);
    else if (num_adj == 0)
        return (false);

    VPR_ERROR(VPR_ERROR_ROUTE,
              "in check_adjacent: num_adj = %d. Expected 0 or 1.\n", num_adj);
    return false; //Should not reach here once thrown
}

static int chanx_chany_adjacent(RRNodeId chanx_node, RRNodeId chany_node) {
    /* Returns 1 if the specified CHANX and CHANY nodes are adjacent, 0         *
     * otherwise.                                                               */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (rr_graph.nodes_are_adjacent(chanx_node, chany_node)) {
        return (1);
    } else {
        return (0);
    }
}

void recompute_occupancy_from_scratch(const Netlist<>& net_list, bool is_flat) {
    /*
     * This routine updates the occ field in the route_ctx.rr_node_route_inf structure
     * according to the resource usage of the current routing.  It does a
     * brute force recompute from scratch that is useful for sanity checking.
     */
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();

    /* First set the occupancy of everything to zero. */
    for (RRNodeId inode : device_ctx.rr_graph.nodes())
        route_ctx.rr_node_route_inf[inode].set_occ(0);

    /* Now go through each net and count the tracks and pins used everywhere */

    for (auto net_id : net_list.nets()) {
        if (!route_ctx.route_trees[net_id])
            continue;

        if (net_list.net_is_ignored(net_id)) /* Skip ignored nets. */
            continue;

        for (auto& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            route_ctx.rr_node_route_inf[inode].set_occ(route_ctx.rr_node_route_inf[inode].occ() + 1);
        }
    }

    /* We only need to reserve output pins if flat routing is not enabled */
    if (!is_flat) {
        /* Now update the occupancy of each of the "locally used" OPINs on each CLB *
         * (CLB outputs used up by being directly wired to subblocks used only      *
         * locally).                                                                */
        for (auto blk_id : net_list.blocks()) {
            auto cluster_blk_id = convert_to_cluster_block_id(blk_id);
            for (int iclass = 0; iclass < (int)physical_tile_type(cluster_blk_id)->class_inf.size(); iclass++) {
                int num_local_opins = route_ctx.clb_opins_used_locally[cluster_blk_id][iclass].size();
                /* Will always be 0 for pads or SINK classes. */
                for (int ipin = 0; ipin < num_local_opins; ipin++) {
                    RRNodeId inode = route_ctx.clb_opins_used_locally[cluster_blk_id][iclass][ipin];
                    VTR_ASSERT(inode && size_t(inode) < device_ctx.rr_graph.num_nodes());
                    route_ctx.rr_node_route_inf[inode].set_occ(route_ctx.rr_node_route_inf[inode].occ() + 1);
                }
            }
        }
    }
}

static void check_locally_used_clb_opins(const t_clb_opins_used& clb_opins_used_locally,
                                         enum e_route_type route_type,
                                         bool is_flat) {
    /* Checks that enough OPINs on CLBs have been set aside (used up) to make a *
     * legal routing if subblocks connect to OPINs directly.                    */

    int iclass, num_local_opins, ipin;
    t_rr_type rr_type;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        for (iclass = 0; iclass < (int)physical_tile_type(blk_id)->class_inf.size(); iclass++) {
            num_local_opins = clb_opins_used_locally[blk_id][iclass].size();
            /* Always 0 for pads and for SINK classes */

            for (ipin = 0; ipin < num_local_opins; ipin++) {
                RRNodeId inode = clb_opins_used_locally[blk_id][iclass][ipin];
                check_node_and_range(RRNodeId(inode), route_type, is_flat); /* Node makes sense? */

                /* Now check that node is an OPIN of the right type. */

                rr_type = rr_graph.node_type(RRNodeId(inode));
                if (rr_type != OPIN) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "in check_locally_used_opins: block #%lu (%s)\n"
                                    "\tClass %d local OPIN is wrong rr_type -- rr_node #%d of type %d.\n",
                                    size_t(blk_id), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), iclass, inode, rr_type);
                }

                ipin = rr_graph.node_pin_num(RRNodeId(inode));
                if (get_class_num_from_pin_physical_num(physical_tile_type(blk_id), ipin) != iclass) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "in check_locally_used_opins: block #%lu (%s):\n"
                                    "\tExpected class %d local OPIN has class %d -- rr_node #: %d.\n",
                                    size_t(blk_id), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), iclass, get_class_num_from_pin_physical_num(physical_tile_type(blk_id), ipin), inode);
                }
            }
        }
    }
}

static void check_node_and_range(RRNodeId inode,
                                 enum e_route_type route_type,
                                 bool is_flat) {
    /* Checks that inode is within the legal range, then calls check_node to    *
     * check that everything else about the node is OK.                         */

    auto& device_ctx = g_vpr_ctx.device();

    if (size_t(inode) >= device_ctx.rr_graph.num_nodes()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_node_and_range: rr_node #%zu is out of legal range (0 to %d).\n", size_t(inode), device_ctx.rr_graph.num_nodes() - 1);
    }
    check_rr_node(device_ctx.rr_graph,
                  device_ctx.rr_indexed_data,
                  device_ctx.grid,
                  device_ctx.chan_width,
                  route_type,
                  size_t(inode),
                  is_flat);
}

//Checks that all non-configurable edges are in a legal configuration
//This check is slow, so it has been moved out of check_route()
static void check_all_non_configurable_edges(const Netlist<>& net_list, bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Checking to ensure non-configurable edges are legal");
    auto non_configurable_rr_sets = identify_non_configurable_rr_sets();

    for (auto net_id : net_list.nets()) {
        check_non_configurable_edges(net_list,
                                     net_id,
                                     non_configurable_rr_sets,
                                     is_flat);
    }
}

// Checks that the specified routing is legal with respect to non-configurable edges
//
//For routing to be legal if *any* non-configurable edge is used, so must *all*
//other non-configurable edges in the same set
static bool check_non_configurable_edges(const Netlist<>& net_list,
                                         ParentNetId net,
                                         const t_non_configurable_rr_sets& non_configurable_rr_sets,
                                         bool is_flat) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (!route_ctx.route_trees[net]) // no routing
        return true;

    // Collect all the edges used by this net's routing
    std::set<t_node_edge> routing_edges;
    std::set<RRNodeId> routing_nodes;
    for (auto& rt_node : route_ctx.route_trees[net].value().all_nodes()) {
        routing_nodes.insert(rt_node.inode);
        if (!rt_node.parent())
            continue;
        t_node_edge edge = {rt_node.parent()->inode, rt_node.inode};
        routing_edges.insert(edge);
    }

    //We need to perform two types of checks:
    //
    // 1) That all nodes in a non-configurable set are included
    // 2) That all (required) non-configurable edges are used
    //
    //We need to check (2) in addition to (1) to ensure that (1) did not pass
    //because the nodes 'happend' to be connected together by configurable
    //routing (to be legal, by definition, they must be connected by
    //non-configurable routing).

    //Check that all nodes in each non-configurable set are full included if any element
    //within a set is used by the routing
    for (const auto& rr_nodes : non_configurable_rr_sets.node_sets) {
        //Compute the intersection of the routing and current non-configurable nodes set
        std::vector<RRNodeId> intersection;
        std::set_intersection(routing_nodes.begin(), routing_nodes.end(),
                              rr_nodes.begin(), rr_nodes.end(),
                              std::back_inserter(intersection));

        //If the intersection is non-empty then the current routing uses
        //at least one non-configurable edge in this set
        if (!intersection.empty()) {
            //To be legal *all* the nodes must be included in the routing
            if (intersection.size() != rr_nodes.size()) {
                //Illegal

                //Compute the difference to identify the missing nodes
                //for detailed error reporting -- the nodes
                //which are in rr_nodes but not in routing_nodes.
                std::vector<RRNodeId> difference;
                std::set_difference(rr_nodes.begin(), rr_nodes.end(),
                                    routing_nodes.begin(), routing_nodes.end(),
                                    std::back_inserter(difference));

                VTR_ASSERT(difference.size() > 0);
                std::string msg = vtr::string_fmt(
                    "Illegal routing for net '%s' (#%zu) some "
                    "required non-configurably connected nodes are missing:\n",
                    net_list.net_name(net).c_str(), size_t(net));

                for (auto inode : difference) {
                    msg += vtr::string_fmt("  Missing %s\n", describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat).c_str());
                }

                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
            }
        }
    }

    //Check that any sets of non-configurable RR graph edges are fully included
    //in the routing, if any of a set's edges are used
    for (const auto& rr_edges : non_configurable_rr_sets.edge_sets) {
        //Compute the intersection of the routing and current non-configurable edge set
        std::vector<t_node_edge> intersection;
        std::set_intersection(routing_edges.begin(), routing_edges.end(),
                              rr_edges.begin(), rr_edges.end(),
                              std::back_inserter(intersection));

        //If the intersection is non-empty then the current routing uses
        //at least one non-configurable edge in this set
        if (!intersection.empty()) {
            //Since at least one non-configurable edge is used, to be legal
            //the full set of non-configurably connected edges must be used.
            //
            //This is somewhat complicted by the fact that non-configurable edges
            //are sometimes bi-directional (e.g. electrical shorts) and so appear
            //in rr_edges twice (once forward, once backward). Only one of the
            //paired edges need appear to be correct.

            //To figure out which edges are missing we compute
            //the difference from rr_edges to routing_edges -- the nodes
            //which are in rr_edges but not in routing_edges.
            std::vector<t_node_edge> difference;
            std::set_difference(rr_edges.begin(), rr_edges.end(),
                                routing_edges.begin(), routing_edges.end(),
                                std::back_inserter(difference));

            //Next we remove edges in the difference if there is a reverse
            //edge in rr_edges and the forward edge is found in routing (or vice-versa).
            //It is OK if there is an unused reverse/forward edge provided the
            //forward/reverse edge is used.
            std::vector<t_node_edge> dedupped_difference;
            std::copy_if(difference.begin(), difference.end(),
                         std::back_inserter(dedupped_difference),
                         [&](t_node_edge forward_edge) {
                             VTR_ASSERT_MSG(!routing_edges.count(forward_edge), "Difference should not contain used routing edges");

                             t_node_edge reverse_edge = {forward_edge.to_node, forward_edge.from_node};

                             //Check whether the reverse edge was used
                             if (rr_edges.count(reverse_edge) && routing_edges.count(reverse_edge)) {
                                 //The reverse edge exists in the set of rr_edges, and was used
                                 //by the routing.
                                 //
                                 //We can therefore safely ignore the fact that this (forward) edge is un-used
                                 return false; //Drop from difference
                             } else {
                                 return true; //Keep, this edge should have been used
                             }
                         });

            //At this point only valid missing node pairs are in the set
            if (!dedupped_difference.empty()) {
                std::string msg = vtr::string_fmt("Illegal routing for net '%s' (#%zu) some required non-configurable edges are missing:\n",
                                                  net_list.net_name(net).c_str(), size_t(net));

                for (t_node_edge missing_edge : dedupped_difference) {
                    msg += vtr::string_fmt("  Expected RR Node: %d and RR Node: %d to be non-configurably connected, but edge missing from routing:\n",
                                           missing_edge.from_node, missing_edge.to_node);
                    msg += vtr::string_fmt("    %s\n", describe_rr_node(device_ctx.rr_graph,
                                                                        device_ctx.grid,
                                                                        device_ctx.rr_indexed_data,
                                                                        RRNodeId(missing_edge.from_node),
                                                                        is_flat)
                                                           .c_str());
                    msg += vtr::string_fmt("    %s\n", describe_rr_node(device_ctx.rr_graph,
                                                                        device_ctx.grid,
                                                                        device_ctx.rr_indexed_data,
                                                                        RRNodeId(missing_edge.to_node),
                                                                        is_flat)
                                                           .c_str());
                }

                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
            }

            //TODO: verify that the switches used in trace are actually non-configurable
        }
    }

    return true;
}

// StubFinder traverses a net definition and find ensures that all nodes that
// are children of a configurable node have at least one sink.
class StubFinder {
  public:
    StubFinder() {}

    // Checks specified net for stubs, return true if at least one stub is
    // found.
    bool CheckNet(ParentNetId net);

    // Returns set of stub nodes.
    const std::set<int>& stub_nodes() {
        return stub_nodes_;
    }

  private:
    bool RecurseTree(const RouteTreeNode& rt_node);

    // Set of stub nodes
    // Note this is an ordered set so that node output is sorted by node
    // id.
    std::set<int> stub_nodes_;
};

//Cheks for stubs in a net's routing.
//
//Stubs (routing branches which don't connect to SINKs) serve no purpose, and only chew up wiring unecessarily.
//The only exception are stubs required by non-configurable switches (e.g. shorts).
//
//We treat any configurable stubs as an error.
void check_net_for_stubs(const Netlist<>& net_list,
                         ParentNetId net,
                         bool is_flat) {
    StubFinder stub_finder;

    bool any_stubs = stub_finder.CheckNet(net);
    if (any_stubs) {
        const auto& device_ctx = g_vpr_ctx.device();
        std::string msg = vtr::string_fmt("Route tree for net '%s' (#%zu) contains stub branches rooted at:\n",
                                          net_list.net_name(net).c_str(), size_t(net));
        for (int inode : stub_finder.stub_nodes()) {
            msg += vtr::string_fmt("    %s\n", describe_rr_node(device_ctx.rr_graph,
                                                                device_ctx.grid,
                                                                device_ctx.rr_indexed_data,
                                                                RRNodeId(inode),
                                                                is_flat)
                                                   .c_str());
        }

        VPR_THROW(VPR_ERROR_ROUTE, msg.c_str());
    }
}

bool StubFinder::CheckNet(ParentNetId net) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    stub_nodes_.clear();

    if (!route_ctx.route_trees[net])
        return false;

    RecurseTree(route_ctx.route_trees[net].value().root());

    return !stub_nodes_.empty();
}

// Returns true if this node is a stub.
bool StubFinder::RecurseTree(const RouteTreeNode& rt_node) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (rt_node.is_leaf()) {
        //If a leaf of the route tree is not a SINK, then it is a stub
        if (rr_graph.node_type(rt_node.inode) != SINK) {
            return true; //It is the current root of this stub
        } else {
            return false;
        }
    }

    bool is_stub = true;
    for (auto& child_node : rt_node.child_nodes()) {
        bool driver_switch_configurable = rr_graph.rr_switch_inf(child_node.parent_switch).configurable();
        bool child_is_stub = RecurseTree(child_node);
        if (!child_is_stub) {
            // Because the child was not a stub, this node is not a stub.
            is_stub = false;
        } else if (driver_switch_configurable) {
            // This child was stub, and we drove it from a configurable
            // edge, this is an error.
            stub_nodes_.insert(size_t(child_node.inode));
        }
    }

    return is_stub;
}
