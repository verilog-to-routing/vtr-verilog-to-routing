#include <cstdio>
#include <cmath>
#include <memory>
#include <fstream>
#include <iostream>
#include <numeric>
#include <chrono>
#include <optional>

#include "NetPinTimingInvalidator.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_geometry.h"
#include "vtr_time.h"
#include "vtr_math.h"
#include "vtr_ndmatrix.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"
#include "vpr_net_pins_matrix.h"

#include "globals.h"
#include "place.h"
#include "annealer.h"
#include "read_place.h"
#include "draw.h"
#include "place_and_route.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "place_macro.h"
#include "histogram.h"
#include "place_util.h"
#include "analytic_placer.h"
#include "initial_placement.h"
#include "place_delay_model.h"
#include "place_timing_update.h"
#include "move_transactions.h"
#include "move_utils.h"
#include "place_constraints.h"
#include "manual_moves.h"
#include "buttons.h"

#include "manual_move_generator.h"

#include "PlacementDelayCalculator.h"
#include "VprTimingGraphResolver.h"
#include "timing_util.h"
#include "timing_info.h"
#include "concrete_timing_info.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

#include "placer_breakpoint.h"
#include "RL_agent_util.h"
#include "place_checkpoint.h"

#include "clustered_netlist_utils.h"

#include "cluster_placement.h"

#include "noc_place_utils.h"

#include "net_cost_handler.h"
#include "placer_state.h"

/*  define the RL agent's reward function factor constant. This factor controls the weight of bb cost *
 *  compared to the timing cost in the agent's reward function. The reward is calculated as           *
 * -1*(1.5-REWARD_BB_TIMING_RELATIVE_WEIGHT)*timing_cost + (1+REWARD_BB_TIMING_RELATIVE_WEIGHT)*bb_cost)
 */
static constexpr float REWARD_BB_TIMING_RELATIVE_WEIGHT = 0.4;

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "draw_types.h"
#    include "draw_global.h"
#    include "draw_color.h"
#endif

/************** Types and defines local to place.c ***************************/
constexpr double INVALID_COST = std::numeric_limits<double>::quiet_NaN();

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

static NetCostHandler alloc_and_load_placement_structs(const t_placer_opts& placer_opts,
                                                       const t_noc_opts& noc_opts,
                                                       const std::vector<t_direct_inf>& directs,
                                                       PlacerState& placer_state,
                                                       std::optional<NocCostHandler>& noc_cost_handler);

static void free_placement_structs();

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm,
                        const t_noc_opts& noc_opts,
                        PlacerState& placer_state,
                        NetCostHandler& net_cost_handler,
                        const std::optional<NocCostHandler>& noc_cost_handler);

static int check_placement_costs(const t_placer_costs& costs,
                                 const PlaceDelayModel* delay_model,
                                 const PlacerCriticalities* criticalities,
                                 const t_place_algorithm& place_algorithm,
                                 PlacerState& placer_state,
                                 NetCostHandler& net_cost_handler);


