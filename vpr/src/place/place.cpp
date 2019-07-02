#include <cstdio>
#include <cmath>
#include <memory>
#include <fstream>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_geometry.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "read_place.h"
#include "draw.h"
#include "place_and_route.h"
#include "net_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "vpr_utils.h"
#include "place_macro.h"
#include "histogram.h"
#include "place_util.h"
#include "place_delay_model.h"

#include "PlacementDelayCalculator.h"
#include "VprTimingGraphResolver.h"
#include "timing_util.h"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

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

/* The maximum number of tries when trying to place a carry chain at a    *
 * random location before trying exhaustive placement - find the fist     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 4

/* Flags for the states of the bounding box.                              *
 * Stored as char for memory efficiency.                                  */
#define NOT_UPDATED_YET 'N'
#define UPDATED_ONCE 'U'
#define GOT_FROM_SCRATCH 'S'

/* For comp_cost.  NORMAL means use the method that generates updateable  *
 * bounding boxes for speed.  CHECK means compute all bounding boxes from *
 * scratch using a very simple routine to allow checks of the other       *
 * costs.                                                                 */
enum e_cost_methods {
    NORMAL,
    CHECK
};

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum e_swap_result {
    REJECTED,
    ACCEPTED,
    ABORTED
};

enum class e_propose_move {
    VALID, //Move successful and legal
    ABORT, //Unable to perform move
};

enum class e_find_affected_blocks_result {
    VALID,       //Move successful
    ABORT,       //Unable to perform move
    INVERT,      //Try move again but with from/to inverted
    INVERT_VALID //Completed inverted move
};

struct t_placer_statistics {
    double av_cost, av_bb_cost, av_timing_cost,
        sum_of_squares;
    int success_sum;
};

struct t_placer_costs {
    //Although we do nost cost calculations with float's we
    //use doubles for the accumulated costs to avoid round-off,
    //particularly on large designs where the magnitude of a single
    //move's delta cost is small compared to the overall cost.
    double cost;
    double bb_cost;
    double timing_cost;
};

struct t_placer_prev_inverse_costs {
    double bb_cost;
    double timing_cost;
};

constexpr float INVALID_DELAY = std::numeric_limits<float>::quiet_NaN();

constexpr double MAX_INV_TIMING_COST = 1.e9;
/* Stops inverse timing cost from going to infinity with very lax timing constraints,
 * which avoids multiplying by a gigantic prev_inverse.timing_cost when auto-normalizing.
 * The exact value of this cost has relatively little impact, but should not be
 * large enough to be on the order of timing costs for normal constraints. */

//Define to log and print debug info about aborted moves
#define DEBUG_ABORTED_MOVES

/********************** Variables local to place.c ***************************/

/* Cost of a net, and a temporary cost of a net used during move assessment. */
static vtr::vector<ClusterNetId, double> net_cost, temp_net_cost;

static t_pl_loc** legal_pos = nullptr; /* [0..device_ctx.num_block_types-1][0..type_tsize - 1] */
static int* num_legal_pos = nullptr;   /* [0..num_legal_pos-1] */

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

/* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]. What is the value of the timing   */
/* driven portion of the cost function. These arrays will be set to  */
/* (criticality * delay) for each point to point connection. */

static vtr::vector<ClusterNetId, double*> point_to_point_timing_cost;
static vtr::vector<ClusterNetId, double*> temp_point_to_point_timing_cost;

/* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]. What is the value of the delay */
/* for each connection in the circuit */
static vtr::vector<ClusterNetId, float*> point_to_point_delay;
static vtr::vector<ClusterNetId, float*> temp_point_to_point_delay;

/* [0..cluster_ctx.clb_nlist.blocks().size()-1][0..pins_per_clb-1]. Indicates which pin on the net */
/* this block corresponds to, this is only required during timing-driven */
/* placement. It is used to allow us to update individual connections on */
/* each net */
static vtr::vector<ClusterBlockId, std::vector<int>> net_pin_indices;

/* [0..cluster_ctx.clb_nlist.nets().size()-1].  Store the bounding box coordinates and the number of    *
 * blocks on each of a net's bounding box (to allow efficient updates),      *
 * respectively.                                                             */

static vtr::vector<ClusterNetId, t_bb> bb_coords, bb_num_on_edges;

/* Store the information on the blocks to be moved in a swap during     *
 * placement, in the form of array of structs instead of struct with    *
 * arrays for cache effifiency                                          *
 */
static t_pl_blocks_to_be_moved blocks_affected;

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

extern vtr::vector<ClusterNetId, float*> f_timing_place_crit; //TODO: encapsulate better

static std::map<std::string, size_t> f_move_abort_reasons;

struct t_type_loc {
    int x = OPEN;
    int y = OPEN;

    t_type_loc(int x_val, int y_val)
        : x(x_val)
        , y(y_val) {}

    //Returns true if this type location has valid x/y values
    operator bool() const {
        return !(x == OPEN || y == OPEN);
    }
};

struct t_compressed_block_grid {
    //If 'cx' is an index in the compressed grid space, then
    //'compressed_to_grid_x[cx]' is the corresponding location in the
    //full (uncompressed) device grid.
    std::vector<int> compressed_to_grid_x;
    std::vector<int> compressed_to_grid_y;

    //The grid is stored with a full/dense x-dimension (since only
    //x values which exist are considered), while the y-dimension is
    //stored sparsely, since we may not have full columns of blocks.
    //This makes it easy to check whether there exist
    std::vector<vtr::flat_map2<int, t_type_loc>> grid;
};

//Compressed grid space for each block type
//Used to efficiently find logically 'adjacent' blocks of the same block type even though
//the may be physically far apart
static std::vector<t_compressed_block_grid> f_compressed_block_grids;

/********************* Static subroutines local to place.c *******************/
#ifdef VERBOSE
static void print_clb_placement(const char* fname);
#endif

static void alloc_and_load_placement_structs(float place_cost_exp,
                                             t_placer_opts placer_opts,
                                             t_direct_inf* directs,
                                             int num_directs);

static void alloc_and_load_net_pin_indices();

static void alloc_and_load_try_swap_structs();

static std::vector<t_compressed_block_grid> create_compressed_block_grids();

static t_compressed_block_grid create_compressed_block_grid(const std::vector<vtr::Point<int>>& locations);

static void free_placement_structs(t_placer_opts placer_opts);

static void alloc_and_load_for_fast_cost_update(float place_cost_exp);

static void free_fast_cost_update();

static void alloc_legal_placements();
static void load_legal_placements();

static void free_legal_placements();

static int check_macro_can_be_placed(int imacro, int itype, t_pl_loc head_pos);

static int try_place_macro(int itype, int ipos, int imacro);

static void initial_placement_pl_macros(int macros_max_num_tries, int* free_locations);

static void initial_placement_blocks(int* free_locations, enum e_pad_loc_type pad_loc_type);
static void initial_placement_location(const int* free_locations, ClusterBlockId blk_id, int& pipos, t_pl_loc& to);

static void initial_placement(enum e_pad_loc_type pad_loc_type,
                              const char* pad_loc_file);

static double comp_bb_cost(e_cost_methods method);

static void apply_move_blocks();
static void revert_move_blocks();
static void commit_move_blocks();
static void clear_move_blocks();

static void update_move_nets(int num_nets_affected);
static void reset_move_nets(int num_nets_affected);

static e_find_affected_blocks_result record_single_block_swap(ClusterBlockId b_from, t_pl_loc to);
static e_find_affected_blocks_result record_block_move(ClusterBlockId blk, t_pl_loc to);

static e_propose_move propose_move(ClusterBlockId b_from, t_pl_loc to);
static e_find_affected_blocks_result find_affected_blocks(ClusterBlockId b_from, t_pl_loc to);

static e_find_affected_blocks_result record_macro_swaps(const int imacro_from, int& imember_from, t_pl_offset swap_offset);
static e_find_affected_blocks_result record_macro_macro_swaps(const int imacro_from, int& imember_from, const int imacro_to, ClusterBlockId blk_to, t_pl_offset swap_offset);

static e_find_affected_blocks_result record_macro_move(std::vector<ClusterBlockId>& displaced_blocks,
                                                       const int imacro,
                                                       t_pl_offset swap_offset);
static e_find_affected_blocks_result identify_macro_self_swap_affected_macros(std::vector<int>& macros, const int imacro, t_pl_offset swap_offset);
static e_find_affected_blocks_result record_macro_self_swaps(const int imacro, t_pl_offset swap_offset);

bool is_legal_swap_to_location(ClusterBlockId blk, t_pl_loc to);

std::set<t_pl_loc> determine_locations_emptied_by_move();

static e_swap_result try_swap(float t,
                              t_placer_costs* costs,
                              t_placer_prev_inverse_costs* prev_inverse_costs,
                              float rlim,
                              const PlaceDelayModel& delay_model,
                              float rlim_escape_fraction,
                              enum e_place_algorithm place_algorithm,
                              float timing_tradeoff);

static ClusterBlockId pick_from_block();

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel& delay_model,
                        enum e_place_algorithm place_algorithm);

static int check_placement_costs(const t_placer_costs& costs,
                                 const PlaceDelayModel& delay_model,
                                 enum e_place_algorithm place_algorithm);
static int check_placement_consistency();
static int check_block_placement_consistency();
static int check_macro_placement_consistency();

static float starting_t(t_placer_costs* costs,
                        t_placer_prev_inverse_costs* prev_inverse_costs,
                        t_annealing_sched annealing_sched,
                        int max_moves,
                        float rlim,
                        const PlaceDelayModel& delay_model,
                        const t_placer_opts& placer_opts);

static void update_t(float* t, float rlim, float success_rat, t_annealing_sched annealing_sched);

static void update_rlim(float* rlim, float success_rat, const DeviceGrid& grid);

static int exit_crit(float t, float cost, t_annealing_sched annealing_sched);

static int count_connections();

static double get_std_dev(int n, double sum_x_squared, double av_x);

static double recompute_bb_cost();

static float comp_td_point_to_point_delay(const PlaceDelayModel& delay_model, ClusterNetId net_id, int ipin);

static void comp_td_point_to_point_delays(const PlaceDelayModel& delay_model);

static void update_td_cost();

static bool driven_by_moved_block(const ClusterNetId net);

static void comp_td_costs(const PlaceDelayModel& delay_model, double* timing_cost);

static e_swap_result assess_swap(double delta_c, double t);

static bool find_to(t_type_ptr type, float rlim, const t_pl_loc from, t_pl_loc& to);

static void get_non_updateable_bb(ClusterNetId net_id, t_bb* bb_coord_new);

static void update_bb(ClusterNetId net_id, t_bb* bb_coord_new, t_bb* bb_edge_new, int xold, int yold, int xnew, int ynew);

static int find_affected_nets_and_update_costs(e_place_algorithm place_algorithm, const PlaceDelayModel& delay_model, double& bb_delta_c, double& timing_delta_c);

static void record_affected_net(const ClusterNetId net, int& num_affected_nets);

static void update_net_bb(const ClusterNetId net, int iblk, const ClusterBlockId blk, const ClusterPinId blk_pin);
static void update_td_delta_costs(const PlaceDelayModel& delay_model, const ClusterNetId net, const ClusterPinId pin, double& delta_timing_cost);

static double get_net_cost(ClusterNetId net_id, t_bb* bb_ptr);

static void get_bb_from_scratch(ClusterNetId net_id, t_bb* coords, t_bb* num_on_edges);

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr);

static void free_try_swap_arrays();

static void outer_loop_recompute_criticalities(t_placer_opts placer_opts,
                                               t_placer_costs* costs,
                                               t_placer_prev_inverse_costs* prev_inverse_costs,
                                               int num_connections,
                                               float crit_exponent,
                                               int* outer_crit_iter_count,
                                               const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                               const PlaceDelayModel& delay_model,
                                               SetupTimingInfo& timing_info);

static void placement_inner_loop(float t,
                                 float rlim,
                                 t_placer_opts placer_opts,
                                 int move_lim,
                                 float crit_exponent,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 t_placer_prev_inverse_costs* prev_inverse_costs,
                                 int* moves_since_cost_recompute,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 const PlaceDelayModel& delay_model,
                                 SetupTimingInfo& timing_info);

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts, const PlaceDelayModel& delay_model, t_placer_costs* costs);

static void calc_placer_stats(t_placer_statistics& stats, float& success_rat, double& std_dev, const t_placer_costs& costs, const int move_lim);

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc);

static void log_move_abort(std::string reason);
static void report_aborted_moves();
static int grid_to_compressed(const std::vector<int>& coords, int point);

static void print_place_status_header();
static void print_place_status(const float t,
                               const float oldt,
                               const t_placer_statistics& stats,
                               const float cpd,
                               const float sTNS,
                               const float sWNS,
                               const float acc_rate,
                               const float std_dev,
                               const float rlim,
                               const float crit_exponent,
                               size_t tot_moves);

