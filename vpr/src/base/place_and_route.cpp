
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include "flat_placement_types.h"
#include "place_macro.h"
#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "globals.h"
#include "place_and_route.h"
#include "place.h"
#include "read_place.h"
#include "read_route.h"
#include "route.h"
#include "route_export.h"
#include "draw.h"
#include "rr_graph.h"
#include "read_xml_arch_file.h"
#include "route_common.h"

#include "RoutingDelayCalculator.h"

/******************* Subroutines local to this module ************************/

static int compute_chan_width(int cfactor, t_chan chan_dist, float distance, float separation, e_graph_type graph_directionality);
static float comp_width(t_chan* chan, float x, float separation);

/************************* Subroutine Definitions ****************************/

/**
 * @brief This routine performs a binary search to find the minimum number of
 *        tracks per channel required to successfully route a circuit, and returns
 *        that minimum width_fac.
 */
int binary_search_place_and_route(const Netlist<>& placement_net_list,
                                  const Netlist<>& router_net_list,
                                  const t_placer_opts& placer_opts_ref,
                                  const t_router_opts& router_opts,
                                  const t_analysis_opts& analysis_opts,
                                  const t_noc_opts& noc_opts,
                                  const t_file_name_opts& filename_opts,
                                  const t_arch* arch,
                                  bool verify_binary_search,
                                  int min_chan_width_hint,
                                  t_det_routing_arch& det_routing_arch,
                                  std::vector<t_segment_inf>& segment_inf,
                                  NetPinsMatrix<float>& net_delay,
                                  const std::shared_ptr<SetupHoldTimingInfo>& timing_info,
                                  const std::shared_ptr<RoutingDelayCalculator>& delay_calc,
                                  bool is_flat) {
    vtr::vector<ParentNetId, vtr::optional<RouteTree>> best_routing; /* Saves the best routing found so far. */
    int current, low, high, final;
    bool success, prev_success, prev2_success, Fc_clipped = false;
    bool using_minw_hint = false;

    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    t_clb_opins_used saved_clb_opins_used_locally;

    int attempt_count;
    int udsd_multiplier;
    int warnings;

    e_graph_type graph_type;
    e_graph_type graph_directionality;

    // We have chosen to pass placer_opts_ref by reference because of its large size.
    // However, since the value is mutated later in the function, we declare a
    // mutable variable called placer_opts equal to placer_opts_ref.

    t_placer_opts placer_opts = placer_opts_ref;

    // Allocate the major routing structures.

    if (router_opts.route_type == e_route_type::GLOBAL) {
        graph_type = e_graph_type::GLOBAL;
        graph_directionality = e_graph_type::BIDIR;
    } else {
        graph_type = (det_routing_arch.directionality == BI_DIRECTIONAL ? e_graph_type::BIDIR : e_graph_type::UNIDIR);
        // Branch on tileable routing
        if (det_routing_arch.directionality == UNI_DIRECTIONAL && det_routing_arch.tileable) {
            graph_type = e_graph_type::UNIDIR_TILEABLE;
        }
        graph_directionality = (det_routing_arch.directionality == BI_DIRECTIONAL ? e_graph_type::BIDIR : e_graph_type::UNIDIR);
    }

    VTR_ASSERT(!net_delay.empty());

    if (det_routing_arch.directionality == BI_DIRECTIONAL)
        udsd_multiplier = 1;
    else
        udsd_multiplier = 2;

    if (router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) {
        current = router_opts.fixed_channel_width + 5 * udsd_multiplier;
        low = router_opts.fixed_channel_width - 1 * udsd_multiplier;
    } else {
        // Initialize binary serach guess

        if (min_chan_width_hint > 0) {
            // If the user provided a hint use it as the initial guess
            VTR_LOG("Initializing minimum channel width search using specified hint\n");
            current = min_chan_width_hint;
            using_minw_hint = true;
        } else {
            // Otherwise base it off the architecture
            VTR_LOG("Initializing minimum channel width search based on maximum CLB pins\n");
            int max_pins = max_pins_per_grid_tile();
            current = max_pins + max_pins % 2;
        }

        low = -1;
    }

    // Constraints must be checked to not break rr_graph generator
    if (det_routing_arch.directionality == UNI_DIRECTIONAL) {
        if (current % 2 != 0) {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                            "Tried odd chan width (%d) in uni-directional routing architecture (chan width must be even).\n",
                            current);
        }
    } else {
        if (det_routing_arch.Fs % 3) {
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

        // Check if the channel width is huge to avoid overflow.
        // Assume the circuit is unroutable with the current router options if we're going to overflow.
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

        if ((current * 3) < det_routing_arch.Fs) {
            VTR_LOG("Width factor is now below specified Fs. Stop search.\n");
            final = high;
            break;
        }

        if (placer_opts.place_freq == PLACE_ALWAYS) {
            placer_opts.place_chan_width = current;
            try_place(placement_net_list,
                      placer_opts,
                      router_opts,
                      analysis_opts,
                      noc_opts,
                      arch->Chans,
                      det_routing_arch,
                      segment_inf,
                      arch->directs,
                      FlatPlacementInfo(), // Pass empty flat placement info.
                      /*is_flat=*/false);
        }
        success = route(router_net_list,
                        current,
                        router_opts,
                        analysis_opts,
                        det_routing_arch, segment_inf,
                        net_delay,
                        timing_info,
                        delay_calc,
                        arch->Chans,
                        arch->directs,
                        (attempt_count == 0) ? ScreenUpdatePriority::MAJOR : ScreenUpdatePriority::MINOR,
                        is_flat);

        attempt_count++;
        fflush(stdout);

        float scale_factor = 2;

        if (success && !Fc_clipped) {
            if (current == high) {
                // Can't go any lower
                final = current;
            }
            high = current;

            // If Fc_output is too high, set to full connectivity but warn the user
            if (Fc_clipped) {
                VTR_LOG_WARN("Fc_output was too high and was clipped to full (maximum) connectivity.\n");
            }

            // Save routing in case it is best.
            save_routing(best_routing,
                         route_ctx.clb_opins_used_locally,
                         saved_clb_opins_used_locally);

            // If the user gave us a minW hint (and we routed successfully at that width)
            // make the initial guess closer to the current value instead of the standard guess.
            //
            // To avoid wasting time at unroutable channel widths we want to determine an un-routable (but close
            // to the hint channel width). Picking a value too far below the hint may cause us to waste time
            // at an un-routable channel width.  Picking a value too close to the hint may cause a spurious
            // failure (c.f. verify_binary_search). The scale_factor below seems a reasonable compromise.
            //
            // Note this is only active for only the first re-routing after the initial guess,
            // and we use the default scale_factor otherwise
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

        } else { // last route not successful
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
                    // FOR Wneed = f(Fs) search
                    if (low < router_opts.fixed_channel_width + 30) {
                        current = low + 5 * udsd_multiplier;
                    } else {
                        VPR_FATAL_ERROR(VPR_ERROR_ROUTE,
                                        "Aborting: Wneed = f(Fs) search found exceedingly large Wneed (at least %d).\n", low);
                    }
                } else {
                    current = low * scale_factor; // Haven't found upper bound yet

                    if (std::abs(current - low) < udsd_multiplier) {
                        // If high and scale_factor are both small, we might have ended
                        // up with no change in current.
                        // In that case, ensure we increase current by at least the track multiplier
                        current = high + udsd_multiplier;
                    }
                    VTR_ASSERT(current > low);
                }
            }
        }
        current = current + current % udsd_multiplier;
    }

    // The binary search above occasionally does not find the minimum routeable channel width.
    // Sometimes a circuit that will not route in 19 channels will route in 18, due to router flukiness.
    // If verify_binary_search is set, the code below will ensure that FPGAs with channel widths of
    // final-2 and final-3 wil not route successfully.
    // If one does route successfully, the router keeps trying smaller channel widths until two in a
    // row (e.g. 8 and 9) fail.

    if (verify_binary_search) {
        VTR_LOG("\n");
        VTR_LOG("Verifying that binary search found min channel width...\n");

        // Actually final - 1 failed, but this makes router
        prev_success = true;

        // try final-2 and final-3 even if both fail: safer
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
                try_place(placement_net_list, placer_opts, router_opts, analysis_opts, noc_opts,
                          arch->Chans, det_routing_arch, segment_inf,
                          arch->directs,
                          FlatPlacementInfo(), // Pass empty flat placement info.
                          /*is_flat=*/false);
            }

            success = route(router_net_list,
                            current,
                            router_opts,
                            analysis_opts,
                            det_routing_arch,
                            segment_inf,
                            net_delay,
                            timing_info,
                            delay_calc,
                            arch->Chans,
                            arch->directs,
                            ScreenUpdatePriority::MINOR,
                            is_flat);

            if (success && !Fc_clipped) {
                final = current;
                save_routing(best_routing,
                             route_ctx.clb_opins_used_locally,
                             saved_clb_opins_used_locally);

                if (placer_opts.place_freq == PLACE_ALWAYS) {
                    auto& cluster_ctx = g_vpr_ctx.clustering();
                    // Cluster-based net_list is used for placement
                    std::string placement_id = print_place(filename_opts.NetFile.c_str(), cluster_ctx.clb_nlist.netlist_id().c_str(),
                                                           filename_opts.PlaceFile.c_str(), g_vpr_ctx.placement().block_locs());
                    g_vpr_ctx.mutable_placement().placement_id = placement_id;
                }
            }

            prev2_success = prev_success;
            prev_success = success;
            current--;
            if (det_routing_arch.directionality == UNI_DIRECTIONAL) {
                // width must be even
                current--;
            }
        }
    }

    // End binary search verification.
    // Restore the best placement (if necessary), the best routing, and the
    // best channel widths for final drawing and statistics output.
    t_chan_width chan_width = init_chan(final, arch->Chans, graph_directionality);

    free_rr_graph();

    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    arch->directs,
                    &warnings,
                    is_flat);

    init_draw_coords(final, g_vpr_ctx.placement().blk_loc_registry());

    // Allocate and load additional rr_graph information needed only by the router.
    alloc_and_load_rr_node_route_structs(router_opts);

    init_route_structs(router_net_list,
                       router_opts.bb_factor,
                       router_opts.has_choke_point,
                       is_flat);

    restore_routing(best_routing,
                    route_ctx.clb_opins_used_locally,
                    saved_clb_opins_used_locally);

    if (Fc_clipped) {
        VTR_LOG_WARN("Best routing Fc_output too high, clipped to full (maximum) connectivity.\n");
    }
    VTR_LOG("Best routing used a channel width factor of %d.\n", final);

    print_route(router_net_list,
                filename_opts.PlaceFile.c_str(),
                filename_opts.RouteFile.c_str(),
                is_flat);

    fflush(stdout);

    return (final);
}

