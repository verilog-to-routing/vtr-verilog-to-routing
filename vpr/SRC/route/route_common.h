/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include <map>

#define DEBUGEXPANSIONRATIO 0
#define DEPTHWISELOOKAHEADEVAL 1
#define NETWISELOOKAHEADEVAL 0
#define LOOKAHEADBYHISTORY 0
// turn this on together with the LOOKAHEADCONGMAP in expand_neighbours
#define CONGESTIONBYCHIPAREA 0
#define PRINTCRITICALPATH 0

#define ONEMORELOOKAHEAD 0
#define LOOKAHEADBFS 1
#define LOOKAHEADCONGPENALTY 0
#define OPINPENALTY 1
// INPRECISE_GET_HEAP_HEAD & SPACEDRIVENHEAP should NOT be both 1
#define INPRECISE_GET_HEAP_HEAD 0
#define SPACEDRIVENHEAP 0

#define OPIN_INIT_PENALTY 2.2
#define OPIN_DECAY_RATE 0.9
#define ALLOWED_HEAP_ERR 0.02  /* for the new get heap head method (take into account (x,y)) */
#define DB_ITRY 5
#define DB_TARGET_NODE 102804
using namespace std;
#if SPACEDRIVENHEAP == 1
extern float heap_min_cost;
#endif
struct s_heap {
	int index;
	union {
		int prev_node;
		struct s_heap *next;
	} u;
	int prev_edge;
#if INPRECISE_GET_HEAP_HEAD == 1 || SPACEDRIVENHEAP == 1
    // XXX: experiment modified get_heap_head method
    short offset_x;
    short offset_y;
#endif
	float cost;
	float backward_path_cost;
    // XXX: added by router experiment
    // the backward Tdel including this node
    float back_Tdel;
	float R_upstream;
};

/* Used by the heap as its fundamental data structure.                      * 
 * index:   Index (ID) of this routing resource node.                       * 
 * cost:    Cost up to and including this node.                             * 
 * u.prev_node:  Index (ID) of the predecessor to this node for             * 
 *          use in traceback.  NO_PREVIOUS if none.                         * 
 * u.next:  pointer to the next s_heap structure in the free                * 
 *          linked list.  Not used when on the heap.                        * 
 * prev_edge:  Index of the edge (between 0 and num_edges-1) used to        *
 *             connect the previous node to this one.  NO_PREVIOUS if       *
 *             there is no previous node.                                   *
 * backward_path_cost:  Used only by the timing-driven router.  The "known" *
 *                      cost of the path up to and including this node.     *
 *                      In this case, the .cost member contains not only    *
 *                      the known backward cost but also an expected cost   *
 *                      to the target.                                      *
 * R_upstream: Used only by the timing-driven router.  Stores the upstream  *
 *             resistance to ground from this node, including the           *
 *             resistance of the node itself (rr_node[index].R).            */

typedef struct {
	int prev_node;
	float pres_cost;
	float acc_cost;
	float path_cost;
	float backward_path_cost;
    //XXX: added to make use of look ahead history
    // the backward Tdel including this node
    float back_Tdel;
	short prev_edge;
	short target_flag;
} t_rr_node_route_inf;

/* Extra information about each rr_node needed only during routing (i.e.    *
 * during the maze expansion).                                              *
 *                                                                          *
 * prev_node:  Index of the previous node used to reach this one;           *
 *             used to generate the traceback.  If there is no              *
 *             predecessor, prev_node = NO_PREVIOUS.                        *
 * pres_cost:  Present congestion cost term for this node.                  *
 * acc_cost:   Accumulated cost term from previous Pathfinder iterations.   *
 * path_cost:  Total cost of the path up to and including this node +       *
 *             the expected cost to the target if the timing_driven router  *
 *             is being used.                                               *
 * backward_path_cost:  Total cost of the path up to and including this     *
 *                      node.  Not used by breadth-first router.            *
 * prev_edge:  Index of the edge (from 0 to num_edges-1) that was used      *
 *             to reach this node from the previous node.  If there is      *
 *             no predecessor, prev_edge = NO_PREVIOUS.                     *
 * target_flag:  Is this node a target (sink) for the current routing?      *
 *               Number of times this node must be reached to fully route.  */

