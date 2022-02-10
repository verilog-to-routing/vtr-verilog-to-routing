#include <cstdio>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

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
#include "route_tree_timing.h"
#include "route_timing.h"
#include "route_breadth_first.h"
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

/**************** Types local to route_common.c ******************/
struct t_trace_branch {
    t_trace* head;
    t_trace* tail;
};

/**************** Static variables local to route_common.c ******************/

/* For managing my own list of currently free trace data structures.    */
static t_trace* trace_free_head = nullptr;
/* For keeping track of the sudo malloc memory for the trace*/
static vtr::t_chunk trace_ch;

static int num_trace_allocated = 0; /* To watch for memory leaks. */
static int num_linked_f_pointer_allocated = 0;

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
static t_trace_branch traceback_branch(int node, int target_net_pin_index, std::unordered_set<int>& main_branch_visited);
static std::pair<t_trace*, t_trace*> add_trace_non_configurable(t_trace* head, t_trace* tail, int node, std::unordered_set<int>& visited);
static std::pair<t_trace*, t_trace*> add_trace_non_configurable_recurr(int node, std::unordered_set<int>& visited, int depth = 0);

static vtr::vector<ClusterNetId, std::vector<int>> load_net_rr_terminals(const RRGraphView& rr_graph);
static vtr::vector<ClusterBlockId, std::vector<int>> load_rr_clb_sources(const RRGraphView& rr_graph);

static t_clb_opins_used alloc_and_load_clb_opins_used_locally();
static void adjust_one_rr_occ_and_acc_cost(int inode, int add_or_sub, float acc_fac);

bool validate_traceback_recurr(t_trace* trace, std::set<int>& seen_rr_nodes);
static bool validate_trace_nodes(t_trace* head, const std::unordered_set<int>& trace_nodes);
static vtr::vector<ClusterNetId, uint8_t> load_is_clock_net();

/************************** Subroutine definitions ***************************/

void save_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing,
                  const t_clb_opins_used& clb_opins_used_locally,
                  t_clb_opins_used& saved_clb_opins_used_locally) {
    /* This routing frees any routing currently held in best routing,       *
     * then copies over the current routing (held in route_ctx.trace), and       *
     * finally sets route_ctx.trace_head and route_ctx.trace_tail to all NULLs so that the      *
     * connection to the saved routing is broken.  This is necessary so     *
     * that the next iteration of the router does not free the saved        *
     * routing elements.  Also saves any data about locally used clb_opins, *
     * since this is also part of the routing.                              */

    t_trace *tptr, *tempptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        /* Free any previously saved routing.  It is no longer best. */
        tptr = best_routing[net_id];
        while (tptr != nullptr) {
            tempptr = tptr->next;
            free_trace_data(tptr);
            tptr = tempptr;
        }

        /* Save a pointer to the current routing in best_routing. */
        best_routing[net_id] = route_ctx.trace[net_id].head;

        /* Set the current (working) routing to NULL so the current trace       *
         * elements won't be reused by the memory allocator.                    */

        route_ctx.trace[net_id].head = nullptr;
        route_ctx.trace[net_id].tail = nullptr;
        route_ctx.trace_nodes[net_id].clear();
    }

    /* Save which OPINs are locally used.                           */
    saved_clb_opins_used_locally = clb_opins_used_locally;
}

/* Deallocates any current routing in route_ctx.trace_head, and replaces it with    *
 * the routing in best_routing.  Best_routing is set to NULL to show that *
 * it no longer points to a valid routing.  NOTE:  route_ctx.trace_tail is not      *
 * restored -- it is set to all NULLs since it is only used in            *
 * update_traceback.  If you need route_ctx.trace_tail restored, modify this        *
 * routine.  Also restores the locally used opin data.                    */
void restore_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing,
                     t_clb_opins_used& clb_opins_used_locally,
                     const t_clb_opins_used& saved_clb_opins_used_locally) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        /* Free any current routing. */
        pathfinder_update_path_occupancy(route_ctx.trace[net_id].head, -1);
        free_traceback(net_id);

        /* Set the current routing to the saved one. */
        route_ctx.trace[net_id].head = best_routing[net_id];
        pathfinder_update_path_occupancy(route_ctx.trace[net_id].head, 1);
        best_routing[net_id] = nullptr; /* No stored routing. */
    }

    /* Restore which OPINs are locally used.                           */
    clb_opins_used_locally = saved_clb_opins_used_locally;
}

/* This routine finds a "magic cookie" for the routing and prints it.    *
 * Use this number as a routing serial number to ensure that programming *
 * changes do not break the router.                                      */
void get_serial_num() {
    int serial_num, inode;
    t_trace* tptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    serial_num = 0;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        /* Global nets will have null trace_heads (never routed) so they *
         * are not included in the serial number calculation.            */

        tptr = route_ctx.trace[net_id].head;
        while (tptr != nullptr) {
            inode = tptr->index;
            serial_num += (size_t(net_id) + 1)
                          * (rr_graph.node_xlow(RRNodeId(inode)) * (device_ctx.grid.width()) - rr_graph.node_yhigh(RRNodeId(inode)));

            serial_num -= rr_graph.node_ptc_num(RRNodeId(inode)) * (size_t(net_id) + 1) * 10;

            serial_num -= rr_graph.node_type(RRNodeId(inode)) * (size_t(net_id) + 1) * 100;
            serial_num %= 2000000000; /* Prevent overflow */
            tptr = tptr->next;
        }
    }
    VTR_LOG("Serial number (magic cookie) for the routing is: %d\n", serial_num);
}

void try_graph(int width_fac, const t_router_opts& router_opts, t_det_routing_arch* det_routing_arch, std::vector<t_segment_inf>& segment_inf, t_chan_width_dist chan_width_dist, t_direct_inf* directs, int num_directs) {
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
                    device_ctx.num_arch_switches,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs, num_directs,
                    &warning_count);
}