/*****************************************************************************/
void try_place(t_placer_opts placer_opts,
               t_annealing_sched annealing_sched,
               t_router_opts router_opts,
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

    int tot_iter, move_lim, moves_since_cost_recompute, width_fac, num_connections,
        outer_crit_iter_count, inner_recompute_limit;
    float t, success_rat, rlim,
        oldt = 0, crit_exponent,
        first_rlim, final_rlim, inverse_delta_rlim;

    t_placer_costs costs;
    t_placer_prev_inverse_costs prev_inverse_costs;

    tatum::TimingPathInfo critical_path;
    float sTNS = NAN;
    float sWNS = NAN;

    double std_dev;
    char msg[vtr::bufsize];
    t_placer_statistics stats;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::shared_ptr<SetupTimingInfo> timing_info;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc;
    std::unique_ptr<PlaceDelayModel> place_delay_model;

    /* Allocated here because it goes into timing critical code where each memory allocation is expensive */
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.block_types, device_ctx.num_block_types);

    /* init file scope variables */
    num_swap_rejected = 0;
    num_swap_accepted = 0;
    num_swap_aborted = 0;
    num_ts_called = 0;

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
        || placer_opts.enable_timing_computations) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = alloc_lookups_and_criticalities(chan_width_dist, placer_opts, router_opts, det_routing_arch, segment_inf, directs, num_directs);

        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    width_fac = placer_opts.place_chan_width;

    init_chan(width_fac, chan_width_dist);

    alloc_and_load_placement_structs(placer_opts.place_cost_exp, placer_opts,
                                     directs, num_directs);

    initial_placement(placer_opts.pad_loc_type, placer_opts.pad_loc_file.c_str());
    init_draw_coords((float)width_fac);

    //Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, pb_gpin_lookup);

    /* Gets initial cost and loads bounding boxes. */

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE || placer_opts.enable_timing_computations) {
        costs.bb_cost = comp_bb_cost(NORMAL);

        crit_exponent = placer_opts.td_place_exp_first; /*this will be modified when rlim starts to change */

        num_connections = count_connections();
        VTR_LOG("\n");
        VTR_LOG("There are %d point to point connections in this circuit.\n", num_connections);
        VTR_LOG("\n");

        //Update the point-to-point delays from the initial placement
        comp_td_point_to_point_delays(*place_delay_model);

        /*
         * Initialize timing analysis
         */
        auto& atom_ctx = g_vpr_ctx.atom();
        placement_delay_calc = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, point_to_point_delay);
        placement_delay_calc->set_tsu_margin_relative(placer_opts.tsu_rel_margin);
        placement_delay_calc->set_tsu_margin_absolute(placer_opts.tsu_abs_margin);
        timing_info = make_setup_timing_info(placement_delay_calc);

        timing_info->update();
        timing_info->set_warn_unconstrained(false); //Don't warn again about unconstrained nodes again during placement

        //Initial slack estimates
        load_criticalities(*timing_info, crit_exponent, netlist_pin_lookup);

        critical_path = timing_info->least_slack_critical_path();

        //Write out the initial timing echo file
        if (isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();

            tatum::write_echo(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());
        }

        /*now we can properly compute costs  */
        comp_td_costs(*place_delay_model, &costs.timing_cost); /*also updates values in point_to_point_delay */

        outer_crit_iter_count = 1;

        prev_inverse_costs.timing_cost = 1 / costs.timing_cost;
        prev_inverse_costs.bb_cost = 1 / costs.bb_cost;
        costs.cost = 1; /*our new cost function uses normalized values of           */
                        /*bb_cost and timing_cost, the value of cost will be reset  */
                        /*to 1 at each temperature when *_TIMING_DRIVEN_PLACE is true */
    } else {            /*BOUNDING_BOX_PLACE */
        costs.cost = costs.bb_cost = comp_bb_cost(NORMAL);
        costs.timing_cost = 0;
        outer_crit_iter_count = 0;
        num_connections = 0;
        crit_exponent = 0;

        prev_inverse_costs.timing_cost = 0; /*inverses not used */
        prev_inverse_costs.bb_cost = 0;
    }

    //Sanity check that initial placement is legal
    check_place(costs, *place_delay_model, placer_opts.place_algorithm);

    //Initial pacement statistics
    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g\n",
            costs.cost, costs.bb_cost, costs.timing_cost);
    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
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

    //Table header
    print_place_status_header();

    sprintf(msg, "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);

    //Draw the initial placement
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    move_lim = (int)(annealing_sched.inner_num * pow(cluster_ctx.clb_nlist.blocks().size(), 1.3333));

    /* Sometimes I want to run the router with a random placement.  Avoid *
     * using 0 moves to stop division by 0 and 0 length vector problems,  *
     * by setting move_lim to 1 (which is still too small to do any       *
     * significant optimization).                                         */
    if (move_lim <= 0)
        move_lim = 1;

    if (placer_opts.inner_loop_recompute_divider != 0) {
        inner_recompute_limit = (int)(0.5 + (float)move_lim / (float)placer_opts.inner_loop_recompute_divider);
    } else {
        /*don't do an inner recompute */
        inner_recompute_limit = move_lim + 1;
    }

    rlim = (float)max(device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

    first_rlim = rlim; /*used in timing-driven placement for exponent computation */
    final_rlim = 1;
    inverse_delta_rlim = 1 / (first_rlim - final_rlim);

    t = starting_t(&costs, &prev_inverse_costs,
                   annealing_sched, move_lim, rlim,
                   *place_delay_model,
                   placer_opts);

    tot_iter = 0;
    moves_since_cost_recompute = 0;

    /* Outer loop of the simmulated annealing begins */
    while (exit_crit(t, costs.cost, annealing_sched) == 0) {
        if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            costs.cost = 1;
        }

        outer_loop_recompute_criticalities(placer_opts, &costs, &prev_inverse_costs,
                                           num_connections,
                                           crit_exponent,
                                           &outer_crit_iter_count,
                                           netlist_pin_lookup,
                                           *place_delay_model,
                                           *timing_info);

        placement_inner_loop(t, rlim, placer_opts,
                             move_lim, crit_exponent, inner_recompute_limit, &stats,
                             &costs,
                             &prev_inverse_costs,
                             &moves_since_cost_recompute,
                             netlist_pin_lookup,
                             *place_delay_model,
                             *timing_info);

        tot_iter += move_lim;

        calc_placer_stats(stats, success_rat, std_dev, costs, move_lim);

        oldt = t; /* for finding and printing alpha. */
        update_t(&t, rlim, success_rat, annealing_sched);

        if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            critical_path = timing_info->least_slack_critical_path();
            sTNS = timing_info->setup_total_negative_slack();
            sWNS = timing_info->setup_worst_negative_slack();
        }

        print_place_status(t, oldt,
                           stats,
                           critical_path.delay(), sTNS, sWNS,
                           success_rat, std_dev, rlim, crit_exponent, tot_iter);

        sprintf(msg, "Cost: %g  BB Cost %g  TD Cost %g  Temperature: %g",
                costs.cost, costs.bb_cost, costs.timing_cost, t);
        update_screen(ScreenUpdatePriority::MINOR, msg, PLACEMENT, timing_info);
        update_rlim(&rlim, success_rat, device_ctx.grid);

        if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            crit_exponent = (1 - (rlim - final_rlim) * inverse_delta_rlim)
                                * (placer_opts.td_place_exp_last - placer_opts.td_place_exp_first)
                            + placer_opts.td_place_exp_first;
        }
#ifdef VERBOSE
        if (getEchoEnabled()) {
            print_clb_placement("first_iteration_clb_placement.echo");
        }
#endif
    }
    /* Outer loop of the simmulated annealing ends */

    outer_loop_recompute_criticalities(placer_opts, &costs,
                                       &prev_inverse_costs,
                                       num_connections,
                                       crit_exponent,
                                       &outer_crit_iter_count,
                                       netlist_pin_lookup,
                                       *place_delay_model,
                                       *timing_info);

    t = 0; /* freeze out */

    /* Run inner loop again with temperature = 0 so as to accept only swaps
     * which reduce the cost of the placement */
    placement_inner_loop(t, rlim, placer_opts,
                         move_lim, crit_exponent, inner_recompute_limit, &stats,
                         &costs,
                         &prev_inverse_costs,
                         &moves_since_cost_recompute,
                         netlist_pin_lookup,
                         *place_delay_model,
                         *timing_info);

    tot_iter += move_lim;
    calc_placer_stats(stats, success_rat, std_dev, costs, move_lim);

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        critical_path = timing_info->least_slack_critical_path();
        sTNS = timing_info->setup_total_negative_slack();
        sWNS = timing_info->setup_worst_negative_slack();
    }

    print_place_status(t, oldt, stats,
                       critical_path.delay(), sTNS, sWNS,
                       success_rat, std_dev, rlim, crit_exponent, tot_iter);

    // TODO:
    // 1. add some subroutine hierarchy!  Too big!

#ifdef VERBOSE
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_END_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_END_CLB_PLACEMENT));
    }
#endif

    check_place(costs, *place_delay_model, placer_opts.place_algorithm);

    //Some stats
    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", num_ts_called);

    if (placer_opts.enable_timing_computations
        && placer_opts.place_algorithm == BOUNDING_BOX_PLACE) {
        /*need this done since the timing data has not been kept up to date*
         *in bounding_box mode */
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++)
                set_timing_place_crit(net_id, ipin, 0); /*dummy crit values */
        }
        comp_td_costs(*place_delay_model, &costs.timing_cost); /*computes point_to_point_delay */
    }

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
        || placer_opts.enable_timing_computations) {
        //Final timing estimate
        VTR_ASSERT(timing_info);

        timing_info->update(); //Tatum
        critical_path = timing_info->least_slack_critical_path();

        if (isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();

            tatum::write_echo(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());
        }

        generate_post_place_timing_reports(placer_opts,
                                           analysis_opts,
                                           *timing_info,
                                           *placement_delay_calc);

        /* Print critical path delay. */
        VTR_LOG("\n");
        VTR_LOG("Placement estimated critical path delay: %g ns",
                1e9 * critical_path.delay());
        VTR_LOG("\n");
        VTR_LOG("Placement estimated setup Total Negative Slack (sTNS): %g ns\n",
                1e9 * timing_info->setup_total_negative_slack());
        VTR_LOG("Placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
                1e9 * timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");

        VTR_LOG("Placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
        VTR_LOG("\n");
    }

    sprintf(msg, "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
    VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, \n",
            costs.cost, costs.bb_cost, costs.timing_cost);
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    // Print out swap statistics
    size_t total_swap_attempts = num_swap_rejected + num_swap_accepted + num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
    float reject_rate = (float)num_swap_rejected / total_swap_attempts;
    float accept_rate = (float)num_swap_accepted / total_swap_attempts;
    float abort_rate = (float)num_swap_aborted / total_swap_attempts;
    VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits, total_swap_attempts);
    VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_accepted, 100 * accept_rate);
    VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_rejected, 100 * reject_rate);
    VTR_LOG("\tSwaps aborted : %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_aborted, 100 * abort_rate);

    report_aborted_moves();

    free_placement_structs(placer_opts);
    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
        || placer_opts.enable_timing_computations) {
        free_lookups_and_criticalities();
    }

    free_try_swap_arrays();
}

/* Function to recompute the criticalities before the inner loop of the annealing */
static void outer_loop_recompute_criticalities(t_placer_opts placer_opts,
                                               t_placer_costs* costs,
                                               t_placer_prev_inverse_costs* prev_inverse_costs,
                                               int num_connections,
                                               float crit_exponent,
                                               int* outer_crit_iter_count,
                                               const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                               const PlaceDelayModel& delay_model,
                                               SetupTimingInfo& timing_info) {
    if (placer_opts.place_algorithm != PATH_TIMING_DRIVEN_PLACE)
        return;

    /*at each temperature change we update these values to be used     */
    /*for normalizing the tradeoff between timing and wirelength (bb)  */
    if (*outer_crit_iter_count >= placer_opts.recompute_crit_iter
        || placer_opts.inner_loop_recompute_divider != 0) {
#ifdef VERBOSE
        VTR_LOG("Outer loop recompute criticalities\n");
#endif
        num_connections = std::max(num_connections, 1); //Avoid division by zero
        VTR_ASSERT(num_connections > 0);

        //Per-temperature timing update
        timing_info.update();
        load_criticalities(timing_info, crit_exponent, netlist_pin_lookup);

        /*recompute costs from scratch, based on new criticalities */
        comp_td_costs(delay_model, &costs->timing_cost);
        *outer_crit_iter_count = 0;
    }
    (*outer_crit_iter_count)++;

    /*at each temperature change we update these values to be used     */
    /*for normalizing the tradeoff between timing and wirelength (bb)  */
    prev_inverse_costs->bb_cost = 1 / costs->bb_cost;
    /*Prevent inverse timing cost from going to infinity */
    prev_inverse_costs->timing_cost = min(1 / costs->timing_cost, MAX_INV_TIMING_COST);
}

