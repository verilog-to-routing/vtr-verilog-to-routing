#include <sys/types.h>

#include <cstdio>
#include <ctime>
#include <climits>
#include <cstdlib>
#include <cmath>
using namespace std;

#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "globals.h"
#include "atom_netlist.h"
#include "place_and_route.h"
#include "place.h"
#include "read_place.h"
#include "route_export.h"
#include "draw.h"
#include "stats.h"
#include "check_route.h"
#include "rr_graph.h"
#include "path_delay.h"
#include "net_delay.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "route_common.h"
#include "place_macro.h"
#include "power.h"

#include "RoutingDelayCalculator.h"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"


/******************* Subroutines local to this module ************************/

static int binary_search_place_and_route(t_placer_opts placer_opts,
        t_file_name_opts filename_opts,
        const t_arch* arch,
        bool verify_binary_search, int min_chan_width_hint,
        t_annealing_sched annealing_sched,
        t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        vtr::vector_map<ClusterNetId, float *> &net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
        const t_timing_inf& timing_inf,
#endif
        std::shared_ptr<SetupHoldTimingInfo> timing_info);

static float comp_width(t_chan * chan, float x, float separation);


/************************* Subroutine Definitions ****************************/

bool place_and_route(t_placer_opts placer_opts,
        t_file_name_opts filename_opts,
        const t_arch* arch,
        t_annealing_sched annealing_sched,
        t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        t_timing_inf timing_inf) {

    /* This routine controls the overall placement and routing of a circuit. */
    char msg[vtr::bufsize];

    bool success = false;
    vtr::t_chunk net_delay_ch = {NULL, 0, NULL};

    t_clb_opins_used clb_opins_used_locally; /* [0..cluster_ctx.clb_nlist.blocks().size()-1][0..num_class-1] */
    clock_t begin, end;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& power_ctx = g_vpr_ctx.mutable_power();

    int max_pins_per_clb = 0;
    for (int i = 0; i < device_ctx.num_block_types; ++i) {
        if (device_ctx.block_types[i].num_pins > max_pins_per_clb) {
            max_pins_per_clb = device_ctx.block_types[i].num_pins;
        }
    }

    if (!placer_opts.doPlacement || placer_opts.place_freq == PLACE_NEVER) {
        /* Read the placement from a file */
        read_place(filename_opts.NetFile.c_str(), filename_opts.PlaceFile.c_str(), filename_opts.verify_file_digests, device_ctx.grid);
        sync_grid_to_blocks();
    } else {
        VTR_ASSERT((PLACE_ONCE == placer_opts.place_freq) || (PLACE_ALWAYS == placer_opts.place_freq));
        begin = clock();
        try_place(placer_opts, annealing_sched, arch->Chans, router_opts,
                det_routing_arch, segment_inf,
#ifdef ENABLE_CLASSIC_VPR_STA
                timing_inf,
#endif
                arch->Directs, arch->num_directs);
        print_place(filename_opts.NetFile.c_str(), cluster_ctx.clb_nlist.netlist_id().c_str(), filename_opts.PlaceFile.c_str());
        end = clock();

        vtr::printf_info("Placement took %g seconds.\n", (float) (end - begin) / CLOCKS_PER_SEC);

    }
    begin = clock();
    post_place_sync();

    fflush(stdout);

    int width_fac = router_opts.fixed_channel_width;

    /* build rr graph and return if we're not doing routing */
    if (!router_opts.doRouting) {
        if (width_fac != NO_FIXED_CHANNEL_WIDTH) {
            //Only try if a fixed channel width is specified
            try_graph(width_fac, router_opts, det_routing_arch,
                    segment_inf, arch->Chans,
                    arch->Directs, arch->num_directs);
        }
        return (true);
    }

    //Routing
    //Initialize the delay calculator
	vtr::vector_map<ClusterNetId, float *> net_delay = alloc_net_delay(&net_delay_ch);

    std::shared_ptr<SetupHoldTimingInfo> timing_info = nullptr;
    std::shared_ptr<RoutingDelayCalculator> routing_delay_calc = nullptr;
    if (timing_inf.timing_analysis_enabled) {
        auto& atom_ctx = g_vpr_ctx.atom();

        routing_delay_calc = std::make_shared<RoutingDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, net_delay);

        timing_info = make_setup_hold_timing_info(routing_delay_calc);
    }



        /* If channel width not fixed, use binary search to find min W */
    if (NO_FIXED_CHANNEL_WIDTH == width_fac) {
        //Binary search for the min channel width
        power_ctx.solution_inf.channel_width = binary_search_place_and_route(placer_opts,
                filename_opts,
                arch,
                router_opts.verify_binary_search, router_opts.min_channel_width_hint,
                annealing_sched, router_opts,
                det_routing_arch, segment_inf, net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
                timing_inf,
#endif
                timing_info);
        success = (power_ctx.solution_inf.channel_width > 0 ? true : false);
    } else {
        //Route at the specified channel width
        power_ctx.solution_inf.channel_width = width_fac;
        if (det_routing_arch->directionality == UNI_DIRECTIONAL) {
            if (width_fac % 2 != 0) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "in pack_place_and_route.c: Given odd chan width (%d) for udsd architecture.\n",
                        width_fac);
            }
        }
        /* Other constraints can be left to rr_graph to check since this is one pass routing */

        /* Allocate the major routing structures. */

        clb_opins_used_locally = alloc_route_structs();

