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
#include "placer_globals.h"
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
#include "read_place.h"
#include "place_constraints.h"
#include "manual_moves.h"
#include "buttons.h"

#include "static_move_generator.h"
#include "simpleRL_move_generator.h"
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

using std::max;
using std::min;

/************** Types and defines local to place.c ***************************/

/* This defines the error tolerance for floating points variables used in *
 * cost computation. 0.01 means that there is a 1% error tolerance.       */
static constexpr double ERROR_TOL = .01;

/* This defines the maximum number of swap attempts before invoking the   *
 * once-in-a-while placement legality check as well as floating point     *
 * variables round-offs check.                                            */
static constexpr int MAX_MOVES_BEFORE_RECOMPUTE = 500000;

constexpr float INVALID_DELAY = std::numeric_limits<float>::quiet_NaN();
constexpr float INVALID_COST = std::numeric_limits<double>::quiet_NaN();

/********************** Variables local to place.c ***************************/

/* These file-scoped variables keep track of the number of swaps       *
 * rejected, accepted or aborted. The total number of swap attempts    *
 * is the sum of the three number.                                     */
static int num_swap_rejected = 0;
static int num_swap_accepted = 0;
static int num_swap_aborted = 0;
static int num_ts_called = 0;

std::unique_ptr<FILE, decltype(&vtr::fclose)> f_move_stats_file(nullptr,
                                                                vtr::fclose);

#ifdef VTR_ENABLE_DEBUG_LOGGIING
#    define LOG_MOVE_STATS_HEADER()                               \
        do {                                                      \
            if (f_move_stats_file) {                              \
                fprintf(f_move_stats_file.get(),                  \
                        "temp,from_blk,to_blk,from_type,to_type," \
                        "blk_count,"                              \
                        "delta_cost,delta_bb_cost,delta_td_cost," \
                        "outcome,reason\n");                      \
            }                                                     \
        } while (false)

#    define LOG_MOVE_STATS_PROPOSED(t, affected_blocks)                                        \
        do {                                                                                   \
            if (f_move_stats_file) {                                                           \
                auto& place_ctx = g_vpr_ctx.placement();                                       \
                auto& cluster_ctx = g_vpr_ctx.clustering();                                    \
                ClusterBlockId b_from = affected_blocks.moved_blocks[0].block_num;             \
                                                                                               \
                t_pl_loc to = affected_blocks.moved_blocks[0].new_loc;                         \
                ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.sub_tile];   \
                                                                                               \
                t_logical_block_type_ptr from_type = cluster_ctx.clb_nlist.block_type(b_from); \
                t_logical_block_type_ptr to_type = nullptr;                                    \
                if (b_to) {                                                                    \
                    to_type = cluster_ctx.clb_nlist.block_type(b_to);                          \
                }                                                                              \
                                                                                               \
                fprintf(f_move_stats_file.get(),                                               \
                        "%g,"                                                                  \
                        "%d,%d,"                                                               \
                        "%s,%s,"                                                               \
                        "%d,",                                                                 \
                        t,                                                                     \
                        int(b_from), int(b_to),                                                \
                        from_type->name, (to_type ? to_type->name : "EMPTY"),                  \
                        affected_blocks.num_moved_blocks);                                     \
            }                                                                                  \
        } while (false)

#    define LOG_MOVE_STATS_OUTCOME(delta_cost, delta_bb_cost, delta_td_cost, \
                                   outcome, reason)                          \
        do {                                                                 \
            if (f_move_stats_file) {                                         \
                fprintf(f_move_stats_file.get(),                             \
                        "%g,%g,%g,"                                          \
                        "%s,%s\n",                                           \
                        delta_cost, delta_bb_cost, delta_td_cost,            \
                        outcome, reason);                                    \
            }                                                                \
        } while (false)

#else

#    define LOG_MOVE_STATS_HEADER()                      \
        do {                                             \
            fprintf(f_move_stats_file.get(),             \
                    "VTR_ENABLE_DEBUG_LOGGING disabled " \
                    "-- No move stats recorded\n");      \
        } while (false)

#    define LOG_MOVE_STATS_PROPOSED(t, blocks_affected) \
        do {                                            \
        } while (false)

#    define LOG_MOVE_STATS_OUTCOME(delta_cost, delta_bb_cost, delta_td_cost, \
                                   outcome, reason)                          \
        do {                                                                 \
        } while (false)

#endif

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

static void alloc_and_load_placement_structs(float place_cost_exp,
                                             const t_placer_opts& placer_opts,
                                             const t_noc_opts& noc_opts,
                                             t_direct_inf* directs,
                                             int num_directs);

static void alloc_and_load_try_swap_structs(const bool cube_bb);
static void free_try_swap_structs();

static void free_placement_structs(const t_placer_opts& placer_opts, const t_noc_opts& noc_opts);

static e_move_result try_swap(const t_annealing_state* state,
                              t_placer_costs* costs,
                              MoveGenerator& move_generator,
                              ManualMoveGenerator& manual_move_generator,
                              SetupTimingInfo* timing_info,
                              NetPinTimingInvalidator* pin_timing_invalidator,
                              t_pl_blocks_to_be_moved& blocks_affected,
                              const PlaceDelayModel* delay_model,
                              PlacerCriticalities* criticalities,
                              PlacerSetupSlacks* setup_slacks,
                              const t_placer_opts& placer_opts,
                              const t_noc_opts& noc_opts,
                              MoveTypeStat& move_type_stat,
                              const t_place_algorithm& place_algorithm,
                              float timing_bb_factor,
                              bool manual_move_enabled);

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm,
                        const t_noc_opts& noc_opts);

static int check_placement_costs(const t_placer_costs& costs,
                                 const PlaceDelayModel* delay_model,
                                 const PlacerCriticalities* criticalities,
                                 const t_place_algorithm& place_algorithm);

static int check_placement_consistency();
static int check_block_placement_consistency();
static int check_macro_placement_consistency();

static float starting_t(const t_annealing_state* state,
                        t_placer_costs* costs,
                        t_annealing_sched annealing_sched,
                        const PlaceDelayModel* delay_model,
                        PlacerCriticalities* criticalities,
                        PlacerSetupSlacks* setup_slacks,
                        SetupTimingInfo* timing_info,
                        MoveGenerator& move_generator,
                        ManualMoveGenerator& manual_move_generator,
                        NetPinTimingInvalidator* pin_timing_invalidator,
                        t_pl_blocks_to_be_moved& blocks_affected,
                        const t_placer_opts& placer_opts,
                        const t_noc_opts& noc_opts,
                        MoveTypeStat& move_type_stat);

static int count_connections();

static void commit_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

static void revert_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

static void invalidate_affected_connections(
    const t_pl_blocks_to_be_moved& blocks_affected,
    NetPinTimingInvalidator* pin_tedges_invalidator,
    TimingInfo* timing_info);

static float analyze_setup_slack_cost(const PlacerSetupSlacks* setup_slacks);

static e_move_result assess_swap(double delta_c, double t);

static void update_placement_cost_normalization_factors(t_placer_costs* costs, const t_placer_opts& placer_opts, const t_noc_opts& noc_opts);

static double get_total_cost(t_placer_costs* costs, const t_placer_opts& placer_opts, const t_noc_opts& noc_opts);

static void free_try_swap_arrays();

static void outer_loop_update_timing_info(const t_placer_opts& placer_opts,
                                          const t_noc_opts& noc_opts,
                                          t_placer_costs* costs,
                                          int num_connections,
                                          float crit_exponent,
                                          int* outer_crit_iter_count,
                                          const PlaceDelayModel* delay_model,
                                          PlacerCriticalities* criticalities,
                                          PlacerSetupSlacks* setup_slacks,
                                          NetPinTimingInvalidator* pin_timing_invalidator,
                                          SetupTimingInfo* timing_info);

static void placement_inner_loop(const t_annealing_state* state,
                                 const t_placer_opts& placer_opts,
                                 const t_noc_opts& noc_opts,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 int* moves_since_cost_recompute,
                                 NetPinTimingInvalidator* pin_timing_invalidator,
                                 const PlaceDelayModel* delay_model,
                                 PlacerCriticalities* criticalities,
                                 PlacerSetupSlacks* setup_slacks,
                                 MoveGenerator& move_generator,
                                 ManualMoveGenerator& manual_move_generator,
                                 t_pl_blocks_to_be_moved& blocks_affected,
                                 SetupTimingInfo* timing_info,
                                 const t_place_algorithm& place_algorithm,
                                 MoveTypeStat& move_type_stat,
                                 float timing_bb_factor);

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat);

//calculate the agent's reward and the total process outcome
static void calculate_reward_and_process_outcome(
    const t_placer_opts& placer_opts,
    const MoveOutcomeStats& move_outcome_stats,
    const double& delta_c,
    float timing_bb_factor,
    MoveGenerator& move_generator);

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

static void print_resources_utilization();

static void print_placement_swaps_stats(const t_annealing_state& state);

static void print_placement_move_types_stats(const MoveTypeStat& move_type_stat);

