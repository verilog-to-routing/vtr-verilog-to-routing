#include <cstdio>
using namespace std;

#include "vtr_log.h"

#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_breadth_first.h"

//Print out extensive debug information about router operations
//#define ROUTER_DEBUG

/********************* Subroutines local to this module *********************/

static bool breadth_first_route_net(ClusterNetId net_id, float bend_cost);

static void breadth_first_expand_trace_segment(t_trace* start_ptr,
                                               int remaining_connections_to_sink,
                                               std::vector<int>& modified_rr_node_inf);

static void breadth_first_expand_neighbours(int inode, float pcost, ClusterNetId net_id, float bend_cost);

static void breadth_first_add_to_heap(const float path_cost, const float bend_cost, const int from_node, const int to_node, const int iconn);

static float evaluate_node_cost(const float prev_path_cost, const float bend_cost, const int from_node, const int to_node);

static void breadth_first_add_source_to_heap(ClusterNetId net_id);

/************************ Subroutine definitions ****************************/

bool try_breadth_first_route(t_router_opts router_opts) {
    /* Iterated maze router ala Pathfinder Negotiated Congestion algorithm,  *
     * (FPGA 95 p. 111).  Returns true if it can route this FPGA, false if   *
     * it can't.                                                             */

    VTR_LOG(
        "**********************************************************************\n"
        "*                         !!! WARNING !!!                            *\n"
        "*                                                                    *\n"
        "*      Routing with the DEPRECATED 'Breadth-First' router, which     *\n"
        "*        is inferrior and may be removed in a future release.        *\n"
        "*                                                                    *\n"
        "*     Use the 'Timing-Driven' router instead, which requires much    *\n"
        "*      less run-time (> 300x faster) and produces higher quality     *\n"
        "*          results (less wirelength, lower delay), even when         *\n"
        "*           architecture timing information is unavailable.          *\n"
        "*                                                                    *\n"
        "*                         !!! WARNING !!!                            *\n"
        "**********************************************************************\n"
        "\n");

    float pres_fac;
    bool success, is_routable, rip_up_local_opins;
    int itry;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    /* Usually the first iteration uses a very small (or 0) pres_fac to find  *
     * the shortest path and get a congestion map.  For fast compiles, I set  *
     * pres_fac high even for the first iteration.                            */

    pres_fac = router_opts.first_iter_pres_fac;

    for (itry = 1; itry <= router_opts.max_router_iterations; itry++) {
        VTR_LOG("Routing Iteration %d\n", itry);

        /* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            route_ctx.net_status[net_id].is_routed = false;
            route_ctx.net_status[net_id].is_fixed = false;
        }

        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            is_routable = try_breadth_first_route_net(net_id, pres_fac, router_opts);
            if (!is_routable) {
                return (false);
            }
        }

        /* Make sure any CLB OPINs used up by subblocks being hooked directly     *
         * to them are reserved for that purpose.                                 */

        if (itry == 1)
            rip_up_local_opins = false;
        else
            rip_up_local_opins = true;

        reserve_locally_used_opins(pres_fac, router_opts.acc_fac, rip_up_local_opins);

        success = feasible_routing();
        if (success) {
            VTR_LOG("Successfully routed after %d routing iterations.\n", itry);
            return (true);
        }

        if (itry == 1)
            pres_fac = router_opts.initial_pres_fac;
        else
            pres_fac *= router_opts.pres_fac_mult;

        pres_fac = min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

        pathfinder_update_cost(pres_fac, router_opts.acc_fac);
    }

    VTR_LOG("Routing failed.\n");

#ifdef ROUTER_DEBUG
    print_invalid_routing_info();
#endif

    return (false);
}

bool try_breadth_first_route_net(ClusterNetId net_id, float pres_fac, t_router_opts router_opts) {
    bool is_routed = false;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (route_ctx.net_status[net_id].is_fixed) { /* Skip pre-routed nets. */
        is_routed = true;

    } else if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Skip ignored nets. */
        is_routed = true;

    } else {
        pathfinder_update_path_cost(route_ctx.trace[net_id].head, -1, pres_fac);
        is_routed = breadth_first_route_net(net_id, router_opts.bend_cost);

        /* Impossible to route? (disconnected rr_graph) */
        if (is_routed) {
            route_ctx.net_status[net_id].is_routed = false;
        } else {
            VTR_LOG("Routing failed.\n");
        }

        pathfinder_update_path_cost(route_ctx.trace[net_id].head, 1, pres_fac);
    }
    return (is_routed);
}

