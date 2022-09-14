
#include "noc_place_utils.h"

/********************** Variables local to noc_place_utils.c pp***************************/
/* Cost of a noc traffic flow, and a temporary cost of a noc traffic flow
during move assessment*/
static vtr::vector<NocTrafficFlowId, double> traffic_flow_aggregate_bandwidth_cost, proposed_traffic_flow_aggregate_bandwidth_cost, traffic_flow_latency_cost, proposed_traffic_flow_latency_cost;

/* Keeps track of traffic flows that have been updated at each placement iteration*/
static std::vector<NocTrafficFlowId> affected_traffic_flows;
/******************************************************************/

void initial_noc_placement(void){

    // need to get placement information about where the router cluster blocks are palced on the device
    const auto& place_ctx = g_vpr_ctx.placement();

    // need to update the link usages within after routing all the traffic flows
    // also need to route all the traffic flows ans store them
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    /* We need all the traffic flow ids to be able to access them. The range
    of traffic flow ids go from 0 to the total number of traffic flows within
    within the NoC. So get the upper range here.*/
    int number_of_traffic_flows = noc_traffic_flows_storage->get_number_of_traffic_flows();

    // go through all the traffic flows and route them. Then once routed, update the links used in the routed traffic flows with their usages
    for(int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){
        
        // get the traffic flow with the current id
        NocTrafficFlowId conv_traffic_flow_id = (NocTrafficFlowId)traffic_flow_id;
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(conv_traffic_flow_id);
 
        // go and update the traffic flow route based on where the router cluster blocks are placed
        std::vector<NocLinkId>& curr_traffic_flow_route = get_traffic_flow_route(conv_traffic_flow_id, noc_ctx.noc_model, *noc_traffic_flows_storage, noc_ctx.noc_flows_router, place_ctx.block_locs);

        // update the links used in the found traffic flow route
        update_traffic_flow_link_usage(curr_traffic_flow_route, noc_ctx.noc_model, link_usage_update_state::increment, curr_traffic_flow.traffic_flow_bandwidth);

    }

    return;

}

int find_affected_noc_routers_and_update_noc_costs(const t_pl_blocks_to_be_moved& blocks_affected, double& noc_aggregate_bandwidth_delta_c, double& noc_latency_delta_c){
    // provides the positions where the affected blocks have moved to
    auto& place_ctx = g_vpr_ctx.placement();
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    // keeps track of traffic flows that have been re-routed
    // This is useful for cases where two moved routers were part of the same traffic flow and prevents us from re-routing the same flow twice.
    std::unordered_set<NocTrafficFlowId> updated_traffic_flows;

    int number_of_affected_traffic_flows = 0;

    // go through the moved blocks and process them only if they are NoC routers
    for (int iblk = 0; iblk < blocks_affected.num_moved_blocks; ++iblk) {
        ClusterBlockId blk = blocks_affected.moved_blocks[iblk].block_num;

        // check if the current moved block is a noc router
        if (noc_traffic_flows_storage->check_if_cluster_block_has_traffic_flows(blk)){
            // current block is a router, so re-route all the traffic flows it is a part of
            re_route_associated_traffic_flows(blk, *noc_traffic_flows_storage, noc_ctx.noc_model, noc_ctx.noc_flows_router, place_ctx.block_locs, updated_traffic_flows, number_of_affected_traffic_flows);
        }

    }

    // parameters needed to calculate latency costs in traffic flows
    double noc_link_latency = noc_ctx.noc_model.get_noc_link_latency();
    double noc_router_latency = noc_ctx.noc_model.get_noc_router_latency();


    // go through all the affected traffic flows and calculate their new costs after being re-routed, then determine the change in cost before the traffic flows were modified
    for (int traffic_flow_affected = 0; traffic_flow_affected < number_of_affected_traffic_flows; traffic_flow_affected++){

        NocTrafficFlowId traffic_flow_id = affected_traffic_flows[traffic_flow_affected];
        // get the traffic flow route
        const std::vector<NocLinkId>& traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route(traffic_flow_id);

        // get the current traffic flow info
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_id);

        proposed_traffic_flow_aggregate_bandwidth_cost[traffic_flow_id] = calculate_traffic_flow_aggregate_bandwidth(traffic_flow_route, curr_traffic_flow.traffic_flow_bandwidth);
        proposed_traffic_flow_latency_cost[traffic_flow_id] = calculate_traffic_flow_latency(traffic_flow_route, noc_router_latency, noc_link_latency);

        noc_aggregate_bandwidth_delta_c += proposed_traffic_flow_aggregate_bandwidth_cost[traffic_flow_id] - traffic_flow_aggregate_bandwidth_cost[traffic_flow_id];
        noc_latency_delta_c += proposed_traffic_flow_latency_cost[traffic_flow_id] - traffic_flow_latency_cost[traffic_flow_id];

    }

    return number_of_affected_traffic_flows;

}