/*****************************************************************************/
void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               t_annealing_sched annealing_sched,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_direct_inf* directs,
               int num_directs,
               bool is_flat) {
    /* Does almost all the work of placing a circuit.  Width_fac gives the   *
     * width of the widest channel.  Place_cost_exp says what exponent the   *
     * width should be taken to when calculating costs.  This allows a       *
     * greater bias for anisotropic architectures.                           */

    /*
     * Currently, the functions that require is_flat as their parameter and are called during placement should
     * receive is_flat as false. For example, if the RR graph of router lookahead is built here, it should be as
     * if is_flat is false, even if is_flat is set to true from the command line.
     */
    VTR_ASSERT(!is_flat);
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    const auto& p_timing_ctx = g_placer_ctx.timing();
    const auto& p_runtime_ctx = g_placer_ctx.runtime();

    auto& timing_ctx = g_vpr_ctx.timing();
    auto pre_place_timing_stats = timing_ctx.stats;
    int tot_iter, moves_since_cost_recompute, num_connections,
        outer_crit_iter_count, inner_recompute_limit;
    float first_crit_exponent, first_rlim, first_t;
    int first_move_lim;

    t_placer_costs costs(placer_opts.place_algorithm);

    tatum::TimingPathInfo critical_path;
    float sTNS = NAN;
    float sWNS = NAN;

    char msg[vtr::bufsize];
    t_placer_statistics stats;

    t_placement_checkpoint placement_checkpoint;

    std::shared_ptr<SetupTimingInfo> timing_info;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc;
    std::unique_ptr<PlaceDelayModel> place_delay_model;
    std::unique_ptr<MoveGenerator> move_generator;
    std::unique_ptr<MoveGenerator> move_generator2;
    std::unique_ptr<ManualMoveGenerator> manual_move_generator;
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks;

    std::unique_ptr<PlacerCriticalities> placer_criticalities;
    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator;

    manual_move_generator = std::make_unique<ManualMoveGenerator>();

    t_pl_blocks_to_be_moved blocks_affected(
        net_list.blocks().size());

    /* init file scope variables */
    num_swap_rejected = 0;
    num_swap_accepted = 0;
    num_swap_aborted = 0;
    num_ts_called = 0;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = alloc_lookups_and_delay_model(net_list,
                                                          chan_width_dist,
                                                          placer_opts,
                                                          router_opts,
                                                          det_routing_arch,
                                                          segment_inf,
                                                          directs,
                                                          num_directs,
                                                          is_flat);

        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(
                getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    g_vpr_ctx.mutable_placement().cube_bb = is_cube_bb(placer_opts.place_bounding_box_mode,
                                                       device_ctx.rr_graph);
    const auto& cube_bb = g_vpr_ctx.placement().cube_bb;

    VTR_LOG("\n");
    VTR_LOG("Bounding box mode is %s\n", (cube_bb ? "Cube" : "Per-layer"));
    VTR_LOG("\n");

    int move_lim = 1;
    move_lim = (int)(annealing_sched.inner_num
                     * pow(net_list.blocks().size(), 1.3333));

    alloc_and_load_placement_structs(placer_opts.place_cost_exp, placer_opts, noc_opts, directs, num_directs);

    vtr::ScopedStartFinishTimer timer("Placement");

    if (noc_opts.noc) {
        normalize_noc_cost_weighting_factor(const_cast<t_noc_opts&>(noc_opts));
    }

    initial_placement(placer_opts,
                      placer_opts.constraints_file.c_str(),
                      noc_opts);

    //create the move generator based on the chosen strategy
    create_move_generators(move_generator, move_generator2, placer_opts, move_lim, noc_opts.noc_centroid_weight);

    if (!placer_opts.write_initial_place_file.empty()) {
        print_place(nullptr,
                    nullptr,
                    (placer_opts.write_initial_place_file + ".init.place").c_str());
    }

#ifdef ENABLE_ANALYTIC_PLACE
    /*
     * Analytic Placer:
     *  Passes in the initial_placement via vpr_context, and passes its placement back via locations marked on
     *  both the clb_netlist and the gird.
     *  Most of anneal is disabled later by setting initial temperature to 0 and only further optimizes in quench
     */
    if (placer_opts.enable_analytic_placer) {
        AnalyticPlacer{}.ap_place();
    }

#endif /* ENABLE_ANALYTIC_PLACE */

    // Update physical pin values
    for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
        place_sync_external_block_connections(block_id);
    }

    const int width_fac = placer_opts.place_chan_width;
    init_draw_coords((float)width_fac);

    /* Allocated here because it goes into timing critical code where each memory allocation is expensive */
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.logical_block_types);
    //Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist,
                                                  atom_ctx.nlist, pb_gpin_lookup);

    /* Gets initial cost and loads bounding boxes. */

    if (placer_opts.place_algorithm.is_timing_driven()) {
        if (cube_bb) {
            costs.bb_cost = comp_bb_cost(NORMAL);
        } else {
            VTR_ASSERT_SAFE(!cube_bb);
            costs.bb_cost = comp_layer_bb_cost(NORMAL);
        }

        first_crit_exponent = placer_opts.td_place_exp_first; /*this will be modified when rlim starts to change */

        num_connections = count_connections();
        VTR_LOG("\n");
        VTR_LOG("There are %d point to point connections in this circuit.\n",
                num_connections);
        VTR_LOG("\n");

        //Update the point-to-point delays from the initial placement
        comp_td_connection_delays(place_delay_model.get());

        /*
         * Initialize timing analysis
         */
        // For placement, we don't use flat-routing
        placement_delay_calc = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist,
                                                                          atom_ctx.lookup,
                                                                          p_timing_ctx.connection_delay,
                                                                          is_flat);
        placement_delay_calc->set_tsu_margin_relative(
            placer_opts.tsu_rel_margin);
        placement_delay_calc->set_tsu_margin_absolute(
            placer_opts.tsu_abs_margin);

        timing_info = make_setup_timing_info(placement_delay_calc,
                                             placer_opts.timing_update_type);

        placer_setup_slacks = std::make_unique<PlacerSetupSlacks>(
            cluster_ctx.clb_nlist, netlist_pin_lookup);

        placer_criticalities = std::make_unique<PlacerCriticalities>(
            cluster_ctx.clb_nlist, netlist_pin_lookup);

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
        crit_params.crit_exponent = first_crit_exponent;
        crit_params.crit_limit = placer_opts.place_crit_limit;

        initialize_timing_info(crit_params, place_delay_model.get(),
                               placer_criticalities.get(), placer_setup_slacks.get(),
                               pin_timing_invalidator.get(), timing_info.get(), &costs);

        critical_path = timing_info->least_slack_critical_path();

        /* Write out the initial timing echo file */
        if (isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(
                getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                *timing_ctx.graph, *timing_ctx.constraints,
                *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(
                analysis_opts.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(
                getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)
                    + std::string(".dot"),
                *timing_info, debug_tnode);
        }

        outer_crit_iter_count = 1;

        /* Initialize the normalization factors. Calling costs.update_norm_factors() *
         * here would fail the golden results of strong_sdc benchmark                */
        costs.timing_cost_norm = 1 / costs.timing_cost;
        costs.bb_cost_norm = 1 / costs.bb_cost;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);

        /* Total cost is the same as wirelength cost normalized*/
        if (cube_bb) {
            costs.bb_cost = comp_bb_cost(NORMAL);
        } else {
            VTR_ASSERT_SAFE(!cube_bb);
            costs.bb_cost = comp_layer_bb_cost(NORMAL);
        }
        costs.bb_cost_norm = 1 / costs.bb_cost;

        /* Timing cost and normalization factors are not used */
        costs.timing_cost = INVALID_COST;
        costs.timing_cost_norm = INVALID_COST;

        /* Other initializations */
        outer_crit_iter_count = 0;
        num_connections = 0;
        first_crit_exponent = 0;
    }

    if (noc_opts.noc) {
        // get the costs associated with the NoC
        costs.noc_cost_terms.aggregate_bandwidth = comp_noc_aggregate_bandwidth_cost();
        std::tie(costs.noc_cost_terms.latency, costs.noc_cost_terms.latency_overrun) = comp_noc_latency_cost();
        costs.noc_cost_terms.congestion = comp_noc_congestion_cost();

        // initialize all the noc normalization factors
        update_noc_normalization_factors(costs);
    }

    // set the starting total placement cost
    costs.cost = get_total_cost(&costs, placer_opts, noc_opts);

    //Sanity check that initial placement is legal
    check_place(costs,
                place_delay_model.get(),
                placer_criticalities.get(),
                placer_opts.place_algorithm,
                noc_opts);

    //Initial placement statistics
    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g\n", costs.cost,
            costs.bb_cost, costs.timing_cost);
    if (noc_opts.noc) {
        print_noc_costs("Initial NoC Placement Costs", costs, noc_opts);
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
        print_histogram(
            create_setup_slack_histogram(*timing_info->setup_analyzer()));
    }

    size_t num_macro_members = 0;
    for (auto& macro : g_vpr_ctx.placement().pl_macros) {
        num_macro_members += macro.members.size();
    }
    VTR_LOG(
        "Placement contains %zu placement macros involving %zu blocks (average macro size %f)\n",
        g_vpr_ctx.placement().pl_macros.size(), num_macro_members,
        float(num_macro_members) / g_vpr_ctx.placement().pl_macros.size());
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
        print_place(nullptr, nullptr, filename.c_str());
    }

    first_move_lim = get_initial_move_lim(placer_opts, annealing_sched);

    if (placer_opts.inner_loop_recompute_divider != 0) {
        inner_recompute_limit = (int)(0.5
                                      + (float)first_move_lim
                                            / (float)placer_opts.inner_loop_recompute_divider);
    } else {
        /*don't do an inner recompute */
        inner_recompute_limit = first_move_lim + 1;
    }

    /* calculate the number of moves in the quench that we should recompute timing after based on the value of *
     * the commandline option quench_recompute_divider                                                         */
    int quench_recompute_limit;
    if (placer_opts.quench_recompute_divider != 0) {
        quench_recompute_limit = (int)(0.5
                                       + (float)move_lim
                                             / (float)placer_opts.quench_recompute_divider);
    } else {
        /*don't do an quench recompute */
        quench_recompute_limit = first_move_lim + 1;
    }

    //allocate helper vectors that are used by many move generators
    place_move_ctx.X_coord.resize(10, 0);
    place_move_ctx.Y_coord.resize(10, 0);
    place_move_ctx.layer_coord.resize(10, 0);

    //allocate move type statistics vectors
    MoveTypeStat move_type_stat;
    move_type_stat.blk_type_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);
    move_type_stat.accepted_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);
    move_type_stat.rejected_moves.resize({device_ctx.logical_block_types.size(), (int)e_move_type::NUMBER_OF_AUTO_MOVES}, 0);

    /* Get the first range limiter */
    first_rlim = (float)max(device_ctx.grid.width() - 1,
                            device_ctx.grid.height() - 1);
    place_move_ctx.first_rlim = first_rlim;

    /* Set the temperature low to ensure that initial placement quality will be preserved */
    first_t = EPSILON;

    t_annealing_state state(annealing_sched,
                            first_t,
                            first_rlim,
                            first_move_lim,
                            first_crit_exponent,
                            device_ctx.grid.get_num_layers());

    /* Update the starting temperature for placement annealing to a more appropriate value */
    state.t = starting_t(&state, &costs, annealing_sched,
                         place_delay_model.get(), placer_criticalities.get(),
                         placer_setup_slacks.get(), timing_info.get(), *move_generator,
                         *manual_move_generator, pin_timing_invalidator.get(),
                         blocks_affected, placer_opts, noc_opts, move_type_stat);

    if (!placer_opts.move_stats_file.empty()) {
        f_move_stats_file = std::unique_ptr<FILE, decltype(&vtr::fclose)>(
            vtr::fopen(placer_opts.move_stats_file.c_str(), "w"),
            vtr::fclose);
        LOG_MOVE_STATS_HEADER();
    }

    tot_iter = 0;
    moves_since_cost_recompute = 0;

    bool skip_anneal = false;

