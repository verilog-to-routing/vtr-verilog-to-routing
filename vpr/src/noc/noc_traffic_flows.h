#ifndef NOC_TRAFFIC_FLOWS_H
#define NOC_TRAFFIC_FLOWS_H

#include "clustered_netlist_fwd.h"
#include "noc_data_types.h"
#include <iostream>
#include <unordered_map>
#include <vector>

/*
    Describes a traffic flow within the NoC, which is the communication between two routers. This description includes the following:
        - The module names of the source and destination routers that are communicating with each other. The module name is the name used when instantiating a router module in the HDL design.
        - The atom IDs of the router modules. By storing this, we can quickly lookup the router modules instead of identifying them by using their names.
        - The bandwidth(size) of data being transferred between the two routers.
        - The maximum allowable latency of the data transfer.

    This datastructure will be primarily used during placement to identify which routers inside the NoC(NocStorage) need to be routed to each other.This is important since the router modules can be moved around to different tiles on the FPGA device. Additionaly, once a the routing has been completed, the bandwidht and latency parameters can be used to calculate the cost of the placement of routers.
*/
struct t_noc_traffic_flow {

    // router module names
    std::string source_router_module_name;
    std::string sink_router_module_name;

    // router module atom ids
    ClusterBlockId source_router_cluster_id;
    ClusterBlockId sink_router_cluster_id;

    // in bytes/s
    double traffic_flow_bandwidth;

    // in seconds
    double max_traffic_flow_latency;

    // constructor
    t_noc_traffic_flow(std::string source_router_name, std::string sink_router_name, ClusterBlockId source_router_id, ClusterBlockId sink_router_id, double flow_bandwidth, double max_flow_latency) : source_router_module_name(source_router_name), sink_router_module_name(sink_router_name), source_router_cluster_id(source_router_id), sink_router_cluster_id(sink_router_id), traffic_flow_bandwidth(flow_bandwidth), max_traffic_flow_latency(max_flow_latency)
    {}

};

class NocTrafficFlows
{
    private:

        vtr::vector<NocTrafficFlowId, t_noc_traffic_flow> list_of_noc_traffic_flows;

        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> source_router_associated_traffic_flows;
        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> sink_router_associated_traffic_flows; 

        vtr::vector<NocTrafficFlowId, bool> processed_traffic_flows;

        // shouldnt have access to this functions
        void add_traffic_flow_to_associated_source_and_sink_routers(NocTrafficFlowId traffic_flow_id);


    public:
        NocTrafficFlows();

        //getters
        const t_noc_traffic_flow& get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const;

        const std::vector<NocTrafficFlowId>& get_traffic_flows_associated_to_source_router(ClusterBlockId source_router_id) const;

        const std::vector<NocTrafficFlowId>& get_traffic_flows_associated_to_sink_router(ClusterBlockId sink_router_id) const;


        // setters
        void create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency);


        //utility functions
        void finshed_noc_traffic_flows_setup(void);
        void reset_traffic_flows_processed_status(void);
        void clear_traffic_flows(void);

};


#endif