bool try_route(int width_fac,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               ClbNetPinsMatrix<float>& net_delay,
               std::shared_ptr<SetupHoldTimingInfo> timing_info,
               std::shared_ptr<RoutingDelayCalculator> delay_calc,
               t_chan_width_dist chan_width_dist,
               t_direct_inf* directs,
               int num_directs,
               ScreenUpdatePriority first_iteration_priority) {
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
                    device_ctx.num_arch_switches,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs, num_directs,
                    &warning_count);

    //Initialize drawing, now that we have an RR graph
    init_draw_coords(width_fac);

    bool success = true;

    /* Allocate and load additional rr_graph information needed only by the router. */
    alloc_and_load_rr_node_route_structs();

    init_route_structs(router_opts.bb_factor);

    if (cluster_ctx.clb_nlist.nets().empty()) {
        VTR_LOG_WARN("No nets to route\n");
    }

    if (router_opts.router_algorithm == BREADTH_FIRST) {
        VTR_LOG("Confirming router algorithm: BREADTH_FIRST.\n");
        success = try_breadth_first_route(router_opts);
    } else { /* TIMING_DRIVEN route */
        VTR_LOG("Confirming router algorithm: TIMING_DRIVEN.\n");
        auto& atom_ctx = g_vpr_ctx.atom();

        IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);
        ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, intra_lb_pb_pin_lookup);

        success = try_timing_driven_route(
            router_opts,
            analysis_opts,
            segment_inf,
            net_delay,
            netlist_pin_lookup,
            timing_info,
            delay_calc,
            first_iteration_priority);

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
        if (route_ctx.rr_node_route_inf[(size_t)rr_id].occ() > rr_graph.node_capacity(rr_id)) {
            return (false);
        }
    }

    return (true);
}

//Returns all RR nodes in the current routing which are congested
std::vector<int> collect_congested_rr_nodes() {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    std::vector<int> congested_rr_nodes;
    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        short occ = route_ctx.rr_node_route_inf[(size_t)rr_id].occ();
        short capacity = rr_graph.node_capacity(rr_id);

        if (occ > capacity) {
            congested_rr_nodes.push_back((size_t)rr_id);
        }
    }

    return congested_rr_nodes;
}

/* Returns a vector from [0..device_ctx.rr_nodes.size()-1] containing the set
 * of nets using each RR node */
std::vector<std::set<ClusterNetId>> collect_rr_node_nets() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::vector<std::set<ClusterNetId>> rr_node_nets(device_ctx.rr_graph.num_nodes());
    for (ClusterNetId inet : cluster_ctx.clb_nlist.nets()) {
        t_trace* trace_elem = route_ctx.trace[inet].head;
        while (trace_elem) {
            int rr_node = trace_elem->index;

            rr_node_nets[rr_node].insert(inet);

            trace_elem = trace_elem->next;
        }
    }
    return rr_node_nets;
}

void pathfinder_update_path_occupancy(t_trace* route_segment_start, int add_or_sub) {
    /* This routine updates the occupancy of the rr_nodes that are affected by
     * the portion of the routing of one net that starts at route_segment_start.
     * If route_segment_start is route_ctx.trace[net_id].head, the
     * occupancy of all the nodes in the routing of net net_id are updated.
     * If add_or_sub is -1 the net (or net portion) is ripped up,
     * if it is 1, the net is added to the routing.
     */

    t_trace* tptr;

    tptr = route_segment_start;
    if (tptr == nullptr) /* No routing yet. */
        return;

    for (;;) {
        pathfinder_update_single_node_occupancy(tptr->index, add_or_sub);

        if (tptr->iswitch == OPEN) { //End of branch
            tptr = tptr->next;       /* Skip next segment. */
            if (tptr == nullptr)
                break;
        }

        tptr = tptr->next;

    } /* End while loop -- did an entire traceback. */
}

void pathfinder_update_single_node_occupancy(int inode, int add_or_sub) {
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
        int overuse = route_ctx.rr_node_route_inf[(size_t)rr_id].occ() - rr_graph.node_capacity(rr_id);

        // If overused, update the acc_cost and add this node to the overuse info
        // If not, do nothing
        if (overuse > 0) {
            route_ctx.rr_node_route_inf[(size_t)rr_id].acc_cost += overuse * acc_fac;

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

/* Call this before you route any nets.  It frees any old traceback and   *
 * sets the list of rr_nodes touched to empty.                            */
void init_route_structs(int bb_factor) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    //Free any old tracebacks
    for (auto net_id : cluster_ctx.clb_nlist.nets())
        free_traceback(net_id);

    //Allocate new tracebacks
    route_ctx.trace.resize(cluster_ctx.clb_nlist.nets().size());
    route_ctx.trace_nodes.resize(cluster_ctx.clb_nlist.nets().size());

    //Various look-ups
    route_ctx.net_rr_terminals = load_net_rr_terminals(device_ctx.rr_graph);
    route_ctx.is_clock_net = load_is_clock_net();
    route_ctx.route_bb = load_route_bb(bb_factor);
    route_ctx.rr_blk_source = load_rr_clb_sources(device_ctx.rr_graph);
    route_ctx.clb_opins_used_locally = alloc_and_load_clb_opins_used_locally();
    route_ctx.net_status.resize(cluster_ctx.clb_nlist.nets().size());
}

/* This routine adds the most recently finished wire segment to the         *
 * traceback linked list.  The first connection starts with the net SOURCE  *
 * and begins at the structure pointed to by route_ctx.trace[net_id].head.  *
 * Each connection ends with a SINK.  After each SINK, the next connection  *
 * begins (if the net has more than 2 pins).  The first element after the   *
 * SINK gives the routing node on a previous piece of the routing, which is *
 * the link from the existing net to this new piece of the net.             *
 * In each traceback I start at the end of a path, which is a SINK with     *
 * target_net_pin_index (net pin index corresponding to the SINK, ranging   *
 * from 1 to fanout), and trace back through its predecessors to the        *
 * beginning.  I have stored information on the predecesser of each node to *
 * make traceback easy -- this sacrificies some memory for easier code      *
 * maintenance.  This routine returns a pointer to the first "new" node in  *
 * the traceback (node not previously in trace).                            */
t_trace* update_traceback(t_heap* hptr, int target_net_pin_index, ClusterNetId net_id) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    auto& trace_nodes = route_ctx.trace_nodes[net_id];

    VTR_ASSERT_SAFE(validate_trace_nodes(route_ctx.trace[net_id].head, trace_nodes));

    t_trace_branch branch = traceback_branch(hptr->index, target_net_pin_index, trace_nodes);

    VTR_ASSERT_SAFE(validate_trace_nodes(branch.head, trace_nodes));

    t_trace* ret_ptr = nullptr;
    if (route_ctx.trace[net_id].tail != nullptr) {
        route_ctx.trace[net_id].tail->next = branch.head; /* Traceback ends with tptr */
        ret_ptr = branch.head->next;                      /* First new segment.       */
    } else {                                              /* This was the first "chunk" of the net's routing */
        route_ctx.trace[net_id].head = branch.head;
        ret_ptr = branch.head; /* Whole traceback is new. */
    }

    route_ctx.trace[net_id].tail = branch.tail;
    return (ret_ptr);
}

