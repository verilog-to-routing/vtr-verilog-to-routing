/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"
#include "vtr_vector.h"


struct t_heap_prev {
    t_heap_prev(int to, int from, short edge)
        : to_node(to), from_node(from), from_edge(edge) {}
    int to_node = NO_PREVIOUS; //The target node
    int from_node = NO_PREVIOUS; //The previous node used to connect to 'to_node'
    short from_edge = NO_PREVIOUS; //The edge used to connect from 'from_node' to 'to_node'
};


/* Used by the heap as its fundamental data structure.
 * Each heap element represents a partial route.
 *
 * next:    pointer to the next s_heap structure in the free
 *          linked list.  Not used when on the heap.
 *
 * cost:    The cost used to sort heap.
 *          For the timing-driven router this is the backward_path_cost +
 *          expected cost to the target.
 *          For the breadth-first router it is the node cost to reach this
 *          point.
 * backward_path_cost:  Used only by the timing-driven router.  The "known"
 *                      cost of the path up to and including this node.
 *                      In this case, the .cost member contains not only
 *                      the known backward cost but also an expected cost
 *                      to the target.
 * R_upstream: Used only by the timing-driven router.  Stores the upstream 
 *             resistance to ground from this node, including the
 *             resistance of the node itself (device_ctx.rr_nodes[index].R).
 *
 * index: The RR node index associated with the costs/R_upstream values
 * nodes: The set of nodes represented by this heap element.
 *        Usually this contains a single element corresponding to index.
 *        However in some cases (e.g. non-configurable edges) multiple RR 
 *        nodes may be required to be used together. In such cases each
 *        node will be inclued in 'nodes' and 'cost', 'backward_path_cost',
 *        R_upstream will be those associated with the lowest cost node (who's
 *        index is stored in 'index' for reference).
 */
struct t_heap {
    t_heap *next = nullptr;

	float cost = 0.;
	float backward_path_cost = 0.;
	float R_upstream = 0.;

	int index = OPEN;
    int prev_node = NO_PREVIOUS;
    int prev_edge = NO_PREVIOUS;
};

/******* Subroutines in route_common used only by other router modules ******/

vtr::vector<ClusterNetId, t_bb> load_route_bb(int bb_factor);

t_bb load_net_route_bb(ClusterNetId net_id, int bb_factor);

void pathfinder_update_path_cost(t_trace *route_segment_start,
		int add_or_sub, float pres_fac);
void pathfinder_update_single_node_cost(int inode, int add_or_sub, float pres_fac);

void pathfinder_update_cost(float pres_fac, float acc_fac);

t_trace *update_traceback(t_heap *hptr, ClusterNetId net_id);

void reset_path_costs(const std::vector<int>& visited_rr_nodes);

float get_rr_cong_cost(int inode);

void mark_ends(ClusterNetId net_id);
void mark_remaining_ends(const std::vector<int>& remaining_sinks);

void add_to_heap(t_heap *hptr);
t_heap *alloc_heap_data();
void node_to_heap(int inode, float cost, int prev_node, int prev_edge,
		float backward_path_cost, float R_upstream);

bool is_empty_heap();

void free_traceback(ClusterNetId net_id);
void free_traceback(t_trace* tptr);

void add_to_mod_list(int inode, std::vector<int>& modified_rr_node_inf);

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

