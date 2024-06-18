
#include "noc_place_utils.h"

#include "globals.h"
#include "vtr_log.h"
#include "vtr_assert.h"
#include "vtr_random.h"

#include "channel_dependency_graph.h"
#include "noc_routing_algorithm_creator.h"
#include "noc_routing.h"
#include "place_constraints.h"
#include "move_transactions.h"

#ifdef ENABLE_NOC_SAT_ROUTING
#include "sat_routing.h"
#endif

#include <fstream>

/********************** Variables local to noc_place_utils.c pp***************************/
/* Proposed and actual cost of a noc traffic flow used for each move assessment */
static vtr::vector<NocTrafficFlowId, TrafficFlowPlaceCost> traffic_flow_costs, proposed_traffic_flow_costs;

/* Keeps track of traffic flows that have been updated at each attempted placement move*/
static std::vector<NocTrafficFlowId> affected_traffic_flows;

/* Proposed and actual congestion cost of a NoC link used for each move assessment */
static vtr::vector<NocLinkId, double> link_congestion_costs, proposed_link_congestion_costs;

/* Keeps track of NoC links whose bandwidth usage have been updated at each attempted placement move*/
static std::unordered_set<NocLinkId> affected_noc_links;
/*********************************************************** *****************************/

/**
 * @brief Randomly select a movable NoC router cluster blocks
 *
 * @param b_from The cluster block ID of the selected NoC router
 * @param from The current location of the selected NoC router
 * @param cluster_from_type Block type of the selected block
 * @return bool True if a block was selected successfully.
 * False if there are no NoC routers in the netlist or the
 * selected NoC router is fixed/
 */
static bool select_random_router_cluster(ClusterBlockId& b_from,
                                         t_pl_loc& from,
                                         t_logical_block_type_ptr& cluster_from_type);

/**
 * @brief Given two traffic flow routes, finds links that appear
 * only in one route.
 *
 * @param prev_links Previous route before re-routing the traffic flow
 * @param curr_links Current route after re-routing the traffic flow
 *
 * @return Unique links that appear only in one of the given routes
 */
static std::vector<NocLinkId> find_affected_links_by_flow_reroute(std::vector<NocLinkId>& prev_links,
                                                                  std::vector<NocLinkId>& curr_links);

void initial_noc_routing(const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& new_traffic_flow_routes) {
    // need to update the link usages within after routing all the traffic flows
    // also need to route all the traffic flows and store them
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    VTR_ASSERT(new_traffic_flow_routes.size() == (size_t)noc_traffic_flows_storage.get_number_of_traffic_flows() ||
               new_traffic_flow_routes.empty());

    /* We need all the traffic flow ids to be able to access them. The range
     * of traffic flow ids go from 0 to the total number of traffic flows within
     * the NoC.
     * Go through all the traffic flows and route them. Then once routed,
     * update the links used in the routed traffic flows with their usages
     */
    for (const auto& traffic_flow_id : noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

        /* Update the traffic flow route based on where the router cluster blocks are placed.
         * If the caller has not provided traffic flow routes, route traffic flow, otherwise use the provided route.
         */
        const std::vector<NocLinkId>& curr_traffic_flow_route = new_traffic_flow_routes.empty()
                                                                    ? route_traffic_flow(traffic_flow_id, noc_ctx.noc_model, noc_traffic_flows_storage, *noc_ctx.noc_flows_router)
                                                                    : new_traffic_flow_routes[traffic_flow_id];

        if (!new_traffic_flow_routes.empty()) {
            noc_traffic_flows_storage.get_mutable_traffic_flow_route(traffic_flow_id) = curr_traffic_flow_route;
        }

        // update the links used in the found traffic flow route, links' bandwidth should be incremented since the traffic flow is routed
        update_traffic_flow_link_usage(curr_traffic_flow_route, noc_ctx.noc_model, 1, curr_traffic_flow.traffic_flow_bandwidth);
    }
}

void reinitialize_noc_routing(t_placer_costs& costs,
                              const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& new_traffic_flow_routes) {
    // used to access NoC links and modify them
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    VTR_ASSERT((size_t)noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows() == new_traffic_flow_routes.size() ||
               new_traffic_flow_routes.empty());

    // Zero out bandwidth usage for all links
    for (auto& noc_link : noc_ctx.noc_model.get_mutable_noc_links()) {
        noc_link.set_bandwidth_usage(0.0);
    }

    // Route traffic flows and update link bandwidth usage
    initial_noc_routing(new_traffic_flow_routes);

    // Initialize traffic_flow_costs
    costs.noc_cost_terms.aggregate_bandwidth = comp_noc_aggregate_bandwidth_cost();
    std::tie(costs.noc_cost_terms.latency, costs.noc_cost_terms.latency_overrun) = comp_noc_latency_cost();
    costs.noc_cost_terms.congestion = comp_noc_congestion_cost();
}

