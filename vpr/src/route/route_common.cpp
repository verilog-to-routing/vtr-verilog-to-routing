#include <cstdio>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

#include "route_tree.h"
#include "vtr_assert.h"
#include "vtr_util.h"
#include "vtr_log.h"
#include "vtr_digest.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "stats.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_parallel.h"
#include "route_timing.h"
#include "place_and_route.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "read_xml_arch_file.h"
#include "draw.h"
#include "echo_files.h"
#include "atom_netlist_utils.h"

#include "route_profiling.h"

#include "timing_util.h"
#include "RoutingDelayCalculator.h"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"
#include "binary_heap.h"
#include "bucket.h"
#include "draw_global.h"

/*  The numbering relation between the channels and clbs is:				*
 *																	        *
 *  |    IO     | chan_   |   CLB     | chan_   |   CLB     |               *
 *  |grid[0][2] | y[0][2] |grid[1][2] | y[1][2] | grid[2][2]|               *
 *  +-----------+         +-----------+         +-----------+               *
 *                                                            } capacity in *
 *   No channel           chan_x[1][1]          chan_x[2][1]  } chan_width  *
 *                                                            } _x[1]       *
 *  +-----------+         +-----------+         +-----------+               *
 *  |           |  chan_  |           |  chan_  |           |               *
 *  |    IO     | y[0][1] |   CLB     | y[1][1] |   CLB     |               *
 *  |grid[0][1] |         |grid[1][1] |         |grid[2][1] |               *
 *  |           |         |           |         |           |               *
 *  +-----------+         +-----------+         +-----------+               *
 *                                                            } capacity in *
 *                        chan_x[1][0]          chan_x[2][0]  } chan_width  *
 *                                                            } _x[0]       *
 *                        +-----------+         +-----------+               *
 *                 No     |           |	   No   |           |               *
 *               Channel  |    IO     | Channel |    IO     |               *
 *                        |grid[1][0] |         |grid[2][0] |               *
 *                        |           |         |           |               *
 *                        +-----------+         +-----------+               *
 *                                                                          *
 *               {=======}              {=======}                           *
 *              Capacity in            Capacity in                          *
 *            chan_width_y[0]        chan_width_y[1]                        *
 *                                                                          */

/******************** Subroutines local to route_common.c *******************/
static vtr::vector<ParentNetId, std::vector<RRNodeId>> load_net_rr_terminals(const RRGraphView& rr_graph,
                                                                             const Netlist<>& net_list,
                                                                             bool is_flat);

static std::tuple<vtr::vector<ParentNetId, std::vector<std::vector<int>>>,
                  vtr::vector<ParentNetId, std::vector<int>>>
load_net_terminal_groups(const RRGraphView& rr_graph,
                         const Netlist<>& net_list,
                         const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                         bool is_flat);

static vtr::vector<ParentBlockId, std::vector<RRNodeId>> load_rr_clb_sources(const RRGraphView& rr_graph,
                                                                             const Netlist<>& net_list,
                                                                             bool is_flat);

static t_clb_opins_used alloc_and_load_clb_opins_used_locally();

static void adjust_one_rr_occ_and_acc_cost(RRNodeId inode, int add_or_sub, float acc_fac);

static vtr::vector<ParentNetId, uint8_t> load_is_clock_net(const Netlist<>& net_list,
                                                           bool is_flat);

static bool classes_in_same_block(ParentBlockId blk_id, int first_class_ptc_num, int second_class_ptc_num, bool is_flat);

/************************** Subroutine definitions ***************************/

void save_routing(vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                  const t_clb_opins_used& clb_opins_used_locally,
                  t_clb_opins_used& saved_clb_opins_used_locally) {
    auto& route_ctx = g_vpr_ctx.routing();

    best_routing = route_ctx.route_trees;

    /* Save which OPINs are locally used. */
    saved_clb_opins_used_locally = clb_opins_used_locally;
}

/* Empties route_ctx.current_rt and copies over best_routing onto it.
 * Also restores the locally used opin data. */
void restore_routing(vtr::vector<ParentNetId, vtr::optional<RouteTree>>& best_routing,
                     t_clb_opins_used& clb_opins_used_locally,
                     const t_clb_opins_used& saved_clb_opins_used_locally) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    route_ctx.route_trees = best_routing;

    /* Restore which OPINs are locally used. */
    clb_opins_used_locally = saved_clb_opins_used_locally;
}

/* This routine finds a "magic cookie" for the routing and prints it.    *
 * Use this number as a routing serial number to ensure that programming *
 * changes do not break the router.                                      */
void get_serial_num(const Netlist<>& net_list) {
    int serial_num;

    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    serial_num = 0;

    for (auto net_id : net_list.nets()) {
        if (!route_ctx.route_trees[net_id])
            continue;

        for (auto& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            RRNodeId inode = rt_node.inode;
            serial_num += (size_t(net_id) + 1)
                          * (rr_graph.node_xlow(inode) * (device_ctx.grid.width()) - rr_graph.node_yhigh(inode));

            serial_num -= rr_graph.node_ptc_num(inode) * (size_t(net_id) + 1) * 10;

            serial_num -= rr_graph.node_type(inode) * (size_t(net_id) + 1) * 100;
            serial_num %= 2000000000; /* Prevent overflow */
        }
    }
    VTR_LOG("Serial number (magic cookie) for the routing is: %d\n", serial_num);
}

void try_graph(int width_fac,
               const t_router_opts& router_opts,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_chan_width_dist chan_width_dist,
               t_direct_inf* directs,
               int num_directs,
               bool is_flat) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    t_graph_type graph_type;
    t_graph_type graph_directionality;
    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
        graph_directionality = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    /* Set the channel widths */
    t_chan_width chan_width = init_chan(width_fac, chan_width_dist, graph_directionality);

    /* Free any old routing graph, if one exists. */
    free_rr_graph();

    /* Set up the routing resource graph defined by this FPGA architecture. */
    int warning_count;
    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs, num_directs,
                    &warning_count,
                    is_flat);
}