static bool breadth_first_route_net(ClusterNetId net_id, float bend_cost) {
    /* Uses a maze routing (Dijkstra's) algorithm to route a net.  The net       *
     * begins at the net output, and expands outward until it hits a target      *
     * pin.  The algorithm is then restarted with the entire first wire segment  *
     * included as part of the source this time.  For an n-pin net, the maze     *
     * router is invoked n-1 times to complete all the connections.  net_id is     *
     * the index of the net to be routed.  Bends are penalized by bend_cost      *
     * (which is typically zero for detailed routing and nonzero only for global *
     * routing), since global routes with lots of bends are tougher to detailed  *
     * route (using a detailed router like SEGA).                                *
     * If this routine finds that a net *cannot* be connected (due to a complete *
     * lack of potential paths, rather than congestion), it returns false, as    *
     * routing is impossible on this architecture.  Otherwise it returns true.   */

    int inode, remaining_connections_to_sink;
    float pcost, new_pcost;
    t_heap* current;
    t_trace* tptr;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

#ifdef ROUTER_DEBUG
    VTR_LOG("Routing Net %zu (%zu sinks)\n", size_t(net_id), cluster_ctx.clb_nlist.net_sinks(net_id).size());
#endif

    free_traceback(net_id);

    breadth_first_add_source_to_heap(net_id);
    mark_ends(net_id);

    tptr = nullptr;
    remaining_connections_to_sink = 0;

    auto src_pin_id = cluster_ctx.clb_nlist.net_driver(net_id);

    std::vector<int> modified_rr_node_inf; //RR node indicies with modified rr_node_route_inf

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) { /* Need n-1 wires to connect n pins */

        breadth_first_expand_trace_segment(tptr, remaining_connections_to_sink, modified_rr_node_inf);
        current = get_heap_head();

        if (current == nullptr) { /* Infeasible routing.  No possible path for net. */
            VTR_LOG("Cannot route net #%zu (%s) from (%s) to sink pin (%s) -- no possible path.\n",
                    size_t(net_id), cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                    cluster_ctx.clb_nlist.pin_name(src_pin_id).c_str(),
                    cluster_ctx.clb_nlist.pin_name(pin_id).c_str());
            reset_path_costs(modified_rr_node_inf); /* Clean up before leaving. */
            return (false);
        }

        inode = current->index;

#ifdef ROUTER_DEBUG
        VTR_LOG("  Popped node %d\n", inode);
#endif

        while (route_ctx.rr_node_route_inf[inode].target_flag == 0) {
            pcost = route_ctx.rr_node_route_inf[inode].path_cost;
            new_pcost = current->cost;
            if (pcost > new_pcost) { /* New path is lowest cost. */
#ifdef ROUTER_DEBUG
                VTR_LOG("    New best cost %g\n", new_pcost);
#endif

#ifdef ROUTER_DEBUG
                VTR_LOG("    Setting routing paths for associated node %d\n", current->index);
#endif
                add_to_mod_list(current->index, modified_rr_node_inf);

                route_ctx.rr_node_route_inf[current->index].path_cost = new_pcost;
                route_ctx.rr_node_route_inf[current->index].prev_node = current->u.prev.node;
                route_ctx.rr_node_route_inf[current->index].prev_edge = current->u.prev.edge;

#ifdef ROUTER_DEBUG
                VTR_LOG("    Expanding node %d neighbours\n", inode);
#endif
                breadth_first_expand_neighbours(inode, new_pcost, net_id, bend_cost);
            }

            free_heap_data(current);
            current = get_heap_head();

            if (current == nullptr) { /* Impossible routing. No path for net. */
                VTR_LOG("Cannot route net #%zu (%s) from (%s) to sink pin (%s) -- no possible path.\n",
                        size_t(net_id), cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                        cluster_ctx.clb_nlist.pin_name(src_pin_id).c_str(), cluster_ctx.clb_nlist.pin_name(pin_id).c_str());
                reset_path_costs(modified_rr_node_inf);
                return (false);
            }

            inode = current->index;

#ifdef ROUTER_DEBUG
            VTR_LOG("  Popped node %d\n", inode);
#endif
        }
#ifdef ROUTER_DEBUG
        VTR_LOG("  Found target node %d\n", inode);
#endif
        if (current != nullptr) {
            //Update link to target
            add_to_mod_list(current->index, modified_rr_node_inf);

            route_ctx.rr_node_route_inf[current->index].path_cost = current->cost;
            route_ctx.rr_node_route_inf[current->index].prev_node = current->u.prev.node;
            route_ctx.rr_node_route_inf[current->index].prev_edge = current->u.prev.edge;
        }

        route_ctx.rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
        remaining_connections_to_sink = route_ctx.rr_node_route_inf[inode].target_flag;
        tptr = update_traceback(current, net_id);
        free_heap_data(current);
    }

