#include <cstdio>
#include <cmath>
#include <memory>
#include <fstream>
#include <iostream>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_geometry.h"
#include "vtr_time.h"

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

#include "uniform_move_generator.h"

#include "PlacementDelayCalculator.h"
#include "VprTimingGraphResolver.h"
#include "timing_util.h"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

#include "placer_breakpoint.h"

using std::max;
using std::min;

/************** Types and defines local to place.c ***************************/

/* Cut off for incremental bounding box updates.                          *
 * 4 is fastest -- I checked.                                             */
/* To turn off incremental bounding box updates, set this to a huge value */
#define SMALL_NET 4

/* This defines the error tolerance for floating points variables used in *
 * cost computation. 0.01 means that there is a 1% error tolerance.       */
#define ERROR_TOL .01

/* This defines the maximum number of swap attempts before invoking the   *
 * once-in-a-while placement legality check as well as floating point     *
 * variables round-offs check.                                            */
#define MAX_MOVES_BEFORE_RECOMPUTE 500000

/* Flags for the states of the bounding box.                              *
 * Stored as char for memory efficiency.                                  */
#define NOT_UPDATED_YET 'N'
#define UPDATED_ONCE 'U'
#define GOT_FROM_SCRATCH 'S'

/* For comp_cost.  NORMAL means use the method that generates updateable  *
 * bounding boxes for speed.  CHECK means compute all bounding boxes from *
 * scratch using a very simple routine to allow checks of the other       *
 * costs.                                   
 */

enum e_cost_methods {
    NORMAL,
    CHECK
};

constexpr float INVALID_DELAY = std::numeric_limits<float>::quiet_NaN();
constexpr float INVALID_COST = std::numeric_limits<double>::quiet_NaN();

/********************** Variables local to place.c ***************************/

/* Cost of a net, and a temporary cost of a net used during move assessment. */
static vtr::vector<ClusterNetId, double> net_cost, proposed_net_cost;

/* [0...cluster_ctx.clb_nlist.nets().size()-1]                                               *
 * A flag array to indicate whether the specific bounding box has been updated   *
 * in this particular swap or not. If it has been updated before, the code       *
 * must use the updated data, instead of the out-of-date data passed into the    *
 * subroutine, particularly used in try_swap(). The value NOT_UPDATED_YET        *
 * indicates that the net has not been updated before, UPDATED_ONCE indicated    *
 * that the net has been updated once, if it is going to be updated again, the   *
 * values from the previous update must be used. GOT_FROM_SCRATCH is only        *
 * applicable for nets larger than SMALL_NETS and it indicates that the          *
 * particular bounding box cannot be updated incrementally before, hence the     *
 * bounding box is got from scratch, so the bounding box would definitely be     *
 * right, DO NOT update again.                                                   */
static vtr::vector<ClusterNetId, char> bb_updated_before;

/* [0..cluster_ctx.clb_nlist.nets().size()-1].  Store the bounding box coordinates and the number of    *
 * blocks on each of a net's bounding box (to allow efficient updates),      *
 * respectively.                                                             */

static vtr::vector<ClusterNetId, t_bb> bb_coords, bb_num_on_edges;

/* The arrays below are used to precompute the inverse of the average   *
 * number of tracks per channel between [subhigh] and [sublow].  Access *
 * them as chan?_place_cost_fac[subhigh][sublow].  They are used to     *
 * speed up the computation of the cost function that takes the length  *
 * of the net bounding box in each dimension, divided by the average    *
 * number of tracks in that direction; for other cost functions they    *
 * will never be used.                                                  *
 */
static float** chanx_place_cost_fac; //[0...device_ctx.grid.width()-2]
static float** chany_place_cost_fac; //[0...device_ctx.grid.height()-2]

/* The following arrays are used by the try_swap function for speed.   */
/* [0...cluster_ctx.clb_nlist.nets().size()-1] */
static vtr::vector<ClusterNetId, t_bb> ts_bb_coord_new, ts_bb_edge_new;
static std::vector<ClusterNetId> ts_nets_to_update;

/* These file-scoped variables keep track of the number of swaps       *
 * rejected, accepted or aborted. The total number of swap attempts    *
 * is the sum of the three number.                                     */
static int num_swap_rejected = 0;
static int num_swap_accepted = 0;
static int num_swap_aborted = 0;
static int num_ts_called = 0;

/* Expected crossing counts for nets with different #'s of pins.  From *
 * ICCAD 94 pp. 690 - 695 (with linear interpolation applied by me).   *
 * Multiplied to bounding box of a net to better estimate wire length  *
 * for higher fanout nets. Each entry is the correction factor for the *
 * fanout index-1                                                      */
static const float cross_count[50] = {/* [0..49] */ 1.0, 1.0, 1.0, 1.0828, 1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493, 1.4974,
                                      1.5455, 1.5937, 1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924,
                                      1.9288, 1.9652, 2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016,
                                      2.2334, 2.2646, 2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772,
                                      2.5064, 2.5356, 2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148,
                                      2.7410, 2.7671, 2.7933};

std::unique_ptr<FILE, decltype(&vtr::fclose)> f_move_stats_file(nullptr, vtr::fclose);

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
                        int(size_t(b_from)), int(size_t(b_to)),                                \
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
static void print_clb_placement(const char* fname);
#endif

static void alloc_and_load_placement_structs(float place_cost_exp,
                                             const t_placer_opts& placer_opts,
                                             t_direct_inf* directs,
                                             int num_directs);

static void alloc_and_load_try_swap_structs();
static void free_try_swap_structs();

static void free_placement_structs(const t_placer_opts& placer_opts);

static void alloc_and_load_for_fast_cost_update(float place_cost_exp);

static void free_fast_cost_update();

static double comp_bb_cost(e_cost_methods method);

static void update_move_nets(int num_nets_affected);
static void reset_move_nets(int num_nets_affected);

static e_move_result try_swap(const t_annealing_state* state,
                              t_placer_costs* costs,
                              MoveGenerator& move_generator,
                              SetupTimingInfo* timing_info,
                              ClusteredPinTimingInvalidator* pin_timing_invalidator,
                              t_pl_blocks_to_be_moved& blocks_affected,
                              const PlaceDelayModel* delay_model,
                              PlacerCriticalities* criticalities,
                              PlacerSetupSlacks* setup_slacks,
                              float rlim_escape_fraction,
                              const t_place_algorithm& place_algorithm,
                              float timing_tradeoff);

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm);

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
                        ClusteredPinTimingInvalidator* pin_timing_invalidator,
                        t_pl_blocks_to_be_moved& blocks_affected,
                        const t_placer_opts& placer_opts);

static int count_connections();

static double recompute_bb_cost();

static void commit_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

static void revert_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

static void invalidate_affected_connections(const t_pl_blocks_to_be_moved& blocks_affected,
                                            ClusteredPinTimingInvalidator* pin_tedges_invalidator,
                                            TimingInfo* timing_info);

static bool driven_by_moved_block(const ClusterNetId net, const t_pl_blocks_to_be_moved& blocks_affected);

static float analyze_setup_slack_cost(const PlacerSetupSlacks* setup_slacks);

static e_move_result assess_swap(double delta_c, double t);

static void get_non_updateable_bb(ClusterNetId net_id, t_bb* bb_coord_new);

static void update_bb(ClusterNetId net_id, t_bb* bb_coord_new, t_bb* bb_edge_new, int xold, int yold, int xnew, int ynew);

static int find_affected_nets_and_update_costs(const t_place_algorithm& place_algorithm,
                                               const PlaceDelayModel* delay_model,
                                               const PlacerCriticalities* criticalities,
                                               t_pl_blocks_to_be_moved& blocks_affected,
                                               double& bb_delta_c,
                                               double& timing_delta_c);

static void record_affected_net(const ClusterNetId net, int& num_affected_nets);

static void update_net_bb(const ClusterNetId net,
                          const t_pl_blocks_to_be_moved& blocks_affected,
                          int iblk,
                          const ClusterBlockId blk,
                          const ClusterPinId blk_pin);
static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  t_pl_blocks_to_be_moved& blocks_affected,
                                  double& delta_timing_cost);

static double get_net_cost(ClusterNetId net_id, t_bb* bb_ptr);

static void get_bb_from_scratch(ClusterNetId net_id, t_bb* coords, t_bb* num_on_edges);

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr);

static void free_try_swap_arrays();

static void outer_loop_update_timing_info(const t_placer_opts& placer_opts,
                                          t_placer_costs* costs,
                                          int num_connections,
                                          float crit_exponent,
                                          int* outer_crit_iter_count,
                                          const PlaceDelayModel* delay_model,
                                          PlacerCriticalities* criticalities,
                                          PlacerSetupSlacks* setup_slacks,
                                          ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                          SetupTimingInfo* timing_info);

static void placement_inner_loop(const t_annealing_state* state,
                                 const t_placer_opts& placer_opts,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 int* moves_since_cost_recompute,
                                 ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                 const PlaceDelayModel* delay_model,
                                 PlacerCriticalities* criticalities,
                                 PlacerSetupSlacks* setup_slacks,
                                 MoveGenerator& move_generator,
                                 t_pl_blocks_to_be_moved& blocks_affected,
                                 SetupTimingInfo* timing_info,
                                 const t_place_algorithm& place_algorithm);

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                         const PlaceDelayModel* delay_model,
                                         const PlacerCriticalities* criticalities,
                                         t_placer_costs* costs);

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc);

static void print_place_status_header();
static void print_place_status(const t_annealing_state& state,
                               const t_placer_statistics& stats,
                               float elapsed_sec,
                               float cpd,
                               float sTNS,
                               float sWNS,
                               size_t tot_moves);
static void print_resources_utilization();

