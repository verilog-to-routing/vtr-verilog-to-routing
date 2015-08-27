#include <cstdio>
#include <ctime>
#include <cmath>

#include <assert.h>

#include <vector>
#include <algorithm>

#include "vpr_utils.h"
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "heapsort.h"
#include "path_delay.h"
#include "net_delay.h"
#include "stats.h"
#include "ReadOptions.h"
#include "profile_lookahead.h"
#include "look_ahead_bfs_precal.h"

using namespace std;

/************************* variables used by new look ahead *****************************/
typedef struct s_opin_cost_inf {
    float Tdel;
    float acc_basecost;
    float acc_C;
} t_opin_cost_inf;
// map key: seg_index; map value: cost of that seg_index from source to target
map<int, t_opin_cost_inf> opin_cost_by_seg_type;
typedef struct s_target_pin_side {
    int chan_type;
    int x;
    int y;
} t_target_pin_side;
// map key: currently expanded inode; map value: ipin side, x, y that will produce a min cost to target
map<int, t_target_pin_side> expanded_node_min_target_side;
int target_chosen_chan_type = UNDEFINED;
int target_pin_x = UNDEFINED;
int target_pin_y = UNDEFINED;

float crit_threshold;   // for switch between new & old lookahead: only use new lookahead for high critical path


/******************* variables for profiling & debugging *******************/
float basecost_for_printing;    // for printing in expand_neighbours()
float acc_route_time = 0.;      // accumulated route time: sum of router runtime over all iterations

float time_pre_itr;
float time_1st_itr;

#if PRINTCRITICALPATH == 1
bool is_first_itr = false;
float max_crit = -1;
float min_crit = 10;
int max_inet = -1;
int max_ipin = -1;
int min_inet = -1;
int min_ipin = -1;
#endif


/******************** Subroutines local to route_timing.c ********************/

static int get_max_pins_per_net(void);

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
		float target_criticality, float astar_fac);

static void timing_driven_expand_neighbours(struct s_heap *current, 
		int inet, int itry,
		float bend_cost, float criticality_fac, int target_node,
		float astar_fac, int highfanout_rlim, bool lookahead_eval, float crit);

static float get_timing_driven_expected_cost(int inode, int target_node,
		float criticality_fac, float R_upstream);

#ifdef INTERPOSER_BASED_ARCHITECTURE
static int get_num_expected_interposer_hops_to_target(int inode, int target_node);
#endif

static int get_expected_segs_to_target(int inode, int target_node,
		int *num_segs_ortho_dir_ptr);

static void update_rr_base_costs(int inet, float largest_criticality);

static void timing_driven_check_net_delays(float **net_delay);

static int mark_node_expansion_by_bin(int inet, int target_node,
		t_rt_node * rt_node);

static double get_overused_ratio();

// should_route_net is used in setup_max_min_criticality() in profile_lookahead.c
//static bool should_route_net(int inet);

static bool turn_on_bfs_map_lookup(float criticality);

static void setup_min_side_reaching_target(int inode, int target_chan_type, int target_x, int target_y, 
        float *min_Tdel, float cur_Tdel, float *C_downstream, float *future_base_cost, 
        float cur_C_downstream, float cur_basecost);

float bfs_direct_lookup(int offset_x, int offset_y, int seg_type, int chan_type, 
                int to_chan_type, float *acc_C, float *acc_basecost);

static void adjust_coord_for_wrong_dir_wire(int *start_chan_type, int *start_dir, int start_len, 
                                            int *x_start, int *y_start, int x_target, int y_target);
 
static float get_precal_Tdel(int start_node, int start_chan_type, int target_chan_type, int start_seg_type, int start_dir,
                                 int x_start, int y_start, int x_target, int y_target, int len_start,
                                 float *acc_C, float *acc_basecost);
   
static void setup_opin_cost_by_seg_type(int source_node, int target_node);

struct more_sinks_than {
	inline bool operator() (const int& net_index1, const int& net_index2) {
		return g_clbs_nlist.net[net_index1].num_sinks() > g_clbs_nlist.net[net_index2].num_sinks();
	}
};



static void congestion_analysis();
static void time_on_fanout_analysis();
constexpr int fanout_per_bin = 5;
static vector<float> time_on_fanout;
static vector<int> itry_on_fanout;

/************************ Subroutine definitions *****************************/
bool try_timing_driven_route(struct s_router_opts router_opts,
		float **net_delay, t_slack * slacks, t_ivec ** clb_opins_used_locally, 
		bool timing_analysis_enabled, const t_timing_inf &timing_inf) {

	/* Timing-driven routing algorithm.  The timing graph (includes slack)   *
	 * must have already been allocated, and net_delay must have been allocated. *
	 * Returns true if the routing succeeds, false otherwise.                    */

	const int max_fanout {get_max_pins_per_net()};
	time_on_fanout.resize((max_fanout / fanout_per_bin) + 1, 0);
	itry_on_fanout.resize((max_fanout / fanout_per_bin) + 1, 0);

	auto sorted_nets = vector<int>(g_clbs_nlist.net.size());

	for (size_t i = 0; i < g_clbs_nlist.net.size(); ++i) {
		sorted_nets[i] = i;
	}

	// sort so net with most sinks is first.
	std::sort(sorted_nets.begin(), sorted_nets.end(), more_sinks_than());

	timing_driven_route_structs route_structs{}; // allocs route strucs (calls default constructor)
	// contains:
	// float *pin_criticality; /* [1..max_pins_per_net-1] */
	// int *sink_order; /* [1..max_pins_per_net-1] */;
	// t_rt_node **rt_node_of_sink; /* [1..max_pins_per_net-1] */

	/* First do one routing iteration ignoring congestion to get reasonable net 
	   delay estimates. Set criticalities to 1 when timing analysis is on to 
	   optimize timing, and to 0 when timing analysis is off to optimize routability. */

	float init_timing_criticality_val = (timing_analysis_enabled ? 1.0 : 0.0);

	/* Variables used to do the optimization of the routing, aborting visibly
	 * impossible Ws */
	double overused_ratio;
	vector<double> historical_overuse_ratio; 
	historical_overuse_ratio.reserve(router_opts.max_router_iterations + 1);

	// for unroutable large circuits, the time per iteration seems to increase dramatically
	vector<float> time_per_iteration;
	time_per_iteration.reserve(router_opts.max_router_iterations + 1);
	
	for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
		if (g_clbs_nlist.net[inet].is_global == false) {
			for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
				slacks->timing_criticality[inet][ipin] = init_timing_criticality_val;
			}
#ifdef PATH_COUNTING
				slacks->path_criticality[inet][ipin] = init_timing_criticality_val;
#endif		
		} else { 
			/* Set delay of global signals to zero. Non-global net delays are set by
			   update_net_delays_from_route_tree() inside timing_driven_route_net(), 
			   which is only called for non-global nets. */
			for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
				net_delay[inet][ipin] = 0.;
			}
		}
	}

	float pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */
#if OPINPENALTY == 1
    opin_penalty = OPIN_INIT_PENALTY;
#else
    opin_penalty = 1;