bool try_route(const Netlist<>& net_list,
               int width_fac,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               NetPinsMatrix<float>& net_delay,
               std::shared_ptr<SetupHoldTimingInfo> timing_info,
               std::shared_ptr<RoutingDelayCalculator> delay_calc,
               t_chan_width_dist chan_width_dist,
               t_direct_inf* directs,
               int num_directs,
               ScreenUpdatePriority first_iteration_priority,
               bool is_flat) {
    /* Attempts a routing via an iterated maze router algorithm.  Width_fac *
     * specifies the relative width of the channels, while the members of   *
     * router_opts determine the value of the costs assigned to routing     *
     * resource node, etc.  det_routing_arch describes the detailed routing *
     * architecture (connection and switch boxes) of the FPGA; it is used   *
     * only if a DETAILED routing has been selected.                        */

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    t_graph_type graph_type;
    t_graph_type graph_directionality;
    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
        graph_directionality = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    /* Set the channel widths */
    t_chan_width chan_width = init_chan(width_fac, chan_width_dist, graph_directionality);

    /* Set up the routing resource graph defined by this FPGA architecture. */
    int warning_count;

    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs,
                    num_directs,
                    &warning_count,
                    is_flat);

    //Initialize drawing, now that we have an RR graph
    init_draw_coords(width_fac);

    bool success = true;

    /* Allocate and load additional rr_graph information needed only by the router. */
    alloc_and_load_rr_node_route_structs();

    init_route_structs(net_list,
                       router_opts.bb_factor,
                       router_opts.has_choking_spot,
                       is_flat);

    if (net_list.nets().empty()) {
        VTR_LOG_WARN("No nets to route\n");
    }

    if (router_opts.router_algorithm == PARALLEL) {
        VTR_LOG("Confirming router algorithm: PARALLEL.\n");

#ifdef VPR_USE_TBB
        auto& atom_ctx = g_vpr_ctx.atom();

        IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);
        ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, intra_lb_pb_pin_lookup);

        success = try_parallel_route(net_list,
                                     *det_routing_arch,
                                     router_opts,
                                     analysis_opts,
                                     segment_inf,
                                     net_delay,
                                     netlist_pin_lookup,
                                     timing_info,
                                     delay_calc,
                                     first_iteration_priority,
                                     is_flat);

        profiling::time_on_fanout_analysis();
#else
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "VPR was not compiled with TBB support required for parallel routing\n");
#endif

    } else { /* TIMING_DRIVEN route */
        VTR_LOG("Confirming router algorithm: TIMING_DRIVEN.\n");
        auto& atom_ctx = g_vpr_ctx.atom();

        IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);
        ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, intra_lb_pb_pin_lookup);
        success = try_timing_driven_route(net_list,
                                          *det_routing_arch,
                                          router_opts,
                                          analysis_opts,
                                          segment_inf,
                                          net_delay,
                                          netlist_pin_lookup,
                                          timing_info,
                                          delay_calc,
                                          first_iteration_priority,
                                          is_flat);

        profiling::time_on_fanout_analysis();
    }

    return (success);
}

bool feasible_routing() {
    /* This routine checks to see if this is a resource-feasible routing.      *
     * That is, are all rr_node capacity limitations respected?  It assumes    *
     * that the occupancy arrays are up to date when it is called.             */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    for (const RRNodeId& rr_id : rr_graph.nodes()) {
        if (route_ctx.rr_node_route_inf[rr_id].occ() > rr_graph.node_capacity(rr_id)) {
            return (false);
        }
    }

    return (true);
}

//Returns all RR nodes in the current routing which are congested
std::vector<RRNodeId> collect_congested_rr_nodes() {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    std::vector<RRNodeId> congested_rr_nodes;
    for (const RRNodeId inode : device_ctx.rr_graph.nodes()) {
        short occ = route_ctx.rr_node_route_inf[inode].occ();
        short capacity = rr_graph.node_capacity(inode);

        if (occ > capacity) {
            congested_rr_nodes.push_back(inode);
        }
    }

    return congested_rr_nodes;
}

/* Returns a vector from [0..device_ctx.rr_nodes.size()-1] containing the set
 * of nets using each RR node */
vtr::vector<RRNodeId, std::set<ClusterNetId>> collect_rr_node_nets() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    vtr::vector<RRNodeId, std::set<ClusterNetId>> rr_node_nets(device_ctx.rr_graph.num_nodes());
    for (ClusterNetId inet : cluster_ctx.clb_nlist.nets()) {
        if (!route_ctx.route_trees[inet])
            continue;
        for (auto& rt_node : route_ctx.route_trees[inet].value().all_nodes()) {
            rr_node_nets[rt_node.inode].insert(inet);
        }
    }
    return rr_node_nets;
}

void pathfinder_update_single_node_occupancy(RRNodeId inode, int add_or_sub) {
    /* Updates pathfinder's occupancy by either adding or removing the
     * usage of a resource node. */

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    int occ = route_ctx.rr_node_route_inf[inode].occ() + add_or_sub;
    route_ctx.rr_node_route_inf[inode].set_occ(occ);
    // can't have negative occupancy
    VTR_ASSERT(occ >= 0);
}

