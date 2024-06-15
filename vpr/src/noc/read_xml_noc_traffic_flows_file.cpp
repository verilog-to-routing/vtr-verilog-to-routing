
#include "read_xml_noc_traffic_flows_file.h"

void read_xml_noc_traffic_flows_file(const char* noc_flows_file) {
    // start by checking that the provided file is a ".flows" file
    if (!vtr::check_file_name_extension(noc_flows_file, ".flows")) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER, "NoC traffic flows file '%s' has an unknown extension. Expecting .flows for NoC traffic flow files.", noc_flows_file);
    }

    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();

    // get the modifiable NoC as we will be adding traffic flows to it here
    NocContext& noc_ctx = g_vpr_ctx.mutable_noc();

    const DeviceContext& device_ctx = g_vpr_ctx.device();

    t_physical_tile_type_ptr noc_router_tile_type = get_physical_type_of_noc_router_tile(device_ctx, noc_ctx);

    /* Get the cluster blocks that are compatible with a physical NoC router
     * tile. These blocks are essentially the "logical routers" that the user
     * instantiated in the HDL design. 
     *
     * This is done so that while parsing the traffic flows file, the cluster
     * identification can be sped up. The traffic flows file provides a string
     * which represents the name of the router modules in the HDL design. Each
     * time the cluster id is needed, the name of the block needs to be 
     * compared to every block in the clustered netlist. This can be very
     * time-consuming, so instead we can compare to only blocks that are
     * compatible to physical NoC router tiles. 
     */
    std::vector<ClusterBlockId> cluster_blocks_compatible_with_noc_router_tiles = get_cluster_blocks_compatible_with_noc_router_tiles(cluster_ctx, noc_router_tile_type);

    /* variable used when parsing the file.
     * Stores xml related information while parsing the file, such as current 
     * line number, current tag and etc. These variables will be used to
     * provide additional information to the user when reporting an error. 
     */
    pugi::xml_document doc;
    pugiutil::loc_data loc_data;

    // go through the file
    try {
        // load the file
        loc_data = pugiutil::load_xml(doc, noc_flows_file);

        // Root tag should be traffic_flows
        auto traffic_flows_tag = pugiutil::get_single_child(doc, "traffic_flows", loc_data);

        // quick check to make sure that we only have single_flow tags within the traffic_flows tag
        pugiutil::expect_only_children(traffic_flows_tag, {"single_flow"}, loc_data);

        // process the individual traffic flows below
        for (pugi::xml_node single_flow : traffic_flows_tag.children()) {
            // current tag is a valid "flow" so process it
            process_single_flow(single_flow, loc_data, cluster_ctx, noc_ctx, noc_router_tile_type, cluster_blocks_compatible_with_noc_router_tiles);
        }

        // insert the clusters to the local collection of all router clusters in the netlist
        noc_ctx.noc_traffic_flows_storage.set_router_cluster_in_netlist(cluster_blocks_compatible_with_noc_router_tiles);

    } catch (pugiutil::XmlError& e) { // used for identifying any of the xml parsing library errors

        vpr_throw(VPR_ERROR_OTHER, noc_flows_file, e.line(), e.what());
    }

    // make sure that all the router modules in the design have an associated traffic flow
    check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_tile_type, noc_flows_file);

    noc_ctx.noc_traffic_flows_storage.finished_noc_traffic_flows_setup();

    // dump out the NocTrafficFlows class information if the user requested it
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_NOC_TRAFFIC_FLOWS)) {
        noc_ctx.noc_traffic_flows_storage.echo_noc_traffic_flows(getEchoFileName(E_ECHO_NOC_TRAFFIC_FLOWS));
    }
}