#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack *slacks = alloc_and_load_timing_graph(timing_inf);
#endif
        success = try_route(width_fac, router_opts, det_routing_arch,
                segment_inf, net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
                slacks,
                timing_inf,
#endif
                timing_info,
                arch->Chans,
                clb_opins_used_locally, arch->Directs, arch->num_directs,
                ScreenUpdatePriority::MAJOR);

        if (timing_inf.timing_analysis_enabled) {
            if (isEchoFileEnabled(E_ECHO_FINAL_ROUTING_TIMING_GRAPH)) {
                auto& timing_ctx = g_vpr_ctx.timing();
                tatum::write_echo(getEchoFileName(E_ECHO_FINAL_ROUTING_TIMING_GRAPH),
                        *timing_ctx.graph, *timing_ctx.constraints, *routing_delay_calc, timing_info->analyzer());
            }
        }

        if (success == false) {

            vtr::printf_info("Circuit is unroutable with a channel width factor of %d.\n", width_fac);
            sprintf(msg, "Routing failed with a channel width factor of %d. ILLEGAL routing shown.", width_fac);
        } else {
            check_route(router_opts.route_type, device_ctx.num_rr_switches, clb_opins_used_locally, segment_inf);
            get_serial_num();

            vtr::printf_info("Circuit successfully routed with a channel width factor of %d.\n", width_fac);

            print_route(filename_opts.PlaceFile.c_str(), filename_opts.RouteFile.c_str());

            if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ROUTING_SINK_DELAYS)) {
                print_sink_delays(getEchoFileName(E_ECHO_ROUTING_SINK_DELAYS));
            }

            sprintf(msg, "Routing succeeded with a channel width factor of %d.\n\n", width_fac);
        }

        init_draw_coords(max_pins_per_clb);
        update_screen(ScreenUpdatePriority::MAJOR, msg, ROUTING, timing_info);

#ifdef ENABLE_CLASSIC_VPR_STA
        VTR_ASSERT(slacks->slack);
        free_timing_graph(slacks);
#endif

        VTR_ASSERT(net_delay.size());

        fflush(stdout);
    }

    /* Frees up all the data structure used in vpr_utils. */
    free_port_pin_from_blk_pin();
    free_blk_pin_from_port_pin();
    free_net_delay(net_delay, &net_delay_ch);

    end = clock();

    vtr::printf_info("Routing took %g seconds.\n", (float) (end - begin) / CLOCKS_PER_SEC);

    if (router_opts.switch_usage_analysis) {
        print_switch_usage();
    }
    delete [] device_ctx.switch_fanin_remap;
    device_ctx.switch_fanin_remap = NULL;

    return (success);
}