#ifdef ENABLE_ANALYTIC_PLACE
    // Analytic placer: When enabled, skip most of the annealing and go straight to quench
    // TODO: refactor goto label.
    if (placer_opts.enable_analytic_placer)
        skip_anneal = true;
#endif /* ENABLE_ANALYTIC_PLACE */

    //RL agent state definition
    e_agent_state agent_state = e_agent_state::EARLY_IN_THE_ANNEAL;

    std::unique_ptr<MoveGenerator> current_move_generator;

    //Define the timing bb weight factor for the agent's reward function
    float timing_bb_factor = REWARD_BB_TIMING_RELATIVE_WEIGHT;

    if (skip_anneal == false) {
        //Table header
        VTR_LOG("\n");
        print_place_status_header(noc_opts.noc);

        /* Outer loop of the simulated annealing begins */
        do {
            vtr::Timer temperature_timer;

            outer_loop_update_timing_info(placer_opts, noc_opts, &costs, num_connections,
                                          state.crit_exponent, &outer_crit_iter_count,
                                          place_delay_model.get(), placer_criticalities.get(),
                                          placer_setup_slacks.get(), pin_timing_invalidator.get(),
                                          timing_info.get());

            if (placer_opts.place_algorithm.is_timing_driven()) {
                critical_path = timing_info->least_slack_critical_path();
                sTNS = timing_info->setup_total_negative_slack();
                sWNS = timing_info->setup_worst_negative_slack();

                //see if we should save the current placement solution as a checkpoint

                if (placer_opts.place_checkpointing
                    && agent_state == e_agent_state::LATE_IN_THE_ANNEAL) {
                    save_placement_checkpoint_if_needed(placement_checkpoint,
                                                        timing_info, costs, critical_path.delay());
                }
            }

            //move the appropriate move_generator to be the current used move generator
            assign_current_move_generator(move_generator, move_generator2,
                                          agent_state, placer_opts, false, current_move_generator);

            //do a complete inner loop iteration
            placement_inner_loop(&state, placer_opts, noc_opts,
                                 inner_recompute_limit,
                                 &stats, &costs, &moves_since_cost_recompute,
                                 pin_timing_invalidator.get(), place_delay_model.get(),
                                 placer_criticalities.get(), placer_setup_slacks.get(),
                                 *current_move_generator, *manual_move_generator,
                                 blocks_affected, timing_info.get(),
                                 placer_opts.place_algorithm, move_type_stat,
                                 timing_bb_factor);

            //move the update used move_generator to its original variable
            update_move_generator(move_generator, move_generator2, agent_state,
                                  placer_opts, false, current_move_generator);

            tot_iter += state.move_lim;
            ++state.num_temps;

            print_place_status(state, stats, temperature_timer.elapsed_sec(),
                               critical_path.delay(), sTNS, sWNS, tot_iter,
                               noc_opts.noc, costs.noc_cost_terms);

            if (placer_opts.place_algorithm.is_timing_driven()
                && placer_opts.place_agent_multistate
                && agent_state == e_agent_state::EARLY_IN_THE_ANNEAL) {
                if (state.alpha < 0.85 && state.alpha > 0.6) {
                    agent_state = e_agent_state::LATE_IN_THE_ANNEAL;
                    VTR_LOG("Agent's 2nd state: \n");
                }
            }

            sprintf(msg, "Cost: %g  BB Cost %g  TD Cost %g  Temperature: %g",
                    costs.cost, costs.bb_cost, costs.timing_cost, state.t);
            update_screen(ScreenUpdatePriority::MINOR, msg, PLACEMENT,
                          timing_info);

            //#ifdef VERBOSE
            //            if (getEchoEnabled()) {
            //                print_clb_placement("first_iteration_clb_placement.echo");
            //            }
            //#endif
        } while (state.outer_loop_update(stats.success_rate, costs, placer_opts,
                                         annealing_sched));
        /* Outer loop of the simulated annealing ends */
    } //skip_anneal ends

    /* Start Quench */
    state.t = 0;                         //Freeze out: only accept solutions that improve placement.
    state.move_lim = state.move_lim_max; //Revert the move limit to initial value.

    auto pre_quench_timing_stats = timing_ctx.stats;
    { /* Quench */

        vtr::ScopedFinishTimer temperature_timer("Placement Quench");

        outer_loop_update_timing_info(placer_opts, noc_opts, &costs, num_connections,
                                      state.crit_exponent, &outer_crit_iter_count,
                                      place_delay_model.get(), placer_criticalities.get(),
                                      placer_setup_slacks.get(), pin_timing_invalidator.get(),
                                      timing_info.get());

        //move the appropriate move_generator to be the current used move generator
        assign_current_move_generator(move_generator, move_generator2,
                                      agent_state, placer_opts, true, current_move_generator);

        /* Run inner loop again with temperature = 0 so as to accept only swaps
         * which reduce the cost of the placement */
        placement_inner_loop(&state, placer_opts, noc_opts,
                             quench_recompute_limit,
                             &stats, &costs, &moves_since_cost_recompute,
                             pin_timing_invalidator.get(), place_delay_model.get(),
                             placer_criticalities.get(), placer_setup_slacks.get(),
                             *current_move_generator, *manual_move_generator,
                             blocks_affected, timing_info.get(),
                             placer_opts.place_quench_algorithm, move_type_stat,
                             timing_bb_factor);

        //move the update used move_generator to its original variable
        update_move_generator(move_generator, move_generator2, agent_state,
                              placer_opts, true, current_move_generator);

        tot_iter += state.move_lim;
        ++state.num_temps;

        if (placer_opts.place_quench_algorithm.is_timing_driven()) {
            critical_path = timing_info->least_slack_critical_path();
            sTNS = timing_info->setup_total_negative_slack();
            sWNS = timing_info->setup_worst_negative_slack();
        }

        print_place_status(state, stats, temperature_timer.elapsed_sec(),
                           critical_path.delay(), sTNS, sWNS, tot_iter,
                           noc_opts.noc, costs.noc_cost_terms);
    }
    auto post_quench_timing_stats = timing_ctx.stats;

    //Final timing analysis
    PlaceCritParams crit_params;
    crit_params.crit_exponent = state.crit_exponent;
    crit_params.crit_limit = placer_opts.place_crit_limit;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        perform_full_timing_update(crit_params, place_delay_model.get(),
                                   placer_criticalities.get(), placer_setup_slacks.get(),
                                   pin_timing_invalidator.get(), timing_info.get(), &costs);
        VTR_LOG("post-quench CPD = %g (ns) \n",
                1e9 * timing_info->least_slack_critical_path().delay());
    }

    //See if our latest checkpoint is better than the current placement solution
    if (placer_opts.place_checkpointing)
        restore_best_placement(placement_checkpoint, timing_info, costs,
                               placer_criticalities, placer_setup_slacks, place_delay_model,
                               pin_timing_invalidator, crit_params, noc_opts);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
                                               state.num_temps + 1, 0);
        VTR_LOG("Saving final placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str());
    }

    // TODO:
    // 1. add some subroutine hierarchy!  Too big!

    //#ifdef VERBOSE
    //    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_END_CLB_PLACEMENT)) {
    //        print_clb_placement(getEchoFileName(E_ECHO_END_CLB_PLACEMENT));
    //    }
    //#endif

    // Update physical pin values
    for (auto block_id : cluster_ctx.clb_nlist.blocks()) {
        place_sync_external_block_connections(block_id);
    }

    check_place(costs,
                place_delay_model.get(),
                placer_criticalities.get(),
                placer_opts.place_algorithm,
                noc_opts);

    //Some stats
    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", num_ts_called);
    report_aborted_moves();

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

        generate_post_place_timing_reports(placer_opts, analysis_opts,
                                           *timing_info, *placement_delay_calc, is_flat);

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
        print_noc_costs("\nNoC Placement Costs", costs, noc_opts);

#ifdef ENABLE_NOC_SAT_ROUTING
        if (costs.noc_cost_terms.congestion > 0.0) {
            VTR_LOG("NoC routing configuration is congested. Invoking the SAT NoC router.\n");
            invoke_sat_router(costs, noc_opts, placer_opts.seed);
        }
#endif //ENABLE_NOC_SAT_ROUTING
    }

    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);
    // Print out swap statistics
    print_resources_utilization();

    print_placement_swaps_stats(state);

    print_placement_move_types_stats(move_type_stat);

    if (noc_opts.noc) {
        write_noc_placement_file(noc_opts.noc_placement_file_name);
    }

    free_placement_structs(placer_opts, noc_opts);
    free_try_swap_arrays();

    print_timing_stats("Placement Quench", post_quench_timing_stats,
                       pre_quench_timing_stats);
    print_timing_stats("Placement Total ", timing_ctx.stats,
                       pre_place_timing_stats);

    VTR_LOG("update_td_costs: connections %g nets %g sum_nets %g total %g\n",
            p_runtime_ctx.f_update_td_costs_connections_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_nets_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_sum_nets_elapsed_sec,
            p_runtime_ctx.f_update_td_costs_total_elapsed_sec);
}