#endif
    acc_route_time = 0.;
	for (int itry = 1; itry <= router_opts.max_router_iterations; ++itry) {
        if (itry == 1) 
            crit_threshold = CRIT_THRES_INIT;
        else 
            crit_threshold *= CRIT_THRES_INC_RATE;
        if (crit_threshold > 0.988) 
            crit_threshold = 0.988;
        if (itry == 2) {
            nodes_expanded_1st_itr = nodes_expanded_cur_itr;
            nodes_expanded_max_itr = nodes_expanded_cur_itr;
        }
        if (itry > 1) {
            nodes_expanded_pre_itr = nodes_expanded_cur_itr;
            nodes_expanded_max_itr = (nodes_expanded_cur_itr > nodes_expanded_max_itr) ? nodes_expanded_cur_itr:nodes_expanded_max_itr;
            if (nodes_expanded_pre_itr > 2 * nodes_expanded_1st_itr) {
                crit_threshold = (crit_threshold > 0.98) ? crit_threshold : 0.98;
                printf("BFS RAISE THRESHOLD to 0.98 for %d\n", itry);
            }
        }
        nodes_expanded_cur_itr = 0;
		clock_t begin = clock();
        total_nodes_expanded = 0;
        itry_share = itry;
        //if (itry <= 20)
            //opin_penalty *= 0.93;
#if OPINPENALTY == 1
        opin_penalty *= OPIN_DECAY_RATE;
#endif
#if DEPTHWISELOOKAHEADEVAL == 1
        if (router_opts.lookahead_eval) {
            subtree_count.clear();
            subtree_size_avg.clear();
            subtree_est_dev_avg.clear();
            subtree_est_dev_abs_avg.clear();
            subtree_size_avg[0] = 0;
        }
#endif
		/* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
		for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
			g_clbs_nlist.net[inet].is_routed = false;
			g_clbs_nlist.net[inet].is_fixed = false;
		}
#if PRINTCRITICALPATH == 1
        if (itry == 1) {
            is_first_itr = true;
        } else {
            is_first_itr = false;
        }
        setup_max_min_criticality(max_crit, min_crit, 
                max_inet, min_inet, max_ipin, min_ipin, slacks);
#endif
		for (unsigned int i = 0; i < g_clbs_nlist.net.size(); ++i) {
			bool is_routable = try_timing_driven_route_net(
				sorted_nets[i],
				itry,
				pres_fac,
				router_opts,
				route_structs.pin_criticality,
				route_structs.sink_order,
				route_structs.rt_node_of_sink,
				net_delay,
				slacks
			);

			if (!is_routable) {
				return (false);
			}
		}
#if PRINTCRITICALPATH == 1
        printf("MAX crit: %f\tMIN crit: %f\tmaxpin: %d\tminpin: %d\n", max_crit, min_crit, max_ipin, min_ipin);
#endif
		clock_t end = clock();

		float time = static_cast<float>(end - begin) / CLOCKS_PER_SEC;
        if (itry == 1) time_1st_itr = time;
        time_pre_itr = time;

        acc_route_time += time;
		time_per_iteration.push_back(time);
        printf("\tTOTAL NODES EXPANDED FOR ITERATION %d is %d\n", itry, total_nodes_expanded);
		if (itry == 1) {
			/* Early exit code for cases where it is obvious that a successful route will not be found 
			   Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit */
			int total_wirelength = 0;
			int available_wirelength = 0;

			for (int i = 0; i < num_rr_nodes; ++i) {
				if (rr_node[i].type == CHANX || rr_node[i].type == CHANY) {
					available_wirelength += 1 + 
							rr_node[i].get_xhigh() - rr_node[i].get_xlow() + 
							rr_node[i].get_yhigh() - rr_node[i].get_ylow();
				}
			}

			for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
				if (g_clbs_nlist.net[inet].is_global == false
						&& g_clbs_nlist.net[inet].num_sinks() != 0) { /* Globals don't count. */
					int bends, wirelength, segments;
					get_num_bends_and_length(inet, &bends, &wirelength, &segments);

					total_wirelength += wirelength;
				}
			}
			vpr_printf_info("Wire length after first iteration %d, total available wire length %d, ratio %g\n",
					total_wirelength, available_wirelength,
					(float) (total_wirelength) / (float) (available_wirelength));
			if ((float) (total_wirelength) / (float) (available_wirelength)> FIRST_ITER_WIRELENTH_LIMIT) {
				vpr_printf_info("Wire length usage ratio exceeds limit of %g, fail routing.\n",
						FIRST_ITER_WIRELENTH_LIMIT);

				time_on_fanout_analysis();
				return false;
			}
			vpr_printf_info("--------- ---------- ----------- ---------------------\n");
			vpr_printf_info("Iteration       Time   Crit Path     Overused RR Nodes\n");
			vpr_printf_info("--------- ---------- ----------- ---------------------\n");
		}

		/* Make sure any CLB OPINs used up by subblocks being hooked directly
		   to them are reserved for that purpose. */

		bool rip_up_local_opins = (itry == 1 ? false : true);
		reserve_locally_used_opins(pres_fac, router_opts.acc_fac, rip_up_local_opins, clb_opins_used_locally);

		/* Pathfinder guys quit after finding a feasible route. I may want to keep 
		   going longer, trying to improve timing.  Think about this some. */

		bool success = feasible_routing();

		/* Verification to check the ratio of overused nodes, depending on the configuration
		 * may abort the routing if the ratio is too high. */
		overused_ratio = get_overused_ratio();
		historical_overuse_ratio.push_back(overused_ratio);

		/* Determine when routing is impossible hence should be aborted */
		if (itry > 5){
			
			int expected_success_route_iter = predict_success_route_iter(historical_overuse_ratio, router_opts);
			if (expected_success_route_iter == UNDEFINED) {
				time_on_fanout_analysis();
                vpr_printf_info("\tTOTAL ROUTE TIME (SUM OF ALL ITR)\t%f\n", acc_route_time);
				return false;
			}

			if (itry > 15) {
				// compare their slopes over the last 5 iterations
				double time_per_iteration_slope = linear_regression_vector(time_per_iteration, itry-5);
				double congestion_per_iteration_slope = linear_regression_vector(historical_overuse_ratio, itry-5);
				if (router_opts.congestion_analysis)
					vpr_printf_info("%f s/iteration %f %/iteration\n", time_per_iteration_slope, congestion_per_iteration_slope);
				// time is increasing and congestion is non-decreasing (grows faster than 10% per iteration)
				if (congestion_per_iteration_slope > 0 && time_per_iteration_slope > 0.1*time_per_iteration.back()
					&& time_per_iteration_slope > 1) {	// filter out noise
					vpr_printf_info("Time per iteration growing too fast at slope %f s/iteration \n\
									 while congestion grows at %f %/iteration, unlikely to finish.\n",
						time_per_iteration_slope, congestion_per_iteration_slope);

					time_on_fanout_analysis();
                    vpr_printf_info("\tTOTAL ROUTE TIME (SUM OF ALL ITR)\t%f\n", acc_route_time);
					return false;
				}
			}
		}
        if (router_opts.lookahead_eval)
            print_lookahead_eval();
        clear_lookahead_history_array();
        // I don't clear the congestion map: it is the accumulated version across all itry
        //clear_congestion_map();
#if CONGESTIONBYCHIPAREA
        if (itry % 4 == 0)
            print_congestion_map();
        clear_congestion_map_relative();
#endif
		if (success) {
            vpr_printf_info("\tTOTAL ROUTE TIME (SUM OF ALL ITR)\t%f\n", acc_route_time);
			if (timing_analysis_enabled) {
				load_timing_graph_net_delays(net_delay);
				do_timing_analysis(slacks, timing_inf, false, false);
				float critical_path_delay = get_critical_path_delay();
                vpr_printf_info("%9d %6.2f sec %8.5f ns   %3.2e (%3.4f %)\n", itry, time, critical_path_delay, overused_ratio*num_rr_nodes, overused_ratio*100);
				vpr_printf_info("Critical path: %g ns\n", critical_path_delay);
			} else {
                vpr_printf_info("%9d %6.2f sec         N/A   %3.2e (%3.4f %)\n", itry, time, overused_ratio*num_rr_nodes, overused_ratio*100);
			}

			vpr_printf_info("Successfully routed after %d routing iterations.\n", itry);
#ifdef DEBUG
			if (timing_analysis_enabled)
				timing_driven_check_net_delays(net_delay);
#endif
			time_on_fanout_analysis();
			return (true);
		}

		if (itry == 1) {
			pres_fac = router_opts.initial_pres_fac;
			pathfinder_update_cost(pres_fac, 0.); /* Acc_fac=0 for first iter. */
		} else {
			pres_fac *= router_opts.pres_fac_mult;

			/* Avoid overflow for high iteration counts, even if acc_cost is big */
			pres_fac = min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

			pathfinder_update_cost(pres_fac, router_opts.acc_fac);
		}

		if (timing_analysis_enabled) {		
			/* Update slack values by doing another timing analysis.
			   Timing_driven_route_net updated the net delay values. */

			load_timing_graph_net_delays(net_delay);

			do_timing_analysis(slacks, timing_inf, false, false);

		} else {
			/* If timing analysis is not enabled, make sure that the criticalities and the
			   net_delays stay as 0 so that wirelength can be optimized. */
			
			for (unsigned int inet = 0; inet < g_clbs_nlist.net.size(); ++inet) {
				for (unsigned int ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ++ipin) {
					slacks->timing_criticality[inet][ipin] = 0.;
#ifdef PATH_COUNTING 		
					slacks->path_criticality[inet][ipin] = 0.; 		
#endif
					net_delay[inet][ipin] = 0.;
				}
			}
		}

		
		if (timing_analysis_enabled) {
			float critical_path_delay = get_critical_path_delay();
            vpr_printf_info("%9d %6.2f sec %8.5f ns   %3.2e (%3.4f %)\n", itry, time, critical_path_delay, overused_ratio*num_rr_nodes, overused_ratio*100);
		} else {
            vpr_printf_info("%9d %6.2f sec         N/A   %3.2e (%3.4f %)\n", itry, time, overused_ratio*num_rr_nodes, overused_ratio*100);
		}

        if (router_opts.congestion_analysis) {
        	congestion_analysis();
        }
		fflush(stdout);
	}
    vpr_printf_info("\tTOTAL ROUTE TIME (SUM OF ALL ITR)\t%f\n", acc_route_time);
	vpr_printf_info("Routing failed.\n");
	time_on_fanout_analysis();
	return (false);
}


void time_on_fanout_analysis() {
	// using the global time_on_fanout and itry_on_fanout
	vpr_printf_info("fanout low           time (s)        attemps\n");
	for (size_t bin = 0; bin < time_on_fanout.size(); ++bin) {
		if (itry_on_fanout[bin]) {	// avoid printing the many 0 bins
			vpr_printf_info("%4d           %14.3f   %12d\n",bin*fanout_per_bin, time_on_fanout[bin], itry_on_fanout[bin]);
		}
	}
}