void pathfinder_update_acc_cost_and_overuse_info(float acc_fac, OveruseInfo& overuse_info) {
    /* This routine recomputes the acc_cost (accumulated congestion cost) of each       *
     * routing resource for the pathfinder algorithm after all nets have been routed.   *
     * It updates the accumulated cost to by adding in the number of extra signals      *
     * sharing a resource right now (i.e. after each complete iteration) times acc_fac. *
     * THIS ROUTINE ASSUMES THE OCCUPANCY VALUES IN RR_NODE ARE UP TO DATE.             *
     * This routine also creates a new overuse info for the current routing iteration.  */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    size_t overused_nodes = 0, total_overuse = 0, worst_overuse = 0;

    for (const RRNodeId& rr_id : rr_graph.nodes()) {
        int overuse = route_ctx.rr_node_route_inf[rr_id].occ() - rr_graph.node_capacity(rr_id);

        // If overused, update the acc_cost and add this node to the overuse info
        // If not, do nothing
        if (overuse > 0) {
            route_ctx.rr_node_route_inf[rr_id].acc_cost += overuse * acc_fac;

            ++overused_nodes;
            total_overuse += overuse;
            worst_overuse = std::max(worst_overuse, size_t(overuse));
        }
    }

    // Update overuse info
    overuse_info.overused_nodes = overused_nodes;
    overuse_info.total_overuse = total_overuse;
    overuse_info.worst_overuse = worst_overuse;
}

/** Update pathfinder cost of all nodes rooted at rt_node, including rt_node itself */
void pathfinder_update_cost_from_route_tree(const RouteTreeNode& root, int add_or_sub) {
    pathfinder_update_single_node_occupancy(root.inode, add_or_sub);
    for (auto& node : root.all_nodes()) {
        pathfinder_update_single_node_occupancy(node.inode, add_or_sub);
    }
}

float update_pres_fac(float new_pres_fac) {
    /* This routine should take the new value of the present congestion factor *
     * and propagate it to all the relevant data fields in the vpr flow.       *
     * Currently, it only updates the pres_fac used by the drawing functions   */
#ifndef NO_GRAPHICS

    // Only updates the drawing pres_fac if graphics is enabled
    get_draw_state_vars()->pres_fac = new_pres_fac;

#endif // NO_GRAPHICS

    return new_pres_fac;
}

/* Call this before you route any nets. It frees any old route trees and
 * sets the list of rr_nodes touched to empty. */
void init_route_structs(const Netlist<>& net_list,
                        int bb_factor,
                        bool has_choking_point,
                        bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    // Allocate and clear a new route_trees
    route_ctx.route_trees.resize(net_list.nets().size());
    std::fill(route_ctx.route_trees.begin(), route_ctx.route_trees.end(), vtr::nullopt);

    //Various look-ups
    route_ctx.net_rr_terminals = load_net_rr_terminals(device_ctx.rr_graph,
                                                       net_list,
                                                       is_flat);

    route_ctx.is_clock_net = load_is_clock_net(net_list, is_flat);
    route_ctx.route_bb = load_route_bb(net_list,
                                       bb_factor);
    route_ctx.rr_blk_source = load_rr_clb_sources(device_ctx.rr_graph,
                                                  net_list,
                                                  is_flat);
    route_ctx.clb_opins_used_locally = alloc_and_load_clb_opins_used_locally();
    route_ctx.net_status.resize(net_list.nets().size());

    if (has_choking_point && is_flat) {
        std::tie(route_ctx.net_terminal_groups, route_ctx.net_terminal_group_num) = load_net_terminal_groups(device_ctx.rr_graph,
                                                                                                             net_list,
                                                                                                             route_ctx.net_rr_terminals,
                                                                                                             is_flat);
    }
}

/* The routine sets the path_cost to HUGE_POSITIVE_FLOAT for  *
 * all channel segments touched by previous routing phases.    */
void reset_path_costs(const std::vector<RRNodeId>& visited_rr_nodes) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto node : visited_rr_nodes) {
        route_ctx.rr_node_route_inf[node].path_cost = std::numeric_limits<float>::infinity();
        route_ctx.rr_node_route_inf[node].backward_path_cost = std::numeric_limits<float>::infinity();
        route_ctx.rr_node_route_inf[node].prev_node = RRNodeId::INVALID();
        route_ctx.rr_node_route_inf[node].prev_edge = RREdgeId::INVALID();
    }
}

/* Returns the congestion cost of using this rr-node plus that of any      *
 * non-configurably connected rr_nodes that must be used when it is used.  */
float get_rr_cong_cost(RRNodeId inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    float cost = get_single_rr_cong_cost(inode, pres_fac);

    if (route_ctx.non_configurable_bitset.get(inode)) {
        // Access unordered_map only when the node is part of a non-configurable set
        auto itr = device_ctx.rr_node_to_non_config_node_set.find(inode);
        if (itr != device_ctx.rr_node_to_non_config_node_set.end()) {
            for (RRNodeId node : device_ctx.rr_non_config_node_sets[itr->second]) {
                if (node == inode) {
                    continue; //Already included above
                }

                cost += get_single_rr_cong_cost(node, pres_fac);
            }
        }
    }
    return (cost);
}

/* Mark all the SINKs of this net as targets by setting their target flags  *
 * to the number of times the net must connect to each SINK.  Note that     *
 * this number can occasionally be greater than 1 -- think of connecting   *
 * the same net to two inputs of an and-gate (and-gate inputs are logically *
 * equivalent, so both will connect to the same SINK).                      */
void mark_ends(const Netlist<>& net_list, ParentNetId net_id) {
    unsigned int ipin;
    RRNodeId inode;

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (ipin = 1; ipin < net_list.net_pins(net_id).size(); ipin++) {
        inode = route_ctx.net_rr_terminals[net_id][ipin];
        route_ctx.rr_node_route_inf[inode].target_flag++;
    }
}

/** like mark_ends, but only performs it for the remaining sinks of a net */
void mark_remaining_ends(ParentNetId net_id) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    const auto& tree = route_ctx.route_trees[net_id].value();

    for (int sink_pin : tree.get_remaining_isinks()) {
        RRNodeId inode = route_ctx.net_rr_terminals[net_id][sink_pin];
        ++route_ctx.rr_node_route_inf[inode].target_flag;
    }
}

