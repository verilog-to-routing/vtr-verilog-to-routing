#pragma once
#include <vector>
/************** Types and defines exported by route_tree_timing.c ************/

struct s_linked_rt_edge {
	struct s_rt_node *child;
	short iswitch;
	struct s_linked_rt_edge *next;
};

typedef struct s_linked_rt_edge t_linked_rt_edge;

/* Linked list listing the children of an rt_node.                           *
 * child:  Pointer to an rt_node (child of the current node).                *
 * iswitch:  Index of the switch type used to connect to the child node.     *
 * next:   Pointer to the next linked_rt_edge in the linked list (allows     *
 *         you to get the next child of the current rt_node).                */

struct s_rt_node {
	union {
		t_linked_rt_edge *child_list;
		struct s_rt_node *next;
	} u;
	struct s_rt_node *parent_node;
	short parent_switch;
	short re_expand;
	int inode;
	float C_downstream;
	float R_upstream;
	float Tdel;
};

typedef struct s_rt_node t_rt_node;

/* Structure describing one node in a routing tree (used to get net delays   *
 * incrementally during routing, as pieces are being added).                 *
 * u.child_list:  Pointer to a linked list of linked_rt_edge.  Each one of   *
 *                the linked list entries gives a child of this node.        *
 * u.next:  Used only when this node is on the free list.  Gives the next    *
 *          node on the free list.                                           *
 * parent_node:  Pointer to the rt_node that is this node's parent (used to  *
 *               make bottom to top traversals).                             *
 * re_expand:  (really bool).  Should this node be put on the heap as     *
 *             part of the partial routing to act as a source for subsequent *
 *             connections?  true->yes, false-> no.                          *
 * parent_switch:  Index of the switch type driving this node (by its        *
 *                 parent).                                                  *
 * inode:  index (ID) of the rr_node that corresponds to this rt_node.       *
 * C_downstream:  Total downstream capacitance from this rt_node.  That is,  *
 *                the total C of the subtree rooted at the current node,     *
 *                including the C of the current node.                       *
 * R_upstream:  Total upstream resistance from this rt_node to the net       *
 *              source, including any rr_node[].R of this node.              *
 * Tdel:  Time delay for the signal to get from the net source to this node. *
 *        Includes the time to go through this node.                         */

/**************** Subroutines exported by route_tree_timing.c ***************/

void alloc_route_tree_timing_structs(void);

void free_route_tree_timing_structs(void);

t_rt_node *init_route_tree_to_source(int inet);

void free_route_tree(t_rt_node * rt_node);

t_rt_node *update_route_tree(struct s_heap *hptr);

void update_net_delays_from_route_tree(float *net_delay,
		const t_rt_node* const * rt_node_of_sink, int inet);
void update_remaining_net_delays_from_route_tree(float* net_delay, 
		const t_rt_node* const * rt_node_of_sink, const std::vector<int>& remaining_sinks);

void load_route_tree_Tdel(t_rt_node* rt_root, float Tarrival);
void load_route_tree_rr_route_inf(t_rt_node* root);	// NO_PREVIOUS for SOURCE


/********** incremental connection-based rerouting ***********/
// logically expensive asserts
#ifdef DEBUG_INCREMENTAL_REROUTING
#define INCREMENTAL_REROUTING_ASSERT(expr) assert(expr)
#else
#define INCREMENTAL_REROUTING_ASSERT(expr) ;
#endif

void print_edge(const t_linked_rt_edge* edge);
void print_route_tree(const t_rt_node* rt_root);
void print_route_tree_inf(const t_rt_node* rt_root);
void print_route_tree_congestion(const t_rt_node* rt_root);

t_rt_node* traceback_to_route_tree(int inet);
t_trace* traceback_from_route_tree(int inet, const t_rt_node* root, int num_remaining_sinks);

bool prune_route_tree(t_rt_node* rt_root, float pres_fac, std::vector<t_rt_node*>& reached_sinks, vector<int>& remaining_targets); 	// 0 R_upstream for SOURCE

void pathfinder_update_cost_from_route_tree(const t_rt_node* rt_root, int add_or_sub, float pres_fac);

bool is_equivalent_route_tree(const t_rt_node* rt_root, const t_rt_node* cloned_rt_root);
bool is_valid_skeleton_tree(const t_rt_node* rt_root);
bool is_valid_route_tree(const t_rt_node* rt_root);
bool is_uncongested_route_tree(const t_rt_node* root);