/* Function to update the setup slacks and criticalities before the inner loop of the annealing/quench */
static void outer_loop_update_timing_info(const t_placer_opts& placer_opts,
                                          const t_noc_opts& noc_opts,
                                          t_placer_costs* costs,
                                          int num_connections,
                                          float crit_exponent,
                                          int* outer_crit_iter_count,
                                          const PlaceDelayModel* delay_model,
                                          PlacerCriticalities* criticalities,
                                          PlacerSetupSlacks* setup_slacks,
                                          NetPinTimingInvalidator* pin_timing_invalidator,
                                          SetupTimingInfo* timing_info) {
    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*at each temperature change we update these values to be used     */
        /*for normalizing the tradeoff between timing and wirelength (bb)  */
        if (*outer_crit_iter_count >= placer_opts.recompute_crit_iter
            || placer_opts.inner_loop_recompute_divider != 0) {
#ifdef VERBOSE
            VTR_LOG("Outer loop recompute criticalities\n");
#endif
            num_connections = std::max(num_connections, 1); //Avoid division by zero
            VTR_ASSERT(num_connections > 0);

            PlaceCritParams crit_params;
            crit_params.crit_exponent = crit_exponent;
            crit_params.crit_limit = placer_opts.place_crit_limit;

            //Update all timing related classes
            perform_full_timing_update(crit_params, delay_model, criticalities,
                                       setup_slacks, pin_timing_invalidator, timing_info, costs);

            *outer_crit_iter_count = 0;
        }
        (*outer_crit_iter_count)++;
    }

    /* Update the cost normalization factors */
    update_placement_cost_normalization_factors(costs, placer_opts, noc_opts);
}

/* Function which contains the inner loop of the simulated annealing */
static void placement_inner_loop(const t_annealing_state* state,
                                 const t_placer_opts& placer_opts,
                                 const t_noc_opts& noc_opts,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 int* moves_since_cost_recompute,
                                 NetPinTimingInvalidator* pin_timing_invalidator,
                                 const PlaceDelayModel* delay_model,
                                 PlacerCriticalities* criticalities,
                                 PlacerSetupSlacks* setup_slacks,
                                 MoveGenerator& move_generator,
                                 ManualMoveGenerator& manual_move_generator,
                                 t_pl_blocks_to_be_moved& blocks_affected,
                                 SetupTimingInfo* timing_info,
                                 const t_place_algorithm& place_algorithm,
                                 MoveTypeStat& move_type_stat,
                                 float timing_bb_factor) {
    int inner_crit_iter_count, inner_iter;

    int inner_placement_save_count = 0; //How many times have we dumped placement to a file this temperature?

    stats->reset();

    inner_crit_iter_count = 1;

    bool manual_move_enabled = false;

    /* Inner loop begins */
    for (inner_iter = 0; inner_iter < state->move_lim; inner_iter++) {
        e_move_result swap_result = try_swap(state, costs, move_generator,
                                             manual_move_generator, timing_info, pin_timing_invalidator,
                                             blocks_affected, delay_model, criticalities, setup_slacks,
                                             placer_opts, noc_opts, move_type_stat, place_algorithm,
                                             timing_bb_factor, manual_move_enabled);

        if (swap_result == ACCEPTED) {
            /* Move was accepted.  Update statistics that are useful for the annealing schedule. */
            stats->single_swap_update(*costs);
            num_swap_accepted++;
        } else if (swap_result == ABORTED) {
            num_swap_aborted++;
        } else { // swap_result == REJECTED
            num_swap_rejected++;
        }

        if (place_algorithm.is_timing_driven()) {
            /* Do we want to re-timing analyze the circuit to get updated slack and criticality values?
             * We do this only once in a while, since it is expensive.
             */
            if (inner_crit_iter_count >= inner_recompute_limit
                && inner_iter != state->move_lim - 1) { /*on last iteration don't recompute */

                inner_crit_iter_count = 0;
#ifdef VERBOSE
                VTR_LOG("Inner loop recompute criticalities\n");
#endif

                PlaceCritParams crit_params;
                crit_params.crit_exponent = state->crit_exponent;
                crit_params.crit_limit = placer_opts.place_crit_limit;

                //Update all timing related classes
                perform_full_timing_update(crit_params, delay_model,
                                           criticalities, setup_slacks, pin_timing_invalidator,
                                           timing_info, costs);
            }
            inner_crit_iter_count++;
        }

        /* Lines below prevent too much round-off error from accumulating
         * in the cost over many iterations (due to incremental updates).
         * This round-off can lead to error checks failing because the cost
         * is different from what you get when you recompute from scratch.
         */
        ++(*moves_since_cost_recompute);
        if (*moves_since_cost_recompute > MAX_MOVES_BEFORE_RECOMPUTE) {
            //VTR_LOG("recomputing costs from scratch, old bb_cost is %g\n", costs->bb_cost);
            recompute_costs_from_scratch(placer_opts, noc_opts, delay_model,
                                         criticalities, costs);
            //VTR_LOG("new_bb_cost is %g\n", costs->bb_cost);
            *moves_since_cost_recompute = 0;
        }

        if (placer_opts.placement_saves_per_temperature >= 1 && inner_iter > 0
            && (inner_iter + 1)
                       % (state->move_lim
                          / placer_opts.placement_saves_per_temperature)
                   == 0) {
            std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
                                                   state->num_temps + 1, inner_placement_save_count);
            VTR_LOG(
                "Saving placement to file at temperature move %d / %d: %s\n",
                inner_iter, state->move_lim, filename.c_str());
            print_place(nullptr, nullptr, filename.c_str());
            ++inner_placement_save_count;
        }
    }

    /* Calculate the success_rate and std_dev of the costs. */
    stats->calc_iteration_stats(*costs, state->move_lim);
}

/*only count non-global connections */
static int count_connections() {
    int count = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;

        count += cluster_ctx.clb_nlist.net_sinks(net_id).size();
    }

    return (count);
}

