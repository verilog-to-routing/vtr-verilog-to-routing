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
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>

#include "vtr_error.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "tatum/error.hpp"

#include "vpr_error.h"
#include "vpr_api.h"
#include "vpr_signal_handler.h"

#include "globals.h"

#include "net_delay.h"
#include "RoutingDelayCalculator.h"
#include "place_and_route.h"
#include "router_delay_profiling.h"
#include "route_tree_type.h"
#include "route_common.h"
#include "route_timing.h"
#include "route_tree_timing.h"
#include "route_export.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "timing_place_lookup.h"

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

static void do_one_route(int source_node, int sink_node,
        const t_router_opts& router_opts,
        const std::vector<t_segment_inf>& segment_inf) {
    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    t_rt_node* rt_root = init_route_tree_to_source_no_net(source_node);
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
    auto router_lookahead = make_router_lookahead(
            router_opts.lookahead_type,
            router_opts.write_router_lookahead,
            router_opts.read_router_lookahead,
            segment_inf
            );
    t_heap* cheapest = timing_driven_route_connection_from_route_tree(rt_root, sink_node, cost_params, bounding_box, *router_lookahead, modified_rr_node_inf, router_stats);

    bool found_path = (cheapest != nullptr);
    if (found_path) {
        VTR_ASSERT(cheapest->index == sink_node);

        t_rt_node* rt_node_of_sink = update_route_tree(cheapest, nullptr);
        free_heap_data(cheapest);

        //find delay
        float net_delay = rt_node_of_sink->Tdel;
        VTR_LOG("Routed successfully, delay = %g!\n", net_delay);
        VTR_LOG("\n");
        print_route_tree_node(rt_root);
        VTR_LOG("\n");
        print_route_tree(rt_root);
        VTR_LOG("\n");

        VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");
        free_route_tree(rt_root);
    } else {
        VTR_LOG("Routed failed");
    }

    //Reset for the next router call
    empty_heap();
    reset_path_costs(modified_rr_node_inf);
}

static void profile_source(int source_rr_node,
        const t_router_opts& router_opts,
        const std::vector<t_segment_inf>& segment_inf) {
    vtr::ScopedStartFinishTimer timer("Profiling source");
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& grid = device_ctx.grid;

    auto router_lookahead = make_router_lookahead(
            router_opts.lookahead_type,
            router_opts.write_router_lookahead,
            router_opts.read_router_lookahead,
            segment_inf
            );
    RouterDelayProfiler profiler(router_lookahead.get());

    int start_x = 0;
    int end_x = grid.width() - 1;
    int start_y = 0;
    int end_y = grid.height() - 1;

    vtr::Matrix<float> delays({grid.width(), grid.height()},
            std::numeric_limits<float>::infinity());
    vtr::Matrix<int> sink_nodes({grid.width(), grid.height()}, OPEN);

    for (int sink_x = start_x; sink_x <= end_x; sink_x++) {
        for (int sink_y = start_y; sink_y <= end_y; sink_y++) {
            if(device_ctx.grid[sink_x][sink_y].type == device_ctx.EMPTY_TYPE) {
                continue;
            }

            auto best_sink_ptcs = get_best_classes(RECEIVER,
                    device_ctx.grid[sink_x][sink_y].type);
            bool successfully_routed;
            for (int sink_ptc : best_sink_ptcs) {
                VTR_ASSERT(sink_ptc != OPEN);

                int sink_rr_node = get_rr_node_index(device_ctx.rr_node_indices, sink_x, sink_y, SINK, sink_ptc);

                if (directconnect_exists(source_rr_node, sink_rr_node)) {
                    //Skip if we shouldn't measure direct connects and a direct connect exists
                    continue;
                }

                VTR_ASSERT(sink_rr_node != OPEN);

                {
                    vtr::ScopedStartFinishTimer delay_timer(vtr::string_fmt(
                        "Routing Src: %d Sink: %d", source_rr_node,
                        sink_rr_node));
                    successfully_routed = profiler.calculate_delay(source_rr_node, sink_rr_node,
                                                        router_opts,
                                                        &delays[sink_x][sink_y]);
                }

                if (successfully_routed) {
                    sink_nodes[sink_x][sink_y] = sink_rr_node;
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


static t_chan_width setup_chan_width(t_router_opts router_opts,
        t_chan_width_dist chan_width_dist) {
    /*we give plenty of tracks, this increases routability for the */
    /*lookup table generation */

    int width_fac;

    if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
        auto& device_ctx = g_vpr_ctx.device();

        auto type = find_most_common_block_type(device_ctx.grid);

        width_fac = 4 * type->num_pins;
        /*this is 2x the value that binary search starts */
        /*this should be enough to allow most pins to   */
        /*connect to tracks in the architecture */
    } else {
        width_fac = router_opts.fixed_channel_width;
    }

    return init_chan(width_fac, chan_width_dist);
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


        t_chan_width chan_width = setup_chan_width(
                vpr_setup.RouterOpts,
                Arch.Chans);
        alloc_routing_structs(
                chan_width,
                vpr_setup.RouterOpts,
                &vpr_setup.RoutingArch,
                vpr_setup.Segments,
                Arch.Directs,
                Arch.num_directs
                );

        if(route_options.profile_source) {
            profile_source(
                route_options.source_rr_node,
                vpr_setup.RouterOpts,
                vpr_setup.Segments
                );
        } else {
            do_one_route(
                    route_options.source_rr_node, 
                    route_options.sink_rr_node,
                    vpr_setup.RouterOpts,
                    vpr_setup.Segments);
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
