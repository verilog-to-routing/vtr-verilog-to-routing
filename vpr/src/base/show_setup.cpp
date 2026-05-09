#include "show_setup.h"

#include <fstream>
#include <tuple>

#include "ap_flow_enums.h"
#include "globals.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "vpr_error.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"

static void show_packer_opts(const t_packer_opts& packer_opts);
static void show_netlist_opts(const t_netlist_opts& netlist_opts);
static void show_placer_opts(const t_placer_opts& placer_opts);
static void show_analytical_placer_opts(const t_ap_opts& ap_opts);
static void show_router_opts(const t_router_opts& router_opts);
static void show_analysis_opts(const t_analysis_opts& analysis_opts);
static void show_noc_opts(const t_noc_opts& noc_opts);
static void show_anneal_sched(const t_annealing_sched& anneal_sched);

void show_setup(const t_vpr_setup& vpr_setup) {
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

    VTR_LOG("Packer: %s\n", stage_action_strings[vpr_setup.PackerOpts.doPacking]);
    VTR_LOG("Placer: %s\n", stage_action_strings[vpr_setup.PlacerOpts.do_placement]);
    VTR_LOG("Analytical Placer: %s\n", stage_action_strings[vpr_setup.APOpts.doAP]);
    VTR_LOG("Router: %s\n", stage_action_strings[vpr_setup.RouterOpts.doRouting]);
    VTR_LOG("Analysis: %s\n", stage_action_strings[vpr_setup.AnalysisOpts.doAnalysis]);
    VTR_LOG("\n");

    VTR_LOG("VPR was run with the following options:\n\n");

    show_netlist_opts(vpr_setup.NetlistOpts);

    if (vpr_setup.PackerOpts.doPacking != e_stage_action::SKIP) {
        show_packer_opts(vpr_setup.PackerOpts);
    }
    if (vpr_setup.PlacerOpts.do_placement != e_stage_action::SKIP) {
        show_placer_opts(vpr_setup.PlacerOpts);
    }
    if (vpr_setup.APOpts.doAP != e_stage_action::SKIP) {
        show_analytical_placer_opts(vpr_setup.APOpts);
    }
    if (vpr_setup.RouterOpts.doRouting != e_stage_action::SKIP) {
        show_router_opts(vpr_setup.RouterOpts);
    }
    if (vpr_setup.AnalysisOpts.doAnalysis != e_stage_action::SKIP) {
        show_analysis_opts(vpr_setup.AnalysisOpts);
    }
    if (vpr_setup.NocOpts.noc) {
        show_noc_opts(vpr_setup.NocOpts);
    }
}

void ClusteredNetlistStats::write_human_(std::ostream& output) const {
    output << "Cluster level netlist and block usage statistics\n";
    output << "Netlist num_nets: " << num_nets << "\n";
    output << "Netlist num_blocks: " << num_blocks << "\n";
    for (const t_logical_block_type& type : logical_block_types) {
        output << "Netlist " << type.name << " blocks: " << num_blocks_type[type.index] << ".\n";
    }

    output << "Netlist inputs pins: " << L_num_p_inputs << "\n";
    output << "Netlist output pins: " << L_num_p_outputs << "\n";
}
void ClusteredNetlistStats::write_json_(std::ostream& output) const {
    output << "{\n";

    output << "  \"num_nets\": \"" << num_nets << "\",\n";
    output << "  \"num_blocks\": \"" << num_blocks << "\",\n";

    output << "  \"input_pins\": \"" << L_num_p_inputs << "\",\n";
    output << "  \"output_pins\": \"" << L_num_p_outputs << "\",\n";

    output << "  \"blocks\": {\n";

    for (const t_logical_block_type& type : logical_block_types) {
        output << "    \"" << type.name << "\": " << num_blocks_type[type.index];
        if ((int)type.index < (int)logical_block_types.size() - 1)
            output << ",\n";
        else
            output << "\n";
    }
    output << "  }\n";
    output << "}\n";
}

