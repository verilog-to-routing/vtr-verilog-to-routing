
#include "noc_place_utils.h"

/********************** Variables local to noc_place_utils.c pp***************************/
/* Proposed and actual cost of a noc traffic flow used for each move assessment */
static vtr::vector<NocTrafficFlowId, TrafficFlowPlaceCost> traffic_flow_costs, proposed_traffic_flow_costs;

/* Keeps track of traffic flows that have been updated at each attempted placement move*/
static std::vector<NocTrafficFlowId> affected_traffic_flows;
/*********************************************************** *****************************/

void initial_noc_routing(void) {
    // need to get placement information about where the router cluster blocks are placed on the device
    const auto& place_ctx = g_vpr_ctx.placement();

    // need to update the link usages within after routing all the traffic flows
    // also need to route all the traffic flows and store them
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    /* We need all the traffic flow ids to be able to access them. The range
     * of traffic flow ids go from 0 to the total number of traffic flows within
     * the NoC.
     * go through all the traffic flows and route them. Then once routed, update the links used in the routed traffic flows with their usages
     */
    for (const auto& traffic_flow_id : noc_traffic_flows_storage->get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);

        // update the traffic flow route based on where the router cluster blocks are placed
        std::vector<NocLinkId>& curr_traffic_flow_route = get_traffic_flow_route(traffic_flow_id, noc_ctx.noc_model, *noc_traffic_flows_storage, *noc_ctx.noc_flows_router, place_ctx.block_locs);

        // update the links used in the found traffic flow route, links' bandwidth should be incremented since the traffic flow is routed
        update_traffic_flow_link_usage(curr_traffic_flow_route, noc_ctx.noc_model, 1, curr_traffic_flow.traffic_flow_bandwidth);
    }

    return;
}

void reinitialize_noc_routing(const t_noc_opts& noc_opts, t_placer_costs& costs) {
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    // Zero out bandwidth usage for all links
    for (auto& noc_link : noc_ctx.noc_model.get_mutable_noc_links()) {
        noc_link.set_bandwidth_usage(0.0);
    }

    // Route traffic flows and update link bandwidth usage
    initial_noc_routing();

    // Initialize traffic_flow_costs
    costs.noc_aggregate_bandwidth_cost = comp_noc_aggregate_bandwidth_cost();
    costs.noc_latency_cost = comp_noc_latency_cost(noc_opts);
}

void find_affected_noc_routers_and_update_noc_costs(const t_pl_blocks_to_be_moved& blocks_affected, double& noc_aggregate_bandwidth_delta_c, double& noc_latency_delta_c, const t_noc_opts& noc_opts) {
    // provides the positions where the affected blocks have moved to
    auto& place_ctx = g_vpr_ctx.placement();
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    // keeps track of traffic flows that have been re-routed
    // This is useful for cases where two moved routers were part of the same traffic flow and prevents us from re-routing the same flow twice.
    std::unordered_set<NocTrafficFlowId> updated_traffic_flows;

    affected_traffic_flows.clear();

    // go through the moved blocks and process them only if they are NoC routers
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        // check if the current moved block is a noc router
        if (noc_traffic_flows_storage->check_if_cluster_block_has_traffic_flows(blk)) {
            // current block is a router, so re-route all the traffic flows it is a part of
            re_route_associated_traffic_flows(blk, *noc_traffic_flows_storage, noc_ctx.noc_model, *noc_ctx.noc_flows_router, place_ctx.block_locs, updated_traffic_flows);
        }
    }

    // go through all the affected traffic flows and calculate their new costs after being re-routed, then determine the change in cost before the traffic flows were modified
    for (auto& traffic_flow_id : affected_traffic_flows) {
        // get the traffic flow route
        const std::vector<NocLinkId>& traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route(traffic_flow_id);

        // get the current traffic flow info
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);

        proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = calculate_traffic_flow_aggregate_bandwidth_cost(traffic_flow_route, curr_traffic_flow);
        proposed_traffic_flow_costs[traffic_flow_id].latency = calculate_traffic_flow_latency_cost(traffic_flow_route, noc_ctx.noc_model, curr_traffic_flow, noc_opts);

        noc_aggregate_bandwidth_delta_c += proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth - traffic_flow_costs[traffic_flow_id].aggregate_bandwidth;
        noc_latency_delta_c += proposed_traffic_flow_costs[traffic_flow_id].latency - traffic_flow_costs[traffic_flow_id].latency;
    }
}

