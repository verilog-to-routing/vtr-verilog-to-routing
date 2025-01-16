
#include "noc_traffic_flows.h"
#include "vpr_error.h"

// constructor indicates that the class has not been properly initialized yet as the user supplied traffic flows have not been added.
NocTrafficFlows::NocTrafficFlows() {
    built_traffic_flows = false;
}

// getters for the traffic flows

int NocTrafficFlows::get_number_of_traffic_flows() const {
    return noc_traffic_flows.size();
}

const t_noc_traffic_flow& NocTrafficFlows::get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const {
    return noc_traffic_flows[traffic_flow_id];
}

const std::vector<NocTrafficFlowId>& NocTrafficFlows::get_traffic_flows_associated_to_router_block(ClusterBlockId router_block_id) const {
    // to be returned in the given router does not have any associated traffic flows
    static const std::vector<NocTrafficFlowId> empty_vector;

    // get a reference to the traffic flows that have the current router as a source or sink
    auto associated_traffic_flows = traffic_flows_associated_to_router_blocks.find(router_block_id);

    // check if there are any traffic flows associated with the current router
    if (associated_traffic_flows != traffic_flows_associated_to_router_blocks.end()) {
        // if we are here then there exists at least 1 traffic flow that includes the current router as a source or sink
        return associated_traffic_flows->second;
    } else {
        return empty_vector;
    }
}

int NocTrafficFlows::get_number_of_routers_used_in_traffic_flows() {
    return traffic_flows_associated_to_router_blocks.size();
}

const std::vector<ClusterBlockId>& NocTrafficFlows::get_router_clusters_in_netlist() const {
    return router_cluster_in_netlist;
}

const std::vector<NocTrafficFlowId>& NocTrafficFlows::get_all_traffic_flow_id() const {
    return noc_traffic_flows_ids;
}

const vtr::vector<NocTrafficFlowId, t_noc_traffic_flow>& NocTrafficFlows::get_all_traffic_flows() const {
    return noc_traffic_flows;
}


// setters for the traffic flows

void NocTrafficFlows::create_noc_traffic_flow(const std::string& source_router_module_name,
                                              const std::string& sink_router_module_name,
                                              ClusterBlockId source_router_cluster_id,
                                              ClusterBlockId sink_router_cluster_id,
                                              double traffic_flow_bandwidth,
                                              double traffic_flow_latency,
                                              int traffic_flow_priority) {
    VTR_ASSERT_MSG(!built_traffic_flows, "NoC traffic flows have already been added, cannot modify further.");

    // create and add the new traffic flow to the vector
    noc_traffic_flows.emplace_back(source_router_module_name,
                                   sink_router_module_name,
                                   source_router_cluster_id,
                                   sink_router_cluster_id,
                                   traffic_flow_bandwidth,
                                   traffic_flow_latency,
                                   traffic_flow_priority);

    //since the new traffic flow was added to the back of the vector, its id will be the index of the last element
    NocTrafficFlowId curr_traffic_flow_id = (NocTrafficFlowId)(noc_traffic_flows.size() - 1);
    noc_traffic_flows_ids.emplace_back(curr_traffic_flow_id);

    // now add the new traffic flow to flows associated with the current source and sink router
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, source_router_cluster_id);
    add_traffic_flow_to_associated_routers(curr_traffic_flow_id, sink_router_cluster_id);
}

void NocTrafficFlows::set_router_cluster_in_netlist(const std::vector<ClusterBlockId>& routers_cluster_id_in_netlist) {
    router_cluster_in_netlist.clear();
    //copy the input vector to the internal vector
    for (auto router_id : routers_cluster_id_in_netlist) {
        router_cluster_in_netlist.emplace_back(router_id);
    }
}

// utility functions for the noc traffic flows

void NocTrafficFlows::finished_noc_traffic_flows_setup() {
    // all the traffic flows have been added, so indicate that the class has been constructed and cannot be modified anymore
    built_traffic_flows = true;
}

void NocTrafficFlows::clear_traffic_flows() {
    // delete any information from internal data structures
    noc_traffic_flows.clear();
    noc_traffic_flows_ids.clear();
    router_cluster_in_netlist.clear();
    traffic_flows_associated_to_router_blocks.clear();

    // indicate that traffic flows need to be added again after clear
    built_traffic_flows = false;
}

bool NocTrafficFlows::check_if_cluster_block_has_traffic_flows(ClusterBlockId block_id) const {
    auto traffic_flows = get_traffic_flows_associated_to_router_block(block_id);

    // indicate whether a vector of traffic flows were found that are associated to the current cluster block
    return (!traffic_flows.empty());
}

// private functions used internally

void NocTrafficFlows::add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId associated_router_id) {
    // get a reference to the traffic flows associated with the current router
    auto router_traffic_flows = traffic_flows_associated_to_router_blocks.find(associated_router_id);

    // check if a vector associated traffic flows exists
    if (router_traffic_flows == traffic_flows_associated_to_router_blocks.end()) {
        // there exists no associated traffic flows for this router, so we add it with the newly created traffic flow id
        traffic_flows_associated_to_router_blocks.insert(std::pair<ClusterBlockId, std::vector<NocTrafficFlowId>>(associated_router_id, {traffic_flow_id}));
    } else {
        // there already is a vector associated of traffic flows for the current router, so add it
        router_traffic_flows->second.emplace_back(traffic_flow_id);
    }
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
    for (const auto& traffic_flow : noc_traffic_flows) {
        // print the current traffic flows data
        fprintf(fp, "Traffic flow ID: %d\n", traffic_flow_id);
        fprintf(fp, "Traffic flow source router block name: %s\n", traffic_flow.source_router_module_name.c_str());
        fprintf(fp, "Traffic flow source router block cluster ID: %lu\n", (size_t)traffic_flow.source_router_cluster_id);
        fprintf(fp, "Traffic flow sink router block name: %s\n", traffic_flow.sink_router_module_name.c_str());
        fprintf(fp, "Traffic flow sink router block cluster ID: %lu\n", (size_t)traffic_flow.sink_router_cluster_id);
        fprintf(fp, "Traffic flow bandwidth: %f bps\n", traffic_flow.traffic_flow_bandwidth);
        fprintf(fp, "Traffic flow latency: %f seconds\n", traffic_flow.max_traffic_flow_latency);

        // separate the next link information
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
    for (const auto& router_traffic_flows : traffic_flows_associated_to_router_blocks) {
        // print the current router cluster id
        fprintf(fp, "Cluster ID %lu: ", (size_t)router_traffic_flows.first);

        // get the vector of associated traffic flows
        auto assoc_traffic_flows = &(router_traffic_flows.second);

        // now go through the associated traffic flows of the current router and print their ids
        for (auto assoc_traffic_flow : *assoc_traffic_flows) {
            fprintf(fp, "%lu ", (size_t)assoc_traffic_flow);
        }

        // separate to the next cluster associated traffic flows information
        fprintf(fp, "\n\n");
    }

    vtr::fclose(fp);
}