/* Function which contains the inner loop of the simulated annealing */
static void placement_inner_loop(float t,
                                 float rlim,
                                 t_placer_opts placer_opts,
                                 int move_lim,
                                 float crit_exponent,
                                 int inner_recompute_limit,
                                 t_placer_statistics* stats,
                                 t_placer_costs* costs,
                                 t_placer_prev_inverse_costs* prev_inverse_costs,
                                 int* moves_since_cost_recompute,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 const PlaceDelayModel& delay_model,
                                 SetupTimingInfo& timing_info) {
    int inner_crit_iter_count, inner_iter;

    stats->av_cost = 0.;
    stats->av_bb_cost = 0.;
    stats->av_timing_cost = 0.;
    stats->sum_of_squares = 0.;
    stats->success_sum = 0;

    inner_crit_iter_count = 1;

    /* Inner loop begins */
    for (inner_iter = 0; inner_iter < move_lim; inner_iter++) {
        e_swap_result swap_result = try_swap(t, costs, prev_inverse_costs, rlim,
                                             delay_model,
                                             placer_opts.rlim_escape_fraction,
                                             placer_opts.place_algorithm,
                                             placer_opts.timing_tradeoff);

        if (swap_result == ACCEPTED) {
            /* Move was accepted.  Update statistics that are useful for the annealing schedule. */
            stats->success_sum++;
            stats->av_cost += costs->cost;
            stats->av_bb_cost += costs->bb_cost;
            stats->av_timing_cost += costs->timing_cost;
            stats->sum_of_squares += (costs->cost) * (costs->cost);
            num_swap_accepted++;
        } else if (swap_result == ABORTED) {
            num_swap_aborted++;
        } else { // swap_result == REJECTED
            num_swap_rejected++;
        }

        if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            /* Do we want to re-timing analyze the circuit to get updated slack and criticality values?
             * We do this only once in a while, since it is expensive.
             */
            if (inner_crit_iter_count >= inner_recompute_limit
                && inner_iter != move_lim - 1) { /*on last iteration don't recompute */

                inner_crit_iter_count = 0;
#ifdef VERBOSE
                VTR_LOG("Inner loop recompute criticalities\n");
#endif
                /* Using the delays in net_delay, do a timing analysis to update slacks and
                 * criticalities; then update the timing cost since it will change.
                 */
                //Inner loop timing update
                timing_info.update();
                load_criticalities(timing_info, crit_exponent, netlist_pin_lookup);

                comp_td_costs(delay_model, &costs->timing_cost);
            }
            inner_crit_iter_count++;
        }
#ifdef VERBOSE
        VTR_LOG("t = %g  cost = %g   bb_cost = %g timing_cost = %g move = %d\n",
                t, costs->cost, costs->bb_cost, costs->timing_cost, inner_iter);
        if (fabs((costs->bb_cost) - comp_bb_cost(CHECK)) > (costs->bb_cost) * ERROR_TOL)
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                      "fabs((*bb_cost) - comp_bb_cost(CHECK)) > (*bb_cost) * ERROR_TOL");
#endif

        /* Lines below prevent too much round-off error from accumulating
         * in the cost over many iterations (due to incremental updates).
         * This round-off can lead to  error checks failing because the cost 
         * is different from what you get when you recompute from scratch.                       
         */
        ++(*moves_since_cost_recompute);
        if (*moves_since_cost_recompute > MAX_MOVES_BEFORE_RECOMPUTE) {
            recompute_costs_from_scratch(placer_opts, delay_model, costs);
            *moves_since_cost_recompute = 0;
        }
    }
    /* Inner loop ends */
}

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts, const PlaceDelayModel& delay_model, t_placer_costs* costs) {
    double new_bb_cost = recompute_bb_cost();
    if (fabs(new_bb_cost - costs->bb_cost) > costs->bb_cost * ERROR_TOL) {
        std::string msg = vtr::string_fmt("in recompute_costs_from_scratch: new_bb_cost = %g, old bb_cost = %g\n",
                                          new_bb_cost, costs->bb_cost);
        VPR_THROW(VPR_ERROR_PLACE, msg.c_str());
    }
    costs->bb_cost = new_bb_cost;

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        double new_timing_cost = 0.;
        comp_td_costs(delay_model, &new_timing_cost);
        if (fabs(new_timing_cost - costs->timing_cost) > costs->timing_cost * ERROR_TOL) {
            std::string msg = vtr::string_fmt("in recompute_costs_from_scratch: new_timing_cost = %g, old timing_cost = %g, ERROR_TOL = %g\n",
                                              new_timing_cost, costs->timing_cost, ERROR_TOL);
            VPR_THROW(VPR_ERROR_PLACE, msg.c_str());
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

static double get_std_dev(int n, double sum_x_squared, double av_x) {
    /* Returns the standard deviation of data set x.  There are n sample points, *
     * sum_x_squared is the summation over n of x^2 and av_x is the average x.   *
     * All operations are done in double precision, since round off error can be *
     * a problem in the initial temp. std_dev calculation for big circuits.      */

    double std_dev;

    if (n <= 1)
        std_dev = 0.;
    else
        std_dev = (sum_x_squared - n * av_x * av_x) / (double)(n - 1);

    if (std_dev > 0.) /* Very small variances sometimes round negative */
        std_dev = sqrt(std_dev);
    else
        std_dev = 0.;

    return (std_dev);
}

static void update_rlim(float* rlim, float success_rat, const DeviceGrid& grid) {
    /* Update the range limited to keep acceptance prob. near 0.44.  Use *
     * a floating point rlim to allow gradual transitions at low temps.  */

    float upper_lim;

    *rlim = (*rlim) * (1. - 0.44 + success_rat);
    upper_lim = max(grid.width() - 1, grid.height() - 1);
    *rlim = min(*rlim, upper_lim);
    *rlim = max(*rlim, (float)1.);
}

/* Update the temperature according to the annealing schedule selected. */
static void update_t(float* t, float rlim, float success_rat, t_annealing_sched annealing_sched) {
    /*  float fac; */

    if (annealing_sched.type == USER_SCHED) {
        *t = annealing_sched.alpha_t * (*t);
    } else { /* AUTO_SCHED */
        if (success_rat > 0.96) {
            *t = (*t) * 0.5;
        } else if (success_rat > 0.8) {
            *t = (*t) * 0.9;
        } else if (success_rat > 0.15 || rlim > 1.) {
            *t = (*t) * 0.95;
        } else {
            *t = (*t) * 0.8;
        }
    }
}

static int exit_crit(float t, float cost, t_annealing_sched annealing_sched) {
    /* Return 1 when the exit criterion is met.                        */

    if (annealing_sched.type == USER_SCHED) {
        if (t < annealing_sched.exit_t) {
            return (1);
        } else {
            return (0);
        }
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Automatic annealing schedule */
    float t_exit = 0.005 * cost / cluster_ctx.clb_nlist.nets().size();

    if (t < t_exit) {
        return (1);
    } else if (std::isnan(t_exit)) {
        //May get nan if there are no nets
        return (1);
    } else {
        return (0);
    }
}

static float starting_t(t_placer_costs* costs,
                        t_placer_prev_inverse_costs* prev_inverse_costs,
                        t_annealing_sched annealing_sched,
                        int max_moves,
                        float rlim,
                        const PlaceDelayModel& delay_model,
                        const t_placer_opts& placer_opts) {
    /* Finds the starting temperature (hot condition).              */

    int i, num_accepted, move_lim;
    double std_dev, av, sum_of_squares; /* Double important to avoid round off */

    if (annealing_sched.type == USER_SCHED)
        return (annealing_sched.init_t);

    auto& cluster_ctx = g_vpr_ctx.clustering();

    move_lim = min(max_moves, (int)cluster_ctx.clb_nlist.blocks().size());

    num_accepted = 0;
    av = 0.;
    sum_of_squares = 0.;

    /* Try one move per block.  Set t high so essentially all accepted. */

    for (i = 0; i < move_lim; i++) {
        e_swap_result swap_result = try_swap(HUGE_POSITIVE_FLOAT, costs, prev_inverse_costs, rlim,
                                             delay_model,
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

    if (num_accepted != 0)
        av /= num_accepted;
    else
        av = 0.;

    std_dev = get_std_dev(num_accepted, sum_of_squares, av);

    if (num_accepted != move_lim) {
        VTR_LOG_WARN("Starting t: %d of %d configurations accepted.\n", num_accepted, move_lim);
    }

#ifdef VERBOSE
    VTR_LOG("std_dev: %g, average cost: %g, starting temp: %g\n", std_dev, av, 20. * std_dev);
#endif

    /* Set the initial temperature to 20 times the standard of deviation */
    /* so that the initial temperature adjusts according to the circuit */
    return (20. * std_dev);
}

//Moves the blocks in blocks_affected to their new locations
static void apply_move_blocks() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    //Swap the blocks, but don't swap the nets or update place_ctx.grid_blocks
    //yet since we don't know whether the swap will be accepted
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        place_ctx.block_locs[blk].loc = blocks_affected.moved_blocks[iblk].new_loc;
    }
}

//Commits the blocks in blocks_affected to their new locations (updates inverse
//lookups via place_ctx.grid_blocks)
static void commit_move_blocks() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Swap physical location */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        t_pl_loc to = blocks_affected.moved_blocks[iblk].new_loc;

        t_pl_loc from = blocks_affected.moved_blocks[iblk].old_loc;

        //Remove from old location only if it hasn't already been updated by a previous block update
        if (place_ctx.grid_blocks[from.x][from.y].blocks[from.z] == blk) {
            ;
            place_ctx.grid_blocks[from.x][from.y].blocks[from.z] = EMPTY_BLOCK_ID;
            --place_ctx.grid_blocks[from.x][from.y].usage;
        }

        //Add to new location
        if (place_ctx.grid_blocks[to.x][to.y].blocks[to.z] == EMPTY_BLOCK_ID) {
            ;
            //Only need to increase usage if previously unused
            ++place_ctx.grid_blocks[to.x][to.y].usage;
        }
        place_ctx.grid_blocks[to.x][to.y].blocks[to.z] = blk;

    } // Finish updating clb for all blocks
}

//Moves the blocks in blocks_affected to their old locations
static void revert_move_blocks() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // Swap the blocks back, nets not yet swapped they don't need to be changed
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        t_pl_loc old = blocks_affected.moved_blocks[iblk].old_loc;

        place_ctx.block_locs[blk].loc = old;

        VTR_ASSERT_SAFE_MSG(place_ctx.grid_blocks[old.x][old.y].blocks[old.z] = blk, "Grid blocks should only have been updated if swap commited (not reverted)");
    }
}

//Clears the current move so a new move can be proposed
static void clear_move_blocks() {
    //Reset moved flags
    blocks_affected.moved_to.clear();
    blocks_affected.moved_from.clear();

    //For run-time we just reset num_moved_blocks to zero, but do not free the blocks_affected
    //array to avoid memory allocation
    blocks_affected.num_moved_blocks = 0;
}

static void update_move_nets(int num_nets_affected) {
    /* update net cost functions and reset flags. */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        bb_coords[net_id] = ts_bb_coord_new[net_id];
        if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET)
            bb_num_on_edges[net_id] = ts_bb_edge_new[net_id];

        net_cost[net_id] = temp_net_cost[net_id];

        /* negative temp_net_cost value is acting as a flag. */
        temp_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
}

static void reset_move_nets(int num_nets_affected) {
    /* Reset the net cost function flags first. */
    for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];
        temp_net_cost[net_id] = -1;
        bb_updated_before[net_id] = NOT_UPDATED_YET;
    }
}

static e_find_affected_blocks_result record_block_move(ClusterBlockId blk, t_pl_loc to) {
    auto res = blocks_affected.moved_to.emplace(to);
    if (!res.second) {
        log_move_abort("duplicate block move to location");
        return e_find_affected_blocks_result::ABORT;
    }

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    t_pl_loc from = place_ctx.block_locs[blk].loc;

    auto res2 = blocks_affected.moved_from.emplace(from);
    if (!res2.second) {
        log_move_abort("duplicate block move from location");
        return e_find_affected_blocks_result::ABORT;
    }

    VTR_ASSERT_SAFE(to.z < int(place_ctx.grid_blocks[to.x][to.y].blocks.size()));

    // Sets up the blocks moved
    int imoved_blk = blocks_affected.num_moved_blocks;
    blocks_affected.moved_blocks[imoved_blk].block_num = blk;
    blocks_affected.moved_blocks[imoved_blk].old_loc = from;
    blocks_affected.moved_blocks[imoved_blk].new_loc = to;
    blocks_affected.num_moved_blocks++;

    return e_find_affected_blocks_result::VALID;
}

