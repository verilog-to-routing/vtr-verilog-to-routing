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
        bus_mux_route_inf[mux_id].init(rr_bus_muxes[mux_id].num_sets, rr_bus_muxes[mux_id].num_bits);
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
    // Scale the control congestion cost by this edge's history cost.
    float edge_acc_cost = mux_inf.edge_acc_cost[mux_inf.edge_index(bus_mux_edge->bit, bus_mux_edge->set)];
    return get_single_rr_cong_base_cost(to_node) * get_single_rr_cong_acc_cost(to_node) * pres_fac * displaced_bits * edge_acc_cost;
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

    t_bus_mux_route_inf& mux_inf = bus_mux_route_inf[bus_mux_edge->mux_id];
    int& occ = mux_inf.set_occ[bus_mux_edge->set];
    occ += add_or_sub;
    VTR_ASSERT(occ >= 0);

    int& edge_occ = mux_inf.edge_occ[mux_inf.edge_index(bus_mux_edge->bit, bus_mux_edge->set)];
    edge_occ += add_or_sub;
    VTR_ASSERT(edge_occ >= 0);
}

void pathfinder_update_bus_mux_acc_cost(vtr::vector<BusMuxId, t_bus_mux_route_inf>& bus_mux_route_inf,
                                        float acc_fac) {
    // Fraction of history cost above 1 retained when at most one input set is in use.
    constexpr float AGREED_MUX_ACC_COST_DECAY = 0.5f;

    for (t_bus_mux_route_inf& mux_inf : bus_mux_route_inf) {
        if (mux_inf.is_control_congested()) {
            for (size_t iedge = 0; iedge < mux_inf.edge_occ.size(); iedge++) {
                if (mux_inf.edge_occ[iedge] > 0) {
                    mux_inf.edge_acc_cost[iedge] += acc_fac;
                }
            }
        } else {
            for (float& edge_acc_cost : mux_inf.edge_acc_cost) {
                edge_acc_cost = 1.f + (edge_acc_cost - 1.f) * AGREED_MUX_ACC_COST_DECAY;
            }
        }
    }
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
