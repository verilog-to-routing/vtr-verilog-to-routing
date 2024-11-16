#include <cstdio>
#include <cmath>
#include <memory>
#include <chrono>
#include <optional>

#include "NetPinTimingInvalidator.h"
#include "clustered_netlist.h"
#include "device_grid.h"
#include "verify_placement.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_time.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "annealer.h"
#include "read_place.h"
#include "draw.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "histogram.h"
#include "place_util.h"
#include "analytic_placer.h"
#include "initial_placement.h"
#include "place_delay_model.h"
#include "place_timing_update.h"
#include "move_transactions.h"
#include "move_utils.h"
#include "buttons.h"

#include "PlacementDelayCalculator.h"
#include "VprTimingGraphResolver.h"
#include "timing_util.h"
#include "timing_info.h"
#include "concrete_timing_info.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

#include "RL_agent_util.h"
#include "place_checkpoint.h"

#include "clustered_netlist_utils.h"

#include "noc_place_utils.h"

#include "net_cost_handler.h"
#include "placer_state.h"
#include "placer.h"

/********************* Static subroutines local to place.c *******************/
#ifdef VERBOSE
void print_clb_placement(const char* fname);
#endif

/**
 * @brief determine the type of the bounding box used by the placer to predict the wirelength
 *
 * @param place_bb_mode The bounding box mode passed by the CLI
 * @param rr_graph The routing resource graph
 */
static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph);

static void free_placement_structs();

static int count_connections();

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat,
                                               const BlkLocRegistry& blk_loc_registry);

static void print_place_status_header(bool noc_enabled);

static void print_place_status(const t_annealing_state& state,
                               const t_placer_statistics& stats,
                               float elapsed_sec,
                               float cpd,
                               float sTNS,
                               float sWNS,
                               size_t tot_moves,
                               bool noc_enabled,
                               const NocCostTerms& noc_cost_terms);

static void print_resources_utilization(const BlkLocRegistry& blk_loc_registry);

static void print_placement_swaps_stats(const t_annealing_state& state, const t_swap_stats& swap_stats);

/**
 * @brief Copies the placement location variables into the global placement context.
 * @param blk_loc_registry The placement location variables to be copied.
 */
static void copy_locs_to_global_state(const BlkLocRegistry& blk_loc_registry);

