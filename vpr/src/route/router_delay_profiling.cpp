#include "router_delay_profiling.h"
#include "globals.h"
#include "route_tree_type.h"
#include "route_common.h"
#include "route_timing.h"
#include "route_tree_timing.h"
#include "route_export.h"
#include "rr_graph.h"

static t_rt_node* setup_routing_resources_no_net(int source_node);

RouterDelayProfiler::RouterDelayProfiler(
    const RouterLookahead* lookahead)
    : router_lookahead_(lookahead) {}

bool RouterDelayProfiler::calculate_delay(int source_node, int sink_node, const t_router_opts& router_opts, float* net_delay) const {
    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    t_rt_node* rt_root = setup_routing_resources_no_net(source_node);
    enable_router_debug(router_opts, ClusterNetId(), sink_node);

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(1);

    //maximum bounding box for placement
    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;

    t_conn_cost_params cost_params;
    cost_params.criticality = 1.;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;

    route_budgets budgeting_inf;

    init_heap(device_ctx.grid);

    std::vector<int> modified_rr_node_inf;
    RouterStats router_stats;
    t_heap* cheapest = timing_driven_route_connection_from_route_tree(rt_root,
                                                                      sink_node, cost_params, bounding_box, *router_lookahead_,
                                                                      modified_rr_node_inf, router_stats);

    bool found_path = (cheapest != nullptr);
    if (found_path) {
        VTR_ASSERT(cheapest->index == sink_node);

        t_rt_node* rt_node_of_sink = update_route_tree(cheapest, nullptr);
        free_heap_data(cheapest);

        //find delay
        *net_delay = rt_node_of_sink->Tdel;

        VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");
        free_route_tree(rt_root);
    }

    //Reset for the next router call
    empty_heap();
    reset_path_costs(modified_rr_node_inf);

    return found_path;
}

//Returns the shortest path delay from src_node to all RR nodes in the RR graph, or NaN if no path exists
std::vector<float> calculate_all_path_delays_from_rr_node(int src_rr_node, const t_router_opts& router_opts) {
    auto& device_ctx = g_vpr_ctx.device();

    std::vector<float> path_delays_to(device_ctx.rr_nodes.size(), std::numeric_limits<float>::quiet_NaN());

    t_rt_node* rt_root = setup_routing_resources_no_net(src_rr_node);

    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;

    t_conn_cost_params cost_params;
    cost_params.criticality = 1.;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;

    std::vector<int> modified_rr_node_inf;
    RouterStats router_stats;

    init_heap(device_ctx.grid);

    std::vector<t_heap> shortest_paths = timing_driven_find_all_shortest_paths_from_route_tree(rt_root,
                                                                                               cost_params,
                                                                                               bounding_box,
                                                                                               modified_rr_node_inf,
                                                                                               router_stats);

    free_route_tree(rt_root);

    VTR_ASSERT(shortest_paths.size() == device_ctx.rr_nodes.size());
    for (int sink_rr_node = 0; sink_rr_node < (int)device_ctx.rr_nodes.size(); ++sink_rr_node) {
        if (sink_rr_node == src_rr_node) {
            path_delays_to[sink_rr_node] = 0.;
        } else {
            if (shortest_paths[sink_rr_node].index == OPEN) continue;

            VTR_ASSERT(shortest_paths[sink_rr_node].index == sink_rr_node);

            //Build the routing tree to get the delay
            rt_root = setup_routing_resources_no_net(src_rr_node);
            t_rt_node* rt_node_of_sink = update_route_tree(&shortest_paths[sink_rr_node], nullptr);

            VTR_ASSERT(rt_node_of_sink->inode == sink_rr_node);

            path_delays_to[sink_rr_node] = rt_node_of_sink->Tdel;

            free_route_tree(rt_root);
        }
    }
    reset_path_costs(modified_rr_node_inf);
    empty_heap();

#if 0
    //Sanity check
    for (int sink_rr_node = 0; sink_rr_node < (int) device_ctx.rr_nodes.size(); ++sink_rr_node) {

        float astar_delay = std::numeric_limits<float>::quiet_NaN();
        if (sink_rr_node == src_rr_node) {
            astar_delay = 0.;
        } else {
            calculate_delay(src_rr_node, sink_rr_node, router_opts, &astar_delay);
        }

        //Sanity check
        float dijkstra_delay = path_delays_to[sink_rr_node];

        float ratio = dijkstra_delay / astar_delay;
        if (astar_delay == 0. && dijkstra_delay == 0.) {
            ratio = 1.;
        }

        VTR_LOG("Delay from %d -> %d: all_shortest_paths %g direct %g ratio %g\n",
                src_rr_node, sink_rr_node,
                dijkstra_delay, astar_delay,
                ratio);
    }
#endif

    return path_delays_to;
}

static t_rt_node* setup_routing_resources_no_net(int source_node) {
    /* Build and return a partial route tree from the legal connections from last iteration.
     * along the way do:
     * 	update pathfinder costs to be accurate to the partial route tree
     *	update the net's traceback to be accurate to the partial route tree
     * 	find and store the pins that still need to be reached in incremental_rerouting_resources.remaining_targets
     * 	find and store the rt nodes that have been reached in incremental_rerouting_resources.reached_rt_sinks
     *	mark the rr_node sinks as targets to be reached */

    t_rt_node* rt_root = init_route_tree_to_source_no_net(source_node);

    return rt_root;
}

void alloc_routing_structs(t_chan_width chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch* det_routing_arch,
                           std::vector<t_segment_inf>& segment_inf,
                           const t_direct_inf* directs,
                           const int num_directs) {
    int warnings;
    t_graph_type graph_type;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    device_ctx.num_arch_switches,
                    det_routing_arch,
                    segment_inf,
                    router_opts.base_cost_type,
                    router_opts.trim_empty_channels,
                    router_opts.trim_obs_channels,
                    router_opts.clock_modeling,
                    directs, num_directs,
                    &warnings);

    alloc_and_load_rr_node_route_structs();

    alloc_route_tree_timing_structs();
}

void free_routing_structs() {
    free_route_structs();
    free_trace_structs();

    free_route_tree_timing_structs();
}