//Calculates how many (and allocates space for) OPINs which must be reserved to
//respect 'instance' equivalence.
//
//TODO: At the moment this makes a significant simplifying assumption. It reserves
//      all OPINs not connected to external nets. This ensures each signal leaving
//      the logic block uses only a single OPIN. This is a safe, but pessmistic
//      behaviour, which prevents any logic duplication (e.g. duplicating LUTs/BLEs)
//      to drive multiple OPINs which could aid routability.
//
//      Note that this only applies for 'intance' equivalence. 'full' equivalence
//      does allow for multiple outputs to be used for a single signal.
static t_clb_opins_used alloc_and_load_clb_opins_used_locally() {
    /* Allocates and loads the data needed to make the router reserve some CLB  *
     * output pins for connections made locally within a CLB (if the netlist    *
     * specifies that this is necessary).                                       */

    t_clb_opins_used clb_opins_used_locally;
    int clb_pin, iclass;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    clb_opins_used_locally.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto type = physical_tile_type(blk_id);
        auto sub_tile = type->sub_tiles[get_sub_tile_index(blk_id)];

        auto class_range = get_class_range_for_block(blk_id);

        clb_opins_used_locally[blk_id].resize((int)type->class_inf.size());

        if (is_io_type(type)) continue;

        int pin_low = 0;
        int pin_high = 0;
        get_pin_range_for_block(blk_id, &pin_low, &pin_high);

        for (clb_pin = pin_low; clb_pin <= pin_high; clb_pin++) {
            auto net = cluster_ctx.clb_nlist.block_net(blk_id, clb_pin);

            if (!net || (net && cluster_ctx.clb_nlist.net_sinks(net).size() == 0)) {
                //There is no external net connected to this pin
                auto port_eq = get_port_equivalency_from_pin_physical_num(type, clb_pin);
                iclass = get_class_num_from_pin_physical_num(type, clb_pin);

                if (port_eq == PortEquivalence::INSTANCE) {
                    //The pin is part of an instance equivalent class, hence we need to reserve a pin

                    VTR_ASSERT(get_pin_type_from_pin_physical_num(type, clb_pin) == DRIVER);

                    /* Check to make sure class is in same range as that assigned to block */
                    VTR_ASSERT(iclass >= class_range.low && iclass <= class_range.high);

                    //We push back OPEN to reserve space to store the exact pin which
                    //will be reserved (determined later)
                    clb_opins_used_locally[blk_id][iclass].emplace_back(OPEN);
                }
            }
        }
    }

    return (clb_opins_used_locally);
}

/*the trace lists are only freed after use by the timing-driven placer */
/*Do not  free them after use by the router, since stats, and draw  */
/*routines use the trace values */
void free_route_structs() {
    /* Frees the temporary storage needed only during the routing.  The  *
     * final routing result is not freed.                                */
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (route_ctx.route_bb.size() != 0) {
        route_ctx.route_bb.clear();
        route_ctx.route_bb.shrink_to_fit();
    }
}

void alloc_and_load_rr_node_route_structs() {
    /* Allocates some extra information about each rr_node that is used only   *
     * during routing.                                                         */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();

    route_ctx.rr_node_route_inf.resize(device_ctx.rr_graph.num_nodes());
    route_ctx.non_configurable_bitset.resize(device_ctx.rr_graph.num_nodes());
    route_ctx.non_configurable_bitset.fill(false);

    reset_rr_node_route_structs();

    for (auto i : device_ctx.rr_node_to_non_config_node_set) {
        route_ctx.non_configurable_bitset.set(i.first, true);
    }
}

void reset_rr_node_route_structs() {
    /* Resets some extra information about each rr_node that is used only   *
     * during routing.                                                         */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(route_ctx.rr_node_route_inf.size() == size_t(device_ctx.rr_graph.num_nodes()));

    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        auto& node_inf = route_ctx.rr_node_route_inf[rr_id];

        node_inf.prev_node = RRNodeId::INVALID();
        node_inf.prev_edge = RREdgeId::INVALID();
        node_inf.acc_cost = 1.0;
        node_inf.path_cost = std::numeric_limits<float>::infinity();
        node_inf.backward_path_cost = std::numeric_limits<float>::infinity();
        node_inf.target_flag = 0;
        node_inf.set_occ(0);
    }
}

/* Allocates and loads the route_ctx.net_rr_terminals data structure. For each net it stores the rr_node   *
 * index of the SOURCE of the net and all the SINKs of the net [clb_nlist.nets()][clb_nlist.net_pins()].    *
 * Entry [inet][pnum] stores the rr index corresponding to the SOURCE (opin) or SINK (ipin) of the pin.     */
static vtr::vector<ParentNetId, std::vector<RRNodeId>> load_net_rr_terminals(const RRGraphView& rr_graph,
                                                                             const Netlist<>& net_list,
                                                                             bool is_flat) {
    vtr::vector<ParentNetId, std::vector<RRNodeId>> net_rr_terminals;

    net_rr_terminals.resize(net_list.nets().size());

    for (auto net_id : net_list.nets()) {
        net_rr_terminals[net_id].resize(net_list.net_pins(net_id).size());

        int pin_count = 0;
        for (auto pin_id : net_list.net_pins(net_id)) {
            auto block_id = net_list.pin_block(pin_id);

            t_block_loc blk_loc;
            blk_loc = get_block_loc(block_id, is_flat);
            int iclass = get_block_pin_class_num(block_id, pin_id, is_flat);
            RRNodeId inode = rr_graph.node_lookup().find_node(blk_loc.loc.layer,
                                                              blk_loc.loc.x,
                                                              blk_loc.loc.y,
                                                              (pin_count == 0 ? SOURCE : SINK), /* First pin is driver */
                                                              iclass);
            VTR_ASSERT(inode != RRNodeId::INVALID());
            net_rr_terminals[net_id][pin_count] = inode;
            pin_count++;
        }
    }

    return net_rr_terminals;
}