void commit_noc_costs() {
    for (auto& traffic_flow_id : affected_traffic_flows) {
        // update the traffic flow costs
        traffic_flow_costs[traffic_flow_id] = proposed_traffic_flow_costs[traffic_flow_id];

        // reset the proposed traffic flows costs
        proposed_traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = -1;
        proposed_traffic_flow_costs[traffic_flow_id].latency = -1;
    }

    return;
}

std::vector<NocLinkId>& get_traffic_flow_route(NocTrafficFlowId traffic_flow_id, const NocStorage& noc_model, NocTrafficFlows& noc_traffic_flows_storage, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations) {
    // get the traffic flow with the current id
    const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow_id);

    // get the source and destination logical router blocks in the current traffic flow
    ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
    ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

    // get the ids of the hard router blocks where the logical router cluster blocks have been placed
    NocRouterId source_router_block_id = noc_model.get_router_at_grid_location(placed_cluster_block_locations[logical_source_router_block_id].loc);
    NocRouterId sink_router_block_id = noc_model.get_router_at_grid_location(placed_cluster_block_locations[logical_sink_router_block_id].loc);

    // route the current traffic flow
    std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_mutable_traffic_flow_route(traffic_flow_id);
    noc_flows_router.route_flow(source_router_block_id, sink_router_block_id, curr_traffic_flow_route, noc_model);

    return curr_traffic_flow_route;
}

void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route, NocStorage& noc_model, int inc_or_dec, double traffic_flow_bandwidth) {
    // go through the links within the traffic flow route and update their bandwidth usage
    for (auto& link_in_route_id : traffic_flow_route) {
        // get the link to update and its current bandwidth
        NocLink& curr_link = noc_model.get_single_mutable_noc_link(link_in_route_id);
        double curr_link_bandwidth = curr_link.get_bandwidth_usage();

        curr_link.set_bandwidth_usage(curr_link_bandwidth + inc_or_dec * traffic_flow_bandwidth);

        // check that the bandwidth never goes to negative
        VTR_ASSERT(curr_link.get_bandwidth_usage() >= 0.0);
    }

    return;
}

void re_route_associated_traffic_flows(ClusterBlockId moved_block_router_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations, std::unordered_set<NocTrafficFlowId>& updated_traffic_flows) {
    // get all the associated traffic flows for the logical router cluster block
    const std::vector<NocTrafficFlowId>* assoc_traffic_flows = noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(moved_block_router_id);

    // now check if there are any associated traffic flows
    if (assoc_traffic_flows != nullptr) {
        // There are traffic flows associated to the current router block so process them
        for (auto& traffic_flow_id : *assoc_traffic_flows) {
            // first check to see whether we have already re-routed the current traffic flow and only re-route it if we haven't already.
            if (updated_traffic_flows.find(traffic_flow_id) == updated_traffic_flows.end()) {
                // now update the current traffic flow by re-routing it based on the new locations of its src and destination routers
                re_route_traffic_flow(traffic_flow_id, noc_traffic_flows_storage, noc_model, noc_flows_router, placed_cluster_block_locations);

                // now make sure we don't update this traffic flow a second time by adding it to the group of updated traffic flows
                updated_traffic_flows.insert(traffic_flow_id);

                // update global datastructures to indicate that the current traffic flow was affected due to router cluster blocks being swapped
                affected_traffic_flows.push_back(traffic_flow_id);
            }
        }
    }

    return;
}

