
#include "noc_traffic_flows.h"
#include "vpr_error.h"

// constructor clears all the traffic flow datastructures
NocTrafficFlows::NocTrafficFlows(void) {
    clear_traffic_flows();
}

// getters for the traffic flows

const t_noc_traffic_flow& NocTrafficFlows::get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const {
    return noc_traffic_flows[traffic_flow_id];
}

const std::vector<NocTrafficFlowId>* NocTrafficFlows::get_traffic_flows_associated_to_source_router(ClusterBlockId source_router_id) const {
    const std::vector<NocTrafficFlowId>* associated_traffic_flows_ref = nullptr;

    // get a reference to the traffic flows that have the current router as a source
    auto associated_traffic_flows = source_router_associated_traffic_flows.find(source_router_id);

    // check if there are any traffic flows associated with the current router
    if (associated_traffic_flows != source_router_associated_traffic_flows.end()) {
        // if we are here then there exists atleast 1 traffic flow that includes the current router as a source
        associated_traffic_flows_ref = &(associated_traffic_flows->second);
    }

    return associated_traffic_flows_ref;
}

const std::vector<NocTrafficFlowId>* NocTrafficFlows::get_traffic_flows_associated_to_sink_router(ClusterBlockId source_router_id) const {
    const std::vector<NocTrafficFlowId>* associated_traffic_flows_ref = nullptr;

    // get a reference to the traffic flows that have the current router as a source
    auto associated_traffic_flows = sink_router_associated_traffic_flows.find(source_router_id);

    // check if there are any traffic flows associated with the current router
    if (associated_traffic_flows != sink_router_associated_traffic_flows.end()) {
        // if we are here then there exists atleast 1 traffic flow that includes the current router as a source
        associated_traffic_flows_ref = &(associated_traffic_flows->second);
    }

    return associated_traffic_flows_ref;
}

bool NocTrafficFlows::get_traffic_flow_processed_status(NocTrafficFlowId traffic_flow_id) {
    return processed_traffic_flows[traffic_flow_id];
}

int NocTrafficFlows::get_number_of_routers_used_in_traffic_flows(void) {
    return noc_router_blocks.size();
}

// setters for the traffic flows

void NocTrafficFlows::create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency) {
    // create and add the new traffic flow to the vector
    noc_traffic_flows.emplace_back(source_router_module_name, sink_router_module_name, source_router_cluster_id, sink_router_cluster_id, traffic_flow_bandwidth, traffic_flow_latency);

    //since the new traffic flow was added to the back of the vector, its id will be the index of the last element
    NocTrafficFlowId curr_traffic_flow_id = (NocTrafficFlowId)(noc_traffic_flows.size() - 1);

    // add the source and sink routers to the set of unique router blocks in the design
    add_router_to_block_set(source_router_cluster_id);
    add_router_to_block_set(sink_router_cluster_id);

    // now add the new traffic flow to flows associated with the current source and sink router
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, source_router_cluster_id, this->source_router_associated_traffic_flows);
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, sink_router_cluster_id, this->sink_router_associated_traffic_flows);

    return;
}

// given a traffic flow and its status, update it
void NocTrafficFlows::set_traffic_flow_status(NocTrafficFlowId traffic_flow_id, bool status) {
    // status is initialized to false, change it to true to reflect that the specified flow has been processed
    processed_traffic_flows[traffic_flow_id] = status;

    return;
}

// utility functions for the noc traffic flows

void NocTrafficFlows::finshed_noc_traffic_flows_setup(void) {
    // get the total number of traffic flows and create a vector of the same size to hold the "processed status" of each traffic flow
    int num_of_traffic_flows = noc_traffic_flows.size();
    // initialize the status to not processed yet
    processed_traffic_flows.resize(num_of_traffic_flows, NOT_PROCESSED);

    return;
}

void NocTrafficFlows::reset_traffic_flows_processed_status(void) {
    for (auto traffic_flow = processed_traffic_flows.begin(); traffic_flow != processed_traffic_flows.end(); traffic_flow++) {
        *traffic_flow = NOT_PROCESSED;
    }

    return;
}

void NocTrafficFlows::clear_traffic_flows(void) {
    // delete any information from internal datastructures
    noc_traffic_flows.clear();
    source_router_associated_traffic_flows.clear();
    sink_router_associated_traffic_flows.clear();
    processed_traffic_flows.clear();
    noc_router_blocks.clear();

    return;
}

bool NocTrafficFlows::check_if_cluster_block_is_a_noc_router(ClusterBlockId block_id) {
    auto router_block = noc_router_blocks.find(block_id);

    bool result = false;

    // Check if the given cluster block was found inside the set of all router blocks in the design, then this was a router block so return true. Otherwise return false.
    if (router_block != noc_router_blocks.end()) {
        result = true;
    }

    return result;
}

// private functions used internally

