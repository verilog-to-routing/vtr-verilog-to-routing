#include <cstdio>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_export.h"
#include "check_route.h"
#include "rr_graph.h"
#include "check_rr_graph.h"
#include "read_xml_arch_file.h"
#include "route_tree_type.h"
#include "route_tree_timing.h"

/******************** Subroutines local to this module **********************/
static void check_node_and_range(int inode, enum e_route_type route_type);
static void check_source(int inode, ClusterNetId net_id);
static void check_sink(int inode, ClusterNetId net_id, bool* pin_done);
static void check_switch(t_trace* tptr, int num_switch);
static bool check_adjacent(int from_node, int to_node);
static int chanx_chany_adjacent(int chanx_node, int chany_node);
static void reset_flags(ClusterNetId inet, bool* connected_to_route);
static void check_locally_used_clb_opins(const t_clb_opins_used& clb_opins_used_locally,
                                         enum e_route_type route_type);

static bool check_non_configurable_edges(ClusterNetId net, const t_non_configurable_rr_sets& non_configurable_rr_sets);
static void check_net_for_stubs(ClusterNetId net);

/************************ Subroutine definitions ****************************/

void check_route(enum e_route_type route_type) {
    /* This routine checks that a routing:  (1) Describes a properly         *
     * connected path for each net, (2) this path connects all the           *
     * pins spanned by that net, and (3) that no routing resources are       *
     * oversubscribed (the occupancy of everything is recomputed from        *
     * scratch).                                                             */

    int max_pins, inode, prev_node;
    unsigned int ipin;
    bool valid, connects;
    bool* connected_to_route; /* [0 .. device_ctx.rr_nodes.size()-1] */
    t_trace* tptr;
    bool* pin_done;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    const int num_switches = device_ctx.rr_switch_inf.size();

    VTR_LOG("\n");
    VTR_LOG("Checking to ensure routing is legal...\n");

    /* Recompute the occupancy from scratch and check for overuse of routing *
     * resources.  This was already checked in order to determine that this  *
     * is a successful routing, but I want to double check it here.          */

    recompute_occupancy_from_scratch();
    valid = feasible_routing();
    if (valid == false) {
        VPR_ERROR(VPR_ERROR_ROUTE,
                  "Error in check_route -- routing resources are overused.\n");
    }

    check_locally_used_clb_opins(route_ctx.clb_opins_used_locally, route_type);

    auto non_configurable_rr_sets = identify_non_configurable_rr_sets();

    connected_to_route = (bool*)vtr::calloc(device_ctx.rr_nodes.size(), sizeof(bool));

    max_pins = 0;
    for (auto net_id : cluster_ctx.clb_nlist.nets())
        max_pins = std::max(max_pins, (int)cluster_ctx.clb_nlist.net_pins(net_id).size());

    pin_done = (bool*)vtr::malloc(max_pins * sizeof(bool));

    /* Now check that all nets are indeed connected. */
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id) || cluster_ctx.clb_nlist.net_sinks(net_id).size() == 0) /* Skip ignored nets. */
            continue;

        for (ipin = 0; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++)
            pin_done[ipin] = false;

        /* Check the SOURCE of the net. */
        tptr = route_ctx.trace[net_id].head;
        if (tptr == nullptr) {
            VPR_ERROR(VPR_ERROR_ROUTE,
                      "in check_route: net %d has no routing.\n", size_t(net_id));
        }

        inode = tptr->index;
        check_node_and_range(inode, route_type);
        check_switch(tptr, num_switches);
        connected_to_route[inode] = true; /* Mark as in path. */

        check_source(inode, net_id);
        pin_done[0] = true;

        prev_node = inode;
        int prev_switch = tptr->iswitch;
        tptr = tptr->next;

        /* Check the rest of the net */
        size_t num_sinks = 0;
        while (tptr != nullptr) {
            inode = tptr->index;
            check_node_and_range(inode, route_type);
            check_switch(tptr, num_switches);

            if (prev_switch == OPEN) { //Start of a new branch
                if (connected_to_route[inode] == false) {
                    VPR_ERROR(VPR_ERROR_ROUTE,
                              "in check_route: node %d does not link into existing routing for net %d.\n", inode, size_t(net_id));
                }
            } else { //Continuing along existing branch
                connects = check_adjacent(prev_node, inode);
                if (!connects) {
                    VPR_ERROR(VPR_ERROR_ROUTE,
                              "in check_route: found non-adjacent segments in traceback while checking net %d:\n"
                              "  %s\n"
                              "  %s\n",
                              size_t(net_id),
                              describe_rr_node(prev_node).c_str(),
                              describe_rr_node(inode).c_str());
                }

                connected_to_route[inode] = true; /* Mark as in path. */

                if (device_ctx.rr_nodes[inode].type() == SINK) {
                    check_sink(inode, net_id, pin_done);
                    num_sinks += 1;
                }

            } /* End of prev_node type != SINK */
            prev_node = inode;
            prev_switch = tptr->iswitch;
            tptr = tptr->next;
        } /* End while */

        if (num_sinks != cluster_ctx.clb_nlist.net_sinks(net_id).size()) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_route: net %zu (%s) has %zu SINKs (expected %zu).\n",
                            size_t(net_id), cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                            num_sinks, cluster_ctx.clb_nlist.net_sinks(net_id).size());
        }

        for (ipin = 0; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            if (pin_done[ipin] == false) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "in check_route: net %zu does not connect to pin %d.\n", size_t(net_id), ipin);
            }
        }

        check_non_configurable_edges(net_id, non_configurable_rr_sets);

        check_net_for_stubs(net_id);

        reset_flags(net_id, connected_to_route);

    } /* End for each net */

    free(pin_done);
    free(connected_to_route);
    VTR_LOG("Completed routing consistency check successfully.\n");
    VTR_LOG("\n");
}