//Traces back a new routing branch starting from the specified SINK 'node' with target_net_pin_index, which is the
//net pin index corresponding to the SINK (ranging from 1 to fanout), and working backwards to any existing routing.
//Returns the new branch, and also updates trace_nodes for any new nodes which are included in the branches traceback.
static t_trace_branch traceback_branch(int node, int target_net_pin_index, std::unordered_set<int>& trace_nodes) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    auto rr_type = rr_graph.node_type(RRNodeId(node));
    if (rr_type != SINK) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                        "in traceback_branch: Expected type = SINK (%d).\n");
    }

    //We construct the main traceback by walking from the given node back to the source,
    //according to the previous edges/nodes recorded in rr_node_route_inf by the router.
    t_trace* branch_head = alloc_trace_data();
    t_trace* branch_tail = branch_head;
    branch_head->index = node;
    branch_head->net_pin_index = target_net_pin_index; //The first node is the SINK node, so store its net pin index
    branch_head->iswitch = OPEN;
    branch_head->next = nullptr;

    trace_nodes.insert(node);

    std::vector<int> new_nodes_added_to_traceback = {node};

    auto iedge = route_ctx.rr_node_route_inf[node].prev_edge;
    int inode = route_ctx.rr_node_route_inf[node].prev_node;

    while (inode != NO_PREVIOUS) {
        //Add the current node to the head of traceback
        t_trace* prev_ptr = alloc_trace_data();
        prev_ptr->index = inode;
        prev_ptr->net_pin_index = OPEN; //Net pin index is invalid for Non-SINK nodes
        prev_ptr->iswitch = rr_graph.rr_nodes().edge_switch(iedge);

        prev_ptr->next = branch_head;
        branch_head = prev_ptr;

        if (trace_nodes.count(inode)) {
            break; //Connected to existing routing
        }

        trace_nodes.insert(inode); //Record this node as visited
        new_nodes_added_to_traceback.push_back(inode);

        iedge = route_ctx.rr_node_route_inf[inode].prev_edge;
        inode = route_ctx.rr_node_route_inf[inode].prev_node;
    }

    //We next re-expand all the main-branch nodes to add any non-configurably connected side branches
    // We are careful to do this *after* the main branch is constructed to ensure nodes which are both
    // non-configurably connected *and* part of the main branch are only added to the traceback once.
    for (int new_node : new_nodes_added_to_traceback) {
        //Expand each main branch node
        std::tie(branch_head, branch_tail) = add_trace_non_configurable(branch_head, branch_tail, new_node, trace_nodes);
    }

    return {branch_head, branch_tail};
}

//Traces any non-configurable subtrees from branch_head, returning the new branch_head and updating trace_nodes
//
//This effectively does a depth-first traversal
static std::pair<t_trace*, t_trace*> add_trace_non_configurable(t_trace* head, t_trace* tail, int node, std::unordered_set<int>& trace_nodes) {
    //Trace any non-configurable subtrees
    t_trace* subtree_head = nullptr;
    t_trace* subtree_tail = nullptr;
    std::tie(subtree_head, subtree_tail) = add_trace_non_configurable_recurr(node, trace_nodes);

    //Add any non-empty subtree to tail of traceback
    if (subtree_head && subtree_tail) {
        if (!head) { //First subtree becomes head
            head = subtree_head;
        } else { //Later subtrees added to tail
            VTR_ASSERT(tail);
            tail->next = subtree_head;
        }

        tail = subtree_tail;
    } else {
        VTR_ASSERT(subtree_head == nullptr && subtree_tail == nullptr);
    }

    return {head, tail};
}