// at the end of a routing iteration, profile how much congestion is taken up by each type of rr_node
// efficient bit array for checking against congested type
struct Congested_node_types {
	uint32_t mask;
	Congested_node_types() : mask{0} {}
	void set_congested(int rr_node_type) {mask |= (1 << rr_node_type);}
	void clear_congested(int rr_node_type) {mask &= ~(1 << rr_node_type);}
	bool is_congested(int rr_node_type) const {return mask & (1 << rr_node_type);}
	bool empty() const {return mask == 0;}
};
void congestion_analysis() {
	static const std::vector<const char*> node_typename {
		"SOURCE",
		"SINK",
		"IPIN",
		"OPIN",
		"CHANX",
		"CHANY",
		"ICE"
	};
	// each type indexes into array which holds the congestion for that type
	std::vector<int> congestion_per_type((size_t) NUM_RR_TYPES, 0);
	// print out specific node information if congestion for type is low enough

	int total_congestion = 0;
	for (int inode = 0; inode < num_rr_nodes; ++inode) {
		const t_rr_node& node = rr_node[inode];
		int congestion = node.get_occ() - node.get_capacity();

		if (congestion > 0) {
			total_congestion += congestion;
			congestion_per_type[node.type] += congestion;
		}
	}

	constexpr int specific_node_print_threshold = 5;
	Congested_node_types congested;
	for (int type = SOURCE; type < NUM_RR_TYPES; ++type) {
		float congestion_percentage = (float)congestion_per_type[type] / (float) total_congestion * 100;
		vpr_printf_info(" %6s: %10.6f %\n", node_typename[type], congestion_percentage); 
		// nodes of that type need specific printing
		if (congestion_per_type[type] > 0 &&
			congestion_per_type[type] < specific_node_print_threshold) congested.set_congested(type);
	}

	// specific print out each congested node
	if (!congested.empty()) {
		vpr_printf_info("Specific congested nodes\nxlow ylow   type\n");
		for (int inode = 0; inode < num_rr_nodes; ++inode) {
			const t_rr_node& node = rr_node[inode];
			if (congested.is_congested(node.type) && (node.get_occ() - node.get_capacity()) > 0) {
				vpr_printf_info("(%3d,%3d) %6s\n", node.get_xlow(), node.get_ylow(), node_typename[node.type]);
			}
		}
	}
}

bool try_timing_driven_route_net(int inet, int itry, float pres_fac, 
		struct s_router_opts router_opts,
		float* pin_criticality, int* sink_order,
		t_rt_node** rt_node_of_sink, float** net_delay, t_slack* slacks) {

	bool is_routed = false;

	if (g_clbs_nlist.net[inet].is_fixed) { /* Skip pre-routed nets. */
		is_routed = true;
	} else if (g_clbs_nlist.net[inet].is_global) { /* Skip global nets. */
		is_routed = true;
	} else if (should_route_net(inet) == false) {
		is_routed = true;
	} else{
		// track time spent vs fanout
		clock_t net_begin = clock();

		is_routed = timing_driven_route_net(inet, itry, pres_fac,
				router_opts.max_criticality, router_opts.criticality_exp, 
				router_opts.astar_fac, router_opts.bend_cost, 
				pin_criticality, sink_order, 
				rt_node_of_sink, net_delay[inet], slacks, router_opts.lookahead_eval);

		float time_for_net = static_cast<float>(clock() - net_begin) / CLOCKS_PER_SEC;
		int net_fanout = g_clbs_nlist.net[inet].num_sinks();
		time_on_fanout[net_fanout / fanout_per_bin] += time_for_net;
		itry_on_fanout[net_fanout / fanout_per_bin] += 1;

		/* Impossible to route? (disconnected rr_graph) */
		if (is_routed) {
			g_clbs_nlist.net[inet].is_routed = true;
			g_atoms_nlist.net[clb_to_vpack_net_mapping[inet]].is_routed = true;
		} else {
			vpr_printf_info("Routing failed.\n");
		}
	}
	return (is_routed);
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
		int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr) {

	/* Allocates all the structures needed only by the timing-driven router.   */

	int max_pins_per_net = get_max_pins_per_net();

	float *pin_criticality = (float *) my_malloc((max_pins_per_net - 1) * sizeof(float));
	*pin_criticality_ptr = pin_criticality - 1; /* First sink is pin #1. */

	int *sink_order = (int *) my_malloc((max_pins_per_net - 1) * sizeof(int));
	*sink_order_ptr = sink_order - 1;

	t_rt_node **rt_node_of_sink = (t_rt_node **) my_malloc((max_pins_per_net - 1) * sizeof(t_rt_node *));
	*rt_node_of_sink_ptr = rt_node_of_sink - 1;

	alloc_route_tree_timing_structs();
}

/* This function gets ratio of overused nodes (overused_nodes / num_rr_nodes) */
static double get_overused_ratio(){
	double overused_nodes = 0.0;
	int inode;
	for(inode = 0; inode < num_rr_nodes; inode++){
		if(rr_node[inode].get_occ() > rr_node[inode].get_capacity())
			overused_nodes += (rr_node[inode].get_occ() - rr_node[inode].get_capacity());
	}
	overused_nodes /= (double)num_rr_nodes;
	return overused_nodes;
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink) {

	/* Frees all the stuctures needed only by the timing-driven router.        */

	free(pin_criticality + 1); /* Starts at index 1. */
	free(sink_order + 1);
	free(rt_node_of_sink + 1);
	free_route_tree_timing_structs();
}

timing_driven_route_structs::timing_driven_route_structs() {
	alloc_timing_driven_route_structs(
		&pin_criticality,
		&sink_order,
		&rt_node_of_sink
	);
}

timing_driven_route_structs::~timing_driven_route_structs() {
	free_timing_driven_route_structs(
		pin_criticality,
		sink_order,
		rt_node_of_sink
	);
}

static int get_max_pins_per_net(void) {

	/* Returns the largest number of pins on any non-global net.    */

	unsigned int inet;
	int max_pins_per_net;

	max_pins_per_net = 0;
	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		if (g_clbs_nlist.net[inet].is_global == false) {
			max_pins_per_net = max(max_pins_per_net,
					(int) g_clbs_nlist.net[inet].pins.size());
		}
	}

	return (max_pins_per_net);
}