void commit_noc_costs(int number_of_affected_traffic_flows){

    for (int traffic_flow_affected = 0; traffic_flow_affected < number_of_affected_traffic_flows; traffic_flow_affected++){

        NocTrafficFlowId traffic_flow_id = affected_traffic_flows[traffic_flow_affected];

        // update the traffic flow costs
        traffic_flow_aggregate_bandwidth_cost[traffic_flow_id] = proposed_traffic_flow_aggregate_bandwidth_cost[traffic_flow_id];
        traffic_flow_latency_cost[traffic_flow_id] = proposed_traffic_flow_latency_cost[traffic_flow_id];

        // reset the proposed traffic flows costs
        proposed_traffic_flow_aggregate_bandwidth_cost[traffic_flow_id] = -1;
        proposed_traffic_flow_latency_cost[traffic_flow_id] = -1;

    }

    return;
}

std::vector<NocLinkId>& get_traffic_flow_route(NocTrafficFlowId traffic_flow_id, const NocStorage& noc_model, NocTrafficFlows& noc_traffic_flows_storage, NocRouting* noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations){

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
    noc_flows_router->route_flow(source_router_block_id, sink_router_block_id, curr_traffic_flow_route, noc_model);


    return curr_traffic_flow_route;

}

void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route, NocStorage& noc_model, link_usage_update_state how_to_update_links, double traffic_flow_bandwidth){

    // go through the links within the traffic flow route and update their bandwidht usage
    for (auto& link_in_route_id : traffic_flow_route){
        
        // get the link to update and its current bandwdith
        NocLink& curr_link = noc_model.get_single_mutable_noc_link(link_in_route_id);
        double curr_link_bandwidth = curr_link.get_bandwidth_usage();

        // update the link badnwidth based on the user provided state
        switch (how_to_update_links) {
            case link_usage_update_state::increment:
                curr_link.set_bandwidth_usage(curr_link_bandwidth + traffic_flow_bandwidth);
                break;
            case link_usage_update_state::decrement:
                curr_link.set_bandwidth_usage(curr_link_bandwidth - traffic_flow_bandwidth);
                break;
            default:
                break;
        }

        // check that the bandwidth never goes to negative
        VTR_ASSERT(curr_link.get_bandwidth_usage() >= 0.0);
  
    }

    return;

}