//Recursive helper function for add_trace_non_configurable()
static std::pair<t_trace*, t_trace*> add_trace_non_configurable_recurr(int node, std::unordered_set<int>& trace_nodes, int depth) {
    t_trace* head = nullptr;
    t_trace* tail = nullptr;

    //Record the non-configurable out-going edges
    std::vector<t_edge_size> unvisited_non_configurable_edges;
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    for (auto iedge : rr_graph.non_configurable_edges(RRNodeId(node))) {
        VTR_ASSERT_SAFE(!rr_graph.edge_is_configurable(RRNodeId(node), iedge));

        int to_node = size_t(rr_graph.edge_sink_node(RRNodeId(node), iedge));

        if (!trace_nodes.count(to_node)) {
            unvisited_non_configurable_edges.push_back(iedge);
        }
    }

    if (unvisited_non_configurable_edges.size() == 0) {
        //Base case: leaf node with no non-configurable edges
        if (depth > 0) { //Arrived via non-configurable edge
            VTR_ASSERT(!trace_nodes.count(node));
            head = alloc_trace_data();
            head->index = node;
            head->iswitch = -1;
            head->next = nullptr;
            tail = head;

            trace_nodes.insert(node);
        }

    } else {
        //Recursive case: intermediate node with non-configurable edges
        for (auto iedge : unvisited_non_configurable_edges) {
            int to_node = size_t(rr_graph.edge_sink_node(RRNodeId(node), iedge));
            int iswitch = rr_graph.edge_switch(RRNodeId(node), iedge);

            VTR_ASSERT(!trace_nodes.count(to_node));
            trace_nodes.insert(node);

            //Recurse
            t_trace* subtree_head = nullptr;
            t_trace* subtree_tail = nullptr;
            std::tie(subtree_head, subtree_tail) = add_trace_non_configurable_recurr(to_node, trace_nodes, depth + 1);

            if (subtree_head && subtree_tail) {
                //Add the non-empty sub-tree

                //Duplicate the original head as the new tail (for the new branch)
                t_trace* intermediate_head = alloc_trace_data();
                intermediate_head->index = node;
                intermediate_head->iswitch = iswitch;
                intermediate_head->next = nullptr;

                intermediate_head->next = subtree_head;

                if (!head) { //First subtree becomes head
                    head = intermediate_head;
                } else { //Later subtrees added to tail
                    VTR_ASSERT(tail);
                    tail->next = intermediate_head;
                }

                tail = subtree_tail;
            } else {
                VTR_ASSERT(subtree_head == nullptr && subtree_tail == nullptr);
            }
        }
    }

    return {head, tail};
}

/* The routine sets the path_cost to HUGE_POSITIVE_FLOAT for  *
 * all channel segments touched by previous routing phases.    */
void reset_path_costs(const std::vector<int>& visited_rr_nodes) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto node : visited_rr_nodes) {
        route_ctx.rr_node_route_inf[node].path_cost = std::numeric_limits<float>::infinity();
        route_ctx.rr_node_route_inf[node].backward_path_cost = std::numeric_limits<float>::infinity();
        route_ctx.rr_node_route_inf[node].prev_node = NO_PREVIOUS;
        route_ctx.rr_node_route_inf[node].prev_edge = RREdgeId::INVALID();
    }
}

/* Returns the congestion cost of using this rr-node plus that of any      *
 * non-configurably connected rr_nodes that must be used when it is used.  */
float get_rr_cong_cost(int inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    float cost = get_single_rr_cong_cost(inode, pres_fac);

    if (route_ctx.non_configurable_bitset.get(inode)) {
        // Access unordered_map only when the node is part of a non-configurable set
        auto itr = device_ctx.rr_node_to_non_config_node_set.find(inode);
        if (itr != device_ctx.rr_node_to_non_config_node_set.end()) {
            for (int node : device_ctx.rr_non_config_node_sets[itr->second]) {
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
void mark_ends(ClusterNetId net_id) {
    unsigned int ipin;
    int inode;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
        inode = route_ctx.net_rr_terminals[net_id][ipin];
        route_ctx.rr_node_route_inf[inode].target_flag++;
    }
}

void mark_remaining_ends(ClusterNetId net_id, const std::vector<int>& remaining_sinks) {
    // like mark_ends, but only performs it for the remaining sinks of a net
    int inode;

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (int sink_pin : remaining_sinks) {
        inode = route_ctx.net_rr_terminals[net_id][sink_pin];
        ++route_ctx.rr_node_route_inf[inode].target_flag;
    }
}

void drop_traceback_tail(ClusterNetId net_id) {
    /* Removes the tail node from the routing traceback and updates
     * it with the previous node from the traceback.
     * This funtion is primarily called to remove the virtual clock
     * sink from the routing traceback and replace it with the clock
     * network root. */
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    auto* tail_ptr = route_ctx.trace[net_id].tail;
    auto node = tail_ptr->index;
    route_ctx.trace_nodes[net_id].erase(node);
    auto* trace_ptr = route_ctx.trace[net_id].head;
    while (trace_ptr != nullptr) {
        t_trace* next_ptr = trace_ptr->next;
        if (next_ptr == tail_ptr) {
            trace_ptr->iswitch = tail_ptr->iswitch;
            trace_ptr->next = nullptr;
            route_ctx.trace[net_id].tail = trace_ptr;
            break;
        }
        trace_ptr = next_ptr;
    }
    free_trace_data(tail_ptr);
}

void free_traceback(ClusterNetId net_id) {
    /* Puts the entire traceback (old routing) for this net on the free list *
     * and sets the route_ctx.trace_head pointers etc. for the net to NULL.            */

    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (route_ctx.trace.empty()) {
        return;
    }

    if (route_ctx.trace[net_id].head == nullptr) {
        return;
    }

    free_traceback(route_ctx.trace[net_id].head);

    route_ctx.trace[net_id].head = nullptr;
    route_ctx.trace[net_id].tail = nullptr;
    route_ctx.trace_nodes[net_id].clear();
}

void free_traceback(t_trace* tptr) {
    while (tptr != nullptr) {
        t_trace* tempptr = tptr->next;
        free_trace_data(tptr);
        tptr = tempptr;
    }
}

/* Allocates data structures into which the key routing data can be saved,   *
 * allowing the routing to be recovered later (e.g. after a another routing  *
 * is attempted).                                                            */
vtr::vector<ClusterNetId, t_trace*> alloc_saved_routing() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    vtr::vector<ClusterNetId, t_trace*> best_routing(cluster_ctx.clb_nlist.nets().size());

    return (best_routing);
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

                iclass = type->pin_class[clb_pin];

                if (type->class_inf[iclass].equivalence == PortEquivalence::INSTANCE) {
                    //The pin is part of an instance equivalent class, hence we need to reserve a pin

                    VTR_ASSERT(type->class_inf[iclass].type == DRIVER);

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
void free_trace_structs() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (route_ctx.trace.empty()) {
        return;
    }

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        free_traceback(net_id);

        if (route_ctx.trace[net_id].head) {
            free(route_ctx.trace[net_id].head);
            free(route_ctx.trace[net_id].tail);
        }
        route_ctx.trace[net_id].head = nullptr;
        route_ctx.trace[net_id].tail = nullptr;
    }
}

void free_route_structs() {
    /* Frees the temporary storage needed only during the routing.  The  *
     * final routing result is not freed.                                */
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (route_ctx.route_bb.size() != 0) {
        route_ctx.route_bb.clear();
        route_ctx.route_bb.shrink_to_fit();
    }
}

/* Frees the data structures needed to save a routing.                     */
void free_saved_routing(vtr::vector<ClusterNetId, t_trace*>& best_routing) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (best_routing[net_id] != nullptr) {
            free(best_routing[net_id]);
            best_routing[net_id] = nullptr;
        }
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
        auto& node_inf = route_ctx.rr_node_route_inf[(size_t)rr_id];

        node_inf.prev_node = NO_PREVIOUS;
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
static vtr::vector<ClusterNetId, std::vector<int>> load_net_rr_terminals(const RRGraphView& rr_graph) {
    vtr::vector<ClusterNetId, std::vector<int>> net_rr_terminals;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    auto nets = cluster_ctx.clb_nlist.nets();
    net_rr_terminals.resize(nets.size());

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto net_pins = cluster_ctx.clb_nlist.net_pins(net_id);
        net_rr_terminals[net_id].resize(net_pins.size());

        int pin_count = 0;
        for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
            auto block_id = cluster_ctx.clb_nlist.pin_block(pin_id);
            int i = place_ctx.block_locs[block_id].loc.x;
            int j = place_ctx.block_locs[block_id].loc.y;
            auto type = physical_tile_type(block_id);

            /* In the routing graph, each (x, y) location has unique pins on it
             * so when there is capacity, blocks are packed and their pin numbers
             * are offset to get their actual rr_node */
            int phys_pin = tile_pin_index(pin_id);

            int iclass = type->pin_class[phys_pin];

            RRNodeId inode = rr_graph.node_lookup().find_node(i, j, (pin_count == 0 ? SOURCE : SINK), /* First pin is driver */
                                                              iclass);
            net_rr_terminals[net_id][pin_count] = size_t(inode);
            pin_count++;
        }
    }

    return net_rr_terminals;
}