bool timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
		float criticality_exp, float astar_fac, float bend_cost,
		float *pin_criticality, int *sink_order,
		t_rt_node ** rt_node_of_sink, float *net_delay, t_slack * slacks, bool lookahead_eval) {

	/* Returns true as long is found some way to hook up this net, even if that *
	 * way resulted in overuse of resources (congestion).  If there is no way   *
	 * to route this net, even ignoring congestion, it returns false.  In this  *
	 * case the rr_graph is disconnected and you can give up. If slacks = NULL, *
	 * give each net a dummy criticality of 0.									*/
	unsigned int ipin;
	int num_sinks, itarget, target_pin, target_node, inode;
	float target_criticality, old_total_cost, new_total_cost, largest_criticality,
		old_back_cost, new_back_cost;
	t_rt_node *rt_root;
	
	struct s_trace *new_route_start_tptr;
	int highfanout_rlim;

	/* Rip-up any old routing. */

	pathfinder_update_one_cost(trace_head[inet], -1, pres_fac);
	free_traceback(inet);
	for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
		if (!slacks) {
			/* Use criticality of 1. This makes all nets critical.  Note: There is a big difference between setting pin criticality to 0
			compared to 1.  If pin criticality is set to 0, then the current path delay is completely ignored during routing.  By setting
			pin criticality to 1, the current path delay to the pin will always be considered and optimized for */
			pin_criticality[ipin] = 1.0;
		} else { 
#ifdef PATH_COUNTING
			/* Pin criticality is based on a weighted sum of timing and path criticalities. */	
			pin_criticality[ipin] =		 ROUTE_PATH_WEIGHT	* slacks->path_criticality[inet][ipin]
								  + (1 - ROUTE_PATH_WEIGHT) * slacks->timing_criticality[inet][ipin]; 
#else
			/* Pin criticality is based on only timing criticality. */
			pin_criticality[ipin] = slacks->timing_criticality[inet][ipin];
#endif
            pin_criticality[ipin] *= 0.98;
            pin_criticality[ipin] += 0.01;
		}
	}

	num_sinks = g_clbs_nlist.net[inet].num_sinks();
	heapsort(sink_order, pin_criticality, num_sinks, 0);

	/* Update base costs according to fanout and criticality rules */

	largest_criticality = pin_criticality[sink_order[1]];
	update_rr_base_costs(inet, largest_criticality);

	mark_ends(inet); /* Only needed to check for multiply-connected SINKs */

	rt_root = init_route_tree_to_source(inet);
    
    node_expanded_per_net = 0;
    node_on_path = 0;
    estimated_cost_deviation = 0;
    estimated_cost_deviation_abs = 0;
	// explore in order of decreasing criticality
	for (itarget = 1; itarget <= num_sinks; itarget++) {
		target_pin = sink_order[itarget];
		target_node = net_rr_terminals[inet][target_pin];
        if (target_node == 52356)
            printf("INET %d\n", inet);
		target_criticality = pin_criticality[target_pin];
#if PRINTCRITICALPATH == 1
        if ((int)inet == max_inet && itarget == max_ipin)
            is_print_cpath = true;
        else
            is_print_cpath = false;
#endif
        target_chosen_chan_type = UNDEFINED;
        target_pin_x = UNDEFINED;
        target_pin_y = UNDEFINED;
        expanded_node_min_target_side.clear();

		highfanout_rlim = mark_node_expansion_by_bin(inet, target_node,
				rt_root);
		
		if (itarget > 1 && itry > 5) {
			/* Enough iterations given to determine opin, to speed up legal solution, do not let net use two opins */
			assert(rr_node[rt_root->inode].type == SOURCE);
			rt_root->re_expand = false;
		}

		// reexplore route tree from root to add any new nodes
		add_route_tree_to_heap(rt_root, target_node, target_criticality,
				astar_fac);

		// cheapest s_heap (gives index to rr_node) in current route tree to be expanded on
		struct s_heap* cheapest = get_heap_head();

		if (cheapest == NULL) { /* Infeasible routing.  No possible path for net. */
			vpr_printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
					   inet, g_clbs_nlist.net[inet].name, itarget);
			reset_path_costs();
			free_route_tree(rt_root);
			return (false);
		}

		inode = cheapest->index;
        node_expanded_per_sink = 0;

        print_start_end_to_sink(true, itry, inet, target_node, itarget);

		while (inode != target_node) {
			old_total_cost = rr_node_route_inf[inode].path_cost;
			new_total_cost = cheapest->cost;
			if (old_total_cost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
				old_back_cost = HUGE_POSITIVE_FLOAT;
			else
				old_back_cost = rr_node_route_inf[inode].backward_path_cost;

			new_back_cost = cheapest->backward_path_cost;

			/* I only re-expand a node if both the "known" backward cost is lower  *
			 * in the new expansion (this is necessary to prevent loops from       *
			 * forming in the routing and causing havoc) *and* the expected total  *
			 * cost to the sink is lower than the old value.  Different R_upstream *
			 * values could make a path with lower back_path_cost less desirable   *
			 * than one with higher cost.  Test whether or not I should disallow   *
			 * re-expansion based on a higher total cost.                          */

			if (old_total_cost > new_total_cost && old_back_cost > new_back_cost) {
				rr_node_route_inf[inode].prev_node = cheapest->u.prev_node;
				rr_node_route_inf[inode].prev_edge = cheapest->prev_edge;
				rr_node_route_inf[inode].path_cost = new_total_cost;
				rr_node_route_inf[inode].backward_path_cost = new_back_cost;
                rr_node_route_inf[inode].back_Tdel = cheapest->back_Tdel;
				if (old_total_cost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
					add_to_mod_list(&rr_node_route_inf[inode].path_cost);

				timing_driven_expand_neighbours(cheapest, inet, itry, bend_cost,
						target_criticality, target_node, astar_fac,
						highfanout_rlim, lookahead_eval, target_criticality);
			}
			free_heap_data(cheapest);
			cheapest = get_heap_head();
            if (target_chosen_chan_type == UNDEFINED) {
                int cheap_inode = cheapest->index;
                if (expanded_node_min_target_side.count(cheap_inode) > 0) {
                    target_chosen_chan_type = expanded_node_min_target_side[cheap_inode].chan_type;
                    target_pin_x = expanded_node_min_target_side[cheap_inode].x;
                    target_pin_y = expanded_node_min_target_side[cheap_inode].y;
                    expanded_node_min_target_side.clear();
                }
            }
			if (cheapest == NULL) { /* Impossible routing.  No path for net. */
				vpr_printf_info("Cannot route net #%d (%s) to sink #%d -- no possible path.\n",
						 inet, g_clbs_nlist.net[inet].name, itarget);
				reset_path_costs();
				free_route_tree(rt_root);
				return (false);
			}
			inode = cheapest->index;
		}

		/* NB:  In the code below I keep two records of the partial routing:  the   *
		 * traceback and the route_tree.  The route_tree enables fast recomputation *
		 * of the Elmore delay to each node in the partial routing.  The traceback  *
		 * lets me reuse all the routines written for breadth-first routing, which  *
		 * all take a traceback structure as input.  Before this routine exits the  *
		 * route_tree structure is destroyed; only the traceback is needed at that  *
		 * point.                                                                   */
#if DEPTHWISELOOKAHEADEVAL == 1
        subtree_size_avg[0] = node_expanded_per_sink;
#endif
		rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
		new_route_start_tptr = update_traceback(cheapest, inet);
		rt_node_of_sink[target_pin] = update_route_tree(cheapest, lookahead_eval, target_criticality);
		free_heap_data(cheapest);
		pathfinder_update_one_cost(new_route_start_tptr, 1, pres_fac);
        print_start_end_to_sink(false, itry, inet, target_node, itarget);
		empty_heap();
		reset_path_costs();
	}
 
	/* For later timing analysis. */

	update_net_delays_from_route_tree(net_delay, rt_node_of_sink, inet);
	free_route_tree(rt_root);
	return (true);
}

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
		float target_criticality, float astar_fac) {

	/* Puts the entire partial routing below and including rt_node onto the heap *
	 * (except for those parts marked as not to be expanded) by calling itself   *
	 * recursively.                                                              */
    opin_cost_by_seg_type.clear();

	int inode;
	t_rt_node *child_node;
	t_linked_rt_edge *linked_rt_edge;
	float tot_cost, backward_path_cost, R_upstream;

	/* Pre-order depth-first traversal */

	if (rt_node->re_expand) {
		inode = rt_node->inode;
		backward_path_cost = target_criticality * rt_node->Tdel;
		R_upstream = rt_node->R_upstream;
		tot_cost = backward_path_cost
				+ astar_fac
						* get_timing_driven_expected_cost(inode, target_node,
								target_criticality, R_upstream);
#if INPRECISE_GET_HEAP_HEAD == 1 || SPACEDRIVENHEAP == 1
        int target_x = rr_node[target_node].get_xlow();
        int target_y = rr_node[target_node].get_ylow();
        int offset_x = (rr_node[inode].get_xlow() + rr_node[inode].get_xhigh()) / 2;
        int offset_y = (rr_node[inode].get_ylow() + rr_node[inode].get_yhigh()) / 2;
        offset_x = abs(target_x - offset_x);
        offset_y = abs(target_y - offset_y);
        node_to_heap(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
                backward_path_cost, R_upstream, rt_node->Tdel, offset_x, offset_y);
#else
		node_to_heap(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
				backward_path_cost, R_upstream, rt_node->Tdel);
#endif
	}

	linked_rt_edge = rt_node->u.child_list;

	while (linked_rt_edge != NULL) {
		child_node = linked_rt_edge->child;
		add_route_tree_to_heap(child_node, target_node, target_criticality,
				astar_fac);
		linked_rt_edge = linked_rt_edge->next;
	}
}

static void timing_driven_expand_neighbours(struct s_heap *current, 
		int inet, int itry,
		float bend_cost, float criticality_fac, int target_node,
		float astar_fac, int highfanout_rlim, bool lookahead_eval, float target_criticality) {

	/* Puts all the rr_nodes adjacent to current on the heap.  rr_nodes outside *
	 * the expanded bounding box specified in route_bb are not added to the     *
	 * heap.                                                                    */

	int iconn, to_node, num_edges, inode, iswitch, target_x, target_y;
	t_rr_type from_type, to_type;
	float new_tot_cost, old_back_pcost, new_back_pcost, R_upstream;
	float new_R_upstream, Tdel;

	inode = current->index;
	old_back_pcost = current->backward_path_cost;
	R_upstream = current->R_upstream;
	num_edges = rr_node[inode].get_num_edges();

	target_x = rr_node[target_node].get_xhigh();
	target_y = rr_node[target_node].get_yhigh();
    if (rr_node[inode].type == SOURCE && route_start) {
        opin_cost_by_seg_type.clear();
        setup_opin_cost_by_seg_type(inode, target_node);
    }   
#if DEBUGEXPANSIONRATIO == 1
    if (itry_share == DB_ITRY && target_node == DB_TARGET_NODE) 
        printf("CRITICALITY %f\n", target_criticality);
    print_expand_neighbours(true, inode, target_node, 
        target_chosen_chan_type, target_pin_x, target_pin_y, 0., 0., 0., 0., 0.);
#endif

	for (iconn = 0; iconn < num_edges; iconn++) {
		to_node = rr_node[inode].edges[iconn];
		if (g_clbs_nlist.net[inet].num_sinks() >= HIGH_FANOUT_NET_LIM) {
			if (rr_node[to_node].get_xhigh() < target_x - highfanout_rlim
					|| rr_node[to_node].get_xlow() > target_x + highfanout_rlim
					|| rr_node[to_node].get_yhigh() < target_y - highfanout_rlim
					|| rr_node[to_node].get_ylow() > target_y + highfanout_rlim){
				continue; /* Node is outside high fanout bin. */
			}
		}
		else if (rr_node[to_node].get_xhigh() < route_bb[inet].xmin
				|| rr_node[to_node].get_xlow() > route_bb[inet].xmax
				|| rr_node[to_node].get_yhigh() < route_bb[inet].ymin
				|| rr_node[to_node].get_ylow() > route_bb[inet].ymax)
			continue; /* Node is outside (expanded) bounding box. */


		/* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
		 * the issue of how to cost them properly so they don't get expanded before *
		 * more promising routes, but makes route-throughs (via CLBs) impossible.   *
		 * Change this if you want to investigate route-throughs.                   */

		to_type = rr_node[to_node].type;
		if (to_type == IPIN
				&& (rr_node[to_node].get_xhigh() != target_x
						|| rr_node[to_node].get_yhigh() != target_y))
			continue;

		/* new_back_pcost stores the "known" part of the cost to this node -- the   *
		 * congestion cost of all the routing resources back to the existing route  *
		 * plus the known delay of the total path back to the source.  new_tot_cost *
		 * is this "known" backward cost + an expected cost to get to the target.   */
#if INPRECISE_GET_HEAP_HEAD == 1 || SPACEDRIVENHEAP == 1
        int offset_x = (rr_node[to_node].get_xlow() + rr_node[to_node].get_xhigh()) / 2;
        int offset_y = (rr_node[to_node].get_ylow() + rr_node[to_node].get_yhigh()) / 2;
        offset_x = abs(offset_x - target_x);
        offset_y = abs(offset_y - target_y);
#endif		
		iswitch = rr_node[inode].switches[iconn];
		if (g_rr_switch_inf[iswitch].buffered) {
			new_R_upstream = g_rr_switch_inf[iswitch].R;
		} else {
			new_R_upstream = R_upstream + g_rr_switch_inf[iswitch].R;
		}
        // Tdel is the delay of current node
		Tdel = rr_node[to_node].C * (new_R_upstream + 0.5 * rr_node[to_node].R);
		Tdel += g_rr_switch_inf[iswitch].Tdel;
		new_R_upstream += rr_node[to_node].R;
        new_back_pcost = old_back_pcost;
		new_back_pcost += criticality_fac * Tdel 
                        + (1. - criticality_fac) * get_rr_cong_cost(to_node);

		if (bend_cost != 0.) {
			from_type = rr_node[inode].type;
			to_type = rr_node[to_node].type;
			if ((from_type == CHANX && to_type == CHANY)
					|| (from_type == CHANY && to_type == CHANX))
				new_back_pcost += bend_cost;
		}
		float expected_cost = get_timing_driven_expected_cost(to_node, target_node,
		    		criticality_fac, new_R_upstream);
 
		new_tot_cost = new_back_pcost + astar_fac * expected_cost;
#if INPRECISE_GET_HEAP_HEAD == 1 || SPACEDRIVENHEAP == 1
 		node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
				new_R_upstream, Tdel + current->back_Tdel, offset_x, offset_y);
#else
        node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
                new_R_upstream, Tdel + current->back_Tdel);