static std::tuple<vtr::vector<ParentNetId, std::vector<std::vector<int>>>,
                  vtr::vector<ParentNetId, std::vector<int>>>
load_net_terminal_groups(const RRGraphView& rr_graph,
                         const Netlist<>& net_list,
                         const vtr::vector<ParentNetId, std::vector<RRNodeId>>& net_rr_terminals,
                         bool is_flat) {
    vtr::vector<ParentNetId, std::vector<std::vector<int>>> net_terminal_groups;
    vtr::vector<ParentNetId, std::vector<int>> net_terminal_group_num;

    net_terminal_groups.resize(net_list.nets().size());
    net_terminal_group_num.resize(net_list.nets().size());

    for (auto net_id : net_list.nets()) {
        net_terminal_groups[net_id].reserve(net_list.net_pins(net_id).size());
        net_terminal_group_num[net_id].resize(net_list.net_pins(net_id).size(), -1);
        std::vector<ParentBlockId> net_pin_blk_id(net_list.net_pins(net_id).size(), ParentBlockId::INVALID());
        std::unordered_map<RRNodeId, int> rr_node_pin_num;
        int pin_count = 0;
        for (auto pin_id : net_list.net_pins(net_id)) {
            if (pin_count == 0) {
                pin_count++;
                continue;
            }
            RRNodeId rr_node_num = net_rr_terminals[net_id][pin_count];
            auto block_id = net_list.pin_block(pin_id);
            net_pin_blk_id[pin_count] = block_id;
            rr_node_pin_num[rr_node_num] = pin_count;

            t_block_loc blk_loc;
            blk_loc = get_block_loc(block_id, is_flat);
            int group_num = -1;
            for (int curr_grp_num = 0; curr_grp_num < (int)net_terminal_groups[net_id].size(); curr_grp_num++) {
                const auto& curr_grp = net_terminal_groups[net_id][curr_grp_num];
                auto group_loc = get_block_loc(net_pin_blk_id[rr_node_pin_num.at(RRNodeId(curr_grp[0]))], is_flat);
                if (blk_loc.loc == group_loc.loc) {
                    if (classes_in_same_block(block_id,
                                              rr_graph.node_ptc_num(RRNodeId(curr_grp[0])),
                                              rr_graph.node_ptc_num(net_rr_terminals[net_id][pin_count]),
                                              is_flat)) {
                        group_num = curr_grp_num;
                        break;
                    }
                }
            }

            if (group_num == -1) {
                /* TODO: net_terminal_groups cannot be fully RRNodeId - ified, because this code calls libarchfpga which 
                 * I think should not be aware of RRNodeIds. Fixing this requires some refactoring to lift the offending functions 
                 * into VPR. */
                std::vector<int> new_group = {int(size_t(rr_node_num))};
                int new_group_num = net_terminal_groups[net_id].size();
                net_terminal_groups[net_id].push_back(new_group);
                net_terminal_group_num[net_id][pin_count] = new_group_num;
            } else {
                net_terminal_groups[net_id][group_num].push_back(size_t(rr_node_num));
                net_terminal_group_num[net_id][pin_count] = group_num;
            }

            pin_count++;
        }
        net_terminal_groups[net_id].shrink_to_fit();
    }

    return std::make_tuple(net_terminal_groups, net_terminal_group_num);
}

/* Saves the rr_node corresponding to each SOURCE and SINK in each CLB      *
 * in the FPGA.  Currently only the SOURCE rr_node values are used, and     *
 * they are used only to reserve pins for locally used OPINs in the router. *
 * [0..cluster_ctx.clb_nlist.blocks().size()-1][0..num_class-1].            *
 * The values for blocks that are pads are NOT valid.                        */
static vtr::vector<ParentBlockId, std::vector<RRNodeId>> load_rr_clb_sources(const RRGraphView& rr_graph,
                                                                             const Netlist<>& net_list,
                                                                             bool is_flat) {
    vtr::vector<ParentBlockId, std::vector<RRNodeId>> rr_blk_source;

    t_rr_type rr_type;

    rr_blk_source.resize(net_list.blocks().size());

    for (auto blk_id : net_list.blocks()) {
        auto type = physical_tile_type(blk_id, is_flat);
        auto class_range = get_class_range_for_block(blk_id, is_flat);
        int num_tile_class = get_tile_class_max_ptc(type, is_flat);
        rr_blk_source[blk_id].resize(num_tile_class);
        for (int iclass = 0; iclass < num_tile_class; iclass++) {
            if (iclass >= class_range.low && iclass <= class_range.high) {
                t_block_loc blk_loc;
                blk_loc = get_block_loc(blk_id, is_flat);
                auto class_type = get_class_type_from_class_physical_num(type, iclass);
                if (class_type == DRIVER) {
                    rr_type = SOURCE;
                } else {
                    VTR_ASSERT(class_type == RECEIVER);
                    rr_type = SINK;
                }

                RRNodeId inode = rr_graph.node_lookup().find_node(blk_loc.loc.layer,
                                                                  blk_loc.loc.x,
                                                                  blk_loc.loc.y,
                                                                  rr_type,
                                                                  iclass);
                rr_blk_source[blk_id][iclass] = inode;
            } else {
                rr_blk_source[blk_id][iclass] = RRNodeId::INVALID();
            }
        }
    }

    return rr_blk_source;
}

