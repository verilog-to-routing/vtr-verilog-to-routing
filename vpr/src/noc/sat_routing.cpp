
#include "sat_routing.h"
#include "turn_model_routing.h"
#include "move_utils.h"

#include "globals.h"

#include <unordered_map>


#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"

static constexpr int NOC_LINK_BANDWIDTH_RESOLUTION = 1024;

std::vector<operations_research::sat::BoolVar> get_flow_link_vars(const std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& map,
                                                                  const std::vector<NocTrafficFlowId>& traffic_flow_ids,
                                                                  const std::vector<NocLinkId>& noc_link_ids) {
    std::vector<operations_research::sat::BoolVar> results;
    for (auto traffic_flow_id : traffic_flow_ids) {
        for (auto noc_link_id : noc_link_ids) {
            auto it = map.find({traffic_flow_id, noc_link_id});
            if (it != map.end()) {
                results.push_back(it->second);
            }
        }
    }
    return results;
}

static void forbid_illegal_turns(std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& flow_link_vars,
                                 operations_research::sat::CpModelBuilder& cp_model) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    auto noc_routing_alg = dynamic_cast<const TurnModelRouting*> (noc_ctx.noc_flows_router.get());
    VTR_ASSERT(noc_routing_alg != nullptr);

    // forbid illegal turns based on the routing algorithm
    // this includes 180 degree turns
    for (const auto& [link1, link2] : noc_routing_alg->get_all_illegal_turns(noc_ctx.noc_model)) {
        for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
            auto& first_var = flow_link_vars[{traffic_flow_id, link1}];
            auto& second_var = flow_link_vars[{traffic_flow_id, link2}];
            cp_model.AddBoolOr({first_var.Not(), second_var.Not()});
        }
    }
}

static vtr::vector<NocTrafficFlowId, int> rescale_traffic_flow_bandwidths() {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    const double link_bandwidth = noc_ctx.noc_model.get_noc_link_bandwidth();

    vtr::vector<NocTrafficFlowId, int> rescaled_traffic_flow_bandwidths;
    rescaled_traffic_flow_bandwidths.resize(traffic_flow_storage.get_number_of_traffic_flows());

    // rescale traffic flow bandwidths
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);
        double bandwidth = traffic_flow.traffic_flow_bandwidth;
        int rescaled_bandwidth = (int)std::floor((bandwidth / link_bandwidth) * NOC_LINK_BANDWIDTH_RESOLUTION);
        rescaled_traffic_flow_bandwidths[traffic_flow_id] = rescaled_bandwidth;
    }

    return  rescaled_traffic_flow_bandwidths;
}

static void add_congestion_constraints(std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& flow_link_vars,
                                       operations_research::sat::CpModelBuilder& cp_model) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    vtr::vector<NocTrafficFlowId, int> rescaled_traffic_flow_bandwidths = rescale_traffic_flow_bandwidths();

    // add NoC link congestion constraints
    for (const auto& noc_link : noc_ctx.noc_model.get_noc_links()) {
        const NocLinkId noc_link_id = noc_link.get_link_id();
        operations_research::sat::LinearExpr lhs;

        for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
            operations_research::sat::BoolVar binary_var = flow_link_vars[{traffic_flow_id, noc_link_id}];
            lhs += operations_research::sat::LinearExpr(binary_var * rescaled_traffic_flow_bandwidths[traffic_flow_id]);
        }

        cp_model.AddLessOrEqual(lhs, NOC_LINK_BANDWIDTH_RESOLUTION);
    }
}

