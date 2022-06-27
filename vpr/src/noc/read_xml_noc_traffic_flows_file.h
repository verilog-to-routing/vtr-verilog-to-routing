#ifndef READ_XML_NOC_TRAFFIC_FLOWS_FILE_H
#define READ_XML_NOC_TRAFFIC_FLOWS_FILE_H

/**
 *  @brief The purpose of this file is to read and parse an xml file that has 
 *         then a '.flows' extension. THis file contains the description of
 *         a number of traffic flows within the NoC. A traffic flow describes
 *         the communication between two routers in the NoC.
 * 
 *         All the processed traffic flows are stored inside the 
 *         NocTrafficFlows class for future use.
 * 
 *         'read_xml_noc_traffic_flows_file' is the main function that performs 
 *         all the tasks listed above and should be used externally to process
 *         the traffic flows file. This file also contains a number of internal
 *         helper functions that assist in parsing the traffic flows file.
 * 
 */

#include "pugixml.hpp"
#include "pugixml_util.hpp"
#include "read_xml_util.h"
#include "globals.h"

#include "vtr_assert.h"
#include "vtr_util.h"
#include "ShowSetup.h"
#include "vpr_error.h"

#include "noc_data_types.h"
#include "noc_traffic_flows.h"

#include <iostream>
#include <vector>
#include <string>

// identifier when an integer conversion failed while reading an attribute value in an xml file
constexpr int NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE = -1;

/**
 * @brief Parse an xml '.flows' file that contains a number of traffic
 *        in the NoC. A traffic flow is a communication between one router
 *        in the NoC to another. The XML file contains a number of these traffic
 *        flows and provides additional information about them, such as the
 *        size of data being tranferred and constraints on the latency of the
 *        data transmission. Once the traffic flows are parsed, they are stored
 *        inside the NocTrafficFlows class.
 * 
 * @param noc_flows_file Name of the noc '.flows' file
 */
void read_xml_noc_traffic_flows_file(const char* noc_flows_file);

/**
 * @brief Takes a <single_flow> tag from the '.flows' file and extracts the
 *        flow information from it. This includes the two routers modules
 *        in the flow, the size of the data transferred and any latency
 *        constraints on the data transmission. The parsed information
 *        is verified and if it is legal then this traffic flow is added
 *        to the NocTrafficFlows class.
 * 
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 * @param cluster_ctx Global variable that contains clustering information. Used
 *                    to get information about the router blocks int he design.
 * @param noc_ctx  Global variable that contains NoC information. Used to access
 *                 the NocTrafficFlows class and store current flow.
 * @param noc_router_tile_type The physical type of a Noc router tile in the
 *                             FPGA.
 */
void process_single_flow(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type);

/**
 * @brief Checks to see that the two router module names provided in the 
 *        traffic flow description are not empty and they dont have the
 *        same names. THe two routers cant be the exact same since a router
 *        cannot communicate with itself.
 * 
 * @param source_router_name A string value of the source router module name
 * @param sink_router_name A string value of the sink router
 *                                module name
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 */
void verify_traffic_flow_router_modules(std::string source_router_name, std::string sink_router_name, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data);

/**
 * @brief Ensures that the a given traffic flows data transmission size and
 *        latency constraints are not negative values.
 * 
 * @param traffic_flow_bandwidth The transmission size betwee the two routers
 *                               in the traffic flow.
 * @param max_traffic_flow_latency The allowable latency for the data
 *                                 transmission between the two routers in the
 *                                 traffic flow.
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 */
void 
verify_traffic_flow_properties(double traffic_flow_bandwidth, double max_traffic_flow_latency, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data);

/**
 * @brief Given a router module name in the design, retrieve the
 *        equivalent clustered router block identifier, which is
 *        a ClusterBlockId.
 * 
 * @param router_module_name The name of the router module in the design for
 *                           which the corresponding block id needs to be found.
 * @param cluster_ctx Global variable that contains clustering information.
 *                    Contains a datastructure to convert a module name to
 *                    a cluster block id.
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 * @return ClusterBlockId The corresponding router block id of the provided
 *         router module name.
 */
ClusterBlockId get_router_module_cluster_id(std::string router_module_name, const ClusteringContext& cluster_ctx, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, t_physical_tile_type_ptr noc_router_tile_type);

/**
 * @brief Checks to see whether a given router block is compatible with a NoC
 *        router tile, this helps determine whether the router block is a router
 *        or not (the user provided a name for another type of block in the
 *        design).
 * 
 * @param router_module_name Name of the router module that we are trying to
 *                           check whether it is of type router.
 * @param router_module_id The ClusterBlockId of the router block we are trying
 *                         to check if its of type router. 
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 * @param cluster_ctx Global variable that contains clustering information.
 *                    Contains a datastructure to get the logical type of a 
 *                    router cluster block.
 * @param noc_router_tile_type The physical type of a Noc router tile in the
 *                             FPGA. Used to check if the router block is
 *                             compatible with a router tile.
 */
void check_traffic_flow_router_module_type(std::string router_module_name, ClusterBlockId router_module_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type);

/**
 * @brief Retreives the physical type of a noc router tile.
 * 
 * @param device_ctx Contains the device information. Has a datastructure that
 *                   can determine a tile type based on grid position on the 
 *                   FPGA. 
 * @param noc_ctx Contains the NoC information. Used to get the grid position
 *                of a NoC router tile.
 * @return t_physical_tile_type_ptr The physical type of a NoC router tile
 */
t_physical_tile_type_ptr get_physical_type_of_noc_router_tile(const DeviceContext& device_ctx, NocContext& noc_ctx);

/**
 * @brief Verify that every router module in the design has an associated
 *        traffic flow to it. If a router module was instantiated in a design
 *        then it should be part of a traffic flow as either a source or
 *        sink router. If the module is not then we need to throw an
 *        error. 
 * 
 * @param noc_ctx Contains the NoC information. Used to get the total number
 *                unique routers found in all traffic flows.
 * @param noc_router_tile_type The physical type of a Noc router tile in the
 *                             FPGA. Used to get the logical types of all
 *                             clustered router blocks in the that can be placed
 *                             on a NoC router tile.
 * @param noc_flows_file The name of the '.flows' file. Used when displaying
 *                       the error.
 * @return bool True implies that all router blocks in the design have an
 *              associated traffic flow. False means there are some router
 *              blocks that do not have a an associated traffic flow.
 */
bool check_that_all_router_blocks_have_an_associated_traffic_flow(NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type, std::string noc_flows_file);

/**
 * @brief Given a traffic flow source and sink router, check to see if there
 *        already exists another traffic flow with the same source and sink
 *        routers. If there already exsists a traffic flow with the same routers
 *        then this is a duplicate and an error is thrown, otherwise no
 *        duplicate traffic flow exists.
 * 
 * @param source_router_id The cluster block id of the source router in 
 *                         the traffic flow to be added.
 * @param sink_router_id The cluster block id of the sink router in the
 *                       the traffic flow to be added.
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 * @param noc_traffic_flow_storage Used to get the previously added traffic 
 *                                 traffic flow information.
 */
void check_for_duplicate_traffic_flow(ClusterBlockId source_router_id, ClusterBlockId sink_router_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const NocTrafficFlows& noc_traffic_flow_storage);


#endif