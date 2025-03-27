
#include "ShowSetup.h"

#include "ap_flow_enums.h"
#include "globals.h"
#include "physical_types_util.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include <fstream>
#include <tuple>

/******** Function Prototypes ********/
static void ShowPackerOpts(const t_packer_opts& PackerOpts);
static void ShowNetlistOpts(const t_netlist_opts& NetlistOpts);
static void ShowPlacerOpts(const t_placer_opts& PlacerOpts);
static void ShowAnalyticalPlacerOpts(const t_ap_opts& APOpts);
static void ShowRouterOpts(const t_router_opts& RouterOpts);
static void ShowAnalysisOpts(const t_analysis_opts& AnalysisOpts);
static void ShowNocOpts(const t_noc_opts& NocOpts);
static void ShowAnnealSched(const t_annealing_sched& AnnealSched);

/******** Function Implementations ********/

void ShowSetup(const t_vpr_setup& vpr_setup) {
    VTR_LOG("Timing analysis: %s\n", (vpr_setup.TimingEnabled ? "ON" : "OFF"));

    VTR_LOG("Circuit netlist file: %s\n", vpr_setup.FileNameOpts.NetFile.c_str());
    VTR_LOG("Circuit placement file: %s\n", vpr_setup.FileNameOpts.PlaceFile.c_str());
    VTR_LOG("Circuit routing file: %s\n", vpr_setup.FileNameOpts.RouteFile.c_str());
    VTR_LOG("Circuit SDC file: %s\n", vpr_setup.Timing.SDCFile.c_str());
    if (vpr_setup.FileNameOpts.read_vpr_constraints_file.empty()) {
        VTR_LOG("Vpr floorplanning constraints file: not specified\n");
    } else {
        VTR_LOG("Vpr floorplanning constraints file: %s\n", vpr_setup.FileNameOpts.read_vpr_constraints_file.c_str());
    }
    VTR_LOG("\n");

    VTR_LOG("Packer: %s\n", (vpr_setup.PackerOpts.doPacking ? "ENABLED" : "DISABLED"));
    VTR_LOG("Placer: %s\n", (vpr_setup.PlacerOpts.doPlacement ? "ENABLED" : "DISABLED"));
    VTR_LOG("Analytical Placer: %s\n", (vpr_setup.APOpts.doAP ? "ENABLED" : "DISABLED"));
    VTR_LOG("Router: %s\n", (vpr_setup.RouterOpts.doRouting ? "ENABLED" : "DISABLED"));
    VTR_LOG("Analysis: %s\n", (vpr_setup.AnalysisOpts.doAnalysis ? "ENABLED" : "DISABLED"));
    VTR_LOG("\n");

    VTR_LOG("VPR was run with the following options:\n\n");

    ShowNetlistOpts(vpr_setup.NetlistOpts);

    if (vpr_setup.PackerOpts.doPacking) {
        ShowPackerOpts(vpr_setup.PackerOpts);
    }
    if (vpr_setup.PlacerOpts.doPlacement) {
        ShowPlacerOpts(vpr_setup.PlacerOpts);
    }
    if (vpr_setup.APOpts.doAP) {
        ShowAnalyticalPlacerOpts(vpr_setup.APOpts);
    }
    if (vpr_setup.RouterOpts.doRouting) {
        ShowRouterOpts(vpr_setup.RouterOpts);
    }
    if (vpr_setup.AnalysisOpts.doAnalysis) {
        ShowAnalysisOpts(vpr_setup.AnalysisOpts);
    }
    if (vpr_setup.NocOpts.noc) {
        ShowNocOpts(vpr_setup.NocOpts);
    }
}

void ClusteredNetlistStats::writeHuman(std::ostream& output) const {
    output << "Cluster level netlist and block usage statistics\n";
    output << "Netlist num_nets: " << num_nets << "\n";
    output << "Netlist num_blocks: " << num_blocks << "\n";
    for (const auto& type : logical_block_types) {
        output << "Netlist " << type.name << " blocks: " << num_blocks_type[type.index] << ".\n";
    }

    output << "Netlist inputs pins: " << L_num_p_inputs << "\n";
    output << "Netlist output pins: " << L_num_p_outputs << "\n";
}
void ClusteredNetlistStats::writeJSON(std::ostream& output) const {
    output << "{\n";

    output << "  \"num_nets\": \"" << num_nets << "\",\n";
    output << "  \"num_blocks\": \"" << num_blocks << "\",\n";

    output << "  \"input_pins\": \"" << L_num_p_inputs << "\",\n";
    output << "  \"output_pins\": \"" << L_num_p_outputs << "\",\n";

    output << "  \"blocks\": {\n";

    for (const auto& type : logical_block_types) {
        output << "    \"" << type.name << "\": " << num_blocks_type[type.index];
        if ((int)type.index < (int)logical_block_types.size() - 1)
            output << ",\n";
        else
            output << "\n";
    }
    output << "  }\n";
    output << "}\n";
}