static int check_placement_consistency(const BlkLocRegistry& blk_loc_registry);
static int check_block_placement_consistency(const BlkLocRegistry& blk_loc_registry);
static int check_macro_placement_consistency(const BlkLocRegistry& blk_loc_registry);

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
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto& timing_ctx = g_vpr_ctx.timing();
    auto pre_place_timing_stats = timing_ctx.stats;

    t_placer_costs costs(placer_opts.place_algorithm, noc_opts.noc);

    tatum::TimingPathInfo critical_path;
    float sTNS = NAN;
    float sWNS = NAN;

    char msg[vtr::bufsize];

    t_placement_checkpoint placement_checkpoint;

    std::shared_ptr<SetupTimingInfo> timing_info;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc;
    std::unique_ptr<PlaceDelayModel> place_delay_model;
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks;
    std::unique_ptr<PlacerCriticalities> placer_criticalities;
    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator;

    t_pl_blocks_to_be_moved blocks_affected(net_list.blocks().size());

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
            place_delay_model->dump_echo(
                getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    g_vpr_ctx.mutable_placement().cube_bb = is_cube_bb(placer_opts.place_bounding_box_mode, device_ctx.rr_graph);
    const bool cube_bb = g_vpr_ctx.placement().cube_bb;

    VTR_LOG("\n");
    VTR_LOG("Bounding box mode is %s\n", (cube_bb ? "Cube" : "Per-layer"));
    VTR_LOG("\n");

    int move_lim = (int)(placer_opts.anneal_sched.inner_num * pow(net_list.blocks().size(), 1.3333));

    PlacerState placer_state(placer_opts.place_algorithm.is_timing_driven(), cube_bb);
    auto& blk_loc_registry = placer_state.mutable_blk_loc_registry();
    const auto& p_timing_ctx = placer_state.timing();
    const auto& p_runtime_ctx = placer_state.runtime();

    std::optional<NocCostHandler> noc_cost_handler;
    // create cost handler objects
    NetCostHandler net_cost_handler = alloc_and_load_placement_structs(placer_opts, noc_opts, directs,
                                                                       placer_state, noc_cost_handler);

#ifndef NO_GRAPHICS
    if (noc_cost_handler.has_value()) {
        get_draw_state_vars()->set_noc_link_bandwidth_usages_ref(noc_cost_handler->get_link_bandwidth_usages());
    }
#endif

    ManualMoveGenerator manual_move_generator(placer_state);

    vtr::ScopedStartFinishTimer timer("Placement");

    if (noc_opts.noc) {
        normalize_noc_cost_weighting_factor(const_cast<t_noc_opts&>(noc_opts));
    }

    initial_placement(placer_opts, placer_opts.constraints_file.c_str(),
                      noc_opts, blk_loc_registry, noc_cost_handler);

    //create the move generator based on the chosen strategy
    auto [move_generator, move_generator2] = create_move_generators(placer_state, placer_opts, move_lim, noc_opts.noc_centroid_weight);

    if (!placer_opts.write_initial_place_file.empty()) {
        print_place(nullptr, nullptr, placer_opts.write_initial_place_file.c_str(), placer_state.block_locs());
    }

#ifdef ENABLE_ANALYTIC_PLACE
    /*
     * Analytic Placer:
     *  Passes in the initial_placement via vpr_context, and passes its placement back via locations marked on
     *  both the clb_netlist and the gird.
     *  Most of anneal is disabled later by setting initial temperature to 0 and only further optimizes in quench
     */
    if (placer_opts.enable_analytic_placer) {
        AnalyticPlacer{blk_loc_registry}.ap_place();
    }

#endif /* ENABLE_ANALYTIC_PLACE */

    // Update physical pin values
    for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
        blk_loc_registry.place_sync_external_block_connections(block_id);
    }

    const int width_fac = placer_opts.place_chan_width;
    init_draw_coords((float)width_fac, blk_loc_registry);

    /* Allocated here because it goes into timing critical code where each memory allocation is expensive */
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.logical_block_types);
    //Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, pb_gpin_lookup);

    /* Gets initial cost and loads bounding boxes. */

    if (placer_opts.place_algorithm.is_timing_driven()) {
        costs.bb_cost = net_cost_handler.comp_bb_cost(e_cost_methods::NORMAL);

        int num_connections = count_connections();
        VTR_LOG("\n");
        VTR_LOG("There are %d point to point connections in this circuit.\n",
                num_connections);
        VTR_LOG("\n");

        //Update the point-to-point delays from the initial placement
        comp_td_connection_delays(place_delay_model.get(), placer_state);

        /*
         * Initialize timing analysis
         */
        // For placement, we don't use flat-routing
        placement_delay_calc = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist,
                                                                          atom_ctx.lookup,
                                                                          p_timing_ctx.connection_delay,
                                                                          is_flat);
        placement_delay_calc->set_tsu_margin_relative(placer_opts.tsu_rel_margin);
        placement_delay_calc->set_tsu_margin_absolute(placer_opts.tsu_abs_margin);

        timing_info = make_setup_timing_info(placement_delay_calc,
                                             placer_opts.timing_update_type);

        placer_setup_slacks = std::make_unique<PlacerSetupSlacks>(cluster_ctx.clb_nlist, netlist_pin_lookup);

        placer_criticalities = std::make_unique<PlacerCriticalities>(cluster_ctx.clb_nlist, netlist_pin_lookup);

        pin_timing_invalidator = make_net_pin_timing_invalidator(
            placer_opts.timing_update_type,
            net_list,
            netlist_pin_lookup,
            atom_ctx.nlist,
            atom_ctx.lookup,
            *timing_info->timing_graph(),
            is_flat);

        //First time compute timing and costs, compute from scratch
        PlaceCritParams crit_params;
        crit_params.crit_exponent = placer_opts.td_place_exp_first;
        crit_params.crit_limit = placer_opts.place_crit_limit;

        initialize_timing_info(crit_params, place_delay_model.get(), placer_criticalities.get(),
                               placer_setup_slacks.get(), pin_timing_invalidator.get(),
                               timing_info.get(), &costs, placer_state);

        critical_path = timing_info->least_slack_critical_path();

        /* Write out the initial timing echo file */
        if (isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(
                getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                *timing_ctx.graph, *timing_ctx.constraints,
                *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);

            write_setup_timing_graph_dot(
                getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)
                    + std::string(".dot"),
                *timing_info, debug_tnode);
        }

        /* Initialize the normalization factors. Calling costs.update_norm_factors() *
         * here would fail the golden results of strong_sdc benchmark                */
        costs.timing_cost_norm = 1 / costs.timing_cost;
        costs.bb_cost_norm = 1 / costs.bb_cost;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);

        /* Total cost is the same as wirelength cost normalized*/
        costs.bb_cost = net_cost_handler.comp_bb_cost(e_cost_methods::NORMAL);
        costs.bb_cost_norm = 1 / costs.bb_cost;

        /* Timing cost and normalization factors are not used */
        costs.timing_cost = INVALID_COST;
        costs.timing_cost_norm = INVALID_COST;
    }

    if (noc_opts.noc) {
        VTR_ASSERT(noc_cost_handler.has_value());

        // get the costs associated with the NoC
        costs.noc_cost_terms.aggregate_bandwidth = noc_cost_handler->comp_noc_aggregate_bandwidth_cost();
        std::tie(costs.noc_cost_terms.latency, costs.noc_cost_terms.latency_overrun) = noc_cost_handler->comp_noc_latency_cost();
        costs.noc_cost_terms.congestion = noc_cost_handler->comp_noc_congestion_cost();

        // initialize all the noc normalization factors
        noc_cost_handler->update_noc_normalization_factors(costs);
    }

    // set the starting total placement cost
    costs.cost = costs.get_total_cost(placer_opts, noc_opts);

    //Sanity check that initial placement is legal
    check_place(costs,
                place_delay_model.get(),
                placer_criticalities.get(),
                placer_opts.place_algorithm,
                noc_opts,
                placer_state,
                net_cost_handler,
                noc_cost_handler);

    //Initial placement statistics
    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g\n", costs.cost,
            costs.bb_cost, costs.timing_cost);
    if (noc_opts.noc) {
        VTR_ASSERT(noc_cost_handler.has_value());

        noc_cost_handler->print_noc_costs("Initial NoC Placement Costs", costs, noc_opts);
    }
    if (placer_opts.place_algorithm.is_timing_driven()) {
        VTR_LOG(
            "Initial placement estimated Critical Path Delay (CPD): %g ns\n",
            1e9 * critical_path.delay());
        VTR_LOG(
            "Initial placement estimated setup Total Negative Slack (sTNS): %g ns\n",
            1e9 * timing_info->setup_total_negative_slack());
        VTR_LOG(
            "Initial placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
            1e9 * timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");

        VTR_LOG("Initial placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
    }

    size_t num_macro_members = 0;
    for (auto& macro : blk_loc_registry.place_macros().macros()) {
        num_macro_members += macro.members.size();
    }
    VTR_LOG(
        "Placement contains %zu placement macros involving %zu blocks (average macro size %f)\n",
        blk_loc_registry.place_macros().macros().size(), num_macro_members,
        float(num_macro_members) / blk_loc_registry.place_macros().macros().size());
    VTR_LOG("\n");

    sprintf(msg,
            "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);

    //Draw the initial placement
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

    //RL agent state definition
    e_agent_state agent_state = e_agent_state::EARLY_IN_THE_ANNEAL;

    //Define the timing bb weight factor for the agent's reward function
    float timing_bb_factor = REWARD_BB_TIMING_RELATIVE_WEIGHT;

    PlacementAnnealer annealer(placer_opts, placer_state, costs, net_cost_handler, noc_cost_handler,
                               noc_opts, *move_generator, *move_generator2, manual_move_generator, place_delay_model.get(),
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
                if (placer_opts.place_checkpointing && agent_state == e_agent_state::LATE_IN_THE_ANNEAL) {
                    save_placement_checkpoint_if_needed(blk_loc_registry.block_locs(),
                                                        placement_checkpoint,
                                                        timing_info, costs, critical_path.delay());
                }
            }

            // select the appropriate move generator
            MoveGenerator& current_move_generator = select_move_generator(move_generator, move_generator2,
                                                                          agent_state, placer_opts, false);

            // do a complete inner loop iteration
            annealer.placement_inner_loop(current_move_generator,
                                          timing_bb_factor);

            print_place_status(annealing_state, placer_stats, temperature_timer.elapsed_sec(),
                               critical_path.delay(), sTNS, sWNS, annealer.get_total_iteration(),
                               noc_opts.noc, costs.noc_cost_terms);

            if (placer_opts.place_algorithm.is_timing_driven()
                && placer_opts.place_agent_multistate
                && agent_state == e_agent_state::EARLY_IN_THE_ANNEAL) {
                if (annealing_state.alpha < 0.85 && annealing_state.alpha > 0.6) {
                    agent_state = e_agent_state::LATE_IN_THE_ANNEAL;
                    VTR_LOG("Agent's 2nd state: \n");
                }
            }

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

        // select the appropriate move generator
        MoveGenerator& current_move_generator = select_move_generator(move_generator, move_generator2,
                                                                      agent_state, placer_opts, true);

        /* Run inner loop again with temperature = 0 so as to accept only swaps
         * which reduce the cost of the placement */
        annealer.placement_inner_loop(current_move_generator, timing_bb_factor);

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

    // TODO:
    // 1. add some subroutine hierarchy!  Too big!

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

    print_timing_stats("Placement Quench", post_quench_timing_stats,
                       pre_quench_timing_stats);
    print_timing_stats("Placement Total ", timing_ctx.stats,
                       pre_place_timing_stats);

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

    if (place_bb_mode == AUTO_BB) {
        // If the auto_bb is used, we analyze the RR graph to see whether is there any inter-layer connection that is not
        // originated from OPIN. If there is any, cube BB is chosen, otherwise, per-layer bb is chosen.
        if (number_layers > 1 && inter_layer_connections_limited_to_opin(rr_graph)) {
            cube_bb = false;
        } else {
            cube_bb = true;
        }
    } else if (place_bb_mode == CUBE_BB) {
        // The user has specifically asked for CUBE_BB
        cube_bb = true;
    } else {
        // The user has specifically asked for PER_LAYER_BB
        VTR_ASSERT_SAFE(place_bb_mode == PER_LAYER_BB);
        cube_bb = false;
    }

    return cube_bb;
}

/* Allocates the major structures needed only by the placer, primarily for *
 * computing costs quickly and such.                                       */
static NetCostHandler alloc_and_load_placement_structs(const t_placer_opts& placer_opts,
                                                       const t_noc_opts& noc_opts,
                                                       const std::vector<t_direct_inf>& directs,
                                                       PlacerState& placer_state,
                                                       std::optional<NocCostHandler>& noc_cost_handler) {
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    place_ctx.lock_loc_vars();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    init_placement_context(placer_state.mutable_blk_loc_registry(), directs);

    int max_pins_per_clb = 0;
    for (const t_physical_tile_type& type : device_ctx.physical_tile_types) {
        max_pins_per_clb = std::max(max_pins_per_clb, type.num_pins);
    }

    place_ctx.compressed_block_grids = create_compressed_block_grids();

    if (noc_opts.noc) {
        noc_cost_handler.emplace(placer_state.block_locs());
    }

    return NetCostHandler{placer_opts, placer_state, num_nets, place_ctx.cube_bb};
}

/* Frees the major structures needed by the placer (and not needed       *
 * elsewhere).   */
static void free_placement_structs() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    vtr::release_memory(place_ctx.compressed_block_grids);
}

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm,
                        const t_noc_opts& noc_opts,
                        PlacerState& placer_state,
                        NetCostHandler& net_cost_handler,
                        const std::optional<NocCostHandler>& noc_cost_handler) {
    /* Checks that the placement has not confused our data structures. *
     * i.e. the clb and block structures agree about the locations of  *
     * every block, blocks are in legal spots, etc.  Also recomputes   *
     * the final placement cost from scratch and makes sure it is      *
     * within roundoff of what we think the cost is.                   */

    int error = 0;

    error += check_placement_consistency(placer_state.blk_loc_registry());
    error += check_placement_costs(costs, delay_model, criticalities, place_algorithm, placer_state, net_cost_handler);
    error += check_placement_floorplanning(placer_state.block_locs());

    if (noc_opts.noc) {
        // check the NoC costs during placement if the user is using the NoC supported flow
        error += noc_cost_handler->check_noc_placement_costs(costs, PL_INCREMENTAL_COST_TOLERANCE, noc_opts);
        // make sure NoC routing configuration does not create any cycles in CDG
        error += (int)noc_cost_handler->noc_routing_has_cycle();
    }

    if (error == 0) {
        VTR_LOG("\n");
        VTR_LOG("Completed placement consistency check successfully.\n");

    } else {
        VPR_ERROR(VPR_ERROR_PLACE,
                  "\nCompleted placement consistency check, %d errors found.\n"
                  "Aborting program.\n",
                  error);
    }
}

