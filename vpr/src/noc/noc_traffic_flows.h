#ifndef NOC_TRAFFIC_FLOWS_H
#define NOC_TRAFFIC_FLOWS_H

/**
 * @file 
 * @brief This file defines the NocTrafficFlows class, which contains all
 * communication between routers in the NoC.
 * 
 * Overview
 * ======== 
 * The NocTrafficFlows class contains all traffic flows in a given 
 * design. A traffic flow is defined by the t_noc_traffic_flow struct.
 * Each traffic flow is indexed by a unique id that can be used to
 * retrieve information about them.  
 * 
 * The class also associates traffic flows to their logical source routers 
 * (start point) and logical sink routers (end point). This is useful if one wants to find traffic flows based on just the source or sink logical router.
 * The routes for the traffic flows are expected to change throughout placement
 * as routers will be moved within the chip. Therefore this class provides
 * a datastructure to keep track of which flows have been updated (re-routed).
 * 
 * Finally, this class also stores all router blocks (logical routers) in the 
 * design.
 * 
 * This class will be primarily used during 
 * placement to identify which routers inside the NoC ( NocStorage ) need to be
 * routed to each other.This is important since the router modules can be moved
 * around to different tiles on the FPGA device.
 * 
 */
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "clustered_netlist_fwd.h"
#include "noc_data_types.h"
#include "vtr_vector.h"
#include "echo_files.h"
#include "vtr_util.h"
#include "vtr_assert.h"

/**
 * @brief Describes a traffic flow within the NoC, which is the communication
 * between two logical routers. The NocTrafficFlows contains a number of this 
 * structure to describe all the communication happening within the NoC.
 * 
 */
struct t_noc_traffic_flow {
    /** stores the partial name of the source router block communicating within this traffic flow. Names must uniquely identify router blocks in the netlist.*/
    std::string source_router_module_name;
    /** stores the partial name of the sink router block communicating within this traffic flow. Names must uniquely identify router blocks in the netlist.*/
    std::string sink_router_module_name;

    /** stores the block id of the source router block communicating within this traffic flow. This can be used to retrieve the block information from the clustered netlist*/
    ClusterBlockId source_router_cluster_id;
    /** stores the block id of the destination router block communicating within this traffic flow. This can be used to retrieve the block information from the clustered netlist*/
    ClusterBlockId sink_router_cluster_id;

    /** The bandwidth of the information transferred between the two routers. Units in bytes per second. This parameters will be used to update the link usage in the noc model after routing the traffic flow.*/
    double traffic_flow_bandwidth;

    /** The maximum allowable time to transmit data between thw two routers, in seconds. This parameter will be used to evaluate a router traffic flow.*/
    double max_traffic_flow_latency;

    /** Indicates the importance of the traffic flow. Higher priority traffic flows will have more importance and will be more likely to have their latency reduced and constraints met. Range: [0-inf) */
    int traffic_flow_priority;

    /** Constructor initializes all variables*/
    t_noc_traffic_flow(std::string source_router_name, std::string sink_router_name, ClusterBlockId source_router_id, ClusterBlockId sink_router_id, double flow_bandwidth, double max_flow_latency, int flow_priority)
        : source_router_module_name(std::move(source_router_name))
        , sink_router_module_name(std::move(sink_router_name))
        , source_router_cluster_id(source_router_id)
        , sink_router_cluster_id(sink_router_id)
        , traffic_flow_bandwidth(flow_bandwidth)
        , max_traffic_flow_latency(max_flow_latency)
        , traffic_flow_priority(flow_priority) {}
};

class NocTrafficFlows {
  private:
    /** contains all the traffic flows provided by the user and their information*/
    vtr::vector<NocTrafficFlowId, t_noc_traffic_flow> noc_traffic_flows;

    /** contains all the traffic flows ids provided by the user*/
    std::vector<NocTrafficFlowId> noc_traffic_flows_ids;

    /** contains the ids of all the router cluster blocks within the design */
    std::vector<ClusterBlockId> router_cluster_in_netlist;