void process_single_flow(pugi::xml_node single_flow_tag,
                         const pugiutil::loc_data& loc_data,
                         const ClusteringContext& cluster_ctx,
                         NocContext& noc_ctx,
                         t_physical_tile_type_ptr noc_router_tile_type,
                         const std::vector<ClusterBlockId>& cluster_blocks_compatible_with_noc_router_tiles) {
    // contains all traffic flows
    NocTrafficFlows* noc_traffic_flow_storage = &noc_ctx.noc_traffic_flows_storage;

    // an accepted list of attributes for the single flow tag
    std::vector<std::string> expected_single_flow_attributes = {"src", "dst", "bandwidth", "latency_cons", "priority"};

    // check that only the accepted single flow  attributes are found in the tag
    pugiutil::expect_only_attributes(single_flow_tag, expected_single_flow_attributes, loc_data);

    // store the names of the routers part of this traffic flow
    // These names should regex match to a logical router within the clustered netlist
    std::string source_router_module_name = pugiutil::get_attribute(single_flow_tag, "src", loc_data, pugiutil::REQUIRED).as_string();

    std::string sink_router_module_name = pugiutil::get_attribute(single_flow_tag, "dst", loc_data, pugiutil::REQUIRED).as_string();

    //verify whether the router module names are valid non-empty strings
    verify_traffic_flow_router_modules(source_router_module_name, sink_router_module_name, single_flow_tag, loc_data);

    // assign the unique block ids of the two router modules after clustering
    ClusterBlockId source_router_id = get_router_module_cluster_id(source_router_module_name, cluster_ctx, single_flow_tag, loc_data, cluster_blocks_compatible_with_noc_router_tiles);
    ClusterBlockId sink_router_id = get_router_module_cluster_id(sink_router_module_name, cluster_ctx, single_flow_tag, loc_data, cluster_blocks_compatible_with_noc_router_tiles);

    // verify that the source and sink modules are actually noc routers
    check_traffic_flow_router_module_type(source_router_module_name, source_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);
    check_traffic_flow_router_module_type(sink_router_module_name, sink_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);

    // store the properties of the traffic flow
    double traffic_flow_bandwidth = get_traffic_flow_bandwidth(single_flow_tag, loc_data);

    double max_traffic_flow_latency = get_max_traffic_flow_latency(single_flow_tag, loc_data);

    int traffic_flow_priority = get_traffic_flow_priority(single_flow_tag, loc_data);

    verify_traffic_flow_properties(traffic_flow_bandwidth, max_traffic_flow_latency, traffic_flow_priority, single_flow_tag, loc_data);

    // The current flow information is legal, so store it
    noc_traffic_flow_storage->create_noc_traffic_flow(source_router_module_name,
                                                      sink_router_module_name,
                                                      source_router_id,
                                                      sink_router_id,
                                                      traffic_flow_bandwidth,
                                                      max_traffic_flow_latency,
                                                      traffic_flow_priority);
}

double get_traffic_flow_bandwidth(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data) {
    double traffic_flow_bandwidth;
    // holds the bandwidth value as a string so that it can be used to convert to a floating point value (this is done so that scientific notation is supported)
    // there is no default value since this is a required attribute
    // Either it is provided or an error is thrown is not provided or it is an illegal value
    std::string traffic_flow_bandwidth_intermediate_val = pugiutil::get_attribute(single_flow_tag, "bandwidth", loc_data, pugiutil::REQUIRED).as_string();

    // now convert the value to double
    traffic_flow_bandwidth = std::atof(traffic_flow_bandwidth_intermediate_val.c_str());

    return traffic_flow_bandwidth;
}

double get_max_traffic_flow_latency(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data) {
    // "set to large value, indicating no constraint
    double max_traffic_flow_latency = NocTrafficFlows::DEFAULT_MAX_TRAFFIC_FLOW_LATENCY;

    // holds the latency value as a string so that it can be used to convert to a floating point value (this is done so that scientific notation is supported)
    std::string max_traffic_flow_latency_intermediate_val;

    // get the corresponding attribute where the latency constraint is stored
    pugi::xml_attribute max_traffic_flow_latency_attribute = pugiutil::get_attribute(single_flow_tag, "latency_cons", loc_data, pugiutil::OPTIONAL);

    // check if the attribute value was provided
    if (max_traffic_flow_latency_attribute) {
        // value was provided, so store it
        max_traffic_flow_latency_intermediate_val = max_traffic_flow_latency_attribute.as_string();

        // now convert the value to double
        max_traffic_flow_latency = std::atof(max_traffic_flow_latency_intermediate_val.c_str());
    }

    return max_traffic_flow_latency;
}