/*****************************************************************************/
void try_place(const t_placer_opts& placer_opts,
               t_annealing_sched annealing_sched,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               t_direct_inf* directs,
               int num_directs) {
    /* Does almost all the work of placing a circuit.  Width_fac gives the   *
     * width of the widest channel.  Place_cost_exp says what exponent the   *
     * width should be taken to when calculating costs.  This allows a       *
     * greater bias for anisotropic architectures.                           */
    auto& device_ctx = g_vpr_ctx.device();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& p_timing_ctx = g_placer_ctx.timing();
    const auto& p_runtime_ctx = g_placer_ctx.runtime();

    auto& timing_ctx = g_vpr_ctx.timing();
    auto pre_place_timing_stats = timing_ctx.stats;

    int tot_iter, moves_since_cost_recompute, width_fac, num_connections,
        outer_crit_iter_count, inner_recompute_limit;
    float first_crit_exponent, first_rlim, first_t;
    int first_move_lim;

    t_placer_costs costs(placer_opts.place_algorithm);

    tatum::TimingPathInfo critical_path;
    float sTNS = NAN;
    float sWNS = NAN;

    char msg[vtr::bufsize];
    t_placer_statistics stats;

    std::shared_ptr<SetupTimingInfo> timing_info;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc;
    std::unique_ptr<PlaceDelayModel> place_delay_model;
    std::unique_ptr<MoveGenerator> move_generator;
    std::unique_ptr<PlacerSetupSlacks> placer_setup_slacks;
    std::unique_ptr<PlacerCriticalities> placer_criticalities;
    std::unique_ptr<ClusteredPinTimingInvalidator> pin_timing_invalidator;

    t_pl_blocks_to_be_moved blocks_affected(cluster_ctx.clb_nlist.blocks().size());

    /* Allocated here because it goes into timing critical code where each memory allocation is expensive */
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.logical_block_types);

    /* init file scope variables */
    num_swap_rejected = 0;
    num_swap_accepted = 0;
    num_swap_aborted = 0;
    num_ts_called = 0;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = alloc_lookups_and_delay_model(chan_width_dist, placer_opts, router_opts, det_routing_arch, segment_inf, directs, num_directs);

        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    move_generator = std::make_unique<UniformMoveGenerator>();

    width_fac = placer_opts.place_chan_width;

    init_chan(width_fac, chan_width_dist);

    alloc_and_load_placement_structs(placer_opts.place_cost_exp, placer_opts,
                                     directs, num_directs);

    initial_placement(placer_opts.pad_loc_type, placer_opts.constraints_file.c_str());

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

    init_draw_coords((float)width_fac);
    //Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, pb_gpin_lookup);

    /* Gets initial cost and loads bounding boxes. */

    if (placer_opts.place_algorithm.is_timing_driven()) {
        costs.bb_cost = comp_bb_cost(NORMAL);

        first_crit_exponent = placer_opts.td_place_exp_first; /*this will be modified when rlim starts to change */

        num_connections = count_connections();
        VTR_LOG("\n");
        VTR_LOG("There are %d point to point connections in this circuit.\n", num_connections);
        VTR_LOG("\n");

        //Update the point-to-point delays from the initial placement
        comp_td_connection_delays(place_delay_model.get());

        /*
         * Initialize timing analysis
         */
        placement_delay_calc = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, p_timing_ctx.connection_delay);
        placement_delay_calc->set_tsu_margin_relative(placer_opts.tsu_rel_margin);
        placement_delay_calc->set_tsu_margin_absolute(placer_opts.tsu_abs_margin);

        timing_info = make_setup_timing_info(placement_delay_calc, placer_opts.timing_update_type);

        placer_setup_slacks = std::make_unique<PlacerSetupSlacks>(cluster_ctx.clb_nlist, netlist_pin_lookup);

        placer_criticalities = std::make_unique<PlacerCriticalities>(cluster_ctx.clb_nlist, netlist_pin_lookup);

        pin_timing_invalidator = std::make_unique<ClusteredPinTimingInvalidator>(cluster_ctx.clb_nlist,
                                                                                 netlist_pin_lookup,
                                                                                 atom_ctx.nlist,
                                                                                 atom_ctx.lookup,
                                                                                 *timing_info->timing_graph());
        //First time compute timing and costs, compute from scratch
        initialize_timing_info(first_crit_exponent,
                               place_delay_model.get(),
                               placer_criticalities.get(),
                               placer_setup_slacks.get(),
                               pin_timing_invalidator.get(),
                               timing_info.get(),
                               &costs);

        critical_path = timing_info->least_slack_critical_path();

        /* Write out the initial timing echo file */
        if (isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH) + std::string(".dot"),
                                         *timing_info, debug_tnode);
        }

        outer_crit_iter_count = 1;

        /* Initialize the normalization factors. Calling costs.update_norm_factors() *
         * here would fail the golden results of strong_sdc benchmark                */
        costs.timing_cost_norm = 1 / costs.timing_cost;
        costs.bb_cost_norm = 1 / costs.bb_cost;
        costs.cost = 1;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);

        /* Total cost is the same as wirelength cost */
        costs.bb_cost = comp_bb_cost(NORMAL);
        costs.cost = costs.bb_cost;

        /* Timing cost and normalization factors are not used */
        costs.timing_cost = INVALID_COST;
        costs.timing_cost_norm = INVALID_COST;
        costs.bb_cost_norm = INVALID_COST;

        /* Other initializations */
        outer_crit_iter_count = 0;
        num_connections = 0;
        first_crit_exponent = 0;
    }

    //Sanity check that initial placement is legal
    check_place(costs, place_delay_model.get(), placer_criticalities.get(), placer_opts.place_algorithm);

    //Initial pacement statistics
    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g\n",
            costs.cost, costs.bb_cost, costs.timing_cost);
    if (placer_opts.place_algorithm.is_timing_driven()) {
        VTR_LOG("Initial placement estimated Critical Path Delay (CPD): %g ns\n",
                1e9 * critical_path.delay());
        VTR_LOG("Initial placement estimated setup Total Negative Slack (sTNS): %g ns\n",
                1e9 * timing_info->setup_total_negative_slack());
        VTR_LOG("Initial placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
                1e9 * timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");

        VTR_LOG("Initial placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
    }
    size_t num_macro_members = 0;
    for (auto& macro : g_vpr_ctx.placement().pl_macros) {
        num_macro_members += macro.members.size();
    }
    VTR_LOG("Placement contains %zu placement macros involving %zu blocks (average macro size %f)\n", g_vpr_ctx.placement().pl_macros.size(), num_macro_members, float(num_macro_members) / g_vpr_ctx.placement().pl_macros.size());
    VTR_LOG("\n");

    sprintf(msg, "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
    //Draw the initial placement
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place", 0, 0);
        VTR_LOG("Saving initial placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str());
    }

    first_move_lim = get_initial_move_lim(placer_opts, annealing_sched);

    if (placer_opts.inner_loop_recompute_divider != 0) {
        inner_recompute_limit = (int)(0.5 + (float)first_move_lim / (float)placer_opts.inner_loop_recompute_divider);
    } else {
        /*don't do an inner recompute */
        inner_recompute_limit = first_move_lim + 1;
    }

    int quench_recompute_limit;
    if (placer_opts.quench_recompute_divider != 0) {
        quench_recompute_limit = (int)(0.5 + (float)first_move_lim / (float)placer_opts.quench_recompute_divider);
    } else {
        /*don't do an quench recompute */
        quench_recompute_limit = first_move_lim + 1;
    }

    /* Get the first range limiter */
    first_rlim = (float)max(device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    /* Set the temperature high so essentially all swaps will be accepted   */
    /* when trying to determine the starting temp for placement inner loop. */
    first_t = HUGE_POSITIVE_FLOAT;

    t_annealing_state state(annealing_sched, first_t, first_rlim, first_move_lim, first_crit_exponent);

    /* Update the starting temperature for placement annealing to a more appropriate value */
    state.t = starting_t(&state,
                         &costs,
                         annealing_sched,
                         place_delay_model.get(),
                         placer_criticalities.get(),
                         placer_setup_slacks.get(),
                         timing_info.get(),
                         *move_generator,
                         pin_timing_invalidator.get(),
                         blocks_affected,
                         placer_opts);

    if (!placer_opts.move_stats_file.empty()) {
        f_move_stats_file = std::unique_ptr<FILE, decltype(&vtr::fclose)>(vtr::fopen(placer_opts.move_stats_file.c_str(), "w"), vtr::fclose);
        LOG_MOVE_STATS_HEADER();
    }

    tot_iter = 0;
    moves_since_cost_recompute = 0;

#ifdef ENABLE_ANALYTIC_PLACE
    // Analytic placer: When enabled, skip most of the annealing and go straight to quench
    // TODO: refactor goto label.
    if (placer_opts.enable_analytic_placer)
        goto quench;
#endif /* ENABLE_ANALYTIC_PLACE */

    //Table header
    VTR_LOG("\n");
    print_place_status_header();

    /* Outer loop of the simulated annealing begins */
    do {
        vtr::Timer temperature_timer;

        outer_loop_update_timing_info(placer_opts,
                                      &costs,
                                      num_connections,
                                      state.crit_exponent,
                                      &outer_crit_iter_count,
                                      place_delay_model.get(),
                                      placer_criticalities.get(),
                                      placer_setup_slacks.get(),
                                      pin_timing_invalidator.get(),
                                      timing_info.get());

        placement_inner_loop(&state, placer_opts,
                             inner_recompute_limit, &stats,
                             &costs,
                             &moves_since_cost_recompute,
                             pin_timing_invalidator.get(),
                             place_delay_model.get(),
                             placer_criticalities.get(),
                             placer_setup_slacks.get(),
                             *move_generator,
                             blocks_affected,
                             timing_info.get(),
                             placer_opts.place_algorithm);

        tot_iter += state.move_lim;
        ++state.num_temps;

        if (placer_opts.place_algorithm.is_timing_driven()) {
            critical_path = timing_info->least_slack_critical_path();
            sTNS = timing_info->setup_total_negative_slack();
            sWNS = timing_info->setup_worst_negative_slack();
        }

        print_place_status(state, stats, temperature_timer.elapsed_sec(), critical_path.delay(), sTNS, sWNS, tot_iter);

        sprintf(msg, "Cost: %g  BB Cost %g  TD Cost %g  Temperature: %g",
                costs.cost, costs.bb_cost, costs.timing_cost, state.t);
        update_screen(ScreenUpdatePriority::MINOR, msg, PLACEMENT, timing_info);

#ifdef VERBOSE
        if (getEchoEnabled()) {
            print_clb_placement("first_iteration_clb_placement.echo");
        }
#endif
    } while (state.outer_loop_update(stats.success_rate, costs, placer_opts, annealing_sched));
    /* Outer loop of the simmulated annealing ends */

#ifdef ENABLE_ANALYTIC_PLACE
// guard quench label, otherwise compiler complains about unused label
quench:
#endif /* ENABLE_ANALYTIC_PLACE */

    /* Start Quench */
    state.t = 0;                         //Freeze out: only accept solutions that improve placement.
    state.move_lim = state.move_lim_max; //Revert the move limit to initial value.

    auto pre_quench_timing_stats = timing_ctx.stats;
    { /* Quench */
        vtr::ScopedFinishTimer temperature_timer("Placement Quench");

        outer_loop_update_timing_info(placer_opts,
                                      &costs,
                                      num_connections,
                                      state.crit_exponent,
                                      &outer_crit_iter_count,
                                      place_delay_model.get(),
                                      placer_criticalities.get(),
                                      placer_setup_slacks.get(),
                                      pin_timing_invalidator.get(),
                                      timing_info.get());

        /* Run inner loop again with temperature = 0 so as to accept only swaps
         * which reduce the cost of the placement */
        placement_inner_loop(&state, placer_opts,
                             quench_recompute_limit, &stats,
                             &costs,
                             &moves_since_cost_recompute,
                             pin_timing_invalidator.get(),
                             place_delay_model.get(),
                             placer_criticalities.get(),
                             placer_setup_slacks.get(),
                             *move_generator,
                             blocks_affected,
                             timing_info.get(),
                             placer_opts.place_quench_algorithm);

        tot_iter += state.move_lim;
        ++state.num_temps;

        if (placer_opts.place_quench_algorithm.is_timing_driven()) {
            critical_path = timing_info->least_slack_critical_path();
            sTNS = timing_info->setup_total_negative_slack();
            sWNS = timing_info->setup_worst_negative_slack();
        }

        print_place_status(state, stats, temperature_timer.elapsed_sec(), critical_path.delay(), sTNS, sWNS, tot_iter);
    }
    auto post_quench_timing_stats = timing_ctx.stats;

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place", state.num_temps + 1, 0);
        VTR_LOG("Saving final placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str());
    }

    // TODO:
    // 1. add some subroutine hierarchy!  Too big!

#ifdef VERBOSE
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_END_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_END_CLB_PLACEMENT));
    }
#endif

    check_place(costs, place_delay_model.get(), placer_criticalities.get(), placer_opts.place_algorithm);

    //Some stats
    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", num_ts_called);
    report_aborted_moves();

    if (placer_opts.place_algorithm.is_timing_driven()) {
        //Final timing estimate
        VTR_ASSERT(timing_info);
        perform_full_timing_update(state.crit_exponent,
                                   place_delay_model.get(),
                                   placer_criticalities.get(),
                                   placer_setup_slacks.get(),
                                   pin_timing_invalidator.get(),
                                   timing_info.get(),
                                   &costs);

        critical_path = timing_info->least_slack_critical_path();

        if (isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH) + std::string(".dot"),
                                         *timing_info, debug_tnode);
        }

        generate_post_place_timing_reports(placer_opts,
                                           analysis_opts,
                                           *timing_info,
                                           *placement_delay_calc);

        /* Print critical path delay metrics */
        VTR_LOG("\n");
        print_setup_timing_summary(*timing_ctx.constraints, *timing_info->setup_analyzer(), "Placement estimated ");
    }

    sprintf(msg, "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
    VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, \n",
            costs.cost, costs.bb_cost, costs.timing_cost);
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);
    // Print out swap statistics
    size_t total_swap_attempts = num_swap_rejected + num_swap_accepted + num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    print_resources_utilization();

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
    float reject_rate = (float)num_swap_rejected / total_swap_attempts;
    float accept_rate = (float)num_swap_accepted / total_swap_attempts;
    float abort_rate = (float)num_swap_aborted / total_swap_attempts;
    VTR_LOG("Placement number of temperatures: %d\n", state.num_temps);
    VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits, total_swap_attempts);
    VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_accepted, 100 * accept_rate);
    VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_rejected, 100 * reject_rate);
    VTR_LOG("\tSwaps aborted : %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_aborted, 100 * abort_rate);

    free_placement_structs(placer_opts);
    free_try_swap_arrays();

    print_timing_stats("Placement Quench", post_quench_timing_stats, pre_quench_timing_stats);
    print_timing_stats("Placement Total ", timing_ctx.stats, pre_place_timing_stats);

    VTR_LOG(
        "update_td_costs: connections %g nets %g sum_nets %g total %g\n",
        p_runtime_ctx.f_update_td_costs_connections_elapsed_sec,
        p_runtime_ctx.f_update_td_costs_nets_elapsed_sec,
        p_runtime_ctx.f_update_td_costs_sum_nets_elapsed_sec,
        p_runtime_ctx.f_update_td_costs_total_elapsed_sec);
}

