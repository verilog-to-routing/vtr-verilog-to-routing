#ifndef NOC_TRAFFIC_FLOWS_H
#define NOC_TRAFFIC_FLOWS_H

/**
 * @file 
 * @brief This file defines the NocTrafficFlows class, which contains all
 * communication betwee routers in the NoC.
 * 
 * Overview
 * ======== 
 * The NocTrafficFlows class contains all traffic flows in a given 
 * design. A traffic flow is defined by the t_noc_traffic_flow struct.
 * Each traffic flow is indexed by a unique id that can be used to
 * retrieve information about them.  
 * 
 * The class also associates traffic flows to their source routers (start point)
 * and sink routers (end point). This is useful if one wants to find traffic
 * flows based on just the source or sink router.
 * 
 * The routes for the traffic flows are expected to change throughout placement
 * as routers will be moved throughout the chip. Therefore this class provides
 * a datastructure to keep track of which flows have been updated (re-routed).
 * 
 * Finally, this class also stores all router blocks in the design.
 * 
 * This class will be primarily used during 
 * placement to identify which routers inside the NoC(NocStorage) need to
 * routed to each other.This is important since the router modules can be moved
 * around to different tiles on the FPGA device.
 * 
 */
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "clustered_netlist_fwd.h"
#include "noc_data_types.h"
#include "vtr_vector.h"
#include "echo_files.h"
#include "vtr_util.h"


/*
    Whenever a traffic flow is processed (routed a connection between
    2 routers and updated the relevant information) we need to
    some way to track it, so that we don't process it again. The flags below indicate whether a traffic flow was processed or not.
*/
constexpr bool PROCESSED = true;
constexpr bool NOT_PROCESSED = false;

/**
 * @brief Describes a traffic flow within the NoC, which is the communication
 * between two routers. THe NocTrafficFlows contains a number of this structure
 * to describe all the communication happening within the NoC.
 * 
 */
struct t_noc_traffic_flow {

    /** stores the names of the router blocks communicating within this traffic flow*/
    std::string source_router_module_name;
    std::string sink_router_module_name;

    /** stores the block id of the two routers blocks communicating within this traffic flow. This can be used to retrieve the block information from the clustered netlist*/
    ClusterBlockId source_router_cluster_id;
    ClusterBlockId sink_router_cluster_id;

    /** The bandwidth of the information transferred between the two routers. Units in bps. This parameters will be used to update the link usage in the noc model after routing the traffic flow.*/
    double traffic_flow_bandwidth;

    /** The maximum allowable time to transmit data between thw two routers. This parameter will be used to evaluate a router traffic flow.*/
    double max_traffic_flow_latency;

    /** Constructor initializes all variables*/
    t_noc_traffic_flow(std::string source_router_name, std::string sink_router_name, ClusterBlockId source_router_id, ClusterBlockId sink_router_id, double flow_bandwidth, double max_flow_latency) : source_router_module_name(source_router_name), sink_router_module_name(sink_router_name), source_router_cluster_id(source_router_id), sink_router_cluster_id(sink_router_id), traffic_flow_bandwidth(flow_bandwidth), max_traffic_flow_latency(max_flow_latency)
    {}

};

class NocTrafficFlows
{
    private:

        /** contains all the traffic flows provided by the user and their information*/
        vtr::vector<NocTrafficFlowId, t_noc_traffic_flow> noc_traffic_flows;

        /**
            A traffic flow has a source and destination router associated to it. So when either the source or destination router for a given flow is moved, we need to find a new route between them. 
            
            Therefore, during placement if two router blocks are swapped, then the only traffic flows we need to re-route are the flows where the two routers are either the source or destination routers of those flows. 

            The datastructures below store all traffic flows for each router where the router is a source for the traffic flow. Similarily, there is a another datastructure where all traffic flows are stored for each router where the router is a sink for the traffic flow. The routers are indexed by their ClusterBlockId.

            This is done so that the traffic that need to be re-routed during placement can be quickly found. 
        */
        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> source_router_associated_traffic_flows;
        std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> sink_router_associated_traffic_flows; 

        /** keeps track of which traffic flows have already been processed.
         * By processed, it tracks whether a traffic flow has been routed or
         * not. During placement, two routers can be swapped, if the
         * two routers share a traffic flow, then the shared traffic flow
         * will be routed twice, where it will routed when processing the
         * flows associated to the source router and then again when 
         * processing the flows associated to the sink router. By using the
         * datastructure below, if whether a traffic flow is processed or not
         * is tracked, then we can ensure that no traffic flows are routed
         * more than once by referring to this datastructure.
         */
        vtr::vector<NocTrafficFlowId, bool> processed_traffic_flows;

        /** contains the cluster ID of all unique router modules in the design.
         * and can quickly determine whether a given cluster in the netlist
         * is a router block or not. whenever placement swaps two clusters, we
         * need to check if the clusters a router blocks or not, this is needed
         * so that we can deterimine if we need to re-route traffic flows. The
         * datastructure below can be used to quickly identify whether a given
         * cluster that was swapped furing palcement is a router block or not.*/
        std::unordered_set<ClusterBlockId> noc_router_blocks;

        // private functions