void find_affected_noc_routers_and_update_noc_costs(const t_pl_blocks_to_be_moved& blocks_affected,
                                                    NocCostTerms& delta_c) {
    /* For speed, delta_c is passed by reference instead of being returned.
     * We expect delta cost terms to be zero to ensure correctness.
     */
    VTR_ASSERT_SAFE(delta_c.aggregate_bandwidth == 0.);
    VTR_ASSERT_SAFE(delta_c.latency == 0.);
    VTR_ASSERT_SAFE(delta_c.latency_overrun == 0.);
    VTR_ASSERT_SAFE(delta_c.congestion == 0.);
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    // keeps track of traffic flows that have been re-routed
    // This is useful for cases where two moved routers were part of the same traffic flow and prevents us from re-routing the same flow twice.
    std::unordered_set<NocTrafficFlowId> updated_traffic_flows;

    affected_traffic_flows.clear();
    affected_noc_links.clear();

    // go through the moved blocks and process them only if they are NoC routers
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        // check if the current moved block is a noc router
        if (noc_traffic_flows_storage.check_if_cluster_block_has_traffic_flows(blk)) {
            // current block is a router, so re-route all the traffic flows it is a part of
            re_route_associated_traffic_flows(blk, noc_traffic_flows_storage, noc_ctx.noc_model, *noc_ctx.noc_flows_router, updated_traffic_flows);
        }
    }

    // go through all the affected traffic flows and calculate their new costs after being re-routed, then determine the change in cost before the traffic flows were modified
    for (auto& traffic_flow_id : affected_traffic_flows) {
        // get the traffic flow route
        const std::vector<NocLinkId>& traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

        // get the current traffic flow info
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // calculate the new aggregate bandwidth and latency costs for the affected traffic flow
        proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = calculate_traffic_flow_aggregate_bandwidth_cost(traffic_flow_route, curr_traffic_flow);
        std::tie(proposed_traffic_flow_costs[traffic_flow_id].latency,
                 proposed_traffic_flow_costs[traffic_flow_id].latency_overrun)
            = calculate_traffic_flow_latency_cost(traffic_flow_route, noc_ctx.noc_model, curr_traffic_flow);

        // compute how much the aggregate bandwidth and latency costs change with this swap
        delta_c.aggregate_bandwidth += proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth - traffic_flow_costs[traffic_flow_id].aggregate_bandwidth;
        delta_c.latency += proposed_traffic_flow_costs[traffic_flow_id].latency - traffic_flow_costs[traffic_flow_id].latency;
        delta_c.latency_overrun += proposed_traffic_flow_costs[traffic_flow_id].latency_overrun - traffic_flow_costs[traffic_flow_id].latency_overrun;
    }

    // Iterate over all affected links and calculate their new congestion cost and store it
    for (const NocLink& link : noc_ctx.noc_model.get_noc_links(affected_noc_links)) {
        // calculate the new congestion cost for the link and store it
        proposed_link_congestion_costs[link] = calculate_link_congestion_cost(link);

        // compute how much the congestion cost changes with this swap
        delta_c.congestion += proposed_link_congestion_costs[link] - link_congestion_costs[link];
    }
}

void commit_noc_costs() {
    // used to access NoC links
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    // Iterate over all the traffic flows affected by the proposed router swap
    for (auto& traffic_flow_id : affected_traffic_flows) {
        // update the traffic flow costs
        traffic_flow_costs[traffic_flow_id] = proposed_traffic_flow_costs[traffic_flow_id];

        // reset the proposed traffic flows costs
        proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = INVALID_NOC_COST_TERM;
        proposed_traffic_flow_costs[traffic_flow_id].latency = INVALID_NOC_COST_TERM;
        proposed_traffic_flow_costs[traffic_flow_id].latency_overrun = INVALID_NOC_COST_TERM;
    }

    // Iterate over all the NoC links whose bandwidth utilization was affected by the proposed move
    for (const NocLink& link : noc_ctx.noc_model.get_noc_links(affected_noc_links)) {
        // commit the new link congestion cost
        link_congestion_costs[link] = proposed_link_congestion_costs[link];

        // invalidate the proposed link congestion flow costs
        proposed_link_congestion_costs[link] = INVALID_NOC_COST_TERM;
    }
}

std::vector<NocLinkId>& route_traffic_flow(NocTrafficFlowId traffic_flow_id,
                                           const NocStorage& noc_model,
                                           NocTrafficFlows& noc_traffic_flows_storage,
                                           NocRouting& noc_flows_router) {
    // provides the positions where the affected blocks have moved to
    auto& place_ctx = g_vpr_ctx.placement();

    // get the traffic flow with the current id
    const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

    // get the source and destination logical router blocks in the current traffic flow
    ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
    ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

    // get the ids of the hard router blocks where the logical router cluster blocks have been placed
    NocRouterId source_router_block_id = noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_source_router_block_id].loc);
    NocRouterId sink_router_block_id = noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_sink_router_block_id].loc);

    // route the current traffic flow
    std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_mutable_traffic_flow_route(traffic_flow_id);
    noc_flows_router.route_flow(source_router_block_id, sink_router_block_id, traffic_flow_id, curr_traffic_flow_route, noc_model);

    return curr_traffic_flow_route;
}

void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route,
                                    NocStorage& noc_model,
                                    int inc_or_dec,
                                    double traffic_flow_bandwidth) {
    // go through the links within the traffic flow route and update their bandwidth usage
    for (auto& link_in_route_id : traffic_flow_route) {
        // get the link to update and its current bandwidth
        NocLink& curr_link = noc_model.get_single_mutable_noc_link(link_in_route_id);
        double curr_link_bandwidth = curr_link.get_bandwidth_usage();

        curr_link.set_bandwidth_usage(curr_link_bandwidth + inc_or_dec * traffic_flow_bandwidth);

        // check that the bandwidth never goes to negative
        VTR_ASSERT(curr_link.get_bandwidth_usage() >= 0.0);
    }
}

