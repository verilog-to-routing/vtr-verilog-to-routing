#include <cstdio>
#include <cmath>
#include <memory>
#include <fstream>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_random.h"
#include "vtr_matrix.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "read_place.h"
#include "draw.h"
#include "place_and_route.h"
#include "net_delay.h"
#include "path_delay.h"
#include "timing_place_lookup.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "vpr_utils.h"
#include "place_macro.h"
#include "histogram.h"
#include "place_util.h"

#include "PlacementDelayCalculator.h"
#include "timing_util.h"
#include "timing_info.h"
#include "tatum/echo_writer.hpp"

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
	NORMAL, CHECK
};

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum e_swap_result {
	REJECTED, ACCEPTED, ABORTED
};

struct t_placer_statistics {
	double av_cost, av_bb_cost, av_timing_cost,
	       sum_of_squares, av_delay_cost;
	int success_sum;
};

struct t_placer_costs {
    float cost;
    float bb_cost;
    float timing_cost;
    float delay_cost;
};

struct t_placer_prev_inverse_costs {
    float bb_cost;
    float timing_cost;
};

#define MAX_INV_TIMING_COST 1.e9
/* Stops inverse timing cost from going to infinity with very lax timing constraints,
which avoids multiplying by a gigantic prev_inverse.timing_cost when auto-normalizing.
The exact value of this cost has relatively little impact, but should not be
large enough to be on the order of timing costs for normal constraints. */

/********************** Variables local to place.c ***************************/

/* Cost of a net, and a temporary cost of a net used during move assessment. */
static vtr::vector<ClusterNetId, float> net_cost, temp_net_cost;

static t_legal_pos **legal_pos = nullptr; /* [0..device_ctx.num_block_types-1][0..type_tsize - 1] */
static int *num_legal_pos = nullptr; /* [0..num_legal_pos-1] */

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

static vtr::vector<ClusterNetId, float *> point_to_point_timing_cost;
static vtr::vector<ClusterNetId, float *> temp_point_to_point_timing_cost;

/* [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]. What is the value of the delay */
/* for each connection in the circuit */
static vtr::vector_map<ClusterNetId, float *> point_to_point_delay_cost;
static vtr::vector_map<ClusterNetId, float *> temp_point_to_point_delay_cost;

/* [0..cluster_ctx.clb_nlist.blocks().size()-1][0..pins_per_clb-1]. Indicates which pin on the net */
/* this block corresponds to, this is only required during timing-driven */
/* placement. It is used to allow us to update individual connections on */
/* each net */
static vtr::vector<ClusterBlockId,std::vector<int>> net_pin_indices;

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

/* The pl_macros array stores all the carry chains placement macros.   *
 * [0...num_pl_macros-1]                                               */
static t_pl_macro * pl_macros = nullptr;
static int num_pl_macros;

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
static const float cross_count[50] = { /* [0..49] */1.0, 1.0, 1.0, 1.0828, 1.1536, 1.2206, 1.2823, 1.3385, 1.3991, 1.4493, 1.4974,
		1.5455, 1.5937, 1.6418, 1.6899, 1.7304, 1.7709, 1.8114, 1.8519, 1.8924,
		1.9288, 1.9652, 2.0015, 2.0379, 2.0743, 2.1061, 2.1379, 2.1698, 2.2016,
		2.2334, 2.2646, 2.2958, 2.3271, 2.3583, 2.3895, 2.4187, 2.4479, 2.4772,
		2.5064, 2.5356, 2.5610, 2.5864, 2.6117, 2.6371, 2.6625, 2.6887, 2.7148,
		2.7410, 2.7671, 2.7933 };

/********************* Static subroutines local to place.c *******************/
#ifdef VERBOSE
	static void print_clb_placement(const char *fname);
#endif

static void alloc_and_load_placement_structs(
		float place_cost_exp, t_placer_opts placer_opts,
		t_direct_inf *directs, int num_directs);

static void alloc_and_load_net_pin_indices();

static void alloc_and_load_try_swap_structs();

static void free_placement_structs(t_placer_opts placer_opts);

static void alloc_and_load_for_fast_cost_update(float place_cost_exp);

static void free_fast_cost_update();

static void alloc_legal_placements();
static void load_legal_placements();

static void free_legal_placements();

static int check_macro_can_be_placed(int imacro, int itype, int x, int y, int z);

static int try_place_macro(int itype, int ipos, int imacro);

static void initial_placement_pl_macros(int macros_max_num_tries, int * free_locations);

static void initial_placement_blocks(int * free_locations, enum e_pad_loc_type pad_loc_type);
static void initial_placement_location(int * free_locations, ClusterBlockId blk_id,
		int *pipos, int *px, int *py, int *pz);

static void initial_placement(enum e_pad_loc_type pad_loc_type,
		const char *pad_loc_file);

static float comp_bb_cost(e_cost_methods method);

static int setup_blocks_affected(ClusterBlockId b_from, int x_to, int y_to, int z_to);

static int find_affected_blocks(ClusterBlockId b_from, int x_to, int y_to, int z_to);

static e_swap_result try_swap(float t,
        t_placer_costs* costs,
        t_placer_prev_inverse_costs* prev_inverse_costs,
		float rlim,
        enum e_place_algorithm place_algorithm, float timing_tradeoff);

static ClusterBlockId pick_from_block();

static void check_place(const t_placer_costs& costs, enum e_place_algorithm place_algorithm);

static float starting_t(t_placer_costs* costs,
        t_placer_prev_inverse_costs* prev_inverse_costs,
		t_annealing_sched annealing_sched, int max_moves, float rlim,
		enum e_place_algorithm place_algorithm, float timing_tradeoff);

static void update_t(float *t, float rlim, float success_rat,
		t_annealing_sched annealing_sched);

static void update_rlim(float *rlim, float success_rat, const DeviceGrid& grid);

static int exit_crit(float t, float cost,
		t_annealing_sched annealing_sched);

static int count_connections();

static double get_std_dev(int n, double sum_x_squared, double av_x);

static float recompute_bb_cost();

static float comp_td_point_to_point_delay(ClusterNetId net_id, int ipin);

static void comp_td_point_to_point_delays();

static void update_td_cost();

static bool driven_by_moved_block(const ClusterNetId net);

static void comp_td_costs(float *timing_cost, float *connection_delay_sum);

static e_swap_result assess_swap(float delta_c, float t);

static bool find_to(t_type_ptr type, float rlim,
		int x_from, int y_from,
		int *px_to, int *py_to, int *pz_to);
static void find_to_location(t_type_ptr type, float rlim,
		int x_from, int y_from,
		int *px_to, int *py_to, int *pz_to);

static void get_non_updateable_bb(ClusterNetId net_id, t_bb *bb_coord_new);

static void update_bb(ClusterNetId net_id, t_bb *bb_coord_new,
		t_bb *bb_edge_new, int xold, int yold, int xnew, int ynew);

static int find_affected_nets_and_update_costs(e_place_algorithm place_algorithm, float& bb_delta_c, float& timing_delta_c, float& delay_delta_c);

static void record_affected_net(const ClusterNetId net, int& num_affected_nets);

static void update_net_bb(const ClusterNetId net, int iblk, const ClusterBlockId blk, const ClusterPinId blk_pin);
static void update_td_delta_costs(const ClusterNetId net, const ClusterPinId pin, float& delta_timing_cost, float& delta_delay_cost);

static float get_net_cost(ClusterNetId net_id, t_bb *bb_ptr);

static void get_bb_from_scratch(ClusterNetId net_id, t_bb *coords,
		t_bb *num_on_edges);

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb *bbptr);

static void free_try_swap_arrays();

static void outer_loop_recompute_criticalities(t_placer_opts placer_opts,
    t_placer_costs* costs,
    t_placer_prev_inverse_costs* prev_inverse_costs,
	int num_connections, float crit_exponent,
	float * place_delay_value,
	int * outer_crit_iter_count, 
    const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
#ifdef ENABLE_CLASSIC_VPR_STA
    t_slack* slacks,
    t_timing_inf timing_inf,
#endif
    SetupTimingInfo& timing_info);

static void placement_inner_loop(float t, float rlim, t_placer_opts placer_opts,
	int move_lim,
	float crit_exponent, int inner_recompute_limit,
	t_placer_statistics *stats,
    t_placer_costs* costs,
    t_placer_prev_inverse_costs* prev_inverse_costs,
    int* moves_since_cost_recompute,
#ifdef ENABLE_CLASSIC_VPR_STA
    t_slack* slacks,
    t_timing_inf timing_inf,
#endif
    const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
    SetupTimingInfo& timing_info);

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts, t_placer_costs* costs);

static void calc_placer_stats(t_placer_statistics& stats, float& success_rat, double& std_dev, const t_placer_costs& costs, const int move_lim);

/*****************************************************************************/
void try_place(t_placer_opts placer_opts,
		t_annealing_sched annealing_sched,
		t_chan_width_dist chan_width_dist, t_router_opts router_opts,
		t_det_routing_arch *det_routing_arch, t_segment_inf * segment_inf,
#ifdef ENABLE_CLASSIC_VPR_STA
		t_timing_inf timing_inf,
#endif
        t_direct_inf *directs, int num_directs) {

	/* Does almost all the work of placing a circuit.  Width_fac gives the   *
	 * width of the widest channel.  Place_cost_exp says what exponent the   *
	 * width should be taken to when calculating costs.  This allows a       *
	 * greater bias for anisotropic architectures.                           */

	int tot_iter, move_lim, moves_since_cost_recompute, width_fac, num_connections,
		outer_crit_iter_count, inner_recompute_limit;
	float t, success_rat, rlim, place_delay_value,
		  oldt, crit_exponent,
		  first_rlim, final_rlim, inverse_delta_rlim;

    t_placer_costs costs;
    t_placer_prev_inverse_costs prev_inverse_costs;

    tatum::TimingPathInfo critical_path;
    float sTNS = NAN;
    float sWNS = NAN;

	double std_dev;
	char msg[vtr::bufsize];
	t_placer_statistics stats;
#ifdef ENABLE_CLASSIC_VPR_STA
	t_slack * slacks = NULL;
#endif

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::shared_ptr<SetupTimingInfo> timing_info;
    std::shared_ptr<PlacementDelayCalculator> placement_delay_calc;

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
		alloc_lookups_and_criticalities(chan_width_dist, router_opts, det_routing_arch, segment_inf, directs, num_directs);

#ifdef ENABLE_CLASSIC_VPR_STA
        slacks = alloc_and_load_timing_graph(timing_inf);
#endif
	}

	width_fac = placer_opts.place_chan_width;

	init_chan(width_fac, chan_width_dist);

	alloc_and_load_placement_structs(placer_opts.place_cost_exp, placer_opts,
			directs, num_directs);

	initial_placement(placer_opts.pad_loc_type, placer_opts.pad_loc_file.c_str());
	init_draw_coords((float) width_fac);

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

		place_delay_value = 0;

        //Update the point-to-point delays from the initial placement
        comp_td_point_to_point_delays();

        /*
         * Initialize timing analysis
         */
        auto& atom_ctx = g_vpr_ctx.atom();
        placement_delay_calc = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, point_to_point_delay_cost);
        timing_info = make_setup_timing_info(placement_delay_calc);

        timing_info->update();
        timing_info->set_warn_unconstrained(false); //Don't warn again about unconstrained nodes again during placement

        //Initial slack estimates
        load_criticalities(*timing_info, crit_exponent, netlist_pin_lookup);

        critical_path = timing_info->least_slack_critical_path();

        //Write out the initial timing echo file
        if(isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();

            tatum::write_echo(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                    *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());
        }