#endif
        if (lookahead_eval) {
            if (new_tot_cost < rr_node_route_inf[to_node].path_cost) {
                node_expanded_per_net ++;
                node_expanded_per_sink ++;
            }
        }
#if DEBUGEXPANSIONRATIO == 1
        if (new_tot_cost < rr_node_route_inf[to_node].path_cost) {
            print_expand_neighbours(false, to_node, target_node, 
                target_chosen_chan_type, target_pin_x, target_pin_y, 
                expected_cost, Tdel + current->back_Tdel, new_tot_cost, 
                basecost_for_printing, get_rr_cong_cost(to_node));
        }
#endif
	} /* End for all neighbours */
}

static float get_timing_driven_expected_cost(int inode, int target_node,
		float criticality_fac, float R_upstream) {
	/* Determines the expected cost (due to both delay and resouce cost) to reach *
	 * the target node from inode.  It doesn't include the cost of inode --       *
	 * that's already in the "known" path_cost.                                   */

	t_rr_type rr_type;
	int cost_index, ortho_cost_index, num_segs_same_dir, num_segs_ortho_dir;
	float expected_cost, cong_cost, Tdel;

	rr_type = rr_node[inode].type;

	if (rr_type == CHANX || rr_type == CHANY || rr_type == OPIN) {
#ifdef INTERPOSER_BASED_ARCHITECTURE		
		int num_interposer_hops = get_num_expected_interposer_hops_to_target(inode, target_node);
#endif
        float Tdel_future = 0.;
#if LOOKAHEADBFS == 1
        if (turn_on_bfs_map_lookup(criticality_fac)) {
            // calculate Tdel, not just Tdel_future
            float C_downstream = -1.0;
            Tdel_future = get_timing_driven_future_Tdel(inode, target_node, &C_downstream, &cong_cost);
            basecost_for_printing = cong_cost;
            Tdel = Tdel_future + R_upstream * C_downstream;
            cong_cost += rr_indexed_data[IPIN_COST_INDEX].base_cost
                       + rr_indexed_data[SINK_COST_INDEX].base_cost;

        } else {
#endif
            if (rr_type == OPIN) {
                // keep the same as the original router
                return 0;
            }
            num_segs_same_dir = get_expected_segs_to_target(inode, target_node,
                    &num_segs_ortho_dir);
            cost_index = rr_node[inode].get_cost_index();
            ortho_cost_index = rr_indexed_data[cost_index].ortho_cost_index;
            cong_cost = num_segs_same_dir * rr_indexed_data[cost_index].base_cost
                      + num_segs_ortho_dir * rr_indexed_data[ortho_cost_index].base_cost;
            cong_cost += rr_indexed_data[IPIN_COST_INDEX].base_cost
                       + rr_indexed_data[SINK_COST_INDEX].base_cost;

            // future without R_upstream
            Tdel_future = num_segs_same_dir * rr_indexed_data[cost_index].T_linear
                         + num_segs_ortho_dir * rr_indexed_data[ortho_cost_index].T_linear
                         + num_segs_same_dir * num_segs_same_dir * rr_indexed_data[cost_index].T_quadratic
                         + num_segs_ortho_dir * num_segs_ortho_dir * rr_indexed_data[ortho_cost_index].T_quadratic;
            // add R_upstream factor
            Tdel = Tdel_future + R_upstream * (num_segs_same_dir * rr_indexed_data[cost_index].C_load
                                + num_segs_ortho_dir * rr_indexed_data[ortho_cost_index].C_load);
#if LOOKAHEADBFS == 1
        }
#endif
		Tdel += rr_indexed_data[IPIN_COST_INDEX].T_linear;

#ifdef INTERPOSER_BASED_ARCHITECTURE
		float interposer_hop_delay = (float)delay_increase * 1e-12;
		Tdel += num_interposer_hops * interposer_hop_delay;
#endif

		expected_cost = criticality_fac * Tdel
				+ (1. - criticality_fac) * cong_cost;
		return (expected_cost);
	}

	else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
		return (rr_indexed_data[SINK_COST_INDEX].base_cost);
	}

	else { /* Change this if you want to investigate route-throughs */
		return (0.);
	}
}


/******************************* START function definition for new look ahead ***************************************/
static bool turn_on_bfs_map_lookup(float criticality) {
    /*
     * check function to decide whether we should use the
     * new lookahead or the old one
     * crit_threshold is updated after each iteration
     */
    // Not currently add the bfs map look up in placement
    if (!route_start)
        return false;
    if (itry_share == 1)
        return true;
    if (criticality > crit_threshold) 
        return true;
    else 
        return false;
}


float get_timing_driven_future_Tdel (int inode, int target_node, 
        float *C_downstream, float *future_base_cost) {
    /*
     * the whole motivation of this version of bfs look ahead:
     *    we don't evaluate the Tdel cost from (x1, y1) to (x2, y2)
     *    instead we evaluate by channel. --> for example: 
     *    give the cost from chanx at (x1, y1) to chany at (x2, y2)
     *    prerequisite of supporting this level of precision:
     *         given a sink at (x,y), we should know if it can be
     *         reached from TOP / RIGHT / BOTTOM / LEFT side of the clb
     *    this prerequisite requires extra backward information about the sink
     *    which is store in g_pin_side, and setup when building rr_graph 
     *    (in rr_graph.c / rr_graph2.c)     
     *
     *
     */

    *future_base_cost = 0;
    *C_downstream = 0;

    if (rr_node[inode].type != CHANX
     && rr_node[inode].type != CHANY
     && rr_node[inode].type != OPIN) {
        return 0;
    }
    int x_start, y_start;
    get_unidir_seg_start(inode, &x_start, &y_start);
    // s_* stands for information related to start node (inode)
    // t_* is for target node
    int start_seg_type = rr_indexed_data[rr_node[inode].get_cost_index()].seg_index;
    int start_dir = rr_node[inode].get_direction();
    int s_len = rr_node[inode].get_len();
    int start_chan_type = rr_node[inode].type;

    if (target_chosen_chan_type == UNDEFINED && rr_node[inode].type != OPIN) {
        // if we haven't decided which side is the cheapest to reach the target node,
        // then we expand and get the estimated cost of all possible sides.
        int target_chan_type;
        int x_target, y_target;
        get_unidir_seg_end(target_node, &x_target, &y_target);
        float cur_C_downstream, cur_basecost;
        float min_Tdel = HUGE_POSITIVE_FLOAT, cur_Tdel = HUGE_POSITIVE_FLOAT;
        if ((g_pin_side[target_node] & static_cast<char>(Pin_side::TOP)) == static_cast<char>(Pin_side::TOP)) {
            target_chan_type = CHANX;
            cur_Tdel = get_precal_Tdel(inode, start_chan_type, target_chan_type, start_seg_type, start_dir, 
                        x_start, y_start, x_target, y_target, s_len, &cur_C_downstream, &cur_basecost);
            setup_min_side_reaching_target(inode, target_chan_type, x_target, y_target, &min_Tdel, cur_Tdel,
                                    C_downstream, future_base_cost, cur_C_downstream, cur_basecost);
        }
        if ((g_pin_side[target_node] & static_cast<char>(Pin_side::RIGHT)) == static_cast<char>(Pin_side::RIGHT)) {
            target_chan_type = CHANY;
            cur_Tdel = get_precal_Tdel(inode, start_chan_type, target_chan_type, start_seg_type, start_dir, 
                        x_start, y_start, x_target, y_target, s_len, &cur_C_downstream, &cur_basecost);
            setup_min_side_reaching_target(inode, target_chan_type, x_target, y_target, &min_Tdel, cur_Tdel,
                                    C_downstream, future_base_cost, cur_C_downstream, cur_basecost);
        }
        if ((g_pin_side[target_node] & static_cast<char>(Pin_side::BOTTOM)) == static_cast<char>(Pin_side::BOTTOM)) {
            target_chan_type = CHANX;
            y_target --;
            cur_Tdel = get_precal_Tdel(inode, start_chan_type, target_chan_type, start_seg_type, start_dir, 
                        x_start, y_start, x_target, y_target, s_len, &cur_C_downstream, &cur_basecost);
            setup_min_side_reaching_target(inode, target_chan_type, x_target, y_target, &min_Tdel, cur_Tdel,
                                    C_downstream, future_base_cost, cur_C_downstream, cur_basecost);
            y_target ++;
        }
        if ((g_pin_side[target_node] & static_cast<char>(Pin_side::LEFT)) == static_cast<char>(Pin_side::LEFT)) {
            target_chan_type = CHANY;
            x_target --;
            cur_Tdel = get_precal_Tdel(inode, start_chan_type, target_chan_type, start_seg_type, start_dir, 
                        x_start, y_start, x_target, y_target, s_len, &cur_C_downstream, &cur_basecost);
            setup_min_side_reaching_target(inode, target_chan_type, x_target, y_target, &min_Tdel, cur_Tdel,
                                    C_downstream, future_base_cost, cur_C_downstream, cur_basecost);
            x_target ++;
        }
        // C_downstream & basecost is set by setup_min_side_reaching_target
        assert(cur_Tdel < 0.98 * HUGE_POSITIVE_FLOAT);
        assert(min_Tdel < 0.98 * HUGE_POSITIVE_FLOAT);
        return min_Tdel;
    } else {
        int target_chan_type, x_target, y_target;
        if (target_chosen_chan_type == UNDEFINED) {
            // always do coarse estimation for OPIN
            // OPIN
            get_unidir_seg_end(target_node, &x_target, &y_target);
            target_chan_type = CHANX;
        } else {
            // let get_precal_Tdel() directly look up correct value
            // from target_chosen_chan_type, target_pin_x, target_pin_y
            target_chan_type = target_chosen_chan_type;
            x_target = target_pin_x;
            y_target = target_pin_y;
        }
        return get_precal_Tdel(inode, start_chan_type, target_chan_type, start_seg_type, start_dir, 
                    x_start, y_start, x_target, y_target, s_len, C_downstream, future_base_cost);
    }
}


