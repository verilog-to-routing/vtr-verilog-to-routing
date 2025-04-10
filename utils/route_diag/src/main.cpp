// Tool to test routing from one node to another node.
//
// This tool is intended to allow extremely verbose router output on exactly
// one path to diagnose router behavior, e.g. look ahead and A* characteristics.
//
// Usage follows typically VPR invocation, except that no circuit is actually
// used.
//
// Tool can either perform one route between a source (--source_rr_node) and
// a sink (--sink_rr_node), or profile a source to all tiles (set
// --source_rr_node and "--profile_source true").

#include <fstream>

#include "vtr_error.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "tatum/error.hpp"

#include "vpr_error.h"
#include "vpr_api.h"
#include "vpr_signal_handler.h"

#include "globals.h"

#include "net_delay.h"
#include "place_and_route.h"
#include "router_delay_profiling.h"
#include "route_tree.h"
#include "route_common.h"
#include "route_net.h"
#include "rr_graph.h"
#include "compute_delta_delays_utils.h"

struct t_route_util_options {
    /* Router diag tool Options */
    argparse::ArgValue<int> source_rr_node;
    argparse::ArgValue<int> sink_rr_node;
    argparse::ArgValue<bool> profile_source;

    t_options options;
};

t_route_util_options read_route_util_options(int argc, const char** argv);

/*
 * Exit codes to signal success/failure to scripts
 * calling vpr
 */
constexpr int SUCCESS_EXIT_CODE = 0; //Everything OK
constexpr int ERROR_EXIT_CODE = 1; //Something went wrong internally
constexpr int INTERRUPTED_EXIT_CODE = 3; //VPR was interrupted by the user (e.g. SIGINT/ctr-C)

static void do_one_route(const Netlist<>& net_list,
                         const t_det_routing_arch& det_routing_arch,
                         RRNodeId source_node,
                         RRNodeId sink_node,
                         const t_router_opts& router_opts,
                         const std::vector<t_segment_inf>& segment_inf,
                         bool is_flat) {
    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    RouteTree tree((RRNodeId(source_node)));

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(1);

    //maximum bounding box for placement
    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;
    bounding_box.layer_min = 0;
    bounding_box.layer_max = device_ctx.grid.get_num_layers() - 1;

    t_conn_cost_params cost_params;
    cost_params.criticality = router_opts.max_criticality;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;

    route_budgets budgeting_inf(net_list, is_flat);


    RouterStats router_stats;
    auto router_lookahead = make_router_lookahead(det_routing_arch,
                                                  router_opts.lookahead_type,
                                                  router_opts.write_router_lookahead,
                                                  router_opts.read_router_lookahead,
                                                  segment_inf,
                                                  is_flat);

    ConnectionRouter<FourAryHeap> router(
        device_ctx.grid,
        *router_lookahead,
            device_ctx.rr_graph.rr_nodes(),
            &device_ctx.rr_graph,
            device_ctx.rr_rc_data,
            device_ctx.rr_graph.rr_switch(),
            g_vpr_ctx.mutable_routing().rr_node_route_inf,
            is_flat);
    enable_router_debug(router_opts, ParentNetId(), sink_node, 1, &router);
    bool found_path;
    RTExploredNode cheapest;
    ConnectionParameters conn_params(ParentNetId::INVALID(),
                                     -1,
                                     false,
                                     std::unordered_map<RRNodeId, int>());
    std::tie(found_path, std::ignore, cheapest) = router.timing_driven_route_connection_from_route_tree(tree.root(),
                                                                                                    sink_node,
                                                                                                    cost_params,
                                                                                                    bounding_box,
                                                                                                    router_stats,
                                                                                                    conn_params);

    if (found_path) {
        VTR_ASSERT(cheapest.index == sink_node);

        vtr::optional<const RouteTreeNode&> rt_node_of_sink;
        std::tie(std::ignore, rt_node_of_sink) = tree.update_from_heap(&cheapest, OPEN, nullptr, router_opts.flat_routing);

        //find delay
        float net_delay = rt_node_of_sink.value().Tdel;
        VTR_LOG("Routed successfully, delay = %g!\n", net_delay);
        VTR_LOG("\n");
        tree.print();
        VTR_LOG("\n");

        VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[tree.root().inode].occ() <= rr_graph.node_capacity(tree.root().inode), "SOURCE should never be congested");
    } else {
        VTR_LOG("Routing failed");
    }

    //Reset for the next router call
    router.reset_path_costs();
}

