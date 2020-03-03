#include <sys/types.h>

#include <cstdio>
#include <ctime>
#include <climits>
#include <cstdlib>
#include <cmath>

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
#include "read_route.h"
#include "route_export.h"
#include "draw.h"
#include "stats.h"
#include "check_route.h"
#include "rr_graph.h"
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

static float comp_width(t_chan* chan, float x, float separation);

/************************* Subroutine Definitions ****************************/

int binary_search_place_and_route(const t_placer_opts& placer_opts_ref,
                                  const t_annealing_sched& annealing_sched,
                                  const t_router_opts& router_opts,
                                  const t_analysis_opts& analysis_opts,
                                  const t_file_name_opts& filename_opts,
                                  const t_arch* arch,
                                  bool verify_binary_search,
                                  int min_chan_width_hint,
                                  t_det_routing_arch* det_routing_arch,
                                  std::vector<t_segment_inf>& segment_inf,
                                  vtr::vector<ClusterNetId, float*>& net_delay,
                                  std::shared_ptr<SetupHoldTimingInfo> timing_info,
                                  std::shared_ptr<RoutingDelayCalculator> delay_calc) {
    /* This routine performs a binary search to find the minimum number of      *
     * tracks per channel required to successfully route a circuit, and returns *
     * that minimum width_fac.                                                  */

    vtr::vector<ClusterNetId, t_trace*> best_routing; /* Saves the best routing found so far. */
    int current, low, high, final;
    bool success, prev_success, prev2_success, Fc_clipped = false;
    bool using_minw_hint = false;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    t_clb_opins_used saved_clb_opins_used_locally;

    int attempt_count;
    int udsd_multiplier;
    int warnings;

    t_graph_type graph_type;

    /* We have chosen to pass placer_opts_ref by reference because of its large size. *      
     * However, since the value is mutated later in the function, we declare a        *
     * mutable variable called placer_opts equal to placer_opts_ref.                  */

    t_placer_opts placer_opts = placer_opts_ref;

    /* Allocate the major routing structures. */

    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    best_routing = alloc_saved_routing();

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
            VTR_LOG("Initializing minimum channel width search using specified hint\n");
            current = min_chan_width_hint;
            using_minw_hint = true;
        } else {
            //Otherwise base it off the architecture
            VTR_LOG("Initializing minimum channel width search based on maximum CLB pins\n");
            int max_pins = max_pins_per_grid_tile();
            current = max_pins + max_pins % 2;
        }

        low = -1;
    }

    /* Constraints must be checked to not break rr_graph generator */
    if (det_routing_arch->directionality == UNI_DIRECTIONAL) {
        if (current % 2 != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Tried odd chan width (%d) in uni-directional routing architecture (chan width must be even).\n",
                            current);
        }
    } else {
        if (det_routing_arch->Fs % 3) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Fs must be three in bidirectional mode.\n");
        }
    }
    VTR_ASSERT(current > 0);

    high = -1;
    final = -1;

    attempt_count = 0;

    while (final == -1) {
        VTR_LOG("\n");
        VTR_LOG("Attempting to route at %d channels (binary search bounds: [%d, %d])\n", current, low, high);
        fflush(stdout);

        /* Check if the channel width is huge to avoid overflow.  Assume the *
         * circuit is unroutable with the current router options if we're    *
         * going to overflow.                                                */
        if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
            if (current > router_opts.fixed_channel_width * 4) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "This circuit appears to be unroutable with the current router options. Last failed at %d.\n"
                                "Aborting routing procedure.\n",
                                low);
            }
        } else {
            if (current > 1000) {
                VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                "This circuit requires a channel width above 1000, probably is not going to route.\n"
                                "Aborting routing procedure.\n");
            }
        }

        if ((current * 3) < det_routing_arch->Fs) {
            VTR_LOG("Width factor is now below specified Fs. Stop search.\n");
            final = high;
            break;
        }

        if (placer_opts.place_freq == PLACE_ALWAYS) {
            placer_opts.place_chan_width = current;
            try_place(placer_opts, annealing_sched, router_opts, analysis_opts,
                      arch->Chans, det_routing_arch, segment_inf,
                      arch->Directs, arch->num_directs);
        }
        success = try_route(current,
                            router_opts,
                            analysis_opts,
                            det_routing_arch, segment_inf,
                            net_delay,
                            timing_info,
                            delay_calc,
                            arch->Chans,
                            arch->Directs, arch->num_directs,
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
                VTR_LOG_WARN("Fc_output was too high and was clipped to full (maximum) connectivity.\n");
            }

            /* Save routing in case it is best. */
            save_routing(best_routing, route_ctx.clb_opins_used_locally, saved_clb_opins_used_locally);

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

            if ((high - std::max(low, 0)) <= 1 * udsd_multiplier) { //No more steps
                final = high;
            }

            if (low != -1) { //Have lower-bound, step to midpoint
                current = (high + low) / scale_factor;
            } else { //Haven't found lower bound yet
                current = high / scale_factor;

                if (std::abs(current - high) < udsd_multiplier) {
                    //If high and scale_factor are both small, we might have ended
                    //up with no change in current.
                    //In that case, ensure we reduce current by at least the track multiplier
                    current = high - udsd_multiplier;
                }
                VTR_ASSERT(current < high);
            }

        } else { /* last route not successful */
            if (success && Fc_clipped) {
                VTR_LOG("Routing rejected, Fc_output was too high.\n");
                success = false;
            }
            low = current;
            if (high != -1) {
                if ((high - low) <= 1 * udsd_multiplier) { //No more steps
                    final = high;
                }

                current = (high + low) / scale_factor; //Step to midpoint
            } else {
                if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
                    /* FOR Wneed = f(Fs) search */
                    if (low < router_opts.fixed_channel_width + 30) {
                        current = low + 5 * udsd_multiplier;
                    } else {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "Aborting: Wneed = f(Fs) search found exceedingly large Wneed (at least %d).\n", low);
                    }
                } else {
                    current = low * scale_factor; /* Haven't found upper bound yet */

                    if (std::abs(current - low) < udsd_multiplier) {
                        //If high and scale_factor are both small, we might have ended
                        //up with no change in current.
                        //In that case, ensure we increase current by at least the track multiplier
                        current = high + udsd_multiplier;
                    }
                    VTR_ASSERT(current > low);
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
        VTR_LOG("\n");
        VTR_LOG("Verifying that binary search found min channel width...\n");

        prev_success = true; /* Actually final - 1 failed, but this makes router */
        /* try final-2 and final-3 even if both fail: safer */
        prev2_success = true;

        current = final - 2;

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
                try_place(placer_opts, annealing_sched, router_opts, analysis_opts,
                          arch->Chans, det_routing_arch, segment_inf,
                          arch->Directs, arch->num_directs);
            }
            success = try_route(current,
                                router_opts,
                                analysis_opts,
                                det_routing_arch,
                                segment_inf, net_delay,
                                timing_info,
                                delay_calc,
                                arch->Chans, arch->Directs, arch->num_directs,
                                ScreenUpdatePriority::MINOR);

            if (success && Fc_clipped == false) {
                final = current;
                save_routing(best_routing, route_ctx.clb_opins_used_locally,
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
    t_chan_width chan_width = init_chan(final, arch->Chans);

    free_rr_graph();

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
                    arch->Directs, arch->num_directs,
                    &warnings);

    init_draw_coords(final);
    restore_routing(best_routing, route_ctx.clb_opins_used_locally, saved_clb_opins_used_locally);

    if (Fc_clipped) {
        VTR_LOG_WARN("Best routing Fc_output too high, clipped to full (maximum) connectivity.\n");
    }
    VTR_LOG("Best routing used a channel width factor of %d.\n", final);

    print_route(filename_opts.PlaceFile.c_str(), filename_opts.RouteFile.c_str());

    free_saved_routing(best_routing);
    fflush(stdout);

    return (final);
}