static float get_precal_Tdel(int start_node, int start_chan_type, int target_chan_type, int start_seg_type, int start_dir,
                                 int x_start, int y_start, int x_target, int y_target, int len_start,
                                 float *acc_C, float *acc_basecost) {
    /*
     * lookup the precalculated Tdel from start node to target node
     * The real target node is SINK, and the target node in this function 
     * is wire that can drive the SINK. So it has chan / seg type
     * I currently adopt different policies for OPIN & wire as start node:
     *    use a coarse Tdel for OPIN, and accurate Tdel for wire
     *    TODO: may want to change OPIN lookup to be accurate as well, since
     *    I am using OPIN penalty to restrict OPIN expansion
     */
    // dealing with CHANX / CHANY / OPIN
    float Tdel = 0.0;
    int offset_x, offset_y;
    if (rr_node[start_node].type == OPIN) {
#if OPINPENALTY == 0
        opin_penalty = 1;
#endif
        float opin_del = HUGE_POSITIVE_FLOAT;
        int to_seg_type = -1;
        int num_edges = rr_node[start_node].get_num_edges();
        for (int iconn = 0; iconn < num_edges; iconn ++) {
            int to_node = rr_node[start_node].edges[iconn];
            int seg_type = rr_indexed_data[rr_node[to_node].get_cost_index()].seg_index;
            float cur_opin_del = 0.5 * rr_node[to_node].R * rr_node[to_node].C;
            if (opin_cost_by_seg_type.count(seg_type) > 0) {
                cur_opin_del += opin_cost_by_seg_type[seg_type].Tdel;
                opin_del = (opin_del > cur_opin_del) ? cur_opin_del : opin_del;
                to_seg_type = seg_type;
            }
        }
        if (opin_del > 0.98 * HUGE_POSITIVE_FLOAT) {
            offset_x = abs(x_start - x_target);
            offset_y = abs(y_start - y_target);
            offset_x = (offset_x > nx-3) ? nx-3 : offset_x;
            offset_y = (offset_y > ny-3) ? ny-3 : offset_y;
            *acc_C = bfs_lookahead_info[offset_x][offset_y][0][0].acc_C_x;
            *acc_basecost = bfs_lookahead_info[offset_x][offset_y][0][0].acc_basecost_x;
            return opin_penalty * bfs_lookahead_info[offset_x][offset_y][0][0].Tdel_x;
        } else {
            *acc_C = opin_cost_by_seg_type[to_seg_type].acc_C;
            *acc_basecost = opin_cost_by_seg_type[to_seg_type].acc_basecost;
            return opin_penalty * opin_del;
        }

    } else {
        // CHANX or CHANY
        bool is_heading_opposite;
        // first check if the direction is correct
        // if wire is heading away from target, we cannot directly lookup the cost map,
        // instead we transform this case to the normal case in adjust_coord_for_wrong_dir_wire()
        if (start_dir == INC_DIRECTION) { 
            if (start_chan_type == CHANX)
                is_heading_opposite = (x_start <= x_target) ? false : true; 
            else
                is_heading_opposite = (y_start <= y_target) ? false : true;
        } else {
            if (start_chan_type == CHANX) {
                if (target_chan_type == CHANX)
                    is_heading_opposite = (x_start >= x_target) ? false : true;
                else
                    is_heading_opposite = (x_start >= x_target + 1) ? false : true;
            } else {
                if (target_chan_type == CHANY)
                    is_heading_opposite = (y_start >= y_target) ? false : true;
                else 
                    is_heading_opposite = (y_start >= y_target + 1) ? false : true;
            }
        }
        // get Tdel: carefully calculate offset x and y
        if (is_heading_opposite) {
            // we make assumption how wires heading for wrong direction 
            // will eventually get to the right track
            adjust_coord_for_wrong_dir_wire(&start_chan_type, &start_dir, len_start, &x_start, &y_start, x_target, y_target);
            int guessed_iswitch = rr_node[start_node].switches[0];
            // XXX not updating basecost and C now
            Tdel += (0.5 * rr_node[start_node].R + g_rr_switch_inf[guessed_iswitch].R) * rr_node[start_node].C;
            Tdel += g_rr_switch_inf[guessed_iswitch].Tdel;
        }
        offset_x = abs(x_start - x_target);
        offset_y = abs(y_start - y_target);
        if (start_chan_type == CHANX && target_chan_type == CHANY) {
            offset_y += (y_start > y_target) ? 1 : 0;
            offset_x += (start_dir == DEC_DIRECTION) ? -1 : 0;
        } else if (start_chan_type == CHANY && target_chan_type == CHANX) {
            offset_x += (x_start > x_target) ? 1 : 0;
            offset_y += (start_dir == DEC_DIRECTION) ? -1 : 0;
        }
        Tdel += bfs_direct_lookup(offset_x, offset_y, start_seg_type, start_chan_type, target_chan_type, acc_C, acc_basecost);
        return Tdel;
    }
}


float bfs_direct_lookup(int offset_x, int offset_y, int seg_type, int chan_type, 
                int to_chan_type, float *acc_C, float *acc_basecost) {
    /*
     * directly lookup the global cost map
     */
    offset_x = (offset_x > (nx-3)) ? nx-3 : offset_x;
    offset_y = (offset_y > (ny-3)) ? ny-3 : offset_y;

    chan_type = (chan_type == CHANX) ? 0 : 1;
    float Tdel = 0.;
    if (to_chan_type == CHANX) {
        *acc_C = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_C_x;
        *acc_basecost = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_basecost_x;
        Tdel = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].Tdel_x;
    } else {
        *acc_C = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_C_y;
        *acc_basecost = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_basecost_y;
        Tdel = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].Tdel_y;
    }
    return Tdel;
}


static void setup_min_side_reaching_target(int inode, int target_chan_type, int target_x, int target_y, 
        float *min_Tdel, float cur_Tdel, float *C_downstream, float *future_base_cost, 
        float cur_C_downstream, float cur_basecost) {
    /*
     * setup expanded_node_min_target_side after we determine which 
     * side of target clb is the cheapest
     */
    if (cur_Tdel < *min_Tdel) {
        *min_Tdel = cur_Tdel;
        *C_downstream = cur_C_downstream;
        *future_base_cost = cur_basecost;
        expanded_node_min_target_side[inode].chan_type = target_chan_type;
        expanded_node_min_target_side[inode].x = target_x;
        expanded_node_min_target_side[inode].y = target_y;
    }
}


static void adjust_coord_for_wrong_dir_wire(int *start_chan_type, int *start_dir, int start_len, 
                                            int *x_start, int *y_start, int x_target, int y_target) {
    /*
     * for nodes heading away from target:
     *    we cannot directly lookup the entry in the global cost map.
     *    I transform the case to the normal case where wire is heading towards
     *    the target by making the following assumption:
     *        the wire heading for wrong direction will choose an othogonal wire
     *        of the same seg type at the next hop, thus switching to the correct
     *        direction. 
     *    This assumption may not be accurate in some cases (e.g.: L16 heading for
     *    wrong dir may likely take a turn using L4 rather than L16 at next hop).
     */
    int orig_dir = *start_dir;
    if (*start_chan_type == CHANX) {
        *start_dir = (*y_start <= y_target) ? INC_DIRECTION : DEC_DIRECTION;
    } else {
        *start_dir = (*x_start <= x_target) ? INC_DIRECTION : DEC_DIRECTION;
    }
    int new_x_start = *x_start;
    int new_y_start = *y_start;
    int len = start_len - 1;
    if (*start_chan_type == CHANX) {
        if (orig_dir == INC_DIRECTION) new_x_start += len;
        else new_x_start -= len;
    } else {
        if (orig_dir == INC_DIRECTION) new_y_start += len;
        else new_y_start -= len;
    }
    *x_start = new_x_start;
    *y_start = new_y_start;
    *start_chan_type = (*start_chan_type == CHANX) ? CHANY : CHANX;
}