/* Function to update the setup slacks and criticalities before the inner loop of the annealing/quench */
static void outer_loop_update_timing_info(const t_placer_opts& placer_opts,
                                          t_placer_costs* costs,
                                          int num_connections,
                                          float crit_exponent,
                                          int* outer_crit_iter_count,
                                          const PlaceDelayModel* delay_model,
                                          PlacerCriticalities* criticalities,
                                          PlacerSetupSlacks* setup_slacks,
                                          ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                          SetupTimingInfo* timing_info) {
    if (!placer_opts.place_algorithm.is_timing_driven()) {
        return;
    }

    /*at each temperature change we update these values to be used     */
    /*for normalizing the tradeoff between timing and wirelength (bb)  */
    if (*outer_crit_iter_count >= placer_opts.recompute_crit_iter
        || placer_opts.inner_loop_recompute_divider != 0) {
#ifdef VERBOSE
        VTR_LOG("Outer loop recompute criticalities\n");
#endif
        num_connections = std::max(num_connections, 1); //Avoid division by zero
        VTR_ASSERT(num_connections > 0);

        //Update all timing related classes
        perform_full_timing_update(crit_exponent,
                                   delay_model,
                                   criticalities,
                                   setup_slacks,
                                   pin_timing_invalidator,
                                   timing_info,
                                   costs);

        *outer_crit_iter_count = 0;
    }
    (*outer_crit_iter_count)++;

    /* Update the cost normalization factors */
    costs->update_norm_factors();
}

/* Function which contains the inner loop of the simulated annealing */
static void placement_inner_loop(const t_annealing_state* state,
                                 const t_placer_opts& placer_opts,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 int* moves_since_cost_recompute,
                                 ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                 const PlaceDelayModel* delay_model,
                                 PlacerCriticalities* criticalities,
                                 PlacerSetupSlacks* setup_slacks,
                                 MoveGenerator& move_generator,
                                 t_pl_blocks_to_be_moved& blocks_affected,
                                 SetupTimingInfo* timing_info,
                                 const t_place_algorithm& place_algorithm) {
    int inner_crit_iter_count, inner_iter;

    int inner_placement_save_count = 0; //How many times have we dumped placement to a file this temperature?

    stats->reset();

    inner_crit_iter_count = 1;

    /* Inner loop begins */
    for (inner_iter = 0; inner_iter < state->move_lim; inner_iter++) {
        e_move_result swap_result = try_swap(state,
                                             costs,
                                             move_generator,
                                             timing_info,
                                             pin_timing_invalidator,
                                             blocks_affected,
                                             delay_model,
                                             criticalities,
                                             setup_slacks,
                                             placer_opts.rlim_escape_fraction,
                                             place_algorithm,
                                             placer_opts.timing_tradeoff);

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
                //Update all timing related classes
                perform_full_timing_update(state->crit_exponent,
                                           delay_model,
                                           criticalities,
                                           setup_slacks,
                                           pin_timing_invalidator,
                                           timing_info,
                                           costs);
            }
            inner_crit_iter_count++;
        }
#ifdef VERBOSE
        VTR_LOG("t = %g  cost = %g   bb_cost = %g timing_cost = %g move = %d\n",
                t, costs->cost, costs->bb_cost, costs->timing_cost, inner_iter);
        if (fabs((costs->bb_cost) - comp_bb_cost(CHECK)) > (costs->bb_cost) * ERROR_TOL)
            VPR_ERROR(VPR_ERROR_PLACE,
                      "fabs((*bb_cost) - comp_bb_cost(CHECK)) > (*bb_cost) * ERROR_TOL");