#ifdef ENABLE_CLASSIC_VPR_STA
        load_timing_graph_net_delays(point_to_point_delay_cost);
		do_timing_analysis(slacks, timing_inf, false, true);

        float cpd_diff_ns = std::abs(get_critical_path_delay() - 1e9*critical_path.delay());
        if(cpd_diff_ns > ERROR_TOL) {
            print_classic_cpds();
            print_tatum_cpds(timing_info->critical_paths());

            vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, "Classic VPR and Tatum critical paths do not match (%g and %g respectively)", get_critical_path_delay(), 1e9*critical_path.delay());
        }
#endif

		/*now we can properly compute costs  */
		comp_td_costs(&costs.timing_cost, &costs.delay_cost); /*also updates values in point_to_point_delay_cost */

		if (getEchoEnabled()) {
#ifdef ENABLE_CLASSIC_VPR_STA
			if(isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_SLACK))
				print_slack(slacks->slack, false, getEchoFileName(E_ECHO_INITIAL_PLACEMENT_SLACK));
			if(isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_CRITICALITY))
				print_criticality(slacks, getEchoFileName(E_ECHO_INITIAL_PLACEMENT_CRITICALITY));
#endif
		}
		outer_crit_iter_count = 1;

		prev_inverse_costs.timing_cost = 1 / costs.timing_cost;
		prev_inverse_costs.bb_cost = 1 / costs.bb_cost;
		costs.cost = 1; /*our new cost function uses normalized values of           */
		/*bb_cost and timing_cost, the value of cost will be reset  */
		/*to 1 at each temperature when *_TIMING_DRIVEN_PLACE is true */
	} else { /*BOUNDING_BOX_PLACE */
		costs.cost = costs.bb_cost = comp_bb_cost(NORMAL);
		costs.timing_cost = 0;
		costs.delay_cost = 0;
		place_delay_value = 0;
		outer_crit_iter_count = 0;
		num_connections = 0;
		crit_exponent = 0;

		prev_inverse_costs.timing_cost = 0; /*inverses not used */
		prev_inverse_costs.bb_cost = 0;
	}

    //Sanity check that initial placement is legal
    check_place(costs, placer_opts.place_algorithm);

    //Initial pacement statistics
    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g delay_cost: %g\n",
            costs.cost, costs.bb_cost, costs.timing_cost, costs.delay_cost);
	if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        VTR_LOG("Initial placement estimated Critical Path Delay (CPD): %g ns\n",
                1e9*critical_path.delay());
        VTR_LOG("Initial placement estimated setup Total Negative Slack (sTNS): %g ns\n",
                1e9*timing_info->setup_total_negative_slack());
        VTR_LOG("Initial placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
                1e9*timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");

        VTR_LOG("Initial placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
    }
    VTR_LOG("\n");

    //Table header
	VTR_LOG("%7s "
                     "%7s %10s %10s %10s "
                     "%10s %7s %10s %8s "
                     "%7s %7s %7s %6s "
                     "%9s %6s\n",
                     "-------",
                     "-------", "----------", "----------", "----------",
                     "----------", "-------", "----------", "--------",
                     "-------", "-------", "-------", "------",
                     "---------", "------");
	VTR_LOG("%7s "
                     "%7s %10s %10s %10s "
                     "%10s %7s %10s %8s "
                     "%7s %7s %7s %6s "
                     "%9s %6s\n",
                     "T",
                     "Cost", "Av BB Cost", "Av TD Cost", "Av Tot Del",
                     "P to P Del", "CPD", "sTNS", "sWNS",
                     "Ac Rate", "Std Dev", "R limit", "Exp",
                     "Tot Moves", "Alpha");
	VTR_LOG("%7s "
                     "%7s %10s %10s %10s "
                     "%10s %7s %10s %8s "
                     "%7s %7s %7s %6s "
                     "%9s %6s\n",
                     "-------",
                     "-------", "----------", "----------", "----------",
                     "----------", "-------", "----------", "--------",
                     "-------", "-------", "-------", "------",
                     "---------", "------");

	sprintf(msg, "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g  Delay Cost: %g \t Channel Factor: %d",
		costs.cost, costs.bb_cost, costs.timing_cost, costs.delay_cost, width_fac);

    //Draw the initial placement
	update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

	move_lim = (int) (annealing_sched.inner_num * pow(cluster_ctx.clb_nlist.blocks().size(), 1.3333));

	/* Sometimes I want to run the router with a random placement.  Avoid *
	 * using 0 moves to stop division by 0 and 0 length vector problems,  *
	 * by setting move_lim to 1 (which is still too small to do any       *
	 * significant optimization).                                         */
	if (move_lim <= 0)
		move_lim = 1;

	if (placer_opts.inner_loop_recompute_divider != 0) {
		inner_recompute_limit = (int)
			(0.5 + (float) move_lim	/ (float) placer_opts.inner_loop_recompute_divider);
    } else {
		/*don't do an inner recompute */
		inner_recompute_limit = move_lim + 1;
    }

	rlim = (float) max(device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);

	first_rlim = rlim; /*used in timing-driven placement for exponent computation */
	final_rlim = 1;
	inverse_delta_rlim = 1 / (first_rlim - final_rlim);

	t = starting_t(&costs, &prev_inverse_costs,
			annealing_sched, move_lim, rlim,
			placer_opts.place_algorithm, placer_opts.timing_tradeoff);

	tot_iter = 0;
	moves_since_cost_recompute = 0;




	/* Outer loop of the simmulated annealing begins */
	while (exit_crit(t, costs.cost, annealing_sched) == 0) {

		if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
			costs.cost = 1;
		}

		outer_loop_recompute_criticalities(placer_opts, &costs, &prev_inverse_costs,
            num_connections,
			crit_exponent, &place_delay_value,
			&outer_crit_iter_count,
            netlist_pin_lookup,
#ifdef ENABLE_CLASSIC_VPR_STA
            slacks,
            timing_inf,
#endif
            *timing_info);

		placement_inner_loop(t, rlim, placer_opts,
			move_lim, crit_exponent, inner_recompute_limit, &stats,
			&costs,
            &prev_inverse_costs,
            &moves_since_cost_recompute,
#ifdef ENABLE_CLASSIC_VPR_STA
            slacks,
            timing_inf,
#endif
            netlist_pin_lookup,
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

        VTR_LOG("%7.3f "
                         "%7.4f %10.4f %-10.5g %-10.5g "
                         "%-10.5g %7.3f % 10.3g % 8.3f "
                         "%7.4f %7.4f %7.4f %6.3f"
                         "%9d %6.3f\n",
                         oldt,
                         stats.av_cost, stats.av_bb_cost, stats.av_timing_cost, stats.av_delay_cost,
                         place_delay_value, 1e9*critical_path.delay(), 1e9*sTNS, 1e9*sWNS,
                         success_rat, std_dev, rlim, crit_exponent,
                         tot_iter, t / oldt);

#ifdef ENABLE_CLASSIC_VPR_STA
        if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
            float cpd_diff_ns = std::abs(get_critical_path_delay() - 1e9*critical_path.delay());
            if(cpd_diff_ns > ERROR_TOL) {
                print_classic_cpds();
                print_tatum_cpds(timing_info->critical_paths());

                vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, "Classic VPR and Tatum critical paths do not match (%g and %g respectively)", get_critical_path_delay(), 1e9*critical_path.delay());
            }
        }
#endif

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
			crit_exponent, &place_delay_value,
			&outer_crit_iter_count,
            netlist_pin_lookup,
#ifdef ENABLE_CLASSIC_VPR_STA
            slacks,
            timing_inf,
#endif
            *timing_info);

	t = 0; /* freeze out */

	/* Run inner loop again with temperature = 0 so as to accept only swaps
	 * which reduce the cost of the placement */
	placement_inner_loop(t, rlim, placer_opts,
			move_lim, crit_exponent, inner_recompute_limit, &stats,
			&costs,
            &prev_inverse_costs,
            &moves_since_cost_recompute,
#ifdef ENABLE_CLASSIC_VPR_STA
            slacks,
            timing_inf,
#endif
            netlist_pin_lookup,
            *timing_info);

	tot_iter += move_lim;
    calc_placer_stats(stats, success_rat, std_dev, costs, move_lim);

	if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        critical_path = timing_info->least_slack_critical_path();
        sTNS = timing_info->setup_total_negative_slack();
        sWNS = timing_info->setup_worst_negative_slack();
    }

    VTR_LOG("%7.3f "
                     "%7.4f %10.4f %-10.5g %-10.5g "
                     "%-10.5g %7.3f % 10.3g % 8.3f "
                     "%7.4f %7.4f %7.4f %6.3f"
                     "%9d %6.3f\n",
                      t,
                      stats.av_cost, stats.av_bb_cost, stats.av_timing_cost, stats.av_delay_cost,
                      place_delay_value, 1e9*critical_path.delay(), 1e9*sTNS, 1e9*sWNS,
                      success_rat, std_dev, rlim, crit_exponent,
                      tot_iter, 0.);

	// TODO:
	// 1. print a message about number of aborted moves.
	// 2. add some subroutine hierarchy!  Too big!

#ifdef VERBOSE
	if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_END_CLB_PLACEMENT)) {
		print_clb_placement(getEchoFileName(E_ECHO_END_CLB_PLACEMENT));
	}