static int check_placement_costs(const t_placer_costs& costs,
                                 const PlaceDelayModel* delay_model,
                                 const PlacerCriticalities* criticalities,
                                 const t_place_algorithm& place_algorithm,
                                 PlacerState& placer_state,
                                 NetCostHandler& net_cost_handler) {
    int error = 0;
    double timing_cost_check;

    double bb_cost_check = net_cost_handler.comp_bb_cost(e_cost_methods::CHECK);

    if (fabs(bb_cost_check - costs.bb_cost) > costs.bb_cost * PL_INCREMENTAL_COST_TOLERANCE) {
        VTR_LOG_ERROR(
            "bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
            bb_cost_check, costs.bb_cost);
        error++;
    }

    if (place_algorithm.is_timing_driven()) {
        comp_td_costs(delay_model, *criticalities, placer_state, &timing_cost_check);
        //VTR_LOG("timing_cost recomputed from scratch: %g\n", timing_cost_check);
        if (fabs(timing_cost_check - costs.timing_cost) > costs.timing_cost * PL_INCREMENTAL_COST_TOLERANCE) {
            VTR_LOG_ERROR(
                "timing_cost_check: %g and timing_cost: %g differ in check_place.\n",
                timing_cost_check, costs.timing_cost);
            error++;
        }
    }
    return error;
}