static int binary_search_place_and_route(t_placer_opts placer_opts,
        t_file_name_opts filename_opts,
        const t_arch* arch,
        bool verify_binary_search, int min_chan_width_hint,
        t_annealing_sched annealing_sched,
        t_router_opts router_opts,
        t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
        vtr::vector_map<ClusterNetId, float *> &net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
        const t_timing_inf& timing_inf,
#endif
        std::shared_ptr<SetupHoldTimingInfo> timing_info) {

    /* This routine performs a binary search to find the minimum number of      *
     * tracks per channel required to successfully route a circuit, and returns *
     * that minimum width_fac.                                                  */

	vtr::vector_map<ClusterNetId, t_trace *>best_routing; /* Saves the best routing found so far. */
    int current, low, high, final;
    int max_pins_per_clb, i;
    bool success, prev_success, prev2_success, Fc_clipped = false;
    char msg[vtr::bufsize];
#ifdef ENABLE_CLASSIC_VPR_STA
    t_slack * slacks = NULL;
#endif
    bool using_minw_hint = false;

    auto& device_ctx = g_vpr_ctx.mutable_device();

    t_clb_opins_used clb_opins_used_locally, saved_clb_opins_used_locally;

    int attempt_count;
    int udsd_multiplier;
    int warnings;

    t_graph_type graph_type;

    /* Allocate the major routing structures. */

    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    max_pins_per_clb = 0;
    for (i = 0; i < device_ctx.num_block_types; i++) {
        t_type_ptr type = &device_ctx.block_types[i];
        //Use the maximum number of pins normalized by block area
        max_pins_per_clb = max(max_pins_per_clb, type->num_pins) / (type->width * type->height);
    }

    clb_opins_used_locally = alloc_route_structs();
    best_routing = alloc_saved_routing();

#ifdef ENABLE_CLASSIC_VPR_STA
    slacks = alloc_and_load_timing_graph(timing_inf);
#endif
    VTR_ASSERT(net_delay.size());

    if (det_routing_arch->directionality == BI_DIRECTIONAL)
        udsd_multiplier = 1;
    else
        udsd_multiplier = 2;

    if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
        current = router_opts.fixed_channel_width + 5 * udsd_multiplier;
        low = router_opts.fixed_channel_width - 1 * udsd_multiplier;
    } else {
        /* Initialize binary serach guess */

        if (min_chan_width_hint > 0) {
            //If the user provided a hint use it as the initial guess
            vtr::printf("Initializing minimum channel width search using specified hint\n");
            current = min_chan_width_hint;
            using_minw_hint = true;
        } else {
            //Otherwise base it off the architecture
            vtr::printf("Initializing minimum channel width search based on maximum CLB pins\n");
            current = max_pins_per_clb + max_pins_per_clb % 2;
        }

        low = -1;
    }

    /* Constraints must be checked to not break rr_graph generator */
    if (det_routing_arch->directionality == UNI_DIRECTIONAL) {
        if (current % 2 != 0) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Tried odd chan width (%d) in uni-directional routing architecture (chan width must be even).\n",
                    current);
        }
    } else {
        if (det_routing_arch->Fs % 3) {
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "Fs must be three in bidirectional mode.\n");
        }
    }
    VTR_ASSERT(current > 0);

    high = -1;
    final = -1;

    attempt_count = 0;


    while (final == -1) {

        vtr::printf_info("\n");
        vtr::printf_info("Attempting to route at %d channels (binary search bounds: [%d, %d])\n", current, low, high);
        fflush(stdout);

        /* Check if the channel width is huge to avoid overflow.  Assume the *
         * circuit is unroutable with the current router options if we're    *
         * going to overflow.                                                */
        if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
            if (current > router_opts.fixed_channel_width * 4) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "This circuit appears to be unroutable with the current router options. Last failed at %d.\n"
                        "Aborting routing procedure.\n", low);
            }
        } else {
            if (current > 1000) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "This circuit requires a channel width above 1000, probably is not going to route.\n"
                        "Aborting routing procedure.\n");
            }
        }

        if ((current * 3) < det_routing_arch->Fs) {
            vtr::printf_info("Width factor is now below specified Fs. Stop search.\n");
            final = high;
            break;
        }

        if (placer_opts.place_freq == PLACE_ALWAYS) {
            placer_opts.place_chan_width = current;
            try_place(placer_opts, annealing_sched, arch->Chans,
                    router_opts, det_routing_arch, segment_inf,
#ifdef ENABLE_CLASSIC_VPR_STA
                    timing_inf,
#endif
                    arch->Directs, arch->num_directs);
        }
        success = try_route(current, router_opts, det_routing_arch, segment_inf,
                net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
                slacks,
                timing_inf,
#endif
                timing_info,
                arch->Chans,
                clb_opins_used_locally, arch->Directs, arch->num_directs,
                (attempt_count == 0) ? ScreenUpdatePriority::MAJOR : ScreenUpdatePriority::MINOR);
        attempt_count++;
        fflush(stdout);

        float scale_factor = 2;

        if (success && !Fc_clipped) {
            if (current == high) {
                /* Can't go any lower */
                final = current;
            }
            high = current;

            /* If Fc_output is too high, set to full connectivity but warn the user */
            if (Fc_clipped) {
                vtr::printf_warning(__FILE__, __LINE__,
                        "Fc_output was too high and was clipped to full (maximum) connectivity.\n");
            }

            /* Save routing in case it is best. */
            save_routing(best_routing, clb_opins_used_locally, saved_clb_opins_used_locally);

            //If the user gave us a minW hint (and we routed successfully at that width)
            //make the initial guess closer to the current value instead of the standard guess. 
            //
            //To avoid wasting time at unroutable channel widths we want to determine an un-routable (but close
            //to the hint channel width). Picking a value too far below the hint may cause us to waste time
            //at an un-routable channel width.  Picking a value too close to the hint may cause a spurious
            //failure (c.f. verify_binary_search). The scale_factor below seems a reasonable compromise.
            //
            //Note this is only active for only the first re-routing after the initial guess, 
            //and we use the default scale_factor otherwise
            if (using_minw_hint && attempt_count == 1) {
                scale_factor = 1.1;
            }

            if ((high - low) <= 1 * udsd_multiplier)
                final = high;
            if (low != -1) {
                current = (high + low) / scale_factor;
            } else {
                current = high / scale_factor; /* haven't found lower bound yet */
            }
        } else { /* last route not successful */
            if (success && Fc_clipped) {
                vtr::printf_info("Routing rejected, Fc_output was too high.\n");
                success = false;
            }
            low = current;
            if (high != -1) {

                if ((high - low) <= 1 * udsd_multiplier)
                    final = high;

                current = (high + low) / scale_factor;
            } else {
                if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
                    /* FOR Wneed = f(Fs) search */
                    if (low < router_opts.fixed_channel_width + 30) {
                        current = low + 5 * udsd_multiplier;
                    } else {
                        vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                                "Aborting: Wneed = f(Fs) search found exceedingly large Wneed (at least %d).\n", low);
                    }
                } else {
                    current = low * scale_factor; /* Haven't found upper bound yet */
                }
            }
        }
        current = current + current % udsd_multiplier;
    }

    /* The binary search above occassionally does not find the minimum    *
     * routeable channel width.  Sometimes a circuit that will not route  *
     * in 19 channels will route in 18, due to router flukiness.  If      *  
     * verify_binary_search is set, the code below will ensure that FPGAs *
     * with channel widths of final-2 and final-3 wil not route           *  
     * successfully.  If one does route successfully, the router keeps    *
     * trying smaller channel widths until two in a row (e.g. 8 and 9)    *
     * fail.                                                              */

    if (verify_binary_search) {

        vtr::printf_info("\n");
        vtr::printf_info("Verifying that binary search found min channel width...\n");

        prev_success = true; /* Actually final - 1 failed, but this makes router */
        /* try final-2 and final-3 even if both fail: safer */
        prev2_success = true;

        current = final -2;

        while (prev2_success || prev_success) {
            if ((router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH)
                    && (current < router_opts.fixed_channel_width)) {
                break;
            }
            fflush(stdout);
            if (current < 1)
                break;
            if (placer_opts.place_freq == PLACE_ALWAYS) {
                placer_opts.place_chan_width = current;
                try_place(placer_opts, annealing_sched, arch->Chans,
                        router_opts, det_routing_arch, segment_inf,
#ifdef ENABLE_CLASSIC_VPR_STA
                        timing_inf,
#endif
                        arch->Directs, arch->num_directs);
            }
            success = try_route(current, router_opts, det_routing_arch,
                    segment_inf, net_delay,
#ifdef ENABLE_CLASSIC_VPR_STA
                    slacks,
                    timing_inf,
#endif
                    timing_info,
                    arch->Chans, clb_opins_used_locally, arch->Directs, arch->num_directs,
                    ScreenUpdatePriority::MINOR);

            if (success && Fc_clipped == false) {
                final = current;
                save_routing(best_routing, clb_opins_used_locally,
                        saved_clb_opins_used_locally);

                if (placer_opts.place_freq == PLACE_ALWAYS) {
                    auto& cluster_ctx = g_vpr_ctx.clustering();
                    print_place(filename_opts.NetFile.c_str(), cluster_ctx.clb_nlist.netlist_id().c_str(),
                            filename_opts.PlaceFile.c_str());
                }
            }

            prev2_success = prev_success;
            prev_success = success;
            current--;
            if (det_routing_arch->directionality == UNI_DIRECTIONAL) {
                current--; /* width must be even */
            }
        }
    }

    /* End binary search verification. */
    /* Restore the best placement (if necessary), the best routing, and  *
     * * the best channel widths for final drawing and statistics output.  */
    init_chan(final, arch->Chans);

    free_rr_graph();

    create_rr_graph(graph_type, device_ctx.num_block_types, device_ctx.block_types, device_ctx.grid,
            &device_ctx.chan_width, det_routing_arch->switch_block_type,
            det_routing_arch->Fs, det_routing_arch->switchblocks,
            det_routing_arch->num_segment,
            device_ctx.num_arch_switches, segment_inf,
            det_routing_arch->global_route_switch,
            det_routing_arch->delayless_switch,
            det_routing_arch->wire_to_arch_ipin_switch,
            router_opts.base_cost_type,
            router_opts.trim_empty_channels,
            router_opts.trim_obs_channels,
            arch->Directs, arch->num_directs,
            det_routing_arch->dump_rr_structs_file,
            &det_routing_arch->wire_to_rr_ipin_switch,
            &device_ctx.num_rr_switches,
            &warnings, router_opts.write_rr_graph_name,
            router_opts.read_rr_graph_name, false);

    restore_routing(best_routing, clb_opins_used_locally,
            saved_clb_opins_used_locally);
    check_route(router_opts.route_type, device_ctx.num_rr_switches,
            clb_opins_used_locally, segment_inf);
    get_serial_num();

    if (Fc_clipped) {
        vtr::printf_warning(__FILE__, __LINE__,
                "Best routing Fc_output too high, clipped to full (maximum) connectivity.\n");
    }
    vtr::printf_info("Best routing used a channel width factor of %d.\n", final);

    print_route(filename_opts.PlaceFile.c_str(), filename_opts.RouteFile.c_str());

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_ROUTING_SINK_DELAYS)) {
        print_sink_delays(getEchoFileName(E_ECHO_ROUTING_SINK_DELAYS));
    }

    init_draw_coords(max_pins_per_clb);
    sprintf(msg, "Routing succeeded with a channel width factor of %d.", final);
    update_screen(ScreenUpdatePriority::MAJOR, msg, ROUTING, timing_info);

    free_saved_routing(best_routing);
    fflush(stdout);