void revert_noc_traffic_flow_routes(const t_pl_blocks_to_be_moved& blocks_affected) {
    // provides the positions where the affected blocks have moved to
    auto& place_ctx = g_vpr_ctx.placement();
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    // keeps track of traffic flows that have been reverted
    // This is useful for cases where two moved routers were part of the same traffic flow and prevents us from re-routing the same flow twice.
    std::unordered_set<NocTrafficFlowId> reverted_traffic_flows;

    // go through the moved blocks and process them only if they are NoC routers
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        // check if the current moved block is a noc router
        if (noc_traffic_flows_storage->check_if_cluster_block_has_traffic_flows(blk)) {
            // current block is a router, so re-route all the traffic flows it is a part of //

            // get all the associated traffic flows for the logical router cluster block
            const std::vector<NocTrafficFlowId>* assoc_traffic_flows = noc_traffic_flows_storage->get_traffic_flows_associated_to_router_block(blk);

            // now check if there are any associated traffic flows
            if (assoc_traffic_flows->size() != 0) {
                // There are traffic flows associated to the current router block so process them
                for (auto& traffic_flow_id : *assoc_traffic_flows) {
                    // first check to see whether we have already reverted the current traffic flow and only revert it if we haven't already.
                    if (reverted_traffic_flows.find(traffic_flow_id) == reverted_traffic_flows.end()) {
                        // Revert the traffic flow route by re-routing it
                        re_route_traffic_flow(traffic_flow_id, *noc_traffic_flows_storage, noc_ctx.noc_model, *noc_ctx.noc_flows_router, place_ctx.block_locs);

                        // make sure we do not revert this traffic flow again
                        reverted_traffic_flows.insert(traffic_flow_id);
                    }
                }
            }
        }
    }

    return;
}

void re_route_traffic_flow(NocTrafficFlowId traffic_flow_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations) {
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
    std::vector<NocLinkId>& re_routed_traffic_flow_route = get_traffic_flow_route(traffic_flow_id, noc_model, noc_traffic_flows_storage, noc_flows_router, placed_cluster_block_locations);
    update_traffic_flow_link_usage(re_routed_traffic_flow_route, noc_model, 1, curr_traffic_flow.traffic_flow_bandwidth);

    return;
}

void recompute_noc_costs(double& new_noc_aggregate_bandwidth_cost, double& new_noc_latency_cost) {
    // reset the cost variables first
    new_noc_aggregate_bandwidth_cost = 0;
    new_noc_latency_cost = 0;

    // go through the costs of all the traffic flows and add them up to recompute the total costs associated with the NoC
    for (const auto& traffic_flow_id : g_vpr_ctx.noc().noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        new_noc_aggregate_bandwidth_cost += traffic_flow_costs[traffic_flow_id].aggregate_bandwidth;
        new_noc_latency_cost += traffic_flow_costs[traffic_flow_id].latency;
    }

    return;
}

void update_noc_normalization_factors(t_placer_costs& costs) {
    //Prevent the norm factors from going to infinity
    costs.noc_aggregate_bandwidth_cost_norm = std::min(1 / costs.noc_aggregate_bandwidth_cost, MAX_INV_NOC_AGGREGATE_BANDWIDTH_COST);
    costs.noc_latency_cost_norm = std::min(1 / costs.noc_latency_cost, MAX_INV_NOC_LATENCY_COST);

    return;
}

double comp_noc_aggregate_bandwidth_cost(void) {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    double noc_aggregate_bandwidth_cost = 0.;

    // now go through each traffic flow route and calculate its
    // aggregate bandwidth. Then store this in local data structures and accumulate it.
    for (const auto& traffic_flow_id : g_vpr_ctx.noc().noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route(traffic_flow_id);

        double curr_traffic_flow_aggregate_bandwidth_cost = calculate_traffic_flow_aggregate_bandwidth_cost(curr_traffic_flow_route, curr_traffic_flow);

        // store the calculated aggregate bandwidth for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_costs[traffic_flow_id].aggregate_bandwidth = curr_traffic_flow_aggregate_bandwidth_cost;

        // accumulate the aggregate bandwidth cost
        noc_aggregate_bandwidth_cost += curr_traffic_flow_aggregate_bandwidth_cost;
    }

    return noc_aggregate_bandwidth_cost;
}

double comp_noc_latency_cost(const t_noc_opts& noc_opts) {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    double noc_latency_cost = 0.;

    // now go through each traffic flow route and calculate its
    // latency. Then store this in local data structures and accumulate it.
    for (const auto& traffic_flow_id : noc_ctx.noc_traffic_flows_storage.get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route(traffic_flow_id);

        double curr_traffic_flow_latency_cost = calculate_traffic_flow_latency_cost(curr_traffic_flow_route, noc_ctx.noc_model, curr_traffic_flow, noc_opts);

        // store the calculated latency for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_costs[traffic_flow_id].latency = curr_traffic_flow_latency_cost;

        // accumulate the aggregate bandwidth cost
        noc_latency_cost += curr_traffic_flow_latency_cost;
    }

    return noc_latency_cost;
}