t_chan_width setup_chan_width(const t_router_opts& router_opts,
                              t_chan_width_dist chan_width_dist) {
    // we give plenty of tracks, this increases routability for the lookup table generation

    e_graph_type graph_directionality;
    int width_fac;

    if (router_opts.fixed_channel_width == NO_FIXED_CHANNEL_WIDTH) {
        auto& device_ctx = g_vpr_ctx.device();

        auto type = find_most_common_tile_type(device_ctx.grid);

        width_fac = 4 * type->num_pins;
        // this is 2x the value that binary search starts
        // this should be enough to allow most pins to
        // connect to tracks in the architecture
    } else {
        width_fac = router_opts.fixed_channel_width;
    }

    if (router_opts.route_type == e_route_type::GLOBAL) {
        graph_directionality = e_graph_type::BIDIR;
    } else {
        graph_directionality = e_graph_type::UNIDIR;
    }

    return init_chan(width_fac, chan_width_dist, graph_directionality);
}

/**
 * @brief Assigns widths to channels (in tracks).
 *
 * Minimum one track per channel. The channel distributions read from
 * the architecture file are scaled by cfactor. The graph directionality
 * is used to determine if the channel width should be rounded to an
 * even number.
 */
t_chan_width init_chan(int cfactor,
                       const t_chan_width_dist& chan_width_dist,
                       e_graph_type graph_directionality) {
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
        // Norm. distance between two channels.
        float separation = 1.0 / num_channels;

        for (size_t i = 0; i < grid.height(); ++i) {
            float y = float(i) / num_channels;
            chan_width.x_list[i] = compute_chan_width(cfactor, chan_x_dist, y, separation, graph_directionality);
            // Minimum channel width 1
            chan_width.x_list[i] = std::max(chan_width.x_list[i], 1);
        }
    }

    if (grid.width() > 1) {
        int num_channels = grid.width() - 1;
        VTR_ASSERT(num_channels > 0);
        // Norm. distance between two channels.
        float separation = 1.0 / num_channels;

        for (size_t i = 0; i < grid.width(); ++i) {
            float x = float(i) / num_channels;
            chan_width.y_list[i] = compute_chan_width(cfactor, chan_y_dist, x, separation, graph_directionality);
            // Minimum channel width 1
            chan_width.y_list[i] = std::max(chan_width.y_list[i], 1);
        }
    }

    auto minmax = std::minmax_element(chan_width.x_list.begin(), chan_width.x_list.end());
    chan_width.x_min = *minmax.first;
    chan_width.x_max = *minmax.second;

    minmax = std::minmax_element(chan_width.y_list.begin(), chan_width.y_list.end());
    chan_width.y_min = *minmax.first;
    chan_width.y_max = *minmax.second;

    chan_width.max = std::max(chan_width.x_max, chan_width.y_max);

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