/* Checks that this SINK node is one of the terminals of inet, and marks   *
 * the appropriate pin as being reached.                                   */
static void check_sink(int inode, ClusterNetId net_id, bool* pin_done) {
    int i, j, ifound, ptc_num, iclass, iblk, pin_index;
    ClusterBlockId bnum;
    unsigned int ipin;
    t_physical_tile_type_ptr type;
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    VTR_ASSERT(device_ctx.rr_nodes[inode].type() == SINK);
    i = device_ctx.rr_nodes[inode].xlow();
    j = device_ctx.rr_nodes[inode].ylow();
    type = device_ctx.grid[i][j].type;
    /* For sinks, ptc_num is the class */
    ptc_num = device_ctx.rr_nodes[inode].ptc_num();
    ifound = 0;

    for (iblk = 0; iblk < type->capacity; iblk++) {
        bnum = place_ctx.grid_blocks[i][j].blocks[iblk]; /* Hardcoded to one cluster_ctx block*/
        ipin = 1;
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            if (cluster_ctx.clb_nlist.pin_block(pin_id) == bnum) {
                pin_index = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
                iclass = type->pin_class[pin_index];
                if (iclass == ptc_num) {
                    /* Could connect to same pin class on the same clb more than once.  Only   *
                     * update pin_done for a pin that hasn't been reached yet.                 */
                    if (pin_done[ipin] == false) {
                        ifound++;
                        pin_done[ipin] = true;
                    }
                }
            }
            ipin++;
        }
    }

    if (ifound > 1 && is_io_type(type)) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_sink: found %d terminals of net %d of pad %d at location (%d, %d).\n", ifound, size_t(net_id), ptc_num, i, j);
    }

    if (ifound < 1) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_sink: node %d does not connect to any terminal of net %s #%lu.\n"
                        "This error is usually caused by incorrectly specified logical equivalence in your architecture file.\n"
                        "You should try to respecify what pins are equivalent or turn logical equivalence off.\n",
                        inode, cluster_ctx.clb_nlist.net_name(net_id).c_str(), size_t(net_id));
    }
}