static void profile_source(const Netlist<>& net_list,
                           const t_det_routing_arch& det_routing_arch,
                           RRNodeId source_rr_node,
                           const t_router_opts& router_opts,
                           const std::vector<t_segment_inf>& segment_inf,
                           bool is_flat) {
    vtr::ScopedStartFinishTimer timer("Profiling source");
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;
    // TODO: We assume if this function is called, the grid has a 2D structure - It assumes everything is on layer number 0, so it won't work yet for multi-layer FPGAs
    VTR_ASSERT(grid.get_num_layers() == 1);
    int layer_num = 0;

    auto router_lookahead = make_router_lookahead(det_routing_arch,
                                                  router_opts.lookahead_type,
                                                  router_opts.write_router_lookahead,
                                                  router_opts.read_router_lookahead,
                                                  segment_inf,
                                                  is_flat);
    RouterDelayProfiler profiler(net_list, router_lookahead.get(), is_flat);

    int start_x = 0;
    int end_x = grid.width() - 1;
    int start_y = 0;
    int end_y = grid.height() - 1;

    vtr::Matrix<float> delays({grid.width(), grid.height()},
            std::numeric_limits<float>::infinity());
    vtr::Matrix<int> sink_nodes({grid.width(), grid.height()}, OPEN);

    for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
            if(device_ctx.grid.get_physical_type({sink_x, sink_y, layer_num}) == device_ctx.EMPTY_PHYSICAL_TILE_TYPE) {
                continue;
            }

            auto best_sink_ptcs = get_best_classes(RECEIVER,
                                                   device_ctx.grid.get_physical_type({sink_x, sink_y, layer_num}));
            bool successfully_routed;
            for (int sink_ptc : best_sink_ptcs) {
                VTR_ASSERT(sink_ptc != OPEN);

                //TODO: should pass layer_num instead of 0 to node_lookup once the multi-die FPGAs support is completed
                RRNodeId sink_rr_node = device_ctx.rr_graph.node_lookup().find_node(0, sink_x, sink_y, SINK, sink_ptc);

                if (directconnect_exists(source_rr_node, sink_rr_node)) {
                    //Skip if we shouldn't measure direct connects and a direct connect exists
                    continue;
                }

                VTR_ASSERT(sink_rr_node);

                {
                    vtr::ScopedStartFinishTimer delay_timer(vtr::string_fmt(
                        "Routing Src: %d Sink: %d", source_rr_node,
                        sink_rr_node));

                    successfully_routed = profiler.calculate_delay(RRNodeId(source_rr_node),
                                                                   RRNodeId(sink_rr_node),
                                                                   router_opts,
                                                                   &delays[sink_x][sink_y]);
                }

                if (successfully_routed) {
                    sink_nodes[sink_x][sink_y] = size_t(sink_rr_node);
                    break;
                }
            }
        }
    }

    VTR_LOG("Delay matrix from source_rr_node: %d\n", source_rr_node);
    for(size_t iy = 0; iy < delays.dim_size(1); ++iy) {
        for(size_t ix = 0; ix < delays.dim_size(0); ++ix) {
            VTR_LOG("%g,", delays[ix][iy]);
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");

    VTR_LOG("Sink matrix used for delay matrix:\n");
    for(size_t iy = 0; iy < sink_nodes.dim_size(1); ++iy) {
        for(size_t ix = 0; ix < sink_nodes.dim_size(0); ++ix) {
            VTR_LOG("%d,", sink_nodes[ix][iy]);
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

t_route_util_options read_route_util_options(int argc, const char** argv) {
    //Explicitly initialize for zero initialization
    t_route_util_options args = t_route_util_options();
    auto parser = create_arg_parser(argv[0], args.options);

    auto& route_diag_grp = parser.add_argument_group("route diagnostic options");
    route_diag_grp.add_argument(args.sink_rr_node, "--sink_rr_node")
        .help("Sink RR node to route for route_diag.")
        .show_in(argparse::ShowIn::HELP_ONLY);
    route_diag_grp.add_argument(args.source_rr_node, "--source_rr_node")
        .help("Source RR node to route for route_diag.")
        .show_in(argparse::ShowIn::HELP_ONLY);
    route_diag_grp.add_argument(args.profile_source, "--profile_source")
        .help(
            "Profile routes from source to IPINs at all locations."
            "This is similiar to the placer delay matrix construction.")
        .show_in(argparse::ShowIn::HELP_ONLY);

    parser.parse_args(argc, argv);

    set_conditional_defaults(args.options);

    verify_args(args.options);

    return args;
}

int main(int argc, const char **argv) {
    t_options Options = t_options();
    t_arch Arch = t_arch();
    t_vpr_setup vpr_setup = t_vpr_setup();

    try {
        vpr_install_signal_handler();

        vpr_initialize_logging();

        /* Print title message */
        vpr_print_title();

        t_route_util_options route_options = read_route_util_options(argc, argv);
        Options = route_options.options;

        /* Read architecture, and circuit netlist */
        vpr_init_with_options(&Options, &vpr_setup, &Arch);

        vpr_create_device_grid(vpr_setup, Arch);

        vpr_setup_clock_networks(vpr_setup, Arch);

        bool is_flat = vpr_setup.RouterOpts.flat_routing;

        const Netlist<>& net_list = is_flat ? (const Netlist<>&)g_vpr_ctx.atom().netlist() :
                                            (const Netlist<>&)g_vpr_ctx.clustering().clb_nlist;

        t_chan_width chan_width = setup_chan_width(vpr_setup.RouterOpts,
                                                   Arch.Chans);

        alloc_routing_structs(chan_width,
                              vpr_setup.RouterOpts,
                              &vpr_setup.RoutingArch,
                              vpr_setup.Segments,
                              Arch.directs,
                              is_flat);

        if(route_options.profile_source) {
            profile_source(net_list,
                           vpr_setup.RoutingArch,
                           RRNodeId(route_options.source_rr_node),
                           vpr_setup.RouterOpts,
                           vpr_setup.Segments,
                           is_flat);
        } else {
            do_one_route(net_list,
                         vpr_setup.RoutingArch,
                         RRNodeId(route_options.source_rr_node),
                         RRNodeId(route_options.sink_rr_node),
                         vpr_setup.RouterOpts,
                         vpr_setup.Segments,
                         is_flat);
        }
        free_routing_structs();

        /* free data structures */
        vpr_free_all(Arch, vpr_setup);

    } catch (const tatum::Error& tatum_error) {
        vtr::printf_error(__FILE__, __LINE__, "STA Engine: %s\n", tatum_error.what());

        return ERROR_EXIT_CODE;

    } catch (const VprError& vpr_error) {
        vpr_print_error(vpr_error);

        if (vpr_error.type() == VPR_ERROR_INTERRUPTED) {
            return INTERRUPTED_EXIT_CODE;
        } else {
            return ERROR_EXIT_CODE;
        }

    } catch (const vtr::VtrError& vtr_error) {
        vtr::printf_error(__FILE__, __LINE__, "%s:%d %s\n", vtr_error.filename_c_str(), vtr_error.line(), vtr_error.what());

        return ERROR_EXIT_CODE;
    }

    /* Signal success to scripts */
    return SUCCESS_EXIT_CODE;
}