///@brief Find the starting temperature for the annealing loop.
static float starting_t(const t_annealing_state* state,
                        t_placer_costs* costs,
                        t_annealing_sched annealing_sched,
                        const PlaceDelayModel* delay_model,
                        PlacerCriticalities* criticalities,
                        PlacerSetupSlacks* setup_slacks,
                        SetupTimingInfo* timing_info,
                        MoveGenerator& move_generator,
                        ManualMoveGenerator& manual_move_generator,
                        NetPinTimingInvalidator* pin_timing_invalidator,
                        t_pl_blocks_to_be_moved& blocks_affected,
                        const t_placer_opts& placer_opts,
                        const t_noc_opts& noc_opts,
                        MoveTypeStat& move_type_stat) {
    if (annealing_sched.type == USER_SCHED) {
        return (annealing_sched.init_t);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Use to calculate the average of cost when swap is accepted. */
    int num_accepted = 0;

    /* Use double types to avoid round off. */
    double av = 0., sum_of_squares = 0.;

    /* Determines the block swap loop count. */
    int move_lim = std::min(state->move_lim_max,
                            (int)cluster_ctx.clb_nlist.blocks().size());

    bool manual_move_enabled = false;

    for (int i = 0; i < move_lim; i++) {
#ifndef NO_GRAPHICS
        //Checks manual move flag for manual move feature
        t_draw_state* draw_state = get_draw_state_vars();
        if (draw_state->show_graphics) {
            manual_move_enabled = manual_move_is_selected();
        }
#endif /*NO_GRAPHICS*/

        //Will not deploy setup slack analysis, so omit crit_exponenet and setup_slack
        e_move_result swap_result = try_swap(state, costs, move_generator,
                                             manual_move_generator, timing_info, pin_timing_invalidator,
                                             blocks_affected, delay_model, criticalities, setup_slacks,
                                             placer_opts, noc_opts, move_type_stat, placer_opts.place_algorithm,
                                             REWARD_BB_TIMING_RELATIVE_WEIGHT, manual_move_enabled);

        if (swap_result == ACCEPTED) {
            num_accepted++;
            av += costs->cost;
            sum_of_squares += costs->cost * costs->cost;
            num_swap_accepted++;
        } else if (swap_result == ABORTED) {
            num_swap_aborted++;
        } else {
            num_swap_rejected++;
        }
    }

    /* Take the average of the accepted swaps' cost values. */
    av = num_accepted > 0 ? (av / num_accepted) : 0.;

    /* Get the standard deviation. */
    double std_dev = get_std_dev(num_accepted, sum_of_squares, av);

    /* Print warning if not all swaps are accepted. */
    if (num_accepted != move_lim) {
        VTR_LOG_WARN("Starting t: %d of %d configurations accepted.\n",
                     num_accepted, move_lim);
    }

#ifdef VERBOSE
    /* Print stats related to finding the initital temp. */
    VTR_LOG("std_dev: %g, average cost: %g, starting temp: %g\n", std_dev, av, 20. * std_dev);
#endif

    // Improved initial placement uses a fast SA for NoC routers and centroid placement
    // for other blocks. The temperature is reduced to prevent SA from destroying the initial placement
    float init_temp = std_dev / 64;

    return init_temp;
}

/**
 * @brief Pick some block and moves it to another spot.
 *
 * If the new location is empty, directly move the block. If the new location
 * is occupied, switch the blocks. Due to the different sizes of the blocks,
 * this block switching may occur for multiple times. It might also cause the
 * current swap attempt to abort due to inability to find suitable locations
 * for moved blocks.
 *
 * The move generator will record all the switched blocks in the variable
 * `blocks_affected`. Afterwards, the move will be assessed by the chosen
 * cost formulation. Currently, there are three ways to assess move cost,
 * which are stored in the enum type `t_place_algorithm`.
 *
 * @return Whether the block swap is accepted, rejected or aborted.
 */
static e_move_result try_swap(const t_annealing_state* state,
                              t_placer_costs* costs,
                              MoveGenerator& move_generator,
                              ManualMoveGenerator& manual_move_generator,
                              SetupTimingInfo* timing_info,
                              NetPinTimingInvalidator* pin_timing_invalidator,
                              t_pl_blocks_to_be_moved& blocks_affected,
                              const PlaceDelayModel* delay_model,
                              PlacerCriticalities* criticalities,
                              PlacerSetupSlacks* setup_slacks,
                              const t_placer_opts& placer_opts,
                              const t_noc_opts& noc_opts,
                              MoveTypeStat& move_type_stat,
                              const t_place_algorithm& place_algorithm,
                              float timing_bb_factor,
                              bool manual_move_enabled) {
    /* Picks some block and moves it to another spot.  If this spot is   *
     * occupied, switch the blocks.  Assess the change in cost function. *
     * rlim is the range limiter.                                        *
     * Returns whether the swap is accepted, rejected or aborted.        *
     * Passes back the new value of the cost functions.                  */

    float rlim_escape_fraction = placer_opts.rlim_escape_fraction;
    float timing_tradeoff = placer_opts.timing_tradeoff;

    PlaceCritParams crit_params;
    crit_params.crit_exponent = state->crit_exponent;
    crit_params.crit_limit = placer_opts.place_crit_limit;

    // move type and block type chosen by the agent
    t_propose_action proposed_action{e_move_type::UNIFORM, -1};

    num_ts_called++;

    MoveOutcomeStats move_outcome_stats;

    /* I'm using negative values of proposed_net_cost as a flag, *
     * so DO NOT use cost functions that can go negative.        */

    double delta_c = 0;        //Change in cost due to this swap.
    double bb_delta_c = 0;     //Change in the bounding box (wiring) cost.
    double timing_delta_c = 0; //Change in the timing cost (delay * criticality).

    // Determine whether we need to force swap two router blocks
    bool router_block_move = false;
    if (noc_opts.noc) {
        router_block_move = check_for_router_swap(noc_opts.noc_swap_percentage);
    }

    /* Allow some fraction of moves to not be restricted by rlim, */
    /* in the hopes of better escaping local minima.              */
    float rlim;
    if (rlim_escape_fraction > 0. && vtr::frand() < rlim_escape_fraction) {
        rlim = std::numeric_limits<float>::infinity();
    } else {
        rlim = state->rlim;
    }

    e_create_move create_move_outcome = e_create_move::ABORT;

    //When manual move toggle button is active, the manual move window asks the user for input.
    if (manual_move_enabled) {
#ifndef NO_GRAPHICS
        create_move_outcome = manual_move_display_and_propose(manual_move_generator, blocks_affected, proposed_action.move_type, rlim, placer_opts, criticalities);
#else  //NO_GRAPHICS 
        //Cast to void to explicitly avoid warning.
        (void)manual_move_generator;
#endif //NO_GRAPHICS
    } else if (router_block_move) {
        // generate a move where two random router blocks are swapped
        create_move_outcome = propose_router_swap(blocks_affected, rlim);
        proposed_action.move_type = e_move_type::UNIFORM;
    } else {
        //Generate a new move (perturbation) used to explore the space of possible placements
        create_move_outcome = move_generator.propose_move(blocks_affected, proposed_action, rlim, placer_opts, criticalities);
    }

    if (proposed_action.logical_blk_type_index != -1) { //if the agent proposed the block type, then collect the block type stat
        ++move_type_stat.blk_type_moves[proposed_action.logical_blk_type_index][(int)proposed_action.move_type];
    }
    LOG_MOVE_STATS_PROPOSED(t, blocks_affected);

    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tBefore move Place cost %e, bb_cost %e, timing cost %e\n", costs->cost, costs->bb_cost, costs->timing_cost);

    e_move_result move_outcome = e_move_result::ABORTED;

    if (create_move_outcome == e_create_move::ABORT) {
        LOG_MOVE_STATS_OUTCOME(std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN(), "ABORTED",
                               "illegal move");

        move_outcome = ABORTED;

    } else {
        VTR_ASSERT(create_move_outcome == e_create_move::VALID);

        /*
         * To make evaluating the move simpler (e.g. calculating changed bounding box),
         * we first move the blocks to their new locations (apply the move to
         * place_ctx.block_locs) and then compute the change in cost. If the move
         * is accepted, the inverse look-up in place_ctx.grid_blocks is updated
         * (committing the move). If the move is rejected, the blocks are returned to
         * their original positions (reverting place_ctx.block_locs to its original state).
         *
         * Note that the inverse look-up place_ctx.grid_blocks is only updated after
         * move acceptance is determined, so it should not be used when evaluating a move.
         */

        /* Update the block positions */
        apply_move_blocks(blocks_affected);

        //Find all the nets affected by this swap and update the wiring costs.
        //This cost value doesn't depend on the timing info.
        //
        //Also find all the pins affected by the swap, and calculates new connection
        //delays and timing costs and store them in proposed_* data structures.
        int num_nets_affected = find_affected_nets_and_update_costs(
            place_algorithm, delay_model, criticalities, blocks_affected,
            bb_delta_c, timing_delta_c);

        //For setup slack analysis, we first do a timing analysis to get the newest
        //slack values resulted from the proposed block moves. If the move turns out
        //to be accepted, we keep the updated slack values and commit the block moves.
        //If rejected, we reject the proposed block moves and revert this timing analysis.
        if (place_algorithm == SLACK_TIMING_PLACE) {
            /* Invalidates timing of modified connections for incremental timing updates. */
            invalidate_affected_connections(blocks_affected,
                                            pin_timing_invalidator, timing_info);

            /* Update the connection_timing_cost and connection_delay *
             * values from the temporary values.                      */
            commit_td_cost(blocks_affected);

            /* Update timing information. Since we are analyzing setup slacks,   *
             * we only update those values and keep the criticalities stale      *
             * so as not to interfere with the original timing driven algorithm. *
             *
             * Note: the timing info must be updated after applying block moves  *
             * and committing the timing driven delays and costs.                *
             * If we wish to revert this timing update due to move rejection,    *
             * we need to revert block moves and restore the timing values.      */
            criticalities->disable_update();
            setup_slacks->enable_update();
            update_timing_classes(crit_params, timing_info, criticalities,
                                  setup_slacks, pin_timing_invalidator);

            /* Get the setup slack analysis cost */
            //TODO: calculate a weighted average of the slack cost and wiring cost
            delta_c = analyze_setup_slack_cost(setup_slacks) * costs->timing_cost_norm;
        } else if (place_algorithm == CRITICALITY_TIMING_PLACE) {
            /* Take delta_c as a combination of timing and wiring cost. In
             * addition to `timing_tradeoff`, we normalize the cost values */
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                           "\t\tMove bb_delta_c %e, bb_cost_norm %e, timing_tradeoff %f, "
                           "timing_delta_c %e, timing_cost_norm %e\n",
                           bb_delta_c,
                           costs->bb_cost_norm,
                           timing_tradeoff,
                           timing_delta_c,
                           costs->timing_cost_norm);
            delta_c = (1 - timing_tradeoff) * bb_delta_c * costs->bb_cost_norm
                      + timing_tradeoff * timing_delta_c
                            * costs->timing_cost_norm;
        } else {
            VTR_ASSERT_SAFE(place_algorithm == BOUNDING_BOX_PLACE);
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                           "\t\tMove bb_delta_c %e, bb_cost_norm %e\n",
                           bb_delta_c,
                           costs->bb_cost_norm);
            delta_c = bb_delta_c * costs->bb_cost_norm;
        }

        NocCostTerms noc_delta_c; // change in NoC cost
        /* Update the NoC datastructure and costs*/
        if (noc_opts.noc) {
            find_affected_noc_routers_and_update_noc_costs(blocks_affected, noc_delta_c);

            // Include the NoC delta costs in the total cost change for this swap
            delta_c += calculate_noc_cost(noc_delta_c, costs->noc_cost_norm_factors, noc_opts);
        }

        /* 1 -> move accepted, 0 -> rejected. */
        move_outcome = assess_swap(delta_c, state->t);

        //Updates the manual_move_state members and displays costs to the user to decide whether to ACCEPT/REJECT manual move.
#ifndef NO_GRAPHICS
        if (manual_move_enabled) {
            move_outcome = pl_do_manual_move(delta_c, timing_delta_c, bb_delta_c, move_outcome);
        }
#endif //NO_GRAPHICS

        if (move_outcome == ACCEPTED) {
            costs->cost += delta_c;
            costs->bb_cost += bb_delta_c;

            if (place_algorithm == SLACK_TIMING_PLACE) {
                /* Update the timing driven cost as usual */
                costs->timing_cost += timing_delta_c;

                //Commit the setup slack information
                //The timing delay and cost values should be committed already
                commit_setup_slacks(setup_slacks);
            }

            if (place_algorithm == CRITICALITY_TIMING_PLACE) {
                costs->timing_cost += timing_delta_c;

                /* Invalidates timing of modified connections for incremental *
                 * timing updates. These invalidations are accumulated for a  *
                 * big timing update in the outer loop.                       */
                invalidate_affected_connections(blocks_affected,
                                                pin_timing_invalidator, timing_info);

                /* Update the connection_timing_cost and connection_delay *
                 * values from the temporary values.                      */
                commit_td_cost(blocks_affected);
            }

            /* Update net cost functions and reset flags. */
            update_move_nets(num_nets_affected,
                             g_vpr_ctx.placement().cube_bb);

            /* Update clb data structures since we kept the move. */
            commit_move_blocks(blocks_affected);

            if (proposed_action.logical_blk_type_index != -1) { //if the agent proposed the block type, then collect the block type stat
                ++move_type_stat.accepted_moves[proposed_action.logical_blk_type_index][(int)proposed_action.move_type];
            }
            if (noc_opts.noc) {
                commit_noc_costs();
                *costs += noc_delta_c;
            }

            //Highlights the new block when manual move is selected.
#ifndef NO_GRAPHICS
            if (manual_move_enabled) {
                manual_move_highlight_new_block_location();
            }
#endif //NO_GRAPHICS

        } else {
            VTR_ASSERT_SAFE(move_outcome == REJECTED);

            /* Reset the net cost function flags first. */
            reset_move_nets(num_nets_affected);

            /* Restore the place_ctx.block_locs data structures to their state before the move. */
            revert_move_blocks(blocks_affected);

            if (place_algorithm == SLACK_TIMING_PLACE) {
                /* Revert the timing delays and costs to pre-update values.       */
                /* These routines must be called after reverting the block moves. */
                //TODO: make this process incremental
                comp_td_connection_delays(delay_model);
                comp_td_costs(delay_model, *criticalities, &costs->timing_cost);

                /* Re-invalidate the affected sink pins since the proposed *
                 * move is rejected, and the same blocks are reverted to   *
                 * their original positions.                               */
                invalidate_affected_connections(blocks_affected,
                                                pin_timing_invalidator, timing_info);

                /* Revert the timing update */
                update_timing_classes(crit_params, timing_info, criticalities,
                                      setup_slacks, pin_timing_invalidator);

                VTR_ASSERT_SAFE_MSG(
                    verify_connection_setup_slacks(setup_slacks),
                    "The current setup slacks should be identical to the values before the try swap timing info update.");
            }

            if (place_algorithm == CRITICALITY_TIMING_PLACE) {
                /* Unstage the values stored in proposed_* data structures */
                revert_td_cost(blocks_affected);
            }

            if (proposed_action.logical_blk_type_index != -1) { //if the agent proposed the block type, then collect the block type stat
                ++move_type_stat.rejected_moves[proposed_action.logical_blk_type_index][(int)proposed_action.move_type];
            }
            /* Revert the traffic flow routes within the NoC*/
            if (noc_opts.noc) {
                revert_noc_traffic_flow_routes(blocks_affected);
            }
        }

        move_outcome_stats.delta_cost_norm = delta_c;
        move_outcome_stats.delta_bb_cost_norm = bb_delta_c
                                                * costs->bb_cost_norm;
        move_outcome_stats.delta_timing_cost_norm = timing_delta_c
                                                    * costs->timing_cost_norm;

        move_outcome_stats.delta_bb_cost_abs = bb_delta_c;
        move_outcome_stats.delta_timing_cost_abs = timing_delta_c;

        LOG_MOVE_STATS_OUTCOME(delta_c, bb_delta_c, timing_delta_c,
                               (move_outcome ? "ACCEPTED" : "REJECTED"), "");
    }
    move_outcome_stats.outcome = move_outcome;

    // If we force a router block move then it was not proposed by the
    // move generator so we should not calculate the reward and update
    // the move generators status since this outcome is not a direct
    // consequence of the move generator
    if (!router_block_move) {
        calculate_reward_and_process_outcome(placer_opts, move_outcome_stats,
                                             delta_c, timing_bb_factor, move_generator);
    } else {
//        std::cout << "Group move delta cost: " << delta_c << std::endl;
    }

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    ifndef NO_GRAPHICS
    stop_placement_and_check_breakpoints(blocks_affected, move_outcome, delta_c, bb_delta_c, timing_delta_c);