#endif

	check_place(costs, placer_opts.place_algorithm);

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
		comp_td_costs(&costs.timing_cost, &costs.delay_cost); /*computes point_to_point_delay_cost */
	}

	if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
			|| placer_opts.enable_timing_computations) {

        //Final timing estimate
        VTR_ASSERT(timing_info);
        timing_info->update(); //Tatum
		critical_path = timing_info->least_slack_critical_path();

        if(isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            auto& timing_ctx = g_vpr_ctx.timing();

            tatum::write_echo(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                    *timing_ctx.graph, *timing_ctx.constraints, *placement_delay_calc, timing_info->analyzer());
        }

#ifdef ENABLE_CLASSIC_VPR_STA
        //Old VPR analyzer
        load_timing_graph_net_delays(point_to_point_delay_cost);
		do_timing_analysis(slacks, timing_inf, false, true);
#endif


		/* Print critical path delay. */
		VTR_LOG("\n");
		VTR_LOG("Placement estimated critical path delay: %g ns",
                1e9*critical_path.delay(), get_critical_path_delay());
#ifdef ENABLE_CLASSIC_VPR_STA
		VTR_LOG(" (classic VPR STA %g ns)", get_critical_path_delay());
#endif
        VTR_LOG("\n");
        VTR_LOG("Placement estimated setup Total Negative Slack (sTNS): %g ns\n",
                1e9*timing_info->setup_total_negative_slack());
        VTR_LOG("Placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
                1e9*timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");

        VTR_LOG("Placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
        VTR_LOG("\n");

#ifdef ENABLE_CLASSIC_VPR_STA
        float cpd_diff_ns = std::abs(get_critical_path_delay() - 1e9*critical_path.delay());
        if(cpd_diff_ns > ERROR_TOL) {
            print_classic_cpds();
            print_tatum_cpds(timing_info->critical_paths());

            vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, "Classic VPR and Tatum critical paths do not match (%g and %g respectively)", get_critical_path_delay(), 1e9*critical_path.delay());
        }
#endif
	}

	sprintf(msg, "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
			costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
	VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, delay_cost: %g\n",
			costs.cost, costs.bb_cost, costs.timing_cost, costs.delay_cost);
	update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

	// Print out swap statistics
	size_t total_swap_attempts = num_swap_rejected + num_swap_accepted + num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
	float reject_rate = (float) num_swap_rejected / total_swap_attempts;
	float accept_rate = (float) num_swap_accepted / total_swap_attempts;
	float abort_rate = (float) num_swap_aborted / total_swap_attempts;
	VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits, total_swap_attempts);
	VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_accepted, 100*accept_rate);
	VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_rejected, 100*reject_rate);
	VTR_LOG("\tSwaps aborted : %*d (%4.1f %%)\n", num_swap_print_digits, num_swap_aborted, 100*abort_rate);

	free_placement_structs(placer_opts);
	if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE
			|| placer_opts.enable_timing_computations) {

#ifdef ENABLE_CLASSIC_VPR_STA
        free_timing_graph(slacks);
#endif

		free_lookups_and_criticalities();
	}

	free_try_swap_arrays();
}

/* Function to recompute the criticalities before the inner loop of the annealing */
static void outer_loop_recompute_criticalities(t_placer_opts placer_opts,
    t_placer_costs* costs,
    t_placer_prev_inverse_costs* prev_inverse_costs,
	int num_connections, float crit_exponent, 
	float * place_delay_value,
	int * outer_crit_iter_count,
    const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
#ifdef ENABLE_CLASSIC_VPR_STA
    t_slack* slacks,
    t_timing_inf timing_inf,
#endif
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

		*place_delay_value = costs->delay_cost / num_connections;

        //Per-temperature timing update
        timing_info.update();
		load_criticalities(timing_info, crit_exponent, netlist_pin_lookup);

#ifdef ENABLE_CLASSIC_VPR_STA
        load_timing_graph_net_delays(point_to_point_delay_cost);
		do_timing_analysis(slacks, timing_inf, false, true);
#endif

		/*recompute costs from scratch, based on new criticalities */
		comp_td_costs(&costs->timing_cost, &costs->delay_cost);
		*outer_crit_iter_count = 0;
	}
	(*outer_crit_iter_count)++;

	/*at each temperature change we update these values to be used     */
	/*for normalizing the tradeoff between timing and wirelength (bb)  */
    prev_inverse_costs->bb_cost = 1 / costs->bb_cost;
    /*Prevent inverse timing cost from going to infinity */
    prev_inverse_costs->timing_cost = min(1 / costs->timing_cost, (float)MAX_INV_TIMING_COST);
}

/* Function which contains the inner loop of the simulated annealing */
static void placement_inner_loop(float t, float rlim, t_placer_opts placer_opts,
    int move_lim,
	float crit_exponent, int inner_recompute_limit,
	t_placer_statistics *stats,
    t_placer_costs* costs,
    t_placer_prev_inverse_costs* prev_inverse_costs,
    int* moves_since_cost_recompute,
#ifdef ENABLE_CLASSIC_VPR_STA
	t_slack* slacks,
	t_timing_inf timing_inf,
#endif
	const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
	SetupTimingInfo& timing_info) {

	int inner_crit_iter_count, inner_iter;

	stats->av_cost = 0.;
	stats->av_bb_cost = 0.;
	stats->av_delay_cost = 0.;
	stats->av_timing_cost = 0.;
	stats->sum_of_squares = 0.;
	stats->success_sum = 0;

	inner_crit_iter_count = 1;

	/* Inner loop begins */
	for (inner_iter = 0; inner_iter < move_lim; inner_iter++) {
		e_swap_result swap_result = try_swap(t, costs, prev_inverse_costs, rlim,
			placer_opts.place_algorithm, placer_opts.timing_tradeoff);

		if (swap_result == ACCEPTED) {
			/* Move was accepted.  Update statistics that are useful for the annealing schedule. */
			stats->success_sum++;
			stats->av_cost += costs->cost;
			stats->av_bb_cost += costs->bb_cost;
			stats->av_timing_cost += costs->timing_cost;
			stats->av_delay_cost += costs->delay_cost;
			stats->sum_of_squares += (costs->cost) * (costs->cost);
			num_swap_accepted++;
		}
		else if (swap_result == ABORTED) {
			num_swap_aborted++;
		}
		else { // swap_result == REJECTED
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

#ifdef ENABLE_CLASSIC_VPR_STA
				load_timing_graph_net_delays(point_to_point_delay_cost);
				do_timing_analysis(slacks, timing_inf, false, true);
#endif

				comp_td_costs(&costs->timing_cost, &costs->delay_cost);
			}
			inner_crit_iter_count++;
		}
#ifdef VERBOSE
		VTR_LOG("t = %g  cost = %g   bb_cost = %g timing_cost = %g move = %d dmax = %g\n",
			t, costs->cost, costs->bb_cost, costs->timing_cost, inner_iter, costs->delay_cost);
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
            recompute_costs_from_scratch(placer_opts, costs); 
            *moves_since_cost_recompute = 0;
        }
	}
	/* Inner loop ends */
}

static void recompute_costs_from_scratch(const t_placer_opts& placer_opts, t_placer_costs* costs) {
    float new_bb_cost = recompute_bb_cost();
    if (fabs(new_bb_cost - costs->bb_cost) > costs->bb_cost * ERROR_TOL) {
        vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                "in recompute_costs_from_scratch: new_bb_cost = %g, old bb_cost = %g\n",
                new_bb_cost, costs->bb_cost);
    }
    costs->bb_cost = new_bb_cost;

    if (placer_opts.place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
        float new_timing_cost = 0.;
        float new_delay_cost = 0.;
        comp_td_costs(&new_timing_cost, &new_delay_cost);
        if (fabs(new_timing_cost - costs->timing_cost) > costs->timing_cost * ERROR_TOL) {
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                    "in recompute_costs_from_scratch: new_timing_cost = %g, old timing_cost = %g, ERROR_TOL = %g\n",
                    new_timing_cost, costs->timing_cost, ERROR_TOL);
        }
        if (fabs(new_delay_cost - costs->delay_cost) > costs->delay_cost * ERROR_TOL) {
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                    "in recompute_costs_from_scratch: new_delay_cost = %g, old delay_cost = %g, ERROR_TOL = %g\n",
                    new_delay_cost, costs->delay_cost, ERROR_TOL);
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
		if (cluster_ctx.clb_nlist.net_is_global(net_id))
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
		std_dev = (sum_x_squared - n * av_x * av_x) / (double) (n - 1);

	if (std_dev > 0.) /* Very small variances sometimes round negative */
		std_dev = sqrt(std_dev);
	else
		std_dev = 0.;

	return (std_dev);
}

static void update_rlim(float *rlim, float success_rat, const DeviceGrid& grid) {

	/* Update the range limited to keep acceptance prob. near 0.44.  Use *
	 * a floating point rlim to allow gradual transitions at low temps.  */

	float upper_lim;

	*rlim = (*rlim) * (1. - 0.44 + success_rat);
	upper_lim = max(grid.width() - 1, grid.height() - 1);
	*rlim = min(*rlim, upper_lim);
	*rlim = max(*rlim, (float)1.);
}