/*****************************************************************************/
void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               const std::vector<t_direct_inf>& directs,
               bool is_flat) {
    /* Does almost all the work of placing a circuit.  Width_fac gives the   *
     * width of the widest channel.  Place_cost_exp says what exponent the   *
     * width should be taken to when calculating costs.  This allows a       *
     * greater bias for anisotropic architectures.                           */

    /* Currently, the functions that require is_flat as their parameter and are called during placement should
     * receive is_flat as false. For example, if the RR graph of router lookahead is built here, it should be as
     * if is_flat is false, even if is_flat is set to true from the command line.
     */
    VTR_ASSERT(!is_flat);
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& atom_ctx = g_vpr_ctx.atom();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& timing_ctx = g_vpr_ctx.timing();
    auto pre_place_timing_stats = timing_ctx.stats;


    float sTNS = NAN;
    float sWNS = NAN;

    char msg[vtr::bufsize];

    /* Placement delay model is independent of the placement and can be shared across
     * multiple placers. So, it is created and initialized once. */
    std::shared_ptr<PlaceDelayModel> place_delay_model;
    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = alloc_lookups_and_delay_model(net_list,
                                                          chan_width_dist,
                                                          placer_opts,
                                                          router_opts,
                                                          det_routing_arch,
                                                          segment_inf,
                                                          directs,
                                                          is_flat);

        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    g_vpr_ctx.mutable_placement().cube_bb = is_cube_bb(placer_opts.place_bounding_box_mode, device_ctx.rr_graph);
    const bool cube_bb = g_vpr_ctx.placement().cube_bb;

    VTR_LOG("\n");
    VTR_LOG("Bounding box mode is %s\n", (cube_bb ? "Cube" : "Per-layer"));
    VTR_LOG("\n");

    int move_lim = (int)(placer_opts.anneal_sched.inner_num * pow(net_list.blocks().size(), 1.3333));

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.lock_loc_vars();
    place_ctx.compressed_block_grids = create_compressed_block_grids();

    Placer placer(net_list, placer_opts, analysis_opts, noc_opts, directs, place_delay_model, cube_bb);

#ifndef NO_GRAPHICS
    if (placer.noc_cost_handler_.has_value()) {
        get_draw_state_vars()->set_noc_link_bandwidth_usages_ref(placer.noc_cost_handler_->get_link_bandwidth_usages());
    }
#endif

    const int width_fac = placer_opts.place_chan_width;
    init_draw_coords((float)width_fac, placer.placer_state_.blk_loc_registry());

    sprintf(msg,
            "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);

    // Draw the initial placement
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place", 0,
                                               0);
        VTR_LOG("Saving initial placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str(), blk_loc_registry.block_locs());
    }

    bool skip_anneal = false;

#ifdef ENABLE_ANALYTIC_PLACE
    // Analytic placer: When enabled, skip most of the annealing and go straight to quench
    // TODO: refactor goto label.
    if (placer_opts.enable_analytic_placer) {
        skip_anneal = true;
    }
#endif /* ENABLE_ANALYTIC_PLACE */

    PlacementAnnealer annealer(placer_opts, placer_state, costs, net_cost_handler, noc_cost_handler,
                               noc_opts, rng, std::move(move_generator), std::move(move_generator2), place_delay_model.get(),
                               placer_criticalities.get(), placer_setup_slacks.get(), timing_info.get(), pin_timing_invalidator.get(), move_lim);

    const t_annealing_state& annealing_state = annealer.get_annealing_state();
    const auto& [swap_stats, move_type_stats, placer_stats] = annealer.get_stats();

    if (!skip_anneal) {
        //Table header
        VTR_LOG("\n");
        print_place_status_header(noc_opts.noc);

        /* Outer loop of the simulated annealing begins */
        do {
            vtr::Timer temperature_timer;

            annealer.outer_loop_update_timing_info();

            if (placer_opts.place_algorithm.is_timing_driven()) {
                critical_path = timing_info->least_slack_critical_path();
                sTNS = timing_info->setup_total_negative_slack();
                sWNS = timing_info->setup_worst_negative_slack();

                // see if we should save the current placement solution as a checkpoint
                if (placer_opts.place_checkpointing && annealer.get_agent_state() == e_agent_state::LATE_IN_THE_ANNEAL) {
                    save_placement_checkpoint_if_needed(blk_loc_registry.block_locs(),
                                                        placement_checkpoint,
                                                        timing_info, costs, critical_path.delay());
                }
            }

            // do a complete inner loop iteration
            annealer.placement_inner_loop();

            print_place_status(annealing_state, placer_stats, temperature_timer.elapsed_sec(),
                               critical_path.delay(), sTNS, sWNS, annealer.get_total_iteration(),
                               noc_opts.noc, costs.noc_cost_terms);

            sprintf(msg, "Cost: %g  BB Cost %g  TD Cost %g  Temperature: %g",
                    costs.cost, costs.bb_cost, costs.timing_cost, annealing_state.t);
            update_screen(ScreenUpdatePriority::MINOR, msg, PLACEMENT, timing_info);

            //#ifdef VERBOSE
            //            if (getEchoEnabled()) {
            //                print_clb_placement("first_iteration_clb_placement.echo");
            //            }
            //#endif
        } while (annealer.outer_loop_update_state());
        /* Outer loop of the simulated annealing ends */
    } //skip_anneal ends

    // Start Quench
    annealer.start_quench();

    auto pre_quench_timing_stats = timing_ctx.stats;
    { /* Quench */

        vtr::ScopedFinishTimer temperature_timer("Placement Quench");

        annealer.outer_loop_update_timing_info();

        /* Run inner loop again with temperature = 0 so as to accept only swaps
         * which reduce the cost of the placement */
        annealer.placement_inner_loop();

        if (placer_opts.place_quench_algorithm.is_timing_driven()) {
            critical_path = timing_info->least_slack_critical_path();
            sTNS = timing_info->setup_total_negative_slack();
            sWNS = timing_info->setup_worst_negative_slack();
        }

        print_place_status(annealing_state, placer_stats, temperature_timer.elapsed_sec(),
                           critical_path.delay(), sTNS, sWNS, annealer.get_total_iteration(),
                           noc_opts.noc, costs.noc_cost_terms);
    }
    auto post_quench_timing_stats = timing_ctx.stats;

    //Final timing analysis
    PlaceCritParams crit_params;
    crit_params.crit_exponent = annealing_state.crit_exponent;
    crit_params.crit_limit = placer_opts.place_crit_limit;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        perform_full_timing_update(crit_params, place_delay_model.get(), placer_criticalities.get(),
                                   placer_setup_slacks.get(), pin_timing_invalidator.get(),
                                   timing_info.get(), &costs, placer_state);
        VTR_LOG("post-quench CPD = %g (ns) \n",
                1e9 * timing_info->least_slack_critical_path().delay());
    }

    //See if our latest checkpoint is better than the current placement solution
    if (placer_opts.place_checkpointing)
        restore_best_placement(placer_state,
                               placement_checkpoint, timing_info, costs,
                               placer_criticalities, placer_setup_slacks, place_delay_model,
                               pin_timing_invalidator, crit_params, noc_cost_handler);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
                                               annealing_state.num_temps + 1, 0);
        VTR_LOG("Saving final placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str(), blk_loc_registry.block_locs());
    }


    //#ifdef VERBOSE
    //    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_END_CLB_PLACEMENT)) {
    //        print_clb_placement(getEchoFileName(E_ECHO_END_CLB_PLACEMENT));
    //    }
    //#endif

    // Update physical pin values
    for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
        blk_loc_registry.place_sync_external_block_connections(block_id);
    }

    check_place(costs,
                place_delay_model.get(),
                placer_criticalities.get(),
                placer_opts.place_algorithm,
                noc_opts,
                placer_state,
                net_cost_handler,
                noc_cost_handler);

    //Some stats
    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", swap_stats.num_ts_called);
    blocks_affected.move_abortion_logger.report_aborted_moves();

    if (placer_opts.place_algorithm.is_timing_driven()) {
        //Final timing estimate
        VTR_ASSERT(timing_info);

        critical_path = timing_info->least_slack_critical_path();

        if (isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(
                getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                *timing_ctx.graph, *timing_ctx.constraints,
                *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(
                analysis_opts.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(
                getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)
                    + std::string(".dot"),
                *timing_info, debug_tnode);
        }

        generate_post_place_timing_reports(placer_opts, analysis_opts, *timing_info,
                                           *placement_delay_calc, is_flat, blk_loc_registry);

        /* Print critical path delay metrics */
        VTR_LOG("\n");
        print_setup_timing_summary(*timing_ctx.constraints,
                                   *timing_info->setup_analyzer(), "Placement estimated ", "");
    }

    sprintf(msg,
            "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
    VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, \n", costs.cost,
            costs.bb_cost, costs.timing_cost);
    // print the noc costs info
    if (noc_opts.noc) {
        VTR_ASSERT(noc_cost_handler.has_value());
        noc_cost_handler->print_noc_costs("\nNoC Placement Costs", costs, noc_opts);

#ifdef ENABLE_NOC_SAT_ROUTING
        if (costs.noc_cost_terms.congestion > 0.0) {
            VTR_LOG("NoC routing configuration is congested. Invoking the SAT NoC router.\n");
            invoke_sat_router(costs, noc_opts, placer_opts.seed);
        }
#endif //ENABLE_NOC_SAT_ROUTING
    }

    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);
    // Print out swap statistics
    print_resources_utilization(blk_loc_registry);

    print_placement_swaps_stats(annealing_state, swap_stats);

    move_type_stats.print_placement_move_types_stats();

    if (noc_opts.noc) {
        write_noc_placement_file(noc_opts.noc_placement_file_name, blk_loc_registry.block_locs());
    }

    free_placement_structs();

    print_timing_stats("Placement Quench", post_quench_timing_stats, pre_quench_timing_stats);
    print_timing_stats("Placement Total ", timing_ctx.stats, pre_place_timing_stats);

    VTR_LOG("update_td_costs: connections %g nets %g sum_nets %g total %g\n",
            p_runtime_ctx.f_update_td_costs_connections_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_nets_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_sum_nets_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_total_elapsed_sec);

    copy_locs_to_global_state(blk_loc_registry);
}