static int check_placement_consistency(const BlkLocRegistry& blk_loc_registry) {
    return check_block_placement_consistency(blk_loc_registry) + check_macro_placement_consistency(blk_loc_registry);
}

static int check_block_placement_consistency(const BlkLocRegistry& blk_loc_registry) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& block_locs = blk_loc_registry.block_locs();
    const auto& grid_blocks = blk_loc_registry.grid_blocks();

    int error = 0;

    vtr::vector<ClusterBlockId, int> bdone(cluster_ctx.clb_nlist.blocks().size(), 0);

    /* Step through device grid and placement. Check it against blocks */
    for (int layer_num = 0; layer_num < (int)device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                const t_physical_tile_loc tile_loc(i, j, layer_num);
                const auto& type = device_ctx.grid.get_physical_type(tile_loc);
                if (grid_blocks.get_usage(tile_loc) > type->capacity) {
                    VTR_LOG_ERROR(
                        "%d blocks were placed at grid location (%d,%d,%d), but location capacity is %d.\n",
                        grid_blocks.get_usage(tile_loc), i, j, layer_num, type->capacity);
                    error++;
                }
                int usage_check = 0;
                for (int k = 0; k < type->capacity; k++) {
                    ClusterBlockId bnum = grid_blocks.block_at_location({i, j, k, layer_num});
                    if (bnum == ClusterBlockId::INVALID()) {
                        continue;
                    }

                    auto logical_block = cluster_ctx.clb_nlist.block_type(bnum);
                    auto physical_tile = type;
                    t_pl_loc block_loc = block_locs[bnum].loc;

                    if (physical_tile_type(block_loc) != physical_tile) {
                        VTR_LOG_ERROR(
                            "Block %zu type (%s) does not match grid location (%zu,%zu, %d) type (%s).\n",
                            size_t(bnum), logical_block->name, i, j, layer_num, physical_tile->name);
                        error++;
                    }

                    auto& loc = block_locs[bnum].loc;
                    if (loc.x != i || loc.y != j || loc.layer != layer_num
                        || !is_sub_tile_compatible(physical_tile, logical_block,
                                                   loc.sub_tile)) {
                        VTR_LOG_ERROR(
                            "Block %zu's location is (%d,%d,%d,%d) but found in grid at (%d,%d,%d,%d).\n",
                            size_t(bnum),
                            loc.x,
                            loc.y,
                            loc.sub_tile,
                            loc.layer,
                            i,
                            j,
                            k,
                            layer_num);
                        error++;
                    }
                    ++usage_check;
                    bdone[bnum]++;
                }
                if (usage_check != grid_blocks.get_usage(tile_loc)) {
                    VTR_LOG_ERROR(
                        "%d block(s) were placed at location (%d,%d,%d), but location contains %d block(s).\n",
                        grid_blocks.get_usage(tile_loc),
                        tile_loc.x,
                        tile_loc.y,
                        tile_loc.layer_num,
                        usage_check);
                    error++;
                }
            }
        }
    }

    /* Check that every block exists in the device_ctx.grid and cluster_ctx.blocks arrays somewhere. */
    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks())
        if (bdone[blk_id] != 1) {
            VTR_LOG_ERROR("Block %zu listed %d times in device context grid.\n",
                          size_t(blk_id), bdone[blk_id]);
            error++;
        }

    return error;
}