/**
 * @brief Computes the channel width and adjusts it to be an an even number if unidirectional
 *        since unidirectional graphs need to have paired wires.
 *
 *   @param cfactor                 Channel width factor: multiplier on the channel width distribution (usually the number of tracks in the widest channel).
 *   @param chan_dist               Channel width distribution.
 *   @param x                       The distance (between 0 and 1) we are across the chip.
 *   @param separation              The distance between two channels in the 0 to 1 coordinate system.
 *   @param graph_directionality    The directionality of the graph (unidirectional or bidirectional).
 */
static int compute_chan_width(int cfactor, t_chan chan_dist, float distance, float separation, e_graph_type graph_directionality) {
    int computed_width;
    computed_width = (int)floor(cfactor * comp_width(&chan_dist, distance, separation) + 0.5);
    if ((e_graph_type::BIDIR == graph_directionality) || computed_width % 2 == 0) {
        return computed_width;
    } else {
        return computed_width - 1;
    }
}

/**
 * @brief Return the relative channel density.
 *
 *   @param chan        points to a channel functional description data structure
 *   @param x           the distance (between 0 and 1) we are across the chip.
 *   @param separation  the distance between two channels, in the 0 to 1 coordinate system
 */
static float comp_width(t_chan* chan, float x, float separation) {
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

    return val;
}

/**
 * @brief After placement, logical pins for blocks, and nets must be updated to correspond with physical pins of type.
 *
 * This is required by blocks with capacity > 1 (e.g. typically IOs with multiple instances in each placement
 * grid location). Since they may be swapped around during placement, we need to update which pins the various
 * nets use.
 *
 * This updates both the external inter-block net connectivity (i.e. the clustered netlist), and the intra-block
 * connectivity (since the internal pins used also change).
 *
 * This function should only be called once
 */
void post_place_sync() {
    // Go through each block
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& blk_loc_registry = g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();

    // Cluster-based netlist is used for placement
    for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
        blk_loc_registry.place_sync_external_block_connections(block_id);
    }
}
