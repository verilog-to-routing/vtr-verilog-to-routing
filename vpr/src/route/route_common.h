/************ Defines and types shared by all route files ********************/
#pragma once
#include <vector>
#include "clustered_netlist.h"
#include "vtr_vector.h"
#include "heap_type.h"
#include "rr_node_fwd.h"
#include "router_stats.h"
#include "globals.h"

/******* Subroutines in route_common used only by other router modules ******/
vtr::vector<ParentNetId, t_bb> load_route_bb(const Netlist<>& net_list,
                                             int bb_factor);

t_bb load_net_route_bb(const Netlist<>& net_list,
                       ParentNetId net_id,
                       int bb_factor);

void pathfinder_update_single_node_occupancy(RRNodeId inode, int add_or_sub);

void pathfinder_update_acc_cost_and_overuse_info(float acc_fac, OveruseInfo& overuse_info);

/** Update pathfinder cost of all nodes under root (including root) */
void pathfinder_update_cost_from_route_tree(const RouteTreeNode& root, int add_or_sub);

float update_pres_fac(float new_pres_fac);

void reset_path_costs(const std::vector<RRNodeId>& visited_rr_nodes);

float get_rr_cong_cost(RRNodeId inode, float pres_fac);

/* Returns the base cost of using this rr_node */
inline float get_single_rr_cong_base_cost(RRNodeId inode) {
    auto& device_ctx = g_vpr_ctx.device();
    auto cost_index = device_ctx.rr_graph.node_cost_index(inode);

    return device_ctx.rr_indexed_data[cost_index].base_cost;
}

/* Returns the accumulated congestion cost of using this rr_node */
inline float get_single_rr_cong_acc_cost(RRNodeId inode) {
    auto& route_ctx = g_vpr_ctx.routing();

    return route_ctx.rr_node_route_inf[inode].acc_cost;
}

/* Returns the present congestion cost of using this rr_node */
inline float get_single_rr_cong_pres_cost(RRNodeId inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    int occ = route_ctx.rr_node_route_inf[inode].occ();
    int capacity = rr_graph.node_capacity(inode);

    if (occ >= capacity) {
        return (1. + pres_fac * (occ + 1 - capacity));
    } else {
        return 1.;
    }
}

/* Returns the congestion cost of using this rr_node,
 * *ignoring* non-configurable edges */
inline float get_single_rr_cong_cost(RRNodeId inode, float pres_fac) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto& route_ctx = g_vpr_ctx.routing();

    float pres_cost;
    int overuse = route_ctx.rr_node_route_inf[inode].occ() - rr_graph.node_capacity(inode);

    if (overuse >= 0) {
        pres_cost = (1. + pres_fac * (overuse + 1));
    } else {
        pres_cost = 1.;
    }

    auto cost_index = rr_graph.node_cost_index(inode);

    float cost = device_ctx.rr_indexed_data[cost_index].base_cost * route_ctx.rr_node_route_inf[inode].acc_cost * pres_cost;

    VTR_ASSERT_DEBUG_MSG(
        cost == get_single_rr_cong_base_cost(inode) * get_single_rr_cong_acc_cost(inode) * get_single_rr_cong_pres_cost(inode, pres_fac),
        "Single rr node congestion cost is inaccurate");

    return cost;
}

void mark_ends(const Netlist<>& net_list, ParentNetId net_id);

void mark_remaining_ends(ParentNetId net_id);

void add_to_mod_list(RRNodeId inode, std::vector<RRNodeId>& modified_rr_node_inf);

void init_route_structs(const Netlist<>& net_list,
                        int bb_factor,
                        bool has_choking_point,
                        bool is_flat);

void alloc_and_load_rr_node_route_structs();

void reset_rr_node_route_structs();

void reserve_locally_used_opins(HeapInterface* heap, float pres_fac, float acc_fac, bool rip_up_local_opins, bool is_flat);

void print_rr_node_route_inf();
void print_rr_node_route_inf_dot();
void print_invalid_routing_info(const Netlist<>& net_list, bool is_flat);

