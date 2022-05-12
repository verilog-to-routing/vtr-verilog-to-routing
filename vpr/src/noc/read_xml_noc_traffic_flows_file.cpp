/**
 *  The purpose of this file is to read and parse an xml file that has then
 *  a '.flows' extension. THis file contains the description of a number of
 *  traffic flows within the NoC. A traffic flow describes the communication
 *  between two routers in the NoC.
 * 
 *  All the processed traffic flows are stored inside the NocTrafficFlows class
 *  for future use.
 * 
 *  'read_xml_noc_traffic_flows_file' is the main function that performs all the
 *  tasks listed above and should be used externally to process the traffic
 *  flows file. This file also contains a number of internal helper
 *  functions that assist in parsing the traffic flows file.
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
#include "read_xml_noc_traffic_flows_file.h"


static void process_single_flow(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type);

static void verify_traffic_flow_router_modules(std::string source_router_name, std::string destination_router_name, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data);

static void verify_traffic_flow_properties(double traffic_flow_bandwidth, double max_traffic_flow_latency, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data);

static ClusterBlockId get_router_module_cluster_id(std::string router_module_name, const ClusteringContext& cluster_ctx, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data);

static void check_traffic_flow_router_module_type(std::string router_module_name, ClusterBlockId router_module_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type);

static t_physical_tile_type_ptr get_physical_type_of_noc_router_tile(const DeviceContext& device_ctx, NocContext& noc_ctx);

static void check_that_all_router_blocks_have_an_associated_traffic_flow(NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type, std::string noc_flows_file);

/**
 * @brief Main driver function that parsed the xml '.flows' file which
 *        contains all the traffic flows in the NoC. This function
 *        takes the parsed information and stores it inside the
 *        NocTrafficFlows class.
 * 
 * @param noc_flows_file Name of the '.flows' file
 */
void read_xml_noc_traffic_flows_file(const char* noc_flows_file){

    // start by checking that the provided file is a ".flows" file
    if (vtr::check_file_name_extension(noc_flows_file, ".flows") == false){
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "NoC traffic flows file '%s' has an unknown extension. Expecting .flows for NoC traffic flow files.", noc_flows_file);
    }

    // cluster information
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    // noc information
    NocContext& noc_ctx = g_vpr_ctx.mutable_noc();

    // device information
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // get the physical type of a noc router
    t_physical_tile_type_ptr noc_router_tile_type = get_physical_type_of_noc_router_tile(device_ctx, noc_ctx);

    /* variabled used when parsing the file */
    pugi::xml_document doc;
    pugiutil::loc_data loc_data;

    // go through the file
    try {
        
        // load the file
        loc_data = pugiutil::load_xml(doc, noc_flows_file);

        // Root tag should be traffic_flows
        auto traffic_flows_tag = pugiutil::get_single_child(doc, "traffic_flows", loc_data);

        // process the individual traffic flows below
        for (pugi::xml_node single_flow : traffic_flows_tag.children()){
            // we can only have "single_flow" tags within "traffic_flows" so check that
            if (single_flow.name() != std::string("single_flow")) {
                bad_tag(single_flow, loc_data, traffic_flows_tag, {"single_flow"});
            }
            else {
                // current tag is a valid "flow" so process it
                process_single_flow(single_flow, loc_data, cluster_ctx, noc_ctx, noc_router_tile_type);
            }
        }

    }
    catch(pugiutil::XmlError& e) { // used for identifying any of the xml parsing library errors
        vpr_throw(VPR_ERROR_OTHER, noc_flows_file, e.line(), e.what());
    }

    // make sure that all the router modules in the design have an associated traffic flow
    check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_tile_type, noc_flows_file);

    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    return;

}

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
static void process_single_flow(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type){

    // contans all traffic flows
    NocTrafficFlows* noc_traffic_flow_storage = &noc_ctx.noc_traffic_flows_storage;

    // an accepted list of attributes for the single flow tag
    std::vector<std::string> expected_single_flow_attributes = {"src", "dst", "bandwidth", "latency_cons"};

    // check that only the accepted single flow  attributes are found in the tag
    pugiutil::expect_only_attributes(single_flow_tag, expected_single_flow_attributes, loc_data);
    
    // store the names of the routers part of this traffic flow
    std::string source_router_module_name = pugiutil::get_attribute(single_flow_tag, "src", loc_data, pugiutil::REQUIRED).as_string();
    
    std::string destination_router_module_name = pugiutil::get_attribute(single_flow_tag, "dst", loc_data, pugiutil::REQUIRED).as_string();

    //verify whether the router module names are legal
    verify_traffic_flow_router_modules(source_router_module_name, destination_router_module_name, single_flow_tag, loc_data);

    // assign the unique block ids of the two router modules after clustering
    ClusterBlockId source_router_id = get_router_module_cluster_id(source_router_module_name, cluster_ctx, single_flow_tag, loc_data);
    ClusterBlockId destination_router_id = get_router_module_cluster_id(destination_router_module_name, cluster_ctx, single_flow_tag, loc_data);

    // verify that the source and destination modules are actually noc routers
    check_traffic_flow_router_module_type(source_router_module_name, source_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);
    check_traffic_flow_router_module_type(destination_router_module_name, destination_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);

    // store the properties of the traffic flow
    double traffic_flow_bandwidth = pugiutil::get_attribute(single_flow_tag, "bandwidth", loc_data, pugiutil::REQUIRED).as_double(NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE);

    double max_traffic_flow_latency = pugiutil::get_attribute(single_flow_tag, "latency_cons", loc_data, pugiutil::REQUIRED).as_double(NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE);

    verify_traffic_flow_properties(traffic_flow_bandwidth, max_traffic_flow_latency, single_flow_tag, loc_data);

    // The current flow information is legal, so store it
    noc_traffic_flow_storage->create_noc_traffic_flow(source_router_module_name, destination_router_module_name, source_router_id, destination_router_id, traffic_flow_bandwidth, max_traffic_flow_latency);

    return;

}