    /**
     * @brief Each traffic flow is composed of a source and destination 
     * router. If the source/destination routers are moved, then the traffic
     * flow needs tp be re-routed. 
     * 
     * This datastructure stores a vector of traffic flows that are associated
     * to each router cluster block. A traffic flow is associated to a router
     * cluster block if the router block is either the source or destination
     * router within the traffic flow.
     * 
     * This is done so that during placement when a router cluster block is 
     * moved then the traffic flows that need to be re-routed due to the moved
     * block can quickly be found.
     * 
     */
    std::unordered_map<ClusterBlockId, std::vector<NocTrafficFlowId>> traffic_flows_associated_to_router_blocks;

    /**
     * Indicates whether the the NocTrafficFlows class has been fully
     * constructed. The expectation is that all the traffic flows will
     * be added in one spot and will not be added later on. So this variable
     * can be used to check whether all the traffic flows have been added or
     * or not. The variable should be used to ensure that this class is not
     * modified once all the traffic flows have been added.
     * 
     */
    bool built_traffic_flows;

    /**
     * @brief Stores the routes that were found by the routing algorithm for
     * all traffic flows within the NoC. This is initialized after all the
     * traffic flows have been added. This datastructure should be used
     * to store the path found whenever a traffic flow needs to be routed/
     * re-routed. Also, this datastructure should be used to access the routed
     * path of a traffic flow. 
     * 
     */
    vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> traffic_flow_routes;

    // private functions

    /**
     * @brief Given a router that is either a source or sink of
     * a traffic flow, the corresponding traffic flow is added
     * to a vector of traffic flows associated to the router.
     * 
     * @param traffic_flow_id A unique id that represents a traffic flow.
     * @param associated_router_id A ClusterBlockId that represents a
     *                             router block. 
     * @param router_associated_traffic_flows A datastructure that stores
     * a vector of traffic flows for a given router block where the traffic
     * flows have the router as a source or sink within the flow.
     *                                        
     */
    void add_traffic_flow_to_associated_routers(NocTrafficFlowId traffic_flow_id, ClusterBlockId associated_router_id);

  public:
    NocTrafficFlows();

    //getters

    /**
     * @return int An integer that represents the number of unique traffic
     * flows within the NoC. 
     */
    int get_number_of_traffic_flows(void) const;

    /**
     * @brief Given a unique id of a traffic flow (t_noc_traffic_flow)
     * retrieve it from the vector of all traffic flows in the design. The
     * retrieved traffic flow cannot be modified but can be used to
     * retrieve information such as the routers involved.
     * 
     * @param traffic_flow_id The unique identifier (NocTrafficFlowId)
     * of the traffic flow to retrieve.
     * @return const t_noc_traffic_flow& The traffic flow represented by
     * the provided identifier.
     */
    const t_noc_traffic_flow& get_single_noc_traffic_flow(NocTrafficFlowId traffic_flow_id) const;

    /**
     * @brief Get a vector of all traffic flows that have a given router
     * block in the clustered netlist as the source (starting point) or sink
     * (destination point) in the flow. If the router block does not have
     * any traffic flows associated to it then NULL is returned. 
     * 
     * @param router_block_id A unique identifier that represents the
     * a router block in the clustered netlist. This router block will
     * be the source or sink router in the retrieved traffic flows.
     * @return const std::vector<NocTrafficFlowId>* A vector of traffic 
     * flows that have the input router block parameter as the source or sink
     * in the flow.
     */
    const std::vector<NocTrafficFlowId>* get_traffic_flows_associated_to_router_block(ClusterBlockId router_block_id) const;

    /**
     * @brief Gets the number of unique router blocks in the
     * clustered netlist that were used within the user provided
     * traffic flows description. 
     * 
     * @return int The total number of unique routers used in
     * the traffic flows provided by the user.
     */
    int get_number_of_routers_used_in_traffic_flows(void);

    /**
     * @brief Gets the routed path of traffic flow. This cannot be
     * modified externally.
     * 
     * @param traffic_flow_id A unique identifier that represents a 
     * traffic flow.
     * @return std::vector<NocLinkId>& A reference to the provided
     * traffic flow's routed path.
     */
    const std::vector<NocLinkId>& get_traffic_flow_route(NocTrafficFlowId traffic_flow_id) const;