void re_route_associated_traffic_flows(ClusterBlockId moved_block_router_id,
                                       NocTrafficFlows& noc_traffic_flows_storage,
                                       NocStorage& noc_model,
                                       NocRouting& noc_flows_router,
                                       std::unordered_set<NocTrafficFlowId>& updated_traffic_flows) {
    // get all the associated traffic flows for the logical router cluster block
    const auto& assoc_traffic_flows = noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(moved_block_router_id);

    // if there are traffic flows associated to the current router block, process them
    for (auto traffic_flow_id : assoc_traffic_flows) {
        // first check to see whether we have already re-routed the current traffic flow and only re-route it if we haven't already.
        if (updated_traffic_flows.find(traffic_flow_id) == updated_traffic_flows.end()) {
            // get all links for this flow route before it is rerouted
            // The returned const std::vector<NocLinkId>& is copied so that we can modify (sort) it
            std::vector<NocLinkId> prev_traffic_flow_links = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

            // now update the current traffic flow by re-routing it based on the new locations of its src and destination routers
            re_route_traffic_flow(traffic_flow_id, noc_traffic_flows_storage, noc_model, noc_flows_router);

            // now make sure we don't update this traffic flow a second time by adding it to the group of updated traffic flows
            updated_traffic_flows.insert(traffic_flow_id);

            // get all links for this flow route after it is rerouted
            std::vector<NocLinkId> curr_traffic_flow_links = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

            // find links that appear in the old route or the new one, but not both of them
            // these are the links whose bandwidth utilization is affected by rerouting
            auto unique_links = find_affected_links_by_flow_reroute(prev_traffic_flow_links, curr_traffic_flow_links);

            // update the static data structure to remember which links were affected by router swap
            affected_noc_links.insert(unique_links.begin(), unique_links.end());

            // update global datastructures to indicate that the current traffic flow was affected due to router cluster blocks being swapped
            affected_traffic_flows.push_back(traffic_flow_id);
        }
    }
}

void revert_noc_traffic_flow_routes(const t_pl_blocks_to_be_moved& blocks_affected) {
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    // keeps track of traffic flows that have been reverted
    // This is useful for cases where two moved routers were part of the same traffic flow and prevents us from re-routing the same flow twice.
    std::unordered_set<NocTrafficFlowId> reverted_traffic_flows;

    // go through the moved blocks and process them only if they are NoC routers
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        // check if the current moved block is a noc router
        if (noc_traffic_flows_storage.check_if_cluster_block_has_traffic_flows(blk)) {
            // current block is a router, so re-route all the traffic flows it is a part of //

            // get all the associated traffic flows for the logical router cluster block
            const std::vector<NocTrafficFlowId>& assoc_traffic_flows = noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(blk);

            // if there are traffic flows associated to the current router block, process them
            for (auto traffic_flow_id : assoc_traffic_flows) {
                // first check to see whether we have already reverted the current traffic flow and only revert it if we haven't already.
                if (reverted_traffic_flows.find(traffic_flow_id) == reverted_traffic_flows.end()) {
                    // Revert the traffic flow route by re-routing it
                    re_route_traffic_flow(traffic_flow_id, noc_traffic_flows_storage, noc_ctx.noc_model, *noc_ctx.noc_flows_router);

                    // make sure we do not revert this traffic flow again
                    reverted_traffic_flows.insert(traffic_flow_id);
                }
            }
        }
    }
}

void re_route_traffic_flow(NocTrafficFlowId traffic_flow_id,
                           NocTrafficFlows& noc_traffic_flows_storage,
                           NocStorage& noc_model,
                           NocRouting& noc_flows_router) {
    // get the current traffic flow info
    const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

    /*  since the current traffic flow route will be 
     * changed, first we need to decrement the bandwidth
     * usage of all links that are part of
     * the existing traffic flow route
     */
    const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);
    update_traffic_flow_link_usage(curr_traffic_flow_route, noc_model, -1, curr_traffic_flow.traffic_flow_bandwidth);

    // now get the re-routed traffic flow route and increment all the link usages with this reverted route
    std::vector<NocLinkId>& re_routed_traffic_flow_route = route_traffic_flow(traffic_flow_id, noc_model, noc_traffic_flows_storage, noc_flows_router);
    update_traffic_flow_link_usage(re_routed_traffic_flow_route, noc_model, 1, curr_traffic_flow.traffic_flow_bandwidth);
}

void recompute_noc_costs(NocCostTerms& new_cost) {
    auto& noc_ctx = g_vpr_ctx.noc();

    // reset the cost variables first
    new_cost = NocCostTerms{0.0, 0.0, 0.0, 0.0};

    // go through the costs of all the traffic flows and add them up to recompute the total costs associated with the NoC
    for (const auto& traffic_flow_id : noc_ctx.noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        new_cost.aggregate_bandwidth += traffic_flow_costs[traffic_flow_id].aggregate_bandwidth;
        new_cost.latency += traffic_flow_costs[traffic_flow_id].latency;
        new_cost.latency_overrun += traffic_flow_costs[traffic_flow_id].latency_overrun;
    }

    // Iterate over all NoC links and accumulate their congestion costs
    for (auto& link_id : noc_ctx.noc_model.get_noc_links()) {
        new_cost.congestion += link_congestion_costs[link_id];
    }
}