void ClusteredNetlistStats::write_xml_(std::ostream& output) const {
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<block_usage_report>\n";

    output << "  <nets num=\"" << num_nets << "\"></nets>\n";
    output << "  <blocks num=\"" << num_blocks << "\">\n";

    for (const t_logical_block_type& type : logical_block_types) {
        output << "    <block type=\"" << type.name << "\" usage=\"" << num_blocks_type[type.index] << "\"></block>\n";
    }
    output << "  </blocks>\n";

    output << "  <input_pins num=\"" << L_num_p_inputs << "\"></input_pins>\n";
    output << "  <output_pins num=\"" << L_num_p_outputs << "\"></output_pins>\n";

    output << "</block_usage_report>\n";
}

ClusteredNetlistStats::ClusteredNetlistStats() {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    L_num_p_inputs = 0;
    L_num_p_outputs = 0;
    num_blocks_type = std::vector<int>(device_ctx.logical_block_types.size(), 0);
    num_nets = (int)cluster_ctx.clb_nlist.nets().size();
    num_blocks = (int)cluster_ctx.clb_nlist.blocks().size();
    logical_block_types = device_ctx.logical_block_types;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        t_logical_block_type_ptr logical_block = cluster_ctx.clb_nlist.block_type(blk_id);
        t_physical_tile_type_ptr physical_tile = pick_physical_type(logical_block);
        num_blocks_type[logical_block->index]++;
        if (physical_tile->is_io()) {
            for (int j = 0; j < logical_block->pb_type->num_pins; j++) {
                int physical_pin = get_physical_pin(physical_tile, logical_block, j);

                if (cluster_ctx.clb_nlist.block_net(blk_id, j) != ClusterNetId::INVALID()) {
                    e_pin_type pin_type = get_pin_type_from_pin_physical_num(physical_tile, physical_pin);
                    if (pin_type == e_pin_type::DRIVER) {
                        L_num_p_inputs++;
                    } else {
                        VTR_ASSERT(pin_type == e_pin_type::RECEIVER);
                        L_num_p_outputs++;
                    }
                }
            }
        }
    }
}

void ClusteredNetlistStats::write(ClusteredNetlistStats::e_clustered_netlist_output_format fmt,
                                  std::ostream& output) const {
    switch (fmt) {
        case ClusteredNetlistStats::e_clustered_netlist_output_format::HUMAN_READABLE:
            write_human_(output);
            break;
        case ClusteredNetlistStats::e_clustered_netlist_output_format::JSON:
            write_json_(output);
            break;
        case ClusteredNetlistStats::e_clustered_netlist_output_format::XML:
            write_xml_(output);
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Unknown extension on in block usage summary file");
            break;
    }
}

void write_clustered_netlist_stats(const std::string& block_usage_filename) {
    const ClusteredNetlistStats stats;

    stats.write(ClusteredNetlistStats::e_clustered_netlist_output_format::HUMAN_READABLE, std::cout);

    if (!block_usage_filename.empty()) {
        ClusteredNetlistStats::e_clustered_netlist_output_format fmt;

        if (vtr::check_file_name_extension(block_usage_filename, ".json")) {
            fmt = ClusteredNetlistStats::e_clustered_netlist_output_format::JSON;
        } else if (vtr::check_file_name_extension(block_usage_filename, ".xml")) {
            fmt = ClusteredNetlistStats::e_clustered_netlist_output_format::XML;
        } else if (vtr::check_file_name_extension(block_usage_filename, ".txt")) {
            fmt = ClusteredNetlistStats::e_clustered_netlist_output_format::HUMAN_READABLE;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unknown extension on output %s", block_usage_filename.c_str());
        }

        std::fstream fp;
        fp.open(block_usage_filename, std::fstream::out | std::fstream::trunc);
        stats.write(fmt, fp);
        fp.close();
    }
}

static void show_anneal_sched(const t_annealing_sched& anneal_sched) {
    VTR_LOG("anneal_sched.type: ");
    switch (anneal_sched.type) {
        case e_sched_type::AUTO_SCHED:
            VTR_LOG("AUTO_SCHED\n");
            break;
        case e_sched_type::USER_SCHED:
            VTR_LOG("USER_SCHED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown annealing schedule\n");
    }

    VTR_LOG("anneal_sched.inner_num: %f\n", anneal_sched.inner_num);

    if (e_sched_type::USER_SCHED == anneal_sched.type) {
        VTR_LOG("anneal_sched.init_t: %f\n", anneal_sched.init_t);
        VTR_LOG("anneal_sched.alpha_t: %f\n", anneal_sched.alpha_t);
        VTR_LOG("anneal_sched.exit_t: %f\n", anneal_sched.exit_t);
    }
}