int check_noc_placement_costs(const t_placer_costs& costs, double error_tolerance, const t_noc_opts& noc_opts) {
    int error = 0;
    double noc_aggregate_bandwidth_cost_check = 0.;
    double noc_latency_cost_check = 0.;

    // get current router block locations
    auto& place_ctx = g_vpr_ctx.placement();
    const vtr::vector_map<ClusterBlockId, t_block_loc>* placed_cluster_block_locations = &place_ctx.block_locs;

    auto& noc_ctx = g_vpr_ctx.noc();
    const NocStorage* noc_model = &noc_ctx.noc_model;
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    // need to create a temporary noc routing algorithm
    NocRoutingAlgorithmCreator routing_algorithm_factory;
    NocRouting* temp_noc_routing_algorithm = routing_algorithm_factory.create_routing_algorithm(noc_opts.noc_routing_algorithm);

    // stores a temporarily found route for a traffic flow
    std::vector<NocLinkId> temp_found_noc_route;

    // go through all the traffic flows and find a route for them based on where the routers are placed within the NoC
    for (const auto& traffic_flow_id : noc_traffic_flows_storage->get_all_traffic_flow_id()) {
        // get the traffic flow with the current id
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_block_id = noc_model->get_router_at_grid_location((*placed_cluster_block_locations)[logical_source_router_block_id].loc);
        NocRouterId sink_router_block_id = noc_model->get_router_at_grid_location((*placed_cluster_block_locations)[logical_sink_router_block_id].loc);

        // route the current traffic flow
        temp_noc_routing_algorithm->route_flow(source_router_block_id, sink_router_block_id, temp_found_noc_route, *noc_model);

        // now calculate the costs associated to the current traffic flow and accumulate it to find the total cost of the NoC placement
        double current_flow_aggregate_bandwidth_cost = calculate_traffic_flow_aggregate_bandwidth_cost(temp_found_noc_route, curr_traffic_flow);
        noc_aggregate_bandwidth_cost_check += current_flow_aggregate_bandwidth_cost;

        double current_flow_latency_cost = calculate_traffic_flow_latency_cost(temp_found_noc_route, *noc_model, curr_traffic_flow, noc_opts);
        noc_latency_cost_check += current_flow_latency_cost;

        // clear the current traffic flow route, so we can route the next traffic flow
        temp_found_noc_route.clear();
    }

    // check whether the aggregate bandwidth placement cost is within the error tolerance
    if (fabs(noc_aggregate_bandwidth_cost_check - costs.noc_aggregate_bandwidth_cost) > costs.noc_aggregate_bandwidth_cost * error_tolerance) {
        VTR_LOG_ERROR(
            "noc_aggregate_bandwidth_cost_check: %g and noc_aggregate_bandwidth_cost: %g differ in check_noc_placement_costs.\n",
            noc_aggregate_bandwidth_cost_check, costs.noc_aggregate_bandwidth_cost);
        error++;
    }

    // only check the recomputed cost if it is above our expected latency cost threshold of 1 pico-second, otherwise there is no point in checking it
    if (noc_latency_cost_check > MIN_EXPECTED_NOC_LATENCY_COST) {
        // check whether the latency placement cost is within the error tolerance
        if (fabs(noc_latency_cost_check - costs.noc_latency_cost) > costs.noc_latency_cost * error_tolerance) {
            VTR_LOG_ERROR(
                "noc_latency_cost_check: %g and noc_latency_cost: %g differ in check_noc_placement_costs.\n",
                noc_latency_cost_check, costs.noc_latency_cost);
            error++;
        }
    }
    // delete the temporary routing algorithm
    delete temp_noc_routing_algorithm;

    return error;
}

double calculate_traffic_flow_aggregate_bandwidth_cost(const std::vector<NocLinkId>& traffic_flow_route, const t_noc_traffic_flow& traffic_flow_info) {
    int num_of_links_in_traffic_flow = traffic_flow_route.size();

    // the traffic flow aggregate bandwidth cost is scaled by its priority, which dictates its importance to the placement
    return (traffic_flow_info.traffic_flow_priority * traffic_flow_info.traffic_flow_bandwidth * num_of_links_in_traffic_flow);
}

