#include "bus_mux_routing.h"

#include <optional>

#include "globals.h"
#include "route_common.h"
#include "route_tree.h"
#include "rr_graph_intra_cluster.h"
#include "vtr_assert.h"

void reset_bus_mux_route_inf(const vtr::vector<BusMuxId, t_rr_bus_mux>& rr_bus_muxes,
                             vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf) {
    bus_mux_route_inf.assign(rr_bus_muxes.size(), t_bus_mux_route_inf());
    for (BusMuxId mux_id : rr_bus_muxes.keys()) {
        bus_mux_route_inf[mux_id].set_occ.assign(rr_bus_muxes[mux_id].num_sets, 0);
    }
}

float get_bus_mux_cong_cost(RRNodeId from_node, RRNodeId to_node, float pres_fac) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    std::optional<t_bus_mux_edge> bus_mux_edge = find_bus_mux_edge(from_node, to_node);
    if (!bus_mux_edge) {
        return 0.f;
    }

    const t_bus_mux_route_inf& mux_inf = route_ctx.bus_mux_route_inf[bus_mux_edge->mux_id];

    // Each routed bit that would need to switch to this edge's input set adds one unit
    // of overuse to to_node's present cost.
    int displaced_bits = mux_inf.bits_on_other_sets(bus_mux_edge->set);
    if (displaced_bits == 0) {
        return 0.f;
    }
    return get_single_rr_cong_base_cost(to_node) * get_single_rr_cong_acc_cost(to_node) * pres_fac * displaced_bits;
}

void pathfinder_update_bus_mux_occupancy(vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf,
                                         const RouteTreeNode& rt_node,
                                         int add_or_sub) {
    // The SOURCE at the root of a route tree has no incoming edge.
    if (!rt_node.parent()) {
        return;
    }
    std::optional<t_bus_mux_edge> bus_mux_edge = find_bus_mux_edge(rt_node.parent()->inode, rt_node.inode);
    if (!bus_mux_edge) {
        return;
    }

    int& occ = bus_mux_route_inf[bus_mux_edge->mux_id].set_occ[bus_mux_edge->set];
    occ += add_or_sub;
    VTR_ASSERT(occ >= 0);
}

bool is_bus_mux_edge_control_congested(const RouteTreeNode& rt_node) {
    const RoutingContext& route_ctx = g_vpr_ctx.routing();

    // The SOURCE at the root of a route tree has no incoming edge.
    if (!rt_node.parent()) {
        return false;
    }
    std::optional<t_bus_mux_edge> bus_mux_edge = find_bus_mux_edge(rt_node.parent()->inode, rt_node.inode);
    if (!bus_mux_edge) {
        return false;
    }
    return route_ctx.bus_mux_route_inf[bus_mux_edge->mux_id].is_control_congested();
}

size_t count_control_congested_bus_muxes(const vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf) {
    size_t num_congested = 0;
    for (const t_bus_mux_route_inf& mux_inf : bus_mux_route_inf) {
        if (mux_inf.is_control_congested()) {
            num_congested++;
        }
    }
    return num_congested;
}