#endif

        /* Lines below prevent too much round-off error from accumulating
         * in the cost over many iterations (due to incremental updates).
         * This round-off can lead to error checks failing because the cost
         * is different from what you get when you recompute from scratch.
         */
        ++(*moves_since_cost_recompute);
        if (*moves_since_cost_recompute > MAX_MOVES_BEFORE_RECOMPUTE) {
            recompute_costs_from_scratch(placer_opts, delay_model, criticalities, costs);
            *moves_since_cost_recompute = 0;
        }

        if (placer_opts.placement_saves_per_temperature >= 1
            && inner_iter > 0
            && (inner_iter + 1) % (state->move_lim / placer_opts.placement_saves_per_temperature) == 0) {
            std::string filename = vtr::string_fmt("placement_%03d_%03d.place", state->num_temps + 1, inner_placement_save_count);
            VTR_LOG("Saving placement to file at temperature move %d / %d: %s\n", inner_iter, state->move_lim, filename.c_str());
            print_place(nullptr, nullptr, filename.c_str());
            ++inner_placement_save_count;
        }
    }

    /* Calculate the success_rate and std_dev of the costs. */
    stats->calc_iteration_stats(*costs, state->move_lim);
}

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                         const PlaceDelayModel* delay_model,
                                         const PlacerCriticalities* criticalities,
                                         t_placer_costs* costs) {
    double new_bb_cost = recompute_bb_cost();
    if (fabs(new_bb_cost - costs->bb_cost) > costs->bb_cost * ERROR_TOL) {
        std::string msg = vtr::string_fmt("in recompute_costs_from_scratch: new_bb_cost = %g, old bb_cost = %g\n",
                                          new_bb_cost, costs->bb_cost);
        VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
    }
    costs->bb_cost = new_bb_cost;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, *criticalities, &new_timing_cost);
        if (fabs(new_timing_cost - costs->timing_cost) > costs->timing_cost * ERROR_TOL) {
            std::string msg = vtr::string_fmt("in recompute_costs_from_scratch: new_timing_cost = %g, old timing_cost = %g, ERROR_TOL = %g\n",
                                              new_timing_cost, costs->timing_cost, ERROR_TOL);
            VPR_ERROR(VPR_ERROR_PLACE, msg.c_str());
        }
        costs->timing_cost = new_timing_cost;
    } else {
        VTR_ASSERT(placer_opts.place_algorithm == BOUNDING_BOX_PLACE);

        costs->cost = new_bb_cost;
    }
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
                        ClusteredPinTimingInvalidator* pin_timing_invalidator,
                        t_pl_blocks_to_be_moved& blocks_affected,
                        const t_placer_opts& placer_opts) {
    /* Use user-specified value for the initial temperature. */
    if (annealing_sched.type == USER_SCHED) {
        return (annealing_sched.init_t);
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Use to calculate the average of cost when swap is accepted. */
    int num_accepted = 0;

    /* Use double types to avoid round off. */
    double av = 0., sum_of_squares = 0.;

    /* Determines the block swap loop count. */
    int move_lim = std::min(state->move_lim_max, (int)cluster_ctx.clb_nlist.blocks().size());

    for (int i = 0; i < move_lim; i++) {
        //Will not deploy setup slack analysis, so omit crit_exponenet and setup_slack
        e_move_result swap_result = try_swap(state,
                                             costs,
                                             move_generator,
                                             timing_info,
                                             pin_timing_invalidator,
                                             blocks_affected,
                                             delay_model,
                                             criticalities,
                                             setup_slacks,
                                             placer_opts.rlim_escape_fraction,
                                             placer_opts.place_algorithm,
                                             placer_opts.timing_tradeoff);

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
        VTR_LOG_WARN("Starting t: %d of %d configurations accepted.\n", num_accepted, move_lim);
    }

#ifdef VERBOSE
    /* Print stats related to finding the initital temp. */
    VTR_LOG("std_dev: %g, average cost: %g, starting temp: %g\n", std_dev, av, 20. * std_dev);
#endif

    /* Set the initial temperature to 20 times the standard of deviation */
    /* so that the initial temperature adjusts according to the circuit */
    return (20. * std_dev);
}

static void update_move_nets(int num_nets_affected) {
    /* update net cost functions and reset flags. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        bb_coords[net_id] = ts_bb_coord_new[net_id];
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET)
            bb_num_on_edges[net_id] = ts_bb_edge_new[net_id];

        net_cost[net_id] = proposed_net_cost[net_id];

        /* negative proposed_net_cost value is acting as a flag. */
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
}

static void reset_move_nets(int num_nets_affected) {
    /* Reset the net cost function flags first. */
    for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];
        proposed_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
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
 * which are stored in the enum type `e_place_algorithm`.
 *
 * @return Whether the block swap is accepted, rejected or aborted.
 */
static e_move_result try_swap(const t_annealing_state* state,
                              t_placer_costs* costs,
                              MoveGenerator& move_generator,
                              SetupTimingInfo* timing_info,
                              ClusteredPinTimingInvalidator* pin_timing_invalidator,
                              t_pl_blocks_to_be_moved& blocks_affected,
                              const PlaceDelayModel* delay_model,
                              PlacerCriticalities* criticalities,
                              PlacerSetupSlacks* setup_slacks,
                              float rlim_escape_fraction,
                              const t_place_algorithm& place_algorithm,
                              float timing_tradeoff) {
    num_ts_called++;

    MoveOutcomeStats move_outcome_stats;

    /* I'm using negative values of proposed_net_cost as a flag, *
     * so DO NOT use cost functions that can go negative.        */

    double delta_c = 0;        //Change in cost due to this swap.
    double bb_delta_c = 0;     //Change in the bounding box (wiring) cost.
    double timing_delta_c = 0; //Change in the timing cost (delay * criticality).

    /* Allow some fraction of moves to not be restricted by rlim, */
    /* in the hopes of better escaping local minima.              */
    float rlim;
    if (rlim_escape_fraction > 0. && vtr::frand() < rlim_escape_fraction) {
        rlim = std::numeric_limits<float>::infinity();
    } else {
        rlim = state->rlim;
    }

    /* Generate a new move (perturbation) used to explore the space of possible placements. */
    e_create_move create_move_outcome = move_generator.propose_move(blocks_affected, rlim);

    LOG_MOVE_STATS_PROPOSED(t, blocks_affected);

    e_move_result move_outcome = ABORTED;

    if (create_move_outcome == e_create_move::ABORT) {
        LOG_MOVE_STATS_OUTCOME(std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN(),
                               "ABORTED", "illegal move");

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
        int num_nets_affected = find_affected_nets_and_update_costs(place_algorithm,
                                                                    delay_model,
                                                                    criticalities,
                                                                    blocks_affected,
                                                                    bb_delta_c,
                                                                    timing_delta_c);

        //For setup slack analysis, we first do a timing analysis to get the newest
        //slack values resulted from the proposed block moves. If the move turns out
        //to be accepted, we keep the updated slack values and commit the block moves.
        //If rejected, we reject the proposed block moves and revert this timing analysis.
        if (place_algorithm == SLACK_TIMING_PLACE) {
            /* Invalidates timing of modified connections for incremental timing updates. */
            invalidate_affected_connections(blocks_affected,
                                            pin_timing_invalidator,
                                            timing_info);

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
            update_timing_classes(state->crit_exponent,
                                  timing_info,
                                  criticalities,
                                  setup_slacks,
                                  pin_timing_invalidator);

            /* Get the setup slack analysis cost */
            //TODO: calculate a weighted average of the slack cost and wiring cost
            delta_c = analyze_setup_slack_cost(setup_slacks);
        } else if (place_algorithm == CRITICALITY_TIMING_PLACE) {
            /* Take delta_c as a combination of timing and wiring cost. In
             * addition to `timing_tradeoff`, we normalize the cost values */
            delta_c = (1 - timing_tradeoff) * bb_delta_c * costs->bb_cost_norm
                      + timing_tradeoff * timing_delta_c * costs->timing_cost_norm;
        } else {
            VTR_ASSERT_SAFE(place_algorithm == BOUNDING_BOX_PLACE);
            delta_c = bb_delta_c;
        }

        /* 1 -> move accepted, 0 -> rejected. */
        move_outcome = assess_swap(delta_c, state->t);

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
                                                pin_timing_invalidator,
                                                timing_info);

                /* Update the connection_timing_cost and connection_delay *
                 * values from the temporary values.                      */
                commit_td_cost(blocks_affected);
            }

            /* Update net cost functions and reset flags. */
            update_move_nets(num_nets_affected);

            /* Update clb data structures since we kept the move. */
            commit_move_blocks(blocks_affected);

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
                                                pin_timing_invalidator,
                                                timing_info);

                /* Revert the timing update */
                update_timing_classes(state->crit_exponent,
                                      timing_info,
                                      criticalities,
                                      setup_slacks,
                                      pin_timing_invalidator);

                VTR_ASSERT_SAFE_MSG(
                    verify_connection_setup_slacks(setup_slacks),
                    "The current setup slacks should be identical to the values before the try swap timing info update.");
            }

            if (place_algorithm == CRITICALITY_TIMING_PLACE) {
                /* Unstage the values stored in proposed_* data structures */
                revert_td_cost(blocks_affected);
            }
        }

        move_outcome_stats.delta_cost_norm = delta_c;
        move_outcome_stats.delta_bb_cost_norm = bb_delta_c * costs->bb_cost_norm;
        move_outcome_stats.delta_timing_cost_norm = timing_delta_c * costs->timing_cost_norm;

        move_outcome_stats.delta_bb_cost_abs = bb_delta_c;
        move_outcome_stats.delta_timing_cost_abs = timing_delta_c;

        LOG_MOVE_STATS_OUTCOME(delta_c, bb_delta_c, timing_delta_c,
                               (move_outcome ? "ACCEPTED" : "REJECTED"), "");
    }

    move_outcome_stats.outcome = move_outcome;

    move_generator.process_outcome(move_outcome_stats);

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    ifndef NO_GRAPHICS
    stop_placement_and_check_breakopints(blocks_affected, move_outcome, delta_c, bb_delta_c, timing_delta_c);
#    endif
#endif

    /* Clear the data structure containing block move info */
    clear_move_blocks(blocks_affected);

    //VTR_ASSERT(check_macro_placement_consistency() == 0);
#if 0
    //Check that each accepted swap yields a valid placement
    check_place(*costs, delay_model, place_algorithm);
#endif

    return move_outcome;
}

/**
 * @brief Find all the nets and pins affected by this swap and update costs.
 *
 * Find all the nets affected by this swap and update the bounding box (wiring)
 * costs. This cost function doesn't depend on the timing info.
 *
 * Find all the connections affected by this swap and update the timing cost.
 * For a connection to be affected, it not only needs to be on or driven by
 * a block, but it also needs to have its delay changed. Otherwise, it will
 * not be added to the affected_pins structure.
 *
 * For more, see update_td_delta_costs().
 *
 * The timing costs are calculated by getting the new connection delays,
 * multiplied by the connection criticalities returned by the timing
 * analyzer. These timing costs are stored in the proposed_* data structures.
 *
 * The change in the bounding box cost is stored in `bb_delta_c`.
 * The change in the timing cost is stored in `timing_delta_c`.
 *
 * @return The number of affected nets.
 */