static void setup_opin_cost_by_seg_type(int source_node, int target_node) {
    /*
     * precalculate the cost from wires that are driven by OPINs of source_node
     * to target_node. Setup the opin cost map: opin_cost_by_seg_type. 
     * Later when expanding OPINs, just lookup opin_cost_by_seg_type
     */
    assert(rr_node[source_node].type == SOURCE);
    int num_to_opin_edges = rr_node[source_node].get_num_edges();
    for (int iopin = 0; iopin < num_to_opin_edges; iopin++) {
        int opin_node = rr_node[source_node].edges[iopin];
        int num_to_wire_edges = rr_node[opin_node].get_num_edges();
        for (int iwire = 0; iwire < num_to_wire_edges; iwire ++) {
            int wire_node = rr_node[opin_node].edges[iwire];
            int seg_type = rr_indexed_data[rr_node[wire_node].get_cost_index()].seg_index;
            if (seg_type >= g_num_segment || seg_type < 0)
                continue;
            int chan_type = (rr_node[wire_node].type == CHANX) ? 0 : 1;
            // quick, coarse calculation
            int offset_x = abs(rr_node[opin_node].get_xlow() - rr_node[target_node].get_xlow());
            int offset_y = abs(rr_node[opin_node].get_ylow() - rr_node[target_node].get_ylow());
            offset_x = (offset_x > (nx-3)) ? nx-3 : offset_x;
            offset_y = (offset_y > (ny-3)) ? ny-3 : offset_y;
            opin_cost_by_seg_type[seg_type].Tdel = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].Tdel_x;
            opin_cost_by_seg_type[seg_type].acc_C = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_C_x;
            opin_cost_by_seg_type[seg_type].acc_basecost = bfs_lookahead_info[offset_x][offset_y][seg_type][chan_type].acc_basecost_x;
            if ((int)opin_cost_by_seg_type.size() == g_num_segment) {
                return;
            }
        }
    }
}

/****************************** END function definition for new look ahead *************************************/

float get_timing_driven_cong_penalty (int inode, int target_node) {
    float penalty = 1.0;
#if CONGESTIONBYCHIPAREA == 1
    int to_x_mid = (rr_node[inode].get_xhigh() + rr_node[inode].get_xlow()) / 2;
    int to_y_mid = (rr_node[inode].get_yhigh() + rr_node[inode].get_ylow()) / 2;
    int target_x_mid = (rr_node[target_node].get_xhigh() + rr_node[target_node].get_xlow()) / 2;
    int target_y_mid = (rr_node[target_node].get_yhigh() + rr_node[target_node].get_ylow()) / 2;
    //float avg_cong_on_chip = total_cong_tile_count / ((nx + 1.)*(ny+1.));
    float avg_cong_bb = 0;
    int x_start = (to_x_mid > target_x_mid) ? target_x_mid : to_x_mid;
    int x_end = (to_x_mid <= target_x_mid) ? target_x_mid : to_x_mid;
    int y_start = (to_y_mid > target_y_mid) ? target_y_mid : to_y_mid;
    int y_end = (to_y_mid <= target_y_mid) ? target_y_mid : to_y_mid;
    for (int i = x_start; i <= x_end; i++) {
        for (int j = y_start; j <= y_end; j++) {
            avg_cong_bb += congestion_map_by_area[i][j];
        }
    }
    avg_cong_bb /= ((x_end - x_start + 1) * (y_end - y_start + 1));
    //float penalty = (avg_cong_bb > avg_cong_on_chip) ? (avg_cong_bb / avg_cong_on_chip) : 1;
    float avg_by_chan_width = (200+200);
    float penalty = (avg_cong_bb > avg_by_chan_width) ? avg_cong_bb / avg_by_chan_width : 1;
    penalty = (penalty > 2) ? 2 : penalty;
    //penalty = 1 + pow(penalty - 1, 1.5+target_criticality);
    //penalty = (penalty > 1.5) ? 1.5 : penalty;
    //if (penalty > 1.2)
    //    printf("penalty %f\n", penalty);
#endif
    return penalty;
}

#ifdef INTERPOSER_BASED_ARCHITECTURE
static int get_num_expected_interposer_hops_to_target(int inode, int target_node) 
{
	/* 
	Description:
		returns the expected number of times you have to cross the 
		interposer in order to go from inode to target_node.
		This does not include inode itself.
	
	Assumptions: 
		1- Cuts are horizontal
		2- target_node is a terminal pin (hence its ylow==yhigh)
		3- wires that go through the interposer are uni-directional

		---------	y=150
		|		|
		---------	y=100
		|		|
		---------	y=50
		|		|
		---------	y=0

		num_cuts = 2, cut_step = 50, cut_locations = {50, 100}
	*/
	int y_start; /* start point y-coordinate. (y-coordinate at the *end* of wire 'i') */
	int y_end;   /* destination (target) y-coordinate */
	int cut_i, y_cut_location, num_expected_hops;
	t_rr_type rr_type;

	num_expected_hops = 0;
	y_end   = rr_node[target_node].get_ylow(); 
	rr_type = rr_node[inode].type;

	if(rr_type == CHANX) 
	{	/* inode is a horizontal wire */
		/* the ylow and yhigh are the same */
		assert(rr_node[inode].get_ylow() == rr_node[inode].get_yhigh());
		y_start = rr_node[inode].get_ylow();
	}
	else
	{	/* inode is a CHANY */
		if(rr_node[inode].direction == INC_DIRECTION)
		{
			y_start = rr_node[inode].get_yhigh();
		}
		else if(rr_node[inode].direction == DEC_DIRECTION)
		{
			y_start = rr_node[inode].get_ylow();
		}
		else
		{
			/*	this means direction is BIDIRECTIONAL! Error out. */
			y_start = -1;
			assert(rr_node[inode].direction!=BI_DIRECTION);
		}
	}

	/* for every cut, is it between 'i' and 'target'? */
	for(cut_i=0 ; cut_i<num_cuts; ++cut_i) 
	{
		y_cut_location = arch_cut_locations[cut_i];
		if( (y_start < y_cut_location &&  y_cut_location < y_end) ||
			(y_end   < y_cut_location &&  y_cut_location < y_start)) 
		{
			++num_expected_hops;
		}
	}

	/* Make there is no off-by-1 error.For current node i: 
	   if it's a vertical wire, node 'i' itself may be crossing the interposer.
	*/
	if(rr_type == CHANY)
	{	
		/* for every cut, does it cut wire 'i'? */
		for(cut_i=0 ; cut_i<num_cuts; ++cut_i) 
		{
			y_cut_location = arch_cut_locations[cut_i];
			if(rr_node[inode].get_ylow() < y_cut_location && y_cut_location < rr_node[inode].get_yhigh())
			{
				++num_expected_hops;
			}
		}
	}

	return num_expected_hops;
}
#endif

/* Macro used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */

#define ROUND_UP(x) (ceil (x - 0.001))

