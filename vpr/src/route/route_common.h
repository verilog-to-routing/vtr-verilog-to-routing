#pragma once

/** @file Misc. router utils: some used by the connection router, some by other
 * router files and some used globally. */

#include <vector>
#include "router_stats.h"
#include "globals.h"
#include "rr_graph_fwd.h"
#include "rr_graph_view.h"
#include "rr_node_cost.h"
#include "vtr_assert.h"

/** This routine checks to see if this is a resource-feasible routing.
 * That is, are all rr_node capacity limitations respected?  It assumes
 * that the occupancy arrays are up to date when it is called. */
bool feasible_routing();

/** Is \p inode inside this bounding box?
 * In the context of the parallel router, an inode is inside a bounding box
 * if its reference point is inside it, which is the drive point for a CHAN and
 * (xlow, ylow, z) of anything else.
 * \p rr_graph is either an RRGraphView or a t_rr_graph_view. */
template<typename RRGraphType>
inline bool inside_bb(const RRGraphType& rr_graph, RRNodeId inode, const t_bb& bb) {
    int x, y;
    Direction dir = rr_graph.node_direction(inode);
    if (dir == Direction::DEC) {
        x = rr_graph.node_xhigh(inode);
        y = rr_graph.node_yhigh(inode);
    } else {
        x = rr_graph.node_xlow(inode);
        y = rr_graph.node_ylow(inode);
    }

    if (x < bb.xmin || x > bb.xmax || y < bb.ymin || y > bb.ymax) {
        return false;
    }

    // A box that spans every layer of the device contains the node's layer,
    // so the layer array does not need to be read. This always holds on a single-layer device.
    int num_layers = g_vpr_ctx.device().grid.get_num_layers();
    if (bb.layer_min <= 0 && bb.layer_max >= num_layers - 1) {
        return true;
    }

    int z = (dir == Direction::DEC) ? rr_graph.node_layer_high(inode) : rr_graph.node_layer_low(inode);
    return z >= bb.layer_min && z <= bb.layer_max;
}

/// @brief Same as inside_bb(rr_graph, inode, bb), reading the RR graph from the device context.
inline bool inside_bb(RRNodeId inode, const t_bb& bb) {
    return inside_bb(g_vpr_ctx.device().rr_graph, inode, bb);
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

void add_to_mod_list(RRNodeId inode, std::vector<RRNodeId>& modified_rr_node_inf);

void init_route_structs(const Netlist<>& net_list,
                        int bb_factor,
                        bool has_choking_point,
                        bool is_flat);

void alloc_and_load_rr_node_route_structs(const t_router_opts& router_opts);

void reset_rr_node_route_structs(const t_router_opts& route_opts);

void reserve_locally_used_opins(HeapInterface* heap, float pres_fac, float acc_fac, bool rip_up_local_opins, bool is_flat);

void print_rr_node_route_inf();
void print_rr_node_route_inf_dot();
void print_invalid_routing_info(const Netlist<>& net_list, bool is_flat);

std::string describe_unrouteable_connection(RRNodeId source_node, RRNodeId sink_node, bool is_flat);