/* Checks that the node passed in is a valid source for this net. */
static void check_source(int inode, ClusterNetId net_id) {
    t_rr_type rr_type;
    t_physical_tile_type_ptr type;
    ClusterBlockId blk_id;
    int i, j, ptc_num, node_block_pin, iclass;
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    rr_type = device_ctx.rr_nodes[inode].type();
    if (rr_type != SOURCE) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net %d begins with a node of type %d.\n", size_t(net_id), rr_type);
    }

    i = device_ctx.rr_nodes[inode].xlow();
    j = device_ctx.rr_nodes[inode].ylow();
    /* for sinks and sources, ptc_num is class */
    ptc_num = device_ctx.rr_nodes[inode].ptc_num();
    /* First node_block for net is the source */
    blk_id = cluster_ctx.clb_nlist.net_driver_block(net_id);
    type = device_ctx.grid[i][j].type;

    if (place_ctx.block_locs[blk_id].loc.x != i || place_ctx.block_locs[blk_id].loc.y != j) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net SOURCE is in wrong location (%d,%d).\n", i, j);
    }

    //Get the driver pin's index in the block
    node_block_pin = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);
    iclass = type->pin_class[node_block_pin];

    if (ptc_num != iclass) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_source: net SOURCE is of wrong class (%d).\n", ptc_num);
    }
}

static void check_switch(t_trace* tptr, int num_switch) {
    /* Checks that the switch leading from this traceback element to the next *
     * one is a legal switch type.                                            */

    int inode;
    short switch_type;

    auto& device_ctx = g_vpr_ctx.device();

    inode = tptr->index;
    switch_type = tptr->iswitch;

    if (device_ctx.rr_nodes[inode].type() != SINK) {
        if (switch_type >= num_switch) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_switch: rr_node %d left via switch type %d.\n"
                            "\tSwitch type is out of range.\n",
                            inode, switch_type);
        }
    }

    else { /* Is a SINK */

        /* Without feedthroughs, there should be no switch.  If feedthroughs are    *
         * allowed, change to treat a SINK like any other node (as above).          */

        if (switch_type != OPEN) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "in check_switch: rr_node %d is a SINK, but attempts to use a switch of type %d.\n", inode, switch_type);
        }
    }
}

static void reset_flags(ClusterNetId inet, bool* connected_to_route) {
    /* This routine resets the flags of all the channel segments contained *
     * in the traceback of net inet to 0.  This allows us to check the     *
     * next net for connectivity (and the default state of the flags       *
     * should always be zero after they have been used).                   */

    t_trace* tptr;
    int inode;

    auto& route_ctx = g_vpr_ctx.routing();

    tptr = route_ctx.trace[inet].head;

    while (tptr != nullptr) {
        inode = tptr->index;
        connected_to_route[inode] = false; /* Not in routed path now. */
        tptr = tptr->next;
    }
}