void re_route_associated_traffic_flows(ClusterBlockId moved_block_router_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting* noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations, std::unordered_set<NocTrafficFlowId>& updated_traffic_flows, int& number_of_affected_traffic_flows){

    // get all the associated traffic flows for the logical router cluster block
    const std::vector<NocTrafficFlowId>* assoc_traffic_flows = noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(moved_block_router_id);

    // now check if there are any associated traffic flows
    if (assoc_traffic_flows != nullptr){
        // There are traffic flows associated to the current router block so process them
        for(auto& traffic_flow_id : *assoc_traffic_flows) {
        
            // first check to see whether we have already re-routed the current traffic flow and only re-route it if we haven't already.
            if (updated_traffic_flows.find(traffic_flow_id) == updated_traffic_flows.end()){

                // now update the current traffic flow by re-routing it based on the new locations of its src and destination routers
                re_route_traffic_flow(traffic_flow_id, noc_traffic_flows_storage, noc_model, noc_flows_router, placed_cluster_block_locations);

                // now make sure we don't update this traffic flow a second time by adding it to the group of updated traffic flows
                updated_traffic_flows.insert(traffic_flow_id);

                // update global datastructures to indicate that the current traffic flow was affected due to router cluster blocks being swapped
                affected_traffic_flows[number_of_affected_traffic_flows] = traffic_flow_id;
                number_of_affected_traffic_flows++;
            }

        }
    }

    return;

}

void revert_noc_traffic_flow_routes(const t_pl_blocks_to_be_moved& blocks_affected){

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
        if (noc_traffic_flows_storage->check_if_cluster_block_has_traffic_flows(blk)){
            // current block is a router, so re-route all the traffic flows it is a part of //

            // get all the associated traffic flows for the logical router cluster block
            const std::vector<NocTrafficFlowId>* assoc_traffic_flows = noc_traffic_flows_storage->get_traffic_flows_associated_to_router_block(blk);

            // now check if there are any associated traffic flows
            if (assoc_traffic_flows != nullptr){
                // There are traffic flows associated to the current router block so process them
                for(auto& traffic_flow_id : *assoc_traffic_flows) {

                    // first check to see whether we have already reverted the current traffic flow and only revert it if we haven't already.
                    if (reverted_traffic_flows.find(traffic_flow_id) == reverted_traffic_flows.end()){
                        // Revert the traffic flow route by re-routing it
                        re_route_traffic_flow(traffic_flow_id, *noc_traffic_flows_storage, noc_ctx.noc_model, noc_ctx.noc_flows_router, place_ctx.block_locs);

                        // make sure we do not revert this traffic flow again
                        reverted_traffic_flows.insert((NocTrafficFlowId)traffic_flow_id); 
                    }
                
                }   
            }
        }
    }

    return;

}

void re_route_traffic_flow(NocTrafficFlowId traffic_flow_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting* noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations){

    // get the current traffic flow info
    const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_id);
                        
    /*  since the current traffic flow route will be 
        changed, first we need to reduce the bandwidh
        usage of all links that are part of
        the existing traffic flow route
    */
    const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage.get_traffic_flow_route(traffic_flow_id);
    update_traffic_flow_link_usage(curr_traffic_flow_route, noc_model, link_usage_update_state::decrement, curr_traffic_flow.traffic_flow_bandwidth);


    // now get the re-routed traffic flow route and update all the link usages with this reverted route
    std::vector<NocLinkId>& re_routed_traffic_flow_route = get_traffic_flow_route((NocTrafficFlowId)traffic_flow_id, noc_model, noc_traffic_flows_storage, noc_flows_router, placed_cluster_block_locations);
    update_traffic_flow_link_usage(re_routed_traffic_flow_route, noc_model, link_usage_update_state::increment, curr_traffic_flow.traffic_flow_bandwidth);

    return;

}

void recompute_noc_costs(double *new_noc_aggregate_bandwidth_cost, double *new_noc_latency_cost){

    int number_of_traffic_flows = g_vpr_ctx.noc().noc_traffic_flows_storage.get_number_of_traffic_flows();

    // go through the costs of all the traffic flows and add them up to recompute the total costs associated with the NoC
    for (int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){

        *new_noc_aggregate_bandwidth_cost += traffic_flow_aggregate_bandwidth_cost[(NocTrafficFlowId)traffic_flow_id];
        *new_noc_latency_cost += traffic_flow_latency_cost[(NocTrafficFlowId)traffic_flow_id];

    }

    return;

}