void update_noc_normalization_factors(t_placer_costs& costs) {
    //Prevent the norm factors from going to infinity
    costs.noc_cost_norm_factors.aggregate_bandwidth = std::min(1 / costs.noc_cost_terms.aggregate_bandwidth, MAX_INV_NOC_AGGREGATE_BANDWIDTH_COST);
    costs.noc_cost_norm_factors.latency = std::min(1 / costs.noc_cost_terms.latency, MAX_INV_NOC_LATENCY_COST);

    // to avoid division by zero and negative numbers
    // latency overrun cost may take very small negative values due to round-off error
    if (costs.noc_cost_terms.latency_overrun > 0.0) {
        costs.noc_cost_norm_factors.latency_overrun = std::min(1 / costs.noc_cost_terms.latency_overrun, MAX_INV_NOC_LATENCY_COST);
    } else {
        costs.noc_cost_norm_factors.latency_overrun = MAX_INV_NOC_LATENCY_COST;
    }

    // to avoid division by zero and negative numbers
    // congestion cost may take very small negative values due to round-off error
    if (costs.noc_cost_terms.congestion > 0.0) {
        costs.noc_cost_norm_factors.congestion = std::min(1 / costs.noc_cost_terms.congestion, MAX_INV_NOC_CONGESTION_COST);
    } else {
        costs.noc_cost_norm_factors.congestion = MAX_INV_NOC_CONGESTION_COST;
    }
}

double comp_noc_aggregate_bandwidth_cost() {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    double noc_aggregate_bandwidth_cost = 0.;

    // now go through each traffic flow route and calculate its
    // aggregate bandwidth. Then store this in local data structures and accumulate it.
    for (const auto& traffic_flow_id : g_vpr_ctx.noc().noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

        double curr_traffic_flow_aggregate_bandwidth_cost = calculate_traffic_flow_aggregate_bandwidth_cost(curr_traffic_flow_route, curr_traffic_flow);

        // store the calculated aggregate bandwidth for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = curr_traffic_flow_aggregate_bandwidth_cost;

        // accumulate the aggregate bandwidth cost
        noc_aggregate_bandwidth_cost += curr_traffic_flow_aggregate_bandwidth_cost;
    }

    return noc_aggregate_bandwidth_cost;
}

std::pair<double, double> comp_noc_latency_cost() {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    std::pair<double, double> noc_latency_cost_terms{0.0, 0.0};

    // now go through each traffic flow route and calculate its
    // latency. Then store this in local data structures and accumulate it.
    for (const auto& traffic_flow_id : noc_ctx.noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

        auto [curr_traffic_flow_latency, curr_traffic_flow_latency_overrun] = calculate_traffic_flow_latency_cost(curr_traffic_flow_route, noc_ctx.noc_model, curr_traffic_flow);

        // store the calculated latency cost terms for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_costs[traffic_flow_id].latency = curr_traffic_flow_latency;
        traffic_flow_costs[traffic_flow_id].latency_overrun = curr_traffic_flow_latency_overrun;

        // accumulate the latency cost terms
        noc_latency_cost_terms.first += curr_traffic_flow_latency;
        noc_latency_cost_terms.second += curr_traffic_flow_latency_overrun;
    }

    return noc_latency_cost_terms;
}

double comp_noc_congestion_cost() {
    // Used to access NoC links
    auto& noc_ctx = g_vpr_ctx.noc();

    double congestion_cost = 0.;

    // Iterate over all NoC links
    for (const auto& link : noc_ctx.noc_model.get_noc_links()) {
        double link_congestion_cost = calculate_link_congestion_cost(link);

        // store the congestion cost for this link in static data structures (this also initializes them)
        link_congestion_costs[link] = link_congestion_cost;

        // accumulate the congestion cost
        congestion_cost += link_congestion_cost;
    }

    return congestion_cost;
}