/**
 * @brief Checks to see that the two router module names provided in the 
 *        traffic flow description are not empty and they dont have the
 *        same names. THe two routers cant be the exact same since a router
 *        cannot communicate with itself.
 * 
 * @param source_router_name A string value of the source router module name
 * @param destination_router_name A string value of the destination router
 *                                module name
 * @param single_flow_tag A xml tag that contains the traffic flow information
 * @param loc_data Contains location data about the current line in the xml file
 */
static void verify_traffic_flow_router_modules(std::string source_router_name, std::string destination_router_name, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data){

    // check that the router module names were legal
    if ((source_router_name.compare("") == 0) || (destination_router_name.compare("") == 0)){
        
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Invalid names for the source and destination NoC router modules.");
    }// check if the source and destination routers have the same name
    else if (source_router_name.compare(destination_router_name) == 0)
    {
        // Cannot have the source and destination routers have the same name (they need to be different). A flow cant go to a single router.
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Source and destination NoC routers cannot be the same modules.");
    }

    return;

}

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
static void verify_traffic_flow_properties(double traffic_flow_bandwidth, double max_traffic_flow_latency, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data){

    // check that the bandwidth and max latency are positive values
    if ((traffic_flow_bandwidth < 0) || (max_traffic_flow_latency < 0)){
        
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The traffic flow bandwidth and latency constraints need to be positive values.");
    }

    return;
}

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
static ClusterBlockId get_router_module_cluster_id(std::string router_module_name, const ClusteringContext& cluster_ctx, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data){

    ClusterBlockId router_module_id = cluster_ctx.clb_nlist.find_block(router_module_name);

    // check if a valid block id was found
    if ((size_t)router_module_id == (size_t)ClusterBlockId::INVALID){
        // if here then the module did not exist in the design, so throw an error
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The router module '%s' does not exist in the design.", router_module_id);
    }

    return router_module_id;

}

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
static void check_traffic_flow_router_module_type(std::string router_module_name, ClusterBlockId router_module_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type){

    // get the logical type of the provided router module
    t_logical_block_type_ptr router_module_logical_type = cluster_ctx.clb_nlist.block_type(router_module_id);

    /*
        Check of the current router nodules logical type is compatible with the physical type if a noc router (can the module be placed on a noc router tile on the FPGA device). If not then this module is not a router so throw an error.
    */
    if (!is_tile_compatible(noc_router_tile_type, router_module_logical_type)){
        
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "%s", "The supplied module name '%s' is not a NoC router. Found in file: %s, line: %d.", router_module_name, loc_data.filename_c_str(), loc_data.line(single_flow_tag));
    }
   
    return;

}

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
static t_physical_tile_type_ptr get_physical_type_of_noc_router_tile(const DeviceContext& device_ctx, NocContext& noc_ctx){

    // get a reference to a single physical noc router
    auto physical_noc_router = noc_ctx.noc_model.get_noc_routers().begin();

    
    //Using the routers grid position go to the device and identify the physical type of the tile located there.
    return device_ctx.grid[physical_noc_router->get_router_grid_position_x()][physical_noc_router->get_router_grid_position_y()].type;

}

/**
 * @brief Verify that every router module in the design has an associated
 *        traffic flow to it. If a router module was instantiated in a design
 *        then it should be part of a traffic flow as either a source or
 *        destination router. If the module is not then we need to throw an
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
 */
static void check_that_all_router_blocks_have_an_associated_traffic_flow(NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type, std::string noc_flows_file){

    // contains the number of all the noc router blocks in the design
    const auto clustered_netlist_stats = ClusteredNetlistStats();

    int number_of_router_blocks_in_design = 0;

    const std::vector<t_sub_tile>* noc_router_subtiles = &(noc_router_tile_type->sub_tiles);

    /*
        Go through the router subtiles and get the router logical block types the subtiles support. Then determine how many of each router logical block types there are in the clustered netlist. The accumulated sum of all these clusters is the total number of router blocks in the design.    */
    for (auto subtile = noc_router_subtiles->begin(); subtile != noc_router_subtiles->end(); subtile++){

        for (auto router_logical_block = subtile->equivalent_sites.begin(); router_logical_block != subtile->equivalent_sites.end();router_logical_block++){

            // get the number of logical blocks in the design of the current logical block type
            number_of_router_blocks_in_design += clustered_netlist_stats.num_blocks_type[(*router_logical_block)->index];

        }
    }

    /*
        Every router block in the design needs to be part of a traffic flow. There can never be a router that isnt part of a traffic flow, other wise the router is doing nothing. So check that the number of unique routers in all traffic flows equals the number of router blocks in the design, otherwise throw an warning to let the user know. If there aren't
        any traffic flows for any routers then the NoC is not being used.
    */
    if (noc_ctx.noc_traffic_flows_storage.get_number_of_routers_used_in_traffic_flows() != number_of_router_blocks_in_design){

        VTR_LOG_WARN("NoC traffic flows file '%s' does not contain all router modules in the design. Every router module in the design must be part of a traffic flow (communicationg to another router).", noc_flows_file);
    }

    return;

}