static vtr::vector<ParentNetId, uint8_t> load_is_clock_net(const Netlist<>& net_list,
                                                           bool is_flat) {
    vtr::vector<ParentNetId, uint8_t> is_clock_net;

    auto& atom_ctx = g_vpr_ctx.atom();
    std::set<AtomNetId> clock_nets = find_netlist_physical_clock_nets(atom_ctx.nlist);

    is_clock_net.resize(net_list.nets().size());
    for (auto net_id : net_list.nets()) {
        std::size_t net_id_num = std::size_t(net_id);
        if (is_flat) {
            AtomNetId atom_net_id = AtomNetId(net_id_num);
            is_clock_net[net_id] = clock_nets.find(atom_net_id) != clock_nets.end();
        } else {
            ClusterNetId cluster_net_id = ClusterNetId(net_id_num);
            is_clock_net[net_id] = clock_nets.find(atom_ctx.lookup.atom_net(cluster_net_id)) != clock_nets.end();
        }
    }

    return is_clock_net;
}

static bool classes_in_same_block(ParentBlockId blk_id, int first_class_ptc_num, int second_class_ptc_num, bool is_flat) {
    t_physical_tile_type_ptr physical_tile = physical_tile_type(blk_id, is_flat);
    return classes_in_same_block(physical_tile, first_class_ptc_num, second_class_ptc_num, is_flat);
}

vtr::vector<ParentNetId, t_bb> load_route_bb(const Netlist<>& net_list,
                                             int bb_factor) {
    vtr::vector<ParentNetId, t_bb> route_bb;

    auto& route_ctx = g_vpr_ctx.routing();

    t_bb full_device_bounding_box;
    {
        auto& device_ctx = g_vpr_ctx.device();
        full_device_bounding_box.xmin = 0;
        full_device_bounding_box.ymin = 0;
        full_device_bounding_box.xmax = device_ctx.grid.width() - 1;
        full_device_bounding_box.ymax = device_ctx.grid.height() - 1;
    }

    auto nets = net_list.nets();
    route_bb.resize(nets.size());
    for (auto net_id : nets) {
        if (!route_ctx.is_clock_net[net_id]) {
            route_bb[net_id] = load_net_route_bb(net_list,
                                                 net_id,
                                                 bb_factor);
        } else {
            // Clocks should use a bounding box that includes the entire
            // fabric. This is because when a clock spine extends from a global
            // buffer point to the net target, the default bounding box
            // behavior may prevent the router from finding a path using the
            // clock network. This is not catastrophic if the only path to the
            // clock sink is via the clock network, because eventually the
            // heap will empty and the router will use the full part bounding
            // box anyways.
            //
            // However if there exists path that goes from the clock network
            // to the general interconnect and back, without leaving the
            // bounding box, the router will find it.  This could be a very
            // suboptimal route.
            //
            // It is safe to use the full device bounding box on clock nets
            // because clock networks tend to be specialized and have low
            // density.
            route_bb[net_id] = full_device_bounding_box;
        }
    }
    return route_bb;
}