/* Saves the rr_node corresponding to each SOURCE and SINK in each CLB      *
 * in the FPGA.  Currently only the SOURCE rr_node values are used, and     *
 * they are used only to reserve pins for locally used OPINs in the router. *
 * [0..cluster_ctx.clb_nlist.blocks().size()-1][0..num_class-1].            *
 * The values for blocks that are padsare NOT valid.                        */
static vtr::vector<ClusterBlockId, std::vector<int>> load_rr_clb_sources(const RRGraphView& rr_graph) {
    vtr::vector<ClusterBlockId, std::vector<int>> rr_blk_source;

    int i, j, iclass;
    t_rr_type rr_type;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    rr_blk_source.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto type = physical_tile_type(blk_id);
        auto sub_tile = type->sub_tiles[get_sub_tile_index(blk_id)];

        auto class_range = get_class_range_for_block(blk_id);

        rr_blk_source[blk_id].resize((int)type->class_inf.size());
        for (iclass = 0; iclass < (int)type->class_inf.size(); iclass++) {
            if (iclass >= class_range.low && iclass <= class_range.high) {
                i = place_ctx.block_locs[blk_id].loc.x;
                j = place_ctx.block_locs[blk_id].loc.y;

                if (type->class_inf[iclass].type == DRIVER)
                    rr_type = SOURCE;
                else
                    rr_type = SINK;

                RRNodeId inode = rr_graph.node_lookup().find_node(i, j, rr_type, iclass);
                rr_blk_source[blk_id][iclass] = size_t(inode);
            } else {
                rr_blk_source[blk_id][iclass] = OPEN;
            }
        }
    }

    return rr_blk_source;
}

static vtr::vector<ClusterNetId, uint8_t> load_is_clock_net() {
    vtr::vector<ClusterNetId, uint8_t> is_clock_net;

    auto& atom_ctx = g_vpr_ctx.atom();
    std::set<AtomNetId> clock_nets = find_netlist_physical_clock_nets(atom_ctx.nlist);

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto nets = cluster_ctx.clb_nlist.nets();

    is_clock_net.resize(nets.size());
    for (auto net_id : nets) {
        is_clock_net[net_id] = clock_nets.find(atom_ctx.lookup.atom_net(net_id)) != clock_nets.end();
    }

    return is_clock_net;
}