/* Update the temperature according to the annealing schedule selected. */
static void update_t(float *t, float rlim, float success_rat,
		t_annealing_sched annealing_sched) {

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

static int exit_crit(float t, float cost,
		t_annealing_sched annealing_sched) {

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
		t_annealing_sched annealing_sched, int max_moves, float rlim,
		enum e_place_algorithm place_algorithm, float timing_tradeoff) {

	/* Finds the starting temperature (hot condition).              */

	int i, num_accepted, move_lim;
	double std_dev, av, sum_of_squares; /* Double important to avoid round off */

	if (annealing_sched.type == USER_SCHED)
		return (annealing_sched.init_t);

    auto& cluster_ctx = g_vpr_ctx.clustering();

	move_lim = min(max_moves, (int) cluster_ctx.clb_nlist.blocks().size());

	num_accepted = 0;
	av = 0.;
	sum_of_squares = 0.;

	/* Try one move per block.  Set t high so essentially all accepted. */

	for (i = 0; i < move_lim; i++) {
		e_swap_result swap_result = try_swap(HUGE_POSITIVE_FLOAT, costs, prev_inverse_costs, rlim,
				place_algorithm, timing_tradeoff);

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
		VTR_LOG_WARN(
				"Starting t: %d of %d configurations accepted.\n", num_accepted, move_lim);
	}

#ifdef VERBOSE
	VTR_LOG("std_dev: %g, average cost: %g, starting temp: %g\n", std_dev, av, 20. * std_dev);
#endif

	/* Set the initial temperature to 20 times the standard of deviation */
	/* so that the initial temperature adjusts according to the circuit */
	return (20. * std_dev);
}


static int setup_blocks_affected(ClusterBlockId b_from, int x_to, int y_to, int z_to) {

	/* Find all the blocks affected when b_from is swapped with b_to.
	 * Returns abort_swap.                  */

	int imoved_blk, imacro;
	int x_from, y_from, z_from;
	ClusterBlockId b_to;
	int abort_swap = false;

    auto& place_ctx = g_vpr_ctx.mutable_placement();

	x_from = place_ctx.block_locs[b_from].x;
	y_from = place_ctx.block_locs[b_from].y;
	z_from = place_ctx.block_locs[b_from].z;

	b_to = place_ctx.grid_blocks[x_to][y_to].blocks[z_to];

	// Check whether the to_location is empty
	if (b_to == EMPTY_BLOCK_ID) {

		// Swap the block, dont swap the nets yet
		place_ctx.block_locs[b_from].x = x_to;
		place_ctx.block_locs[b_from].y = y_to;
		place_ctx.block_locs[b_from].z = z_to;

		// Sets up the blocks moved
		imoved_blk = blocks_affected.num_moved_blocks;
		blocks_affected.moved_blocks[imoved_blk].block_num = b_from;
		blocks_affected.moved_blocks[imoved_blk].xold = x_from;
		blocks_affected.moved_blocks[imoved_blk].xnew = x_to;
		blocks_affected.moved_blocks[imoved_blk].yold = y_from;
		blocks_affected.moved_blocks[imoved_blk].ynew = y_to;
		blocks_affected.moved_blocks[imoved_blk].zold = z_from;
		blocks_affected.moved_blocks[imoved_blk].znew = z_to;
		blocks_affected.moved_blocks[imoved_blk].swapped_to_was_empty = true;
		blocks_affected.moved_blocks[imoved_blk].swapped_from_is_empty = true;
		blocks_affected.num_moved_blocks ++;

	} else if (b_to != INVALID_BLOCK_ID) {

		// Does not allow a swap with a macro yet
		get_imacro_from_iblk(&imacro, b_to, pl_macros, num_pl_macros);
		if (imacro != -1) {
			abort_swap = true;
			return (abort_swap);
		}

		// Swap the block, dont swap the nets yet
		place_ctx.block_locs[b_to].x = x_from;
		place_ctx.block_locs[b_to].y = y_from;
		place_ctx.block_locs[b_to].z = z_from;

		place_ctx.block_locs[b_from].x = x_to;
		place_ctx.block_locs[b_from].y = y_to;
		place_ctx.block_locs[b_from].z = z_to;

		// Sets up the blocks moved
		imoved_blk = blocks_affected.num_moved_blocks;
		blocks_affected.moved_blocks[imoved_blk].block_num = b_from;
		blocks_affected.moved_blocks[imoved_blk].xold = x_from;
		blocks_affected.moved_blocks[imoved_blk].xnew = x_to;
		blocks_affected.moved_blocks[imoved_blk].yold = y_from;
		blocks_affected.moved_blocks[imoved_blk].ynew = y_to;
		blocks_affected.moved_blocks[imoved_blk].zold = z_from;
		blocks_affected.moved_blocks[imoved_blk].znew = z_to;
		blocks_affected.moved_blocks[imoved_blk].swapped_to_was_empty = false;
		blocks_affected.moved_blocks[imoved_blk].swapped_from_is_empty = false;
		blocks_affected.num_moved_blocks ++;

		imoved_blk = blocks_affected.num_moved_blocks;
		blocks_affected.moved_blocks[imoved_blk].block_num = b_to;
		blocks_affected.moved_blocks[imoved_blk].xold = x_to;
		blocks_affected.moved_blocks[imoved_blk].xnew = x_from;
		blocks_affected.moved_blocks[imoved_blk].yold = y_to;
		blocks_affected.moved_blocks[imoved_blk].ynew = y_from;
		blocks_affected.moved_blocks[imoved_blk].zold = z_to;
		blocks_affected.moved_blocks[imoved_blk].znew = z_from;
		blocks_affected.moved_blocks[imoved_blk].swapped_to_was_empty = false;
		blocks_affected.moved_blocks[imoved_blk].swapped_from_is_empty = false;
		blocks_affected.num_moved_blocks ++;

	} // Finish swapping the blocks and setting up blocks_affected

	return (abort_swap);

}

static int find_affected_blocks(ClusterBlockId b_from, int x_to, int y_to, int z_to) {

	/* Finds and set ups the affected_blocks array.
	 * Returns abort_swap. */

	int imacro, imember;
	int x_swap_offset, y_swap_offset, z_swap_offset, x_from, y_from, z_from;
	ClusterBlockId curr_b_from;
	int curr_x_from, curr_y_from, curr_z_from, curr_x_to, curr_y_to, curr_z_to;
	int abort_swap = false;

    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

	x_from = place_ctx.block_locs[b_from].x;
	y_from = place_ctx.block_locs[b_from].y;
	z_from = place_ctx.block_locs[b_from].z;

	get_imacro_from_iblk(&imacro, b_from, pl_macros, num_pl_macros);
	if ( imacro != -1) {
		// b_from is part of a macro, I need to swap the whole macro

		// Record down the relative position of the swap
		x_swap_offset = x_to - x_from;
		y_swap_offset = y_to - y_from;
		z_swap_offset = z_to - z_from;

		for (imember = 0; imember < pl_macros[imacro].num_blocks && abort_swap == false; imember++) {

			// Gets the new from and to info for every block in the macro
			// cannot use the old from and to info
			curr_b_from = pl_macros[imacro].members[imember].blk_index;

			curr_x_from = place_ctx.block_locs[curr_b_from].x;
			curr_y_from = place_ctx.block_locs[curr_b_from].y;
			curr_z_from = place_ctx.block_locs[curr_b_from].z;

			curr_x_to = curr_x_from + x_swap_offset;
			curr_y_to = curr_y_from + y_swap_offset;
			curr_z_to = curr_z_from + z_swap_offset;

			//Make sure that the swap_to location is valid
            //It must be:
            // * chip, and
            // * match the correct block type
            //
            //Note that we need to explicitly check that the types match, since the device floorplan is not
            //(neccessarily) translationally invariant for an arbitrary macro
			if (   curr_x_to < 1 || curr_x_to >= int(device_ctx.grid.width())
                || curr_y_to < 1 || curr_y_to >= int(device_ctx.grid.height())
                || curr_z_to < 0
                || device_ctx.grid[curr_x_to][curr_y_to].type != cluster_ctx.clb_nlist.block_type(curr_b_from)) {
				abort_swap = true;
			} else {
				abort_swap = setup_blocks_affected(curr_b_from, curr_x_to, curr_y_to, curr_z_to);
			}
		} // Finish going through all the blocks in the macro

	} else {
		// This is not a macro - I could use the from and to info from before
		abort_swap = setup_blocks_affected(b_from, x_to, y_to, z_to);

	} // Finish handling cases for blocks in macro and otherwise

	return (abort_swap);

}

static e_swap_result try_swap(float t,
        t_placer_costs* costs,
        t_placer_prev_inverse_costs* prev_inverse_costs,
		float rlim,
		enum e_place_algorithm place_algorithm,
        float timing_tradeoff) {

	/* Picks some block and moves it to another spot.  If this spot is   *
	 * occupied, switch the blocks.  Assess the change in cost function. *
	 * rlim is the range limiter.                                        *
     * Returns whether the swap is accepted, rejected or aborted.        *
	 * Passes back the new value of the cost functions.                  */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	num_ts_called ++;

	/* I'm using negative values of temp_net_cost as a flag, so DO NOT   *
	 * use cost functions that can go negative.                          */

	float delta_c = 0; /* Change in cost due to this swap. */
	float bb_delta_c = 0;
	float timing_delta_c = 0;
	float delay_delta_c = 0.0;

	/* Pick a random block to be swapped with another random block.   */
    ClusterBlockId b_from = pick_from_block();
    if (!b_from) {
        return ABORTED; //No movable block found
    }

	int x_from = place_ctx.block_locs[b_from].x;
	int y_from = place_ctx.block_locs[b_from].y;
	int z_from = place_ctx.block_locs[b_from].z;

    int x_to = OPEN;
    int y_to = OPEN;
    int z_to = OPEN;

    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid[x_from][y_from].type;
    VTR_ASSERT(cluster_from_type == grid_from_type);

	if (!find_to(cluster_ctx.clb_nlist.block_type(b_from), rlim, x_from, y_from, &x_to, &y_to, &z_to))
		return REJECTED;

#if 0
    auto& grid = g_vpr_ctx.device().grid;
	int b_to = place_ctx.grid_blocks[x_to][y_to].blocks[z_to];
	VTR_LOG( "swap [%d][%d][%d] %s \"%s\" <=> [%d][%d][%d] %s \"%s\"\n",
		x_from, y_from, z_from, grid[x_from][y_from].type->name, (b_from != -1 ? cluster_ctx.blocks[b_from].name : ""),
		x_to, y_to, z_to, grid[x_to][y_to].type->name, (b_to != -1 ? cluster_ctx.blocks[b_to].name : ""));
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

	bool abort_swap = find_affected_blocks(b_from, x_to, y_to, z_to);

	if (abort_swap == false) {

		// Find all the nets affected by this swap and update thier bounding box
		int num_nets_affected = find_affected_nets_and_update_costs(place_algorithm, bb_delta_c, timing_delta_c, delay_delta_c);

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
				/*update the point_to_point_timing_cost and point_to_point_delay_cost
				 * values from the temporary values */
				costs->timing_cost += timing_delta_c;
				costs->delay_cost += delay_delta_c;

				update_td_cost();
			}

			/* update net cost functions and reset flags. */
			for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
				ClusterNetId net_id = ts_nets_to_update[inet_affected];

				bb_coords[net_id] = ts_bb_coord_new[net_id];
				if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET)
					bb_num_on_edges[net_id] = ts_bb_edge_new[net_id];

				net_cost[net_id] = temp_net_cost[net_id];

				/* negative temp_net_cost value is acting as a flag. */
				temp_net_cost[net_id] =	-1;
				bb_updated_before[net_id] = NOT_UPDATED_YET;
			}

			/* Update clb data structures since we kept the move. */
			/* Swap physical location */
			for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {

				x_to = blocks_affected.moved_blocks[iblk].xnew;
				y_to = blocks_affected.moved_blocks[iblk].ynew;
				z_to = blocks_affected.moved_blocks[iblk].znew;

				x_from = blocks_affected.moved_blocks[iblk].xold;
				y_from = blocks_affected.moved_blocks[iblk].yold;
				z_from = blocks_affected.moved_blocks[iblk].zold;

				b_from = blocks_affected.moved_blocks[iblk].block_num;

				place_ctx.grid_blocks[x_to][y_to].blocks[z_to] = b_from;

				if (blocks_affected.moved_blocks[iblk].swapped_to_was_empty) {
					place_ctx.grid_blocks[x_to][y_to].usage++;
				}
				if (blocks_affected.moved_blocks[iblk].swapped_from_is_empty) {
					place_ctx.grid_blocks[x_from][y_from].usage--;
					place_ctx.grid_blocks[x_from][y_from].blocks[z_from] = EMPTY_BLOCK_ID;
				}

			} // Finish updating clb for all blocks

		} else { /* Move was rejected.  */

			/* Reset the net cost function flags first. */
			for (int inet_affected = 0; inet_affected < num_nets_affected; inet_affected++) {
				ClusterNetId net_id = ts_nets_to_update[inet_affected];
				temp_net_cost[net_id] = -1;
				bb_updated_before[net_id] = NOT_UPDATED_YET;
			}

			/* Restore the place_ctx.block_locs data structures to their state before the move. */
			for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
				b_from = blocks_affected.moved_blocks[iblk].block_num;

				place_ctx.block_locs[b_from].x = blocks_affected.moved_blocks[iblk].xold;
				place_ctx.block_locs[b_from].y = blocks_affected.moved_blocks[iblk].yold;
				place_ctx.block_locs[b_from].z = blocks_affected.moved_blocks[iblk].zold;
			}
		}

		/* Resets the num_moved_blocks, but do not free blocks_moved array. Defensive Coding */
		blocks_affected.num_moved_blocks = 0;