static int find_affected_nets_and_update_costs(const t_place_algorithm& place_algorithm,
                                               const PlaceDelayModel* delay_model,
                                               const PlacerCriticalities* criticalities,
                                               t_pl_blocks_to_be_moved& blocks_affected,
                                               double& bb_delta_c,
                                               double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int num_affected_nets = 0;

    /* Go through all the blocks moved. */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        /* Go through all the pins in the moved block. */
        for (ClusterPinId blk_pin : cluster_ctx.clb_nlist.block_pins(blk)) {
            ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(blk_pin);
            VTR_ASSERT_SAFE_MSG(net_id, "Only valid nets should be found in compressed netlist block pins");

            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                //TODO: Do we require anyting special here for global nets?
                //"Global nets are assumed to span the whole chip, and do not effect costs."
                continue;

            /* Record effected nets */
            record_affected_net(net_id, num_affected_nets);

            /* Update the net bounding boxes. */
            update_net_bb(net_id, blocks_affected, iblk, blk, blk_pin);

            if (place_algorithm.is_timing_driven()) {
                /* Determine the change in connection delay and timing cost. */
                update_td_delta_costs(delay_model, *criticalities, net_id, blk_pin, blocks_affected, timing_delta_c);
            }
        }
    }

    /* Now update the bounding box costs (since the net bounding     *
     * boxes are up-to-date). The cost is only updated once per net. */
    for (int inet_affected = 0; inet_affected < num_affected_nets; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        proposed_net_cost[net_id] = get_net_cost(net_id, &ts_bb_coord_new[net_id]);
        bb_delta_c += proposed_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}

///@brief Record effected nets.
static void record_affected_net(const ClusterNetId net, int& num_affected_nets) {
    /* Record effected nets. */
    if (proposed_net_cost[net] < 0.) {
        /* Net not marked yet. */
        ts_nets_to_update[num_affected_nets] = net;
        num_affected_nets++;

        /* Flag to say we've marked this net. */
        proposed_net_cost[net] = 1.;
    }
}

/**
 * @brief Update the net bounding boxes.
 *
 * Do not update the net cost here since it should only
 * be updated once per net, not once per pin.
 */
static void update_net_bb(const ClusterNetId net,
                          const t_pl_blocks_to_be_moved& blocks_affected,
                          int iblk,
                          const ClusterBlockId blk,
                          const ClusterPinId blk_pin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_updated_before[net] == NOT_UPDATED_YET) { //Only once per-net
            get_non_updateable_bb(net, &ts_bb_coord_new[net]);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = tile_pin_index(blk_pin);

        t_physical_tile_type_ptr blk_type = physical_tile_type(blk);
        int pin_width_offset = blk_type->pin_width_offset[iblk_pin];
        int pin_height_offset = blk_type->pin_height_offset[iblk_pin];

        //Incremental bounding box update
        update_bb(net, &ts_bb_coord_new[net],
                  &ts_bb_edge_new[net],
                  blocks_affected.moved_blocks[iblk].old_loc.x + pin_width_offset,
                  blocks_affected.moved_blocks[iblk].old_loc.y + pin_height_offset,
                  blocks_affected.moved_blocks[iblk].new_loc.x + pin_width_offset,
                  blocks_affected.moved_blocks[iblk].new_loc.y + pin_height_offset);
    }
}

/**
 * @brief Calculate the new connection delay and timing cost of all the
 *        sink pins affected by moving a specific pin to a new location.
 *        Also calculates the total change in the timing cost.
 *
 * Assumes that the blocks have been moved to the proposed new locations.
 * Otherwise, the routine comp_td_single_connection_delay() will not be
 * able to calculate the most up to date connection delay estimation value.
 *
 * If the moved pin is a driver pin, then all the sink connections that are
 * driven by this driver pin are considered.
 *
 * If the moved pin is a sink pin, then it is the only pin considered. But
 * in some cases, the sink is already accounted for if it is also driven
 * by a driver pin located on a moved block. Computing it again would double
 * count its affect on the total timing cost change (delta_timing_cost).
 *
 * It is possible for some connections to have unchanged delays. For instance,
 * if we are using a dx/dy delay model, this could occur if a sink pin moved
 * to a new position with the same dx/dy from its net's driver pin.
 *
 * We skip these connections with unchanged delay values as their delay need
 * not be updated. Their timing costs also do not require any update, since
 * the criticalities values are always kept stale/unchanged during an block
 * swap attempt. (Unchanged Delay * Unchanged Criticality = Unchanged Cost)
 *
 * This is also done to minimize the number of timing node/edge invalidations
 * for incremental static timing analysis (incremental STA).
 */
static void update_td_delta_costs(const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities& criticalities,
                                  const ClusterNetId net,
                                  const ClusterPinId pin,
                                  t_pl_blocks_to_be_moved& blocks_affected,
                                  double& delta_timing_cost) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    const auto& connection_delay = g_placer_ctx.timing().connection_delay;
    auto& connection_timing_cost = g_placer_ctx.mutable_timing().connection_timing_cost;
    auto& proposed_connection_delay = g_placer_ctx.mutable_timing().proposed_connection_delay;
    auto& proposed_connection_timing_cost = g_placer_ctx.mutable_timing().proposed_connection_timing_cost;

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        /* This pin is a net driver on a moved block. */
        /* Recompute all point to point connection delays for the net sinks. */
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
            float temp_delay = comp_td_single_connection_delay(delay_model, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                continue;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;
            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net, ipin);
            blocks_affected.affected_pins.push_back(sink_pin);
        }
    } else {
        /* This pin is a net sink on a moved block */
        VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin) == PinType::SINK);

        /* Check if this sink's net is driven by a moved block */
        if (!driven_by_moved_block(net, blocks_affected)) {
            /* Get the sink pin index in the net */
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin);

            float temp_delay = comp_td_single_connection_delay(delay_model, net, ipin);
            /* If the delay hasn't changed, do not mark this pin as affected */
            if (temp_delay == connection_delay[net][ipin]) {
                return;
            }

            /* Calculate proposed delay and cost values */
            proposed_connection_delay[net][ipin] = temp_delay;
            proposed_connection_timing_cost[net][ipin] = criticalities.criticality(net, ipin) * temp_delay;
            delta_timing_cost += proposed_connection_timing_cost[net][ipin] - connection_timing_cost[net][ipin];

            /* Record this connection in blocks_affected.affected_pins */
            blocks_affected.affected_pins.push_back(pin);
        }
    }
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
        proposed_setup_slacks.push_back(setup_slacks->setup_slack(net_id, ipin));
    }

    //Sort in ascending order, from the worse slack value to the best
    std::sort(original_setup_slacks.begin(), original_setup_slacks.end());
    std::sort(proposed_setup_slacks.begin(), proposed_setup_slacks.end());

    //Check the first pair of slack values that are different
    //If found, return their difference
    for (size_t idiff = 0; idiff < original_setup_slacks.size(); ++idiff) {
        float slack_diff = original_setup_slacks[idiff] - proposed_setup_slacks[idiff];

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
    if (delta_c <= 0) {
        return ACCEPTED;
    }

    if (t == 0.) {
        return REJECTED;
    }

    float fnum = vtr::frand();
    float prob_fac = std::exp(-delta_c / t);
    if (prob_fac > fnum) {
        return ACCEPTED;
    }

    return REJECTED;
}

static double recompute_bb_cost() {
    /* Recomputes the cost to eliminate roundoff that may have accrued.  *
     * This routine does as little work as possible to compute this new  *
     * cost.                                                             */

    double cost = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Bounding boxes don't have to be recomputed; they're correct. */
            cost += net_cost[net_id];
        }
    }

    return (cost);
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
 * the ClusteredPinTimingInvalidator class.
 */
static void invalidate_affected_connections(const t_pl_blocks_to_be_moved& blocks_affected,
                                            ClusteredPinTimingInvalidator* pin_tedges_invalidator,
                                            TimingInfo* timing_info) {
    VTR_ASSERT_SAFE(timing_info);
    VTR_ASSERT_SAFE(pin_tedges_invalidator);

    /* Invalidate timing graph edges affected by the move */
    for (ClusterPinId pin : blocks_affected.affected_pins) {
        pin_tedges_invalidator->invalidate_connection(pin, timing_info);
    }
}

//Returns true if 'net' is driven by one of the blocks in 'blocks_affected'
static bool driven_by_moved_block(const ClusterNetId net, const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterBlockId net_driver_block = cluster_ctx.clb_nlist.net_driver_block(net);
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        if (net_driver_block == blocks_affected.moved_blocks[iblk].block_num) {
            return true;
        }
    }
    return false;
}

/* Finds the cost from scratch.  Done only when the placement   *
 * has been radically changed (i.e. after initial placement).   *
 * Otherwise find the cost change incrementally.  If method     *
 * check is NORMAL, we find bounding boxes that are updateable  *
 * for the larger nets.  If method is CHECK, all bounding boxes *
 * are found via the non_updateable_bb routine, to provide a    *
 * cost which can be used to check the correctness of the       *
 * other routine.                                               */
static double comp_bb_cost(e_cost_methods method) {
    double cost = 0;
    double expected_wirelength = 0.0;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {       /* for each net ... */
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            /* Small nets don't use incremental updating on their bounding boxes, *
             * so they can use a fast bounding box calculator.                    */
            if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == NORMAL) {
                get_bb_from_scratch(net_id, &bb_coords[net_id],
                                    &bb_num_on_edges[net_id]);
            } else {
                get_non_updateable_bb(net_id, &bb_coords[net_id]);
            }

            net_cost[net_id] = get_net_cost(net_id, &bb_coords[net_id]);
            cost += net_cost[net_id];
            if (method == CHECK)
                expected_wirelength += get_net_wirelength_estimate(net_id, &bb_coords[net_id]);
        }
    }

    if (method == CHECK) {
        VTR_LOG("\n");
        VTR_LOG("BB estimate of min-dist (placement) wire length: %.0f\n", expected_wirelength);
    }
    return cost;
}