int get_traffic_flow_priority(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data) {
    // default priority is 1 (indicating that the traffic flow is weighted equal to all others)
    int traffic_flow_priority = 1;

    // get the corresponding attribute where the priority is stored
    pugi::xml_attribute traffic_flow_priority_attribute = pugiutil::get_attribute(single_flow_tag, "priority", loc_data, pugiutil::OPTIONAL);

    // check if the attribute value was provided
    if (traffic_flow_priority_attribute) {
        // value was provided, so store it
        traffic_flow_priority = traffic_flow_priority_attribute.as_int(NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE);
    }

    return traffic_flow_priority;
}

void verify_traffic_flow_router_modules(const std::string& source_router_name, const std::string& sink_router_name, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data) {
    // check that the source router module name is not empty
    if (source_router_name == "") {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Invalid name for the source NoC router module.");
    }
    // check that the sink router module name is not empty
    if (sink_router_name == "") {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Invalid name for the sink NoC router module.");
    }
    // check if the source and sink routers have the same name
    if (source_router_name == sink_router_name) {
        // Cannot have the source and sink routers have the same name (they need to be different). A flow cant go to a single router.
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Source and sink NoC routers cannot be the same modules.");
    }
}

void verify_traffic_flow_properties(double traffic_flow_bandwidth, double max_traffic_flow_latency, int traffic_flow_priority, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data) {
    // check that the bandwidth is a positive value
    if (traffic_flow_bandwidth <= 0.) {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The traffic flow bandwidths are expected to be a non-zero positive floating point or integer values.");
    }

    // check that the latency constraint is also a positive value
    if (max_traffic_flow_latency <= 0.) {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The latency constraints need to be a non-zero positive floating point or integer values.");
    }

    // check that the priority is a positive non-zero value
    if (traffic_flow_priority <= 0) {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The traffic flow priorities expected to be positive, non-zero integer values.");
    }
}

ClusterBlockId get_router_module_cluster_id(const std::string& router_module_name,
                                            const ClusteringContext& cluster_ctx,
                                            pugi::xml_node single_flow_tag,
                                            const pugiutil::loc_data& loc_data,
                                            const std::vector<ClusterBlockId>& cluster_blocks_compatible_with_noc_router_tiles) {
    ClusterBlockId router_module_id = ClusterBlockId::INVALID();

    // Given a regex pattern, use it to match a name of a cluster router block within the clustered netlist. If a matching cluster block is found, then return its cluster block id.
    try {
        router_module_id = cluster_ctx.clb_nlist.find_block_by_name_fragment(router_module_name, cluster_blocks_compatible_with_noc_router_tiles);
    } catch (const std::regex_error& error) {
        // if there was an error with matching the regex string,report it to the user here
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), error.what());
    }

    // check if a valid block id was found
    if (router_module_id == ClusterBlockId::INVALID()) {
        // if here then the module did not exist in the design, so throw an error
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The router module '%s' does not exist in the design.", router_module_name.c_str());
    }

    return router_module_id;
}

void check_traffic_flow_router_module_type(const std::string& router_module_name, ClusterBlockId router_module_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type) {
    // get the logical type of the provided router module
    t_logical_block_type_ptr router_module_logical_type = cluster_ctx.clb_nlist.block_type(router_module_id);

    /*
     * Check whether the current router module logical type is compatible
     * with the physical type of a noc router (can the module be placed on a 
     * noc router tile on the FPGA device). If not then this module is not a
     * router so throw an error.
     */
    if (!is_tile_compatible(noc_router_tile_type, router_module_logical_type)) {
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The supplied module name '%s' is not a NoC router.", router_module_name.c_str());
    }
}