void NocTrafficFlows::add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId associated_router_id, std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>>& router_associated_traffic_flows) {
    // get a reference to the traffic flows associated with the current router
    auto router_traffic_flows = router_associated_traffic_flows.find(associated_router_id);

    // check if a vector asssociated traffic flows exists
    if (router_traffic_flows == router_associated_traffic_flows.end()) {
        // there exists no associated traffic flows for this router, so we add it with the newly created traffic flow id
        router_associated_traffic_flows.insert(std::pair<ClusterBlockId, std::vector<NocTrafficFlowId>>(associated_router_id, {traffic_flow_id}));
    } else {
        // there already is a vector associated of traffic flows for the current router, so add it
        router_traffic_flows->second.emplace_back(traffic_flow_id);
    }

    return;
}

void NocTrafficFlows::add_router_to_block_set(ClusterBlockId router_block_id) {
    noc_router_blocks.insert(router_block_id);

    return;
}

void NocTrafficFlows::echo_noc_traffic_flows(char* file_name) {
    FILE* fp;
    fp = vtr::fopen(file_name, "w");

    // file header
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "NoC Traffic Flows\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // print all the routers and their properties
    fprintf(fp, "List of NoC Traffic Flows:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    int traffic_flow_id = 0;

    // go through each traffic flow and print its information
    for (auto traffic_flow = noc_traffic_flows.begin(); traffic_flow != noc_traffic_flows.end(); traffic_flow++) {
        // print the current traffic flows data
        fprintf(fp, "Traffic flow ID: %d\n", traffic_flow_id);
        fprintf(fp, "Traffic flow source router block name: %s\n", traffic_flow->source_router_module_name.c_str());
        fprintf(fp, "Traffic flow source router block cluster ID: %lu\n", (size_t)traffic_flow->source_router_cluster_id);
        fprintf(fp, "Traffic flow sink router block name: %s\n", traffic_flow->sink_router_module_name.c_str());
        fprintf(fp, "Traffic flow sink router block cluster ID: %lu\n", (size_t)traffic_flow->sink_router_cluster_id);
        fprintf(fp, "Traffic flow bandwidth: %f bps\n", traffic_flow->traffic_flow_bandwidth);
        fprintf(fp, "Traffic flow latency: %f seconds\n", traffic_flow->max_traffic_flow_latency);

        // seperate the next link information
        fprintf(fp, "\n");

        // update the id for the next traffic flow
        traffic_flow_id++;
    }

    // now print the associated traffic flow information for router cluster blocks that act as source routers in traffic flows
    fprintf(fp, "List of traffic flows where the router block cluster is a source router:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // go through each router block cluster and print its associated traffic flows where the cluster is a source router for those traffic flows
    // we only print out the traffic flow ids
    for (auto it = source_router_associated_traffic_flows.begin(); it != source_router_associated_traffic_flows.end(); it++) {
        // print the current router cluster id
        fprintf(fp, "Cluster ID %lu: ", (size_t)it->first);

        // get the vector of associated traffic flows
        auto assoc_traffic_flows = &(it->second);

        // now go through the traffic flows this router is a source router and add it
        for (auto traffic_flow = assoc_traffic_flows->begin(); traffic_flow != assoc_traffic_flows->end(); traffic_flow++) {
            fprintf(fp, "%lu ", (size_t)*traffic_flow);
        }

        // seperate the next cluster information
        fprintf(fp, "\n\n");
    }

    // now print the associated traffic flow information for router cluster blocks that act as source routers in traffic flows
    fprintf(fp, "List of traffic flows where the router block cluster is a sink router:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // go through each router block cluster and print its associated traffic flows where the cluster is a sink router for those traffic flows
    // we only print out the traffic flow ids
    for (auto it = sink_router_associated_traffic_flows.begin(); it != sink_router_associated_traffic_flows.end(); it++) {
        // print the current router cluster id
        fprintf(fp, "Router block cluster ID %lu: ", (size_t)it->first);

        // get the vector of associated traffic flows
        auto assoc_traffic_flows = &(it->second);

        // now go through the traffic flows this router is a sink router and add it
        for (auto traffic_flow = assoc_traffic_flows->begin(); traffic_flow != assoc_traffic_flows->end(); traffic_flow++) {
            fprintf(fp, "%lu ", (size_t)*traffic_flow);
        }

        // seperate the next cluster information
        fprintf(fp, "\n\n");
    }

    // now print all the unique router block cluster ids in the netlist
    fprintf(fp, "List of all router block cluster IDs in the netlist:\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // print all the unique router block cluster ids
    for (auto single_router_block = noc_router_blocks.begin(); single_router_block != noc_router_blocks.end(); single_router_block++) {
        fprintf(fp, "Router cluster block ID: %lu\n", (size_t)*single_router_block);
    }

    vtr::fclose(fp);

    return;
}