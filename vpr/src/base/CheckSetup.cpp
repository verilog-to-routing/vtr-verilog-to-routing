#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "echo_files.h"
#include "read_xml_arch_file.h"
#include "CheckSetup.h"

void CheckSetup(const t_packer_opts& PackerOpts,
                const t_placer_opts& PlacerOpts,
                const t_router_opts& RouterOpts,
                const t_server_opts& ServerOpts,
                const t_det_routing_arch& RoutingArch,
                const std::vector<t_segment_inf>& Segments,
                const t_timing_inf& Timing,
                const t_chan_width_dist Chans) {
    int i;
    int Tmp;

    if (!Timing.timing_analysis_enabled && PackerOpts.timing_driven) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Packing cannot be timing driven without timing analysis enabled\n");
    }

    if (PackerOpts.load_flat_placement) {
        if (PackerOpts.device_layout == "auto") {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Legalization requires a fixed device layout.\n");
        }
        if (!PlacerOpts.constraints_file.empty()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Cannot specify a fixed clusters file when running legalization.\n");
        }
    }


    if ((GLOBAL == RouterOpts.route_type)
        && (PlacerOpts.place_algorithm.is_timing_driven())) {
        /* Works, but very weird.  Can't optimize timing well, since you're
         * not doing proper architecture delay modelling. */
        VTR_LOG_WARN(
            "Using global routing with timing-driven placement. "
            "This is allowed, but strange, and circuit speed will suffer.\n");
    }

    if (!Timing.timing_analysis_enabled
        && (PlacerOpts.place_algorithm.is_timing_driven())) {
        /* May work, not tested */
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Timing analysis must be enabled for timing-driven placement.\n");
    }

    if (!PlacerOpts.doPlacement && (!PlacerOpts.constraints_file.empty())) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "A block location file requires that placement is enabled.\n");
    }

    if (PlacerOpts.place_algorithm.is_timing_driven() &&
        PlacerOpts.place_static_move_prob.size() > NUM_PL_MOVE_TYPES) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The number of provided placer move probabilities (%d) should equal or less than the total number of supported moves (%d).\n",
                        PlacerOpts.place_static_move_prob.size(),
                        NUM_PL_MOVE_TYPES);
    }

    if (!PlacerOpts.place_algorithm.is_timing_driven() &&
        PlacerOpts.place_static_move_prob.size() > NUM_PL_NONTIMING_MOVE_TYPES) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The number of placer non timing move probabilities (%d) should equal to or less than the total number of supported moves (%d).\n",
                        PlacerOpts.place_static_move_prob.size(),
                        NUM_PL_MOVE_TYPES);
    }

    // Rules for doing analytical placement.
    if (PlacerOpts.doAnalyticalPlacement) {
        // Make sure that the --place option was not set.
        if (PlacerOpts.doPlacement) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Cannot perform both analytical and non-analytical placement.\n");
        }
        // Make sure that the --pack option was not set.
        if (PackerOpts.doPacking) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Analytical placement should skip packing.\n");
        }

        // TODO: Should check that read_vpr_constraint_file is non-empty or
        //       check within analytical placement that the floorplanning has
        //       some fixed blocks somewhere. Maybe we can live without fixed
        //       blocks.

        // FIXME: Should we enforce that the size of the device is fixed? Or is
        //        that defined in the constraints file?
    }

    if (RouterOpts.doRouting) {
        if (!Timing.timing_analysis_enabled
            && (DEMAND_ONLY != RouterOpts.base_cost_type && DEMAND_ONLY_NORMALIZED_LENGTH != RouterOpts.base_cost_type)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "base_cost_type must be demand_only or demand_only_normalized_length when timing analysis is disabled.\n");
        }
    }

    if (DETAILED == RouterOpts.route_type) {
        if ((Chans.chan_x_dist.type != UNIFORM)
            || (Chans.chan_y_dist.type != UNIFORM)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Detailed routing currently only supported on FPGAs with uniform channel distributions.\n");
        }
    }

    for (i = 0; i < (int)Segments.size(); ++i) {
        Tmp = Segments[i].arch_opin_switch;
        auto& device_ctx = g_vpr_ctx.device();
        if (!device_ctx.arch_switch_inf[Tmp].buffered()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "arch_opin_switch (#%d) of segment type #%d is not buffered.\n", Tmp, i);
        }
    }

    if ((PlacerOpts.place_chan_width != NO_FIXED_CHANNEL_WIDTH) && PlacerOpts.place_chan_width < 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Place channel width must be positive.\n");
    }
    if ((RouterOpts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) && RouterOpts.fixed_channel_width < 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Routing channel width must be positive.\n");
    }

    if (UNI_DIRECTIONAL == RoutingArch.directionality) {
        if ((RouterOpts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH)
            && (RouterOpts.fixed_channel_width % 2 > 0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Routing channel width must be even for unidirectional.\n");
        }
        if ((PlacerOpts.place_chan_width != NO_FIXED_CHANNEL_WIDTH)
            && (PlacerOpts.place_chan_width % 2 > 0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Place channel width must be even for unidirectional.\n");
        }
    }

    if (ServerOpts.is_server_mode_enabled) {
        if (ServerOpts.port_num < DYMANIC_PORT_RANGE_MIN || ServerOpts.port_num > DYNAMIC_PORT_RANGE_MAX) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Specified server port number `--port %d` is out of range [%d-%d]. Please specify a port number within that range.\n",
                                ServerOpts.port_num, DYMANIC_PORT_RANGE_MIN, DYNAMIC_PORT_RANGE_MAX);
        }
    }
}
