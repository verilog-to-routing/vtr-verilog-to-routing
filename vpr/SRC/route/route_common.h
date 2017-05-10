/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>

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
 *             resistance of the node itself (device_ctx.rr_nodes[index].R).*/
struct t_heap {
	union {
		int prev_node;
		t_heap *next;
	} u;
	float cost;
	float backward_path_cost;
	float R_upstream;
	int index;
	int prev_edge;
};


/******* Subroutines in route_common used only by other router modules ******/

void pathfinder_update_path_cost(t_trace *route_segment_start,
		int add_or_sub, float pres_fac);
void pathfinder_update_single_node_cost(int inode, int add_or_sub, float pres_fac);

void pathfinder_update_cost(float pres_fac, float acc_fac);

t_trace *update_traceback(t_heap *hptr, int inet);

void reset_path_costs(void);

float get_rr_cong_cost(int inode);

void mark_ends(int inet);
void mark_remaining_ends(const std::vector<int>& remaining_sinks);

void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream);

bool is_empty_heap(void);

void free_traceback(int inet);

void add_to_mod_list(float *fptr);

namespace heap_ {
	void build_heap();
	void sift_down(size_t hole);
	void sift_up(size_t tail, t_heap* const hptr);
	void push_back(t_heap* const hptr);
	void push_back_node(int inode, float total_cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream);
	bool is_valid();
	void pop_heap();
	void print_heap();
	void verify_extract_top();
}

t_heap *get_heap_head(void);

void empty_heap(void);

void free_heap_data(t_heap *hptr);

void invalidate_heap_entries(int sink_node, int ipin_node);

void init_route_structs(int bb_factor);

void free_rr_node_route_structs(void);

void alloc_and_load_rr_node_route_structs(void);

void reset_rr_node_route_structs(void);

void alloc_route_static_structs(void);


void free_trace_structs(void);

void reserve_locally_used_opins(float pres_fac, float acc_fac, bool rip_up_local_opins,
		vtr::t_ivec ** clb_opins_used_locally);

void free_chunk_memory_trace(void);

void print_traceback(int inet);

t_trace* alloc_trace_data(void);
void free_trace_data(t_trace* trace);