t_physical_tile_type_ptr get_physical_type_of_noc_router_tile(const DeviceContext& device_ctx, NocContext& noc_ctx) {
    // get a reference to a single physical noc router
    // assuming that all routers have the same physical type, so we are just using the physical type of the first router stored within the NoC
    auto physical_noc_router = noc_ctx.noc_model.get_noc_routers().begin();

    // Cannot guarantee that there are any physical routers within the NoC, so check if the NoC has any routers, if it doesn't then throw an error
    VTR_ASSERT(physical_noc_router != noc_ctx.noc_model.get_noc_routers().end());

    //Using the routers grid position go to the device and identify the physical type of the tile located there.
    return device_ctx.grid.get_physical_type({physical_noc_router->get_router_grid_position_x(),
                                              physical_noc_router->get_router_grid_position_y(),
                                              physical_noc_router->get_router_layer_position()});
}

bool check_that_all_router_blocks_have_an_associated_traffic_flow(NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type, const std::string& noc_flows_file) {
    bool result = true;

    // contains the number of all the noc router blocks in the design
    const auto clustered_netlist_stats = ClusteredNetlistStats();

    int number_of_router_blocks_in_design = 0;

    const std::vector<t_sub_tile>* noc_router_subtiles = &(noc_router_tile_type->sub_tiles);

    /*
     * Go through the router subtiles and get the router logical block types the subtiles support. Then determine how many of each router logical block types there are in the clustered netlist. The accumulated sum of all these clusters is the total number of router blocks in the design.    */
    for (const auto & noc_router_subtile : *noc_router_subtiles) {
        for (auto router_logical_block : noc_router_subtile.equivalent_sites) {
            // get the number of logical blocks in the design of the current logical block type
            number_of_router_blocks_in_design += clustered_netlist_stats.num_blocks_type[router_logical_block->index];
        }
    }

    /* Every router block in the design needs to be part of a traffic flow.
     * There can never be a router that isn't part of a traffic flow, otherwise
     * the router is doing nothing. So check that the number of unique routers 
     * in all traffic flows equals the number of router blocks in the design,
     * otherwise throw a warning to let the user know. If there aren't
     * any traffic flows for any routers then the NoC is not being used.
     */
    if (noc_ctx.noc_traffic_flows_storage.get_number_of_routers_used_in_traffic_flows() != number_of_router_blocks_in_design) {
        VTR_LOG_WARN("NoC traffic flows file '%s' does not contain all router modules in the design. Every router module in the design must be part of a traffic flow (communicating to another router). Otherwise the router is unused.\n", noc_flows_file.c_str());

        result = false;
    }

    return result;
}

std::vector<ClusterBlockId> get_cluster_blocks_compatible_with_noc_router_tiles(const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type) {
    // get the ids of all cluster blocks in the netlist
    auto cluster_netlist_blocks = cluster_ctx.clb_nlist.blocks();

    // vector to store all the cluster blocks ids that can be placed within a physical NoC router tile on the FPGA
    std::vector<ClusterBlockId> cluster_blocks_compatible_with_noc_router_tiles;

    for (auto cluster_blk_id : cluster_netlist_blocks) {
        // get the logical type of the block
        t_logical_block_type_ptr cluster_block_type = cluster_ctx.clb_nlist.block_type(cluster_blk_id);

        // check if the current block is compatible with a NoC router tile
        // if it is, then this block is a NoC outer instantiated by the user in the design, so add it to the vector compatible blocks
        if (is_tile_compatible(noc_router_tile_type, cluster_block_type)) {
            cluster_blocks_compatible_with_noc_router_tiles.push_back(cluster_blk_id);
        }
    }

    return cluster_blocks_compatible_with_noc_router_tiles;
}