static bool check_adjacent(int from_node, int to_node) {
    /* This routine checks if the rr_node to_node is reachable from from_node.   *
     * It returns true if is reachable and false if it is not.  Check_node has   *
     * already been used to verify that both nodes are valid rr_nodes, so only   *
     * adjacency is checked here.
     * Special case: direct OPIN to IPIN connections need not be adjacent.  These
     * represent specially-crafted connections such as carry-chains or more advanced
     * blocks where adjacency is overridden by the architect */

    int from_xlow, from_ylow, to_xlow, to_ylow, from_ptc, to_ptc, iclass;
    int num_adj, to_xhigh, to_yhigh, from_xhigh, from_yhigh;
    bool reached;
    t_rr_type from_type, to_type;
    t_physical_tile_type_ptr from_grid_type, to_grid_type;

    auto& device_ctx = g_vpr_ctx.device();

    reached = false;

    for (t_edge_size iconn = 0; iconn < device_ctx.rr_nodes[from_node].num_edges(); iconn++) {
        if (device_ctx.rr_nodes[from_node].edge_sink_node(iconn) == to_node) {
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

    from_type = device_ctx.rr_nodes[from_node].type();
    from_xlow = device_ctx.rr_nodes[from_node].xlow();
    from_ylow = device_ctx.rr_nodes[from_node].ylow();
    from_xhigh = device_ctx.rr_nodes[from_node].xhigh();
    from_yhigh = device_ctx.rr_nodes[from_node].yhigh();
    from_ptc = device_ctx.rr_nodes[from_node].ptc_num();
    to_type = device_ctx.rr_nodes[to_node].type();
    to_xlow = device_ctx.rr_nodes[to_node].xlow();
    to_ylow = device_ctx.rr_nodes[to_node].ylow();
    to_xhigh = device_ctx.rr_nodes[to_node].xhigh();
    to_yhigh = device_ctx.rr_nodes[to_node].yhigh();
    to_ptc = device_ctx.rr_nodes[to_node].ptc_num();

    switch (from_type) {
        case SOURCE:
            VTR_ASSERT(to_type == OPIN);

            //The OPIN should be contained within the bounding box of it's connected source
            if (from_xlow <= to_xlow
                && from_ylow <= to_ylow
                && from_xhigh >= to_xhigh
                && from_yhigh >= to_yhigh) {
                from_grid_type = device_ctx.grid[from_xlow][from_ylow].type;
                to_grid_type = device_ctx.grid[to_xlow][to_ylow].type;
                VTR_ASSERT(from_grid_type == to_grid_type);

                iclass = to_grid_type->pin_class[to_ptc];
                if (iclass == from_ptc)
                    num_adj++;
            }
            break;

        case SINK:
            /* SINKS are adjacent to not connected */
            break;

        case OPIN:
            if (to_type == CHANX || to_type == CHANY) {
                num_adj += 1; //adjacent
            } else {
                VTR_ASSERT(to_type == IPIN); /* direct OPIN to IPIN connections not necessarily adjacent */
                return true;                 /* Special case, direct OPIN to IPIN connections need not be adjacent */
            }

            break;

        case IPIN:
            VTR_ASSERT(to_type == SINK);

            //An IPIN should be contained within the bounding box of it's connected sink
            if (from_xlow >= to_xlow
                && from_ylow >= to_ylow
                && from_xhigh <= to_xhigh
                && from_yhigh <= to_yhigh) {
                from_grid_type = device_ctx.grid[from_xlow][from_ylow].type;
                to_grid_type = device_ctx.grid[to_xlow][to_ylow].type;
                VTR_ASSERT(from_grid_type == to_grid_type);

                iclass = from_grid_type->pin_class[from_ptc];
                if (iclass == to_ptc)
                    num_adj++;
            }
            break;

        case CHANX:
            if (to_type == IPIN) {
                num_adj += 1; //adjacent
            } else if (to_type == CHANX) {
                from_xhigh = device_ctx.rr_nodes[from_node].xhigh();
                to_xhigh = device_ctx.rr_nodes[to_node].xhigh();
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
                VTR_ASSERT(0);
            }
            break;

        case CHANY:
            if (to_type == IPIN) {
                num_adj += 1; //adjacent
            } else if (to_type == CHANY) {
                from_yhigh = device_ctx.rr_nodes[from_node].yhigh();
                to_yhigh = device_ctx.rr_nodes[to_node].yhigh();
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
                VTR_ASSERT(0);
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

static int chanx_chany_adjacent(int chanx_node, int chany_node) {
    /* Returns 1 if the specified CHANX and CHANY nodes are adjacent, 0         *
     * otherwise.                                                               */

    int chanx_y, chanx_xlow, chanx_xhigh;
    int chany_x, chany_ylow, chany_yhigh;

    auto& device_ctx = g_vpr_ctx.device();

    chanx_y = device_ctx.rr_nodes[chanx_node].ylow();
    chanx_xlow = device_ctx.rr_nodes[chanx_node].xlow();
    chanx_xhigh = device_ctx.rr_nodes[chanx_node].xhigh();

    chany_x = device_ctx.rr_nodes[chany_node].xlow();
    chany_ylow = device_ctx.rr_nodes[chany_node].ylow();
    chany_yhigh = device_ctx.rr_nodes[chany_node].yhigh();

    if (chany_ylow > chanx_y + 1 || chany_yhigh < chanx_y)
        return (0);

    if (chanx_xlow > chany_x + 1 || chanx_xhigh < chany_x)
        return (0);

    return (1);
}

void recompute_occupancy_from_scratch() {
    /*
     * This routine updates the occ field in the route_ctx.rr_node_route_inf structure
     * according to the resource usage of the current routing.  It does a
     * brute force recompute from scratch that is useful for sanity checking.
     */

    int inode, iclass, ipin, num_local_opins;
    t_trace* tptr;

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* First set the occupancy of everything to zero. */

    for (size_t inode_idx = 0; inode_idx < device_ctx.rr_nodes.size(); inode_idx++)
        route_ctx.rr_node_route_inf[inode_idx].set_occ(0);

    /* Now go through each net and count the tracks and pins used everywhere */

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) /* Skip ignored nets. */
            continue;

        tptr = route_ctx.trace[net_id].head;
        if (tptr == nullptr)
            continue;

        for (;;) {
            inode = tptr->index;
            route_ctx.rr_node_route_inf[inode].set_occ(route_ctx.rr_node_route_inf[inode].occ() + 1);

            if (tptr->iswitch == OPEN) {
                tptr = tptr->next; /* Skip next segment. */
                if (tptr == nullptr)
                    break;
            }

            tptr = tptr->next;
        }
    }

    /* Now update the occupancy of each of the "locally used" OPINs on each CLB *
     * (CLB outputs used up by being directly wired to subblocks used only      *
     * locally).                                                                */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        for (iclass = 0; iclass < physical_tile_type(blk_id)->num_class; iclass++) {
            num_local_opins = route_ctx.clb_opins_used_locally[blk_id][iclass].size();
            /* Will always be 0 for pads or SINK classes. */
            for (ipin = 0; ipin < num_local_opins; ipin++) {
                inode = route_ctx.clb_opins_used_locally[blk_id][iclass][ipin];
                route_ctx.rr_node_route_inf[inode].set_occ(route_ctx.rr_node_route_inf[inode].occ() + 1);
            }
        }
    }
}

static void check_locally_used_clb_opins(const t_clb_opins_used& clb_opins_used_locally,
                                         enum e_route_type route_type) {
    /* Checks that enough OPINs on CLBs have been set aside (used up) to make a *
     * legal routing if subblocks connect to OPINs directly.                    */

    int iclass, num_local_opins, inode, ipin;
    t_rr_type rr_type;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        for (iclass = 0; iclass < physical_tile_type(blk_id)->num_class; iclass++) {
            num_local_opins = clb_opins_used_locally[blk_id][iclass].size();
            /* Always 0 for pads and for SINK classes */

            for (ipin = 0; ipin < num_local_opins; ipin++) {
                inode = clb_opins_used_locally[blk_id][iclass][ipin];
                check_node_and_range(inode, route_type); /* Node makes sense? */

                /* Now check that node is an OPIN of the right type. */

                rr_type = device_ctx.rr_nodes[inode].type();
                if (rr_type != OPIN) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "in check_locally_used_opins: block #%lu (%s)\n"
                                    "\tClass %d local OPIN is wrong rr_type -- rr_node #%d of type %d.\n",
                                    size_t(blk_id), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), iclass, inode, rr_type);
                }

                ipin = device_ctx.rr_nodes[inode].ptc_num();
                if (physical_tile_type(blk_id)->pin_class[ipin] != iclass) {
                    VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                    "in check_locally_used_opins: block #%lu (%s):\n"
                                    "\tExpected class %d local OPIN has class %d -- rr_node #: %d.\n",
                                    size_t(blk_id), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), iclass, physical_tile_type(blk_id)->pin_class[ipin], inode);
                }
            }
        }
    }
}

