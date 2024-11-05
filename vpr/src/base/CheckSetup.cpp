
#include "CheckSetup.h"

#include "vtr_log.h"
#include "vpr_types.h"
#include "vpr_error.h"
#include "globals.h"
#include "read_xml_arch_file.h"


static constexpr int DYMANIC_PORT_RANGE_MIN = 49152;
static constexpr int DYNAMIC_PORT_RANGE_MAX = 65535;

void CheckSetup(const t_packer_opts& packer_opts,
                const t_placer_opts& placer_opts,
                const t_ap_opts& ap_opts,
                const t_router_opts& router_opts,
                const t_server_opts& server_opts,
                const t_det_routing_arch& routing_arch,
                const std::vector<t_segment_inf>& segments,
                const t_timing_inf& timing,
                const t_chan_width_dist& chans) {
    if (!timing.timing_analysis_enabled && packer_opts.timing_driven) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Packing cannot be timing driven without timing analysis enabled\n");
    }

    if (packer_opts.load_flat_placement) {
        if (packer_opts.device_layout == "auto") {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Legalization requires a fixed device layout.\n");
        }
        if (!placer_opts.constraints_file.empty()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Cannot specify a fixed clusters file when running legalization.\n");
        }
    }


    if ((GLOBAL == router_opts.route_type)
        && (placer_opts.place_algorithm.is_timing_driven())) {
        /* Works, but very weird.  Can't optimize timing well, since you're
         * not doing proper architecture delay modelling. */
        VTR_LOG_WARN(
            "Using global routing with timing-driven placement. "
            "This is allowed, but strange, and circuit speed will suffer.\n");
    }

    if (!timing.timing_analysis_enabled
        && (placer_opts.place_algorithm.is_timing_driven())) {
        /* May work, not tested */
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Timing analysis must be enabled for timing-driven placement.\n");
    }

    if (!placer_opts.doPlacement && (!placer_opts.constraints_file.empty())) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "A block location file requires that placement is enabled.\n");
    }

    if (placer_opts.place_algorithm.is_timing_driven() &&
        placer_opts.place_static_move_prob.size() > NUM_PL_MOVE_TYPES) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The number of provided placer move probabilities (%d) should equal or less than the total number of supported moves (%d).\n",
                        placer_opts.place_static_move_prob.size(),
                        NUM_PL_MOVE_TYPES);
    }

    if (!placer_opts.place_algorithm.is_timing_driven() &&
        placer_opts.place_static_move_prob.size() > NUM_PL_NONTIMING_MOVE_TYPES) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "The number of placer non timing move probabilities (%d) should equal to or less than the total number of supported moves (%d).\n",
                        placer_opts.place_static_move_prob.size(),
                        NUM_PL_MOVE_TYPES);
    }

    // Rules for doing Analytical Placement
    if (ap_opts.doAP) {
        // Make sure that the --place option was not set.
        if (placer_opts.doPlacement) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Cannot perform both analytical and non-analytical placement.\n");
        }
        // Make sure that the --pack option was not set.
        if (packer_opts.doPacking) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Analytical placement should skip packing.\n");
        }

        // TODO: Should check that read_vpr_constraint_file is non-empty or
        //       check within analytical placement that the floorplanning has
        //       some fixed blocks somewhere. Maybe we can live without fixed
        //       blocks.

        // TODO: Should we enforce that the size of the device is fixed. This
        //       goes with ensuring that some blocks are fixed.
    }

    if (router_opts.doRouting) {
        if (!timing.timing_analysis_enabled
            && (DEMAND_ONLY != router_opts.base_cost_type && DEMAND_ONLY_NORMALIZED_LENGTH != router_opts.base_cost_type)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "base_cost_type must be demand_only or demand_only_normalized_length when timing analysis is disabled.\n");
        }
    }

    if (DETAILED == router_opts.route_type) {
        if ((chans.chan_x_dist.type != UNIFORM)
            || (chans.chan_y_dist.type != UNIFORM)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Detailed routing currently only supported on FPGAs with uniform channel distributions.\n");
        }
    }

    for (int i = 0; i < (int)segments.size(); ++i) {
        int opin_switch_idx = segments[i].arch_opin_switch;
        auto& device_ctx = g_vpr_ctx.device();
        if (!device_ctx.arch_switch_inf[opin_switch_idx].buffered()) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "arch_opin_switch (#%d) of segment type #%d is not buffered.\n", opin_switch_idx, i);
        }
    }

    if ((placer_opts.place_chan_width != NO_FIXED_CHANNEL_WIDTH) && placer_opts.place_chan_width < 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Place channel width must be positive.\n");
    }
    if ((router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH) && router_opts.fixed_channel_width < 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Routing channel width must be positive.\n");
    }

    if (UNI_DIRECTIONAL == routing_arch.directionality) {
        if ((router_opts.fixed_channel_width != NO_FIXED_CHANNEL_WIDTH)
            && (router_opts.fixed_channel_width % 2 > 0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Routing channel width must be even for unidirectional.\n");
        }
        if ((placer_opts.place_chan_width != NO_FIXED_CHANNEL_WIDTH)
            && (placer_opts.place_chan_width % 2 > 0)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Place channel width must be even for unidirectional.\n");
        }
    }

    if (server_opts.is_server_mode_enabled) {
        if (server_opts.port_num < DYMANIC_PORT_RANGE_MIN || server_opts.port_num > DYNAMIC_PORT_RANGE_MAX) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Specified server port number `--port %d` is out of range [%d-%d]. Please specify a port number within that range.\n",
                                server_opts.port_num, DYMANIC_PORT_RANGE_MIN, DYNAMIC_PORT_RANGE_MAX);
        }
    }
}