t_bb load_net_route_bb(const Netlist<>& net_list,
                       ParentNetId net_id,
                       int bb_factor) {
    /*
     * This routine loads the bounding box used to limit the space
     * searched by the maze router when routing a specific net. The search is
     * limited to channels contained with the net bounding box expanded
     * by bb_factor channels on each side.  For example, if bb_factor is
     * 0, the maze router must route each net within its bounding box.
     * If bb_factor = max(device_ctx.grid.width()-1, device_cts.grid.height() - 1),
     * the maze router will search every channel in
     * the FPGA if necessary.  The bounding box returned by this routine
     * are different from the ones used by the placer in that they are
     * clipped to lie within (0,0) and (device_ctx.grid.width()-1,device_ctx.grid.height()-1)
     * rather than (1,1) and (device_ctx.grid.width()-1,device_ctx.grid.height()-1).
     */
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    //Ensure bb_factor is bounded by the device size
    //This ensures that if someone passes in an extremely large bb_factor
    //(e.g. std::numeric_limits<int>::max()) the later addition/subtraction
    //of bb_factor will not cause under/overflow
    int max_dim = std::max<int>(device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
    bb_factor = std::min(bb_factor, max_dim);

    RRNodeId driver_rr = RRNodeId(route_ctx.net_rr_terminals[net_id][0]);
    VTR_ASSERT(rr_graph.node_type(driver_rr) == SOURCE);

    VTR_ASSERT(rr_graph.node_xlow(driver_rr) <= rr_graph.node_xhigh(driver_rr));
    VTR_ASSERT(rr_graph.node_ylow(driver_rr) <= rr_graph.node_yhigh(driver_rr));

    int xmin = rr_graph.node_xlow(driver_rr);
    int ymin = rr_graph.node_ylow(driver_rr);
    int xmax = rr_graph.node_xhigh(driver_rr);
    int ymax = rr_graph.node_yhigh(driver_rr);

    auto net_sinks = net_list.net_sinks(net_id);
    for (size_t ipin = 1; ipin < net_sinks.size() + 1; ++ipin) { //Start at 1 since looping through sinks
        RRNodeId sink_rr = RRNodeId(route_ctx.net_rr_terminals[net_id][ipin]);
        VTR_ASSERT(rr_graph.node_type(sink_rr) == SINK);

        VTR_ASSERT(rr_graph.node_xlow(sink_rr) <= rr_graph.node_xhigh(sink_rr));
        VTR_ASSERT(rr_graph.node_ylow(sink_rr) <= rr_graph.node_yhigh(sink_rr));

        xmin = std::min<int>(xmin, rr_graph.node_xlow(sink_rr));
        xmax = std::max<int>(xmax, rr_graph.node_xhigh(sink_rr));
        ymin = std::min<int>(ymin, rr_graph.node_ylow(sink_rr));
        ymax = std::max<int>(ymax, rr_graph.node_yhigh(sink_rr));
    }

    /* Want the channels on all 4 sides to be usuable, even if bb_factor = 0. */
    xmin -= 1;
    ymin -= 1;

    /* Expand the net bounding box by bb_factor, then clip to the physical *
     * chip area.                                                          */

    t_bb bb;

    bb.xmin = std::max<int>(xmin - bb_factor, 0);
    bb.xmax = std::min<int>(xmax + bb_factor, device_ctx.grid.width() - 1);
    bb.ymin = std::max<int>(ymin - bb_factor, 0);
    bb.ymax = std::min<int>(ymax + bb_factor, device_ctx.grid.height() - 1);

    return bb;
}

void add_to_mod_list(RRNodeId inode, std::vector<RRNodeId>& modified_rr_node_inf) {
    auto& route_ctx = g_vpr_ctx.routing();

    if (std::isinf(route_ctx.rr_node_route_inf[inode].path_cost)) {
        modified_rr_node_inf.push_back(inode);
    }
}

//To ensure the router can only swap pins which are actually logically equivalent, some block output pins must be
//reserved in certain cases.
//
// In the RR graph, output pin equivalence is modelled by a single SRC node connected to (multiple) OPINs, modelling
// that each of the OPINs is logcially equivalent (i.e. it doesn't matter through which the router routes a signal,
// so long as it is from the appropriate SRC).
//
// This correctly models 'full' equivalence (e.g. if there is a full crossbar between the outputs), but is too
// optimistic for 'instance' equivalence (which typcially models the pin equivalence possible by swapping sub-block
// instances like BLEs). In particular, for the 'instance' equivalence case, some of the 'equivalent' block outputs
// may be used by internal signals which are routed entirely *within* the block (i.e. the signals which never leave
// the block). These signals effectively 'use-up' an output pin which should now be unavailable to the router.
//
// To model this we 'reserve' these locally used outputs, ensuring that the router will not use them (as if it did
// this would equate to duplicating a BLE into an already in-use BLE instance, which is clearly incorrect).
void reserve_locally_used_opins(HeapInterface* heap, float pres_fac, float acc_fac, bool rip_up_local_opins, bool is_flat) {
    VTR_ASSERT(is_flat == false);
    int num_local_opin, iconn, num_edges;
    int iclass, ipin;
    float cost;
    t_heap* heap_head_ptr;
    t_physical_tile_type_ptr type;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (rip_up_local_opins) {
        for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
            type = physical_tile_type(blk_id);
            for (iclass = 0; iclass < (int)type->class_inf.size(); iclass++) {
                num_local_opin = route_ctx.clb_opins_used_locally[blk_id][iclass].size();

                if (num_local_opin == 0) continue;
                auto port_eq = get_port_equivalency_from_class_physical_num(type, iclass);
                VTR_ASSERT(port_eq == PortEquivalence::INSTANCE);

                /* Always 0 for pads and for RECEIVER (IPIN) classes */
                for (ipin = 0; ipin < num_local_opin; ipin++) {
                    RRNodeId inode = route_ctx.clb_opins_used_locally[blk_id][iclass][ipin];
                    VTR_ASSERT(inode && size_t(inode) < rr_graph.num_nodes());
                    adjust_one_rr_occ_and_acc_cost(inode, -1, acc_fac);
                }
            }
        }
    }

    // Make sure heap is empty before we add nodes to the heap.
    heap->empty_heap();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        type = physical_tile_type(blk_id);
        for (iclass = 0; iclass < (int)type->class_inf.size(); iclass++) {
            num_local_opin = route_ctx.clb_opins_used_locally[blk_id][iclass].size();

            if (num_local_opin == 0) continue;

            auto class_eq = get_port_equivalency_from_class_physical_num(type, iclass);
            VTR_ASSERT(class_eq == PortEquivalence::INSTANCE);

            //From the SRC node we walk through it's out going edges to collect the
            //OPIN nodes. We then push them onto a heap so the OPINs with lower
            //congestion cost are popped-off/reserved first. (Intuitively, we want
            //the reserved OPINs to move out of the way of congestion, by preferring
            //to reserve OPINs with lower congestion costs).
            RRNodeId from_node = route_ctx.rr_blk_source[(const ParentBlockId&)blk_id][iclass];
            num_edges = rr_graph.num_edges(RRNodeId(from_node));
            for (iconn = 0; iconn < num_edges; iconn++) {
                RRNodeId to_node = rr_graph.edge_sink_node(RRNodeId(from_node), iconn);

                VTR_ASSERT(rr_graph.node_type(RRNodeId(to_node)) == OPIN);

                //Add the OPIN to the heap according to it's congestion cost
                cost = get_rr_cong_cost(to_node, pres_fac);
                add_node_to_heap(heap, route_ctx.rr_node_route_inf,
                                 to_node, cost, RRNodeId::INVALID(), RREdgeId::INVALID(),
                                 0., 0.);
            }

            for (ipin = 0; ipin < num_local_opin; ipin++) {
                //Pop the nodes off the heap. We get them from the heap so we
                //reserve those pins with lowest congestion cost first.
                heap_head_ptr = heap->get_heap_head();
                RRNodeId inode(heap_head_ptr->index);

                VTR_ASSERT(rr_graph.node_type(inode) == OPIN);

                adjust_one_rr_occ_and_acc_cost(inode, 1, acc_fac);
                route_ctx.clb_opins_used_locally[blk_id][iclass][ipin] = inode;
                heap->free(heap_head_ptr);
            }

            heap->empty_heap();
        }
    }
}