#if 0
        //Check that each accepted swap yields a valid placement
        check_place(costs, place_algorithm);
#endif

		return (keep_switch);
	} else {

		/* Restore the place_ctx.block_locs data structures to their state before the move. */
		for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
			b_from = blocks_affected.moved_blocks[iblk].block_num;

			place_ctx.block_locs[b_from].x = blocks_affected.moved_blocks[iblk].xold;
			place_ctx.block_locs[b_from].y = blocks_affected.moved_blocks[iblk].yold;
			place_ctx.block_locs[b_from].z = blocks_affected.moved_blocks[iblk].zold;
		}

		/* Resets the num_moved_blocks, but do not free blocks_moved array. Defensive Coding */
		blocks_affected.num_moved_blocks = 0;

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
        ClusterBlockId b_from = ClusterBlockId(vtr::irand((int) cluster_ctx.clb_nlist.blocks().size() - 1));

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
static int find_affected_nets_and_update_costs(e_place_algorithm place_algorithm, float& bb_delta_c, float& timing_delta_c, float& delay_delta_c) {
    VTR_ASSERT_SAFE(bb_delta_c == 0.);
    VTR_ASSERT_SAFE(timing_delta_c == 0.);
    VTR_ASSERT_SAFE(delay_delta_c == 0.);
    auto& cluster_ctx = g_vpr_ctx.clustering();

	int num_affected_nets = 0;

	//Go through all the blocks moved
	for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; iblk++) {
		ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

		//Go through all the pins in the moved block
        for (ClusterPinId blk_pin : cluster_ctx.clb_nlist.block_pins(blk)) {
			ClusterNetId net_id = cluster_ctx.clb_nlist.pin_net(blk_pin);
            VTR_ASSERT_SAFE_MSG(net_id, "Only valid nets should be found in compressed netlist block pins");

			if (cluster_ctx.clb_nlist.net_is_global(net_id))
				continue; //Global nets are assumed to span the whole chip, and do not effect costs

            //Record effected nets
            record_affected_net(net_id, num_affected_nets);

            //Update the net bounding boxes
            //
            //Do not update the net cost here since it should only be updated
            //once per net, not once per pin.
            update_net_bb(net_id, iblk, blk, blk_pin);

            if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
                //Determine the change in timing costs if required
                update_td_delta_costs(net_id, blk_pin, timing_delta_c, delay_delta_c);
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

        if(bb_updated_before[net] == NOT_UPDATED_YET) { //Only once per-net
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
                blocks_affected.moved_blocks[iblk].xold + pin_width_offset,
                blocks_affected.moved_blocks[iblk].yold + pin_height_offset,
                blocks_affected.moved_blocks[iblk].xnew + pin_width_offset,
                blocks_affected.moved_blocks[iblk].ynew + pin_height_offset);
    }

}

static void update_td_delta_costs(const ClusterNetId net, const ClusterPinId pin, float& delta_timing_cost, float& delta_delay_cost) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.pin_type(pin) == PinType::DRIVER) {
        //This pin is a net driver on a moved block.
        //Re-compute all point to point connections for this net.
        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
            float temp_delay = comp_td_point_to_point_delay(net, ipin);
            temp_point_to_point_delay_cost[net][ipin] = temp_delay;

            temp_point_to_point_timing_cost[net][ipin] = get_timing_place_crit(net, ipin) * temp_delay;
            delta_timing_cost += temp_point_to_point_timing_cost[net][ipin] - point_to_point_timing_cost[net][ipin];
            delta_delay_cost += temp_point_to_point_delay_cost[net][ipin] - point_to_point_delay_cost[net][ipin];

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

            float temp_delay = comp_td_point_to_point_delay(net, net_pin);
            temp_point_to_point_delay_cost[net][net_pin] = temp_delay;

            temp_point_to_point_timing_cost[net][net_pin] = get_timing_place_crit(net, net_pin) * temp_delay;
            delta_timing_cost += temp_point_to_point_timing_cost[net][net_pin] - point_to_point_timing_cost[net][net_pin];
            delta_delay_cost += temp_point_to_point_delay_cost[net][net_pin] - point_to_point_delay_cost[net][net_pin];
        }
    }
}

static bool find_to(t_type_ptr type, float rlim,
		int x_from, int y_from,
		int *px_to, int *py_to, int *pz_to) {

	/* Returns the point to which I want to swap, properly range limited.
	 * rlim must always be between 1 and device_ctx.grid.width() - 2 (inclusive) for this routine
	 * to work. Note -2 for no perim channels
	 */

	int min_x, max_x, min_y, max_y;
	int num_tries;
	int active_area;
	bool is_legal;
	int itype;

    auto& grid = g_vpr_ctx.device().grid;
    auto& place_ctx = g_vpr_ctx.placement();

    auto grid_type = grid[x_from][y_from].type;
	VTR_ASSERT(type == grid_type);

	int rlx = min<float>(grid.width() - 1, rlim);
	int rly = min<float>(grid.height() - 1, rlim); /* Added rly for aspect_ratio != 1 case. */
	active_area = 4 * rlx * rly;

	min_x = max<float>(0, x_from - rlx);
	max_x = min<float>(grid.width() - 1, x_from + rlx);
	min_y = max<float>(0, y_from - rly);
	max_y = min<float>(grid.height() - 1, y_from + rly);

	if (rlx < 1 || rlx > int(grid.width() - 1)) {
		vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,"in find_to: rlx = %d out of range\n", rlx);
	}
	if (rly < 1 || rly > int(grid.height() - 1)) {
		vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,"in find_to: rly = %d out of range\n", rly);
	}

	num_tries = 0;
	itype = type->index;

	do { /* Until legal */
		is_legal = true;

		/* Limit the number of tries when searching for an alternative position */
		if(num_tries >= 2 * min(active_area / (type->width * type->height), num_legal_pos[itype]) + 10) {
			/* Tried randomly searching for a suitable position */
			return false;
		} else {
			num_tries++;
		}

		find_to_location(type, rlim, x_from, y_from,
				px_to, py_to, pz_to);

		if((x_from == *px_to) && (y_from == *py_to)) {
			is_legal = false;
		} else if(*px_to > max_x || *px_to < min_x || *py_to > max_y || *py_to < min_y) {
			is_legal = false;
		} else if(grid[*px_to][*py_to].type != grid[x_from][y_from].type) {
			is_legal = false;
		} else {
			/* Find z_to and test to validate that the "to" block is *not* fixed */
			*pz_to = 0;
			if (grid[*px_to][*py_to].type->capacity > 1) {
				*pz_to = vtr::irand(grid[*px_to][*py_to].type->capacity - 1);
			}
			ClusterBlockId b_to = place_ctx.grid_blocks[*px_to][*py_to].blocks[*pz_to];
			if ((b_to != EMPTY_BLOCK_ID) && (place_ctx.block_locs[b_to].is_fixed == true)) {
				is_legal = false;
			}
		}

		VTR_ASSERT(*px_to >= 0 && *px_to < int(grid.width()));
		VTR_ASSERT(*py_to >= 0 && *py_to < int(grid.height()));
	} while (is_legal == false);

	if (*px_to < 0 || *px_to > int(grid.width() - 1) || *py_to < 0 || *py_to > int(grid.height() - 1)) {
		vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,"in routine find_to: (x_to,y_to) = (%d,%d)\n", *px_to, *py_to);
	}

	VTR_ASSERT(type == grid[*px_to][*py_to].type);
	return true;
}

static void find_to_location(t_type_ptr type, float rlim,
		int x_from, int y_from,
		int *px_to, int *py_to, int *pz_to) {

    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

	int itype = type->index;


	int rlx = min<float>(grid.width() - 1, rlim);
	int rly = min<float>(grid.height() - 1, rlim); /* Added rly for aspect_ratio != 1 case. */
	int active_area = 4 * rlx * rly;

	int min_x = max<float>(0, x_from - rlx);
	int max_x = min<float>(grid.width() - 1, x_from + rlx);
	int min_y = max<float>(0, y_from - rly);
	int max_y = min<float>(grid.height() - 1, y_from + rly);

	*pz_to = 0;
	if (int(grid.width() / 4) < rlx || int(grid.height() / 4) < rly || num_legal_pos[itype] < active_area) {
		int ipos = vtr::irand(num_legal_pos[itype] - 1);
		*px_to = legal_pos[itype][ipos].x;
		*py_to = legal_pos[itype][ipos].y;
		*pz_to = legal_pos[itype][ipos].z;
	} else {
		int x_rel = vtr::irand(max(0, max_x - min_x));
		int y_rel = vtr::irand(max(0, max_y - min_y));
		*px_to = min_x + x_rel;
		*py_to = min_y + y_rel;
		*px_to = (*px_to) - grid[*px_to][*py_to].width_offset; /* align it */
		*py_to = (*py_to) - grid[*px_to][*py_to].height_offset; /* align it */
	}
}

static e_swap_result assess_swap(float delta_c, float t) {

	/* Returns: 1 -> move accepted, 0 -> rejected. */
	if (delta_c <= 0) {
        return ACCEPTED;
	}

	if (t == 0.) {
		return REJECTED;
    }

	float fnum = vtr::frand();
	float prob_fac = exp(-delta_c / t);
	if (prob_fac > fnum) {
        return ACCEPTED;
	}

	return REJECTED;
}