static void check_node_and_range(int inode, enum e_route_type route_type) {
    /* Checks that inode is within the legal range, then calls check_node to    *
     * check that everything else about the node is OK.                         */

    auto& device_ctx = g_vpr_ctx.device();

    if (inode < 0 || inode >= (int)device_ctx.rr_nodes.size()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in check_node_and_range: rr_node #%d is out of legal, range (0 to %d).\n", inode, device_ctx.rr_nodes.size() - 1);
    }
    check_rr_node(inode, route_type, device_ctx);
}

//Checks that the specified routing is legal with respect to non-configurable edges
//
//For routing to be legal if *any* non-configurable edge is used, so must *all*
//other non-configurable edges in the same set
static bool check_non_configurable_edges(ClusterNetId net, const t_non_configurable_rr_sets& non_configurable_rr_sets) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_trace* head = route_ctx.trace[net].head;

    //Collect all the edges used by this net's routing
    std::set<t_node_edge> routing_edges;
    std::set<int> routing_nodes;
    for (t_trace* trace = head; trace != nullptr; trace = trace->next) {
        int inode = trace->index;

        routing_nodes.insert(inode);

        if (trace->iswitch == OPEN) {
            continue; //End of branch
        } else if (trace->next) {
            int inode_next = trace->next->index;

            t_node_edge edge = {inode, inode_next};

            routing_edges.insert(edge);
        }
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
        std::vector<int> intersection;
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
                std::vector<int> difference;
                std::set_difference(rr_nodes.begin(), rr_nodes.end(),
                                    routing_nodes.begin(), routing_nodes.end(),
                                    std::back_inserter(difference));

                VTR_ASSERT(difference.size() > 0);
                std::string msg = vtr::string_fmt(
                    "Illegal routing for net '%s' (#%zu) some "
                    "required non-configurably connected nodes are missing:\n",
                    cluster_ctx.clb_nlist.net_name(net).c_str(), size_t(net));

                for (auto inode : difference) {
                    msg += vtr::string_fmt("  Missing %s\n", describe_rr_node(inode).c_str());
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
                                                  cluster_ctx.clb_nlist.net_name(net).c_str(), size_t(net));

                for (t_node_edge missing_edge : dedupped_difference) {
                    msg += vtr::string_fmt("  Expected RR Node: %d and RR Node: %d to be non-configurably connected, but edge missing from routing:\n",
                                           missing_edge.from_node, missing_edge.to_node);
                    msg += vtr::string_fmt("    %s\n", describe_rr_node(missing_edge.from_node).c_str());
                    msg += vtr::string_fmt("    %s\n", describe_rr_node(missing_edge.to_node).c_str());
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
    // Checks specified net for stubs, return true if at least one stub is
    // found.
    bool CheckNet(ClusterNetId net);

    // Returns set of stub nodes.
    const std::set<int>& stub_nodes() {
        return stub_nodes_;
    }

  private:
    bool RecurseTree(t_rt_node* rt_root);

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
void check_net_for_stubs(ClusterNetId net) {
    StubFinder stub_finder;

    bool any_stubs = stub_finder.CheckNet(net);
    if (any_stubs) {
        auto& cluster_ctx = g_vpr_ctx.clustering();
        std::string msg = vtr::string_fmt("Route tree for net '%s' (#%zu) contains stub branches rooted at:\n",
                                          cluster_ctx.clb_nlist.net_name(net).c_str(), size_t(net));
        for (int inode : stub_finder.stub_nodes()) {
            msg += vtr::string_fmt("    %s\n", describe_rr_node(inode).c_str());
        }

        VPR_THROW(VPR_ERROR_ROUTE, msg.c_str());
    }
}

bool StubFinder::CheckNet(ClusterNetId net) {
    stub_nodes_.clear();

    t_rt_node* rt_root = traceback_to_route_tree(net);
    RecurseTree(rt_root);
    free_route_tree(rt_root);

    return !stub_nodes_.empty();
}

// Returns true if this node is a stub.
bool StubFinder::RecurseTree(t_rt_node* rt_root) {
    auto& device_ctx = g_vpr_ctx.device();

    if (rt_root->u.child_list == nullptr) {
        //If a leaf of the route tree is not a SINK, then it is a stub
        if (device_ctx.rr_nodes[rt_root->inode].type() != SINK) {
            return true; //It is the current root of this stub
        } else {
            return false;
        }
    } else {
        bool is_stub = true;
        for (t_linked_rt_edge* edge = rt_root->u.child_list; edge != nullptr; edge = edge->next) {
            bool driver_switch_configurable = device_ctx.rr_switch_inf[edge->iswitch].configurable();

            bool child_is_stub = RecurseTree(edge->child);
            if (!child_is_stub) {
                // Because the child was not a stub, this node is not a stub.
                is_stub = false;
            } else if (driver_switch_configurable) {
                // This child was stub, and we drove it from a configurable
                // edge, this is an error.
                stub_nodes_.insert(edge->child->inode);
            }
        }

        return is_stub;
    }
}
