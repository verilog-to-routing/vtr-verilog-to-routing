
#include "noc_traffic_flows.h"
#include "vpr_error.h"

// constructor indicates that the class has not been properly initialized yet as the user supplied traffic flows have not been added.
NocTrafficFlows::NocTrafficFlows() {
    built_traffic_flows = false;
}

// getters for the traffic flows

const t_noc_traffic_flow& NocTrafficFlows::get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const {
    return noc_traffic_flows[traffic_flow_id];
}

const std::vector<NocTrafficFlowId>* NocTrafficFlows::get_traffic_flows_associated_to_router_block(ClusterBlockId router_block_id) {
    const std::vector<NocTrafficFlowId>* associated_traffic_flows_ref = nullptr;

    // get a reference to the traffic flows that have the current router as a source or sink
    auto associated_traffic_flows = traffic_flows_associated_to_router_blocks.find(router_block_id);

    // check if there are any traffic flows associated with the current router
    if (associated_traffic_flows != traffic_flows_associated_to_router_blocks.end()) {
        // if we are here then there exists at least 1 traffic flow that includes the current router as a source or sink
        associated_traffic_flows_ref = &(associated_traffic_flows->second);
    }

    return associated_traffic_flows_ref;
}

int NocTrafficFlows::get_number_of_routers_used_in_traffic_flows(void) {
    return traffic_flows_associated_to_router_blocks.size();
}

// setters for the traffic flows

void NocTrafficFlows::create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency) {
    VTR_ASSERT_MSG(!built_traffic_flows, "NoC traffic flows have already been added, cannot modify further.");

    // create and add the new traffic flow to the vector
    noc_traffic_flows.emplace_back(source_router_module_name, sink_router_module_name, source_router_cluster_id, sink_router_cluster_id, traffic_flow_bandwidth, traffic_flow_latency);

    //since the new traffic flow was added to the back of the vector, its id will be the index of the last element
    NocTrafficFlowId curr_traffic_flow_id = (NocTrafficFlowId)(noc_traffic_flows.size() - 1);

    // now add the new traffic flow to flows associated with the current source and sink router
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, source_router_cluster_id);
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, sink_router_cluster_id);

    return;
}

// utility functions for the noc traffic flows

void NocTrafficFlows::finshed_noc_traffic_flows_setup(void) {
    // all the traffic flows have been added, so indicate that the class has been constructed and cannot be modified anymore
    built_traffic_flows = true;
    return;
}

void NocTrafficFlows::clear_traffic_flows(void) {
    // delete any information from internal datastructures
    noc_traffic_flows.clear();
    traffic_flows_associated_to_router_blocks.clear();

    // indicate that traffic flows need to be added again after clear
    built_traffic_flows = false;

    return;
}

bool NocTrafficFlows::check_if_cluster_block_has_traffic_flows(ClusterBlockId block_id) {
    auto traffic_flows = get_traffic_flows_associated_to_router_block(block_id);

    // indicate whether a vector of traffic flows were found that are associated to the curre cluster block
    return (traffic_flows != nullptr);
}

// private functions used internally

void NocTrafficFlows::add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId associated_router_id) {
    // get a reference to the traffic flows associated with the current router
    auto router_traffic_flows = traffic_flows_associated_to_router_blocks.find(associated_router_id);

    // check if a vector asssociated traffic flows exists
    if (router_traffic_flows == traffic_flows_associated_to_router_blocks.end()) {
        // there exists no associated traffic flows for this router, so we add it with the newly created traffic flow id
        traffic_flows_associated_to_router_blocks.insert(std::pair<ClusterBlockId, std::vector<NocTrafficFlowId>>(associated_router_id, {traffic_flow_id}));
    } else {
        // there already is a vector associated of traffic flows for the current router, so add it
        router_traffic_flows->second.emplace_back(traffic_flow_id);
    }

    return;
}

void NocTrafficFlows::echo_noc_traffic_flows(char* file_name) {
    FILE* fp;
    fp = vtr::fopen(file_name, "w");

    // file header
    fprintf(fp, "----------------------------------------------------------------------------\n");
    fprintf(fp, "NoC Traffic Flows\n");
    fprintf(fp, "----------------------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // print all the routers and their properties
    fprintf(fp, "List of NoC Traffic Flows:\n");
    fprintf(fp, "----------------------------------------------------------------------------\n");
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

    // now print the router cluster ids and their associated traffic flow information for router cluster blocks
    // The associated traffic flows represent flows where the router block is either a source or destination router
    fprintf(fp, "List of all unique router cluster blocks and their associated traffic flows:\n");
    fprintf(fp, "----------------------------------------------------------------------------\n");
    fprintf(fp, "\n");

    // go through each router block cluster and print its associated traffic flows
    // we only print out the traffic flow ids
    for (auto it = traffic_flows_associated_to_router_blocks.begin(); it != traffic_flows_associated_to_router_blocks.end(); it++) {
        // print the current router cluster id
        fprintf(fp, "Cluster ID %lu: ", (size_t)it->first);

        // get the vector of associated traffic flows
        auto assoc_traffic_flows = &(it->second);

        // now go through the associated traffic flows of the current router and print their ids
        for (auto traffic_flow = assoc_traffic_flows->begin(); traffic_flow != assoc_traffic_flows->end(); traffic_flow++) {
            fprintf(fp, "%lu ", (size_t)*traffic_flow);
        }

        // seperate to the next cluster associated traffic flows information
        fprintf(fp, "\n\n");
    }

    vtr::fclose(fp);

    return;
}