/**************** Variables shared by all route_files ***********************/
extern int itry_share;
extern float opin_penalty;
extern float future_cong_penalty;
extern int node_on_path;
extern int node_expanded_per_net;
extern int node_expanded_per_sink;
extern float estimated_cost_deviation;
extern float estimated_cost_deviation_abs;
/*
 * subtree: the subtree built when routing a SOURCE to a SINK, consisting of nodes expanded
 * map key: depth of the subtree
 * map value:
 *     subtree_count: total number of subtrees of this depth
 *     subtree_size_avg: average number of nodes on the subtree of that depth
           subtree_size_avg[0] is storing the number of expanded nodes of current routing
 *     subtree_est_dev_avg: average deviation of estimated lookahead cost and actual future path cost of this depth subtree
 *     subtree_est_dev_abs_avg: the absolute deviation
 *     
 */ 
extern int total_nodes_expanded;
extern int nodes_expanded_cur_itr;
extern int nodes_expanded_pre_itr;
extern int nodes_expanded_1st_itr;
extern int nodes_expanded_max_itr;
extern float cong_cur_itr;
extern float cong_pre_itr;
#if DEPTHWISELOOKAHEADEVAL == 1
extern map<int, int> subtree_count;
extern map<int, float> subtree_size_avg;
extern map<int, float> subtree_est_dev_avg;
extern map<int, float> subtree_est_dev_abs_avg;
#endif
#if LOOKAHEADBYHISTORY == 1
/*
 * the below 4 2D arrays are indexed by relative position between
 * 2 nodes on the chip. They are gradually setup in the process of 
 * routing --> serve as cost history to facilitate look ahead estimation.
 * max_cost_coord & min_cost_coord:
 *     storing the relative position of BB on chip
 *     purpose is to verify that max cost happens at the center of chip
 *     pair.first = BB.xlow / (nx - BB.x_span)
 *     pair.second = BB.ylow / (ny - BB.y_span)
 */
extern float **max_cost_by_relative_pos;
extern float **min_cost_by_relative_pos;
extern float **avg_cost_by_relative_pos;
extern float **dev_cost_by_relative_pos;
extern int **count_by_relative_pos;
extern pair<float, float> **max_cost_coord;
extern pair<float, float> **min_cost_coord;
#endif
#define CONG_MAP_BASE_COST 5
#if CONGESTIONBYCHIPAREA == 1
extern int **congestion_map_by_area;
// congestion_map_relative: stores the relative change in consecutive iterations
extern int **congestion_map_relative;
extern int total_cong_tile_count;
#endif
#if PRINTCRITICALPATH == 1
extern bool is_print_cpath;
#endif
extern t_rr_node_route_inf *rr_node_route_inf; /* [0..num_rr_nodes-1] */
extern struct s_bb *route_bb; /* [0..num_nets-1]     */

/******* Subroutines in route_common used only by other router modules ******/

void pathfinder_update_one_cost(struct s_trace *route_segment_start,
		int add_or_sub, float pres_fac);

void pathfinder_update_cost(float pres_fac, float acc_fac);

struct s_trace *update_traceback(struct s_heap *hptr, int inet);

void reset_path_costs(void);

float get_rr_cong_cost(int inode);

void mark_ends(int inet);
#if INPRECISE_GET_HEAP_HEAD == 1 || SPACEDRIVENHEAP == 1
void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream, float back_Tdel, int offset_x, int offset_y);
#else
void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream, float back_Tdel);
#endif
bool is_empty_heap(void);

void free_traceback(int inet);

void add_to_mod_list(float *fptr);

struct s_heap *get_heap_head(void);

void empty_heap(void);

void free_heap_data(struct s_heap *hptr);

void invalidate_heap_entries(int sink_node, int ipin_node);

void init_route_structs(int bb_factor);

void free_rr_node_route_structs(void);

void alloc_and_load_rr_node_route_structs(void);

void reset_rr_node_route_structs(void);

void alloc_route_static_structs(void);

void free_trace_structs(void);

void reserve_locally_used_opins(float pres_fac, float acc_fac, bool rip_up_local_opins,
		t_ivec ** clb_opins_used_locally);

void free_chunk_memory_trace(void);

int predict_success_route_iter(const std::vector<double>& historical_overuse_ratio, const t_router_opts& router_opts);