static void adjust_one_rr_occ_and_acc_cost(RRNodeId inode, int add_or_sub, float acc_fac) {
    /* Increments or decrements (depending on add_or_sub) the occupancy of    *
     * one rr_node, and adjusts the present cost of that node appropriately.  */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int new_occ = route_ctx.rr_node_route_inf[inode].occ() + add_or_sub;
    int capacity = rr_graph.node_capacity(inode);
    route_ctx.rr_node_route_inf[inode].set_occ(new_occ);

    if (new_occ < capacity) {
    } else {
        if (add_or_sub == 1) {
            route_ctx.rr_node_route_inf[inode].acc_cost += (new_occ - capacity) * acc_fac;
        }
    }
}

//Print information about an invalid routing, caused by overused routing resources
void print_invalid_routing_info(const Netlist<>& net_list, bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    //Build a look-up of nets using each RR node
    std::multimap<RRNodeId, ParentNetId> rr_node_nets;

    for (auto net_id : net_list.nets()) {
        if (!route_ctx.route_trees[net_id])
            continue;

        for (auto& rt_node : route_ctx.route_trees[net_id].value().all_nodes()) {
            rr_node_nets.emplace(size_t(rt_node.inode), net_id);
        }
    }

    for (const RRNodeId inode : device_ctx.rr_graph.nodes()) {
        int node_x, node_y;
        node_x = rr_graph.node_xlow(inode);
        node_y = rr_graph.node_ylow(inode);

        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int cap = rr_graph.node_capacity(inode);
        if (occ > cap) {
            VTR_LOG("  %s is overused (occ=%d capacity=%d)\n", describe_rr_node(rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, inode, is_flat).c_str(), occ, cap);

            auto range = rr_node_nets.equal_range(inode);
            for (auto itr = range.first; itr != range.second; ++itr) {
                auto net_id = itr->second;
                VTR_LOG("    Used by net %s (%zu)\n", net_list.net_name(net_id).c_str(), size_t(net_id));
                for (auto pin : net_list.net_pins(net_id)) {
                    t_block_loc blk_loc;
                    auto blk = net_list.pin_block(pin);
                    blk_loc = get_block_loc(blk, is_flat);
                    if (blk_loc.loc.x == node_x && blk_loc.loc.y == node_y) {
                        VTR_LOG("      Is in the same cluster: %s \n", describe_rr_node(rr_graph, device_ctx.grid,
                                                                                        device_ctx.rr_indexed_data, itr->first, is_flat)
                                                                           .c_str());
                    }
                }
            }
        }
    }
}

void print_rr_node_route_inf() {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    for (size_t inode = 0; inode < route_ctx.rr_node_route_inf.size(); ++inode) {
        const auto& inf = route_ctx.rr_node_route_inf[RRNodeId(inode)];
        if (!std::isinf(inf.path_cost)) {
            RRNodeId prev_node = inf.prev_node;
            RREdgeId prev_edge = inf.prev_edge;
            auto switch_id = rr_graph.rr_nodes().edge_switch(prev_edge);
            VTR_LOG("rr_node: %d prev_node: %d prev_edge: %zu",
                    inode, prev_node, (size_t)prev_edge);

            if (prev_node.is_valid() && bool(prev_edge) && !rr_graph.rr_switch_inf(RRSwitchId(switch_id)).configurable()) {
                VTR_LOG("*");
            }

            VTR_LOG(" pcost: %g back_pcost: %g\n",
                    inf.path_cost, inf.backward_path_cost);
        }
    }
}

void print_rr_node_route_inf_dot() {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_LOG("digraph G {\n");
    VTR_LOG("\tnode[shape=record]\n");
    for (size_t inode = 0; inode < route_ctx.rr_node_route_inf.size(); ++inode) {
        const auto& inf = route_ctx.rr_node_route_inf[RRNodeId(inode)];
        if (!std::isinf(inf.path_cost)) {
            VTR_LOG("\tnode%zu[label=\"{%zu (%s)", inode, inode, rr_graph.node_type_string(RRNodeId(inode)));
            if (inf.occ() > rr_graph.node_capacity(RRNodeId(inode))) {
                VTR_LOG(" x");
            }
            VTR_LOG("}\"]\n");
        }
    }
    for (size_t inode = 0; inode < route_ctx.rr_node_route_inf.size(); ++inode) {
        const auto& inf = route_ctx.rr_node_route_inf[RRNodeId(inode)];
        if (!std::isinf(inf.path_cost)) {
            RRNodeId prev_node = inf.prev_node;
            RREdgeId prev_edge = inf.prev_edge;
            auto switch_id = rr_graph.rr_nodes().edge_switch(prev_edge);

            if (prev_node.is_valid() && prev_edge.is_valid()) {
                VTR_LOG("\tnode%d -> node%zu [", prev_node, inode);
                if (rr_graph.rr_switch_inf(RRSwitchId(switch_id)).configurable()) {
                    VTR_LOG("label=\"*\"");
                }

                VTR_LOG("];\n");
            }
        }
    }

    VTR_LOG("}\n");
}

std::string describe_unrouteable_connection(RRNodeId source_node, RRNodeId sink_node, bool is_flat) {
    const auto& device_ctx = g_vpr_ctx.device();
    std::string msg = vtr::string_fmt(
        "Cannot route from %s (%s) to "
        "%s (%s) -- no possible path",
        rr_node_arch_name(source_node, is_flat).c_str(),
        describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, source_node, is_flat).c_str(),
        rr_node_arch_name(sink_node, is_flat).c_str(),
        describe_rr_node(device_ctx.rr_graph, device_ctx.grid, device_ctx.rr_indexed_data, sink_node, is_flat).c_str());

    return msg;
}

float get_cost_from_lookahead(const RouterLookahead& router_lookahead,
                              const RRGraphView& /*rr_graph_view*/,
                              RRNodeId from_node,
                              RRNodeId to_node,
                              float R_upstream,
                              const t_conn_cost_params cost_params,
                              bool /*is_flat*/) {
    return router_lookahead.get_expected_cost(from_node, to_node, cost_params, R_upstream);
}