static e_find_affected_blocks_result record_single_block_swap(ClusterBlockId b_from, t_pl_loc to) {
    /* Find all the blocks affected when b_from is swapped with b_to.
     * Returns abort_swap.                  */

    VTR_ASSERT_SAFE(b_from);

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    VTR_ASSERT_SAFE(to.z < int(place_ctx.grid_blocks[to.x][to.y].blocks.size()));

    ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

    e_find_affected_blocks_result outcome = e_find_affected_blocks_result::VALID;

    // Check whether the to_location is empty
    if (b_to == EMPTY_BLOCK_ID) {
        // Sets up the blocks moved
        outcome = record_block_move(b_from, to);

    } else if (b_to != INVALID_BLOCK_ID) {
        // Sets up the blocks moved
        outcome = record_block_move(b_from, to);

        if (outcome != e_find_affected_blocks_result::VALID) {
            return outcome;
        }

        t_pl_loc from = place_ctx.block_locs[b_from].loc;
        outcome = record_block_move(b_to, from);

    } // Finish swapping the blocks and setting up blocks_affected

    return outcome;
}

static e_propose_move propose_move(ClusterBlockId b_from, t_pl_loc to) {
    e_find_affected_blocks_result outcome = find_affected_blocks(b_from, to);

    if (outcome == e_find_affected_blocks_result::INVERT) {
        //Try inverting the swap direction

        auto& place_ctx = g_vpr_ctx.placement();
        ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        if (!b_to) {
            log_move_abort("inverted move no to block");
            outcome = e_find_affected_blocks_result::ABORT;
        } else {
            t_pl_loc from = place_ctx.block_locs[b_from].loc;

            outcome = find_affected_blocks(b_to, from);

            if (outcome == e_find_affected_blocks_result::INVERT) {
                log_move_abort("inverted move recurrsion");
                outcome = e_find_affected_blocks_result::ABORT;
            }
        }
    }

    if (outcome == e_find_affected_blocks_result::VALID
        || outcome == e_find_affected_blocks_result::INVERT_VALID) {
        return e_propose_move::VALID;
    } else {
        VTR_ASSERT_SAFE(outcome == e_find_affected_blocks_result::ABORT);
        return e_propose_move::ABORT;
    }
}

static e_find_affected_blocks_result find_affected_blocks(ClusterBlockId b_from, t_pl_loc to) {
    /* Finds and set ups the affected_blocks array.
     * Returns abort_swap. */
    VTR_ASSERT_SAFE(b_from);

    int imacro_from;
    ClusterBlockId curr_b_from;
    e_find_affected_blocks_result outcome = e_find_affected_blocks_result::VALID;

    auto& place_ctx = g_vpr_ctx.placement();

    t_pl_loc from = place_ctx.block_locs[b_from].loc;

    auto& pl_macros = place_ctx.pl_macros;

    get_imacro_from_iblk(&imacro_from, b_from, pl_macros);
    if (imacro_from != -1) {
        // b_from is part of a macro, I need to swap the whole macro

        // Record down the relative position of the swap
        t_pl_offset swap_offset = to - from;

        int imember_from = 0;
        outcome = record_macro_swaps(imacro_from, imember_from, swap_offset);

        VTR_ASSERT_SAFE(outcome != e_find_affected_blocks_result::VALID || imember_from == int(pl_macros[imacro_from].members.size()));

    } else {
        ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];
        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, b_to, pl_macros);

        if (imacro_to != -1) {
            //To block is a macro but from is a single block.
            //
            //Since we support swapping a macro as 'from' to a single 'to' block,
            //just invert the swap direction (which is equivalent)
            outcome = e_find_affected_blocks_result::INVERT;
        } else {
            // This is not a macro - I could use the from and to info from before
            outcome = record_single_block_swap(b_from, to);
        }

    } // Finish handling cases for blocks in macro and otherwise

    return outcome;
}

//Records all the block movements required to move the macro imacro_from starting at member imember_from
//to a new position offset from its current position by swap_offset. The new location may be a
//single (non-macro) block, or another macro.
static e_find_affected_blocks_result record_macro_swaps(const int imacro_from, int& imember_from, t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& pl_macros = place_ctx.pl_macros;

    e_find_affected_blocks_result outcome = e_find_affected_blocks_result::VALID;

    for (; imember_from < int(pl_macros[imacro_from].members.size()) && outcome == e_find_affected_blocks_result::VALID; imember_from++) {
        // Gets the new from and to info for every block in the macro
        // cannot use the old from and to info
        ClusterBlockId curr_b_from = pl_macros[imacro_from].members[imember_from].blk_index;

        t_pl_loc curr_from = place_ctx.block_locs[curr_b_from].loc;

        t_pl_loc curr_to = curr_from + swap_offset;

        //Make sure that the swap_to location is valid
        //It must be:
        // * on chip, and
        // * match the correct block type
        //
        //Note that we need to explicitly check that the types match, since the device floorplan is not
        //(neccessarily) translationally invariant for an arbitrary macro
        if (!is_legal_swap_to_location(curr_b_from, curr_to)) {
            log_move_abort("macro_from swap to location illegal");
            outcome = e_find_affected_blocks_result::ABORT;
        } else {
            ClusterBlockId b_to = place_ctx.grid_blocks[curr_to.x][curr_to.y].blocks[curr_to.z];
            int imacro_to = -1;
            get_imacro_from_iblk(&imacro_to, b_to, pl_macros);

            if (imacro_to != -1) {
                //To block is a macro

                if (imacro_from == imacro_to) {
                    outcome = record_macro_self_swaps(imacro_from, swap_offset);
                    imember_from = pl_macros[imacro_from].members.size();
                    break; //record_macro_self_swaps() handles this case completely, so we don't need to continue the loop
                } else {
                    outcome = record_macro_macro_swaps(imacro_from, imember_from, imacro_to, b_to, swap_offset);
                    if (outcome == e_find_affected_blocks_result::INVERT_VALID) {
                        break; //The move was inverted and successfully proposed, don't need to continue the loop
                    }
                    imember_from -= 1; //record_macro_macro_swaps() will have already advanced the original imember_from
                }
            } else {
                //To block is not a macro
                outcome = record_single_block_swap(curr_b_from, curr_to);
            }
        }
    } // Finish going through all the blocks in the macro
    return outcome;
}

//Records all the block movements required to move the macro imacro_from starting at member imember_from
//to a new position offset from its current position by swap_offset. The new location must be where
//blk_to is located and blk_to must be part of imacro_to.
static e_find_affected_blocks_result record_macro_macro_swaps(const int imacro_from, int& imember_from, const int imacro_to, ClusterBlockId blk_to, t_pl_offset swap_offset) {
    //Adds the macro imacro_to to the set of affected block caused by swapping 'blk_to' to it's
    //new position.
    //
    //This function is only called when both the main swap's from/to blocks are placement macros.
    //The position in the from macro ('imacro_from') is specified by 'imember_from', and the relevant
    //macro fro the to block is 'imacro_to'.

    auto& place_ctx = g_vpr_ctx.placement();

    //At the moment, we only support blk_to being the first element of the 'to' macro.
    //
    //For instance, this means that we can swap two carry chains so long as one starts
    //below the other (not a big limitation since swapping in the oppostie direction would
    //allow these blocks to swap)
    if (place_ctx.pl_macros[imacro_to].members[0].blk_index != blk_to) {
        int imember_to = 0;
        auto outcome = record_macro_swaps(imacro_to, imember_to, -swap_offset);
        if (outcome == e_find_affected_blocks_result::INVERT) {
            log_move_abort("invert recursion2");
            outcome = e_find_affected_blocks_result::ABORT;
        } else if (outcome == e_find_affected_blocks_result::VALID) {
            outcome = e_find_affected_blocks_result::INVERT_VALID;
        }
        return outcome;
    }

    //From/To blocks should be exactly the swap offset appart
    ClusterBlockId blk_from = place_ctx.pl_macros[imacro_from].members[imember_from].blk_index;
    VTR_ASSERT_SAFE(place_ctx.block_locs[blk_from].loc + swap_offset == place_ctx.block_locs[blk_to].loc);

    //Continue walking along the overlapping parts of the from and to macros, recording
    //each block swap.
    //
    //At the momemnt we only support swapping the two macros if they have the same shape.
    //This will be the case with the common cases we care about (i.e. carry-chains), so
    //we just abort in any other cases (if these types of macros become more common in
    //the future this could be updated).
    //
    //Unless the two macros have thier root blocks aligned (i.e. the mutual overlap starts
    //at imember_from == 0), then theree will be a fixed offset between the macros' relative
    //position. We record this as from_to_macro_*_offset which is used to verify the shape
    //of the macros is consistent.
    //
    //NOTE: We mutate imember_from so the outer from macro walking loop moves in lock-step
    int imember_to = 0;
    t_pl_offset from_to_macro_offset = place_ctx.pl_macros[imacro_from].members[imember_from].offset;
    for (; imember_from < int(place_ctx.pl_macros[imacro_from].members.size()) && imember_to < int(place_ctx.pl_macros[imacro_to].members.size());
         ++imember_from, ++imember_to) {
        //Check that both macros have the same shape while they overlap
        if (place_ctx.pl_macros[imacro_from].members[imember_from].offset != place_ctx.pl_macros[imacro_to].members[imember_to].offset + from_to_macro_offset) {
            log_move_abort("macro shapes disagree");
            return e_find_affected_blocks_result::ABORT;
        }

        ClusterBlockId b_from = place_ctx.pl_macros[imacro_from].members[imember_from].blk_index;

        t_pl_loc curr_to = place_ctx.block_locs[b_from].loc + swap_offset;

        ClusterBlockId b_to = place_ctx.pl_macros[imacro_to].members[imember_to].blk_index;
        VTR_ASSERT_SAFE(curr_to == place_ctx.block_locs[b_to].loc);

        if (!is_legal_swap_to_location(b_from, curr_to)) {
            log_move_abort("macro_from swap to location illegal");
            return e_find_affected_blocks_result::ABORT;
        }

        auto outcome = record_single_block_swap(b_from, curr_to);
        if (outcome != e_find_affected_blocks_result::VALID) {
            return outcome;
        }
    }

    if (imember_to < int(place_ctx.pl_macros[imacro_to].members.size())) {
        //The to macro extends beyond the from macro.
        //
        //Swap the remainder of the 'to' macro to locations after the 'from' macro.
        //Note that we are swapping in the opposite direction so the swap offsets are inverted.
        return record_macro_swaps(imacro_to, imember_to, -swap_offset);
    }

    return e_find_affected_blocks_result::VALID;
}

//Returns the set of macros affected by moving imacro by the specified offset
//
//The resulting 'macros' may contain duplicates
static e_find_affected_blocks_result identify_macro_self_swap_affected_macros(std::vector<int>& macros, const int imacro, t_pl_offset swap_offset) {
    e_find_affected_blocks_result outcome = e_find_affected_blocks_result::VALID;
    auto& place_ctx = g_vpr_ctx.placement();

    for (size_t imember = 0; imember < place_ctx.pl_macros[imacro].members.size() && outcome == e_find_affected_blocks_result::VALID; ++imember) {
        ClusterBlockId blk = place_ctx.pl_macros[imacro].members[imember].blk_index;

        t_pl_loc from = place_ctx.block_locs[blk].loc;
        t_pl_loc to = from + swap_offset;

        if (!is_legal_swap_to_location(blk, to)) {
            log_move_abort("macro move to location illegal");
            return e_find_affected_blocks_result::ABORT;
        }

        ClusterBlockId blk_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, blk_to, place_ctx.pl_macros);

        if (imacro_to != -1) {
            auto itr = std::find(macros.begin(), macros.end(), imacro_to);
            if (itr == macros.end()) {
                macros.push_back(imacro_to);
                outcome = identify_macro_self_swap_affected_macros(macros, imacro_to, swap_offset);
            }
        }
    }
    return e_find_affected_blocks_result::VALID;
}

//Moves the macro imacro by the specified offset
//
//Records the block movements in block_moves, the other blocks displaced in displaced_blocks,
//and any generated empty locations in empty_locations.
//
//This function moves a single macro and does not check for overlap with other macros!
static e_find_affected_blocks_result record_macro_move(std::vector<ClusterBlockId>& displaced_blocks,
                                                       const int imacro,
                                                       t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();

    for (const t_pl_macro_member& member : place_ctx.pl_macros[imacro].members) {
        t_pl_loc from = place_ctx.block_locs[member.blk_index].loc;

        t_pl_loc to = from + swap_offset;

        if (!is_legal_swap_to_location(member.blk_index, to)) {
            log_move_abort("macro move to location illegal");
            return e_find_affected_blocks_result::ABORT;
        }

        ClusterBlockId blk_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];

        record_block_move(member.blk_index, to);

        int imacro_to = -1;
        get_imacro_from_iblk(&imacro_to, blk_to, place_ctx.pl_macros);
        if (blk_to && imacro_to != imacro) { //Block displaced only if exists and not part of current macro
            displaced_blocks.push_back(blk_to);
        }
    }
    return e_find_affected_blocks_result::VALID;
}