/* Allocates the major structures needed only by the placer, primarily for *
 * computing costs quickly and such.                                       */
static void alloc_and_load_placement_structs(float place_cost_exp,
                                             const t_placer_opts& placer_opts,
                                             t_direct_inf* directs,
                                             int num_directs) {
    int max_pins_per_clb;
    unsigned int ipin;

    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    auto& p_timing_ctx = g_placer_ctx.mutable_timing();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    init_placement_context();

    max_pins_per_clb = 0;
    for (const auto& type : device_ctx.physical_tile_types) {
        max_pins_per_clb = max(max_pins_per_clb, type.num_pins);
    }

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /* Allocate structures associated with timing driven placement */
        /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]  */

        p_timing_ctx.connection_delay = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, 0.f);
        p_timing_ctx.proposed_connection_delay = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, 0.f);

        p_timing_ctx.connection_setup_slack = make_net_pins_matrix<float>(cluster_ctx.clb_nlist, std::numeric_limits<float>::infinity());

        p_timing_ctx.connection_timing_cost = PlacerTimingCosts(cluster_ctx.clb_nlist);
        p_timing_ctx.proposed_connection_timing_cost = make_net_pins_matrix<double>(cluster_ctx.clb_nlist, 0.);
        p_timing_ctx.net_timing_cost.resize(num_nets, 0.);

        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                p_timing_ctx.connection_delay[net_id][ipin] = 0;
                p_timing_ctx.proposed_connection_delay[net_id][ipin] = INVALID_DELAY;

                p_timing_ctx.proposed_connection_timing_cost[net_id][ipin] = INVALID_DELAY;

                if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) continue;

                p_timing_ctx.connection_timing_cost[net_id][ipin] = INVALID_DELAY;
            }
        }
    }

    net_cost.resize(num_nets, -1.);
    proposed_net_cost.resize(num_nets, -1.);
    bb_coords.resize(num_nets, t_bb());
    bb_num_on_edges.resize(num_nets, t_bb());

    /* Used to store costs for moves not yet made and to indicate when a net's   *
     * cost has been recomputed. proposed_net_cost[inet] < 0 means net's cost hasn't *
     * been recomputed.                                                          */
    bb_updated_before.resize(num_nets, NOT_UPDATED_YET);

    alloc_and_load_for_fast_cost_update(place_cost_exp);

    alloc_and_load_try_swap_structs();

    place_ctx.pl_macros = alloc_and_load_placement_macros(directs, num_directs);
}

/* Frees the major structures needed by the placer (and not needed       *
 * elsewhere).   */
static void free_placement_structs(const t_placer_opts& placer_opts) {
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

    vtr::release_memory(net_cost);
    vtr::release_memory(proposed_net_cost);
    vtr::release_memory(bb_coords);
    vtr::release_memory(bb_num_on_edges);

    vtr::release_memory(bb_updated_before);

    free_fast_cost_update();

    free_try_swap_structs();
}

static void alloc_and_load_try_swap_structs() {
    /* Allocate the local bb_coordinate storage, etc. only once. */
    /* Allocate with size cluster_ctx.clb_nlist.nets().size() for any number of nets affected. */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    ts_bb_coord_new.resize(num_nets, t_bb());
    ts_bb_edge_new.resize(num_nets, t_bb());
    ts_nets_to_update.resize(num_nets, ClusterNetId::INVALID());

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.compressed_block_grids = create_compressed_block_grids();
}

static void free_try_swap_structs() {
    vtr::release_memory(ts_bb_coord_new);
    vtr::release_memory(ts_bb_edge_new);
    vtr::release_memory(ts_nets_to_update);

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    vtr::release_memory(place_ctx.compressed_block_grids);
}

/* This routine finds the bounding box of each net from scratch (i.e.   *
 * from only the block location information).  It updates both the       *
 * coordinate and number of pins on each edge information.  It           *
 * should only be called when the bounding box information is not valid. */
static void get_bb_from_scratch(ClusterNetId net_id, t_bb* coords, t_bb* num_on_edges) {
    int pnum, x, y, xmin, xmax, ymin, ymax;
    int xmin_edge, xmax_edge, ymin_edge, ymax_edge;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);
    VTR_ASSERT(pnum >= 0);
    x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

    x = max(min<int>(x, grid.width() - 2), 1);
    y = max(min<int>(y, grid.height() - 2), 1);

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;
    xmin_edge = 1;
    ymin_edge = 1;
    xmax_edge = 1;
    ymax_edge = 1;

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

        /* Code below counts IO blocks as being within the 1..grid.width()-2, 1..grid.height()-2 clb array. *
         * This is because channels do not go out of the 0..grid.width()-2, 0..grid.height()-2 range, and   *
         * I always take all channels impinging on the bounding box to be within   *
         * that bounding box.  Hence, this "movement" of IO blocks does not affect *
         * the which channels are included within the bounding box, and it         *
         * simplifies the code a lot.                                              */

        x = max(min<int>(x, grid.width() - 2), 1);  //-2 for no perim channels
        y = max(min<int>(y, grid.height() - 2), 1); //-2 for no perim channels

        if (x == xmin) {
            xmin_edge++;
        }
        if (x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
            xmax_edge++;
        } else if (x < xmin) {
            xmin = x;
            xmin_edge = 1;
        } else if (x > xmax) {
            xmax = x;
            xmax_edge = 1;
        }

        if (y == ymin) {
            ymin_edge++;
        }
        if (y == ymax) {
            ymax_edge++;
        } else if (y < ymin) {
            ymin = y;
            ymin_edge = 1;
        } else if (y > ymax) {
            ymax = y;
            ymax_edge = 1;
        }
    }

    /* Copy the coordinates and number on edges information into the proper   *
     * structures.                                                            */
    coords->xmin = xmin;
    coords->xmax = xmax;
    coords->ymin = ymin;
    coords->ymax = ymax;

    num_on_edges->xmin = xmin_edge;
    num_on_edges->xmax = xmax_edge;
    num_on_edges->ymin = ymin_edge;
    num_on_edges->ymax = ymax_edge;
}

static double wirelength_crossing_count(size_t fanout) {
    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if (fanout > 50) {
        return 2.7933 + 0.02616 * (fanout - 50);
    } else {
        return cross_count[fanout - 1];
    }
}

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr) {
    /* WMF: Finds the estimate of wirelength due to one net by looking at   *
     * its coordinate bounding box.                                         */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bbptr->xmax - bbptr->xmin + 1) * crossing;

    ncost += (bbptr->ymax - bbptr->ymin + 1) * crossing;

    return (ncost);
}

static double get_net_cost(ClusterNetId net_id, t_bb* bbptr) {
    /* Finds the cost due to one net by looking at its coordinate bounding  *
     * box.                                                                 */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    crossing = wirelength_crossing_count(cluster_ctx.clb_nlist.net_pins(net_id).size());

    /* Could insert a check for xmin == xmax.  In that case, assume  *
     * connection will be made with no bends and hence no x-cost.    *
     * Same thing for y-cost.                                        */

    /* Cost = wire length along channel * cross_count / average      *
     * channel capacity.   Do this for x, then y direction and add.  */

    ncost = (bbptr->xmax - bbptr->xmin + 1) * crossing
            * chanx_place_cost_fac[bbptr->ymax][bbptr->ymin - 1];

    ncost += (bbptr->ymax - bbptr->ymin + 1) * crossing
             * chany_place_cost_fac[bbptr->xmax][bbptr->xmin - 1];

    return (ncost);
}

/* Finds the bounding box of a net and stores its coordinates in the  *
 * bb_coord_new data structure.  This routine should only be called   *
 * for small nets, since it does not determine enough information for *
 * the bounding box to be updated incrementally later.                *
 * Currently assumes channels on both sides of the CLBs forming the   *
 * edges of the bounding box can be used.  Essentially, I am assuming *
 * the pins always lie on the outside of the bounding box.            */