#ifdef ROUTER_DEBUG
    VTR_LOG("Routed Net %zu\n", size_t(net_id));
#endif

    empty_heap();
    reset_path_costs(modified_rr_node_inf);
    return (true);
}

static void breadth_first_expand_trace_segment(t_trace* start_ptr,
                                               int remaining_connections_to_sink,
                                               std::vector<int>& modified_rr_node_inf) {
    /* Adds all the rr_nodes in the traceback segment starting at tptr (and     *
     * continuing to the end of the traceback) to the heap with a cost of zero. *
     * This allows expansion to begin from the existing wiring.  The            *
     * remaining_connections_to_sink value is 0 if the route segment ending     *
     * at this location is the last one to connect to the SINK ending the route *
     * segment.  This is the usual case.  If it is not the last connection this *
     * net must make to this SINK, I have a hack to ensure the next connection  *
     * to this SINK goes through a different IPIN.  Without this hack, the      *
     * router would always put all the connections from this net to this SINK   *
     * through the same IPIN.  With LUTs or cluster-based logic blocks, you     *
     * should never have a net connecting to two logically-equivalent pins on   *
     * the same logic block, so the hack will never execute.  If your logic     *
     * block is an and-gate, however, nets might connect to two and-inputs on   *
     * the same logic block, and since the and-inputs are logically-equivalent, *
     * this means two connections to the same SINK.                             */

    t_trace *tptr, *next_ptr;
    int inode, sink_node, last_ipin_node;

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    tptr = start_ptr;
    if (tptr != nullptr && device_ctx.rr_nodes[tptr->index].type() == SINK) {
        /* During logical equivalence case, only use one opin */
        tptr = tptr->next;
    }

    if (remaining_connections_to_sink == 0) { /* Usual case. */
        while (tptr != nullptr) {
#ifdef ROUTER_DEBUG
            VTR_LOG("  Adding previous routing node %d to heap\n", tptr->index);
#endif
            node_to_heap(tptr->index, 0., NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);
            tptr = tptr->next;
        }
    } else { /* This case never executes for most logic blocks. */

        /* Weird case.  Lots of hacks. The cleanest way to do this would be to empty *
         * the heap, update the congestion due to the partially-completed route, put *
         * the whole route so far (excluding IPINs and SINKs) on the heap with cost  *
         * 0., and expand till you hit the next SINK.  That would be slow, so I      *
         * do some hacks to enable incremental wavefront expansion instead.          */

        if (tptr == nullptr)
            return; /* No route yet */

        next_ptr = tptr->next;
        last_ipin_node = OPEN; /* Stops compiler from complaining. */

        /* Can't put last SINK on heap with NO_PREVIOUS, etc, since that won't let  *
         * us reach it again.  Instead, leave the last traceback element (SINK) off *
         * the heap.                                                                */

        while (next_ptr != nullptr) {
            inode = tptr->index;
#ifdef ROUTER_DEBUG
            VTR_LOG("  Adding previous routing node %d to heap*\n", tptr->index);
#endif
            node_to_heap(inode, 0., NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);

            if (device_ctx.rr_nodes[inode].type() == IPIN)
                last_ipin_node = inode;

            tptr = next_ptr;
            next_ptr = tptr->next;
        }

        /* This will stop the IPIN node used to get to this SINK from being         *
         * reexpanded for the remainder of this net's routing.  This will make us   *
         * hook up more IPINs to this SINK (which is what we want).  If IPIN        *
         * doglegs are allowed in the graph, we won't be able to use this IPIN to   *
         * do a dogleg, since it won't be re-expanded.  Shouldn't be a big problem. */

        add_to_mod_list(last_ipin_node, modified_rr_node_inf);
        route_ctx.rr_node_route_inf[last_ipin_node].path_cost = -HUGE_POSITIVE_FLOAT;

        /* Also need to mark the SINK as having high cost, so another connection can *
         * be made to it.                                                            */

        sink_node = tptr->index;
        add_to_mod_list(sink_node, modified_rr_node_inf);
        route_ctx.rr_node_route_inf[sink_node].path_cost = HUGE_POSITIVE_FLOAT;

        /* Finally, I need to remove any pending connections to this SINK via the    *
         * IPIN I just used (since they would result in congestion).  Scan through   *
         * the heap to do this.                                                      */

        invalidate_heap_entries(sink_node, last_ipin_node);
    }
}

