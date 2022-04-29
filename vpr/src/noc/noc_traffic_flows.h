#ifndef NOC_TRAFFIC_FLOWS_H
#define NOC_TRAFFIC_FLOWS_H

/*
 * The NocTrafficFlows class description header file.
 *
 * The NocTrafficFlows class contains all the communication within the NoC. So
 * this includes the source and destination routers involved in the
 * communication link and its properties (size and constraints on the link).
 * We call this infor mation a traffic flow.
 * 
 * In addition, the NoCTrafficFlows class includes additional variables and 
 * functions to quickly access the traffic flows. 
 *
 */

#include "clustered_netlist_fwd.h"
#include "noc_data_types.h"
#include "vtr_vector.h"
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>


/*
    Whenever a traffic flow is processed (routed a connection between
    2 routers and updated the relevant information) we need to
    some way to track it, so that we don't process it again. The flags below indicate whether a traffic flow was processed or not.
*/
constexpr bool PROCESSED = true;
constexpr bool NOT_PROCESSED = false;

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

        // contains all the traffic flows provided by the user and their information
        vtr::vector<NocTrafficFlowId, t_noc_traffic_flow> list_of_noc_traffic_flows;

        /*
            A traffic flow has a source and destination router associated to it. So when either the source or destination router for a given flow is moved, we need to find a new route between them. 
            
            Therefore, during placement if two router blocks are swapped, then the only traffic flows we need to re-route are the flows where the two routers are either the source or destination routers of those flows. 

            The datastructures below store a list of traffic flows for each router then its a source router for the traffic flow. Similarily, there is a another datastructure for where a list of traffic flows are stored for each router where its a source router for the traffic flow. The routers are indexed by their ClusterBlockId.

            This is done so that the traffic that need to be re-routed during placement are quickly found. 
        */
        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> source_router_associated_traffic_flows;
        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> sink_router_associated_traffic_flows; 

        // keeps track of which traffic flows have already been processed
        vtr::vector<NocTrafficFlowId, bool> processed_traffic_flows;

        // contains the cluster ID of all unique router modules in the design
        // can quikly determine whether a given cluster is a router module or not
        std::unordered_set<ClusterBlockId> noc_router_blocks;

        // shouldnt have access to these functions
        void add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId source_router_id, std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>>& router_associated_traffic_flows);

        void add_router_to_block_set(ClusterBlockId router_block_id);


    public:
        NocTrafficFlows();

        //getters
        const t_noc_traffic_flow& get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const;

        const std::vector<NocTrafficFlowId>* get_traffic_flows_associated_to_source_router(ClusterBlockId source_router_id) const;

        const std::vector<NocTrafficFlowId>* get_traffic_flows_associated_to_sink_router(ClusterBlockId sink_router_id) const;

        bool get_traffic_flow_processed_status(NocTrafficFlowId traffic_flow_id);

        int get_number_of_routers_used_in_traffic_flows(void);


        // setters
        void create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency);

        void set_traffic_flow_status(NocTrafficFlowId traffic_flow_id, bool status);

        //utility functions
        void finshed_noc_traffic_flows_setup(void);
        void reset_traffic_flows_processed_status(void);
        void clear_traffic_flows(void);
        bool check_if_cluster_block_is_a_noc_router(ClusterBlockId block_id);

};


#endif