int check_noc_placement_costs(const t_placer_costs& costs, double error_tolerance, const t_noc_opts& noc_opts) {
    int error = 0;
    NocCostTerms cost_check{0.0, 0.0, 0.0, 0.0};

    // get current router block locations
    auto& place_ctx = g_vpr_ctx.placement();
    const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations = place_ctx.block_locs;

    auto& noc_ctx = g_vpr_ctx.noc();
    const NocStorage& noc_model = noc_ctx.noc_model;
    const NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    // a copy of NoC link storage used to calculate link bandwidth utilization from scratch
    vtr::vector<NocLinkId, NocLink> temp_noc_link_storage = noc_model.get_noc_links();

    // reset bandwidth utilization for all links
    std::for_each(temp_noc_link_storage.begin(), temp_noc_link_storage.end(), [](NocLink& link) { link.set_bandwidth_usage(0.0); });

    // need to create a temporary noc routing algorithm
    std::unique_ptr<NocRouting> temp_noc_routing_algorithm = NocRoutingAlgorithmCreator::create_routing_algorithm(noc_opts.noc_routing_algorithm);

    // stores a temporarily found route for a traffic flow
    std::vector<NocLinkId> temp_found_noc_route;

    // go through all the traffic flows and find a route for them based on where the routers are placed within the NoC
    for (const auto& traffic_flow_id : noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        // get the traffic flow with the current id
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_block_id = noc_model.get_router_at_grid_location(placed_cluster_block_locations[logical_source_router_block_id].loc);
        NocRouterId sink_router_block_id = noc_model.get_router_at_grid_location(placed_cluster_block_locations[logical_sink_router_block_id].loc);

        // route the current traffic flow
        temp_noc_routing_algorithm->route_flow(source_router_block_id, sink_router_block_id, traffic_flow_id, temp_found_noc_route, noc_model);

        // now calculate the costs associated to the current traffic flow and accumulate it to find the total cost of the NoC placement
        double current_flow_aggregate_bandwidth_cost = calculate_traffic_flow_aggregate_bandwidth_cost(temp_found_noc_route, curr_traffic_flow);
        cost_check.aggregate_bandwidth += current_flow_aggregate_bandwidth_cost;

        auto [curr_traffic_flow_latency_cost, curr_traffic_flow_latency_overrun_cost] = calculate_traffic_flow_latency_cost(temp_found_noc_route, noc_model, curr_traffic_flow);
        cost_check.latency += curr_traffic_flow_latency_cost;
        cost_check.latency_overrun += curr_traffic_flow_latency_overrun_cost;

        // increase bandwidth utilization for the links that constitute the current flow's route
        for (auto& link_id : temp_found_noc_route) {
            auto& link = temp_noc_link_storage[link_id];
            double curr_link_bw_util = link.get_bandwidth_usage();
            link.set_bandwidth_usage(curr_link_bw_util + curr_traffic_flow.traffic_flow_bandwidth);
            VTR_ASSERT(link.get_bandwidth_usage() >= 0.0);
        }

        // clear the current traffic flow route, so we can route the next traffic flow
        temp_found_noc_route.clear();
    }

    // Iterate over all NoC links and accumulate congestion cost
    for (const auto& link : temp_noc_link_storage) {
        cost_check.congestion += calculate_link_congestion_cost(link);
    }

    // check whether the aggregate bandwidth placement cost is within the error tolerance
    if (fabs(cost_check.aggregate_bandwidth - costs.noc_cost_terms.aggregate_bandwidth) > costs.noc_cost_terms.aggregate_bandwidth * error_tolerance) {
        VTR_LOG_ERROR(
            "noc_aggregate_bandwidth_cost_check: %g and noc_aggregate_bandwidth_cost: %g differ in check_noc_placement_costs.\n",
            cost_check.aggregate_bandwidth, costs.noc_cost_terms.aggregate_bandwidth);
        error++;
    }

    // only check the recomputed cost if it is above our expected latency cost threshold of 1 pico-second, otherwise there is no point in checking it
    if (cost_check.latency > MIN_EXPECTED_NOC_LATENCY_COST) {
        // check whether the latency placement cost is within the error tolerance
        if (fabs(cost_check.latency - costs.noc_cost_terms.latency) > costs.noc_cost_terms.latency * error_tolerance) {
            VTR_LOG_ERROR(
                "noc_latency_cost_check: %g and noc_latency_cost: %g differ in check_noc_placement_costs.\n",
                cost_check.latency, costs.noc_cost_terms.latency);
            error++;
        }
    }

    // only check the recomputed cost if it is above our expected latency cost threshold of 1 pico-second, otherwise there is no point in checking it
    if (cost_check.latency_overrun > MIN_EXPECTED_NOC_LATENCY_COST) {
        // check whether the latency overrun placement cost is within the error tolerance
        if (fabs(cost_check.latency_overrun - costs.noc_cost_terms.latency_overrun) > costs.noc_cost_terms.latency_overrun * error_tolerance) {
            VTR_LOG_ERROR(
                "noc_latency_overrun_cost_check: %g and noc_latency_overrun_cost: %g differ in check_noc_placement_costs.\n",
                cost_check.latency_overrun, costs.noc_cost_terms.latency_overrun);
            error++;
        }
    }

    // check the recomputed congestion cost only if it is higher than the minimum expected value
    if (cost_check.congestion > MIN_EXPECTED_NOC_CONGESTION_COST) {
        // check whether the NoC congestion cost is within the error range
        if (fabs(cost_check.congestion - costs.noc_cost_terms.congestion) > costs.noc_cost_terms.congestion * error_tolerance) {
            VTR_LOG_ERROR(
                "noc_congestion_cost_check: %g and noc_congestion_cost: %g differ in check_noc_placement_costs.\n",
                cost_check.congestion, costs.noc_cost_terms.congestion);
            error++;
        }
    }

    return error;
}

double calculate_traffic_flow_aggregate_bandwidth_cost(const std::vector<NocLinkId>& traffic_flow_route, const t_noc_traffic_flow& traffic_flow_info) {
    int num_of_links_in_traffic_flow = traffic_flow_route.size();

    // the traffic flow aggregate bandwidth cost is scaled by its priority, which dictates its importance to the placement
    return (traffic_flow_info.traffic_flow_priority * traffic_flow_info.traffic_flow_bandwidth * num_of_links_in_traffic_flow);
}

