/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"


struct t_heap_prev {
    t_heap_prev(int to, int from, short edge)
        : to_node(to), from_node(from), from_edge(edge) {}
    int to_node = NO_PREVIOUS; //The target node
    int from_node = NO_PREVIOUS; //The previous node used to connect to 'to_node'
    short from_edge = NO_PREVIOUS; //The edge used to connect from 'from_node' to 'to_node'
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
 *             resistance of the node itself (device_ctx.rr_nodes[index].R).*/
struct t_heap {
    t_heap *next = nullptr;

	float cost = 0.;
	float backward_path_cost = 0.;
	float R_upstream = 0.;
	int index = OPEN;

    std::vector<t_heap_prev> previous;
};

/******* Subroutines in route_common used only by other router modules ******/

vtr::vector_map<ClusterNetId, t_bb> load_route_bb(int bb_factor);

t_bb load_net_route_bb(ClusterNetId net_id, int bb_factor);

void pathfinder_update_path_cost(t_trace *route_segment_start,
		int add_or_sub, float pres_fac);
void pathfinder_update_single_node_cost(int inode, int add_or_sub, float pres_fac);

void pathfinder_update_cost(float pres_fac, float acc_fac);

t_trace *update_traceback(t_heap *hptr, ClusterNetId net_id);

void reset_path_costs(const std::vector<int>& visited_rr_nodes);
void reset_path_costs();

float get_rr_cong_cost(int inode);

void mark_ends(ClusterNetId net_id);
void mark_remaining_ends(const std::vector<int>& remaining_sinks);

void add_to_heap(t_heap *hptr);
t_heap *alloc_heap_data();
void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream);

bool is_empty_heap();

void free_traceback(ClusterNetId net_id);

void add_to_mod_list(int inode, std::vector<int>& modified_rr_node_inf);
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

t_heap *get_heap_head();

void empty_heap();

void free_heap_data(t_heap *hptr);

void invalidate_heap_entries(int sink_node, int ipin_node);

void init_route_structs(int bb_factor);

void alloc_and_load_rr_node_route_structs();

void reset_rr_node_route_structs();

void free_trace_structs();

void init_heap(const DeviceGrid& grid);
void reserve_locally_used_opins(float pres_fac, float acc_fac, bool rip_up_local_opins);

void free_chunk_memory_trace();

bool validate_traceback(t_trace* trace);
void print_traceback(ClusterNetId net_id);
void print_traceback(const t_trace* trace);

void print_rr_node_route_inf();
void print_rr_node_route_inf_dot();
void print_invalid_routing_info();

t_trace* alloc_trace_data();
void free_trace_data(t_trace* trace);