/*only count non-global connections */
static int count_connections() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int count = 0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        count += cluster_ctx.clb_nlist.net_sinks(net_id).size();
    }

    return count;
}

static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph) {
    bool cube_bb;
    const int number_layers = g_vpr_ctx.device().grid.get_num_layers();

    if (place_bb_mode == e_place_bounding_box_mode::AUTO_BB) {
        // If the auto_bb is used, we analyze the RR graph to see whether is there any inter-layer connection that is not
        // originated from OPIN. If there is any, cube BB is chosen, otherwise, per-layer bb is chosen.
        if (number_layers > 1 && inter_layer_connections_limited_to_opin(rr_graph)) {
            cube_bb = false;
        } else {
            cube_bb = true;
        }
    } else if (place_bb_mode == e_place_bounding_box_mode::CUBE_BB) {
        // The user has specifically asked for CUBE_BB
        cube_bb = true;
    } else {
        // The user has specifically asked for PER_LAYER_BB
        VTR_ASSERT_SAFE(place_bb_mode == e_place_bounding_box_mode::PER_LAYER_BB);
        cube_bb = false;
    }

    return cube_bb;
}

/* Frees the major structures needed by the placer (and not needed       *
 * elsewhere).   */
static void free_placement_structs() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    vtr::release_memory(place_ctx.compressed_block_grids);
}

