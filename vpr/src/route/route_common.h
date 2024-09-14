#pragma once

/** @file Misc. router utils: some used by the connection router, some by other
 * router files and some used globally. */

#include <vector>
#include "clustered_netlist.h"
#include "rr_node_fwd.h"
#include "router_stats.h"
#include "globals.h"

/** This routine checks to see if this is a resource-feasible routing.
 * That is, are all rr_node capacity limitations respected?  It assumes
 * that the occupancy arrays are up to date when it is called. */
bool feasible_routing();

/** Is \p inode inside this bounding box?
 * In the context of the parallel router, an inode is inside a bounding box
 * if its reference point is inside it, which is the drive point for a CHAN and
 * (xlow, ylow, z) of anything else */
inline bool inside_bb(RRNodeId inode, const t_bb& bb) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int x, y, z;
    Direction dir = rr_graph.node_direction(inode);
    if (dir == Direction::DEC) {
        x = rr_graph.node_xhigh(inode);
        y = rr_graph.node_yhigh(inode);
        z = rr_graph.node_layer(inode);
    } else {
        x = rr_graph.node_xlow(inode);
        y = rr_graph.node_ylow(inode);
        z = rr_graph.node_layer(inode);
    }

    return x >= bb.xmin && x <= bb.xmax && y >= bb.ymin && y <= bb.ymax && z >= bb.layer_min && z <= bb.layer_max;
}

vtr::vector<ParentNetId, t_bb> load_route_bb(const Netlist<>& net_list,
                                             int bb_factor);

t_bb load_net_route_bb(const Netlist<>& net_list,
                       ParentNetId net_id,
                       int bb_factor);

void pathfinder_update_single_node_occupancy(RRNodeId inode, int add_or_sub);

void pathfinder_update_acc_cost_and_overuse_info(float acc_fac, OveruseInfo& overuse_info);

/** Update pathfinder cost of all nodes under root (including root) */
void pathfinder_update_cost_from_route_tree(const RouteTreeNode& root, int add_or_sub);

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