static void breadth_first_expand_neighbours(int inode, float pcost, ClusterNetId net_id, float bend_cost) {
    /* Puts all the rr_nodes adjacent to inode on the heap.  rr_nodes outside   *
     * the expanded bounding box specified in route_bb are not added to the     *
     * heap.  pcost is the path_cost to get to inode.                           */

    int iconn, to_node, num_edges;

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    num_edges = device_ctx.rr_nodes[inode].num_edges();
    for (iconn = 0; iconn < num_edges; iconn++) {
        to_node = device_ctx.rr_nodes[inode].edge_sink_node(iconn);

        if (device_ctx.rr_nodes[to_node].xhigh() < route_ctx.route_bb[net_id].xmin
            || device_ctx.rr_nodes[to_node].xlow() > route_ctx.route_bb[net_id].xmax
            || device_ctx.rr_nodes[to_node].yhigh() < route_ctx.route_bb[net_id].ymin
            || device_ctx.rr_nodes[to_node].ylow() > route_ctx.route_bb[net_id].ymax)
            continue; /* Node is outside (expanded) bounding box. */

        breadth_first_add_to_heap(pcost, bend_cost, inode, to_node, iconn);
    }
}

//Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
static void breadth_first_add_to_heap(const float path_cost, const float bend_cost, const int from_node, const int to_node, const int iconn) {
#ifdef ROUTER_DEBUG
    VTR_LOG("      Expanding node %d\n", to_node);
#endif

    //Create a heap element to represent this node (and any non-configurably connected nodes)
    t_heap* next = alloc_heap_data();
    next->index = to_node;
    next->backward_path_cost = OPEN;
    next->R_upstream = OPEN;
    next->cost = std::numeric_limits<float>::infinity();

    //Path cost to 'to_node'
    float new_path_cost = evaluate_node_cost(path_cost, bend_cost, from_node, to_node);

    next->cost = new_path_cost;

    //Record how we reached this node
    next->index = to_node;
    next->u.prev.edge = iconn;
    next->u.prev.node = from_node;

    add_to_heap(next);
}

static float evaluate_node_cost(const float prev_path_cost, const float bend_cost, const int from_node, const int to_node) {
    auto& device_ctx = g_vpr_ctx.device();

    float tot_cost = prev_path_cost + get_rr_cong_cost(to_node);

    if (bend_cost != 0.) {
        t_rr_type from_type = device_ctx.rr_nodes[from_node].type();
        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if ((from_type == CHANX && to_type == CHANY)
            || (from_type == CHANY && to_type == CHANX))
            tot_cost += bend_cost;
    }

    return tot_cost;
}

static void breadth_first_add_source_to_heap(ClusterNetId net_id) {
    /* Adds the SOURCE of this net to the heap.  Used to start a net's routing. */

    int inode;
    float cost;

    auto& route_ctx = g_vpr_ctx.routing();

    inode = route_ctx.net_rr_terminals[net_id][0]; /* SOURCE */
    cost = get_rr_cong_cost(inode);

#ifdef ROUTER_DEBUG
    VTR_LOG("  Adding Source node %d to heap\n", inode);
#endif

    node_to_heap(inode, cost, NO_PREVIOUS, NO_PREVIOUS, OPEN, OPEN);
}