#ifdef ENABLE_CLASSIC_VPR_STA
    free_timing_graph(slacks);
#endif

    return (final);

}

void init_chan(int cfactor, t_chan_width_dist chan_width_dist) {

    /* Assigns widths to channels (in tracks).  Minimum one track          * 
     * per channel. The channel distributions read from the architecture  *
     * file are scaled by cfactor.                                         */

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;

    t_chan chan_x_dist = chan_width_dist.chan_x_dist;
    t_chan chan_y_dist = chan_width_dist.chan_y_dist;

    if (grid.height() > 1) {
        int num_channels = grid.height() - 1;
        VTR_ASSERT(num_channels > 0);
        float separation = 1.0 / num_channels; /* Norm. distance between two channels. */

        for (size_t i = 0; i < grid.height(); ++i) {
            float y = float(i) / num_channels;
            device_ctx.chan_width.x_list[i] = (int) floor(cfactor * comp_width(&chan_x_dist, y, separation) + 0.5);

            device_ctx.chan_width.x_list[i] = max(device_ctx.chan_width.x_list[i], 1); //Minimum channel width 1
        }
    }

    if (grid.width() > 1) {
        int num_channels = grid.width() - 1;
        VTR_ASSERT(num_channels > 0);
        float separation = 1.0 / num_channels; /* Norm. distance between two channels. */

        for (size_t i = 0; i < grid.width(); ++i) { //-2 for no perim channels
            float x = float(i) / num_channels;

            device_ctx.chan_width.y_list[i] = (int) floor(cfactor * comp_width(&chan_y_dist, x, separation) + 0.5);
            device_ctx.chan_width.y_list[i] = max(device_ctx.chan_width.y_list[i], 1); //Minimum channel width 1
        }
    }

    device_ctx.chan_width.max = 0;
    device_ctx.chan_width.x_max = device_ctx.chan_width.y_max = INT_MIN;
    device_ctx.chan_width.x_min = device_ctx.chan_width.y_min = INT_MAX;
    for (size_t i = 0; i < grid.height(); ++i) {
        device_ctx.chan_width.max = max(device_ctx.chan_width.max, device_ctx.chan_width.x_list[i]);
        device_ctx.chan_width.x_max = max(device_ctx.chan_width.x_max, device_ctx.chan_width.x_list[i]);
        device_ctx.chan_width.x_min = min(device_ctx.chan_width.x_min, device_ctx.chan_width.x_list[i]);
    }
    for (size_t i = 0; i < grid.width(); ++i) {
        device_ctx.chan_width.max = max(device_ctx.chan_width.max, device_ctx.chan_width.y_list[i]);
        device_ctx.chan_width.y_max = max(device_ctx.chan_width.y_max, device_ctx.chan_width.y_list[i]);
        device_ctx.chan_width.y_min = min(device_ctx.chan_width.y_min, device_ctx.chan_width.y_list[i]);
    }

#ifdef VERBOSE
    vtr::printf_info("\n");
    vtr::printf_info("device_ctx.chan_width.x_list:\n");
    for (size_t i = 0; i < grid.height(); ++i) {
        vtr::printf_info("%d  ", device_ctx.chan_width.x_list[i]);
    }
    vtr::printf_info("\n");
    vtr::printf_info("device_ctx.chan_width.y_list:\n");
    for (size_t i = 0; i < grid.width(); ++i) {
        vtr::printf_info("%d  ", device_ctx.chan_width.y_list[i]);
    }
    vtr::printf_info("\n");
#endif
}