void ClusteredNetlistStats::writeXML(std::ostream& output) const {
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<block_usage_report>\n";

    output << "  <nets num=\"" << num_nets << "\"></nets>\n";
    output << "  <blocks num=\"" << num_blocks << "\">\n";

    for (const auto& type : logical_block_types) {
        output << "    <block type=\"" << type.name << "\" usage=\"" << num_blocks_type[type.index] << "\"></block>\n";
    }
    output << "  </blocks>\n";

    output << "  <input_pins num=\"" << L_num_p_inputs << "\"></input_pins>\n";
    output << "  <output_pins num=\"" << L_num_p_outputs << "\"></output_pins>\n";

    output << "</block_usage_report>\n";
}

ClusteredNetlistStats::ClusteredNetlistStats() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    L_num_p_inputs = 0;
    L_num_p_outputs = 0;
    num_blocks_type = std::vector<int>(device_ctx.logical_block_types.size(), 0);
    num_nets = (int)cluster_ctx.clb_nlist.nets().size();
    num_blocks = (int)cluster_ctx.clb_nlist.blocks().size();
    logical_block_types = device_ctx.logical_block_types;

    /* Count I/O input and output pads */
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);
        auto physical_tile = pick_physical_type(logical_block);
        num_blocks_type[logical_block->index]++;
        if (is_io_type(physical_tile)) {
            for (int j = 0; j < logical_block->pb_type->num_pins; j++) {
                int physical_pin = get_physical_pin(physical_tile, logical_block, j);

                if (cluster_ctx.clb_nlist.block_net(blk_id, j) != ClusterNetId::INVALID()) {
                    auto pin_type = get_pin_type_from_pin_physical_num(physical_tile, physical_pin);
                    if (pin_type == DRIVER) {
                        L_num_p_inputs++;
                    } else {
                        VTR_ASSERT(pin_type == RECEIVER);
                        L_num_p_outputs++;
                    }
                }
            }
        }
    }
}

void ClusteredNetlistStats::write(OutputFormat fmt, std::ostream& output) const {
    switch (fmt) {
        case HumanReadable:
            writeHuman(output);
            break;
        case JSON:
            writeJSON(output);
            break;
        case XML:
            writeXML(output);
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Unknown extension on in block usage summary file");
            break;
    }
}

void writeClusteredNetlistStats(const std::string& block_usage_filename) {
    const auto stats = ClusteredNetlistStats();

    // Print out the human-readable version to stdout

    stats.write(ClusteredNetlistStats::OutputFormat::HumanReadable, std::cout);

    if (!block_usage_filename.empty()) {
        ClusteredNetlistStats::OutputFormat fmt;

        if (vtr::check_file_name_extension(block_usage_filename, ".json")) {
            fmt = ClusteredNetlistStats::OutputFormat::JSON;
        } else if (vtr::check_file_name_extension(block_usage_filename, ".xml")) {
            fmt = ClusteredNetlistStats::OutputFormat::XML;
        } else if (vtr::check_file_name_extension(block_usage_filename, ".txt")) {
            fmt = ClusteredNetlistStats::OutputFormat::HumanReadable;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unknown extension on output %s", block_usage_filename.c_str());
        }

        std::fstream fp;

        fp.open(block_usage_filename, std::fstream::out | std::fstream::trunc);
        stats.write(fmt, fp);
        fp.close();
    }
}

static void ShowAnnealSched(const t_annealing_sched& AnnealSched) {
    VTR_LOG("AnnealSched.type: ");
    switch (AnnealSched.type) {
        case e_sched_type::AUTO_SCHED:
            VTR_LOG("AUTO_SCHED\n");
            break;
        case e_sched_type::USER_SCHED:
            VTR_LOG("USER_SCHED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown annealing schedule\n");
    }

    VTR_LOG("AnnealSched.inner_num: %f\n", AnnealSched.inner_num);

    if (e_sched_type::USER_SCHED == AnnealSched.type) {
        VTR_LOG("AnnealSched.init_t: %f\n", AnnealSched.init_t);
        VTR_LOG("AnnealSched.alpha_t: %f\n", AnnealSched.alpha_t);
        VTR_LOG("AnnealSched.exit_t: %f\n", AnnealSched.exit_t);
    }
}