int check_macro_placement_consistency(const BlkLocRegistry& blk_loc_registry) {
    const auto& pl_macros = blk_loc_registry.place_macros().macros();
    const auto& block_locs = blk_loc_registry.block_locs();
    const auto& grid_blocks = blk_loc_registry.grid_blocks();

    int error = 0;

    /* Check the pl_macro placement are legal - blocks are in the proper relative position. */
    for (size_t imacro = 0; imacro < pl_macros.size(); imacro++) {
        auto head_iblk = pl_macros[imacro].members[0].blk_index;

        for (size_t imember = 0; imember < pl_macros[imacro].members.size(); imember++) {
            auto member_iblk = pl_macros[imacro].members[imember].blk_index;

            // Compute the supposed member's x,y,z location
            t_pl_loc member_pos = block_locs[head_iblk].loc + pl_macros[imacro].members[imember].offset;

            // Check the blk_loc_registry.block_locs data structure first
            if (block_locs[member_iblk].loc != member_pos) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                error++;
            }

            // Then check the blk_loc_registry.grid data structure
            if (grid_blocks.block_at_location(member_pos) != member_iblk) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                error++;
            }
        } // Finish going through all the members
    } // Finish going through all the macros

    return error;
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
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

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

    int max_block_name = 0;
    int max_tile_name = 0;

    //Record the resource requirement
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    std::map<t_logical_block_type_ptr, std::map<t_physical_tile_type_ptr, size_t>> num_placed_instances;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const t_pl_loc& loc = block_locs[blk_id].loc;

        auto physical_tile = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        num_type_instances[logical_block]++;
        num_placed_instances[logical_block][physical_tile]++;

        max_block_name = std::max<int>(max_block_name, strlen(logical_block->name));
        max_tile_name = std::max<int>(max_tile_name, strlen(physical_tile->name));
    }

    VTR_LOG("\n");
    VTR_LOG("Placement resource usage:\n");
    for (const auto [logical_block_type_ptr, _] : num_type_instances) {
        for (const auto [physical_tile_type_ptr, num_instances] : num_placed_instances[logical_block_type_ptr]) {
            VTR_LOG("  %-*s implemented as %-*s: %d\n", max_block_name,
                    logical_block_type_ptr->name, max_tile_name,
                    physical_tile_type_ptr->name, num_instances);
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
