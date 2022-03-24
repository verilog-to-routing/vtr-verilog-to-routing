#include "router_delay_profiling.h"
#include "globals.h"
#include "route_common.h"
#include "route_timing.h"
#include "route_export.h"
#include "route_tree.h"
#include "rr_graph.h"
#include "vtr_time.h"
#include "draw.h"

RouterDelayProfiler::RouterDelayProfiler(const Netlist<>& net_list,
                                         const RouterLookahead* lookahead,
                                         bool is_flat)
    : net_list_(net_list)
    , router_(
          g_vpr_ctx.device().grid,
          *lookahead,
          g_vpr_ctx.device().rr_graph.rr_nodes(),
          &g_vpr_ctx.device().rr_graph,
          g_vpr_ctx.device().rr_rc_data,
          g_vpr_ctx.device().rr_graph.rr_switch(),
          g_vpr_ctx.mutable_routing().rr_node_route_inf,
          is_flat)
    , is_flat_(is_flat) {}

bool RouterDelayProfiler::calculate_delay(RRNodeId source_node, RRNodeId sink_node, const t_router_opts& router_opts, float* net_delay) {
    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    //vtr::ScopedStartFinishTimer t(vtr::string_fmt("Profiling Delay from %s at %d,%d (%s) to %s at %d,%d (%s)",
    //rr_graph.node_type_string(RRNodeId(source_node)),
    //rr_graph.node_xlow(RRNodeId(source_node)),
    //rr_graph.node_ylow(RRNodeId(source_node)),
    //rr_node_arch_name(source_node).c_str(),
    //rr_graph.node_type_string(RRNodeId(sink_node)),
    //rr_graph.node_xlow(RRNodeId(sink_node)),
    //rr_graph.node_ylow(RRNodeId(sink_node)),
    //rr_node_arch_name(sink_node).c_str()));

    RouteTree tree((RRNodeId(source_node)));
    enable_router_debug(router_opts, ParentNetId(), sink_node, 0, &router_);

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
    cost_params.astar_fac = router_opts.router_profiler_astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;

    route_budgets budgeting_inf(net_list_, is_flat_);

    router_.clear_modified_rr_node_info();
    RouterStats router_stats;

    bool found_path;
    t_heap cheapest;
    ConnectionParameters conn_params(ParentNetId::INVALID(),
                                     -1,
                                     false,
                                     std::unordered_map<RRNodeId, int>());
    std::tie(found_path, std::ignore, cheapest) = router_.timing_driven_route_connection_from_route_tree(
        tree.root(),
        sink_node,
        cost_params,
        bounding_box,
        router_stats,
        conn_params,
        true);

    if (found_path) {
        VTR_ASSERT(RRNodeId(cheapest.index) == sink_node);

        vtr::optional<const RouteTreeNode&> rt_node_of_sink;
        std::tie(std::ignore, rt_node_of_sink) = tree.update_from_heap(&cheapest, OPEN, nullptr, is_flat_);

        //find delay
        *net_delay = rt_node_of_sink->Tdel;

        VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[tree.root().inode].occ() <= rr_graph.node_capacity(tree.root().inode), "SOURCE should never be congested");
    }

    //VTR_LOG("Explored %zu of %zu (%.2f) RR nodes: path delay %g\n", router_stats.heap_pops, device_ctx.rr_nodes.size(), float(router_stats.heap_pops) / device_ctx.rr_nodes.size(), *net_delay);

    //update_screen(ScreenUpdatePriority::MAJOR, "Profiled delay", ROUTING, nullptr);

    //Reset for the next router call
    router_.reset_path_costs();

    return found_path;
}

//Returns the shortest path delay from src_node to all RR nodes in the RR graph, or NaN if no path exists
vtr::vector<RRNodeId, float> calculate_all_path_delays_from_rr_node(RRNodeId src_rr_node,
                                                                    const t_router_opts& router_opts,
                                                                    bool is_flat) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    vtr::vector<RRNodeId, float> path_delays_to(device_ctx.rr_graph.num_nodes(), std::numeric_limits<float>::quiet_NaN());

    RouteTree tree((RRNodeId(src_rr_node)));

    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;

    t_conn_cost_params cost_params;
    cost_params.criticality = 1.;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;
    /* This function is called during placement. Thus, the flat routing option should be disabled. */
    //TODO: Placement is run with is_flat=false. However, since is_flat is passed, det_routing_arch should
    //be also passed
    VTR_ASSERT(is_flat == false);
    t_det_routing_arch det_routing_arch;
    auto router_lookahead = make_router_lookahead(det_routing_arch, e_router_lookahead::NO_OP,
                                                  /*write_lookahead=*/"", /*read_lookahead=*/"",
                                                  /*segment_inf=*/{},
                                                  is_flat);

    ConnectionRouter<BinaryHeap> router(
        device_ctx.grid,
        *router_lookahead,
        device_ctx.rr_graph.rr_nodes(),
        &g_vpr_ctx.device().rr_graph,
        device_ctx.rr_rc_data,
        device_ctx.rr_graph.rr_switch(),
        route_ctx.rr_node_route_inf,
        is_flat);
    RouterStats router_stats;
    ConnectionParameters conn_params(ParentNetId::INVALID(), OPEN, false, std::unordered_map<RRNodeId, int>());
    vtr::vector<RRNodeId, t_heap> shortest_paths = router.timing_driven_find_all_shortest_paths_from_route_tree(tree.root(),
                                                                                                                cost_params,
                                                                                                                bounding_box,
                                                                                                                router_stats,
                                                                                                                conn_params);

    VTR_ASSERT(shortest_paths.size() == device_ctx.rr_graph.num_nodes());
    for (int isink = 0; isink < (int)device_ctx.rr_graph.num_nodes(); ++isink) {
        RRNodeId sink_rr_node(isink);
        if (RRNodeId(sink_rr_node) == src_rr_node) {
            path_delays_to[sink_rr_node] = 0.;
        } else {
            if (!shortest_paths[sink_rr_node].index.is_valid()) continue;

            VTR_ASSERT(RRNodeId(shortest_paths[sink_rr_node].index) == sink_rr_node);

            //Build the routing tree to get the delay
            tree = RouteTree(RRNodeId(src_rr_node));
            vtr::optional<const RouteTreeNode&> rt_node_of_sink;
            std::tie(std::ignore, rt_node_of_sink) = tree.update_from_heap(&shortest_paths[sink_rr_node], OPEN, nullptr, router_opts.flat_routing);

            VTR_ASSERT(rt_node_of_sink->inode == RRNodeId(sink_rr_node));

            path_delays_to[sink_rr_node] = rt_node_of_sink->Tdel;
        }
    }
    router.reset_path_costs();

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

void alloc_routing_structs(t_chan_width chan_width,
                           const t_router_opts& router_opts,
                           t_det_routing_arch* det_routing_arch,
                           std::vector<t_segment_inf>& segment_inf,
                           const t_direct_inf* directs,
                           const int num_directs,
                           bool is_flat) {
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
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs, num_directs,
                    &warnings,
                    is_flat);

    alloc_and_load_rr_node_route_structs();
}

void free_routing_structs() {
    free_route_structs();
}