    /**
     * @brief Gets the routed path of a traffic flow. The path
     * returned can and is expected to be  modified externally.
     * 
     * @param traffic_flow_id A unique identifier that represents a 
     * traffic flow.
     * @return std::vector<NocLinkId>& A reference to the provided
     * traffic flow's vector of links used from the src to dst.
     */
    std::vector<NocLinkId>& get_mutable_traffic_flow_route(NocTrafficFlowId traffic_flow_id);

    /**
     * @return a vector ([0..num_logical_router-1]) where each entry gives the clusterBlockId
     * of a logical NoC router. Used for fast lookups in the placer.
     */
    const std::vector<ClusterBlockId>& get_router_clusters_in_netlist(void) const;

    /**
     * @return provides access to all traffic flows' ids to allow a range-based
     * loop through all traffic flows, used in noc_place_utils.cpp functions.
     */
    const std::vector<NocTrafficFlowId>& get_all_traffic_flow_id(void) const;

    // setters

    /**
     * @brief Given a set of parameters that specify a traffic
     * flow, create and add the specified traffic flow it to the vector of
     * flows in the design. 
     * 
     * Finally, the newly created traffic flow is
     * also added to internal datastructures that can be used to quickly
     * look up which traffic flows contain a specific router cluster block.
     * 
     * @param source_router_module_name A string that represents the
     * name of the source router block in the traffic flow. This is
     * provided by the user.
     * @param sink_router_module_name A string that represents the name
     * of the sink router block in the traffic flow. This is provided by
     * the user.
     * @param source_router_cluster_id The source router block id that
     * uniquely identifies this block in the clustered netlist.
     * @param sink_router_cluster_id  The sink router block id that
     * uniquely identifier this block in the clustered netlist.
     * @param traffic_flow_bandwidth The size of the data transmission
     * in this traffic flow (units of bps).
     * @param traffic_flow_latency The maximum allowable delay between
     * transmitting data at the source router and having it received
     * at the sink router.
     * @param traffic_flow_priority The importance of a given traffic flow.
     */
    void create_noc_traffic_flow(const std::string& source_router_module_name, const std::string& sink_router_module_name, ClusterBlockId source_router_cluster_id, ClusterBlockId sink_router_cluster_id, double traffic_flow_bandwidth, double traffic_flow_latency, int traffic_flow_priority);

    /**
     * @brief Copies the passed in router_cluster_id_in_netlist vector to the
     * private internal vector.
     *
     * @param routers_cluster_id_in_netlist A vector ([0..num_logical_routers-1]) containing all routers'
     * ClusterBlockId extracted from netlist.
     *
     */
    void set_router_cluster_in_netlist(const std::vector<ClusterBlockId>& routers_cluster_id_in_netlist);

    //utility functions

    /**
     * @brief Indicates that the class has been fully constructed, meaning
     * that all the traffic flows have been added and cannot be added anymore.
     * This function should be called only after adding all the traffic flows
     * provided by the user. Additionally, creates the storage space for storing
     * the routed paths for all traffic flows.
     * 
     */

    void finished_noc_traffic_flows_setup(void);

    /**
     * @brief Resets the class by clearing internal
     * datastructures.
     * 
     */
    void clear_traffic_flows(void);

    /**
     * @brief Given a block from the clustered netlist, determine
     * if the block has traffic flows that it is a part of. There are
     * three possible cases seen by this function. Case 1 is when the
     * block is not a router. Case 2 is when the block is a router and
     * has not traffic flows it is a part of. And finally case three is
     * when the block is a router and has traffic flows it is a part of.
     * This function should be used during placement when clusters are
     * moved or placed. This function can indicate if the moved cluster
     * needs traffic flows to be re-routed. If a cluster is a part of a 
     * traffic flow, then this means that the cluster is either the
     * source or sink router of the traffic flow.
     * 
     * @param block_id A unique identifier that represents a cluster
     * block in the clustered netlist
     * @return true The block has traffic flows that it is a part of
     * @return false The block has no traffic flows it is a prt of
     */
    bool check_if_cluster_block_has_traffic_flows(ClusterBlockId block_id) const;

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