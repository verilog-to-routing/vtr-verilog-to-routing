#include "static_heap.h"
#include "binary_heap.h"
#include "globals.h"

static BinaryHeap heap;

t_heap* alloc_heap_data() {
    return heap.alloc();
}
void free_heap_data(t_heap* hptr) {
    heap.free(hptr);
}

void init_heap(const DeviceGrid& grid) {
    heap.init_heap(grid);
}

t_heap* get_heap_head() {
    return heap.get_heap_head();
}

void empty_heap() {
    heap.empty_heap();
}

void add_to_heap(t_heap* hptr) {
    heap.add_to_heap(hptr);
}

bool is_empty_heap() {
    return heap.is_empty_heap();
}

namespace heap_ {

bool is_valid() {
    return heap.is_valid();
}

void build_heap() {
    heap.build_heap();
}

void push_back_node(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream) {
    /* Puts an rr_node on the heap with the same condition as node_to_heap,
     * but do not fix heap property yet as that is more efficiently done from
     * bottom up with build_heap    */

    auto& route_ctx = g_vpr_ctx.routing();
    if (total_cost >= route_ctx.rr_node_route_inf[inode].path_cost)
        return;

    t_heap* hptr = alloc_heap_data();
    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->u.prev.node = prev_node;
    hptr->u.prev.edge = prev_edge;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;

    heap.push_back(hptr);
}

void add_node_to_heap(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream) {
    /* Puts an rr_node on the heap with the same condition as node_to_heap,
     * but do not fix heap property yet as that is more efficiently done from
     * bottom up with build_heap    */

    auto& route_ctx = g_vpr_ctx.routing();
    if (total_cost >= route_ctx.rr_node_route_inf[inode].path_cost)
        return;

    t_heap* hptr = alloc_heap_data();
    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->u.prev.node = prev_node;
    hptr->u.prev.edge = prev_edge;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;

    heap.add_to_heap(hptr);
}

} // namespace heap_

void node_to_heap(int inode, float total_cost, int prev_node, int prev_edge, float backward_path_cost, float R_upstream) {
    /* Puts an rr_node on the heap, if the new cost given is lower than the     *
     * current path_cost to this channel segment.  The index of its predecessor *
     * is stored to make traceback easy.  The index of the edge used to get     *
     * from its predecessor to it is also stored to make timing analysis, etc.  *
     * easy.  The backward_path_cost and R_upstream values are used only by the *
     * timing-driven router -- the breadth-first router ignores them.           */

    auto& route_ctx = g_vpr_ctx.routing();
    if (total_cost >= route_ctx.rr_node_route_inf[inode].path_cost)
        return;

    t_heap* hptr = alloc_heap_data();
    hptr->index = inode;
    hptr->cost = total_cost;
    VTR_ASSERT_SAFE(hptr->u.prev.node == NO_PREVIOUS);
    VTR_ASSERT_SAFE(hptr->u.prev.edge == NO_PREVIOUS);
    hptr->u.prev.node = prev_node;
    hptr->u.prev.edge = prev_edge;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;

    heap.add_to_heap(hptr);
}

void invalidate_heap_entries(int sink_node, int ipin_node) {
    heap.invalidate_heap_entries(sink_node, ipin_node);
}

void heap_free_all_memory() {
    heap.free_all_memory();
}