static void add_continuity_constraints(std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& flow_link_vars,
                                       operations_research::sat::CpModelBuilder& cp_model)
{
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;
    const auto& place_ctx = g_vpr_ctx.placement();

    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_source_router_block_id].loc);
        NocRouterId sink_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_sink_router_block_id].loc);

        std::vector<operations_research::sat::BoolVar> vars;

        // exactly one outgoing link of the source must be selected
        const auto& src_outgoing_link_ids = noc_ctx.noc_model.get_noc_router_outgoing_links(source_router_id);
        vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, src_outgoing_link_ids);
        cp_model.AddExactlyOne(vars);

        // exactly one incoming link of the sink must be selected
        const auto& dst_incoming_link_ids = noc_ctx.noc_model.get_noc_router_incoming_links(sink_router_id);
        vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, dst_incoming_link_ids);
        cp_model.AddExactlyOne(vars);

        // each NoC router has at most one incoming and one outgoing link activated
        for (const auto& noc_router : noc_ctx.noc_model.get_noc_routers()) {
            const int noc_router_user_id = noc_router.get_router_user_id();
            const NocRouterId noc_router_id = noc_ctx.noc_model.convert_router_id(noc_router_user_id);

            if (noc_router_id == source_router_id || noc_router_id == sink_router_id) {
                continue;
            }

            const auto& incoming_links = noc_ctx.noc_model.get_noc_router_incoming_links(noc_router_id);
            vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, incoming_links);
            cp_model.AddAtMostOne(vars);

            operations_research::sat::LinearExpr lhs;
            lhs = operations_research::sat::LinearExpr::Sum(vars);

            const auto& outgoing_links = noc_ctx.noc_model.get_noc_router_outgoing_links(noc_router_id);
            vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, outgoing_links);
            cp_model.AddAtMostOne(vars);

            operations_research::sat::LinearExpr rhs;
            rhs = operations_research::sat::LinearExpr::Sum(vars);

            cp_model.AddEquality(lhs, rhs);
        }
    }
}

static void add_distance_constraints(std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& flow_link_vars,
                                     operations_research::sat::CpModelBuilder& cp_model,
                                     const std::vector<NocLinkId>& up,
                                     const std::vector<NocLinkId>& down,
                                     const std::vector<NocLinkId>& right,
                                     const std::vector<NocLinkId>& left) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;
    const auto& place_ctx = g_vpr_ctx.placement();
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    const int num_layers = g_vpr_ctx.device().grid.get_num_layers();

    // Get the logical block type for router
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = place_ctx.compressed_block_grids[router_block_type->index];

    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_src_router_block_id = traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_dst_router_block_id = traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId src_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_src_router_block_id].loc);
        NocRouterId dst_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_dst_router_block_id].loc);

        // get source, current, and destination NoC routers
        const auto& src_router = noc_ctx.noc_model.get_single_noc_router(src_router_id);
        const auto& dst_router = noc_ctx.noc_model.get_single_noc_router(dst_router_id);

        // get the position of source, current, and destination NoC routers
        const auto src_router_pos = src_router.get_router_physical_location();
        const auto dst_router_pos = dst_router.get_router_physical_location();

        // get the compressed location for source, current, and destination NoC routers
        auto compressed_src_loc = get_compressed_loc(compressed_noc_grid, t_pl_loc{src_router_pos, 0}, num_layers)[src_router_pos.layer_num];
        auto compressed_dst_loc = get_compressed_loc(compressed_noc_grid, t_pl_loc{dst_router_pos, 0}, num_layers)[dst_router_pos.layer_num];

        // calculate the distance between the current router and the destination
        const int delta_x = compressed_dst_loc.x - compressed_src_loc.x;
        const int delta_y = compressed_dst_loc.y - compressed_src_loc.y;

        auto right_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, right);
        auto left_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, left);
        auto up_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, up);
        auto down_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, down);

        operations_research::sat::LinearExpr horizontal_expr;
        horizontal_expr += operations_research::sat::LinearExpr::Sum(right_vars) - operations_research::sat::LinearExpr::Sum(left_vars);
        cp_model.AddEquality(horizontal_expr, delta_x);

        operations_research::sat::LinearExpr vertical_expr;
        vertical_expr += operations_research::sat::LinearExpr::Sum(up_vars) - operations_research::sat::LinearExpr::Sum(down_vars);
        cp_model.AddEquality(vertical_expr, delta_y);
    }
}