#    endif
#endif

    /* Clear the data structure containing block move info */
    clear_move_blocks(blocks_affected);

    //VTR_ASSERT(check_macro_placement_consistency() == 0);
#if 0
    // Check that each accepted swap yields a valid placement. This will
    // greatly slow the placer, but can debug some issues.
    check_place(*costs, delay_model, criticalities, place_algorithm, noc_opts);
#endif
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tAfter move Place cost %e, bb_cost %e, timing cost %e\n", costs->cost, costs->bb_cost, costs->timing_cost);
    return move_outcome;
}

static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph) {
    bool cube_bb;
    const int number_layers = g_vpr_ctx.device().grid.get_num_layers();

    // If the FPGA has only layer, then we can only use cube bounding box
    if (number_layers == 1) {
        cube_bb = true;
    } else {
        VTR_ASSERT(number_layers > 1);
        if (place_bb_mode == AUTO_BB) {
            // If the auto_bb is used, we analyze the RR graph to see whether is there any inter-layer connection that is not
            // originated from OPIN. If there is any, cube BB is chosen, otherwise, per-layer bb is chosen.
            if (inter_layer_connections_limited_to_opin(rr_graph)) {
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
    }

    return cube_bb;
}

/**
 * @brief Updates all the cost normalization factors during the outer
 * loop iteration of the placement. At each temperature change, these
 * values are updated so that we can balance the tradeoff between the
 * different placement cost components (timing, wirelength and NoC).
 * Depending on the placement mode the corresponding normalization factors are 
 * updated.
 * 
 * @param costs Contains the normalization factors which need to be updated
 * @param placer_opts Determines the placement mode
 * @param noc_opts Determines if placement includes the NoC
 */
static void update_placement_cost_normalization_factors(t_placer_costs* costs, const t_placer_opts& placer_opts, const t_noc_opts& noc_opts) {
    /* Update the cost normalization factors */
    costs->update_norm_factors();

    // update the noc normalization factors if the palcement includes the NoC
    if (noc_opts.noc) {
        update_noc_normalization_factors(*costs);
    }

    // update the current total placement cost
    costs->cost = get_total_cost(costs, placer_opts, noc_opts);

    return;
}

/**
 * @brief Compute the total normalized cost for a given placement. This
 * computation will vary depending on the placement modes.
 * 
 * @param costs The current placement cost components and their normalization
 * factors
 * @param placer_opts Determines the placement mode
 * @param noc_opts Determines if placement includes the NoC
 * @return double The computed total cost of the current placement
 */
static double get_total_cost(t_placer_costs* costs, const t_placer_opts& placer_opts, const t_noc_opts& noc_opts) {
    double total_cost = 0.0;

    if (placer_opts.place_algorithm == BOUNDING_BOX_PLACE) {
        // in bounding box mode we only care about wirelength
        total_cost = costs->bb_cost * costs->bb_cost_norm;
    } else if (placer_opts.place_algorithm.is_timing_driven()) {
        // in timing mode we include both wirelength and timing costs
        total_cost = (1 - placer_opts.timing_tradeoff) * (costs->bb_cost * costs->bb_cost_norm) + (placer_opts.timing_tradeoff) * (costs->timing_cost * costs->timing_cost_norm);
    }

    if (noc_opts.noc) {
        // in noc mode we include noc aggregate bandwidth and noc latency
        total_cost += calculate_noc_cost(costs->noc_cost_terms, costs->noc_cost_norm_factors, noc_opts);
    }

    return total_cost;
}

/**
 * @brief Check if the setup slack has gotten better or worse due to block swap.
 *
 * Get all the modified slack values via the PlacerSetupSlacks class, and compare
 * then with the original values at these connections. Sort them and compare them
 * one by one, and return the difference of the first different pair.
 *
 * If the new slack value is larger(better), than return a negative value so that
 * the move will be accepted. If the new slack value is smaller(worse), return a
 * positive value so that the move will be rejected.
 *
 * If no slack values have changed, then return an arbitrary positive number. A
 * move resulting in no change in the slack values should probably be unnecessary.
 *
 * The sorting is need to prevent in the unlikely circumstances that a bad slack
 * value suddenly got very good due to the block move, while a good slack value
 * got very bad, perhaps even worse than the original worse slack value.
 */
static float analyze_setup_slack_cost(const PlacerSetupSlacks* setup_slacks) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    const auto& p_timing_ctx = g_placer_ctx.timing();
    const auto& connection_setup_slack = p_timing_ctx.connection_setup_slack;

    //Find the original/proposed setup slacks of pins with modified values
    std::vector<float> original_setup_slacks, proposed_setup_slacks;

    auto clb_pins_modified = setup_slacks->pins_with_modified_setup_slack();
    for (ClusterPinId clb_pin : clb_pins_modified) {
        ClusterNetId net_id = clb_nlist.pin_net(clb_pin);
        size_t ipin = clb_nlist.pin_net_index(clb_pin);

        original_setup_slacks.push_back(connection_setup_slack[net_id][ipin]);
        proposed_setup_slacks.push_back(
            setup_slacks->setup_slack(net_id, ipin));
    }

    //Sort in ascending order, from the worse slack value to the best
    std::stable_sort(original_setup_slacks.begin(), original_setup_slacks.end());
    std::stable_sort(proposed_setup_slacks.begin(), proposed_setup_slacks.end());

    //Check the first pair of slack values that are different
    //If found, return their difference
    for (size_t idiff = 0; idiff < original_setup_slacks.size(); ++idiff) {
        float slack_diff = original_setup_slacks[idiff]
                           - proposed_setup_slacks[idiff];

        if (slack_diff != 0) {
            return slack_diff;
        }
    }

    //If all slack values are identical (or no modified slack values),
    //reject this move by returning an arbitrary positive number as cost.
    return 1;
}

static e_move_result assess_swap(double delta_c, double t) {
    /* Returns: 1 -> move accepted, 0 -> rejected. */
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\tTemperature is: %e delta_c is %e\n", t, delta_c);
    if (delta_c <= 0) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is accepted(delta_c < 0)\n");
        return ACCEPTED;
    }

    if (t == 0.) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is rejected(t == 0)\n");
        return REJECTED;
    }

    float fnum = vtr::frand();
    float prob_fac = std::exp(-delta_c / t);
    if (prob_fac > fnum) {
        VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is accepted(hill climbing)\n");
        return ACCEPTED;
    }
    VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug, "\t\tMove is rejected(hill climbing)\n");
    return REJECTED;
}

/**
 * @brief Update the connection_timing_cost values from the temporary
 *        values for all connections that have/haven't changed.
 *
 * All the connections have already been gathered by blocks_affected.affected_pins
 * after running the routine find_affected_nets_and_update_costs() in try_swap().
 */