static void ShowRouterOpts(const t_router_opts& RouterOpts) {
    VTR_LOG("RouterOpts.route_type: ");
    switch (RouterOpts.route_type) {
        case GLOBAL:
            VTR_LOG("GLOBAL\n");
            break;
        case DETAILED:
            VTR_LOG("DETAILED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown router opt\n");
    }

    VTR_LOG("RouterOpts.flat_routing: ");
    if (RouterOpts.flat_routing) {
        VTR_LOG("true\n");
    } else {
        VTR_LOG("false\n");
    }

    VTR_LOG("RouterOpts.choke_points: ");
    if (RouterOpts.has_choke_point) {
        VTR_LOG("on\n");
    } else {
        VTR_LOG("off\n");
    }

    VTR_ASSERT(GLOBAL == RouterOpts.route_type || DETAILED == RouterOpts.route_type);

    VTR_LOG("RouterOpts.router_algorithm: ");
    switch (RouterOpts.router_algorithm) {
        case PARALLEL:
            VTR_LOG("PARALLEL\n");
            break;
        case PARALLEL_DECOMP:
            VTR_LOG("PARALLEL_DECOMP\n");
            break;
        case TIMING_DRIVEN:
            VTR_LOG("TIMING_DRIVEN\n");
            break;
        default:
            switch (RouterOpts.route_type) {
                case DETAILED:
                    VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "<Unknown>\n");
                case GLOBAL:
                    VTR_LOG_ERROR("Unknown router algorithm\n");
                    break;
                default:
                    break;
            }
    }

    VTR_LOG("RouterOpts.base_cost_type: ");
    switch (RouterOpts.base_cost_type) {
        case DELAY_NORMALIZED:
            VTR_LOG("DELAY_NORMALIZED\n");
            break;
        case DELAY_NORMALIZED_LENGTH:
            if (GLOBAL == RouterOpts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH\n");
            break;
        case DELAY_NORMALIZED_LENGTH_BOUNDED:
            if (GLOBAL == RouterOpts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH_BOUNDED\n");
            break;
        case DELAY_NORMALIZED_FREQUENCY:
            if (GLOBAL == RouterOpts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_FREQUENCY\n");
            break;
        case DELAY_NORMALIZED_LENGTH_FREQUENCY:
            if (GLOBAL == RouterOpts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH_FREQUENCY\n");
            break;
        case DEMAND_ONLY:
            VTR_LOG("DEMAND_ONLY\n");
            break;
        case DEMAND_ONLY_NORMALIZED_LENGTH:
            if (GLOBAL == RouterOpts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DEMAND_ONLY_NORMALIZED_LENGTH\n");
            break;
        default:
            switch (RouterOpts.route_type) {
                case DETAILED:
                    VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown base_cost_type\n");
                case GLOBAL:
                    VTR_LOG_ERROR("Unknown router base cost type\n");
                    break;
                default:
                    break;
            }
    }

    VTR_LOG("RouterOpts.fixed_channel_width: ");
    if (NO_FIXED_CHANNEL_WIDTH == RouterOpts.fixed_channel_width) {
        VTR_LOG("NO_FIXED_CHANNEL_WIDTH\n");
    } else {
        VTR_LOG("%d\n", RouterOpts.fixed_channel_width);
    }

    if (DETAILED == RouterOpts.route_type) {
        VTR_LOG("RouterOpts.check_route: ");
        switch (RouterOpts.check_route) {
            case e_check_route_option::OFF:
                VTR_LOG("OFF\n");
                break;
            case e_check_route_option::QUICK:
                VTR_LOG("QUICK\n");
                break;
            case e_check_route_option::FULL:
                VTR_LOG("FULL\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown check_route value\n");
        }
    }

    VTR_LOG("RouterOpts.acc_fac: %f\n", RouterOpts.acc_fac);
    VTR_LOG("RouterOpts.bb_factor: %d\n", RouterOpts.bb_factor);
    VTR_LOG("RouterOpts.bend_cost: %f\n", RouterOpts.bend_cost);
    VTR_LOG("RouterOpts.first_iter_pres_fac: %f\n", RouterOpts.first_iter_pres_fac);
    VTR_LOG("RouterOpts.initial_pres_fac: %f\n", RouterOpts.initial_pres_fac);
    VTR_LOG("RouterOpts.pres_fac_mult: %f\n", RouterOpts.pres_fac_mult);
    VTR_LOG("RouterOpts.max_pres_fac: %f\n", RouterOpts.max_pres_fac);
    VTR_LOG("RouterOpts.max_router_iterations: %d\n", RouterOpts.max_router_iterations);
    VTR_LOG("RouterOpts.min_incremental_reroute_fanout: %d\n", RouterOpts.min_incremental_reroute_fanout);
    VTR_LOG("RouterOpts.do_check_rr_graph: %s\n", RouterOpts.do_check_rr_graph ? "true" : "false");
    VTR_LOG("RouterOpts.verify_binary_search: %s\n", RouterOpts.verify_binary_search ? "true" : "false");
    VTR_LOG("RouterOpts.min_channel_width_hint: %d\n", RouterOpts.min_channel_width_hint);
    VTR_LOG("RouterOpts.read_rr_edge_metadata: %s\n", RouterOpts.read_rr_edge_metadata ? "true" : "false");
    VTR_LOG("RouterOpts.exit_after_first_routing_iteration: %s\n", RouterOpts.exit_after_first_routing_iteration ? "true" : "false");

    if (TIMING_DRIVEN == RouterOpts.router_algorithm) {
        VTR_LOG("RouterOpts.astar_fac: %f\n", RouterOpts.astar_fac);
        VTR_LOG("RouterOpts.astar_offset: %f\n", RouterOpts.astar_offset);
        VTR_LOG("RouterOpts.router_profiler_astar_fac: %f\n", RouterOpts.router_profiler_astar_fac);
        VTR_LOG("RouterOpts.criticality_exp: %f\n", RouterOpts.criticality_exp);
        VTR_LOG("RouterOpts.max_criticality: %f\n", RouterOpts.max_criticality);
        VTR_LOG("RouterOpts.init_wirelength_abort_threshold: %f\n", RouterOpts.init_wirelength_abort_threshold);

        if (GLOBAL == RouterOpts.route_type)
            VTR_LOG("RouterOpts.incr_reroute_delay_ripup: %f\n", RouterOpts.incr_reroute_delay_ripup);
        else {
            std::string incr_delay_ripup_opts[3] = {"ON", "OFF", "AUTO"};
            VTR_LOG("RouterOpts.incr_reroute_delay_ripup: %s\n", incr_delay_ripup_opts[(size_t)RouterOpts.incr_reroute_delay_ripup].c_str());
        }

        VTR_LOG("RouterOpts.save_routing_per_iteration: %s\n", RouterOpts.save_routing_per_iteration ? "true" : "false");
        VTR_LOG("RouterOpts.congested_routing_iteration_threshold_frac: %f\n", RouterOpts.congested_routing_iteration_threshold_frac);
        VTR_LOG("RouterOpts.high_fanout_threshold: %d\n", RouterOpts.high_fanout_threshold);
        VTR_LOG("RouterOpts.router_debug_net: %d\n", RouterOpts.router_debug_net);
        VTR_LOG("RouterOpts.router_debug_sink_rr: %d\n", RouterOpts.router_debug_sink_rr);
        VTR_LOG("RouterOpts.router_debug_iteration: %d\n", RouterOpts.router_debug_iteration);
        VTR_LOG("RouterOpts.max_convergence_count: %d\n", RouterOpts.max_convergence_count);
        VTR_LOG("RouterOpts.reconvergence_cpd_threshold: %f\n", RouterOpts.reconvergence_cpd_threshold);
        VTR_LOG("RouterOpts.update_lower_bound_delays: %s\n", RouterOpts.update_lower_bound_delays ? "true" : "false");
        VTR_LOG("RouterOpts.first_iteration_timing_report_file: %s\n", RouterOpts.first_iteration_timing_report_file.c_str());

        VTR_LOG("RouterOpts.route_bb_update: ");
        switch (RouterOpts.route_bb_update) {
            case e_route_bb_update::STATIC:
                VTR_LOG("STATIC\n");
                break;
            case e_route_bb_update::DYNAMIC:
                VTR_LOG("DYNAMIC\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown route_bb_update\n");
        }

        VTR_LOG("RouterOpts.lookahead_type: ");
        switch (RouterOpts.lookahead_type) {
            case e_router_lookahead::CLASSIC:
                VTR_LOG("CLASSIC\n");
                break;
            case e_router_lookahead::MAP:
                VTR_LOG("MAP\n");
                break;
            case e_router_lookahead::COMPRESSED_MAP:
                VTR_LOG("COMPRESSED_MAP\n");
                break;
            case e_router_lookahead::EXTENDED_MAP:
                VTR_LOG("EXTENDED_MAP\n");
                break;
            case e_router_lookahead::NO_OP:
                VTR_LOG("NO_OP\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown lookahead_type\n");
        }

        VTR_LOG("RouterOpts.initial_timing: ");
        switch (RouterOpts.initial_timing) {
            case e_router_initial_timing::ALL_CRITICAL:
                VTR_LOG("ALL_CRITICAL\n");
                break;
            case e_router_initial_timing::LOOKAHEAD:
                VTR_LOG("LOOKAHEAD\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown initial_timing\n");
        }

        VTR_LOG("RouterOpts.router_heap: ");
        switch (RouterOpts.router_heap) {
            case e_heap_type::INVALID_HEAP:
                VTR_LOG("INVALID_HEAP\n");
                break;
            case e_heap_type::BINARY_HEAP:
                VTR_LOG("BINARY_HEAP\n");
                break;
            case e_heap_type::FOUR_ARY_HEAP:
                VTR_LOG("FOUR_ARY_HEAP\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown router_heap\n");
        }
    }

    if (DETAILED == RouterOpts.route_type) {
        if (RouterOpts.routing_failure_predictor == SAFE)
            VTR_LOG("RouterOpts.routing_failure_predictor = SAFE\n");
        else if (RouterOpts.routing_failure_predictor == AGGRESSIVE)
            VTR_LOG("RouterOpts.routing_failure_predictor = AGGRESSIVE\n");
        else if (RouterOpts.routing_failure_predictor == OFF)
            VTR_LOG("RouterOpts.routing_failure_predictor = OFF\n");

        if (RouterOpts.routing_budgets_algorithm == DISABLE) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = DISABLE\n");
        } else if (RouterOpts.routing_budgets_algorithm == MINIMAX) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = MINIMAX\n");
        } else if (RouterOpts.routing_budgets_algorithm == YOYO) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = YOYO\n");
        } else if (RouterOpts.routing_budgets_algorithm == SCALE_DELAY) {
            VTR_LOG("RouterOpts.routing_budgets_algorithm = SCALE_DELAY\n");
        }
    }

    VTR_LOG("\n");
}

static void ShowPlacerOpts(const t_placer_opts& PlacerOpts) {
    VTR_LOG("PlacerOpts.place_freq: ");
    switch (PlacerOpts.place_freq) {
        case PLACE_ONCE:
            VTR_LOG("PLACE_ONCE\n");
            break;
        case PLACE_ALWAYS:
            VTR_LOG("PLACE_ALWAYS\n");
            break;
        case PLACE_NEVER:
            VTR_LOG("PLACE_NEVER\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown Place Freq\n");
    }
    if ((PLACE_ONCE == PlacerOpts.place_freq)
        || (PLACE_ALWAYS == PlacerOpts.place_freq)) {
        VTR_LOG("PlacerOpts.place_algorithm: ");
        switch (PlacerOpts.place_algorithm.get()) {
            case e_place_algorithm::BOUNDING_BOX_PLACE:
                VTR_LOG("BOUNDING_BOX_PLACE\n");
                break;
            case e_place_algorithm::CRITICALITY_TIMING_PLACE:
                VTR_LOG("CRITICALITY_TIMING_PLACE\n");
                break;
            case e_place_algorithm::SLACK_TIMING_PLACE:
                VTR_LOG("SLACK_TIMING_PLACE\n");
                break;
            default:
                VTR_LOG_ERROR("Unknown placement algorithm\n");
        }

        VTR_LOG("PlacerOpts.pad_loc_type: ");
        switch (PlacerOpts.pad_loc_type) {
            case e_pad_loc_type::FREE:
                VTR_LOG("FREE\n");
                break;
            case e_pad_loc_type::RANDOM:
                VTR_LOG("RANDOM\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown I/O pad location type\n");
        }

        VTR_LOG("PlacerOpts.constraints_file: ");
        if (PlacerOpts.constraints_file.empty()) {
            VTR_LOG("No constraints file given\n");
        } else {
            VTR_LOG("Using constraints file '%s'\n", PlacerOpts.constraints_file.c_str());
        }

        VTR_LOG("PlacerOpts.place_chan_width: %d\n", PlacerOpts.place_chan_width);

        if (PlacerOpts.place_algorithm.is_timing_driven()) {
            VTR_LOG("PlacerOpts.inner_loop_recompute_divider: %d\n", PlacerOpts.inner_loop_recompute_divider);
            VTR_LOG("PlacerOpts.recompute_crit_iter: %d\n", PlacerOpts.recompute_crit_iter);
            VTR_LOG("PlacerOpts.timing_tradeoff: %f\n", PlacerOpts.timing_tradeoff);
            VTR_LOG("PlacerOpts.td_place_exp_first: %f\n", PlacerOpts.td_place_exp_first);
            VTR_LOG("PlacerOpts.td_place_exp_last: %f\n", PlacerOpts.td_place_exp_last);
            VTR_LOG("PlacerOpts.delay_offset: %f\n", PlacerOpts.delay_offset);
            VTR_LOG("PlacerOpts.delay_ramp_delta_threshold: %d\n", PlacerOpts.delay_ramp_delta_threshold);
            VTR_LOG("PlacerOpts.delay_ramp_slope: %f\n", PlacerOpts.delay_ramp_slope);
            VTR_LOG("PlacerOpts.tsu_rel_margin: %f\n", PlacerOpts.tsu_rel_margin);
            VTR_LOG("PlacerOpts.tsu_abs_margin: %f\n", PlacerOpts.tsu_abs_margin);
            VTR_LOG("PlacerOpts.post_place_timing_report_file: %s\n", PlacerOpts.post_place_timing_report_file.c_str());
            VTR_LOG("PlacerOpts.allowed_tiles_for_delay_model: %s\n", PlacerOpts.allowed_tiles_for_delay_model.c_str());

            std::string e_reducer_strings[5] = {"MIN", "MAX", "MEDIAN", "ARITHMEAN", "GEOMEAN"};
            if ((size_t)PlacerOpts.delay_model_reducer > 4)
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_reducer\n");
            VTR_LOG("PlacerOpts.delay_model_reducer: %s\n", e_reducer_strings[(size_t)PlacerOpts.delay_model_reducer].c_str());

            std::string place_delay_model_strings[3] = {"SIMPLE", "DELTA", "DELTA_OVERRIDE"};
            if ((size_t)PlacerOpts.delay_model_type > 2)
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_type\n");
            VTR_LOG("PlacerOpts.delay_model_type: %s\n", place_delay_model_strings[(size_t)PlacerOpts.delay_model_type].c_str());
        }

        VTR_LOG("PlacerOpts.rlim_escape_fraction: %f\n", PlacerOpts.rlim_escape_fraction);
        VTR_LOG("PlacerOpts.move_stats_file: %s\n", PlacerOpts.move_stats_file.c_str());
        VTR_LOG("PlacerOpts.placement_saves_per_temperature: %d\n", PlacerOpts.placement_saves_per_temperature);

        VTR_LOG("PlacerOpts.effort_scaling: ");
        switch (PlacerOpts.effort_scaling) {
            case CIRCUIT:
                VTR_LOG("CIRCUIT\n");
                break;
            case DEVICE_CIRCUIT:
                VTR_LOG("DEVICE_CIRCUIT\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown effort_scaling\n");
        }

        VTR_LOG("PlacerOpts.place_delta_delay_matrix_calculation_method: ");
        switch (PlacerOpts.place_delta_delay_matrix_calculation_method) {
            case e_place_delta_delay_algorithm::ASTAR_ROUTE:
                VTR_LOG("ASTAR_ROUTE\n");
                break;
            case e_place_delta_delay_algorithm::DIJKSTRA_EXPANSION:
                VTR_LOG("DIJKSTRA_EXPANSION\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delta_delay_matrix_calculation_method\n");
        }

        VTR_LOG("PlaceOpts.seed: %d\n", PlacerOpts.seed);

        ShowAnnealSched(PlacerOpts.anneal_sched);
    }
    VTR_LOG("\n");
}

static void ShowAnalyticalPlacerOpts(const t_ap_opts& APOpts) {
    VTR_LOG("AnalyticalPlacerOpts.analytical_solver_type: ");
    switch (APOpts.analytical_solver_type) {
        case e_ap_analytical_solver::QP_Hybrid:
            VTR_LOG("qp-hybrid\n");
            break;
        case e_ap_analytical_solver::LP_B2B:
            VTR_LOG("lp-b2b\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown analytical_solver_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.partial_legalizer_type: ");
    switch (APOpts.partial_legalizer_type) {
        case e_ap_partial_legalizer::BiPartitioning:
            VTR_LOG("bipartitioning\n");
            break;
        case e_ap_partial_legalizer::FlowBased:
            VTR_LOG("flow-based\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown partial_legalizer_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.full_legalizer_type: ");
    switch (APOpts.full_legalizer_type) {
        case e_ap_full_legalizer::Naive:
            VTR_LOG("naive\n");
            break;
        case e_ap_full_legalizer::APPack:
            VTR_LOG("appack\n");
            break;
        case e_ap_full_legalizer::Basic_Min_Disturbance:
            VTR_LOG("basic-min-disturbance\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown full_legalizer_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.detailed_placer_type: ");
    switch (APOpts.detailed_placer_type) {
        case e_ap_detailed_placer::Identity:
            VTR_LOG("none\n");
            break;
        case e_ap_detailed_placer::Annealer:
            VTR_LOG("annealer\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown detailed_placer_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.log_verbosity: %d\n", APOpts.log_verbosity);
}

static void ShowNetlistOpts(const t_netlist_opts& NetlistOpts) {
    VTR_LOG("NetlistOpts.absorb_buffer_luts            : %s\n", (NetlistOpts.absorb_buffer_luts) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_primary_ios    : %s\n", (NetlistOpts.sweep_dangling_primary_ios) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_nets           : %s\n", (NetlistOpts.sweep_dangling_nets) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_dangling_blocks         : %s\n", (NetlistOpts.sweep_dangling_blocks) ? "true" : "false");
    VTR_LOG("NetlistOpts.sweep_constant_primary_outputs: %s\n", (NetlistOpts.sweep_constant_primary_outputs) ? "true" : "false");
    VTR_LOG("NetlistOpts.netlist_verbosity             : %d\n", NetlistOpts.netlist_verbosity);

    std::string const_gen_inference_strings[3] = {"NONE", "COMB", "COMB_SEQ"};
    if ((size_t)NetlistOpts.const_gen_inference > 3)
        VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_reducer\n");
    VTR_LOG("NetlistOpts.const_gen_inference           : %s\n", const_gen_inference_strings[(size_t)NetlistOpts.const_gen_inference].c_str());

    VTR_LOG("\n");
}

static void ShowAnalysisOpts(const t_analysis_opts& AnalysisOpts) {
    VTR_LOG("AnalysisOpts.gen_post_synthesis_netlist: %s\n", (AnalysisOpts.gen_post_synthesis_netlist) ? "true" : "false");
    VTR_LOG("AnalysisOpts.timing_report_npaths: %d\n", AnalysisOpts.timing_report_npaths);
    VTR_LOG("AnalysisOpts.timing_report_skew: %s\n", AnalysisOpts.timing_report_skew ? "true" : "false");
    VTR_LOG("AnalysisOpts.echo_dot_timing_graph_node: %s\n", AnalysisOpts.echo_dot_timing_graph_node.c_str());

    VTR_LOG("AnalysisOpts.timing_report_detail: ");
    switch (AnalysisOpts.timing_report_detail) {
        case e_timing_report_detail::NETLIST:
            VTR_LOG("NETLIST\n");
            break;
        case e_timing_report_detail::AGGREGATED:
            VTR_LOG("AGGREGATED\n");
            break;
        case e_timing_report_detail::DETAILED_ROUTING:
            VTR_LOG("DETAILED_ROUTING\n");
            break;
        case e_timing_report_detail::DEBUG:
            VTR_LOG("DEBUG\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown timing_report_detail\n");
    }

    const auto opts = {
        std::make_tuple(&AnalysisOpts.post_synth_netlist_unconn_input_handling, "post_synth_netlist_unconn_input_handling"),
        std::make_tuple(&AnalysisOpts.post_synth_netlist_unconn_output_handling, "post_synth_netlist_unconn_output_handling"),
    };
    for (const auto& opt : opts) {
        auto value = *std::get<0>(opt);
        VTR_LOG("AnalysisOpts.%s: ", std::get<1>(opt));
        switch (value) {
            case e_post_synth_netlist_unconn_handling::UNCONNECTED:
                VTR_LOG("UNCONNECTED\n");
                break;
            case e_post_synth_netlist_unconn_handling::NETS:
                VTR_LOG("NETS\n");
                break;
            case e_post_synth_netlist_unconn_handling::GND:
                VTR_LOG("GND\n");
                break;
            case e_post_synth_netlist_unconn_handling::VCC:
                VTR_LOG("VCC\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown post_synth_netlist_unconn_handling\n");
        }
    }
    VTR_LOG("\n");
}

static void ShowPackerOpts(const t_packer_opts& PackerOpts) {
    VTR_LOG("PackerOpts.allow_unrelated_clustering: ");
    if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::ON) {
        VTR_LOG("true\n");
    } else if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        VTR_LOG("false\n");
    } else if (PackerOpts.allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
        VTR_LOG("auto\n");
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown packer allow_unrelated_clustering\n");
    }
    VTR_LOG("PackerOpts.alpha_clustering: %f\n", PackerOpts.alpha);
    VTR_LOG("PackerOpts.beta_clustering: %f\n", PackerOpts.beta);
    VTR_LOG("PackerOpts.cluster_seed_type: ");
    switch (PackerOpts.cluster_seed_type) {
        case e_cluster_seed::TIMING:
            VTR_LOG("TIMING\n");
            break;
        case e_cluster_seed::MAX_INPUTS:
            VTR_LOG("MAX_INPUTS\n");
            break;
        case e_cluster_seed::BLEND:
            VTR_LOG("BLEND\n");
            break;
        case e_cluster_seed::MAX_PINS:
            VTR_LOG("MAX_PINS\n");
            break;
        case e_cluster_seed::MAX_INPUT_PINS:
            VTR_LOG("MAX_INPUT_PINS\n");
            break;
        case e_cluster_seed::BLEND2:
            VTR_LOG("BLEND2\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown packer cluster_seed_type\n");
    }
    VTR_LOG("PackerOpts.connection_driven: %s", (PackerOpts.connection_driven ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.global_clocks: %s", (PackerOpts.global_clocks ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.inter_cluster_net_delay: %f\n", PackerOpts.inter_cluster_net_delay);
    VTR_LOG("PackerOpts.timing_driven: %s", (PackerOpts.timing_driven ? "true\n" : "false\n"));
    VTR_LOG("PackerOpts.target_external_pin_util: %s", vtr::join(PackerOpts.target_external_pin_util, " ").c_str());
    VTR_LOG("\n");
    VTR_LOG("\n");
}

static void ShowNocOpts(const t_noc_opts& NocOpts) {
    VTR_LOG("NocOpts.noc_flows_file: %s\n", NocOpts.noc_flows_file.c_str());
    VTR_LOG("NocOpts.noc_routing_algorithm: %s\n", NocOpts.noc_routing_algorithm.c_str());
    VTR_LOG("NocOpts.noc_placement_weighting: %f\n", NocOpts.noc_placement_weighting);
    VTR_LOG("NocOpts.noc_aggregate_bandwidth_weighting: %f\n", NocOpts.noc_aggregate_bandwidth_weighting);
    VTR_LOG("NocOpts.noc_latency_constraints_weighting: %f\n", NocOpts.noc_latency_constraints_weighting);
    VTR_LOG("NocOpts.noc_latency_weighting: %f\n", NocOpts.noc_latency_weighting);
    VTR_LOG("NocOpts.noc_congestion_weighting: %f\n", NocOpts.noc_congestion_weighting);
    VTR_LOG("NocOpts.noc_swap_percentage: %d%%\n", NocOpts.noc_swap_percentage);
    VTR_LOG("NocOpts.noc_sat_routing_bandwidth_resolution: %d\n", NocOpts.noc_sat_routing_bandwidth_resolution);
    VTR_LOG("NocOpts.noc_sat_routing_latency_overrun_weighting: %d\n", NocOpts.noc_sat_routing_latency_overrun_weighting);
    VTR_LOG("NocOpts.noc_sat_routing_congestion_weighting: %d\n", NocOpts.noc_sat_routing_congestion_weighting);
    VTR_LOG("NocOpts.noc_sat_routing_num_workers: %d\n", NocOpts.noc_sat_routing_num_workers);
    VTR_LOG("NocOpts.noc_routing_algorithm: %s\n", NocOpts.noc_placement_file_name.c_str());
    VTR_LOG("\n");
}