t_chan_width init_chan(int cfactor, t_chan_width_dist chan_width_dist) {
    /* Assigns widths to channels (in tracks).  Minimum one track          *
     * per channel. The channel distributions read from the architecture  *
     * file are scaled by cfactor.                                         */

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& grid = device_ctx.grid;

    t_chan chan_x_dist = chan_width_dist.chan_x_dist;
    t_chan chan_y_dist = chan_width_dist.chan_y_dist;

    t_chan_width chan_width;
    chan_width.x_list.resize(grid.height());
    chan_width.y_list.resize(grid.width());

    if (grid.height() > 1) {
        int num_channels = grid.height() - 1;
        VTR_ASSERT(num_channels > 0);
        float separation = 1.0 / num_channels; /* Norm. distance between two channels. */

        for (size_t i = 0; i < grid.height(); ++i) {
            float y = float(i) / num_channels;
            chan_width.x_list[i] = (int)floor(cfactor * comp_width(&chan_x_dist, y, separation) + 0.5);
            chan_width.x_list[i] = std::max(chan_width.x_list[i], 1); //Minimum channel width 1
        }
    }

    if (grid.width() > 1) {
        int num_channels = grid.width() - 1;
        VTR_ASSERT(num_channels > 0);
        float separation = 1.0 / num_channels; /* Norm. distance between two channels. */

        for (size_t i = 0; i < grid.width(); ++i) { //-2 for no perim channels
            float x = float(i) / num_channels;

            chan_width.y_list[i] = (int)floor(cfactor * comp_width(&chan_y_dist, x, separation) + 0.5);
            chan_width.y_list[i] = std::max(chan_width.y_list[i], 1); //Minimum channel width 1
        }
    }

    chan_width.max = 0;
    chan_width.x_max = chan_width.y_max = INT_MIN;
    chan_width.x_min = chan_width.y_min = INT_MAX;
    for (size_t i = 0; i < grid.height(); ++i) {
        chan_width.max = std::max(chan_width.max, chan_width.x_list[i]);
        chan_width.x_max = std::max(chan_width.x_max, chan_width.x_list[i]);
        chan_width.x_min = std::min(chan_width.x_min, chan_width.x_list[i]);
    }
    for (size_t i = 0; i < grid.width(); ++i) {
        chan_width.max = std::max(chan_width.max, chan_width.y_list[i]);
        chan_width.y_max = std::max(chan_width.y_max, chan_width.y_list[i]);
        chan_width.y_min = std::min(chan_width.y_min, chan_width.y_list[i]);
    }

#ifdef VERBOSE
    VTR_LOG("\n");
    VTR_LOG("device_ctx.chan_width.x_list:\n");
    for (size_t i = 0; i < grid.height(); ++i) {
        VTR_LOG("%d  ", chan_width.x_list[i]);
    }
    VTR_LOG("\n");
    VTR_LOG("device_ctx.chan_width.y_list:\n");
    for (size_t i = 0; i < grid.width(); ++i) {
        VTR_LOG("%d  ", chan_width.y_list[i]);
    }
    VTR_LOG("\n");
#endif

    return chan_width;
}

static float comp_width(t_chan* chan, float x, float separation) {
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
            val = (float)fabs((double)(x - chan->xpeak));
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
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
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