static float recompute_bb_cost() {
	/* Recomputes the cost to eliminate roundoff that may have accrued.  *
	 * This routine does as little work as possible to compute this new  *
	 * cost.                                                             */

	float cost = 0;

	auto& cluster_ctx = g_vpr_ctx.clustering();

	for (auto net_id : cluster_ctx.clb_nlist.nets()) { /* for each net ... */
		if (!cluster_ctx.clb_nlist.net_is_global(net_id)) { /* Do only if not global. */
			/* Bounding boxes don't have to be recomputed; they're correct. */
			cost += net_cost[net_id];
		}
	}

	return (cost);
}

/*returns the delay of one point to point connection */
static float comp_td_point_to_point_delay(ClusterNetId net_id, int ipin) {
	auto& cluster_ctx = g_vpr_ctx.clustering();
	auto& place_ctx = g_vpr_ctx.placement();

	float delay_source_to_sink = 0.;

	if (!cluster_ctx.clb_nlist.net_is_global(net_id)) {
		//Only estimate delay for signals routed through the inter-block
		//routing network. Global signals are assumed to have zero delay.

		ClusterBlockId source_block = cluster_ctx.clb_nlist.net_driver_block(net_id);
		ClusterBlockId sink_block = cluster_ctx.clb_nlist.net_pin_block(net_id, ipin);

		VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.block_type(source_block) != nullptr);
		VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.block_type(sink_block) != nullptr);

		int delta_x = abs(place_ctx.block_locs[sink_block].x - place_ctx.block_locs[source_block].x);
		int delta_y = abs(place_ctx.block_locs[sink_block].y - place_ctx.block_locs[source_block].y);

        /* Note: This heuristic only considers delta_x and delta_y, a much better heuristic
         *       would be to to create a more comprehensive lookup table.
         *
         *       In particular this aproach does not accurately capture the effect of fast
         *       carry-chain connections.
         */
        delay_source_to_sink = get_delta_delay(delta_x, delta_y);
        if (delay_source_to_sink < 0) {
            vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
                    "in comp_td_point_to_point_delay: Bad delay_source_to_sink value delta(%d, %d) delay of %g\n"
                    "in comp_td_point_to_point_delay: Delay is less than 0\n",
                    delta_x, delta_y, delay_source_to_sink);
        }
    }


	return (delay_source_to_sink);
}

//Recompute all point to point delays, updating point_to_point_delay_cost
static void comp_td_point_to_point_delays() {
	auto& cluster_ctx = g_vpr_ctx.clustering();

	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
		for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
			point_to_point_delay_cost[net_id][ipin] = comp_td_point_to_point_delay(net_id, ipin);
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

			if (cluster_ctx.clb_nlist.net_is_global(net_id))
				continue;

			if (cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::DRIVER) {
                //This net is being driven by a moved block, recompute
                //all point to point connections on this net.
				for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
					point_to_point_delay_cost[net_id][ipin] = temp_point_to_point_delay_cost[net_id][ipin];
					temp_point_to_point_delay_cost[net_id][ipin] = -1;
					point_to_point_timing_cost[net_id][ipin] = temp_point_to_point_timing_cost[net_id][ipin];
					temp_point_to_point_timing_cost[net_id][ipin] = -1;
				}
			} else {
                //This pin is a net sink on a moved block
                VTR_ASSERT_SAFE(cluster_ctx.clb_nlist.pin_type(pin_id) == PinType::SINK);

				/* The following "if" prevents the value from being updated twice. */
				if (!driven_by_moved_block(net_id)) {
                    int net_pin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

					point_to_point_delay_cost[net_id][net_pin] = temp_point_to_point_delay_cost[net_id][net_pin];
					temp_point_to_point_delay_cost[net_id][net_pin] = -1;
					point_to_point_timing_cost[net_id][net_pin] = temp_point_to_point_timing_cost[net_id][net_pin];
					temp_point_to_point_timing_cost[net_id][net_pin] = -1;
				}
			}
		} /* Finished going through all the pins in the moved block */
	} /* Finished going through all the blocks moved */
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

static void comp_td_costs(float *timing_cost, float *connection_delay_sum) {
	/* Computes the cost (from scratch) from the delays and criticalities    *
	 * of all point to point connections, we define the timing cost of       *
	 * each connection as criticality*delay.                                 */

    auto& cluster_ctx = g_vpr_ctx.clustering();

	float new_timing_cost = 0.;
	float new_connection_delay_sum = 0.;

	for (auto net_id : cluster_ctx.clb_nlist.nets()) { /* For each net ... */

		if (cluster_ctx.clb_nlist.net_is_global(net_id)) { /* Do only if not global. */
            continue;
        }

        for (unsigned ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            float conn_delay = comp_td_point_to_point_delay(net_id, ipin);
            float conn_timing_cost = conn_delay * get_timing_place_crit(net_id, ipin);

            new_connection_delay_sum += conn_delay;
            point_to_point_delay_cost[net_id][ipin] = conn_delay;
            temp_point_to_point_delay_cost[net_id][ipin] = -1; /* Undefined */

            point_to_point_timing_cost[net_id][ipin] = conn_timing_cost;
            temp_point_to_point_timing_cost[net_id][ipin] = -1; /* Undefined */
            new_timing_cost += conn_timing_cost;
        }
	}

	/* Make sure timing cost does not go above MIN_TIMING_COST. */
	*timing_cost = new_timing_cost;

	*connection_delay_sum = new_connection_delay_sum;
}