double calculate_traffic_flow_latency_cost(const std::vector<NocLinkId>& traffic_flow_route, const NocStorage& noc_model, const t_noc_traffic_flow& traffic_flow_info, const t_noc_opts& noc_opts) {
    // there will always be one more router than links in a traffic flow
    int num_of_links_in_traffic_flow = traffic_flow_route.size();
    int num_of_routers_in_traffic_flow = num_of_links_in_traffic_flow + 1;
    double max_latency = traffic_flow_info.max_traffic_flow_latency;

    // latencies of the noc
    double noc_link_latency = noc_model.get_noc_link_latency();
    double noc_router_latency = noc_model.get_noc_router_latency();

    // calculate the traffic flow_latency
    double latency = (noc_link_latency * num_of_links_in_traffic_flow) + (noc_router_latency * num_of_routers_in_traffic_flow);

    // calculate the cost
    double single_traffic_flow_latency_cost = (noc_opts.noc_latency_constraints_weighting * std::max(0., latency - max_latency)) + (noc_opts.noc_latency_weighting * latency);

    // scale the latency cost by its priority to indicate its importance
    return (single_traffic_flow_latency_cost * traffic_flow_info.traffic_flow_priority);
}

int get_number_of_traffic_flows_with_latency_cons_met(void) {
    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    int count_of_achieved_latency_cons = 0;

    // now go through each traffic flow route and check if its latency constraint was met
    for (const auto& traffic_flow_id : noc_traffic_flows_storage->get_all_traffic_flow_id()) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route(traffic_flow_id);

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

void allocate_and_load_noc_placement_structs(void) {
    auto& noc_ctx = g_vpr_ctx.noc();

    int number_of_traffic_flows = noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows();

    traffic_flow_costs.resize(number_of_traffic_flows);
    proposed_traffic_flow_costs.resize(number_of_traffic_flows);

    return;
}

void free_noc_placement_structs(void) {
    vtr::release_memory(traffic_flow_costs);
    vtr::release_memory(proposed_traffic_flow_costs);
    vtr::release_memory(affected_traffic_flows);

    return;
}

/* Below are functions related to the feature that forces to the placer to swap router blocks for a certain percentage of the total number of swaps */
bool check_for_router_swap(int user_supplied_noc_router_swap_percentage) {
    /* A random number between 0-100 is generated here and compared to the user
     * supplied value. If the random number is less than the user supplied
     * value we indicate that a router block should be swapped. By doing this
     * we now only swap router blocks for the percentage of time the user
     * supplied.
     * */
    return (vtr::irand(99) < user_supplied_noc_router_swap_percentage) ? true : false;
}

e_create_move propose_router_swap(t_pl_blocks_to_be_moved& blocks_affected, float rlim) {
    // need to access all the router cluster blocks in the design
    auto& noc_ctx = g_vpr_ctx.noc();
    // get a reference to the collection of router cluster blocks in the design
    const std::vector<ClusterBlockId>& router_clusters = noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist();

    // if there are no router cluster blocks to swap then abort
    if (router_clusters.empty()) {
        return e_create_move::ABORT;
    }

    int number_of_router_blocks = router_clusters.size();

    //randomly choose a router block to move
    int random_cluster_block_index = vtr::irand(number_of_router_blocks - 1);
    ClusterBlockId b_from = router_clusters[random_cluster_block_index];

    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //check if the block is movable
    if (place_ctx.block_locs[b_from].is_fixed) {
        return e_create_move::ABORT;
    }

    t_pl_loc from = place_ctx.block_locs[b_from].loc;
    auto cluster_from_type = cluster_ctx.clb_nlist.block_type(b_from);
    auto grid_from_type = g_vpr_ctx.device().grid.get_physical_type({from.x, from.y, from.layer});
    VTR_ASSERT(is_tile_compatible(grid_from_type, cluster_from_type));

    // now choose a compatible block to swap with
    t_pl_loc to;

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

void write_noc_placement_file(std::string file_name) {
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

    return;
}