#ifdef VERBOSE
void print_clb_placement(const char* fname) {
    /* Prints out the clb placements to a file.  */
    FILE* fp;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    fp = vtr::fopen(fname, "w");
    fprintf(fp, "Complex block placements:\n\n");

    fprintf(fp, "Block #\tName\t(X, Y, Z).\n");
    for (auto i : cluster_ctx.clb_nlist.blocks()) {
        fprintf(fp, "#%d\t%s\t(%d, %d, %d).\n", i, cluster_ctx.clb_nlist.block_name(i).c_str(), place_ctx.block_locs[i].loc.x, place_ctx.block_locs[i].loc.y, place_ctx.block_locs[i].loc.sub_tile);
    }

    fclose(fp);
}
#endif

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat,
                                               const BlkLocRegistry& blk_loc_registry) {
    const auto& timing_ctx = g_vpr_ctx.timing();
    const auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph,
                                    delay_calc, is_flat, blk_loc_registry);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                          *timing_ctx.constraints);

    timing_reporter.report_timing_setup(
        placer_opts.post_place_timing_report_file,
        *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
}

#if 0
static void update_screen_debug();

//Performs a major (i.e. interactive) placement screen update.
//This function with no arguments is useful for calling from a debugger to
//look at the intermediate implemetnation state.
static void update_screen_debug() {
    update_screen(ScreenUpdatePriority::MAJOR, "DEBUG", PLACEMENT, nullptr);
}
#endif

static void print_place_status_header(bool noc_enabled) {
    if (!noc_enabled) {
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
        VTR_LOG(
            "Tnum   Time       T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha\n");
        VTR_LOG(
            "      (sec)                                          (ns)       (ns)     (ns)                                                 \n");
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
    } else {
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------ -------- -------- ---------  ---------\n");
        VTR_LOG(
            "Tnum   Time       T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha Agg. BW  Agg. Lat Lat Over. NoC Cong.\n");
        VTR_LOG(
            "      (sec)                                          (ns)       (ns)     (ns)                                                   (bps)     (ns)     (ns)             \n");
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------ -------- -------- --------- ---------\n");
    }
}