static e_find_affected_blocks_result record_macro_self_swaps(const int imacro, t_pl_offset swap_offset) {
    auto& place_ctx = g_vpr_ctx.placement();

    //Reset any paritao move
    clear_move_blocks();

    //Collect the macros affected
    std::vector<int> affected_macros;
    auto outcome = identify_macro_self_swap_affected_macros(affected_macros, imacro,
                                                            swap_offset);

    if (outcome != e_find_affected_blocks_result::VALID) {
        return outcome;
    }

    //Remove any duplicate macros
    affected_macros.resize(std::distance(affected_macros.begin(), std::unique(affected_macros.begin(), affected_macros.end())));

    std::vector<ClusterBlockId> displaced_blocks;

    //Move all the affected macros by the offset
    for (int imacro_affected : affected_macros) {
        outcome = record_macro_move(displaced_blocks, imacro_affected, swap_offset);

        if (outcome != e_find_affected_blocks_result::VALID) {
            return outcome;
        }
    }

    auto is_non_macro_block = [&](ClusterBlockId blk) {
        int imacro_blk = -1;
        get_imacro_from_iblk(&imacro_blk, blk, place_ctx.pl_macros);

        if (std::find(affected_macros.begin(), affected_macros.end(), imacro_blk) != affected_macros.end()) {
            return false;
        }
        return true;
    };

    std::vector<ClusterBlockId> non_macro_displaced_blocks;
    std::copy_if(displaced_blocks.begin(), displaced_blocks.end(), std::back_inserter(non_macro_displaced_blocks), is_non_macro_block);

    //Based on the currently queued block moves, find the empty 'holes' left behind
    auto empty_locs = determine_locations_emptied_by_move();

    VTR_ASSERT_SAFE(empty_locs.size() >= non_macro_displaced_blocks.size());

    //Fit the displaced blocks into the empty locations
    auto loc_itr = empty_locs.begin();
    for (auto blk : non_macro_displaced_blocks) {
        outcome = record_block_move(blk, *loc_itr);
        ++loc_itr;
    }

    return outcome;
}

bool is_legal_swap_to_location(ClusterBlockId blk, t_pl_loc to) {
    //Make sure that the swap_to location is valid
    //It must be:
    // * on chip, and
    // * match the correct block type
    //
    //Note that we need to explicitly check that the types match, since the device floorplan is not
    //(neccessarily) translationally invariant for an arbitrary macro

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (to.x < 0 || to.x >= int(device_ctx.grid.width())
        || to.y < 0 || to.y >= int(device_ctx.grid.height())
        || to.z < 0 || to.z >= device_ctx.grid[to.x][to.y].type->capacity
        || (device_ctx.grid[to.x][to.y].type != cluster_ctx.clb_nlist.block_type(blk))) {
        return false;
    }
    return true;
}

//Examines the currently proposed move and determine any empty locations
std::set<t_pl_loc> determine_locations_emptied_by_move() {
    std::set<t_pl_loc> moved_from;
    std::set<t_pl_loc> moved_to;

    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        //When a block is moved it's old location becomes free
        moved_from.emplace(blocks_affected.moved_blocks[iblk].old_loc);

        //But any block later moved to a position fills it
        moved_to.emplace(blocks_affected.moved_blocks[iblk].new_loc);
    }

    std::set<t_pl_loc> empty_locs;
    std::set_difference(moved_from.begin(), moved_from.end(),
                        moved_to.begin(), moved_to.end(),
                        std::inserter(empty_locs, empty_locs.begin()));

    return empty_locs;
}

static e_swap_result try_swap(float t,
                              t_placer_costs* costs,
                              t_placer_prev_inverse_costs* prev_inverse_costs,
                              float rlim,
                              const PlaceDelayModel& delay_model,
                              float rlim_escape_fraction,
                              enum e_place_algorithm place_algorithm,
                              float timing_tradeoff) {
    /* Picks some block and moves it to another spot.  If this spot is   *
     * occupied, switch the blocks.  Assess the change in cost function. *
     * rlim is the range limiter.                                        *
     * Returns whether the swap is accepted, rejected or aborted.        *
     * Passes back the new value of the cost functions.                  */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    num_ts_called++;

    /* I'm using negative values of temp_net_cost as a flag, so DO NOT   *
     * use cost functions that can go negative.                          */

    double delta_c = 0; /* Change in cost due to this swap. */
    double bb_delta_c = 0;
    double timing_delta_c = 0;

    /* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
    if (!b_from) {
        return ABORTED; //No movable block found
    }

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[from.x][from.y].type;
    VTR_ASSERT(cluster_from_type == grid_from_type);

    //Allow some fraction of moves to not be restricted by rlim,
    //in the hopes of better escaping local minima
    if (rlim_escape_fraction > 0. && vtr::frand() < rlim_escape_fraction) {
        rlim = std::numeric_limits<float>::infinity();
    }

    t_pl_loc to;
    if (!find_to(cluster_ctx.clb_nlist.block_type(b_from), rlim, from, to))
        return REJECTED;

#if 0
    auto& grid = g_vpr_ctx.device().grid;
	ClusterBlockId b_to = place_ctx.grid_blocks[to.x][to.y].blocks[to.z];
	VTR_LOG( "swap [%d][%d][%d] %s block %zu \"%s\" <=> [%d][%d][%d] %s block ",
		from.x, from.y, from.z, grid[from.x][from.y].type->name, size_t(b_from), (b_from ? cluster_ctx.clb_nlist.block_name(b_from).c_str() : ""),
		to.x, to.y, to.z, grid[to.x][to.y].type->name);
    if (b_to) {
        VTR_LOG("%zu \"%s\"", size_t(b_to), cluster_ctx.clb_nlist.block_name(b_to).c_str());
    } else {
        VTR_LOG("(EMPTY)");
    }
    VTR_LOG("\n");
#endif

    /* Make the switch in order to make computing the new bounding *
     * box simpler.  If the cost increase is too high, switch them *
     * back.  (place_ctx.block_locs data structures switched, clbs not switched   *
     * until success of move is determined.)                       *
     * Also check that whether those are the only 2 blocks         *
     * to be moved - check for carry chains and other placement    *
     * macros.                                                     */

    /* Check whether the from_block is part of a macro first.      *
     * If it is, the whole macro has to be moved. Calculate the    *
     * x, y, z offsets of the swap to maintain relative placements *
     * of the blocks. Abort the swap if the to_block is part of a  *
     * macro (not supported yet).                                  */

    e_propose_move move_outcome = propose_move(b_from, to);

    if (move_outcome == e_propose_move::VALID) {
        //Swap the blocks
        apply_move_blocks();

        // Find all the nets affected by this swap and update thier bounding box
        int num_nets_affected = find_affected_nets_and_update_costs(place_algorithm, delay_model, bb_delta_c, timing_delta_c);

        if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            /*in this case we redefine delta_c as a combination of timing and bb.  *
             *additionally, we normalize all values, therefore delta_c is in       *
             *relation to 1*/

            delta_c = (1 - timing_tradeoff) * bb_delta_c * prev_inverse_costs->bb_cost
                      + timing_tradeoff * timing_delta_c * prev_inverse_costs->timing_cost;
        } else {
            delta_c = bb_delta_c;
        }

        /* 1 -> move accepted, 0 -> rejected. */
        e_swap_result keep_switch = assess_swap(delta_c, t);

        if (keep_switch == ACCEPTED) {
            costs->cost += delta_c;
            costs->bb_cost += bb_delta_c;

            if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
                /*update the point_to_point_timing_cost and point_to_point_delay
                 * values from the temporary values */
                costs->timing_cost += timing_delta_c;

                update_td_cost();
            }

            /* update net cost functions and reset flags. */
            update_move_nets(num_nets_affected);

            /* Update clb data structures since we kept the move. */
            commit_move_blocks();

        } else { /* Move was rejected.  */
                 /* Reset the net cost function flags first. */
            reset_move_nets(num_nets_affected);

            /* Restore the place_ctx.block_locs data structures to their state before the move. */
            revert_move_blocks();
        }

        clear_move_blocks();

        //VTR_ASSERT(check_macro_placement_consistency() == 0);
#if 0
        //Check that each accepted swap yields a valid placement
        check_place(*costs, delay_model, place_algorithm);
#endif

        return (keep_switch);
    } else {
        VTR_ASSERT_SAFE(move_outcome == e_propose_move::ABORT);

        clear_move_blocks();

        return ABORTED;
    }
}

//Pick a random block to be swapped with another random block.
//If none is found return ClusterBlockId::INVALID()
static ClusterBlockId pick_from_block() {
    /* Some blocks may be fixed, and should never be moved from their *
     * initial positions. If we randomly selected such a block try    *
     * another random block.                                          *
     *                                                                *
     * We need to track the blocks we have tried to avoid an infinite *
     * loop if all blocks are fixed.                                  */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    std::unordered_set<ClusterBlockId> tried_from_blocks;

    //So long as untried blocks remain
    while (tried_from_blocks.size() < cluster_ctx.clb_nlist.blocks().size()) {
        //Pick a block at random
        ClusterBlockId b_from = ClusterBlockId(vtr::irand((int)cluster_ctx.clb_nlist.blocks().size() - 1));

        //Record it as tried
        tried_from_blocks.insert(b_from);

        if (place_ctx.block_locs[b_from].is_fixed) {
            continue; //Fixed location, try again
        }

        //Found a movable block
        return b_from;
    }

    //No movable blocks found
    return ClusterBlockId::INVALID();
}

//Puts all the nets changed by the current swap into nets_to_update,
//and updates their bounding box.
//
//Returns the number of affected nets.
static int find_affected_nets_and_update_costs(e_place_algorithm place_algorithm, const PlaceDelayModel& delay_model, double& bb_delta_c, double& timing_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int num_affected_nets = 0;

    //Go through all the blocks moved
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        //Go through all the pins in the moved block
        for (ClusterPinId blk_pin : cluster_ctx.clb_nlist.block_pins(blk)) {
            ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(blk_pin);
            VTR_ASSERT_SAFE_MSG(net_id, "Only valid nets should be found in compressed netlist block pins");

            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                continue; //TODO: do we require anyting special here for global nets. "Global nets are assumed to span the whole chip, and do not effect costs"

            //Record effected nets
            record_affected_net(net_id, num_affected_nets);

            //Update the net bounding boxes
            //
            //Do not update the net cost here since it should only be updated
            //once per net, not once per pin.
            update_net_bb(net_id, iblk, blk, blk_pin);

            if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
                //Determine the change in timing costs if required
                update_td_delta_costs(delay_model, net_id, blk_pin, timing_delta_c);
            }
        }
    }

    /* Now update the bounding box costs (since the net bounding boxes are up-to-date).
     * The cost is only updated once per net.
     */
    for (int inet_affected = 0; inet_affected < num_affected_nets; inet_affected++) {
        ClusterNetId net_id = ts_nets_to_update[inet_affected];

        temp_net_cost[net_id] = get_net_cost(net_id, &ts_bb_coord_new[net_id]);
        bb_delta_c += temp_net_cost[net_id] - net_cost[net_id];
    }

    return num_affected_nets;
}

static void record_affected_net(const ClusterNetId net, int& num_affected_nets) {
    //Record effected nets
    if (temp_net_cost[net] < 0.) {
        //Net not marked yet.
        ts_nets_to_update[num_affected_nets] = net;
        num_affected_nets++;

        //Flag to say we've marked this net.
        temp_net_cost[net] = 1.;
    }
}

static void update_net_bb(const ClusterNetId net, int iblk, const ClusterBlockId blk, const ClusterPinId blk_pin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.net_sinks(net).size() < SMALL_NET) {
        //For small nets brute-force bounding box update is faster

        if (bb_updated_before[net] == NOT_UPDATED_YET) { //Only once per-net
            get_non_updateable_bb(net, &ts_bb_coord_new[net]);
        }
    } else {
        //For large nets, update bounding box incrementally
        int iblk_pin = cluster_ctx.clb_nlist.pin_physical_index(blk_pin);

        t_type_ptr blk_type = cluster_ctx.clb_nlist.block_type(blk);
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

static void update_td_delta_costs(const PlaceDelayModel& delay_model, const ClusterNetId net, const ClusterPinId pin, double& delta_timing_cost) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        //This pin is a net driver on a moved block.
        //Re-compute all point to point connections for this net.
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
            float temp_delay = comp_td_point_to_point_delay(delay_model, net, ipin);
            temp_point_to_point_delay[net][ipin] = temp_delay;

            temp_point_to_point_timing_cost[net][ipin] = get_timing_place_crit(net, ipin) * temp_delay;
            delta_timing_cost += temp_point_to_point_timing_cost[net][ipin] - point_to_point_timing_cost[net][ipin];
        }
    } else {
        //This pin is a net sink on a moved block
        VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin) == PinType::SINK);

        //If this net is being driven by a moved block, we do not
        //need to compute the change in the timing cost (here) since it will
        //be computed by the net's driver pin (since the driver block moved).
        //
        //Computing it here would double count the change, and mess up the
        //delta_timing_cost value.
        if (!driven_by_moved_block(net)) {
            int net_pin = cluster_ctx.clb_nlist.pin_net_index(pin);

            float temp_delay = comp_td_point_to_point_delay(delay_model, net, net_pin);
            temp_point_to_point_delay[net][net_pin] = temp_delay;

            temp_point_to_point_timing_cost[net][net_pin] = get_timing_place_crit(net, net_pin) * temp_delay;
            delta_timing_cost += temp_point_to_point_timing_cost[net][net_pin] - point_to_point_timing_cost[net][net_pin];
        }
    }
}