static void commit_td_cost(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    auto& p_timing_ctx = g_placer_ctx.mutable_timing();
    auto& connection_delay = p_timing_ctx.connection_delay;
    auto& proposed_connection_delay = p_timing_ctx.proposed_connection_delay;
    auto& connection_timing_cost = p_timing_ctx.connection_timing_cost;
    auto& proposed_connection_timing_cost = p_timing_ctx.proposed_connection_timing_cost;

    //Go through all the sink pins affected
    for (ClusterPinId pin_id : blocks_affected.affected_pins) {
        ClusterNetId net_id = clb_nlist.pin_net(pin_id);
        int ipin = clb_nlist.pin_net_index(pin_id);

        //Commit the timing delay and cost values
        connection_delay[net_id][ipin] = proposed_connection_delay[net_id][ipin];
        proposed_connection_delay[net_id][ipin] = INVALID_DELAY;
        connection_timing_cost[net_id][ipin] = proposed_connection_timing_cost[net_id][ipin];
        proposed_connection_timing_cost[net_id][ipin] = INVALID_DELAY;
    }
}

//Reverts modifications to proposed_connection_delay and proposed_connection_timing_cost based on
//the move proposed in blocks_affected
static void revert_td_cost(const t_pl_blocks_to_be_moved& blocks_affected) {
#ifndef VTR_ASSERT_SAFE_ENABLED
    static_cast<void>(blocks_affected);
#else
    //Invalidate temp delay & timing cost values to match sanity checks in
    //comp_td_connection_cost()
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    auto& p_timing_ctx = g_placer_ctx.mutable_timing();
    auto& proposed_connection_delay = p_timing_ctx.proposed_connection_delay;
    auto& proposed_connection_timing_cost = p_timing_ctx.proposed_connection_timing_cost;

    for (ClusterPinId pin : blocks_affected.affected_pins) {
        ClusterNetId net = clb_nlist.pin_net(pin);
        int ipin = clb_nlist.pin_net_index(pin);
        proposed_connection_delay[net][ipin] = INVALID_DELAY;
        proposed_connection_timing_cost[net][ipin] = INVALID_DELAY;
    }
#endif
}

/**
 * @brief Invalidates the connections affected by the specified block moves.
 *
 * All the connections recorded in blocks_affected.affected_pins have different
 * values for `proposed_connection_delay` and `connection_delay`.
 *
 * Invalidate all the timing graph edges associated with these connections via
 * the NetPinTimingInvalidator class.
 */
static void invalidate_affected_connections(
    const t_pl_blocks_to_be_moved& blocks_affected,
    NetPinTimingInvalidator* pin_tedges_invalidator,
    TimingInfo* timing_info) {
    VTR_ASSERT_SAFE(timing_info);
    VTR_ASSERT_SAFE(pin_tedges_invalidator);

    /* Invalidate timing graph edges affected by the move */
    for (ClusterPinId pin : blocks_affected.affected_pins) {
        pin_tedges_invalidator->invalidate_connection(pin, timing_info);
    }
}

/* Allocates the major structures needed only by the placer, primarily for *
 * computing costs quickly and such.                                       */
static void alloc_and_load_placement_structs(float place_cost_exp,
                                             const t_placer_opts& placer_opts,
                                             const t_noc_opts& noc_opts,
                                             t_direct_inf* directs,
                                             int num_directs) {
    int max_pins_per_clb;
    unsigned int ipin;

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    const auto& cube_bb = place_ctx.cube_bb;

    auto& p_timing_ctx = g_placer_ctx.mutable_timing();
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    const int num_layers = device_ctx.grid.get_num_layers();

    init_placement_context();

    max_pins_per_clb = 0;
    for (const auto& type : device_ctx.physical_tile_types) {
        max_pins_per_clb = max(max_pins_per_clb, type.num_pins);
    }

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /* Allocate structures associated with timing driven placement */
        /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]  */

        p_timing_ctx.connection_delay = make_net_pins_matrix<float>(
            (const Netlist<>&)cluster_ctx.clb_nlist, 0.f);
        p_timing_ctx.proposed_connection_delay = make_net_pins_matrix<float>(
            cluster_ctx.clb_nlist, 0.f);

        p_timing_ctx.connection_setup_slack = make_net_pins_matrix<float>(
            cluster_ctx.clb_nlist, std::numeric_limits<float>::infinity());

        p_timing_ctx.connection_timing_cost = PlacerTimingCosts(
            cluster_ctx.clb_nlist);
        p_timing_ctx.proposed_connection_timing_cost = make_net_pins_matrix<
            double>(cluster_ctx.clb_nlist, 0.);
        p_timing_ctx.net_timing_cost.resize(num_nets, 0.);

        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size();
                 ipin++) {
                p_timing_ctx.connection_delay[net_id][ipin] = 0;
                p_timing_ctx.proposed_connection_delay[net_id][ipin] = INVALID_DELAY;

                p_timing_ctx.proposed_connection_timing_cost[net_id][ipin] = INVALID_DELAY;

                if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                    continue;

                p_timing_ctx.connection_timing_cost[net_id][ipin] = INVALID_DELAY;
            }
        }
    }

    init_place_move_structs(num_nets);

    if (cube_bb) {
        place_move_ctx.bb_coords.resize(num_nets, t_bb());
        place_move_ctx.bb_num_on_edges.resize(num_nets, t_bb());
    } else {
        VTR_ASSERT_SAFE(!cube_bb);
        place_move_ctx.layer_bb_num_on_edges.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
        place_move_ctx.layer_bb_coords.resize(num_nets, std::vector<t_2D_bb>(num_layers, t_2D_bb()));
    }

    place_move_ctx.num_sink_pin_layer.resize({num_nets, size_t(num_layers)});
    for (size_t flat_idx = 0; flat_idx < place_move_ctx.num_sink_pin_layer.size(); flat_idx++) {
        auto& elem = place_move_ctx.num_sink_pin_layer.get(flat_idx);
        elem = OPEN;
    }

    alloc_and_load_chan_w_factors_for_place_cost (place_cost_exp);

    alloc_and_load_try_swap_structs(cube_bb);

    place_ctx.pl_macros = alloc_and_load_placement_macros(directs, num_directs);

    if (noc_opts.noc) {
        allocate_and_load_noc_placement_structs();
    }
}

/* Frees the major structures needed by the placer (and not needed       *
 * elsewhere).   */
static void free_placement_structs(const t_placer_opts& placer_opts, const t_noc_opts& noc_opts) {
    auto& place_move_ctx = g_placer_ctx.mutable_move();

    if (placer_opts.place_algorithm.is_timing_driven()) {
        auto& p_timing_ctx = g_placer_ctx.mutable_timing();

        vtr::release_memory(p_timing_ctx.connection_timing_cost);
        vtr::release_memory(p_timing_ctx.connection_delay);
        vtr::release_memory(p_timing_ctx.connection_setup_slack);
        vtr::release_memory(p_timing_ctx.proposed_connection_timing_cost);
        vtr::release_memory(p_timing_ctx.proposed_connection_delay);
        vtr::release_memory(p_timing_ctx.net_timing_cost);
    }

    free_placement_macros_structs();

    free_place_move_structs();

    vtr::release_memory(place_move_ctx.bb_coords);
    vtr::release_memory(place_move_ctx.bb_num_on_edges);
    vtr::release_memory(place_move_ctx.bb_coords);

    vtr::release_memory(place_move_ctx.layer_bb_num_on_edges);
    vtr::release_memory(place_move_ctx.layer_bb_coords);

    place_move_ctx.num_sink_pin_layer.clear();

    free_chan_w_factors_for_place_cost ();

    free_try_swap_structs();

    if (noc_opts.noc) {
        free_noc_placement_structs();
    }
}

static void alloc_and_load_try_swap_structs(const bool cube_bb) {
    /* Allocate the local bb_coordinate storage, etc. only once. */
    /* Allocate with size cluster_ctx.clb_nlist.nets().size() for any number of nets affected. */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    init_try_swap_net_cost_structs(num_nets, cube_bb);

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.compressed_block_grids = create_compressed_block_grids();
}