vtr::vector<ClusterNetId, t_bb> load_route_bb(int bb_factor) {
    vtr::vector<ClusterNetId, t_bb> route_bb;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    t_bb full_device_bounding_box;
    {
        auto& device_ctx = g_vpr_ctx.device();
        full_device_bounding_box.xmin = 0;
        full_device_bounding_box.ymin = 0;
        full_device_bounding_box.xmax = device_ctx.grid.width() - 1;
        full_device_bounding_box.ymax = device_ctx.grid.height() - 1;
    }

    auto nets = cluster_ctx.clb_nlist.nets();
    route_bb.resize(nets.size());
    for (auto net_id : nets) {
        if (!route_ctx.is_clock_net[net_id]) {
            route_bb[net_id] = load_net_route_bb(net_id, bb_factor);
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

t_bb load_net_route_bb(ClusterNetId net_id, int bb_factor) {
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
    auto& cluster_ctx = g_vpr_ctx.clustering();
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

    auto net_sinks = cluster_ctx.clb_nlist.net_sinks(net_id);
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

void add_to_mod_list(int inode, std::vector<int>& modified_rr_node_inf) {
    auto& route_ctx = g_vpr_ctx.routing();

    if (std::isinf(route_ctx.rr_node_route_inf[inode].path_cost)) {
        modified_rr_node_inf.push_back(inode);
    }
}

t_trace*
alloc_trace_data() {
    t_trace* temp_ptr;

    if (trace_free_head == nullptr) { /* No elements on the free list */
        trace_free_head = (t_trace*)vtr::chunk_malloc(sizeof(t_trace), &trace_ch);
        trace_free_head->next = nullptr;
    }
    temp_ptr = trace_free_head;
    temp_ptr->net_pin_index = OPEN; //default
    trace_free_head = trace_free_head->next;
    num_trace_allocated++;
    return (temp_ptr);
}

void free_trace_data(t_trace* tptr) {
    /* Puts the traceback structure pointed to by tptr on the free list. */

    tptr->next = trace_free_head;
    trace_free_head = tptr;
    num_trace_allocated--;
}

void print_route(FILE* fp, const vtr::vector<ClusterNetId, t_traceback>& tracebacks) {
    if (tracebacks.empty()) return; //Only if routing exists

    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            fprintf(fp, "\n\nNet %zu (%s)\n\n", size_t(net_id), cluster_ctx.clb_nlist.net_name(net_id).c_str());
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() == false) {
                fprintf(fp, "\n\nUsed in local cluster only, reserved one CLB pin\n\n");
            } else {
                t_trace* tptr = route_ctx.trace[net_id].head;

                while (tptr != nullptr) {
                    int inode = tptr->index;
                    auto rr_node = RRNodeId(inode);
                    t_rr_type rr_type = rr_graph.node_type(rr_node);
                    int ilow = rr_graph.node_xlow(rr_node);
                    int jlow = rr_graph.node_ylow(rr_node);

                    fprintf(fp, "Node:\t%d\t%6s (%d,%d) ", inode,
                            rr_graph.node_type_string(rr_node), ilow, jlow);

                    if ((ilow != rr_graph.node_xhigh(rr_node))
                        || (jlow != rr_graph.node_yhigh(rr_node)))
                        fprintf(fp, "to (%d,%d) ", rr_graph.node_xhigh(rr_node),
                                rr_graph.node_yhigh(rr_node));

                    switch (rr_type) {
                        case IPIN:
                        case OPIN:
                            if (is_io_type(device_ctx.grid[ilow][jlow].type)) {
                                fprintf(fp, " Pad: ");
                            } else { /* IO Pad. */
                                fprintf(fp, " Pin: ");
                            }
                            break;

                        case CHANX:
                        case CHANY:
                            fprintf(fp, " Track: ");
                            break;

                        case SOURCE:
                        case SINK:
                            if (is_io_type(device_ctx.grid[ilow][jlow].type)) {
                                fprintf(fp, " Pad: ");
                            } else { /* IO Pad. */
                                fprintf(fp, " Class: ");
                            }
                            break;

                        default:
                            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                            "in print_route: Unexpected traceback element type: %d (%s).\n",
                                            rr_type, rr_graph.node_type_string(rr_node));
                            break;
                    }

                    fprintf(fp, "%d  ", rr_graph.node_ptc_num(rr_node));

                    auto physical_tile = device_ctx.grid[ilow][jlow].type;
                    if (!is_io_type(physical_tile) && (rr_type == IPIN || rr_type == OPIN)) {
                        int pin_num = rr_graph.node_pin_num(rr_node);
                        int xoffset = device_ctx.grid[ilow][jlow].width_offset;
                        int yoffset = device_ctx.grid[ilow][jlow].height_offset;
                        int sub_tile_offset = physical_tile->get_sub_tile_loc_from_pin(pin_num);

                        ClusterBlockId iblock = place_ctx.grid_blocks[ilow - xoffset][jlow - yoffset].blocks[sub_tile_offset];
                        VTR_ASSERT(iblock);
                        t_pb_graph_pin* pb_pin = get_pb_graph_node_pin_from_block_pin(iblock, pin_num);
                        t_pb_type* pb_type = pb_pin->parent_node->pb_type;
                        fprintf(fp, " %s.%s[%d] ", pb_type->name, pb_pin->port->name, pb_pin->pin_number);
                    }

                    /* Uncomment line below if you're debugging and want to see the switch types *
                     * used in the routing.                                                      */
                    fprintf(fp, "Switch: %d", tptr->iswitch);

                    //Save net pin index for sinks
                    if (rr_type == SINK) {
                        fprintf(fp, " Net_pin_index: %d", tptr->net_pin_index);
                    }

                    fprintf(fp, "\n");

                    tptr = tptr->next;
                }
            }
        } else { /* Global net.  Never routed. */
            fprintf(fp, "\n\nNet %zu (%s): global net connecting:\n\n", size_t(net_id),
                    cluster_ctx.clb_nlist.net_name(net_id).c_str());

            for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
                ClusterBlockId block_id = cluster_ctx.clb_nlist.pin_block(pin_id);
                int pin_index = tile_pin_index(pin_id);
                int iclass = physical_tile_type(block_id)->pin_class[pin_index];

                fprintf(fp, "Block %s (#%zu) at (%d,%d), Pin class %d.\n",
                        cluster_ctx.clb_nlist.block_name(block_id).c_str(), size_t(block_id),
                        place_ctx.block_locs[block_id].loc.x,
                        place_ctx.block_locs[block_id].loc.y,
                        iclass);
            }
        }
    }
}