static bool find_to(t_type_ptr type, float rlim, const t_pl_loc from, t_pl_loc& to) {
    //Finds a legal swap to location for the given type, starting from 'x_from' and 'y_from'
    //
    //Note that the range limit (rlim) is applied in a logical sense (i.e. 'compressed' grid space consisting
    //of the same block types, and not the physical grid space). This means, for example, that columns of 'rare'
    //blocks (e.g. DSPs/RAMs) which are physically far appart but logically adjacent will be swappable even
    //at an rlim fo 1.
    //
    //This ensures that such blocks don't get locked down too early during placement (as would be the
    //case with a physical distance rlim)
    auto& grid = g_vpr_ctx.device().grid;

    auto grid_type = grid[from.x][from.y].type;
    VTR_ASSERT(type == grid_type);

    //Retrieve the compressed block grid for this block type
    const auto& compressed_block_grid = f_compressed_block_grids[type->index];

    //Determine the rlim in each dimension
    int rlim_x = min<int>(compressed_block_grid.compressed_to_grid_x.size(), rlim);
    int rlim_y = min<int>(compressed_block_grid.compressed_to_grid_y.size(), rlim); /* for aspect_ratio != 1 case. */

    //Determine the coordinates in the compressed grid space of the current block
    int cx_from = grid_to_compressed(compressed_block_grid.compressed_to_grid_x, from.x);
    int cy_from = grid_to_compressed(compressed_block_grid.compressed_to_grid_y, from.y);

    //Determin the valid compressed grid location ranges
    int min_cx = std::max(0, cx_from - rlim_x);
    int max_cx = std::min<int>(compressed_block_grid.compressed_to_grid_x.size() - 1, cx_from + rlim_x);
    int delta_cx = max_cx - min_cx;

    int min_cy = std::max(0, cy_from - rlim_y);
    int max_cy = std::min<int>(compressed_block_grid.compressed_to_grid_y.size() - 1, cy_from + rlim_y);

    int cx_to = OPEN;
    int cy_to = OPEN;
    std::unordered_set<int> tried_cx_to;
    bool legal = false;
    while (!legal && (int)tried_cx_to.size() < delta_cx) { //Until legal or all possibilities exhaused
        //Pick a random x-location within [min_cx, max_cx],
        //until we find a legal swap, or have exhuasted all possiblites
        cx_to = min_cx + vtr::irand(delta_cx);

        VTR_ASSERT(cx_to >= min_cx);
        VTR_ASSERT(cx_to <= max_cx);

        //Record this x location as tried
        auto res = tried_cx_to.insert(cx_to);
        if (!res.second) {
            continue; //Already tried this position
        }

        //Pick a random y location
        //
        //We are careful here to consider that there may be a sparse
        //set of candidate blocks in the y-axis at this x location.
        //
        //The candidates are stored in a flat_map so we can efficiently find the set of valid
        //candidates with upper/lower bound.
        auto y_lower_iter = compressed_block_grid.grid[cx_to].lower_bound(min_cy);
        if (y_lower_iter == compressed_block_grid.grid[cx_to].end()) {
            continue;
        }

        auto y_upper_iter = compressed_block_grid.grid[cx_to].upper_bound(max_cy);

        if (y_lower_iter->first > min_cy) {
            //No valid blocks at this x location which are within rlim_y
            //
            //Fall back to allow the whole y range
            y_lower_iter = compressed_block_grid.grid[cx_to].begin();
            y_upper_iter = compressed_block_grid.grid[cx_to].end();

            min_cy = y_lower_iter->first;
            max_cy = (y_upper_iter - 1)->first;
        }

        int y_range = std::distance(y_lower_iter, y_upper_iter);
        VTR_ASSERT(y_range >= 0);

        //At this point we know y_lower_iter and y_upper_iter
        //bound the range of valid blocks at this x-location, which
        //are within rlim_y
        std::unordered_set<int> tried_dy;
        while (!legal && (int)tried_dy.size() < y_range) { //Until legal or all possibilities exhausted
            //Randomly pick a y location
            int dy = vtr::irand(y_range - 1);

            //Record this y location as tried
            auto res2 = tried_dy.insert(dy);
            if (!res2.second) {
                continue; //Already tried this position
            }

            //Key in the y-dimension is the compressed index location
            cy_to = (y_lower_iter + dy)->first;

            VTR_ASSERT(cy_to >= min_cy);
            VTR_ASSERT(cy_to <= max_cy);

            if (cx_from == cx_to && cy_from == cy_to) {
                continue; //Same from/to location -- try again for new y-position
            } else {
                legal = true;
            }
        }
    }

    if (!legal) {
        //No valid position found
        return false;
    }

    VTR_ASSERT(cx_to != OPEN);
    VTR_ASSERT(cy_to != OPEN);

    //Convert to true (uncompressed) grid locations
    to.x = compressed_block_grid.compressed_to_grid_x[cx_to];
    to.y = compressed_block_grid.compressed_to_grid_y[cy_to];

    //Each x/y location contains only a single type, so we can pick a random
    //z (capcity) location
    to.z = vtr::irand(type->capacity - 1);

    auto& device_ctx = g_vpr_ctx.device();
    VTR_ASSERT_MSG(device_ctx.grid[to.x][to.y].type == type, "Type must match");
    VTR_ASSERT_MSG(device_ctx.grid[to.x][to.y].width_offset == 0, "Should be at block base location");
    VTR_ASSERT_MSG(device_ctx.grid[to.x][to.y].height_offset == 0, "Should be at block base location");

    return true;
}

static e_swap_result assess_swap(double delta_c, double t) {
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

/*returns the delay of one point to point connection */
static float comp_td_point_to_point_delay(const PlaceDelayModel& delay_model, ClusterNetId net_id, int ipin) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    float delay_source_to_sink = 0.;

    if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        //Only estimate delay for signals routed through the inter-block
        //routing network. TODO: Do how should we compute the delay for globals. "Global signals are assumed to have zero delay."

        ClusterPinId source_pin = cluster_ctx.clb_nlist.net_driver(net_id);
        ClusterPinId sink_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);

        ClusterBlockId source_block = cluster_ctx.clb_nlist.pin_block(source_pin);
        ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(sink_pin);

        int source_block_ipin = cluster_ctx.clb_nlist.pin_physical_index(source_pin);
        int sink_block_ipin = cluster_ctx.clb_nlist.pin_physical_index(sink_pin);

        int source_x = place_ctx.block_locs[source_block].loc.x;
        int source_y = place_ctx.block_locs[source_block].loc.y;
        int sink_x = place_ctx.block_locs[sink_block].loc.x;
        int sink_y = place_ctx.block_locs[sink_block].loc.y;

        /* Note: This heuristic only considers delta_x and delta_y, a much better heuristic
         *       would be to to create a more comprehensive lookup table.
         *
         *       In particular this aproach does not accurately capture the effect of fast
         *       carry-chain connections.
         */
        delay_source_to_sink = delay_model.delay(source_x,
                                                 source_y,
                                                 source_block_ipin,
                                                 sink_x,
                                                 sink_y,
                                                 sink_block_ipin);
        if (delay_source_to_sink < 0) {
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                      "in comp_td_point_to_point_delay: Bad delay_source_to_sink value %g from %s (at %d,%d) to %s (at %d,%d)\n"
                      "in comp_td_point_to_point_delay: Delay is less than 0\n",
                      block_type_pin_index_to_name(cluster_ctx.clb_nlist.block_type(source_block), source_block_ipin).c_str(),
                      source_x, source_y,
                      block_type_pin_index_to_name(cluster_ctx.clb_nlist.block_type(sink_block), sink_block_ipin).c_str(),
                      sink_x, sink_y,
                      delay_source_to_sink);
        }
    }

    return (delay_source_to_sink);
}

//Recompute all point to point delays, updating point_to_point_delay
static void comp_td_point_to_point_delays(const PlaceDelayModel& delay_model) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            point_to_point_delay[net_id][ipin] = comp_td_point_to_point_delay(delay_model, net_id, ipin);
        }
    }
}

/* Update the point_to_point_timing_cost values from the temporary *
 * values for all connections that have changed.                   */
static void update_td_cost() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Go through all the blocks moved. */
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        ClusterBlockId bnum = blocks_affected.moved_blocks[iblk].block_num;
        for (ClusterPinId pin_id : cluster_ctx.clb_nlist.block_pins(bnum)) {
            ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(pin_id);

            if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
                continue;

            if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
                //This net is being driven by a moved block, recompute
                //all point to point connections on this net.
                for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                    point_to_point_delay[net_id][ipin] = temp_point_to_point_delay[net_id][ipin];
                    temp_point_to_point_delay[net_id][ipin] = INVALID_DELAY;
                    point_to_point_timing_cost[net_id][ipin] = temp_point_to_point_timing_cost[net_id][ipin];
                    temp_point_to_point_timing_cost[net_id][ipin] = INVALID_DELAY;
                }
            } else {
                //This pin is a net sink on a moved block
                VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::SINK);

                /* The following "if" prevents the value from being updated twice. */
                if (!driven_by_moved_block(net_id)) {
                    int net_pin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

                    point_to_point_delay[net_id][net_pin] = temp_point_to_point_delay[net_id][net_pin];
                    temp_point_to_point_delay[net_id][net_pin] = INVALID_DELAY;
                    point_to_point_timing_cost[net_id][net_pin] = temp_point_to_point_timing_cost[net_id][net_pin];
                    temp_point_to_point_timing_cost[net_id][net_pin] = INVALID_DELAY;
                }
            }
        } /* Finished going through all the pins in the moved block */
    }     /* Finished going through all the blocks moved */
}

static bool driven_by_moved_block(const ClusterNetId net) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    ClusterBlockId net_driver_block = cluster_ctx.clb_nlist.net_driver_block(net);
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
        if (net_driver_block == blocks_affected.moved_blocks[iblk].block_num) {
            return true;
        }
    }
    return false;
}

static void comp_td_costs(const PlaceDelayModel& delay_model, double* timing_cost) {
    /* Computes the cost (from scratch) from the delays and criticalities    *
     * of all point to point connections, we define the timing cost of       *
     * each connection as criticality*delay.                                 */

    auto& cluster_ctx = g_vpr_ctx.clustering();

    double new_timing_cost = 0.;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) { /* For each net ... */

        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Do only if not ignored. */
            continue;
        }

        for (unsigned ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            float conn_delay = comp_td_point_to_point_delay(delay_model, net_id, ipin);
            float conn_timing_cost = conn_delay * get_timing_place_crit(net_id, ipin);

            point_to_point_delay[net_id][ipin] = conn_delay;
            temp_point_to_point_delay[net_id][ipin] = INVALID_DELAY;

            point_to_point_timing_cost[net_id][ipin] = conn_timing_cost;
            temp_point_to_point_timing_cost[net_id][ipin] = INVALID_DELAY;
            new_timing_cost += conn_timing_cost;
        }
    }

    /* Make sure timing cost does not go above MIN_TIMING_COST. */
    *timing_cost = new_timing_cost;
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

/* Frees the major structures needed by the placer (and not needed       *
 * elsewhere).   */
static void free_placement_structs(t_placer_opts placer_opts) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    free_legal_placements();
    free_fast_cost_update();

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
        || placer_opts.enable_timing_computations) {
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            /*add one to the address since it is indexed from 1 not 0 */
            point_to_point_timing_cost[net_id]++;
            free(point_to_point_timing_cost[net_id]);

            temp_point_to_point_timing_cost[net_id]++;
            free(temp_point_to_point_timing_cost[net_id]);

            point_to_point_delay[net_id]++;
            free(point_to_point_delay[net_id]);

            temp_point_to_point_delay[net_id]++;
            free(temp_point_to_point_delay[net_id]);
        }

        point_to_point_timing_cost.clear();
        point_to_point_delay.clear();
        temp_point_to_point_timing_cost.clear();
        temp_point_to_point_delay.clear();

        net_pin_indices.clear();
    }

    free_placement_macros_structs();

    /* Frees up all the data structure used in vpr_utils. */
    free_port_pin_from_blk_pin();
    free_blk_pin_from_port_pin();
}

/* Allocates the major structures needed only by the placer, primarily for *
 * computing costs quickly and such.                                       */
