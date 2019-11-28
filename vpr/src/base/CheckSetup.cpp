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
                const t_det_routing_arch& RoutingArch,
                const std::vector<t_segment_inf>& Segments,
                const t_timing_inf Timing,
                const t_chan_width_dist Chans) {
    int i;
    int Tmp;

    if (!Timing.timing_analysis_enabled && PackerOpts.timing_driven) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Packing cannot be timing driven without timing analysis enabled\n");
    }

    if ((GLOBAL == RouterOpts.route_type)
        && (BOUNDING_BOX_PLACE != PlacerOpts.place_algorithm)) {
        /* Works, but very weird.  Can't optimize timing well, since you're
         * not doing proper architecture delay modelling. */
        VTR_LOG_WARN(
            "Using global routing with timing-driven placement. "
            "This is allowed, but strange, and circuit speed will suffer.\n");
    }

    if ((false == Timing.timing_analysis_enabled)
        && (PlacerOpts.place_algorithm == PATH_TIMING_DRIVEN_PLACE)) {
        /* May work, not tested */
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "Timing analysis must be enabled for timing-driven placement.\n");
    }

    if (!PlacerOpts.doPlacement && (USER == PlacerOpts.pad_loc_type)) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "A pad location file requires that placement is enabled.\n");
    }

    if (RouterOpts.doRouting) {
        if (!Timing.timing_analysis_enabled
            && (DEMAND_ONLY != RouterOpts.base_cost_type && DEMAND_ONLY_NORMALIZED_LENGTH != RouterOpts.base_cost_type)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "base_cost_type must be demand_only or demand_only_normailzed_length when timing analysis is disabled.\n");
        }
    }

    if (DETAILED == RouterOpts.route_type) {
        if ((Chans.chan_x_dist.type != UNIFORM)
            || (Chans.chan_y_dist.type != UNIFORM)
            || (Chans.chan_x_dist.peak != Chans.chan_y_dist.peak)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Detailed routing currently only supported on FPGAs with all channels of equal width.\n");
        }
    }

    for (i = 0; i < (int)Segments.size(); ++i) {
        Tmp = Segments[i].arch_opin_switch;
        auto& device_ctx = g_vpr_ctx.device();
        if (false == device_ctx.arch_switch_inf[Tmp].buffered()) {
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
}