static float comp_width(t_chan * chan, float x, float separation) {

    /* Return the relative channel density.  *chan points to a channel   *
     * functional description data structure, and x is the distance      *   
     * (between 0 and 1) we are across the chip.  separation is the      *   
     * distance between two channels, in the 0 to 1 coordinate system.   */

    float val;

    switch (chan->type) {

        case UNIFORM:
            val = chan->peak;
            break;

        case GAUSSIAN:
            val = (x - chan->xpeak) * (x - chan->xpeak)
                    / (2 * chan->width * chan->width);
            val = chan->peak * exp(-val);
            val += chan->dc;
            break;

        case PULSE:
            val = (float) fabs((double) (x - chan->xpeak));
            if (val > chan->width / 2.) {
                val = 0;
            } else {
                val = chan->peak;
            }
            val += chan->dc;
            break;

        case DELTA:
            val = x - chan->xpeak;
            if (val > -separation / 2. && val <= separation / 2.)
                val = chan->peak;
            else
                val = 0.;
            val += chan->dc;
            break;

        default:
            vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                    "in comp_width: Unknown channel type %d.\n", chan->type);
            val = OPEN;
            break;
    }

    return (val);
}

/*
 * After placement, logical pins for blocks, and nets must be updated to correspond with physical pins of type.
 * This is required by blocks with capacity > 1 (e.g. typically IOs with multiple instaces in each placement
 * gride location). Since they may be swapped around during placement, we need to update which pins the various
 * nets use.
 *
 * This updates both the external inter-block net connecitivity (i.e. the clustered netlist), and the intra-block
 * connectivity (since the internal pins used also change).
 *
 * This function should only be called once 
 */
void post_place_sync() {
    /* Go through each block */
	auto& cluster_ctx = g_vpr_ctx.clustering();
	for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
        place_sync_external_block_connections(block_id);
    }
}