static void show_router_opts(const t_router_opts& router_opts) {
    VTR_LOG("router_opts.route_type: ");
    switch (router_opts.route_type) {
        case e_route_type::GLOBAL:
            VTR_LOG("GLOBAL\n");
            break;
        case e_route_type::DETAILED:
            VTR_LOG("DETAILED\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown router opt\n");
    }

    VTR_LOG("router_opts.flat_routing: ");
    if (router_opts.flat_routing) {
        VTR_LOG("true\n");
    } else {
        VTR_LOG("false\n");
    }

    VTR_LOG("router_opts.choke_points: ");
    if (router_opts.has_choke_point) {
        VTR_LOG("on\n");
    } else {
        VTR_LOG("off\n");
    }

    VTR_ASSERT(e_route_type::GLOBAL == router_opts.route_type || e_route_type::DETAILED == router_opts.route_type);

    VTR_LOG("router_opts.router_algorithm: ");
    switch (router_opts.router_algorithm) {
        case NESTED:
            VTR_LOG("NESTED\n");
            break;
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
            switch (router_opts.route_type) {
                case e_route_type::DETAILED:
                    VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "<Unknown>\n");
                case e_route_type::GLOBAL:
                    VTR_LOG_ERROR("Unknown router algorithm\n");
                    break;
                default:
                    break;
            }
    }

    VTR_LOG("router_opts.base_cost_type: ");
    switch (router_opts.base_cost_type) {
        case DELAY_NORMALIZED:
            VTR_LOG("DELAY_NORMALIZED\n");
            break;
        case DELAY_NORMALIZED_LENGTH:
            if (e_route_type::GLOBAL == router_opts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH\n");
            break;
        case DELAY_NORMALIZED_LENGTH_BOUNDED:
            if (e_route_type::GLOBAL == router_opts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH_BOUNDED\n");
            break;
        case DELAY_NORMALIZED_FREQUENCY:
            if (e_route_type::GLOBAL == router_opts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_FREQUENCY\n");
            break;
        case DELAY_NORMALIZED_LENGTH_FREQUENCY:
            if (e_route_type::GLOBAL == router_opts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DELAY_NORMALIZED_LENGTH_FREQUENCY\n");
            break;
        case DEMAND_ONLY:
            VTR_LOG("DEMAND_ONLY\n");
            break;
        case DEMAND_ONLY_NORMALIZED_LENGTH:
            if (e_route_type::GLOBAL == router_opts.route_type) {
                VTR_LOG_ERROR("Unknown router base cost type\n");
                break;
            }

            VTR_LOG("DEMAND_ONLY_NORMALIZED_LENGTH\n");
            break;
        default:
            switch (router_opts.route_type) {
                case e_route_type::DETAILED:
                    VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown base_cost_type\n");
                case e_route_type::GLOBAL:
                    VTR_LOG_ERROR("Unknown router base cost type\n");
                    break;
                default:
                    break;
            }
    }

    VTR_LOG("router_opts.fixed_channel_width: ");
    if (NO_FIXED_CHANNEL_WIDTH == router_opts.fixed_channel_width) {
        VTR_LOG("NO_FIXED_CHANNEL_WIDTH\n");
    } else {
        VTR_LOG("%d\n", router_opts.fixed_channel_width);
    }

    if (e_route_type::DETAILED == router_opts.route_type) {
        VTR_LOG("router_opts.check_route: ");
        switch (router_opts.check_route) {
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

    VTR_LOG("router_opts.acc_fac: %f\n", router_opts.acc_fac);
    VTR_LOG("router_opts.bb_factor: %d\n", router_opts.bb_factor);
    VTR_LOG("router_opts.bend_cost: %f\n", router_opts.bend_cost);
    VTR_LOG("router_opts.first_iter_pres_fac: %f\n", router_opts.first_iter_pres_fac);
    VTR_LOG("router_opts.initial_pres_fac: %f\n", router_opts.initial_pres_fac);
    VTR_LOG("router_opts.pres_fac_mult: %f\n", router_opts.pres_fac_mult);
    VTR_LOG("router_opts.max_pres_fac: %f\n", router_opts.max_pres_fac);
    VTR_LOG("router_opts.max_router_iterations: %d\n", router_opts.max_router_iterations);
    VTR_LOG("router_opts.min_incremental_reroute_fanout: %d\n", router_opts.min_incremental_reroute_fanout);
    VTR_LOG("router_opts.initial_acc_cost_chan_congestion_threshold: %f\n", router_opts.initial_acc_cost_chan_congestion_threshold);
    VTR_LOG("router_opts.initial_acc_cost_chan_congestion_weight: %f\n", router_opts.initial_acc_cost_chan_congestion_weight);
    VTR_LOG("router_opts.do_check_rr_graph: %s\n", router_opts.do_check_rr_graph ? "true" : "false");
    VTR_LOG("router_opts.verify_binary_search: %s\n", router_opts.verify_binary_search ? "true" : "false");
    VTR_LOG("router_opts.min_channel_width_hint: %d\n", router_opts.min_channel_width_hint);
    VTR_LOG("router_opts.read_rr_edge_metadata: %s\n", router_opts.read_rr_edge_metadata ? "true" : "false");
    VTR_LOG("router_opts.exit_after_first_routing_iteration: %s\n", router_opts.exit_after_first_routing_iteration ? "true" : "false");

    if (TIMING_DRIVEN == router_opts.router_algorithm) {
        VTR_LOG("router_opts.astar_fac: %f\n", router_opts.astar_fac);
        VTR_LOG("router_opts.astar_offset: %f\n", router_opts.astar_offset);
        VTR_LOG("router_opts.router_profiler_astar_fac: %f\n", router_opts.router_profiler_astar_fac);
        VTR_LOG("router_opts.enable_parallel_connection_router: %s\n", router_opts.enable_parallel_connection_router ? "true" : "false");
        VTR_LOG("router_opts.post_target_prune_fac: %f\n", router_opts.post_target_prune_fac);
        VTR_LOG("router_opts.post_target_prune_offset: %g\n", router_opts.post_target_prune_offset);
        VTR_LOG("router_opts.multi_queue_num_threads: %d\n", router_opts.multi_queue_num_threads);
        VTR_LOG("router_opts.multi_queue_num_queues: %d\n", router_opts.multi_queue_num_queues);
        VTR_LOG("router_opts.multi_queue_direct_draining: %s\n", router_opts.multi_queue_direct_draining ? "true" : "false");
        VTR_LOG("router_opts.criticality_exp: %f\n", router_opts.criticality_exp);
        VTR_LOG("router_opts.max_criticality: %f\n", router_opts.max_criticality);
        VTR_LOG("router_opts.init_wirelength_abort_threshold: %f\n", router_opts.init_wirelength_abort_threshold);

        if (e_route_type::GLOBAL == router_opts.route_type)
            VTR_LOG("router_opts.incr_reroute_delay_ripup: %f\n", router_opts.incr_reroute_delay_ripup);
        else {
            std::string incr_delay_ripup_opts[3] = {"ON", "OFF", "AUTO"};
            VTR_LOG("router_opts.incr_reroute_delay_ripup: %s\n", incr_delay_ripup_opts[(size_t)router_opts.incr_reroute_delay_ripup].c_str());
        }

        VTR_LOG("router_opts.save_routing_per_iteration: %s\n", router_opts.save_routing_per_iteration ? "true" : "false");
        VTR_LOG("router_opts.congested_routing_iteration_threshold_frac: %f\n", router_opts.congested_routing_iteration_threshold_frac);
        VTR_LOG("router_opts.high_fanout_threshold: %d\n", router_opts.high_fanout_threshold);
        VTR_LOG("router_opts.router_debug_net: %d\n", router_opts.router_debug_net);
        VTR_LOG("router_opts.router_debug_sink_rr: %d\n", router_opts.router_debug_sink_rr);
        VTR_LOG("router_opts.router_debug_iteration: %d\n", router_opts.router_debug_iteration);
        VTR_LOG("router_opts.max_convergence_count: %d\n", router_opts.max_convergence_count);
        VTR_LOG("router_opts.reconvergence_cpd_threshold: %f\n", router_opts.reconvergence_cpd_threshold);
        VTR_LOG("router_opts.update_lower_bound_delays: %s\n", router_opts.update_lower_bound_delays ? "true" : "false");
        VTR_LOG("router_opts.first_iteration_timing_report_file: %s\n", router_opts.first_iteration_timing_report_file.c_str());

        VTR_LOG("router_opts.route_bb_update: ");
        switch (router_opts.route_bb_update) {
            case e_route_bb_update::STATIC:
                VTR_LOG("STATIC\n");
                break;
            case e_route_bb_update::DYNAMIC:
                VTR_LOG("DYNAMIC\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown route_bb_update\n");
        }

        VTR_LOG("router_opts.lookahead_type: ");
        switch (router_opts.lookahead_type) {
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
            case e_router_lookahead::SIMPLE:
                VTR_LOG("SIMPLE\n");
                break;
            case e_router_lookahead::NO_OP:
                VTR_LOG("NO_OP\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown lookahead_type\n");
        }

        VTR_LOG("router_opts.initial_timing: ");
        switch (router_opts.initial_timing) {
            case e_router_initial_timing::ALL_CRITICAL:
                VTR_LOG("ALL_CRITICAL\n");
                break;
            case e_router_initial_timing::LOOKAHEAD:
                VTR_LOG("LOOKAHEAD\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown initial_timing\n");
        }

        VTR_LOG("router_opts.router_heap: ");
        switch (router_opts.router_heap) {
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

    if (e_route_type::DETAILED == router_opts.route_type) {
        if (router_opts.routing_failure_predictor == SAFE)
            VTR_LOG("router_opts.routing_failure_predictor = SAFE\n");
        else if (router_opts.routing_failure_predictor == AGGRESSIVE)
            VTR_LOG("router_opts.routing_failure_predictor = AGGRESSIVE\n");
        else if (router_opts.routing_failure_predictor == OFF)
            VTR_LOG("router_opts.routing_failure_predictor = OFF\n");

        if (router_opts.routing_budgets_algorithm == DISABLE) {
            VTR_LOG("router_opts.routing_budgets_algorithm = DISABLE\n");
        } else if (router_opts.routing_budgets_algorithm == MINIMAX) {
            VTR_LOG("router_opts.routing_budgets_algorithm = MINIMAX\n");
        } else if (router_opts.routing_budgets_algorithm == YOYO) {
            VTR_LOG("router_opts.routing_budgets_algorithm = YOYO\n");
        } else if (router_opts.routing_budgets_algorithm == SCALE_DELAY) {
            VTR_LOG("router_opts.routing_budgets_algorithm = SCALE_DELAY\n");
        }
    }

    VTR_LOG("\n");
}

static void show_placer_opts(const t_placer_opts& placer_opts) {
    VTR_LOG("placer_opts.place_freq: ");
    switch (placer_opts.place_freq) {
        case e_place_freq::ONCE:
            VTR_LOG("ONCE\n");
            break;
        case e_place_freq::ALWAYS:
            VTR_LOG("ALWAYS\n");
            break;
        default:
            VTR_LOG_ERROR("Unknown Place Freq\n");
    }
    if (placer_opts.place_freq == e_place_freq::ONCE || placer_opts.place_freq == e_place_freq::ALWAYS) {
        VTR_LOG("placer_opts.place_algorithm: ");
        switch (placer_opts.place_algorithm.get()) {
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

        VTR_LOG("placer_opts.pad_loc_type: ");
        switch (placer_opts.pad_loc_type) {
            case e_pad_loc_type::FREE:
                VTR_LOG("FREE\n");
                break;
            case e_pad_loc_type::RANDOM:
                VTR_LOG("RANDOM\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown I/O pad location type\n");
        }

        VTR_LOG("placer_opts.constraints_file: ");
        if (placer_opts.constraints_file.empty()) {
            VTR_LOG("No constraints file given\n");
        } else {
            VTR_LOG("Using constraints file '%s'\n", placer_opts.constraints_file.c_str());
        }

        VTR_LOG("placer_opts.place_chan_width: %d\n", placer_opts.place_chan_width);

        VTR_LOG("placer_opts.congestion_factor: %f\n", placer_opts.congestion_factor);
        if (placer_opts.congestion_factor > 0.0f) {
            VTR_LOG("placer_opts.congestion_rlim_trigger_ratio: %f\n", placer_opts.congestion_rlim_trigger_ratio);
            VTR_LOG("placer_opts.congestion_chan_util_threshold: %f\n", placer_opts.congestion_chan_util_threshold);
        }

        if (placer_opts.place_algorithm.is_timing_driven()) {
            VTR_LOG("placer_opts.inner_loop_recompute_divider: %d\n", placer_opts.inner_loop_recompute_divider);
            VTR_LOG("placer_opts.recompute_crit_iter: %d\n", placer_opts.recompute_crit_iter);
            VTR_LOG("placer_opts.timing_tradeoff: %f\n", placer_opts.timing_tradeoff);
            VTR_LOG("placer_opts.td_place_exp_first: %f\n", placer_opts.td_place_exp_first);
            VTR_LOG("placer_opts.td_place_exp_last: %f\n", placer_opts.td_place_exp_last);
            VTR_LOG("placer_opts.delay_offset: %f\n", placer_opts.delay_offset);
            VTR_LOG("placer_opts.delay_ramp_delta_threshold: %d\n", placer_opts.delay_ramp_delta_threshold);
            VTR_LOG("placer_opts.delay_ramp_slope: %f\n", placer_opts.delay_ramp_slope);
            VTR_LOG("placer_opts.tsu_rel_margin: %f\n", placer_opts.tsu_rel_margin);
            VTR_LOG("placer_opts.tsu_abs_margin: %f\n", placer_opts.tsu_abs_margin);
            VTR_LOG("placer_opts.post_place_timing_report_file: %s\n", placer_opts.post_place_timing_report_file.c_str());
            VTR_LOG("placer_opts.allowed_tiles_for_delay_model: %s\n", placer_opts.allowed_tiles_for_delay_model.c_str());

            std::string e_reducer_strings[5] = {"MIN", "MAX", "MEDIAN", "ARITHMEAN", "GEOMEAN"};
            if ((size_t)placer_opts.delay_model_reducer > 4)
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_reducer\n");
            VTR_LOG("placer_opts.delay_model_reducer: %s\n", e_reducer_strings[(size_t)placer_opts.delay_model_reducer].c_str());

            std::string place_delay_model_strings[3] = {"SIMPLE", "DELTA", "DELTA_OVERRIDE"};
            if ((size_t)placer_opts.delay_model_type > 2)
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_type\n");
            VTR_LOG("placer_opts.delay_model_type: %s\n", place_delay_model_strings[(size_t)placer_opts.delay_model_type].c_str());
        }

        VTR_LOG("placer_opts.rlim_escape_fraction: %f\n", placer_opts.rlim_escape_fraction);
        VTR_LOG("placer_opts.move_stats_file: %s\n", placer_opts.move_stats_file.c_str());
        VTR_LOG("placer_opts.placement_saves_per_temperature: %d\n", placer_opts.placement_saves_per_temperature);

        VTR_LOG("placer_opts.effort_scaling: ");
        switch (placer_opts.effort_scaling) {
            case CIRCUIT:
                VTR_LOG("CIRCUIT\n");
                break;
            case DEVICE_CIRCUIT:
                VTR_LOG("DEVICE_CIRCUIT\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown effort_scaling\n");
        }

        VTR_LOG("placer_opts.place_delta_delay_matrix_calculation_method: ");
        switch (placer_opts.place_delta_delay_matrix_calculation_method) {
            case e_place_delta_delay_algorithm::ASTAR_ROUTE:
                VTR_LOG("ASTAR_ROUTE\n");
                break;
            case e_place_delta_delay_algorithm::DIJKSTRA_EXPANSION:
                VTR_LOG("DIJKSTRA_EXPANSION\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delta_delay_matrix_calculation_method\n");
        }

        VTR_LOG("placer_opts.seed: %d\n", placer_opts.seed);

        show_anneal_sched(placer_opts.anneal_sched);

        VTR_LOG("placer_opts.anneal_init_t_estimator: ");
        switch (placer_opts.anneal_init_t_estimator) {
            case e_anneal_init_t_estimator::COST_VARIANCE:
                VTR_LOG("COST_VARIANCE\n");
                break;
            case e_anneal_init_t_estimator::EQUILIBRIUM:
                VTR_LOG("EQUILIBRIUM\n");
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown anneal_init_t_estimator\n");
        }
    }
    VTR_LOG("\n");
}

static void show_analytical_placer_opts(const t_ap_opts& ap_opts) {
    VTR_LOG("AnalyticalPlacerOpts.analytical_solver_type: ");
    switch (ap_opts.analytical_solver_type) {
        case e_ap_analytical_solver::Identity:
            VTR_LOG("identity\n");
            break;
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
    switch (ap_opts.partial_legalizer_type) {
        case e_ap_partial_legalizer::Identity:
            VTR_LOG("none\n");
            break;
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
    switch (ap_opts.full_legalizer_type) {
        case e_ap_full_legalizer::Naive:
            VTR_LOG("naive\n");
            break;
        case e_ap_full_legalizer::APPack:
            VTR_LOG("appack\n");
            break;
        case e_ap_full_legalizer::FlatRecon:
            VTR_LOG("flat-recon\n");
            break;
        case e_ap_full_legalizer::SAPack:
            VTR_LOG("sa-pack\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown full_legalizer_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.detailed_placer_type: ");
    switch (ap_opts.detailed_placer_type) {
        case e_ap_detailed_placer::Identity:
            VTR_LOG("none\n");
            break;
        case e_ap_detailed_placer::Annealer:
            VTR_LOG("annealer\n");
            break;
        default:
            VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown detailed_placer_type\n");
    }

    VTR_LOG("AnalyticalPlacerOpts.ap_timing_tradeoff: %f\n", ap_opts.ap_timing_tradeoff);
    VTR_LOG("AnalyticalPlacerOpts.ap_high_fanout_threshold: %d\n", ap_opts.ap_high_fanout_threshold);
    VTR_LOG("AnalyticalPlacerOpts.log_verbosity: %d\n", ap_opts.log_verbosity);
}

static void show_netlist_opts(const t_netlist_opts& netlist_opts) {
    VTR_LOG("netlist_opts.absorb_buffer_luts            : %s\n", (netlist_opts.absorb_buffer_luts) ? "true" : "false");
    VTR_LOG("netlist_opts.sweep_dangling_primary_ios    : %s\n", (netlist_opts.sweep_dangling_primary_ios) ? "true" : "false");
    VTR_LOG("netlist_opts.sweep_dangling_nets           : %s\n", (netlist_opts.sweep_dangling_nets) ? "true" : "false");
    VTR_LOG("netlist_opts.sweep_dangling_blocks         : %s\n", (netlist_opts.sweep_dangling_blocks) ? "true" : "false");
    VTR_LOG("netlist_opts.sweep_constant_primary_outputs: %s\n", (netlist_opts.sweep_constant_primary_outputs) ? "true" : "false");
    VTR_LOG("netlist_opts.netlist_verbosity             : %d\n", netlist_opts.netlist_verbosity);

    std::string const_gen_inference_strings[3] = {"NONE", "COMB", "COMB_SEQ"};
    if ((size_t)netlist_opts.const_gen_inference > 3)
        VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown delay_model_reducer\n");
    VTR_LOG("netlist_opts.const_gen_inference           : %s\n", const_gen_inference_strings[(size_t)netlist_opts.const_gen_inference].c_str());

    VTR_LOG("\n");
}

static void show_analysis_opts(const t_analysis_opts& analysis_opts) {
    VTR_LOG("analysis_opts.gen_post_synthesis_netlist: %s\n", (analysis_opts.gen_post_synthesis_netlist) ? "true" : "false");
    VTR_LOG("analysis_opts.gen_post_implementation_merged_netlist: %s\n", analysis_opts.gen_post_implementation_merged_netlist ? "true" : "false");
    VTR_LOG("analysis_opts.gen_post_implementation_sdc: %s\n", analysis_opts.gen_post_implementation_sdc ? "true" : "false");
    VTR_LOG("analysis_opts.timing_report_npaths: %d\n", analysis_opts.timing_report_npaths);
    VTR_LOG("analysis_opts.timing_report_skew: %s\n", analysis_opts.timing_report_skew ? "true" : "false");
    VTR_LOG("analysis_opts.echo_dot_timing_graph_node: %s\n", analysis_opts.echo_dot_timing_graph_node.c_str());

    VTR_LOG("analysis_opts.timing_report_detail: ");
    switch (analysis_opts.timing_report_detail) {
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
        std::make_tuple(&analysis_opts.post_synth_netlist_unconn_input_handling, "post_synth_netlist_unconn_input_handling"),
        std::make_tuple(&analysis_opts.post_synth_netlist_unconn_output_handling, "post_synth_netlist_unconn_output_handling"),
    };
    for (const auto& opt : opts) {
        e_post_synth_netlist_unconn_handling value = *std::get<0>(opt);
        VTR_LOG("analysis_opts.%s: ", std::get<1>(opt));
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
    VTR_LOG("analysis_opts.post_synth_netlist_module_parameters: %s\n", analysis_opts.post_synth_netlist_module_parameters ? "on" : "off");
    VTR_LOG("\n");
}

static void show_packer_opts(const t_packer_opts& packer_opts) {
    VTR_LOG("packer_opts.allow_unrelated_clustering: ");
    if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::ON) {
        VTR_LOG("true\n");
    } else if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        VTR_LOG("false\n");
    } else if (packer_opts.allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
        VTR_LOG("auto\n");
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_UNKNOWN, "Unknown packer allow_unrelated_clustering\n");
    }
    VTR_LOG("packer_opts.timing_gain_weight: %f\n", packer_opts.timing_gain_weight);
    VTR_LOG("packer_opts.connection_gain_weight: %f\n", packer_opts.connection_gain_weight);
    VTR_LOG("packer_opts.cluster_seed_type: ");
    switch (packer_opts.cluster_seed_type) {
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
    VTR_LOG("packer_opts.connection_driven: %s", (packer_opts.connection_driven ? "true\n" : "false\n"));
    VTR_LOG("packer_opts.timing_driven: %s", (packer_opts.timing_driven ? "true\n" : "false\n"));
    VTR_LOG("packer_opts.target_external_pin_util: %s", vtr::join(packer_opts.target_external_pin_util, " ").c_str());
    VTR_LOG("\n");
    VTR_LOG("\n");
}

static void show_noc_opts(const t_noc_opts& noc_opts) {
    VTR_LOG("noc_opts.noc_flows_file: %s\n", noc_opts.noc_flows_file.c_str());
    VTR_LOG("noc_opts.noc_routing_algorithm: %s\n", noc_opts.noc_routing_algorithm.c_str());
    VTR_LOG("noc_opts.noc_placement_weighting: %f\n", noc_opts.noc_placement_weighting);
    VTR_LOG("noc_opts.noc_aggregate_bandwidth_weighting: %f\n", noc_opts.noc_aggregate_bandwidth_weighting);
    VTR_LOG("noc_opts.noc_latency_constraints_weighting: %f\n", noc_opts.noc_latency_constraints_weighting);
    VTR_LOG("noc_opts.noc_latency_weighting: %f\n", noc_opts.noc_latency_weighting);
    VTR_LOG("noc_opts.noc_congestion_weighting: %f\n", noc_opts.noc_congestion_weighting);
    VTR_LOG("noc_opts.noc_swap_percentage: %d%%\n", noc_opts.noc_swap_percentage);
    VTR_LOG("noc_opts.noc_sat_routing_bandwidth_resolution: %d\n", noc_opts.noc_sat_routing_bandwidth_resolution);
    VTR_LOG("noc_opts.noc_sat_routing_latency_overrun_weighting: %d\n", noc_opts.noc_sat_routing_latency_overrun_weighting);
    VTR_LOG("noc_opts.noc_sat_routing_congestion_weighting: %d\n", noc_opts.noc_sat_routing_congestion_weighting);
    VTR_LOG("noc_opts.noc_sat_routing_num_workers: %d\n", noc_opts.noc_sat_routing_num_workers);
    VTR_LOG("noc_opts.noc_routing_algorithm: %s\n", noc_opts.noc_placement_file_name.c_str());
    VTR_LOG("\n");
}
