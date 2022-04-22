
#include "noc_traffic_flows.h"
#include "vpr_error.h"

NocTrafficFlows::NocTrafficFlows(void){
    clear_traffic_flows();
}

// getters for the traffic flows

const t_noc_traffic_flow& NocTrafficFlows::get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const {

    return list_of_noc_traffic_flows[traffic_flow_id];
}

const std::vector<NocTrafficFlowId>* NocTrafficFlows::get_traffic_flows_associated_to_source_router(ClusterBlockId source_router_id) const {

    const std::vector<NocTrafficFlowId>* traffic_flow_list = nullptr;

    // get a reference to the list of traffic flows that have the current router as a source 
    auto source_router_traffic_flows_list = source_router_associated_traffic_flows.find(source_router_id);

    // check if there are any traffic flows associated with the current router
    if (source_router_traffic_flows_list != source_router_associated_traffic_flows.end()){

        // if we are here then there exists atleast 1 traffic flow that includes the current router as a source
        traffic_flow_list = &(source_router_traffic_flows_list->second);
    }

    return traffic_flow_list;
}

const std::vector<NocTrafficFlowId>* NocTrafficFlows::get_traffic_flows_associated_to_sink_router(ClusterBlockId source_router_id) const {

    const std::vector<NocTrafficFlowId>* traffic_flow_list = nullptr;

    // get a reference to the list of traffic flows that have the current router as a source 
    auto sink_router_traffic_flows_list = sink_router_associated_traffic_flows.find(source_router_id);

    // check if there are any traffic flows associated with the current router
    if (sink_router_traffic_flows_list != sink_router_associated_traffic_flows.end()){

        // if we are here then there exists atleast 1 traffic flow that includes the current router as a source
        traffic_flow_list = &(sink_router_traffic_flows_list->second);
    }

    return traffic_flow_list;
}

bool NocTrafficFlows::get_traffic_flow_processed_status(NocTrafficFlowId traffic_flow_id){

    return processed_traffic_flows[traffic_flow_id];
}

// setters for the traffic flows

void NocTrafficFlows::create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency){

    // create and add the new traffic flow to the list
    list_of_noc_traffic_flows.emplace_back(source_router_module_name, sink_router_module_name, source_router_cluster_id, sink_router_cluster_id, traffic_flow_bandwidth, traffic_flow_latency);

    //since the new traffic flow was added to the back of the list, its id will be the index of the last element
    NocTrafficFlowId curr_traffic_flow_id = (NocTrafficFlowId)(list_of_noc_traffic_flows.size() - 1);

    // add the source and sink routers to the set of unique router blocks in the design
    add_router_to_block_set(source_router_cluster_id);
    add_router_to_block_set(sink_router_cluster_id);

    // now add the new traffic flow to flows associated with the current source and sink router
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, source_router_cluster_id, this->source_router_associated_traffic_flows);
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, source_router_cluster_id, this->sink_router_associated_traffic_flows);

    return;

}

void NocTrafficFlows::set_traffic_flow_as_processed(NocTrafficFlowId traffic_flow_id){

    // status is initialized to false, change it to true to reflect that the specified flow has been processed 
    processed_traffic_flows[traffic_flow_id] = PROCESSED;

    return;
}

// utility functions for the noc traffic flows

void NocTrafficFlows::finshed_noc_traffic_flows_setup(void){

    // get the total number of traffic flows and create a vectorof the same size to hold the "processed status" of each traffic flow
    int num_of_traffic_flows = list_of_noc_traffic_flows.size();
    // initialize the status to not processed yet
    processed_traffic_flows.resize(num_of_traffic_flows,NOT_PROCESSED);

    return;
}

void NocTrafficFlows::reset_traffic_flows_processed_status(void){

    for (auto traffic_flow = processed_traffic_flows.begin(); traffic_flow != processed_traffic_flows.end(); traffic_flow++){
        
        *traffic_flow = NOT_PROCESSED;
    }

    return;
}

void NocTrafficFlows::clear_traffic_flows(void){

    // delete any information from internal datastructures
    list_of_noc_traffic_flows.clear();
    source_router_associated_traffic_flows.clear();
    sink_router_associated_traffic_flows.clear();
    processed_traffic_flows.clear();
    noc_router_blocks.clear();

    return;
}

bool NocTrafficFlows::check_if_cluster_block_is_a_noc_router(ClusterBlockId block_id){

    auto router_block = noc_router_blocks.find(block_id);
    bool result = false;
    
    if (router_block != noc_router_blocks.end()){
        result = true;
    }

    return result;
}

// private functions used internally

void NocTrafficFlows::add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId router_id, std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>>& router_associated_traffic_flows){

    // get a reference to the list of traffic flows associated with the current router
    auto router_traffic_flows = router_associated_traffic_flows.find(router_id);

    // check if a list exists 
    if (router_traffic_flows == router_associated_traffic_flows.end()){
        // there exists no list of traffic flows for this router, so we add it with the newly created traffic flow id
        router_associated_traffic_flows.insert(std::pair<ClusterBlockId, std::vector<NocTrafficFlowId>>(router_id, {traffic_flow_id}));
    }
    else{
        // there already is a list of traffic flows for the current router, so add it to the list
        router_traffic_flows->second.emplace_back(traffic_flow_id);
    }

    
    return;

}

void NocTrafficFlows::add_router_to_block_set(ClusterBlockId router_block_id){

    noc_router_blocks.insert(router_block_id);

    return;
}