static void print_place_status(const t_annealing_state& state,
                               const t_placer_statistics& stats,
                               float elapsed_sec,
                               float cpd,
                               float sTNS,
                               float sWNS,
                               size_t tot_moves,
                               bool noc_enabled,
                               const NocCostTerms& noc_cost_terms) {
    VTR_LOG(
        "%4zu %6.1f %7.1e "
        "%7.3f %10.2f %-10.5g "
        "%7.3f % 10.3g % 8.3f "
        "%7.3f %7.4f %6.1f %8.2f",
        state.num_temps, elapsed_sec, state.t,
        stats.av_cost, stats.av_bb_cost, stats.av_timing_cost,
        1e9 * cpd, 1e9 * sTNS, 1e9 * sWNS,
        stats.success_rate, stats.std_dev, state.rlim, state.crit_exponent);

    pretty_print_uint(" ", tot_moves, 9, 3);

    VTR_LOG(" %6.3f", state.alpha);

    if (noc_enabled) {
        VTR_LOG(
            " %7.2e %7.2e"
            " %8.2e %8.2f",
            noc_cost_terms.aggregate_bandwidth, noc_cost_terms.latency,
            noc_cost_terms.latency_overrun, noc_cost_terms.congestion);
    }

    VTR_LOG("\n");
    fflush(stdout);
}

static void print_resources_utilization(const BlkLocRegistry& blk_loc_registry) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& block_locs = blk_loc_registry.block_locs();

    size_t max_block_name = 0;
    size_t max_tile_name = 0;

    //Record the resource requirement
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    std::map<t_logical_block_type_ptr, std::map<t_physical_tile_type_ptr, size_t>> num_placed_instances;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const t_pl_loc& loc = block_locs[blk_id].loc;

        t_physical_tile_type_ptr physical_tile = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
        t_logical_block_type_ptr logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        num_type_instances[logical_block]++;
        num_placed_instances[logical_block][physical_tile]++;

        max_block_name = std::max(max_block_name, logical_block->name.length());
        max_tile_name = std::max(max_tile_name, physical_tile->name.length());
    }

    VTR_LOG("\n");
    VTR_LOG("Placement resource usage:\n");
    for (const auto [logical_block_type_ptr, _] : num_type_instances) {
        for (const auto [physical_tile_type_ptr, num_instances] : num_placed_instances[logical_block_type_ptr]) {
            VTR_LOG("  %-*s implemented as %-*s: %d\n", max_block_name,
                    logical_block_type_ptr->name.c_str(), max_tile_name,
                    physical_tile_type_ptr->name.c_str(), num_instances);
        }
    }
    VTR_LOG("\n");
}

static void print_placement_swaps_stats(const t_annealing_state& state, const t_swap_stats& swap_stats) {
    size_t total_swap_attempts = swap_stats.num_swap_rejected + swap_stats.num_swap_accepted + swap_stats.num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
    float reject_rate = (float)swap_stats.num_swap_rejected / total_swap_attempts;
    float accept_rate = (float)swap_stats.num_swap_accepted / total_swap_attempts;
    float abort_rate = (float)swap_stats.num_swap_aborted / total_swap_attempts;
    VTR_LOG("Placement number of temperatures: %d\n", state.num_temps);
    VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits,
            total_swap_attempts);
    VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_accepted, 100 * accept_rate);
    VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_rejected, 100 * reject_rate);
    VTR_LOG("\tSwaps aborted: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_aborted, 100 * abort_rate);
}

static void copy_locs_to_global_state(const BlkLocRegistry& blk_loc_registry) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // the placement location variables should be unlocked before being accessed
    place_ctx.unlock_loc_vars();

    // copy the local location variables into the global state
    auto& global_blk_loc_registry = place_ctx.mutable_blk_loc_registry();
    global_blk_loc_registry = blk_loc_registry;

#ifndef NO_GRAPHICS
    // update the graphics' reference to placement location variables
    get_draw_state_vars()->set_graphics_blk_loc_registry_ref(global_blk_loc_registry);
#endif
}