        /**
         * @brief Given a router that is either a source or sink of
         * a traffic flow, the corresponding traffic flow is added
         * to a vector of traffic flows associated to the router.
         * 
         * @param traffic_flow_id A unique id that represents a traffic flow.
         * @param associated_router_id A ClusterblockId that represents a
         *                             router block. 
         * @param router_associated_traffic_flows A datastructure that stores
         * a vector of traffic flows for a given router block where the traffic
         * flows have the router as a source or sink within the flow.
         *                                        
         */
        void add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId associated_router_id, std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>>& router_associated_traffic_flows);

        /**
         * @brief Given a router block in the clustered netlist, store it
         * internally.
         * 
         * @param router_block_id A unique identifier that rerpesents a router block in the clustered netlist. This id will be stored.
         */
        void add_router_to_block_set(ClusterBlockId router_block_id);


    public:
        NocTrafficFlows();

        //getters

        /**
         * @brief Given a unique id of a traffic flow (t_noc_traffic_flow)
         * retrieve it from the vector of all traffic flows in the design. The
         * retrieved traffic flow cannot be modified but can be used to
         * retireve information such as the routers involved.
         * 
         * @param traffic_flow_id The unique identifier (NocTrafficFlowId)
         * of the traffic flow to retrieve.
         * @return const t_noc_traffic_flow& The traffic flow represented by
         * the provided identifier.
         */
        const t_noc_traffic_flow& get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const;

        /**
         * @brief Get a vector of all traffic flows that have a given router
         * block in the clustered netlist as the source (starting point) in the 
         * flow.
         * 
         * @param source_router_id A unique identifier that represents the
         * a router block in the clustered netlist. This router block will
         * be the source router in the retrieved traffic flow.
         * @return const std::vector<NocTrafficFlowId>* A vector of traffic 
         * flows that have the input router block parameter as the source in the
         * flow.
         */
        const std::vector<NocTrafficFlowId>* get_traffic_flows_associated_to_source_router(ClusterBlockId source_router_id) const;
        
        /**
         * @brief Get a vector of all traffic flows that have a given router
         * block in the clustered netlist as the sink (end point) in the 
         * flow.
         * 
         * @param sink_router_id A unique identifier that represents the
         * a router block in the clustered netlist. This router block will
         * be the sink router in the retrieved traffic flow.
         * @return const std::vector<NocTrafficFlowId>* A vector of traffic 
         * flows that have the input router block parameter as the sink in the
         * flow.
         */
        const std::vector<NocTrafficFlowId>* get_traffic_flows_associated_to_sink_router(ClusterBlockId sink_router_id) const;

        /**
         * @brief Given a traffic flow, determine whether it has been processed
         * or not. By processed, it checks to see whether the traffic flow has
         * been routed. This function should be used before routing a traffic
         * flow.
         * 
         * @param traffic_flow_id A unique identifier that represents a traffic
         * flow
         * @return true Represents the case where the traffic flow has been
         * processed
         * @return false Represents the case where the traffic flow has not
         * been processed 
         */
        bool get_traffic_flow_processed_status(NocTrafficFlowId traffic_flow_id);

        /**
         * @brief Gets the number of unique router blocks in the
         * clustered netlist that were used within the user provided
         * traffic flows description. 
         * 
         * @return int The total number of unique routers used in
         * the traffic flows provided by the user.
         */
        int get_number_of_routers_used_in_traffic_flows(void);


        // setters

        /**
         * @brief Given a set of parameters that describe a traffic
         * flow, create it an add it to the vector of traffic flows in the
         * design. Additionally, the two router blocks involved have their
         * ids stored to to keep track of all router blocks that are used
         * in traffic flows. Finally, the newly created traffic flow is
         * also added to a vector of traffic flows that are associated to both 
         * the source and sink routers of the flow.
         * 
         * @param source_router_module_name A string that represents the
         * name of the source router block in the traffic flow. THis is
         * provided by the user.
         * @param sink_router_module_name A string that represents the name
         * of the sink router block in the traffic flow. This is provided by
         * the user.
         * @param source_router_cluster_id The source router block id that
         * uniquely identifies this block in the clustered netlist.
         * @param sink_router_cluster_id  The sink router block id that
         * uniquely identifier this block in the clusterd netlist.
         * @param traffic_flow_bandwidth The size of the data transmission
         * in this traffic flow (units of bps).
         * @param traffic_flow_latency The maximum allowable delay between
         * transmitting data at the source router and having it received
         * at the sink router.
         */
        void create_noc_traffic_flow(std::string source_router_module_name, std::string sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency);
        
        /**
         * @brief Set the status of a given traffic flow. The status indicates
         * whether the traffic flow has been routed or not.
         * 
         * @param traffic_flow_id A unique identifier that represents a traffic
         * flow.
         * @param status THe updated status of the traffic flow. True indicating
         * that the traffic flow has been router and False indicating that
         * the traffic flow has not been routed.
         */
        void set_traffic_flow_status(NocTrafficFlowId traffic_flow_id, bool status);

        //utility functions

        /**
         * @brief Determines the total number of traffic flows in
         * the design and creates a vector to keep track of the traffic
         * flows processed status. This function should be called after
         * all the traffic flows have been created. The traffic flows are
         * all initialized to being not processed.
         * 
         */
        void finshed_noc_traffic_flows_setup(void);

        /**
         * @brief Goes through the status of all traffic flows
         * and sets them to being not processed. This should be
         * used before each iteraition of placement.
         * 
         */
        void reset_traffic_flows_processed_status(void);

        /**
         * @brief Resets the class by clearning internal
         * satastructures.
         * 
         */
        void clear_traffic_flows(void);

        /**
         * @brief Given a block from the clustered netlist, determine
         * if the block is a router. This function should be used during
         * each iteration of placement to check whether two swapped clusters
         * are routers or not.
         * 
         * @param block_id A unique identifier that represents a cluster
         * block in the clustered netlist
         * @return true The block is a router
         * @return false THe block is not a router
         */
        bool check_if_cluster_block_is_a_noc_router(ClusterBlockId block_id);

        /**
         * @brief Writes out the NocTrafficFlows class information to a file.
         * This includes printing out each internal datastructure information.
         * 
         * @param file_name The name of the file that contains the NoC
         * traffic flow information   
         */
        void echo_noc_traffic_flows(char* file_name);

};


#endif