static void group_noc_links_based_on_direction(std::vector<NocLinkId>& up,
                                               std::vector<NocLinkId>& down,
                                               std::vector<NocLinkId>& right,
                                               std::vector<NocLinkId>& left) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;

    for (const auto& noc_link : noc_model.get_noc_links()) {
        const NocLinkId noc_link_id = noc_link.get_link_id();
        const NocRouterId src_noc_router_id = noc_link.get_source_router();
        const NocRouterId dst_noc_router_id = noc_link.get_sink_router();
        const NocRouter& src_noc_router = noc_model.get_single_noc_router(src_noc_router_id);
        const NocRouter& dst_noc_router = noc_model.get_single_noc_router(dst_noc_router_id);
        auto src_loc = src_noc_router.get_router_physical_location();
        auto dst_loc = dst_noc_router.get_router_physical_location();

        VTR_ASSERT(src_loc.x == dst_loc.x || src_loc.y == dst_loc.y);

        if (src_loc.x == dst_loc.x) { // vertical link
            if (dst_loc.y > src_loc.y) {
                up.push_back(noc_link_id);
            } else {
                down.push_back(noc_link_id);
            }
        } else { // horizontal link
            if (dst_loc.x > src_loc.x) {
                right.push_back(noc_link_id);
            } else {
                left.push_back(noc_link_id);
            }
        }
    }
}

static int comp_max_number_of_traversed_links(NocTrafficFlowId traffic_flow_id) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

    const double noc_link_latency = noc_model.get_noc_link_latency();
    const double noc_router_latency = noc_model.get_noc_router_latency();
    const double traffic_flow_latency_constraint = traffic_flow.max_traffic_flow_latency;

    VTR_ASSERT(traffic_flow_latency_constraint < 0.1);

    int n_max_links = std::floor((traffic_flow_latency_constraint - noc_router_latency) / (noc_link_latency + noc_router_latency));

    return n_max_links;

}


std::vector<NocLinkId> sort_noc_links_in_chain_order(const std::vector<NocLinkId>& links) {
    std::vector<NocLinkId> route;
    if (links.empty()) {
        return route;
    }

    const auto& noc_model = g_vpr_ctx.noc().noc_model;

    // Create a map to find pairs by their first element
    std::unordered_map<NocRouterId , NocLinkId> src_map;
    std::unordered_map<NocRouterId , bool> is_dst;
    for (const auto& l : links) {
        NocRouterId src_router_id = noc_model.get_single_noc_link(l).get_source_router();
        NocRouterId dst_router_id = noc_model.get_single_noc_link(l).get_sink_router();
        src_map[src_router_id] = l;
        is_dst[dst_router_id] = true;
    }

    // Find the starting pair (whose first element is not a second element of any pair)
    auto it = links.begin();
    for (; it != links.end(); ++it) {
        NocRouterId src_router_id = noc_model.get_single_noc_link(*it).get_source_router();
        if (is_dst.find(src_router_id) == is_dst.end()) {
            break;
        }
    }

    // Reconstruct the chain starting from the found starting pair
    auto current = *it;
    while (true) {
        route.push_back(current);
        NocRouterId dst_router_id = noc_model.get_single_noc_link(current).get_sink_router();
        auto nextIt = src_map.find(dst_router_id);
        if (nextIt == src_map.end()) {
            break; // End of chain
        }
        current = nextIt->second;
    }

    VTR_ASSERT(route.size() == links.size());

    return route;
}

static vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> convert_vars_to_routes(std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar>& flow_link_vars,
                                                                                    const operations_research::sat::CpSolverResponse& response) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    VTR_ASSERT(response.status() == operations_research::sat::CpSolverStatus::FEASIBLE ||
               response.status() == operations_research::sat::CpSolverStatus::OPTIMAL);

    vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> routes;
    routes.resize(traffic_flow_storage.get_number_of_traffic_flows());

    for (auto& [key, var] : flow_link_vars) {
        auto [traffic_flow_id, noc_link_id] = key;
        bool value = operations_research::sat::SolutionBooleanValue(response, var);
        if (value) {
            routes[traffic_flow_id].push_back(noc_link_id);
        }
    }

    for (auto& route : routes) {
        route = sort_noc_links_in_chain_order(route);
    }

    return routes;
}

vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> noc_sat_route(bool minimize_aggregate_bandwidth) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    // Used to add variables and  constraints to a CP-SAT model
    operations_research::sat::CpModelBuilder cp_model;

    /*
     * For each traffic flow and NoC link pair, we create a boolean variable.
     * When a variable associated with traffic flow t and NoC link l is set,
     * it means that t is routed through l.
     */
    std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, operations_research::sat::BoolVar> flow_link_vars;

    /*
     * Each traffic flow latency constraint is translated to how many NoC links
     * the traffic flow can traverse without violating the constraint.
     * These integer variables specify the number of additional links traversed
     * beyond the maximum allowed number of links.
     */
    std::map<NocTrafficFlowId , operations_research::sat::IntVar> latency_overrun_vars;
    // TODO: specify the domain based on NoC topology
    operations_research::Domain latency_overrun_domain(0, 20);

    // create boolean variables for each traffic flow and link pair
    // create integer variables for traffic flows with constrained latency
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // create an integer variable for each latency-constrained traffic flow
        if (traffic_flow.max_traffic_flow_latency < 0.1) {
            latency_overrun_vars[traffic_flow_id] = cp_model.NewIntVar(latency_overrun_domain);
        }

        // create (traffic flow, NoC link) pair boolean variables
        for (const auto& noc_link : noc_model.get_noc_links()) {
            const NocLinkId noc_link_id = noc_link.get_link_id();
            flow_link_vars[{traffic_flow_id, noc_link_id}] = cp_model.NewBoolVar();
        }
    }

    for (auto& [traffic_flow_id, latency_overrun_var] : latency_overrun_vars) {
        int n_max_links = comp_max_number_of_traversed_links(traffic_flow_id);
        auto link_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id},
                                            {noc_model.get_noc_links().keys().begin(), noc_model.get_noc_links().keys().end()});

        operations_research::sat::LinearExpr latency_overrun_expr;
        latency_overrun_expr += operations_research::sat::LinearExpr::Sum(link_vars);
        latency_overrun_expr -= n_max_links;

        cp_model.AddMaxEquality(latency_overrun_var, {latency_overrun_expr, 0});
    }

    forbid_illegal_turns(flow_link_vars, cp_model);

    add_congestion_constraints(flow_link_vars, cp_model);

    add_continuity_constraints(flow_link_vars, cp_model);

    std::vector<NocLinkId> up, down, right, left;
    group_noc_links_based_on_direction(up, down, right, left);

    add_distance_constraints(flow_link_vars, cp_model, up, down, right, left);

    operations_research::sat::LinearExpr latency_overrun_sum;
    for (auto& [traffic_flow_id, latency_overrun_var] : latency_overrun_vars) {
        latency_overrun_sum += latency_overrun_var;
    }

    cp_model.Minimize(latency_overrun_sum);

    operations_research::sat::CpSolverResponse response = operations_research::sat::Solve(cp_model.Build());

    if (response.status() == operations_research::sat::CpSolverStatus::FEASIBLE ||
        response.status() == operations_research::sat::CpSolverStatus::OPTIMAL) {

        if (!minimize_aggregate_bandwidth) {
            auto routes = convert_vars_to_routes(flow_link_vars, response);
            return routes;
        } else {
            int latency_overrun_value = operations_research::sat::SolutionIntegerValue(response, latency_overrun_sum);
            cp_model.AddEquality(latency_overrun_sum, latency_overrun_value);

            auto rescaled_traffic_flow_bandwidths = rescale_traffic_flow_bandwidths();
            operations_research::sat::LinearExpr agg_bw_expr;
            for (auto& [key, var] : flow_link_vars) {
                auto [traffic_flow_id, noc_link_id] = key;
                agg_bw_expr += operations_research::sat::LinearExpr(var * rescaled_traffic_flow_bandwidths[traffic_flow_id]);
            }

            cp_model.Minimize(agg_bw_expr);
            response = operations_research::sat::Solve(cp_model.Build());

            if (response.status() == operations_research::sat::CpSolverStatus::FEASIBLE ||
                response.status() == operations_research::sat::CpSolverStatus::OPTIMAL) {
                auto routes = convert_vars_to_routes(flow_link_vars, response);
                return routes;
            }
        }
    }

    return {};
}