static void free_try_swap_structs() {
    free_try_swap_net_cost_structs();

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    vtr::release_memory(place_ctx.compressed_block_grids);
}

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm,
                        const t_noc_opts& noc_opts) {
    /* Checks that the placement has not confused our data structures. *
     * i.e. the clb and block structures agree about the locations of  *
     * every block, blocks are in legal spots, etc.  Also recomputes   *
     * the final placement cost from scratch and makes sure it is      *
     * within roundoff of what we think the cost is.                   */

    int error = 0;

    error += check_placement_consistency();
    error += check_placement_costs(costs, delay_model, criticalities,
                                   place_algorithm);
    error += check_placement_floorplanning();

    if (noc_opts.noc) {
        // check the NoC costs during placement if the user is using the NoC supported flow
        error += check_noc_placement_costs(costs, ERROR_TOL, noc_opts);
        // make sure NoC routing configuration does not create any cycles in CDG
        error += (int)noc_routing_has_cycle();
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
                                 const t_place_algorithm& place_algorithm) {
    int error = 0;
    double bb_cost_check;
    double timing_cost_check;

    const auto& cube_bb = g_vpr_ctx.placement().cube_bb;

    if (cube_bb) {
        bb_cost_check = comp_bb_cost(CHECK);
    } else {
        VTR_ASSERT_SAFE(!cube_bb);
        bb_cost_check = comp_layer_bb_cost(CHECK);
    }

    if (fabs(bb_cost_check - costs.bb_cost) > costs.bb_cost * ERROR_TOL) {
        VTR_LOG_ERROR(
            "bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
            bb_cost_check, costs.bb_cost);
        error++;
    }

    if (place_algorithm.is_timing_driven()) {
        comp_td_costs(delay_model, *criticalities, &timing_cost_check);
        //VTR_LOG("timing_cost recomputed from scratch: %g\n", timing_cost_check);
        if (fabs(
                timing_cost_check
                - costs.timing_cost)
            > costs.timing_cost * ERROR_TOL) {
            VTR_LOG_ERROR(
                "timing_cost_check: %g and timing_cost: %g differ in check_place.\n",
                timing_cost_check, costs.timing_cost);
            error++;
        }
    }
    return error;
}

static int check_placement_consistency() {
    return check_block_placement_consistency()
           + check_macro_placement_consistency();
}

static int check_block_placement_consistency() {
    int error = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    vtr::vector<ClusterBlockId, int> bdone(
        cluster_ctx.clb_nlist.blocks().size(), 0);

    /* Step through device grid and placement. Check it against blocks */
    for (int layer_num = 0; layer_num < (int)device_ctx.grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_ctx.grid.width(); i++) {
            for (int j = 0; j < (int)device_ctx.grid.height(); j++) {
                const t_physical_tile_loc tile_loc(i, j, layer_num);
                const auto& type = device_ctx.grid.get_physical_type(tile_loc);
                if (place_ctx.grid_blocks.get_usage(tile_loc) > type->capacity) {
                    VTR_LOG_ERROR(
                        "%d blocks were placed at grid location (%d,%d,%d), but location capacity is %d.\n",
                        place_ctx.grid_blocks.get_usage(tile_loc), i, j, layer_num,
                        type->capacity);
                    error++;
                }
                int usage_check = 0;
                for (int k = 0; k < type->capacity; k++) {
                    auto bnum = place_ctx.grid_blocks.block_at_location({i, j, k, layer_num});
                    if (EMPTY_BLOCK_ID == bnum || INVALID_BLOCK_ID == bnum)
                        continue;

                    auto logical_block = cluster_ctx.clb_nlist.block_type(bnum);
                    auto physical_tile = type;

                    if (physical_tile_type(bnum) != physical_tile) {
                        VTR_LOG_ERROR(
                            "Block %zu type (%s) does not match grid location (%zu,%zu, %d) type (%s).\n",
                            size_t(bnum), logical_block->name, i, j, layer_num, physical_tile->name);
                        error++;
                    }

                    auto& loc = place_ctx.block_locs[bnum].loc;
                    if (loc.x != i || loc.y != j || loc.layer != layer_num
                        || !is_sub_tile_compatible(physical_tile, logical_block,
                                                   loc.sub_tile)) {
                        VTR_LOG_ERROR(
                            "Block %zu's location is (%d,%d,%d) but found in grid at (%zu,%zu,%d,%d).\n",
                            size_t(bnum),
                            loc.x,
                            loc.y,
                            loc.sub_tile,
                            tile_loc.x,
                            tile_loc.y,
                            tile_loc.layer_num,
                            layer_num);
                        error++;
                    }
                    ++usage_check;
                    bdone[bnum]++;
                }
                if (usage_check != place_ctx.grid_blocks.get_usage(tile_loc)) {
                    VTR_LOG_ERROR(
                        "%d block(s) were placed at location (%d,%d,%d), but location contains %d block(s).\n",
                        place_ctx.grid_blocks.get_usage(tile_loc),
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
    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        if (bdone[blk_id] != 1) {
            VTR_LOG_ERROR("Block %zu listed %d times in device context grid.\n",
                          size_t(blk_id), bdone[blk_id]);
            error++;
        }

    return error;
}

int check_macro_placement_consistency() {
    int error = 0;
    auto& place_ctx = g_vpr_ctx.placement();

    auto& pl_macros = place_ctx.pl_macros;

    /* Check the pl_macro placement are legal - blocks are in the proper relative position. */
    for (size_t imacro = 0; imacro < place_ctx.pl_macros.size(); imacro++) {
        auto head_iblk = pl_macros[imacro].members[0].blk_index;

        for (size_t imember = 0; imember < pl_macros[imacro].members.size();
             imember++) {
            auto member_iblk = pl_macros[imacro].members[imember].blk_index;

            // Compute the suppossed member's x,y,z location
            t_pl_loc member_pos = place_ctx.block_locs[head_iblk].loc
                                  + pl_macros[imacro].members[imember].offset;

            // Check the place_ctx.block_locs data structure first
            if (place_ctx.block_locs[member_iblk].loc != member_pos) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                error++;
            }

            // Then check the place_ctx.grid data structure
            if (place_ctx.grid_blocks.block_at_location(member_pos)
                != member_iblk) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                error++;
            }
        } // Finish going through all the members
    }     // Finish going through all the macros
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

static void free_try_swap_arrays() {
    g_vpr_ctx.mutable_placement().compressed_block_grids.clear();
}

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup,
                                    *timing_ctx.graph, delay_calc, is_flat);
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

static void print_resources_utilization() {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

    int max_block_name = 0;
    int max_tile_name = 0;

    //Record the resource requirement
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    std::map<t_logical_block_type_ptr,
             std::map<t_physical_tile_type_ptr, size_t>>
        num_placed_instances;
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto block_loc = place_ctx.block_locs[blk_id];
        auto loc = block_loc.loc;

        auto physical_tile = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        num_type_instances[logical_block]++;
        num_placed_instances[logical_block][physical_tile]++;

        max_block_name = std::max<int>(max_block_name,
                                       strlen(logical_block->name));
        max_tile_name = std::max<int>(max_tile_name,
                                      strlen(physical_tile->name));
    }

    VTR_LOG("\n");
    VTR_LOG("Placement resource usage:\n");
    for (auto logical_block : num_type_instances) {
        for (auto physical_tile : num_placed_instances[logical_block.first]) {
            VTR_LOG("  %-*s implemented as %-*s: %d\n", max_block_name,
                    logical_block.first->name, max_tile_name,
                    physical_tile.first->name, physical_tile.second);
        }
    }
    VTR_LOG("\n");
}

static void print_placement_swaps_stats(const t_annealing_state& state) {
    size_t total_swap_attempts = num_swap_rejected + num_swap_accepted
                                 + num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
    float reject_rate = (float)num_swap_rejected / total_swap_attempts;
    float accept_rate = (float)num_swap_accepted / total_swap_attempts;
    float abort_rate = (float)num_swap_aborted / total_swap_attempts;
    VTR_LOG("Placement number of temperatures: %d\n", state.num_temps);
    VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits,
            total_swap_attempts);
    VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits,
            num_swap_accepted, 100 * accept_rate);
    VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits,
            num_swap_rejected, 100 * reject_rate);
    VTR_LOG("\tSwaps aborted: %*d (%4.1f %%)\n", num_swap_print_digits,
            num_swap_aborted, 100 * abort_rate);
}

static void print_placement_move_types_stats(const MoveTypeStat& move_type_stat) {
    VTR_LOG("\n\nPlacement perturbation distribution by block and move type: \n");

    VTR_LOG(
        "------------------ ----------------- ---------------- ---------------- --------------- ------------ \n");
    VTR_LOG(
        "    Block Type         Move Type       (%%) of Total      Accepted(%%)     Rejected(%%)    Aborted(%%)\n");
    VTR_LOG(
        "------------------ ----------------- ---------------- ---------------- --------------- ------------ \n");

    int total_moves = 0;
    for (size_t i = 0; i < move_type_stat.blk_type_moves.size(); ++i) {
        total_moves +=  move_type_stat.blk_type_moves.get(i);
    }


    auto& device_ctx = g_vpr_ctx.device();
    int count = 0;
    int num_of_avail_moves = move_type_stat.blk_type_moves.size() / device_ctx.logical_block_types.size();

    //Print placement information for each block type
    for (const auto& itype : device_ctx.logical_block_types) {
        //Skip non-existing block types in the netlist
        if (itype.index == 0 || movable_blocks_per_type(itype).empty()) {
            continue;
        }

        count = 0;
        for (int imove = 0; imove < num_of_avail_moves; imove++) {
            const auto& move_name = move_type_to_string(e_move_type(imove));
            int moves = move_type_stat.blk_type_moves[itype.index][imove];
            if (moves != 0) {
                int accepted = move_type_stat.accepted_moves[itype.index][imove];
                int rejected = move_type_stat.rejected_moves[itype.index][imove];
                int aborted = moves - (accepted + rejected);
                if (count == 0) {
                    VTR_LOG("%-18.20s", itype.name);
                } else {
                    VTR_LOG("                  ");
                }
                VTR_LOG(
                    " %-22.20s %-16.2f %-15.2f %-14.2f %-13.2f\n",
                    move_name.c_str(), 100.0f * (float)moves / (float)total_moves,
                    100.0f * (float)accepted / (float)moves, 100.0f * (float)rejected / (float)moves,
                    100.0f * (float)aborted / (float)moves);
            }
            count++;
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

static void calculate_reward_and_process_outcome(
    const t_placer_opts& placer_opts,
    const MoveOutcomeStats& move_outcome_stats,
    const double& delta_c,
    float timing_bb_factor,
    MoveGenerator& move_generator) {
    std::string reward_fun_string = placer_opts.place_reward_fun;
    static std::optional<e_reward_function> reward_fun;
    if (!reward_fun.has_value()) {
        reward_fun = string_to_reward(reward_fun_string);
    }

    if (reward_fun == BASIC) {
        move_generator.process_outcome(-1 * delta_c, reward_fun.value());
    } else if (reward_fun == NON_PENALIZING_BASIC
               || reward_fun == RUNTIME_AWARE) {
        if (delta_c < 0) {
            move_generator.process_outcome(-1 * delta_c, reward_fun.value());
        } else {
            move_generator.process_outcome(0, reward_fun.value());
        }
    } else if (reward_fun == WL_BIASED_RUNTIME_AWARE) {
        if (delta_c < 0) {
            float reward = -1
                           * (move_outcome_stats.delta_cost_norm
                              + (0.5 - timing_bb_factor)
                                    * move_outcome_stats.delta_timing_cost_norm
                              + timing_bb_factor
                                    * move_outcome_stats.delta_bb_cost_norm);
            move_generator.process_outcome(reward, reward_fun.value());
        } else {
            move_generator.process_outcome(0, reward_fun.value());
        }
    }
}

bool placer_needs_lookahead(const t_vpr_setup& vpr_setup) {
    return (vpr_setup.PlacerOpts.place_algorithm.is_timing_driven());
}