double comp_noc_aggregate_bandwidth_cost(void){

    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    int number_of_traffic_flows = noc_traffic_flows_storage->get_number_of_traffic_flows();

    double noc_aggregate_bandwidth_cost = 0.;

    // now go through each traffic flow route and calculate its
    // aggregate bandwidth. Then store this in local datastrucutres and accumulate it.
    for (int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){

        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_id);
        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route((NocTrafficFlowId)traffic_flow_id);

        double curr_traffic_flow_aggregate_bandwidth_cost = calculate_traffic_flow_aggregate_bandwidth(curr_traffic_flow_route, curr_traffic_flow.traffic_flow_bandwidth);

        // store the calculated aggregate bandwidth for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_aggregate_bandwidth_cost[(NocTrafficFlowId)traffic_flow_id] = curr_traffic_flow_aggregate_bandwidth_cost;

        // accumumulate the aggregate bandwidth cost
        noc_aggregate_bandwidth_cost += curr_traffic_flow_aggregate_bandwidth_cost;
    }

    return noc_aggregate_bandwidth_cost;

}

double comp_noc_latency_cost(void){

    // used to get traffic flow route information
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    // datastructure that stores all the traffic flow routes
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;

    int number_of_traffic_flows = noc_traffic_flows_storage->get_number_of_traffic_flows();

    double noc_latency_cost = 0.;
    
    // get latency properties of the NoC
    double noc_router_latency = noc_ctx.noc_model.get_noc_router_latency();
    double noc_link_latency = noc_ctx.noc_model.get_noc_link_latency();

    // now go through each traffic flow route and calculate its
    // latency. Then store this in local datastrucutres and accumulate it.
    for (int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){

        const std::vector<NocLinkId>& curr_traffic_flow_route = noc_traffic_flows_storage->get_traffic_flow_route((NocTrafficFlowId)traffic_flow_id);

        double curr_traffic_flow_latency_cost = calculate_traffic_flow_latency(curr_traffic_flow_route, noc_router_latency, noc_link_latency);

        // store the calculated latency for the current traffic flow in local datastructures (this also initializes them)
        traffic_flow_latency_cost[(NocTrafficFlowId)traffic_flow_id] = curr_traffic_flow_latency_cost;

        // accumumulate the aggregate bandwidth cost
        noc_latency_cost += curr_traffic_flow_latency_cost;;
    }

    return noc_latency_cost;

}