static void alloc_and_load_placement_structs(float place_cost_exp,
                                             t_placer_opts placer_opts,
                                             t_direct_inf* directs,
                                             int num_directs) {
    int max_pins_per_clb, i;
    unsigned int ipin;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    init_placement_context();

    alloc_legal_placements();
    load_legal_placements();

    max_pins_per_clb = 0;
    for (i = 0; i < device_ctx.num_block_types; i++) {
        max_pins_per_clb = max(max_pins_per_clb, device_ctx.block_types[i].num_pins);
    }

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
        || placer_opts.enable_timing_computations) {
        /* Allocate structures associated with timing driven placement */
        /* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]  */
        point_to_point_delay.resize(num_nets);
        temp_point_to_point_delay.resize(num_nets);

        point_to_point_timing_cost.resize(num_nets);
        temp_point_to_point_timing_cost.resize(num_nets);

        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            size_t num_sinks = cluster_ctx.clb_nlist.net_sinks(net_id).size();
            /* In the following, subract one so index starts at *
             * 1 instead of 0 */
            point_to_point_delay[net_id] = (float*)vtr::malloc(num_sinks * sizeof(float));
            point_to_point_delay[net_id]--;

            temp_point_to_point_delay[net_id] = (float*)vtr::malloc(num_sinks * sizeof(float));
            temp_point_to_point_delay[net_id]--;

            point_to_point_timing_cost[net_id] = (double*)vtr::malloc(num_sinks * sizeof(double));
            point_to_point_timing_cost[net_id]--;

            temp_point_to_point_timing_cost[net_id] = (double*)vtr::malloc(num_sinks * sizeof(double));
            temp_point_to_point_timing_cost[net_id]--;
        }
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
                point_to_point_delay[net_id][ipin] = 0;
                temp_point_to_point_delay[net_id][ipin] = 0;
            }
        }
    }

    net_cost.resize(num_nets, -1.);
    temp_net_cost.resize(num_nets, -1.);
    bb_coords.resize(num_nets, t_bb());
    bb_num_on_edges.resize(num_nets, t_bb());

    /* Used to store costs for moves not yet made and to indicate when a net's   *
     * cost has been recomputed. temp_net_cost[inet] < 0 means net's cost hasn't *
     * been recomputed.                                                          */
    bb_updated_before.resize(num_nets, NOT_UPDATED_YET);

    alloc_and_load_for_fast_cost_update(place_cost_exp);

    alloc_and_load_net_pin_indices();

    alloc_and_load_try_swap_structs();

    place_ctx.pl_macros = alloc_and_load_placement_macros(directs, num_directs);
}

/* Allocates and loads net_pin_indices array, this array allows us to quickly   *
 * find what pin on the net a block pin corresponds to. Returns the pointer   *
 * to the 2D net_pin_indices array.                                             */
static void alloc_and_load_net_pin_indices() {
    unsigned int netpin;
    int itype, max_pins_per_clb = 0;

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Compute required size. */
    for (itype = 0; itype < device_ctx.num_block_types; itype++)
        max_pins_per_clb = max(max_pins_per_clb, device_ctx.block_types[itype].num_pins);

    /* Allocate for maximum size. */
    net_pin_indices.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        net_pin_indices[blk_id].resize(max_pins_per_clb);

    /* Load the values */
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id))
            continue;
        netpin = 0;
        for (auto pin_id : cluster_ctx.clb_nlist.net_pins(net_id)) {
            int pin_index = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
            ClusterBlockId block_id = cluster_ctx.clb_nlist.pin_block(pin_id);
            net_pin_indices[block_id][pin_index] = netpin;
            netpin++;
        }
    }
}

static void alloc_and_load_try_swap_structs() {
    /* Allocate the local bb_coordinate storage, etc. only once. */
    /* Allocate with size cluster_ctx.clb_nlist.nets().size() for any number of nets affected. */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t num_nets = cluster_ctx.clb_nlist.nets().size();

    ts_bb_coord_new.resize(num_nets, t_bb());
    ts_bb_edge_new.resize(num_nets, t_bb());
    ts_nets_to_update.resize(num_nets, ClusterNetId::INVALID());

    /* Allocate with size cluster_ctx.clb_nlist.blocks().size() for any number of moved blocks. */
    blocks_affected.moved_blocks = std::vector<t_pl_moved_block>(cluster_ctx.clb_nlist.blocks().size());
    blocks_affected.num_moved_blocks = 0;

    f_compressed_block_grids = create_compressed_block_grids();
}

static std::vector<t_compressed_block_grid> create_compressed_block_grids() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    //Collect the set of x/y locations for each instace of a block type
    std::vector<std::vector<vtr::Point<int>>> block_locations(device_ctx.num_block_types);
    for (size_t x = 0; x < grid.width(); ++x) {
        for (size_t y = 0; y < grid.height(); ++y) {
            const t_grid_tile& tile = grid[x][y];
            if (tile.width_offset == 0 && tile.height_offset == 0) {
                //Only record at block root location
                block_locations[tile.type->index].emplace_back(x, y);
            }
        }
    }

    std::vector<t_compressed_block_grid> compressed_type_grids(device_ctx.num_block_types);
    for (int itype = 0; itype < device_ctx.num_block_types; itype++) {
        compressed_type_grids[itype] = create_compressed_block_grid(block_locations[itype]);
    }

    return compressed_type_grids;
}

//Given a set of locations, returns a 2D matrix in a compressed space
static t_compressed_block_grid create_compressed_block_grid(const std::vector<vtr::Point<int>>& locations) {
    t_compressed_block_grid compressed_grid;

    if (locations.empty()) {
        return compressed_grid;
    }

    {
        std::vector<int> x_locs;
        std::vector<int> y_locs;

        //Record all the x/y locations seperately
        for (auto point : locations) {
            x_locs.emplace_back(point.x());
            y_locs.emplace_back(point.y());
        }

        //Uniquify x/y locations
        std::sort(x_locs.begin(), x_locs.end());
        x_locs.erase(unique(x_locs.begin(), x_locs.end()), x_locs.end());

        std::sort(y_locs.begin(), y_locs.end());
        y_locs.erase(unique(y_locs.begin(), y_locs.end()), y_locs.end());

        //The index of an x-position in x_locs corresponds to it's compressed
        //x-coordinate (similarly for y)
        compressed_grid.compressed_to_grid_x = x_locs;
        compressed_grid.compressed_to_grid_y = y_locs;
    }

    //
    //Build the compressed grid
    //

    //Create a full/dense x-dimension (since there must be at least one
    //block per x location)
    compressed_grid.grid.resize(compressed_grid.compressed_to_grid_x.size());

    //Fill-in the y-dimensions
    //
    //Note that we build the y-dimension sparsely (using a flat map), since
    //there may not be full columns of blocks at each x location, this makes
    //it efficient to find the non-empty blocks in the y dimension
    for (auto point : locations) {
        //Determine the compressed indices in the x & y dimensions
        auto x_itr = std::lower_bound(compressed_grid.compressed_to_grid_x.begin(), compressed_grid.compressed_to_grid_x.end(), point.x());
        int cx = std::distance(compressed_grid.compressed_to_grid_x.begin(), x_itr);

        auto y_itr = std::lower_bound(compressed_grid.compressed_to_grid_y.begin(), compressed_grid.compressed_to_grid_y.end(), point.y());
        int cy = std::distance(compressed_grid.compressed_to_grid_y.begin(), y_itr);

        VTR_ASSERT(cx >= 0 && cx < (int)compressed_grid.compressed_to_grid_x.size());
        VTR_ASSERT(cy >= 0 && cy < (int)compressed_grid.compressed_to_grid_y.size());

        VTR_ASSERT(compressed_grid.compressed_to_grid_x[cx] == point.x());
        VTR_ASSERT(compressed_grid.compressed_to_grid_y[cy] == point.y());

        auto result = compressed_grid.grid[cx].insert(std::make_pair(cy, t_type_loc(point.x(), point.y())));

        VTR_ASSERT_MSG(result.second, "Duplicates should not exist in compressed grid space");
    }

    return compressed_grid;
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
    pnum = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);
    x = place_ctx.block_locs[bnum].loc.x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

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
        pnum = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

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

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb* bbptr) {
    /* WMF: Finds the estimate of wirelength due to one net by looking at   *
     * its coordinate bounding box.                                         */

    double ncost, crossing;
    auto& cluster_ctx = g_vpr_ctx.clustering();

    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if (((cluster_ctx.clb_nlist.net_pins(net_id).size()) > 50)
        && ((cluster_ctx.clb_nlist.net_pins(net_id).size()) < 85)) {
        crossing = 2.7933 + 0.02616 * ((cluster_ctx.clb_nlist.net_pins(net_id).size()) - 50);
    } else if ((cluster_ctx.clb_nlist.net_pins(net_id).size()) >= 85) {
        crossing = 2.7933 + 0.011 * (cluster_ctx.clb_nlist.net_pins(net_id).size())
                   - 0.0000018 * (cluster_ctx.clb_nlist.net_pins(net_id).size())
                         * (cluster_ctx.clb_nlist.net_pins(net_id).size());
    } else {
        crossing = cross_count[cluster_ctx.clb_nlist.net_pins(net_id).size() - 1];
    }

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

    /* Get the expected "crossing count" of a net, based on its number *
     * of pins.  Extrapolate for very large nets.                      */

    if ((cluster_ctx.clb_nlist.net_pins(net_id).size()) > 50) {
        crossing = 2.7933 + 0.02616 * ((cluster_ctx.clb_nlist.net_pins(net_id).size()) - 50);
        /*    crossing = 3.0;    Old value  */
    } else {
        crossing = cross_count[(cluster_ctx.clb_nlist.net_pins(net_id).size()) - 1];
    }

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
    pnum = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);
    x = place_ctx.block_locs[bnum].loc.x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
    y = place_ctx.block_locs[bnum].loc.y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

    xmin = x;
    ymin = y;
    xmax = x;
    ymax = y;

    for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
        bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
        pnum = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
        x = place_ctx.block_locs[bnum].loc.x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
        y = place_ctx.block_locs[bnum].loc.y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

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

static void alloc_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    legal_pos = new t_pl_loc*[device_ctx.num_block_types];
    num_legal_pos = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));

    /* Initialize all occupancy to zero. */

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            place_ctx.grid_blocks[i][j].usage = 0;

            for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                    place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID;
                    if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                        num_legal_pos[device_ctx.grid[i][j].type->index]++;
                    }
                }
            }
        }
    }

    for (int i = 0; i < device_ctx.num_block_types; i++) {
        legal_pos[i] = new t_pl_loc[num_legal_pos[i]];
    }
}

static void load_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    int* index = (int*)vtr::calloc(device_ctx.num_block_types, sizeof(int));

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] == INVALID_BLOCK_ID) {
                    continue;
                }
                if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                    int itype = device_ctx.grid[i][j].type->index;
                    legal_pos[itype][index[itype]].x = i;
                    legal_pos[itype][index[itype]].y = j;
                    legal_pos[itype][index[itype]].z = k;
                    index[itype]++;
                }
            }
        }
    }
    free(index);
}

static void free_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();

    for (int i = 0; i < device_ctx.num_block_types; i++) {
        delete[] legal_pos[i];
    }
    delete[] legal_pos; /* Free the mapping list */
    free(num_legal_pos);
}

static int check_macro_can_be_placed(int imacro, int itype, t_pl_loc head_pos) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    // Every macro can be placed until proven otherwise
    int macro_can_be_placed = true;

    auto& pl_macros = place_ctx.pl_macros;

    // Check whether all the members can be placed
    for (size_t imember = 0; imember < pl_macros[imacro].members.size(); imember++) {
        t_pl_loc member_pos = head_pos + pl_macros[imacro].members[imember].offset;

        // Check whether the location could accept block of this type
        // Then check whether the location could still accomodate more blocks
        // Also check whether the member position is valid, that is the member's location
        // still within the chip's dimemsion and the member_z is allowed at that location on the grid
        if (member_pos.x < int(device_ctx.grid.width()) && member_pos.y < int(device_ctx.grid.height())
            && device_ctx.grid[member_pos.x][member_pos.y].type->index == itype
            && place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.z] == EMPTY_BLOCK_ID) {
            // Can still accomodate blocks here, check the next position
            continue;
        } else {
            // Cant be placed here - skip to the next try
            macro_can_be_placed = false;
            break;
        }
    }

    return (macro_can_be_placed);
}

static int try_place_macro(int itype, int ipos, int imacro) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    int macro_placed = false;

    // Choose a random position for the head
    t_pl_loc head_pos = legal_pos[itype][ipos];

    // If that location is occupied, do nothing.
    if (place_ctx.grid_blocks[head_pos.x][head_pos.y].blocks[head_pos.z] != EMPTY_BLOCK_ID) {
        return (macro_placed);
    }

    int macro_can_be_placed = check_macro_can_be_placed(imacro, itype, head_pos);

    if (macro_can_be_placed) {
        auto& pl_macros = place_ctx.pl_macros;

        // Place down the macro
        macro_placed = true;
        for (size_t imember = 0; imember < pl_macros[imacro].members.size(); imember++) {
            t_pl_loc member_pos = head_pos + pl_macros[imacro].members[imember].offset;

            ClusterBlockId iblk = pl_macros[imacro].members[imember].blk_index;
            place_ctx.block_locs[iblk].loc = member_pos;

            place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.z] = pl_macros[imacro].members[imember].blk_index;
            place_ctx.grid_blocks[member_pos.x][member_pos.y].usage++;

            // Could not ensure that the randomiser would not pick this location again
            // So, would have to do a lazy removal - whenever I come across a block that could not be placed,
            // go ahead and remove it from the legal_pos[][] array

        } // Finish placing all the members in the macro

    } // End of this choice of legal_pos

    return (macro_placed);
}