static int get_expected_segs_to_target(int inode, int target_node,
		int *num_segs_ortho_dir_ptr) {

	/* Returns the number of segments the same type as inode that will be needed *
	 * to reach target_node (not including inode) in each direction (the same    *
	 * direction (horizontal or vertical) as inode and the orthogonal direction).*/

	t_rr_type rr_type;
	int target_x, target_y, num_segs_same_dir, cost_index, ortho_cost_index;
	int no_need_to_pass_by_clb;
	float inv_length, ortho_inv_length, ylow, yhigh, xlow, xhigh;

	#ifdef INTERPOSER_BASED_ARCHITECTURE
	int num_expected_hops = get_num_expected_interposer_hops_to_target(inode, target_node);
	#endif
	
	target_x = rr_node[target_node].get_xlow();
	target_y = rr_node[target_node].get_ylow();
	cost_index = rr_node[inode].get_cost_index();
	inv_length = rr_indexed_data[cost_index].inv_length;
	ortho_cost_index = rr_indexed_data[cost_index].ortho_cost_index;
	ortho_inv_length = rr_indexed_data[ortho_cost_index].inv_length;
	rr_type = rr_node[inode].type;

	if (rr_type == CHANX) {
		ylow = rr_node[inode].get_ylow();
		xhigh = rr_node[inode].get_xhigh();
		xlow = rr_node[inode].get_xlow();

		/* Count vertical (orthogonal to inode) segs first. */

		if (ylow > target_y) { /* Coming from a row above target? */
			*num_segs_ortho_dir_ptr =
					(int)(ROUND_UP((ylow - target_y + 1.) * ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else if (ylow < target_y - 1) { /* Below the CLB bottom? */
			*num_segs_ortho_dir_ptr = (int)(ROUND_UP((target_y - ylow) *
					ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else { /* In a row that passes by target CLB */
			*num_segs_ortho_dir_ptr = 0;
			no_need_to_pass_by_clb = 0;
		}

#ifdef INTERPOSER_BASED_ARCHITECTURE		
		(*num_segs_ortho_dir_ptr) = (*num_segs_ortho_dir_ptr) + 2*num_expected_hops;
#endif
		
		/* Now count horizontal (same dir. as inode) segs. */
        // TODO: must take into account direction
        // TODO: if distance is less than wire length, could enforce the restriction that end point to target distance should not be larger than start point to target distance
        int x = (rr_node[inode].get_direction() == INC_DIRECTION) ? xhigh : xlow;
        x += 0;
		if (xlow > target_x + no_need_to_pass_by_clb) {
			//num_segs_same_dir = (int)(ROUND_UP((xlow - no_need_to_pass_by_clb -
			//				target_x) * inv_length));
            num_segs_same_dir = (int)(ROUND_UP((x - no_need_to_pass_by_clb - target_x) * inv_length));
		} else if (xhigh < target_x - no_need_to_pass_by_clb) {
			//num_segs_same_dir = (int)(ROUND_UP((target_x - no_need_to_pass_by_clb -
			//				xhigh) * inv_length));
            num_segs_same_dir = (int)(ROUND_UP((target_x - no_need_to_pass_by_clb - x) * inv_length));
		} else {
			num_segs_same_dir = 0;
		}
        /*
        //XXX: discourage wires going in the wrong dir
        int x_start = (rr_node[inode].get_direction() == INC_DIRECTION) ? xlow : xhigh;
        if (abs(x_start - target_x) < abs(x - target_x))
            num_segs_same_dir ++;
        */
	}

	else { /* inode is a CHANY */
		ylow = rr_node[inode].get_ylow();
		yhigh = rr_node[inode].get_yhigh();
		xlow = rr_node[inode].get_xlow();

		/* Count horizontal (orthogonal to inode) segs first. */

		if (xlow > target_x) { /* Coming from a column right of target? */
			*num_segs_ortho_dir_ptr = (int)(
					ROUND_UP((xlow - target_x + 1.) * ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else if (xlow < target_x - 1) { /* Left of and not adjacent to the CLB? */
			*num_segs_ortho_dir_ptr = (int)(ROUND_UP((target_x - xlow) *
					ortho_inv_length));
			no_need_to_pass_by_clb = 1;
		} else { /* In a column that passes by target CLB */
			*num_segs_ortho_dir_ptr = 0;
			no_need_to_pass_by_clb = 0;
		}

		/* Now count vertical (same dir. as inode) segs. */
        // TODO: changed the way num seg is calculated
        int y = (rr_node[inode].get_direction() == INC_DIRECTION) ? yhigh : ylow;
        y += 0;
		if (ylow > target_y + no_need_to_pass_by_clb) {
			//num_segs_same_dir = (int)(ROUND_UP((ylow - no_need_to_pass_by_clb -
			//				target_y) * inv_length));
            num_segs_same_dir = (int)(ROUND_UP((y - no_need_to_pass_by_clb - target_y) * inv_length));
		} else if (yhigh < target_y - no_need_to_pass_by_clb) {
			//num_segs_same_dir = (int)(ROUND_UP((target_y - no_need_to_pass_by_clb -
			//				yhigh) * inv_length));
            num_segs_same_dir = (int)(ROUND_UP((target_y - no_need_to_pass_by_clb - y) * inv_length));
		} else {
			num_segs_same_dir = 0;
		}
        /*
        //XXX: discourage wires going in the wrong dir
        int y_start = (rr_node[inode].get_direction() == INC_DIRECTION) ? ylow : yhigh;
        if (abs(y_start - target_y) < abs(y - target_y))
            num_segs_same_dir ++;
        */
#ifdef INTERPOSER_BASED_ARCHITECTURE		
		num_segs_same_dir = num_segs_same_dir + 2*num_expected_hops;
#endif
	}

	return (num_segs_same_dir);
}

static void update_rr_base_costs(int inet, float largest_criticality) {

	/* Changes the base costs of different types of rr_nodes according to the  *
	 * criticality, fanout, etc. of the current net being routed (inet).       */

	float fanout, factor;
	int index;

	fanout = g_clbs_nlist.net[inet].num_sinks();

	/* Other reasonable values for factor include fanout and 1 */
	factor = sqrt(fanout);

	for (index = CHANX_COST_INDEX_START; index < num_rr_indexed_data; index++) {
		if (rr_indexed_data[index].T_quadratic > 0.) { /* pass transistor */
			rr_indexed_data[index].base_cost =
					rr_indexed_data[index].saved_base_cost * factor;
		} else {
			rr_indexed_data[index].base_cost =
					rr_indexed_data[index].saved_base_cost;
		}
	}
}

/* Nets that have high fanout can take a very long time to route. Routing for sinks that are part of high-fanout nets should be
   done within a rectangular 'bin' centered around a target (as opposed to the entire net bounding box) the size of which is returned by this function. */
static int mark_node_expansion_by_bin(int inet, int target_node,
		t_rt_node * rt_node) {
	int target_xlow, target_ylow, target_xhigh, target_yhigh;
	int rlim = 1;
	int inode;
	float area;
	bool success;
	t_linked_rt_edge *linked_rt_edge;
	t_rt_node * child_node;

	target_xlow = rr_node[target_node].get_xlow();
	target_ylow = rr_node[target_node].get_ylow();
	target_xhigh = rr_node[target_node].get_xhigh();
	target_yhigh = rr_node[target_node].get_yhigh();

	if (g_clbs_nlist.net[inet].num_sinks() < HIGH_FANOUT_NET_LIM) {
		/* This algorithm only applies to high fanout nets */
		return 1;
	}
	if (rt_node == NULL || rt_node->u.child_list == NULL) {
		/* If unknown traceback, set radius of bin to be size of chip */
		rlim = max(nx + 2, ny + 2);
		return rlim;
	}

	area = (route_bb[inet].xmax - route_bb[inet].xmin)
			* (route_bb[inet].ymax - route_bb[inet].ymin);
	if (area <= 0) {
		area = 1;
	}

	rlim = (int)(ceil(sqrt((float) area / (float) g_clbs_nlist.net[inet].num_sinks())));

	success = false;
	/* determine quickly a feasible bin radius to route sink for high fanout nets 
	 this is necessary to prevent super long runtimes for high fanout nets; in best case, a reduction in complexity from O(N^2logN) to O(NlogN) (Swartz fast router)
	 */
	linked_rt_edge = rt_node->u.child_list;
	while (success == false && linked_rt_edge != NULL) {
		while (linked_rt_edge != NULL && success == false) {
			child_node = linked_rt_edge->child;
			inode = child_node->inode;
			if (!(rr_node[inode].type == IPIN || rr_node[inode].type == SINK)) {
				if (rr_node[inode].get_xlow() <= target_xhigh + rlim
						&& rr_node[inode].get_xhigh() >= target_xhigh - rlim
						&& rr_node[inode].get_ylow() <= target_yhigh + rlim
						&& rr_node[inode].get_yhigh() >= target_yhigh - rlim) {
					success = true;
				}
			}
			linked_rt_edge = linked_rt_edge->next;
		}

		if (success == false) {
			if (rlim > max(nx + 2, ny + 2)) {
				vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
					 "VPR internal error, net %s has paths that are not found in traceback.\n", g_clbs_nlist.net[inet].name);
			}
			/* if sink not in bin, increase bin size until fit */
			rlim *= 2;
		} else {
			/* Sometimes might just catch a wire in the end segment, need to give it some channel space to explore */
			rlim += 4;
		}
		linked_rt_edge = rt_node->u.child_list;
	}

	/* adjust rlim to account for width/height of block containing the target sink */
	int target_span = max( target_xhigh - target_xlow, target_yhigh - target_ylow );
	rlim += target_span;

	/* redetermine expansion based on rlim */
	linked_rt_edge = rt_node->u.child_list;
	while (linked_rt_edge != NULL) {
		child_node = linked_rt_edge->child;
		inode = child_node->inode;
		if (!(rr_node[inode].type == IPIN || rr_node[inode].type == SINK)) {
			if (rr_node[inode].get_xlow() <= target_xhigh + rlim
					&& rr_node[inode].get_xhigh() >= target_xhigh - rlim
					&& rr_node[inode].get_ylow() <= target_yhigh + rlim
					&& rr_node[inode].get_yhigh() >= target_yhigh - rlim) {
				child_node->re_expand = true;
			} else {
				child_node->re_expand = false;
			}
		}
		linked_rt_edge = linked_rt_edge->next;
	}
	return rlim;
}

#define ERROR_TOL 0.0001

static void timing_driven_check_net_delays(float **net_delay) {

	/* Checks that the net delays computed incrementally during timing driven    *
	 * routing match those computed from scratch by the net_delay.c module.      */

	unsigned int inet, ipin;
	float **net_delay_check;

	t_chunk list_head_net_delay_check_ch = {NULL, 0, NULL};

	/*struct s_linked_vptr *ch_list_head_net_delay_check;*/

	net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch, g_clbs_nlist.net,
		g_clbs_nlist.net.size());
	load_net_delay_from_routing(net_delay_check, g_clbs_nlist.net, g_clbs_nlist.net.size());

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {
		for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
			if (net_delay_check[inet][ipin] == 0.) { /* Should be only GLOBAL nets */
				if (fabs(net_delay[inet][ipin]) > ERROR_TOL) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in timing_driven_check_net_delays: net %d pin %d.\n"
						"\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
						inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
				}
			} else {
				if (fabs(1.0 - net_delay[inet][ipin] / net_delay_check[inet][ipin]) > ERROR_TOL) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
						"in timing_driven_check_net_delays: net %d pin %d.\n"
						"\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
						inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
				}
			}
		}
	}

	free_net_delay(net_delay_check, &list_head_net_delay_check_ch);
	vpr_printf_info("Completed net delay value cross check successfully.\n");
}


/* Detect if net should be routed or not */
bool should_route_net(int inet) {
	t_trace * tptr = trace_head[inet];

	if (tptr == NULL) {
		/* No routing yet. */
		return true;
	} 

	for (;;) {
		int inode = tptr->index;
		int occ = rr_node[inode].get_occ();
		int capacity = rr_node[inode].get_capacity();

		if (occ > capacity) {
			return true; /* overuse detected */
		}

		if (rr_node[inode].type == SINK) {
			tptr = tptr->next; /* Skip next segment. */
			if (tptr == NULL)
				break;
		}

		tptr = tptr->next;

	} /* End while loop -- did an entire traceback. */

	return false; /* Current route has no overuse */
}