/* Prints out the routing to file route_file.  */
void print_route(const char* placement_file, const char* route_file) {
    FILE* fp;

    fp = fopen(route_file, "w");

    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    fprintf(fp, "Placement_File: %s Placement_ID: %s\n", placement_file, place_ctx.placement_id.c_str());

    fprintf(fp, "Array size: %zu x %zu logic blocks.\n", device_ctx.grid.width(), device_ctx.grid.height());
    fprintf(fp, "\nRouting:");

    print_route(fp, route_ctx.trace);

    fclose(fp);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_MEM)) {
        fp = vtr::fopen(getEchoFileName(E_ECHO_MEM), "w");
        fprintf(fp, "\nNum_trace_allocated: %d\n",
                num_trace_allocated);
        fprintf(fp, "Num_linked_f_pointer_allocated: %d\n",
                num_linked_f_pointer_allocated);
        fclose(fp);
    }

    //Save the digest of the route file
    route_ctx.routing_id = vtr::secure_digest_file(route_file);
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
void reserve_locally_used_opins(HeapInterface* heap, float pres_fac, float acc_fac, bool rip_up_local_opins) {
    int num_local_opin, inode, from_node, iconn, num_edges, to_node;
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
                VTR_ASSERT(type->class_inf[iclass].equivalence == PortEquivalence::INSTANCE);

                /* Always 0 for pads and for RECEIVER (IPIN) classes */
                for (ipin = 0; ipin < num_local_opin; ipin++) {
                    inode = route_ctx.clb_opins_used_locally[blk_id][iclass][ipin];
                    VTR_ASSERT(inode >= 0 && inode < (ssize_t)rr_graph.num_nodes());
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

            VTR_ASSERT(type->class_inf[iclass].equivalence == PortEquivalence::INSTANCE);

            //From the SRC node we walk through it's out going edges to collect the
            //OPIN nodes. We then push them onto a heap so the OPINs with lower
            //congestion cost are popped-off/reserved first. (Intuitively, we want
            //the reserved OPINs to move out of the way of congestion, by preferring
            //to reserve OPINs with lower congestion costs).
            from_node = route_ctx.rr_blk_source[blk_id][iclass];
            num_edges = rr_graph.num_edges(RRNodeId(from_node));
            for (iconn = 0; iconn < num_edges; iconn++) {
                to_node = size_t(rr_graph.edge_sink_node(RRNodeId(from_node), iconn));

                VTR_ASSERT(rr_graph.node_type(RRNodeId(to_node)) == OPIN);

                //Add the OPIN to the heap according to it's congestion cost
                cost = get_rr_cong_cost(to_node, pres_fac);
                add_node_to_heap(heap, route_ctx.rr_node_route_inf,
                                 to_node, cost, OPEN, RREdgeId::INVALID(),
                                 0., 0.);
            }

            for (ipin = 0; ipin < num_local_opin; ipin++) {
                //Pop the nodes off the heap. We get them from the heap so we
                //reserve those pins with lowest congestion cost first.
                heap_head_ptr = heap->get_heap_head();
                inode = heap_head_ptr->index;

                VTR_ASSERT(rr_graph.node_type(RRNodeId(inode)) == OPIN);

                adjust_one_rr_occ_and_acc_cost(inode, 1, acc_fac);
                route_ctx.clb_opins_used_locally[blk_id][iclass][ipin] = inode;
                heap->free(heap_head_ptr);
            }

            heap->empty_heap();
        }
    }
}

static void adjust_one_rr_occ_and_acc_cost(int inode, int add_or_sub, float acc_fac) {
    /* Increments or decrements (depending on add_or_sub) the occupancy of    *
     * one rr_node, and adjusts the present cost of that node appropriately.  */

    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int new_occ = route_ctx.rr_node_route_inf[inode].occ() + add_or_sub;
    int capacity = rr_graph.node_capacity(RRNodeId(inode));
    route_ctx.rr_node_route_inf[inode].set_occ(new_occ);

    if (new_occ < capacity) {
    } else {
        if (add_or_sub == 1) {
            route_ctx.rr_node_route_inf[inode].acc_cost += (new_occ - capacity) * acc_fac;
        }
    }
}

void free_chunk_memory_trace() {
    if (trace_ch.chunk_ptr_head != nullptr) {
        free_chunk_memory(&trace_ch);
        trace_ch.chunk_ptr_head = nullptr;
        trace_free_head = nullptr;
    }
}

// connection based overhaul (more specificity than nets)
// utility and debugging functions -----------------------
void print_traceback(ClusterNetId net_id) {
    // linearly print linked list
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    VTR_LOG("traceback %zu: ", size_t(net_id));
    t_trace* head = route_ctx.trace[net_id].head;
    while (head) {
        int inode{head->index};
        if (rr_graph.node_type(RRNodeId(inode)) == SINK)
            VTR_LOG("%d(sink)(%d)->", inode, route_ctx.rr_node_route_inf[inode].occ());
        else
            VTR_LOG("%d(%d)->", inode, route_ctx.rr_node_route_inf[inode].occ());
        head = head->next;
    }
    VTR_LOG("\n");
}

void print_traceback(const t_trace* trace) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();
    const t_trace* prev = nullptr;
    while (trace) {
        int inode = trace->index;
        VTR_LOG("%d (%s)", inode, rr_node_typename[rr_graph.node_type(RRNodeId(inode))]);

        if (trace->iswitch == OPEN) {
            VTR_LOG(" !"); //End of branch
        }

        if (prev && prev->iswitch != OPEN && !rr_graph.rr_switch_inf(RRSwitchId(prev->iswitch)).configurable()) {
            VTR_LOG("*"); //Reached non-configurably
        }

        if (route_ctx.rr_node_route_inf[inode].occ() > rr_graph.node_capacity(RRNodeId(inode))) {
            VTR_LOG(" x"); //Overused
        }
        VTR_LOG("\n");
        prev = trace;
        trace = trace->next;
    }
    VTR_LOG("\n");
}

bool validate_traceback(t_trace* trace) {
    std::set<int> seen_rr_nodes;

    return validate_traceback_recurr(trace, seen_rr_nodes);
}