std::pair<double, double> calculate_traffic_flow_latency_cost(const std::vector<NocLinkId>& traffic_flow_route,
                                                              const NocStorage& noc_model,
                                                              const t_noc_traffic_flow& traffic_flow_info) {

    double noc_link_latency_component = 0.0;
    if (noc_model.get_detailed_link_latency()) {
        for (const NocLink& link : noc_model.get_noc_links(traffic_flow_route)) {
            double link_latency = link.get_latency();
            noc_link_latency_component += link_latency;
        }
    } else {
        auto num_of_links_in_traffic_flow = (double)traffic_flow_route.size();
        double noc_link_latency = noc_model.get_noc_link_latency();
        noc_link_latency_component = noc_link_latency * num_of_links_in_traffic_flow;
    }

    double noc_router_latency_component = 0.0;

    if (noc_model.get_detailed_router_latency()) {
        NocLinkId first_noc_link_id = traffic_flow_route[0];
        const NocLink& first_noc_link = noc_model.get_single_noc_link(first_noc_link_id);
        NocRouterId source_noc_router_id = first_noc_link.get_source_router();
        const NocRouter& source_noc_router = noc_model.get_single_noc_router(source_noc_router_id);
        noc_router_latency_component = source_noc_router.get_latency();

        for (const NocLink& link : noc_model.get_noc_links(traffic_flow_route)) {
            const NocRouterId sink_router_id = link.get_sink_router();
            const NocRouter& sink_router = noc_model.get_single_noc_router(sink_router_id);
            double noc_router_latency = sink_router.get_latency();
            noc_router_latency_component += noc_router_latency;
        }
    } else {
        // there will always be one more router than links in a traffic flow
        auto num_of_routers_in_traffic_flow = (double)traffic_flow_route.size() + 1;
        double noc_router_latency = noc_model.get_noc_router_latency();
        noc_router_latency_component = noc_router_latency * num_of_routers_in_traffic_flow;
    }


    // calculate the total traffic flow latency
    double latency = noc_router_latency_component + noc_link_latency_component;

    // calculate the traffic flow latency overrun
    double max_latency = traffic_flow_info.max_traffic_flow_latency;
    double latency_overrun = std::max(latency - max_latency, 0.);

    // scale the latency cost by its priority to indicate its importance
    latency *= traffic_flow_info.traffic_flow_priority;
    latency_overrun *= traffic_flow_info.traffic_flow_priority;

    return {latency, latency_overrun};
}

double calculate_link_congestion_cost(const NocLink& link) {
    double congested_bw_ratio = link.get_congested_bandwidth_ratio();

    return congested_bw_ratio;
}

void normalize_noc_cost_weighting_factor(t_noc_opts& noc_opts) {
    // calculate the sum of all weighting factors
    double weighting_factor_sum = noc_opts.noc_latency_weighting +
                                  noc_opts.noc_latency_constraints_weighting +
                                  noc_opts.noc_congestion_weighting +
                                  noc_opts.noc_aggregate_bandwidth_weighting;

    // Normalize weighting factor so they add up to 1
    noc_opts.noc_aggregate_bandwidth_weighting /= weighting_factor_sum;
    noc_opts.noc_latency_weighting /= weighting_factor_sum;
    noc_opts.noc_latency_constraints_weighting /= weighting_factor_sum;
    noc_opts.noc_congestion_weighting /= weighting_factor_sum;
}

double calculate_noc_cost(const NocCostTerms& cost_terms,
                          const NocCostTerms& norm_factors,
                          const t_noc_opts& noc_opts) {
    double cost = 0.0;

    /* NoC's contribution to the placement cost is a weighted sum over:
     * 1) Traffic flow aggregate bandwidth cost
     * 2) Traffic flow latency cost
     * 3) Traffic flow latency overrun cost
     * 4) Link congestion cost
     *
     * Since NoC-related cost terms have different scales, they are
     * rescaled by multiplying each cost term with its corresponding
     * normalization factor. Then, a weighted sum over normalized cost terms
     * is computed. Weighting factors determine the contribution of each
     * normalized term to the sum.
     */
    cost = noc_opts.noc_placement_weighting * (cost_terms.aggregate_bandwidth * norm_factors.aggregate_bandwidth * noc_opts.noc_aggregate_bandwidth_weighting + cost_terms.latency * norm_factors.latency * noc_opts.noc_latency_weighting + cost_terms.latency_overrun * norm_factors.latency_overrun * noc_opts.noc_latency_constraints_weighting + cost_terms.congestion * norm_factors.congestion * noc_opts.noc_congestion_weighting);

    return cost;
}

int get_number_of_traffic_flows_with_latency_cons_met() {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows& noc_traffic_flows_storage = noc_ctx.noc_traffic_flows_storage;

    int count_of_achieved_latency_cons = 0;

    // now go through each traffic flow route and check if its latency constraint was met
    for (const auto& traffic_flow_id : noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);

        // there will always be one more router than links in a traffic flow
        int num_of_links_in_traffic_flow = curr_traffic_flow_route.size();
        int num_of_routers_in_traffic_flow = num_of_links_in_traffic_flow + 1;
        double max_latency = curr_traffic_flow.max_traffic_flow_latency;

        // latencies of the noc
        double noc_link_latency = noc_ctx.noc_model.get_noc_link_latency();
        double noc_router_latency = noc_ctx.noc_model.get_noc_router_latency();

        // calculate the traffic flow_latency
        double latency = (noc_link_latency * num_of_links_in_traffic_flow) + (noc_router_latency * num_of_routers_in_traffic_flow);

        // we check whether the latency constraint was met here
        if ((std::max(0., latency - max_latency)) < MIN_EXPECTED_NOC_LATENCY_COST) {
            count_of_achieved_latency_cons++;
        }
    }

    return count_of_achieved_latency_cons;
}

int get_number_of_congested_noc_links() {
    // get NoC links
    auto& noc_links = g_vpr_ctx.noc().noc_model.get_noc_links();

    int num_congested_links = 0;

    // Iterate over all NoC links and count the congested ones
    for (const auto& link : noc_links) {
        double congested_bw_ratio = link.get_congested_bandwidth_ratio();

        if (congested_bw_ratio > MIN_EXPECTED_NOC_CONGESTION_COST) {
            num_congested_links++;
        }
    }

    return num_congested_links;
}

