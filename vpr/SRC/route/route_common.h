/************ Defines and types shared by all route files ********************/

struct s_heap {
	int index;
	float cost;
	union {
		int prev_node;
		struct s_heap *next;
	} u;
	int prev_edge;
	float backward_path_cost;
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

void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream);

boolean is_empty_heap(void);

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

void reserve_locally_used_opins(float pres_fac, boolean rip_up_local_opins,
		t_ivec ** clb_opins_used_locally);

void free_chunk_memory_trace(void);

//===========================================================================//
#include "TCH_PreRoutedHandler.h"

bool validate_prerouted_nets(void);
bool restrict_prerouted_path(
		int net_index, int try_count,
		int src_node, int sink_node, 
		int from_node, int to_node);
//===========================================================================//
