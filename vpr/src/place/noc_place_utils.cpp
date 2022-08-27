
#include "noc_place_utils.h"


void initial_noc_placement(void){

    // need to get placement information about where the router cluster blocks are palced on the device
    const auto& place_ctx = g_vpr_ctx.placement();

    // need to update the link usages within after routing all the traffic flows
    // also need to route all the traffic flows ans store them
    auto& noc_ctx = g_vpr_ctx.mutable_noc();

    NocStorage* noc_model = &noc_ctx.noc_model;
    NocTrafficFlows* noc_traffic_flows_storage = &noc_ctx.noc_traffic_flows_storage;
    NocRouting* noc_flows_router = noc_ctx.noc_flows_router;

    const vtr::vector_map<ClusterBlockId, t_block_loc>* placed_cluster_block_locations = &place_ctx.block_locs;

    /* We need all the traffic flow ids to be able to access them. The range
    of traffic flow ids go from 0 to the total number of traffic flows within
    within the NoC. So get the upper range here.*/
    int number_of_traffic_flows = noc_traffic_flows_storage->get_number_of_traffic_flows();

    // go through all the traffic flows and route them. Then once routed, update the links used in the routed traffic flows with their usages
    for(int traffic_flow_id = 0; traffic_flow_id < number_of_traffic_flows; traffic_flow_id++){
        
        // get the traffic flow with the current id
        NocTrafficFlowId conv_traffic_flow_id = (NocTrafficFlowId)traffic_flow_id;
        const t_noc_traffic_flow curr_traffic_flow = noc_traffic_flows_storage->get_single_noc_traffic_flow(conv_traffic_flow_id); 

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = curr_traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = curr_traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_block_id = get_router_block_id_where_cluster_block_is_placed(logical_source_router_block_id, placed_cluster_block_locations, *noc_model);
        NocRouterId sink_router_block_id = get_router_block_id_where_cluster_block_is_placed(logical_sink_router_block_id, placed_cluster_block_locations, *noc_model);

        // route the current traffic flow
        std::vector<NocLinkId> curr_traffic_flow_route = noc_traffic_flows_storage->get_mutable_traffic_flow_route(conv_traffic_flow_id);
        noc_flows_router->route_flow(source_router_block_id, sink_router_block_id, curr_traffic_flow_route, *noc_model);

        // update the links used in the found traffic flow route
        update_traffic_flow_link_usage(curr_traffic_flow_route, *noc_model, link_usage_update_state::increment, curr_traffic_flow.traffic_flow_bandwidth);

    }

    return;

}

NocRouterId get_router_block_id_where_cluster_block_is_placed(ClusterBlockId cluster_blk_id, const vtr::vector_map<ClusterBlockId, t_block_loc>* placed_cluster_block_locations, const NocStorage& noc_model){

    // start by getting the the grid location of the provided logical router cluster block
    auto logical_router_block_grid_location_ref = placed_cluster_block_locations->find(cluster_blk_id);

    // quick check to make sure that the provided cluster block is placed on the FPGA device (should this be a vtr_fatal_error()??)
    VTR_ASSERT(logical_router_block_grid_location_ref != placed_cluster_block_locations->end());

    // now get the id of the hard router block at the found grid location
    NocRouterId hard_router_block_id = noc_model.get_router_at_grid_location(logical_router_block_grid_location_ref->loc);

    return hard_router_block_id;

}

void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route, NocStorage& noc_model, link_usage_update_state how_to_update_links, double traffic_flow_bandwidth){

    // go through the links within the traffic flow route and update their bandwidht usage
    for (auto& link_in_route_id : traffic_flow_route){
        
        // get the link and its bandwidth so we can update it
        NocLink link_in_route = noc_model.get_single_mutable_noc_link(link_in_route_id);
        double curr_link_bandwidth = link_in_route.get_bandwidth_usage();

        // update the link badnwidth based on the user provided state
        switch (how_to_update_links) {
            case link_usage_update_state::increment:
                link_in_route.set_bandwidth_usage(curr_link_bandwidth + traffic_flow_bandwidth);
                break;
            case link_usage_update_state::decrement:
                link_in_route.set_bandwidth_usage(curr_link_bandwidth - traffic_flow_bandwidth);
                break;
            default:
                break;
        }

        // check that the bandwidth never goes to negative
        VTR_ASSERT(link_in_route.get_bandwidth_usage() >= 0.0);
  
    }

    return;

}