double get_total_congestion_bandwidth_ratio() {
    // get NoC links
    auto& noc_links = g_vpr_ctx.noc().noc_model.get_noc_links();

    double accum_congestion_ratio = 0.0;

    // Iterate over all NoC links and count the congested ones
    for (const auto& link : noc_links) {
        double congested_bw_ratio = link.get_congested_bandwidth_ratio();
        accum_congestion_ratio += congested_bw_ratio;
    }

    return accum_congestion_ratio;
}

std::vector<NocLink> get_top_n_congested_links(int n) {
    // get NoC links
    vtr::vector<NocLinkId, NocLink> noc_links = g_vpr_ctx.noc().noc_model.get_noc_links();

    // Sort links based on their congested bandwidth ration in descending order
    // stable_sort is used to make sure the order is the same across different machines/compilers
    // Note that when the vector is sorted, indexing it with NocLinkId does return the corresponding link
    std::stable_sort(noc_links.begin(), noc_links.end(), [](const NocLink& l1, const NocLink& l2) {
        return l1.get_congested_bandwidth_ratio() > l2.get_congested_bandwidth_ratio();
    });

    int pick_n = std::min((int)noc_links.size(), n);

    return std::vector<NocLink>{noc_links.begin(), noc_links.begin() + pick_n};
}

void allocate_and_load_noc_placement_structs() {
    auto& noc_ctx = g_vpr_ctx.noc();

    int number_of_traffic_flows = noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows();

    traffic_flow_costs.resize(number_of_traffic_flows, {INVALID_NOC_COST_TERM, INVALID_NOC_COST_TERM});
    proposed_traffic_flow_costs.resize(number_of_traffic_flows, {INVALID_NOC_COST_TERM, INVALID_NOC_COST_TERM});

    int number_of_noc_links = noc_ctx.noc_model.get_number_of_noc_links();

    link_congestion_costs.resize(number_of_noc_links, INVALID_NOC_COST_TERM);
    proposed_link_congestion_costs.resize(number_of_noc_links, INVALID_NOC_COST_TERM);
}

void free_noc_placement_structs() {
    vtr::release_memory(traffic_flow_costs);
    vtr::release_memory(proposed_traffic_flow_costs);
    vtr::release_memory(affected_traffic_flows);

    vtr::release_memory(link_congestion_costs);
    vtr::release_memory(proposed_link_congestion_costs);
    vtr::release_memory(affected_noc_links);
}

/* Below are functions related to the feature that forces to the placer to swap router blocks for a certain percentage of the total number of swaps */
bool check_for_router_swap(int user_supplied_noc_router_swap_percentage) {
    /* A random number between 0-100 is generated here and compared to the user
     * supplied value. If the random number is less than the user supplied
     * value we indicate that a router block should be swapped. By doing this
     * we now only swap router blocks for the percentage of time the user
     * supplied.
     * */
    return (vtr::irand(99) < user_supplied_noc_router_swap_percentage);
}

static bool select_random_router_cluster(ClusterBlockId& b_from, t_pl_loc& from, t_logical_block_type_ptr& cluster_from_type) {
    // need to access all the router cluster blocks in the design
    auto& noc_ctx = g_vpr_ctx.noc();
    //
    auto& place_ctx = g_vpr_ctx.placement();
    //
    auto& cluster_ctx = g_vpr_ctx.clustering();

    // get a reference to the collection of router cluster blocks in the design
    const std::vector<ClusterBlockId>& router_clusters = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    // if there are no router cluster blocks, return false
    if (router_clusters.empty()) {
        return false;
    }

    const int number_of_router_blocks = router_clusters.size();

    //randomly choose a router block to move
    const int random_cluster_block_index = vtr::irand(number_of_router_blocks - 1);
    b_from = router_clusters[random_cluster_block_index];

    //check if the block is movable
    if (place_ctx.block_locs[b_from].is_fixed) {
        return false;
    }

    from = place_ctx.block_locs[b_from].loc;
    cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    return true;
}

e_create_move propose_router_swap(t_pl_blocks_to_be_moved& blocks_affected, float rlim) {
    // block ID for the randomly selected router cluster
    ClusterBlockId b_from;
    // current location of the randomly selected router cluster
    t_pl_loc from;
    // logical block type of the randomly selected router cluster
    t_logical_block_type_ptr cluster_from_type;
    bool random_select_success = false;

    // Randomly select a router cluster
    random_select_success = select_random_router_cluster(b_from, from, cluster_from_type);

    // If a random router cluster could not be selected, no move can be proposed
    if (!random_select_success) {
        return e_create_move::ABORT;
    }

    // now choose a compatible block to swap with
    t_pl_loc to;
    to.layer = from.layer;
    if (!find_to_loc_uniform(cluster_from_type, rlim, from, to, b_from)) {
        return e_create_move::ABORT;
    }

    e_create_move create_move = ::create_move(blocks_affected, b_from, to);

    //Check that all the blocks affected by the move would still be in a legal floorplan region after the swap
    if (!floorplan_legal(blocks_affected)) {
        return e_create_move::ABORT;
    }

    return create_move;
}