std::string describe_unrouteable_connection(RRNodeId source_node, RRNodeId sink_node, bool is_flat);

/* If flat_routing isn't enabled, this function would simply pass from_node and to_node to the router_lookahead.
 * However, if flat_routing is enabled, we can not do the same. For the time being, router lookahead is not aware
 * of internal nodes, thus if we pass an intra-cluster node to it, it would raise an error. Thus, we choose one
 * node on the same tile of that internal node and pass it to the router lookahead.
 */
float get_cost_from_lookahead(const RouterLookahead& router_lookahead,
                              const RRGraphView& rr_graph_view,
                              RRNodeId from_node,
                              RRNodeId to_node,
                              float R_upstream,
                              const t_conn_cost_params cost_params,
                              bool is_flat);

/* Creates a new t_heap object to be placed on the heap, if the new cost    *
 * given is lower than the current path_cost to this channel segment.  The  *
 * index of its predecessor is stored to make traceback easy.  The index of *
 * the edge used to get from its predecessor to it is also stored to make   *
 * timing analysis, etc.                                                    *
 *                                                                          *
 * Returns t_heap suitable for adding to heap or nullptr if node is more    *
 * expensive than previously explored path.                                 */
template<typename T, typename RouteInf>
t_heap* prepare_to_add_node_to_heap(
    T* heap,
    const RouteInf& rr_node_route_inf,
    RRNodeId inode,
    float total_cost,
    RRNodeId prev_node,
    RREdgeId prev_edge,
    float backward_path_cost,
    float R_upstream) {
    if (total_cost >= rr_node_route_inf[inode].path_cost)
        return nullptr;

    t_heap* hptr = heap->alloc();

    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->set_prev_node(prev_node);
    hptr->set_prev_edge(prev_edge);
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;
    return hptr;
}

/* Puts an rr_node on the heap if it is the cheapest path.    */
template<typename T, typename RouteInf>
void add_node_to_heap(
    T* heap,
    const RouteInf& rr_node_route_inf,
    RRNodeId inode,
    float total_cost,
    RRNodeId prev_node,
    RREdgeId prev_edge,
    float backward_path_cost,
    float R_upstream) {
    t_heap* hptr = prepare_to_add_node_to_heap(
        heap,
        rr_node_route_inf, inode, total_cost, prev_node,
        prev_edge, backward_path_cost, R_upstream);
    if (hptr) {
        heap->add_to_heap(hptr);
    }
}

/* Puts an rr_node on the heap with the same condition as add_node_to_heap,
 * but do not fix heap property yet as that is more efficiently done from
 * bottom up with build_heap    */
template<typename T, typename RouteInf>
void push_back_node(
    T* heap,
    const RouteInf& rr_node_route_inf,
    RRNodeId inode,
    float total_cost,
    RRNodeId prev_node,
    RREdgeId prev_edge,
    float backward_path_cost,
    float R_upstream) {
    t_heap* hptr = prepare_to_add_node_to_heap(
        heap,
        rr_node_route_inf, inode, total_cost, prev_node, prev_edge,
        backward_path_cost, R_upstream);
    if (hptr) {
        heap->push_back(hptr);
    }
}

/* Puts an rr_node on the heap with the same condition as node_to_heap,
 * but do not fix heap property yet as that is more efficiently done from
 * bottom up with build_heap. Certain information is also added     */
template<typename T>
void push_back_node_with_info(
    T* heap,
    RRNodeId inode,
    float total_cost,
    float backward_path_cost,
    float R_upstream,
    float backward_path_delay,
    PathManager* rcv_path_manager) {
    t_heap* hptr = heap->alloc();
    rcv_path_manager->alloc_path_struct(hptr->path_data);

    hptr->index = inode;
    hptr->cost = total_cost;
    hptr->backward_path_cost = backward_path_cost;
    hptr->R_upstream = R_upstream;

    hptr->path_data->backward_delay = backward_path_delay;

    heap->push_back(hptr);
}