/* Finds the cost from scratch.  Done only when the placement   *
* has been radically changed (i.e. after initial placement).   *
* Otherwise find the cost change incrementally.  If method     *
* check is NORMAL, we find bounding boxes that are updateable  *
* for the larger nets.  If method is CHECK, all bounding boxes *
* are found via the non_updateable_bb routine, to provide a    *
* cost which can be used to check the correctness of the       *
* other routine.                                               */
static float comp_bb_cost(e_cost_methods method) {
	float cost = 0;
	double expected_wirelength = 0.0;
	auto& cluster_ctx = g_vpr_ctx.clustering();

	for (auto net_id : cluster_ctx.clb_nlist.nets()) { /* for each net ... */
		if (!cluster_ctx.clb_nlist.net_is_global(net_id)) { /* Do only if not global. */
			/* Small nets don't use incremental updating on their bounding boxes, *
			 * so they can use a fast bounding box calculator.                    */
			if (cluster_ctx.clb_nlist.net_sinks(net_id).size() >= SMALL_NET && method == NORMAL) {
				get_bb_from_scratch(net_id, &bb_coords[net_id],
					&bb_num_on_edges[net_id]);
			}
			else {
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
	int imacro;

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

			point_to_point_delay_cost[net_id]++;
			free(point_to_point_delay_cost[net_id]);

			temp_point_to_point_delay_cost[net_id]++;
			free(temp_point_to_point_delay_cost[net_id]);
		}

		point_to_point_timing_cost.clear();
		point_to_point_delay_cost.clear();
		temp_point_to_point_timing_cost.clear();
		temp_point_to_point_delay_cost.clear();

		net_pin_indices.clear();
	}

	free_placement_macros_structs();

	for (imacro = 0; imacro < num_pl_macros; imacro++)
		free(pl_macros[imacro].members);
	free(pl_macros);

	/* Defensive coding. */
	pl_macros = nullptr;

	/* Frees up all the data structure used in vpr_utils. */
	free_port_pin_from_blk_pin();
	free_blk_pin_from_port_pin();

}

/* Allocates the major structures needed only by the placer, primarily for *
* computing costs quickly and such.                                       */
static void alloc_and_load_placement_structs(
	float place_cost_exp, t_placer_opts placer_opts,
	t_direct_inf *directs, int num_directs) {

	int max_pins_per_clb, i;
	unsigned int ipin;

	auto& device_ctx = g_vpr_ctx.device();
	auto& cluster_ctx = g_vpr_ctx.clustering();

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
		point_to_point_delay_cost.resize(num_nets);
		temp_point_to_point_delay_cost.resize(num_nets);

		point_to_point_timing_cost.resize(num_nets);
		temp_point_to_point_timing_cost.resize(num_nets);

		for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            size_t num_sinks = cluster_ctx.clb_nlist.net_sinks(net_id).size();
			/* In the following, subract one so index starts at *
			 * 1 instead of 0 */
			point_to_point_delay_cost[net_id] = (float *)vtr::malloc(num_sinks * sizeof(float));
			point_to_point_delay_cost[net_id]--;

			temp_point_to_point_delay_cost[net_id] = (float *)vtr::malloc(num_sinks * sizeof(float));
			temp_point_to_point_delay_cost[net_id]--;

			point_to_point_timing_cost[net_id] = (float *)vtr::malloc(num_sinks * sizeof(float));
			point_to_point_timing_cost[net_id]--;

			temp_point_to_point_timing_cost[net_id] = (float *)vtr::malloc(num_sinks * sizeof(float));
			temp_point_to_point_timing_cost[net_id]--;
		}
		for (auto net_id : cluster_ctx.clb_nlist.nets()) {
			for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
				point_to_point_delay_cost[net_id][ipin] = 0;
				temp_point_to_point_delay_cost[net_id][ipin] = 0;
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

	num_pl_macros = alloc_and_load_placement_macros(directs, num_directs, &pl_macros);
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
		if (cluster_ctx.clb_nlist.net_is_global(net_id))
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
	blocks_affected.moved_blocks = (t_pl_moved_block*) vtr::calloc((int) cluster_ctx.clb_nlist.blocks().size(), sizeof(t_pl_moved_block));
	blocks_affected.num_moved_blocks = 0;

}

/* This routine finds the bounding box of each net from scratch (i.e.   *
* from only the block location information).  It updates both the       *
* coordinate and number of pins on each edge information.  It           *
* should only be called when the bounding box information is not valid. */
static void get_bb_from_scratch(ClusterNetId net_id, t_bb *coords,
		t_bb *num_on_edges) {
	int pnum, x, y, xmin, xmax, ymin, ymax;
	int xmin_edge, xmax_edge, ymin_edge, ymax_edge;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();
	auto& grid = device_ctx.grid;

	ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
	pnum = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);
	x = place_ctx.block_locs[bnum].x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
	y = place_ctx.block_locs[bnum].y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

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
		x = place_ctx.block_locs[bnum].x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
		y = place_ctx.block_locs[bnum].y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

		/* Code below counts IO blocks as being within the 1..grid.width()-2, 1..grid.height()-2 clb array. *
		 * This is because channels do not go out of the 0..grid.width()-2, 0..grid.height()-2 range, and   *
		 * I always take all channels impinging on the bounding box to be within   *
		 * that bounding box.  Hence, this "movement" of IO blocks does not affect *
		 * the which channels are included within the bounding box, and it         *
		 * simplifies the code a lot.                                              */

		x = max(min<int>(x, grid.width() - 2), 1); //-2 for no perim channels
		y = max(min<int>(y, grid.height() - 2), 1); //-2 for no perim channels

		if (x == xmin) {
			xmin_edge++;
		}
		if (x == xmax) { /* Recall that xmin could equal xmax -- don't use else */
			xmax_edge++;
		}
		else if (x < xmin) {
			xmin = x;
			xmin_edge = 1;
		}
		else if (x > xmax) {
			xmax = x;
			xmax_edge = 1;
		}

		if (y == ymin) {
			ymin_edge++;
		}
		if (y == ymax) {
			ymax_edge++;
		}
		else if (y < ymin) {
			ymin = y;
			ymin_edge = 1;
		}
		else if (y > ymax) {
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

static double get_net_wirelength_estimate(ClusterNetId net_id, t_bb *bbptr) {

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

static float get_net_cost(ClusterNetId net_id, t_bb *bbptr) {

	/* Finds the cost due to one net by looking at its coordinate bounding  *
	 * box.                                                                 */

	float ncost, crossing;
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
static void get_non_updateable_bb(ClusterNetId net_id, t_bb *bb_coord_new) {
    //TODO: account for multiple physical pin instances per logical pin

	int xmax, ymax, xmin, ymin, x, y;
	int pnum;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

	ClusterBlockId bnum = cluster_ctx.clb_nlist.net_driver_block(net_id);
	pnum = cluster_ctx.clb_nlist.net_pin_physical_index(net_id, 0);
	x = place_ctx.block_locs[bnum].x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
	y = place_ctx.block_locs[bnum].y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

	xmin = x;
	ymin = y;
	xmax = x;
	ymax = y;

	for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
		bnum = cluster_ctx.clb_nlist.pin_block(pin_id);
		pnum = cluster_ctx.clb_nlist.pin_physical_index(pin_id);
		x = place_ctx.block_locs[bnum].x + cluster_ctx.clb_nlist.block_type(bnum)->pin_width_offset[pnum];
		y = place_ctx.block_locs[bnum].y + cluster_ctx.clb_nlist.block_type(bnum)->pin_height_offset[pnum];

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

	bb_coord_new->xmin = max(min<int>(xmin, device_ctx.grid.width() - 2), 1); //-2 for no perim channels
	bb_coord_new->ymin = max(min<int>(ymin, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
	bb_coord_new->xmax = max(min<int>(xmax, device_ctx.grid.width() - 2), 1); //-2 for no perim channels
	bb_coord_new->ymax = max(min<int>(ymax, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
}

static void update_bb(ClusterNetId net_id, t_bb *bb_coord_new,
		t_bb *bb_edge_new, int xold, int yold, int xnew, int ynew) {

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

	xnew = max(min<int>(xnew, device_ctx.grid.width() - 2), 1); //-2 for no perim channels
	ynew = max(min<int>(ynew, device_ctx.grid.height() - 2), 1); //-2 for no perim channels
	xold = max(min<int>(xold, device_ctx.grid.width() - 2), 1); //-2 for no perim channels
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

	legal_pos = (t_legal_pos **) vtr::malloc(device_ctx.num_block_types * sizeof(t_legal_pos *));
	num_legal_pos = (int *) vtr::calloc(device_ctx.num_block_types, sizeof(int));

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
		legal_pos[i] = (t_legal_pos *) vtr::malloc(num_legal_pos[i] * sizeof(t_legal_pos));
	}
}

static void load_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

	int* index = (int *) vtr::calloc(device_ctx.num_block_types, sizeof(int));

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
		free(legal_pos[i]);
	}
	free(legal_pos); /* Free the mapping list */
	free(num_legal_pos);
}



static int check_macro_can_be_placed(int imacro, int itype, int x, int y, int z) {

	int imember;
	size_t member_x, member_y, member_z;

    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

	// Every macro can be placed until proven otherwise
	int macro_can_be_placed = true;

	// Check whether all the members can be placed
	for (imember = 0; imember < pl_macros[imacro].num_blocks; imember++) {
		member_x = x + pl_macros[imacro].members[imember].x_offset;
		member_y = y + pl_macros[imacro].members[imember].y_offset;
		member_z = z + pl_macros[imacro].members[imember].z_offset;

		// Check whether the location could accept block of this type
		// Then check whether the location could still accomodate more blocks
		// Also check whether the member position is valid, that is the member's location
		// still within the chip's dimemsion and the member_z is allowed at that location on the grid
		if (member_x < device_ctx.grid.width() && member_y < device_ctx.grid.height()
				&& device_ctx.grid[member_x][member_y].type->index == itype
				&& place_ctx.grid_blocks[member_x][member_y].blocks[member_z] == EMPTY_BLOCK_ID) {
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


static int try_place_macro(int itype, int ipos, int imacro){

	int x, y, z, member_x, member_y, member_z, imember;

    auto& place_ctx = g_vpr_ctx.mutable_placement();

	int macro_placed = false;

	// Choose a random position for the head
	x = legal_pos[itype][ipos].x;
	y = legal_pos[itype][ipos].y;
	z = legal_pos[itype][ipos].z;

	// If that location is occupied, do nothing.
	if (place_ctx.grid_blocks[x][y].blocks[z] != EMPTY_BLOCK_ID) {
		return (macro_placed);
	}

	int macro_can_be_placed = check_macro_can_be_placed(imacro, itype, x, y, z);

	if (macro_can_be_placed) {

		// Place down the macro
		macro_placed = true;
		for (imember = 0; imember < pl_macros[imacro].num_blocks; imember++) {

			member_x = x + pl_macros[imacro].members[imember].x_offset;
			member_y = y + pl_macros[imacro].members[imember].y_offset;
			member_z = z + pl_macros[imacro].members[imember].z_offset;

            ClusterBlockId iblk = pl_macros[imacro].members[imember].blk_index;
			place_ctx.block_locs[iblk].x = member_x;
			place_ctx.block_locs[iblk].y = member_y;
			place_ctx.block_locs[iblk].z = member_z;

			place_ctx.grid_blocks[member_x][member_y].blocks[member_z] = pl_macros[imacro].members[imember].blk_index;
			place_ctx.grid_blocks[member_x][member_y].usage++;

			// Could not ensure that the randomiser would not pick this location again
			// So, would have to do a lazy removal - whenever I come across a block that could not be placed,
			// go ahead and remove it from the legal_pos[][] array

		} // Finish placing all the members in the macro

	} // End of this choice of legal_pos

	return (macro_placed);

}


static void initial_placement_pl_macros(int macros_max_num_tries, int * free_locations) {

	int macro_placed;
	int imacro, itype, itry, ipos;
	ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

	/* Macros are harder to place.  Do them first */
	for (imacro = 0; imacro < num_pl_macros; imacro++) {

		// Every macro are not placed in the beginnning
		macro_placed = false;

		// Assume that all the blocks in the macro are of the same type
		blk_id = pl_macros[imacro].members[0].blk_index;
		itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;
		if (free_locations[itype] < pl_macros[imacro].num_blocks) {
			vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
					"Initial placement failed.\n"
					"Could not place macro length %d with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
					"VPR cannot auto-size for your circuit, please resize the FPGA manually.\n",
					pl_macros[imacro].num_blocks, cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.block_types[itype].name, itype);
		}

		// Try to place the macro first, if can be placed - place them, otherwise try again
		for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {

			// Choose a random position for the head
			ipos = vtr::irand(free_locations[itype] - 1);

			// Try to place the macro
			macro_placed = try_place_macro(itype, ipos, imacro);

		} // Finished all tries


		if (macro_placed == false){
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
						"Could not place macro length %d with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
						"Please manually size the FPGA because VPR can't do this yet.\n",
						pl_macros[imacro].num_blocks, cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.block_types[itype].name, itype);
			}

		} else {
			// This macro has been placed successfully, proceed to place the next macro
			continue;
		}
	} // Finish placing all the pl_macros successfully
}

/* Place blocks that are NOT a part of any macro.
* We'll randomly place each block in the clustered netlist, one by one. */
static void initial_placement_blocks(int * free_locations, enum e_pad_loc_type pad_loc_type) {
	int itype, ipos, x, y, z;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& device_ctx = g_vpr_ctx.device();

	for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
		if (place_ctx.block_locs[blk_id].x != -1) { // -1 is a sentinel for an empty block
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

			initial_placement_location(free_locations, blk_id, &ipos, &x, &y, &z);

			// Make sure that the position is EMPTY_BLOCK before placing the block down
			VTR_ASSERT(place_ctx.grid_blocks[x][y].blocks[z] == EMPTY_BLOCK_ID);

			place_ctx.grid_blocks[x][y].blocks[z] = blk_id;
			place_ctx.grid_blocks[x][y].usage++;

			place_ctx.block_locs[blk_id].x = x;
			place_ctx.block_locs[blk_id].y = y;
			place_ctx.block_locs[blk_id].z = z;

            //Mark IOs as fixed if specifying a (fixed) random placement
            if(is_io_type(cluster_ctx.clb_nlist.block_type(blk_id)) && pad_loc_type == RANDOM) {
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

static void initial_placement_location(int * free_locations, ClusterBlockId blk_id,
		int *pipos, int *px_to, int *py_to, int *pz_to) {

    auto& cluster_ctx = g_vpr_ctx.clustering();

	int itype = cluster_ctx.clb_nlist.block_type(blk_id)->index;

	*pipos = vtr::irand(free_locations[itype] - 1);
	*px_to = legal_pos[itype][*pipos].x;
	*py_to = legal_pos[itype][*pipos].y;
	*pz_to = legal_pos[itype][*pipos].z;
}

static void initial_placement(enum e_pad_loc_type pad_loc_type,
		const char *pad_loc_file) {

	/* Randomly places the blocks to create an initial placement. We rely on
	 * the legal_pos array already being loaded.  That legal_pos[itype] is an
	 * array that gives every legal value of (x,y,z) that can accomodate a block.
	 * The number of such locations is given by num_legal_pos[itype].
	 */
	int itype, x, y, z, ipos;
	int *free_locations; /* [0..device_ctx.num_block_types-1].
						  * Stores how many locations there are for this type that *might* still be free.
						  * That is, this stores the number of entries in legal_pos[itype] that are worth considering
						  * as you look for a free location.
						  */
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

	free_locations = (int *) vtr::malloc(device_ctx.num_block_types * sizeof(int));
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
		place_ctx.block_locs[blk_id].x = OPEN;
		place_ctx.block_locs[blk_id].y = OPEN;
		place_ctx.block_locs[blk_id].z = OPEN;
	}

	initial_placement_pl_macros(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, free_locations);

	// All the macros are placed, update the legal_pos[][] array
	for (itype = 0; itype < device_ctx.num_block_types; itype++) {
		VTR_ASSERT(free_locations[itype] >= 0);
		for (ipos = 0; ipos < free_locations[itype]; ipos++) {
			x = legal_pos[itype][ipos].x;
			y = legal_pos[itype][ipos].y;
			z = legal_pos[itype][ipos].z;

			// Check if that location is occupied.  If it is, remove from legal_pos
			if (place_ctx.grid_blocks[x][y].blocks[z] != EMPTY_BLOCK_ID && place_ctx.grid_blocks[x][y].blocks[z] != INVALID_BLOCK_ID) {
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

	chanx_place_cost_fac = (float **) vtr::malloc((device_ctx.grid.height()) * sizeof(float *));
	for (size_t i = 0; i < device_ctx.grid.height(); i++)
		chanx_place_cost_fac[i] = (float *) vtr::malloc((i + 1) * sizeof(float));

	chany_place_cost_fac = (float **) vtr::malloc((device_ctx.grid.width() + 1) * sizeof(float *));
	for (size_t i = 0; i < device_ctx.grid.width(); i++)
		chany_place_cost_fac[i] = (float *) vtr::malloc((i + 1) * sizeof(float));

	/* First compute the number of tracks between channel high and channel *
	 * low, inclusive, in an efficient manner.                             */

	chanx_place_cost_fac[0][0] = device_ctx.chan_width.x_list[0];

	for (size_t high = 1; high < device_ctx.grid.height(); high++) {
		chanx_place_cost_fac[high][high] = device_ctx.chan_width.x_list[high];
		for (size_t low = 0; low < high; low++) {
			chanx_place_cost_fac[high][low] =
					chanx_place_cost_fac[high - 1][low] + device_ctx.chan_width.x_list[high];
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
			chanx_place_cost_fac[high][low] = pow(
					(double) chanx_place_cost_fac[high][low],
					(double) place_cost_exp);
		}

	/* Now do the same thing for the y-directed channels.  First get the  *
	 * number of tracks between channel high and channel low, inclusive.  */

	chany_place_cost_fac[0][0] = device_ctx.chan_width.y_list[0];

	for (size_t high = 1; high < device_ctx.grid.width(); high++) {
		chany_place_cost_fac[high][high] = device_ctx.chan_width.y_list[high];
		for (size_t low = 0; low < high; low++) {
			chany_place_cost_fac[high][low] =
					chany_place_cost_fac[high - 1][low] + device_ctx.chan_width.y_list[high];
		}
	}

	/* Now compute the inverse of the average number of tracks per channel *
	 * between high and low.  Take to specified power.                     */

	for (size_t high = 0; high < device_ctx.grid.width(); high++)
		for (size_t low = 0; low <= high; low++) {
			chany_place_cost_fac[high][low] = (high - low + 1.)
					/ chany_place_cost_fac[high][low];
			chany_place_cost_fac[high][low] = pow(
					(double) chany_place_cost_fac[high][low],
					(double) place_cost_exp);
		}
}

static void check_place(const t_placer_costs& costs, enum e_place_algorithm place_algorithm) {

	/* Checks that the placement has not confused our data structures. *
	 * i.e. the clb and block structures agree about the locations of  *
	 * every block, blocks are in legal spots, etc.  Also recomputes   *
	 * the final placement cost from scratch and makes sure it is      *
	 * within roundoff of what we think the cost is.                   */

	vtr::vector<ClusterBlockId, int> bdone;
	int error = 0;
	ClusterBlockId bnum, head_iblk, member_iblk;
	float bb_cost_check;
	int usage_check;
	float timing_cost_check, delay_cost_check;
	int imacro, imember, member_x, member_y, member_z;

	bb_cost_check = comp_bb_cost(CHECK);
	if (fabs(bb_cost_check - costs.bb_cost) > costs.bb_cost * ERROR_TOL) {
		VTR_LOG_ERROR(
				"bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
				bb_cost_check, costs.bb_cost);
		error++;
	}

	if (place_algorithm == PATH_TIMING_DRIVEN_PLACE) {
		comp_td_costs(&timing_cost_check, &delay_cost_check);
		//VTR_LOG("timing_cost recomputed from scratch: %g\n", timing_cost_check);
		if (fabs(timing_cost_check - costs.timing_cost) > costs.timing_cost * ERROR_TOL) {
			VTR_LOG_ERROR(
					"timing_cost_check: %g and timing_cost: %g differ in check_place.\n",
					timing_cost_check, costs.timing_cost);
			error++;
		}
		//VTR_LOG("delay_cost recomputed from scratch: %g\n", delay_cost_check);
		if (fabs(delay_cost_check - costs.delay_cost) > costs.delay_cost * ERROR_TOL) {
			VTR_LOG_ERROR(
					"delay_cost_check: %g and delay_cost: %g differ in check_place.\n",
					delay_cost_check, costs.delay_cost);
			error++;
		}
	}

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

	bdone.resize(cluster_ctx.clb_nlist.blocks().size(), 0);

	/* Step through device grid and placement. Check it against blocks */
	for (size_t i = 0; i < device_ctx.grid.width(); i++)
		for (size_t j = 0; j < device_ctx.grid.height(); j++) {
			if (place_ctx.grid_blocks[i][j].usage > device_ctx.grid[i][j].type->capacity) {
				VTR_LOG_ERROR(
						"Block at grid location (%zu,%zu) overused. Usage is %d.\n",
						i, j, place_ctx.grid_blocks[i][j].usage);
				error++;
			}
			usage_check = 0;
			for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
				bnum = place_ctx.grid_blocks[i][j].blocks[k];
				if (EMPTY_BLOCK_ID == bnum || INVALID_BLOCK_ID == bnum)
					continue;

				if (cluster_ctx.clb_nlist.block_type(bnum) != device_ctx.grid[i][j].type) {
					VTR_LOG_ERROR(
							"Block %zu type (%s) does not match grid location (%zu,%zu) type (%s).\n",
							size_t(bnum), cluster_ctx.clb_nlist.block_type(bnum)->name, i, j, device_ctx.grid[i][j].type->name);
					error++;
				}
				if ((place_ctx.block_locs[bnum].x != int(i)) || (place_ctx.block_locs[bnum].y != int(j))) {
					VTR_LOG_ERROR(
							"Block %zu location conflicts with grid(%zu,%zu) data.\n",
							size_t(bnum), i, j);
					error++;
				}
				++usage_check;
				bdone[bnum]++;
			}
			if (usage_check != place_ctx.grid_blocks[i][j].usage) {
				VTR_LOG_ERROR(
						"Location (%zu,%zu) usage is %d, but has actual usage %d.\n",
						i, j, place_ctx.grid_blocks[i][j].usage, usage_check);
				error++;
			}
		}

	/* Check that every block exists in the device_ctx.grid and cluster_ctx.blocks arrays somewhere. */
	for (auto blk_id : cluster_ctx.clb_nlist.blocks())
		if (bdone[blk_id] != 1) {
			VTR_LOG_ERROR(
					"Block %zu listed %d times in data structures.\n",
					size_t(blk_id), bdone[blk_id]);
			error++;
		}
	bdone.clear();

	/* Check the pl_macro placement are legal - blocks are in the proper relative position. */
	for (imacro = 0; imacro < num_pl_macros; imacro++) {

		head_iblk = pl_macros[imacro].members[0].blk_index;

		for (imember = 0; imember < pl_macros[imacro].num_blocks; imember++) {

			member_iblk = pl_macros[imacro].members[imember].blk_index;

			// Compute the suppossed member's x,y,z location
			member_x = place_ctx.block_locs[head_iblk].x + pl_macros[imacro].members[imember].x_offset;
			member_y = place_ctx.block_locs[head_iblk].y + pl_macros[imacro].members[imember].y_offset;
			member_z = place_ctx.block_locs[head_iblk].z + pl_macros[imacro].members[imember].z_offset;

			// Check the place_ctx.block_locs data structure first
			if (place_ctx.block_locs[member_iblk].x != member_x
					|| place_ctx.block_locs[member_iblk].y != member_y
					|| place_ctx.block_locs[member_iblk].z != member_z) {
				VTR_LOG_ERROR(
						"Block %zu in pl_macro #%d is not placed in the proper orientation.\n",
						size_t(member_iblk), imacro);
				error++;
			}

			// Then check the place_ctx.grid data structure
			if (place_ctx.grid_blocks[member_x][member_y].blocks[member_z] != member_iblk) {
				VTR_LOG_ERROR(
						"Block %zu in pl_macro #%d is not placed in the proper orientation.\n",
						size_t(member_iblk), imacro);
				error++;
			}
		} // Finish going through all the members
	} // Finish going through all the macros

	if (error == 0) {
		VTR_LOG("\n");
		VTR_LOG("Completed placement consistency check successfully.\n");

	} else {
		vpr_throw(VPR_ERROR_PLACE, __FILE__, __LINE__,
				"\nCompleted placement consistency check, %d errors found.\n"
				"Aborting program.\n", error);
	}

}

#ifdef VERBOSE
static void print_clb_placement(const char *fname) {

	/* Prints out the clb placements to a file.  */
	FILE *fp;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

	fp = vtr::fopen(fname, "w");
	fprintf(fp, "Complex block placements:\n\n");

	fprintf(fp, "Block #\tName\t(X, Y, Z).\n");
	for(auto i : cluster_ctx.clb_nlist.blocks()) {
		fprintf(fp, "#%d\t%s\t(%d, %d, %d).\n", i, cluster_ctx.clb_nlist.block_name(i), place_ctx.block_locs[i].x, place_ctx.block_locs[i].y, place_ctx.block_locs[i].z);
	}

	fclose(fp);
}
#endif

static void free_try_swap_arrays() {
	if(blocks_affected.moved_blocks != nullptr) {
		free(blocks_affected.moved_blocks);

		blocks_affected.moved_blocks = nullptr;
		blocks_affected.num_moved_blocks = 0;
	}
}

static void calc_placer_stats(t_placer_statistics& stats, float& success_rat, double& std_dev, const t_placer_costs& costs, const int move_lim) {
	success_rat = ((float) stats.success_sum) / move_lim;
	if (stats.success_sum == 0) {
		stats.av_cost = costs.cost;
		stats.av_bb_cost = costs.bb_cost;
		stats.av_delay_cost = costs.delay_cost;
		stats.av_timing_cost = costs.timing_cost;
	} else {
		stats.av_cost /= stats.success_sum;
		stats.av_bb_cost /= stats.success_sum;
		stats.av_delay_cost /= stats.success_sum;
		stats.av_timing_cost /= stats.success_sum;
	}

	std_dev = get_std_dev(stats.success_sum, stats.sum_of_squares, stats.av_cost);
}