bool validate_traceback_recurr(t_trace* trace, std::set<int>& seen_rr_nodes) {
    if (!trace) {
        return true;
    }

    seen_rr_nodes.insert(trace->index);

    t_trace* next = trace->next;

    if (next) {
        if (trace->iswitch == OPEN) { //End of a branch

            //Verify that the next element (branch point) has been already seen in the traceback so far
            if (!seen_rr_nodes.count(next->index)) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Traceback branch point %d not found", next->index);
            } else {
                //Recurse along the new branch
                return validate_traceback_recurr(next, seen_rr_nodes);
            }
        } else { //Midway along branch

            //Check there is an edge connecting trace and next

            auto& device_ctx = g_vpr_ctx.device();
            const auto& rr_graph = device_ctx.rr_graph;
            bool found = false;
            for (t_edge_size iedge = 0; iedge < rr_graph.num_edges(RRNodeId(trace->index)); ++iedge) {
                int to_node = size_t(rr_graph.edge_sink_node(RRNodeId(trace->index), iedge));

                if (to_node == next->index) {
                    found = true;

                    //Verify that the switch matches
                    int rr_iswitch = rr_graph.edge_switch(RRNodeId(trace->index), iedge);
                    if (trace->iswitch != rr_iswitch) {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Traceback mismatched switch type: traceback %d rr_graph %d (RR nodes %d -> %d)\n",
                                        trace->iswitch, rr_iswitch,
                                        trace->index, to_node);
                    }
                    break;
                }
            }

            if (!found) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Traceback no RR edge between RR nodes %d -> %d\n", trace->index, next->index);
            }

            //Recurse
            return validate_traceback_recurr(next, seen_rr_nodes);
        }
    }

    VTR_ASSERT(!next);
    return true; //End of traceback
}

//Print information about an invalid routing, caused by overused routing resources
void print_invalid_routing_info() {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    //Build a look-up of nets using each RR node
    std::multimap<int, ClusterNetId> rr_node_nets;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        t_trace* tptr = route_ctx.trace[net_id].head;

        while (tptr != nullptr) {
            rr_node_nets.emplace(tptr->index, net_id);
            tptr = tptr->next;
        }
    }

    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        size_t inode = (size_t)rr_id;
        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int cap = rr_graph.node_capacity(rr_id);
        if (occ > cap) {
            VTR_LOG("  %s is overused (occ=%d capacity=%d)\n", describe_rr_node(inode).c_str(), occ, cap);

            auto range = rr_node_nets.equal_range(inode);
            for (auto itr = range.first; itr != range.second; ++itr) {
                auto net_id = itr->second;
                VTR_LOG("    Used by net %s (%zu)\n", cluster_ctx.clb_nlist.net_name(net_id).c_str(), size_t(net_id));
            }
        }
    }
}

void print_rr_node_route_inf() {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    for (size_t inode = 0; inode < route_ctx.rr_node_route_inf.size(); ++inode) {
        if (!std::isinf(route_ctx.rr_node_route_inf[inode].path_cost)) {
            int prev_node = route_ctx.rr_node_route_inf[inode].prev_node;
            RREdgeId prev_edge = route_ctx.rr_node_route_inf[inode].prev_edge;
            auto switch_id = rr_graph.rr_nodes().edge_switch(prev_edge);
            VTR_LOG("rr_node: %d prev_node: %d prev_edge: %zu",
                    inode, prev_node, (size_t)prev_edge);

            if (prev_node != OPEN && bool(prev_edge) && !rr_graph.rr_switch_inf(RRSwitchId(switch_id)).configurable()) {
                VTR_LOG("*");
            }

            VTR_LOG(" pcost: %g back_pcost: %g\n",
                    route_ctx.rr_node_route_inf[inode].path_cost, route_ctx.rr_node_route_inf[inode].backward_path_cost);
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
        if (!std::isinf(route_ctx.rr_node_route_inf[inode].path_cost)) {
            VTR_LOG("\tnode%zu[label=\"{%zu (%s)", inode, inode, rr_graph.node_type_string(RRNodeId(inode)));
            if (route_ctx.rr_node_route_inf[inode].occ() > rr_graph.node_capacity(RRNodeId(inode))) {
                VTR_LOG(" x");
            }
            VTR_LOG("}\"]\n");
        }
    }
    for (size_t inode = 0; inode < route_ctx.rr_node_route_inf.size(); ++inode) {
        if (!std::isinf(route_ctx.rr_node_route_inf[inode].path_cost)) {
            int prev_node = route_ctx.rr_node_route_inf[inode].prev_node;
            RREdgeId prev_edge = route_ctx.rr_node_route_inf[inode].prev_edge;
            auto switch_id = rr_graph.rr_nodes().edge_switch(prev_edge);

            if (prev_node != OPEN && bool(prev_edge)) {
                VTR_LOG("\tnode%d -> node%zu [", prev_node, inode);
                if (prev_node != OPEN && bool(prev_edge) && !rr_graph.rr_switch_inf(RRSwitchId(switch_id)).configurable()) {
                    VTR_LOG("label=\"*\"");
                }

                VTR_LOG("];\n");
            }
        }
    }

    VTR_LOG("}\n");
}

static bool validate_trace_nodes(t_trace* head, const std::unordered_set<int>& trace_nodes) {
    //Verifies that all nodes in the traceback 'head' are conatined in 'trace_nodes'

    if (!head) {
        return true;
    }

    std::vector<int> missing_from_trace_nodes;
    for (t_trace* tptr = head; tptr != nullptr; tptr = tptr->next) {
        if (!trace_nodes.count(tptr->index)) {
            missing_from_trace_nodes.push_back(tptr->index);
        }
    }

    if (!missing_from_trace_nodes.empty()) {
        std::string msg = vtr::string_fmt(
            "The following %zu nodes were found in traceback"
            " but were missing from trace_nodes: %s\n",
            missing_from_trace_nodes.size(),
            vtr::join(missing_from_trace_nodes, ", ").c_str());

        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, msg.c_str());
        return false;
    }

    return true;
}

// True if router will use a lookahead.
//
// This controls whether the router lookahead cache will be primed outside of
// the router ScopedStartFinishTimer.
bool router_needs_lookahead(enum e_router_algorithm router_algorithm) {
    switch (router_algorithm) {
        case BREADTH_FIRST:
            return false;
        case TIMING_DRIVEN:
            return true;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Unknown routing algorithm %d",
                            router_algorithm);
    }
}

std::string describe_unrouteable_connection(const int source_node, const int sink_node) {
    std::string msg = vtr::string_fmt(
        "Cannot route from %s (%s) to "
        "%s (%s) -- no possible path",
        rr_node_arch_name(source_node).c_str(), describe_rr_node(source_node).c_str(),
        rr_node_arch_name(sink_node).c_str(), describe_rr_node(sink_node).c_str());

    return msg;
}