int check_noc_placement_costs(const t_placer_costs& costs, double error_tolerance){

    int error = 0;
    double noc_aggregate_bandwidth_cost_check = 0.;
    double noc_latency_cost_check = 0.;

    // get current router block locations
    auto& place_ctx = g_vpr_ctx.placement();
    const vtr::vector_map<ClusterBlockId, t_block_loc>* placed_cluster_block_locations = &place_ctx.block_locs;

    auto& noc_ctx = g_vpr_ctx.noc();
    const NocStorage* noc_model = &noc_ctx.noc_model;
    const NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;
    NocRouting* noc_flows_router = noc_ctx.noc_flows_router;

    int number_of_traffic_flows = noc_traffic_flows_storage->get_number_of_traffic_flows();

    // stores a temporarily found route for a traffic flow
    std::vector<NocLinkId> temp_found_noc_route;

    // local parameters needed to calculate latency of traffic flows
    double noc_link_latency = noc_model->get_noc_link_latency();
    double noc_router_latency = noc_model->get_noc_router_latency();

    // go through all the traffic flows and find route for them based on where the routers are placed within the NoC
    for (int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){

        // get the traffic flow with the current id
        const t_noc_traffic_flow& curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_id); 

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_block_id = noc_model->get_router_at_grid_location((*placed_cluster_block_locations)[logical_source_router_block_id].loc);
        NocRouterId sink_router_block_id = noc_model->get_router_at_grid_location((*placed_cluster_block_locations)[logical_sink_router_block_id].loc);

        // route the current traffic flow
        noc_flows_router->route_flow(source_router_block_id, sink_router_block_id, temp_found_noc_route, *noc_model);

        // now calculate the costs associated to the current traffic flow and accumulate it to find the total cost of the NoC placement
        traffic_flow_aggregate_bandwidth_cost[(NocTrafficFlowId)traffic_flow_id] = calculate_traffic_flow_aggregate_bandwidth(temp_found_noc_route, curr_traffic_flow.traffic_flow_bandwidth);
        noc_aggregate_bandwidth_cost_check +=  traffic_flow_aggregate_bandwidth_cost[(NocTrafficFlowId)traffic_flow_id];

        traffic_flow_latency_cost[(NocTrafficFlowId)traffic_flow_id] = calculate_traffic_flow_latency(temp_found_noc_route, noc_router_latency, noc_link_latency);
        noc_latency_cost_check +=  traffic_flow_latency_cost[(NocTrafficFlowId)traffic_flow_id];

        // clear the current traffic flow route so we can route the next traffic flow
        temp_found_noc_route.clear();

    }

    // check whether the aggregate bandwidth placement cost is within the error tolerance
    if (fabs(noc_aggregate_bandwidth_cost_check - costs.noc_aggregate_bandwidth_cost) > costs.noc_aggregate_bandwidth_cost * error_tolerance) {
        VTR_LOG_ERROR(
            "noc_aggregate_bandwidth_cost_check: %g and noc_aggregate_bandwidth_cost: %g differ in check_noc_placement_costs.\n",
            noc_aggregate_bandwidth_cost_check, costs.noc_aggregate_bandwidth_cost);
        error++;
    }

    // check whether the latency placement cost is within the error tolerance
    if (fabs(noc_latency_cost_check - costs.noc_latency_cost) > costs.noc_latency_cost * error_tolerance) {
        VTR_LOG_ERROR(
            "noc_latency_cost_check: %g and noc_latency_cost: %g differ in check_noc_placement_costs.\n",
            noc_latency_cost_check, costs.noc_latency_cost);
        error++;
    }

    return error;

}

double calculate_traffic_flow_aggregate_bandwidth(const std::vector<NocLinkId>& traffic_flow_route, double traffic_flow_bandwidth){

    int num_of_links_in_traffic_flow = traffic_flow_route.size();
    
    return num_of_links_in_traffic_flow * traffic_flow_bandwidth;

}

double calculate_traffic_flow_latency(const std::vector<NocLinkId>& traffic_flow_route, double noc_router_latency, double noc_link_latency){
    
    // there will always be one more router than links in a traffic flow
    int num_of_links_in_traffic_flow = traffic_flow_route.size();   
    int num_of_routers_in_traffic_flow = num_of_links_in_traffic_flow + 1;

    return ((noc_link_latency*num_of_links_in_traffic_flow) + (noc_router_latency*num_of_routers_in_traffic_flow));
}

void allocate_and_load_noc_placement_structs(void){

    auto& noc_ctx = g_vpr_ctx.noc();

    int number_of_traffic_flows = noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows();

    traffic_flow_aggregate_bandwidth_cost.resize(number_of_traffic_flows, -1);
    proposed_traffic_flow_aggregate_bandwidth_cost.resize(number_of_traffic_flows, -1);
    traffic_flow_latency_cost.resize(number_of_traffic_flows, -1);
    proposed_traffic_flow_latency_cost.resize(number_of_traffic_flows, -1);

    affected_traffic_flows.resize(number_of_traffic_flows, NocTrafficFlowId::INVALID());

    return;
}

void free_noc_placement_structs(void){

    vtr::release_memory(traffic_flow_aggregate_bandwidth_cost);
    vtr::release_memory(proposed_traffic_flow_aggregate_bandwidth_cost);
    vtr::release_memory(traffic_flow_latency_cost);
    vtr::release_memory(proposed_traffic_flow_latency_cost);
    vtr::release_memory(affected_traffic_flows);

    return;
}