static void initial_placement_pl_macros(int macros_max_num_tries, int* free_locations) {
    int macro_placed;
    int itype, itry, ipos;
    ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    auto& pl_macros = place_ctx.pl_macros;

    /* Macros are harder to place.  Do them first */
    for (size_t imacro = 0; imacro < place_ctx.pl_macros.size(); imacro++) {
        // Every macro are not placed in the beginnning
        macro_placed = false;

        // Assume that all the blocks in the macro are of the same type
        blk_id = pl_macros[imacro].members[0].blk_index;
        itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;
        if (free_locations[itype] < int(pl_macros[imacro].members.size())) {
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                      "Initial placement failed.\n"
                      "Could not place macro length %zu with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
                      "VPR cannot auto-size for your circuit, please resize the FPGA manually.\n",
                      pl_macros[imacro].members.size(), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.block_types[itype].name, itype);
        }

        // Try to place the macro first, if can be placed - place them, otherwise try again
        for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
            // Choose a random position for the head
            ipos = vtr::irand(free_locations[itype] - 1);

            // Try to place the macro
            macro_placed = try_place_macro(itype, ipos, imacro);

        } // Finished all tries

        if (macro_placed == false) {
            // if a macro still could not be placed after macros_max_num_tries times,
            // go through the chip exhaustively to find a legal placement for the macro
            // place the macro on the first location that is legal
            // then set macro_placed = true;
            // if there are no legal positions, error out

            // Exhaustive placement of carry macros
            for (ipos = 0; ipos < free_locations[itype] && macro_placed == false; ipos++) {
                // Try to place the macro
                macro_placed = try_place_macro(itype, ipos, imacro);

            } // Exhausted all the legal placement position for this macro

            // If macro could not be placed after exhaustive placement, error out
            if (macro_placed == false) {
                // Error out
                vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                          "Initial placement failed.\n"
                          "Could not place macro length %zu with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
                          "Please manually size the FPGA because VPR can't do this yet.\n",
                          pl_macros[imacro].members.size(), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.block_types[itype].name, itype);
            }

        } else {
            // This macro has been placed successfully, proceed to place the next macro
            continue;
        }
    } // Finish placing all the pl_macros successfully
}

/* Place blocks that are NOT a part of any macro.
 * We'll randomly place each block in the clustered netlist, one by one. */
static void initial_placement_blocks(int* free_locations, enum e_pad_loc_type pad_loc_type) {
    int itype, ipos;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (place_ctx.block_locs[blk_id].loc.x != -1) { // -1 is a sentinel for an empty block
            // block placed.
            continue;
        }

        /* Don't do IOs if the user specifies IOs; we'll read those locations later. */
        if (!(is_io_type(cluster_ctx.clb_nlist.block_type(blk_id)) && pad_loc_type == USER)) {
            /* Randomly select a free location of the appropriate type for blk_id.
             * We have a linearized list of all the free locations that can
             * accomodate a block of that type in free_locations[itype].
             * Choose one randomly and put blk_id there. Then we don't want to pick
             * that location again, so remove it from the free_locations array.
             */
            itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;
            if (free_locations[itype] <= 0) {
                vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                          "Initial placement failed.\n"
                          "Could not place block %s (#%zu); no free locations of type %s (#%d).\n",
                          cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.block_types[itype].name, itype);
            }

            t_pl_loc to;
            initial_placement_location(free_locations, blk_id, ipos, to);

            // Make sure that the position is EMPTY_BLOCK before placing the block down
            VTR_ASSERT(place_ctx.grid_blocks[to.x][to.y].blocks[to.z] == EMPTY_BLOCK_ID);

            place_ctx.grid_blocks[to.x][to.y].blocks[to.z] = blk_id;
            place_ctx.grid_blocks[to.x][to.y].usage++;

            place_ctx.block_locs[blk_id].loc = to;

            //Mark IOs as fixed if specifying a (fixed) random placement
            if (is_io_type(cluster_ctx.clb_nlist.block_type(blk_id)) && pad_loc_type == RANDOM) {
                place_ctx.block_locs[blk_id].is_fixed = true;
            }

            /* Ensure randomizer doesn't pick this location again, since it's occupied. Could shift all the
             * legal positions in legal_pos to remove the entry (choice) we just used, but faster to
             * just move the last entry in legal_pos to the spot we just used and decrement the
             * count of free_locations. */
            legal_pos[itype][ipos] = legal_pos[itype][free_locations[itype] - 1]; /* overwrite used block position */
            free_locations[itype]--;
        }
    }
}

static void initial_placement_location(const int* free_locations, ClusterBlockId blk_id, int& ipos, t_pl_loc& to) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;

    ipos = vtr::irand(free_locations[itype] - 1);
    to = legal_pos[itype][ipos];
}

static void initial_placement(enum e_pad_loc_type pad_loc_type,
                              const char* pad_loc_file) {
    /* Randomly places the blocks to create an initial placement. We rely on
     * the legal_pos array already being loaded.  That legal_pos[itype] is an
     * array that gives every legal value of (x,y,z) that can accomodate a block.
     * The number of such locations is given by num_legal_pos[itype].
     */
    int itype, ipos;
    int* free_locations; /* [0..device_ctx.num_block_types-1].
                          * Stores how many locations there are for this type that *might* still be free.
                          * That is, this stores the number of entries in legal_pos[itype] that are worth considering
                          * as you look for a free location.
                          */
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    free_locations = (int*)vtr::malloc(device_ctx.num_block_types * sizeof(int));
    for (itype = 0; itype < device_ctx.num_block_types; itype++) {
        free_locations[itype] = num_legal_pos[itype];
    }

    /* We'll use the grid to record where everything goes. Initialize to the grid has no
     * blocks placed anywhere.
     */
    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            place_ctx.grid_blocks[i][j].usage = 0;
            itype = device_ctx.grid[i][j].type->index;
            for (int k = 0; k < device_ctx.block_types[itype].capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                    place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID;
                }
            }
        }
    }

    /* Similarly, mark all blocks as not being placed yet. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        place_ctx.block_locs[blk_id].loc = t_pl_loc();
    }

    initial_placement_pl_macros(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, free_locations);

    // All the macros are placed, update the legal_pos[][] array
    for (itype = 0; itype < device_ctx.num_block_types; itype++) {
        VTR_ASSERT(free_locations[itype] >= 0);
        for (ipos = 0; ipos < free_locations[itype]; ipos++) {
            t_pl_loc pos = legal_pos[itype][ipos];

            // Check if that location is occupied.  If it is, remove from legal_pos
            if (place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.z] != EMPTY_BLOCK_ID && place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.z] != INVALID_BLOCK_ID) {
                legal_pos[itype][ipos] = legal_pos[itype][free_locations[itype] - 1];
                free_locations[itype]--;

                // After the move, I need to check this particular entry again
                ipos--;
                continue;
            }
        }
    } // Finish updating the legal_pos[][] and free_locations[] array

    initial_placement_blocks(free_locations, pad_loc_type);

    if (pad_loc_type == USER) {
        read_user_pad_loc(pad_loc_file);
    }

    /* Restore legal_pos */
    load_legal_placements();

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
    free(free_locations);
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
            chany_place_cost_fac[high][low] = (high - low + 1.)
                                              / chany_place_cost_fac[high][low];
            chany_place_cost_fac[high][low] = pow((double)chany_place_cost_fac[high][low], (double)place_cost_exp);
        }
}

static void check_place(const t_placer_costs& costs,
                        const PlaceDelayModel& delay_model,
                        enum e_place_algorithm place_algorithm) {
    /* Checks that the placement has not confused our data structures. *
     * i.e. the clb and block structures agree about the locations of  *
     * every block, blocks are in legal spots, etc.  Also recomputes   *
     * the final placement cost from scratch and makes sure it is      *
     * within roundoff of what we think the cost is.                   */

    int error = 0;

    error += check_placement_consistency();
    error += check_placement_costs(costs, delay_model, place_algorithm);

    if (error == 0) {
        VTR_LOG("\n");
        VTR_LOG("Completed placement consistency check successfully.\n");

    } else {
        vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                  "\nCompleted placement consistency check, %d errors found.\n"
                  "Aborting program.\n",
                  error);
    }
}

static int check_placement_costs(const t_placer_costs& costs,
                                 const PlaceDelayModel& delay_model,
                                 enum e_place_algorithm place_algorithm) {
    int error = 0;
    double bb_cost_check;
    double timing_cost_check;

    bb_cost_check = comp_bb_cost(CHECK);
    if (fabs(bb_cost_check - costs.bb_cost) > costs.bb_cost * ERROR_TOL) {
        VTR_LOG_ERROR("bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
                      bb_cost_check, costs.bb_cost);
        error++;
    }

    if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        comp_td_costs(delay_model, &timing_cost_check);
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

                if (cluster_ctx.clb_nlist.block_type(bnum) != device_ctx.grid[i][j].type) {
                    VTR_LOG_ERROR("Block %zu type (%s) does not match grid location (%zu,%zu) type (%s).\n",
                                  size_t(bnum), cluster_ctx.clb_nlist.block_type(bnum)->name, i, j, device_ctx.grid[i][j].type->name);
                    error++;
                }
                if ((place_ctx.block_locs[bnum].loc.x != int(i)) || (place_ctx.block_locs[bnum].loc.y != int(j))) {
                    VTR_LOG_ERROR("Block %zu's location is (%d,%d,%d) but found in grid at (%zu,%zu,%d).\n",
                                  size_t(bnum), place_ctx.block_locs[bnum].loc.x, place_ctx.block_locs[bnum].loc.y, place_ctx.block_locs[bnum].loc.z,
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
            if (place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.z] != member_iblk) {
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
        fprintf(fp, "#%d\t%s\t(%d, %d, %d).\n", i, cluster_ctx.clb_nlist.block_name(i), place_ctx.block_locs[i].x, place_ctx.block_locs[i].y, place_ctx.block_locs[i].z);
    }

    fclose(fp);
}
#endif

static void free_try_swap_arrays() {
    blocks_affected.moved_blocks.clear();
    blocks_affected.num_moved_blocks = 0;
    f_compressed_block_grids.clear();
}

static void calc_placer_stats(t_placer_statistics& stats, float& success_rat, double& std_dev, const t_placer_costs& costs, const int move_lim) {
    success_rat = ((float)stats.success_sum) / move_lim;
    if (stats.success_sum == 0) {
        stats.av_cost = costs.cost;
        stats.av_bb_cost = costs.bb_cost;
        stats.av_timing_cost = costs.timing_cost;
    } else {
        stats.av_cost /= stats.success_sum;
        stats.av_bb_cost /= stats.success_sum;
        stats.av_timing_cost /= stats.success_sum;
    }

    std_dev = get_std_dev(stats.success_sum, stats.sum_of_squares, stats.av_cost);
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

#ifdef DEBUG_ABORTED_MOVES
static void log_move_abort(std::string reason) {
    ++f_move_abort_reasons[reason];
#else
static void log_move_abort(std::string /*reason*/) {
#endif
}

static void report_aborted_moves() {
#ifdef DEBUG_ABORTED_MOVES
    VTR_LOG("\n");
    VTR_LOG("Aborted Move Reasons:\n");
    for (auto kv : f_move_abort_reasons) {
        VTR_LOG("  %s: %zu\n", kv.first.c_str(), kv.second);
    }
#endif
}

static int grid_to_compressed(const std::vector<int>& coords, int point) {
    auto itr = std::lower_bound(coords.begin(), coords.end(), point);
    VTR_ASSERT(*itr == point);

    return std::distance(coords.begin(), itr);
}

static void print_place_status_header() {
    VTR_LOG("------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
    VTR_LOG("      T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha\n");
    VTR_LOG("------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
}

static void print_place_status(const float t,
                               const float oldt,
                               const t_placer_statistics& stats,
                               const float cpd,
                               const float sTNS,
                               const float sWNS,
                               const float acc_rate,
                               const float std_dev,
                               const float rlim,
                               const float crit_exponent,
                               size_t tot_moves) {
    VTR_LOG(
        "%7.1e "
        "%7.3f %10.2f %-10.5g "
        "%7.3f % 10.3g % 8.3f "
        "%7.3f %7.4f %6.1f %8.2f",
        oldt,
        stats.av_cost, stats.av_bb_cost, stats.av_timing_cost,
        1e9 * cpd, 1e9 * sTNS, 1e9 * sWNS,
        acc_rate, std_dev, rlim, crit_exponent);

    pretty_print_uint(" ", tot_moves, 10, 3);

    VTR_LOG(" %6.3f\n", t / oldt);
    fflush(stdout);
}