static void get_non_updateable_bb(ClusterNetId net_id, t_bb* bb_coord_new) {
    //TODO: account for multiple physical pin instances per logical pin

    int xmax, ymax, xmin, ymin, x, y;
    int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
    pnum = net_pin_to_tile_pin_index(net_id, 0);
    x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = tile_pin_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x + physical_tile_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y + physical_tile_type(bnum)->pin_height_offset[pnum];

        if (x < xmin) {
            xmin = x;
        } else if (x > xmax) {
            xmax = x;
        }

        if (y < ymin) {
            ymin = y;
        } else if (y > ymax) {
            ymax = y;
        }
    }

    /* Now I've found the coordinates of the bounding box.  There are no *
     * channels beyond device_ctx.grid.width()-2 and                     *
     * device_ctx.grid.height() - 2, so I want to clip to that.  As well,*
     * since I'll always include the channel immediately below and the   *
     * channel immediately to the left of the bounding box, I want to    *
     * clip to 1 in both directions as well (since minimum channel index *
     * is 0).  See route_common.cpp for a channel diagram.               */

    bb_coord_new->xmin = max(min<int>(xmin, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new->ymin = max(min<int>(ymin, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    bb_coord_new->xmax = max(min<int>(xmax, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    bb_coord_new->ymax = max(min<int>(ymax, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
}

static void update_bb(ClusterNetId net_id, t_bb* bb_coord_new, t_bb* bb_edge_new, int xold, int yold, int xnew, int ynew) {
    /* Updates the bounding box of a net by storing its coordinates in    *
     * the bb_coord_new data structure and the number of blocks on each   *
     * edge in the bb_edge_new data structure.  This routine should only  *
     * be called for large nets, since it has some overhead relative to   *
     * just doing a brute force bounding box calculation.  The bounding   *
     * box coordinate and edge information for inet must be valid before  *
     * this routine is called.                                            *
     * Currently assumes channels on both sides of the CLBs forming the   *
     * edges of the bounding box can be used.  Essentially, I am assuming *
     * the pins always lie on the outside of the bounding box.            *
     * The x and y coordinates are the pin's x and y coordinates.         */
    /* IO blocks are considered to be one cell in for simplicity.         */
    //TODO: account for multiple physical pin instances per logical pin

    t_bb *curr_bb_edge, *curr_bb_coord;

    auto& device_ctx = g_vpr_ctx.device();

    xnew = max(min<int>(xnew, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    ynew = max(min<int>(ynew, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
    xold = max(min<int>(xold, device_ctx.grid.width() - 2), 1);  //-2 for no perim channels
    yold = max(min<int>(yold, device_ctx.grid.height() - 2), 1); //-2 for no perim channels

    /* Check if the net had been updated before. */
    if (bb_updated_before[net_id] == GOT_FROM_SCRATCH) {
        /* The net had been updated from scratch, DO NOT update again! */
        return;
    } else if (bb_updated_before[net_id] == NOT_UPDATED_YET) {
        /* The net had NOT been updated before, could use the old values */
        curr_bb_coord = &bb_coords[net_id];
        curr_bb_edge = &bb_num_on_edges[net_id];
        bb_updated_before[net_id] = UPDATED_ONCE;
    } else {
        /* The net had been updated before, must use the new values */
        curr_bb_coord = bb_coord_new;
        curr_bb_edge = bb_edge_new;
    }

    /* Check if I can update the bounding box incrementally. */

    if (xnew < xold) { /* Move to left. */

        /* Update the xmax fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord->xmax) { /* Old position at xmax. */
            if (curr_bb_edge->xmax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->xmax = curr_bb_edge->xmax - 1;
                bb_coord_new->xmax = curr_bb_coord->xmax;
            }
        } else { /* Move to left, old postion was not at xmax. */
            bb_coord_new->xmax = curr_bb_coord->xmax;
            bb_edge_new->xmax = curr_bb_edge->xmax;
        }

        /* Now do the xmin fields for coordinates and number of edges. */

        if (xnew < curr_bb_coord->xmin) { /* Moved past xmin */
            bb_coord_new->xmin = xnew;
            bb_edge_new->xmin = 1;
        } else if (xnew == curr_bb_coord->xmin) { /* Moved to xmin */
            bb_coord_new->xmin = xnew;
            bb_edge_new->xmin = curr_bb_edge->xmin + 1;
        } else { /* Xmin unchanged. */
            bb_coord_new->xmin = curr_bb_coord->xmin;
            bb_edge_new->xmin = curr_bb_edge->xmin;
        }
        /* End of move to left case. */

    } else if (xnew > xold) { /* Move to right. */

        /* Update the xmin fields for coordinates and number of edges first. */

        if (xold == curr_bb_coord->xmin) { /* Old position at xmin. */
            if (curr_bb_edge->xmin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->xmin = curr_bb_edge->xmin - 1;
                bb_coord_new->xmin = curr_bb_coord->xmin;
            }
        } else { /* Move to right, old position was not at xmin. */
            bb_coord_new->xmin = curr_bb_coord->xmin;
            bb_edge_new->xmin = curr_bb_edge->xmin;
        }

        /* Now do the xmax fields for coordinates and number of edges. */

        if (xnew > curr_bb_coord->xmax) { /* Moved past xmax. */
            bb_coord_new->xmax = xnew;
            bb_edge_new->xmax = 1;
        } else if (xnew == curr_bb_coord->xmax) { /* Moved to xmax */
            bb_coord_new->xmax = xnew;
            bb_edge_new->xmax = curr_bb_edge->xmax + 1;
        } else { /* Xmax unchanged. */
            bb_coord_new->xmax = curr_bb_coord->xmax;
            bb_edge_new->xmax = curr_bb_edge->xmax;
        }
        /* End of move to right case. */

    } else { /* xnew == xold -- no x motion. */
        bb_coord_new->xmin = curr_bb_coord->xmin;
        bb_coord_new->xmax = curr_bb_coord->xmax;
        bb_edge_new->xmin = curr_bb_edge->xmin;
        bb_edge_new->xmax = curr_bb_edge->xmax;
    }

    /* Now account for the y-direction motion. */

    if (ynew < yold) { /* Move down. */

        /* Update the ymax fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord->ymax) { /* Old position at ymax. */
            if (curr_bb_edge->ymax == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->ymax = curr_bb_edge->ymax - 1;
                bb_coord_new->ymax = curr_bb_coord->ymax;
            }
        } else { /* Move down, old postion was not at ymax. */
            bb_coord_new->ymax = curr_bb_coord->ymax;
            bb_edge_new->ymax = curr_bb_edge->ymax;
        }

        /* Now do the ymin fields for coordinates and number of edges. */

        if (ynew < curr_bb_coord->ymin) { /* Moved past ymin */
            bb_coord_new->ymin = ynew;
            bb_edge_new->ymin = 1;
        } else if (ynew == curr_bb_coord->ymin) { /* Moved to ymin */
            bb_coord_new->ymin = ynew;
            bb_edge_new->ymin = curr_bb_edge->ymin + 1;
        } else { /* ymin unchanged. */
            bb_coord_new->ymin = curr_bb_coord->ymin;
            bb_edge_new->ymin = curr_bb_edge->ymin;
        }
        /* End of move down case. */

    } else if (ynew > yold) { /* Moved up. */

        /* Update the ymin fields for coordinates and number of edges first. */

        if (yold == curr_bb_coord->ymin) { /* Old position at ymin. */
            if (curr_bb_edge->ymin == 1) {
                get_bb_from_scratch(net_id, bb_coord_new, bb_edge_new);
                bb_updated_before[net_id] = GOT_FROM_SCRATCH;
                return;
            } else {
                bb_edge_new->ymin = curr_bb_edge->ymin - 1;
                bb_coord_new->ymin = curr_bb_coord->ymin;
            }
        } else { /* Moved up, old position was not at ymin. */
            bb_coord_new->ymin = curr_bb_coord->ymin;
            bb_edge_new->ymin = curr_bb_edge->ymin;
        }

        /* Now do the ymax fields for coordinates and number of edges. */

        if (ynew > curr_bb_coord->ymax) { /* Moved past ymax. */
            bb_coord_new->ymax = ynew;
            bb_edge_new->ymax = 1;
        } else if (ynew == curr_bb_coord->ymax) { /* Moved to ymax */
            bb_coord_new->ymax = ynew;
            bb_edge_new->ymax = curr_bb_edge->ymax + 1;
        } else { /* ymax unchanged. */
            bb_coord_new->ymax = curr_bb_coord->ymax;
            bb_edge_new->ymax = curr_bb_edge->ymax;
        }
        /* End of move up case. */

    } else { /* ynew == yold -- no y motion. */
        bb_coord_new->ymin = curr_bb_coord->ymin;
        bb_coord_new->ymax = curr_bb_coord->ymax;
        bb_edge_new->ymin = curr_bb_edge->ymin;
        bb_edge_new->ymax = curr_bb_edge->ymax;
    }

    if (bb_updated_before[net_id] == NOT_UPDATED_YET) {
        bb_updated_before[net_id] = UPDATED_ONCE;
    }
}

static void free_fast_cost_update() {
    auto& device_ctx = g_vpr_ctx.device();

    for (size_t i = 0; i < device_ctx.grid.height(); i++) {
        free(chanx_place_cost_fac[i]);
    }
    free(chanx_place_cost_fac);
    chanx_place_cost_fac = nullptr;

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        free(chany_place_cost_fac[i]);
    }
    free(chany_place_cost_fac);
    chany_place_cost_fac = nullptr;
}

static void alloc_and_load_for_fast_cost_update(float place_cost_exp) {
    /* Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac *
     * arrays with the inverse of the average number of tracks per channel   *
     * between [subhigh] and [sublow].  This is only useful for the cost     *
     * function that takes the length of the net bounding box in each        *
     * dimension divided by the average number of tracks in that direction.  *
     * For other cost functions, you don't have to bother calling this       *
     * routine; when using the cost function described above, however, you   *
     * must always call this routine after you call init_chan and before     *
     * you do any placement cost determination.  The place_cost_exp factor   *
     * specifies to what power the width of the channel should be taken --   *
     * larger numbers make narrower channels more expensive.                 */

    auto& device_ctx = g_vpr_ctx.device();

    /* Access arrays below as chan?_place_cost_fac[subhigh][sublow].  Since   *
     * subhigh must be greater than or equal to sublow, we only need to       *
     * allocate storage for the lower half of a matrix.                       */

    chanx_place_cost_fac = (float**)vtr::malloc((device_ctx.grid.height()) * sizeof(float*));
    for (size_t i = 0; i < device_ctx.grid.height(); i++)
        chanx_place_cost_fac[i] = (float*)vtr::malloc((i + 1) * sizeof(float));

    chany_place_cost_fac = (float**)vtr::malloc((device_ctx.grid.width() + 1) * sizeof(float*));
    for (size_t i = 0; i < device_ctx.grid.width(); i++)
        chany_place_cost_fac[i] = (float*)vtr::malloc((i + 1) * sizeof(float));

    /* First compute the number of tracks between channel high and channel *
     * low, inclusive, in an efficient manner.                             */

    chanx_place_cost_fac[0][0] = device_ctx.chan_width.x_list[0];

    for (size_t high = 1; high < device_ctx.grid.height(); high++) {
        chanx_place_cost_fac[high][high] = device_ctx.chan_width.x_list[high];
        for (size_t low = 0; low < high; low++) {
            chanx_place_cost_fac[high][low] = chanx_place_cost_fac[high - 1][low] + device_ctx.chan_width.x_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  The cost function divides by the average     *
     * number of tracks per channel, so by storing the inverse I convert   *
     * this to a faster multiplication.  Take this final number to the     *
     * place_cost_exp power -- numbers other than one mean this is no      *
     * longer a simple "average number of tracks"; it is some power of     *
     * that, allowing greater penalization of narrow channels.             */

    for (size_t high = 0; high < device_ctx.grid.height(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chanx_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANX place cost fac is 0 at %d %d\n", high, low);
                chanx_place_cost_fac[high][low] = 1.0f;
            }

            chanx_place_cost_fac[high][low] = (high - low + 1.)
                                              / chanx_place_cost_fac[high][low];
            chanx_place_cost_fac[high][low] = pow((double)chanx_place_cost_fac[high][low], (double)place_cost_exp);
        }

    /* Now do the same thing for the y-directed channels.  First get the  *
     * number of tracks between channel high and channel low, inclusive.  */

    chany_place_cost_fac[0][0] = device_ctx.chan_width.y_list[0];

    for (size_t high = 1; high < device_ctx.grid.width(); high++) {
        chany_place_cost_fac[high][high] = device_ctx.chan_width.y_list[high];
        for (size_t low = 0; low < high; low++) {
            chany_place_cost_fac[high][low] = chany_place_cost_fac[high - 1][low] + device_ctx.chan_width.y_list[high];
        }
    }

    /* Now compute the inverse of the average number of tracks per channel *
     * between high and low.  Take to specified power.                     */

    for (size_t high = 0; high < device_ctx.grid.width(); high++)
        for (size_t low = 0; low <= high; low++) {
            /* Since we will divide the wiring cost by the average channel *
             * capacity between high and low, having only 0 width channels *
             * will result in infinite wiring capacity normalization       *
             * factor, and extremely bad placer behaviour. Hence we change *
             * this to a small (1 track) channel capacity instead.         */
            if (chany_place_cost_fac[high][low] == 0.0f) {
                VTR_LOG_WARN("CHANY place cost fac is 0 at %d %d\n", high, low);
                chany_place_cost_fac[high][low] = 1.0f;
            }

            chany_place_cost_fac[high][low] = (high - low + 1.)
                                              / chany_place_cost_fac[high][low];
            chany_place_cost_fac[high][low] = pow((double)chany_place_cost_fac[high][low], (double)place_cost_exp);
        }
}

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        const t_place_algorithm& place_algorithm) {
    /* Checks that the placement has not confused our data structures. *
     * i.e. the clb and block structures agree about the locations of  *
     * every block, blocks are in legal spots, etc.  Also recomputes   *
     * the final placement cost from scratch and makes sure it is      *
     * within roundoff of what we think the cost is.                   */

    int error = 0;

    error += check_placement_consistency();
    error += check_placement_costs(costs, delay_model, criticalities, place_algorithm);

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

    bb_cost_check = comp_bb_cost(CHECK);
    if (fabs(bb_cost_check - costs.bb_cost) > costs.bb_cost * ERROR_TOL) {
        VTR_LOG_ERROR("bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
                      bb_cost_check, costs.bb_cost);
        error++;
    }

    if (place_algorithm.is_timing_driven()) {
        comp_td_costs(delay_model, *criticalities, &timing_cost_check);
        //VTR_LOG("timing_cost recomputed from scratch: %g\n", timing_cost_check);
        if (fabs(timing_cost_check - costs.timing_cost) > costs.timing_cost * ERROR_TOL) {
            VTR_LOG_ERROR("timing_cost_check: %g and timing_cost: %g differ in check_place.\n",
                          timing_cost_check, costs.timing_cost);
            error++;
        }
    }
    return error;
}

static int check_placement_consistency() {
    return check_block_placement_consistency() + check_macro_placement_consistency();
}

static int check_block_placement_consistency() {
    int error = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    vtr::vector<ClusterBlockId, int> bdone(cluster_ctx.clb_nlist.blocks().size(), 0);

    /* Step through device grid and placement. Check it against blocks */
    for (size_t i = 0; i < device_ctx.grid.width(); i++)
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            if (place_ctx.grid_blocks[i][j].usage > device_ctx.grid[i][j].type->capacity) {
                VTR_LOG_ERROR("Block at grid location (%zu,%zu) overused. Usage is %d.\n",
                              i, j, place_ctx.grid_blocks[i][j].usage);
                error++;
            }
            int usage_check = 0;
            for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
                auto bnum = place_ctx.grid_blocks[i][j].blocks[k];
                if (EMPTY_BLOCK_ID == bnum || INVALID_BLOCK_ID == bnum)
                    continue;

                auto logical_block = cluster_ctx.clb_nlist.block_type(bnum);
                auto physical_tile = device_ctx.grid[i][j].type;

                if (physical_tile_type(bnum) != physical_tile) {
                    VTR_LOG_ERROR("Block %zu type (%s) does not match grid location (%zu,%zu) type (%s).\n",
                                  size_t(bnum), logical_block->name, i, j, physical_tile->name);
                    error++;
                }

                auto& loc = place_ctx.block_locs[bnum].loc;
                if (loc.x != int(i) || loc.y != int(j) || !is_sub_tile_compatible(physical_tile, logical_block, loc.sub_tile)) {
                    VTR_LOG_ERROR("Block %zu's location is (%d,%d,%d) but found in grid at (%zu,%zu,%d).\n",
                                  size_t(bnum), loc.x, loc.y, loc.sub_tile,
                                  i, j, k);
                    error++;
                }
                ++usage_check;
                bdone[bnum]++;
            }
            if (usage_check != place_ctx.grid_blocks[i][j].usage) {
                VTR_LOG_ERROR("Location (%zu,%zu) usage is %d, but has actual usage %d.\n",
                              i, j, place_ctx.grid_blocks[i][j].usage, usage_check);
                error++;
            }
        }

    /* Check that every block exists in the device_ctx.grid and cluster_ctx.blocks arrays somewhere. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        if (bdone[blk_id] != 1) {
            VTR_LOG_ERROR("Block %zu listed %d times in data structures.\n",
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

        for (size_t imember = 0; imember < pl_macros[imacro].members.size(); imember++) {
            auto member_iblk = pl_macros[imacro].members[imember].blk_index;

            // Compute the suppossed member's x,y,z location
            t_pl_loc member_pos = place_ctx.block_locs[head_iblk].loc + pl_macros[imacro].members[imember].offset;

            // Check the place_ctx.block_locs data structure first
            if (place_ctx.block_locs[member_iblk].loc != member_pos) {
                VTR_LOG_ERROR("Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                              size_t(member_iblk), imacro);
                error++;
            }

            // Then check the place_ctx.grid data structure
            if (place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.sub_tile] != member_iblk) {
                VTR_LOG_ERROR("Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                              size_t(member_iblk), imacro);
                error++;
            }
        } // Finish going through all the members
    }     // Finish going through all the macros
    return error;
}

#ifdef VERBOSE
static void print_clb_placement(const char* fname) {
    /* Prints out the clb placements to a file.  */
    FILE* fp;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    fp = vtr::fopen(fname, "w");
    fprintf(fp, "Complex block placements:\n\n");

    fprintf(fp, "Block #\tName\t(X, Y, Z).\n");
    for (auto i : cluster_ctx.clb_nlist.blocks()) {
        fprintf(fp, "#%d\t%s\t(%d, %d, %d).\n", i, cluster_ctx.clb_nlist.block_name(i), place_ctx.block_locs[i].x, place_ctx.block_locs[i].y, place_ctx.block_locs[i].sub_tile);
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
                                               const PlacementDelayCalculator& delay_calc) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, delay_calc);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_setup(placer_opts.post_place_timing_report_file, *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
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

static void print_place_status_header() {
    VTR_LOG("---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
    VTR_LOG("Tnum   Time       T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha\n");
    VTR_LOG("      (sec)                                          (ns)       (ns)     (ns)                                                 \n");
    VTR_LOG("---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
}

static void print_place_status(const t_annealing_state& state,
                               const t_placer_statistics& stats,
                               float elapsed_sec,
                               float cpd,
                               float sTNS,
                               float sWNS,
                               size_t tot_moves) {
    VTR_LOG(
        "%4zu "
        "%6.1f "
        "%7.1e "
        "%7.3f %10.2f %-10.5g "
        "%7.3f % 10.3g % 8.3f "
        "%7.3f %7.4f %6.1f %8.2f",
        state.num_temps,
        elapsed_sec,
        state.t,
        stats.av_cost, stats.av_bb_cost, stats.av_timing_cost,
        1e9 * cpd, 1e9 * sTNS, 1e9 * sWNS,
        stats.success_rate, stats.std_dev, state.rlim, state.crit_exponent);

    pretty_print_uint(" ", tot_moves, 9, 3);

    VTR_LOG(" %6.3f\n", state.alpha);
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
    std::map<t_logical_block_type_ptr, std::map<t_physical_tile_type_ptr, size_t>> num_placed_instances;
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto block_loc = place_ctx.block_locs[blk_id];
        auto loc = block_loc.loc;

        auto physical_tile = device_ctx.grid[loc.x][loc.y].type;
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        num_type_instances[logical_block]++;
        num_placed_instances[logical_block][physical_tile]++;

        max_block_name = std::max<int>(max_block_name, strlen(logical_block->name));
        max_tile_name = std::max<int>(max_tile_name, strlen(physical_tile->name));
    }

    VTR_LOG("\n");
    VTR_LOG("Placement resource usage:\n");
    for (auto logical_block : num_type_instances) {
        for (auto physical_tile : num_placed_instances[logical_block.first]) {
            VTR_LOG("  %-*s implemented as %-*s: %d\n", max_block_name, logical_block.first->name, max_tile_name, physical_tile.first->name, physical_tile.second);
        }
    }
    VTR_LOG("\n");
}

bool placer_needs_lookahead(const t_vpr_setup& vpr_setup) {
    return (vpr_setup.PlacerOpts.place_algorithm.is_timing_driven());
}