void write_noc_placement_file(const std::string& file_name) {
    // we need the clustered netlist to get the names of all the NoC router cluster blocks
    auto& cluster_ctx = g_vpr_ctx.clustering();
    // we need to the placement context to determine the final placed locations of the NoC router cluster blocks
    auto& placement_ctx = g_vpr_ctx.placement();
    // we need the NoC context to identify the physical router ids based on their locations on the device
    auto& noc_ctx = g_vpr_ctx.noc();

    // file to write the placement information to
    std::fstream noc_placement_file;

    // open file for write and check if it opened correctly
    noc_placement_file.open(file_name.c_str(), std::ios::out);
    if (!noc_placement_file) {
        VTR_LOG_ERROR(
            "Failed to open the placement file '%s' to write out the NoC router placement information.\n",
            file_name.c_str());
    }

    // assume that the FPGA device has a single layer (2-D), so when we write the placement file the layer value will be constant
    int layer_number = 0;

    // get a reference to the collection of router cluster blocks in the design
    const std::vector<ClusterBlockId>& router_clusters = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();
    //get the noc model to determine the physical routers where clusters are placed
    const NocStorage& noc_model = noc_ctx.noc_model;

    // get a reference to the clustered netlist
    const ClusteredNetlist& cluster_block_netlist = cluster_ctx.clb_nlist;

    // get a reference to the clustered block placement locations
    const vtr::vector_map<ClusterBlockId, t_block_loc>& clustered_block_placed_locations = placement_ctx.block_locs;

    // go through all the cluster blocks and write out their information in the placement file
    for (const auto& single_cluster_id : router_clusters) {
        // check if the current cluster id is valid
        if (single_cluster_id == ClusterBlockId::INVALID()) {
            VTR_LOG_ERROR(
                "A cluster block id stored as an identifier for a NoC router block was invalid.\n");
        }

        // get the name of the router cluster block
        const std::string& cluster_name = cluster_block_netlist.block_name(single_cluster_id);

        //get the placement location of the current router cluster block
        const t_block_loc& cluster_location = clustered_block_placed_locations[single_cluster_id];

        // now get the corresponding physical router block id the cluster is located on
        NocRouterId physical_router_cluster_is_placed_on = noc_model.get_router_at_grid_location(cluster_location.loc);

        // write the current cluster information to the output file
        noc_placement_file << cluster_name << " " << layer_number << " " << (size_t)physical_router_cluster_is_placed_on << "\n";
    }

    // finished writing placement information so close the file
    noc_placement_file.close();
}

bool noc_routing_has_cycle() {
    // used to access traffic flow routes
    const auto& noc_ctx = g_vpr_ctx.noc();
    // get all traffic flow routes
    const auto& traffic_flow_routes = noc_ctx.noc_traffic_flows_storage.get_all_traffic_flow_routes();

    bool has_cycle = noc_routing_has_cycle(traffic_flow_routes);

    return has_cycle;
}

bool noc_routing_has_cycle(const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& routes) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& place_ctx = g_vpr_ctx.placement();

    ChannelDependencyGraph channel_dependency_graph(noc_ctx.noc_model,
                                                    noc_ctx.noc_traffic_flows_storage,
                                                    routes,
                                                    place_ctx.block_locs);

    bool has_cycles = channel_dependency_graph.has_cycles();

    return has_cycles;
}

#ifdef ENABLE_NOC_SAT_ROUTING
void invoke_sat_router(t_placer_costs& costs, const t_noc_opts& noc_opts, int seed) {

    auto traffic_flow_routes = noc_sat_route(true, noc_opts, seed);

    if (!traffic_flow_routes.empty()) {
        bool has_cycle = noc_routing_has_cycle(traffic_flow_routes);
        if (has_cycle) {
            VTR_LOG("SAT NoC routing has cycles.\n");
        }

        reinitialize_noc_routing(costs, traffic_flow_routes);

        print_noc_costs("\nNoC Placement Costs after SAT routing", costs, noc_opts);

    } else {
        VTR_LOG("SAT routing failed.\n");
    }
}
#endif

void print_noc_costs(std::string_view header, const t_placer_costs& costs, const t_noc_opts& noc_opts) {
    VTR_LOG("%s. "
        "cost: %g, "
        "aggregate_bandwidth_cost: %g, "
        "latency_cost: %g, "
        "n_met_latency_constraints: %d, "
        "latency_overrun_cost: %g, "
        "congestion_cost: %g, "
        "accum_congested_ratio: %g, "
        "n_congested_links: %d \n",
        header.data(),
        calculate_noc_cost(costs.noc_cost_terms, costs.noc_cost_norm_factors, noc_opts),
        costs.noc_cost_terms.aggregate_bandwidth,
        costs.noc_cost_terms.latency,
        get_number_of_traffic_flows_with_latency_cons_met(),
        costs.noc_cost_terms.latency_overrun,
        costs.noc_cost_terms.congestion,
        get_total_congestion_bandwidth_ratio(),
        get_number_of_congested_noc_links());
}

static std::vector<NocLinkId> find_affected_links_by_flow_reroute(std::vector<NocLinkId>& prev_links,
                                                                  std::vector<NocLinkId>& curr_links) {
    // Sort both link containers
    std::stable_sort(prev_links.begin(), prev_links.end());
    std::stable_sort(curr_links.begin(), curr_links.end());

    // stores links that appear either in prev_links or curr_links but not both of them
    std::vector<NocLinkId> unique_links;

    // find links that are unique to prev_links
    std::set_difference(prev_links.begin(), prev_links.end(),
                        curr_links.begin(), curr_links.end(),
                        std::back_inserter(unique_links));

    // find links that are unique to curr_links
    std::set_difference(curr_links.begin(), curr_links.end(),
                        prev_links.begin(), prev_links.end(),
                        std::back_inserter(unique_links));

    return unique_links;
}