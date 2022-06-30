
#include "read_xml_noc_traffic_flows_file.h"


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

        // quick check to make sure that we only have single_flow tags within the traffic_flows tag
        pugiutil::expect_only_children(traffic_flows_tag, {"single_flow"}, loc_data);

        // process the individual traffic flows below
        for (pugi::xml_node single_flow : traffic_flows_tag.children()){
            // current tag is a valid "flow" so process it
            process_single_flow(single_flow, loc_data, cluster_ctx, noc_ctx, noc_router_tile_type);
        }

    }
    catch(pugiutil::XmlError& e) { // used for identifying any of the xml parsing library errors
        
        vpr_throw(VPR_ERROR_OTHER, noc_flows_file, e.line(), e.what());
    }

    // make sure that all the router modules in the design have an associated traffic flow
    check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_tile_type, noc_flows_file);

    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // dump out the NocTrafficFlows class information if the user requested it
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_NOC_TRAFFIC_FLOWS)) {
        noc_ctx.noc_traffic_flows_storage.echo_noc_traffic_flows(getEchoFileName(E_ECHO_NOC_TRAFFIC_FLOWS));
    }

    return;

}

void process_single_flow(pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type){

    // contains all traffic flows
    NocTrafficFlows* noc_traffic_flow_storage = &noc_ctx.noc_traffic_flows_storage;

    // an accepted set of attributes for the single flow tag
    std::vector<std::string> expected_single_flow_attributes = {"src", "dst", "bandwidth", "latency_cons"};

    // check that only the accepted single flow  attributes are found in the tag
    pugiutil::expect_only_attributes(single_flow_tag, expected_single_flow_attributes, loc_data);
    
    // store the names of the routers part of this traffic flow
    std::string source_router_module_name = pugiutil::get_attribute(single_flow_tag, "src", loc_data, pugiutil::REQUIRED).as_string();
    
    std::string sink_router_module_name = pugiutil::get_attribute(single_flow_tag, "dst", loc_data, pugiutil::REQUIRED).as_string();

    //verify whether the router module names are legal
    verify_traffic_flow_router_modules(source_router_module_name, sink_router_module_name, single_flow_tag, loc_data);

    // assign the unique block ids of the two router modules after clustering
    ClusterBlockId source_router_id = get_router_module_cluster_id(source_router_module_name, cluster_ctx, single_flow_tag, loc_data, noc_router_tile_type);
    ClusterBlockId sink_router_id = get_router_module_cluster_id(sink_router_module_name, cluster_ctx, single_flow_tag, loc_data, noc_router_tile_type);

    // verify that the source and sink modules are actually noc routers
    check_traffic_flow_router_module_type(source_router_module_name, source_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);
    check_traffic_flow_router_module_type(sink_router_module_name, sink_router_id, single_flow_tag, loc_data, cluster_ctx, noc_router_tile_type);

    // store the properties of the traffic flow
    double traffic_flow_bandwidth = pugiutil::get_attribute(single_flow_tag, "bandwidth", loc_data, pugiutil::REQUIRED).as_double(NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE);

    double max_traffic_flow_latency = pugiutil::get_attribute(single_flow_tag, "latency_cons", loc_data, pugiutil::REQUIRED).as_double(NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE);

    verify_traffic_flow_properties(traffic_flow_bandwidth, max_traffic_flow_latency, single_flow_tag, loc_data);

    // make sure that the current traffic flow was not added previously
    check_for_duplicate_traffic_flow(source_router_id, sink_router_id,single_flow_tag, loc_data, *noc_traffic_flow_storage);

    // The current flow information is legal, so store it
    noc_traffic_flow_storage->create_noc_traffic_flow(source_router_module_name, sink_router_module_name, source_router_id, sink_router_id, traffic_flow_bandwidth, max_traffic_flow_latency);

    return;

}

void verify_traffic_flow_router_modules(std::string source_router_name, std::string sink_router_name, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data){

    // check that the router module names were legal
    if ((source_router_name == "") || (sink_router_name == "")){
        
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Invalid names for the source and sink NoC router modules.");
    }// check if the source and sink routers have the same name
    else if (source_router_name == sink_router_name)
    {
        // Cannot have the source and sink routers have the same name (they need to be different). A flow cant go to a single router.
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "Source and sink NoC routers cannot be the same modules.");
    }

    return;

}

void verify_traffic_flow_properties(double traffic_flow_bandwidth, double max_traffic_flow_latency, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data){

    // check that the bandwidth and max latency are positive values
    if ((traffic_flow_bandwidth < 0) || (max_traffic_flow_latency < 0)){
        
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The traffic flow bandwidth and latency constraints need to be positive values.");
    }

    return;
}

ClusterBlockId get_router_module_cluster_id(std::string router_module_name, const ClusteringContext& cluster_ctx, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, t_physical_tile_type_ptr noc_router_tile_type){

    ClusterBlockId router_module_id = ClusterBlockId::INVALID();

    // get the subtiles of the noc router physical type
    const std::vector<t_sub_tile>* noc_router_subtiles = &noc_router_tile_type->sub_tiles; 

    /*
        iterate through all the logical block types that can be placed on the
        NoC router tile. Check if the there is a cluster block that matches
        the input router module name and is one of the supported logical
        block types.
    */
    for (auto sub_tile = noc_router_subtiles->begin(); sub_tile != noc_router_subtiles->end(); sub_tile++){

        //get the logical types the current tile supports
        const std::vector<t_logical_block_type_ptr>* supported_noc_logical_types = &sub_tile->equivalent_sites;

        // go through each logical type and check if there is a cluster block
        // of that type that also matches the input module name
        for (auto logical_type = supported_noc_logical_types->begin(); logical_type != supported_noc_logical_types->end(); logical_type++){
            
            try{
                router_module_id = cluster_ctx.clb_nlist.find_block_with_matching_name(router_module_name, *logical_type);
            }catch(const std::regex_error& error){
                // if there was an error with matching the regex string,report it to the user here
                vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), error.what());
            }

            // found a block for the current logical type, so exit
            if (router_module_id != ClusterBlockId::INVALID()){
                break;
            }

        }
        // found a block for the current sub tile, so exit
        if (router_module_id != ClusterBlockId::INVALID()){
            break;
        }
    
    }

    // check if a valid block id was found
    if (router_module_id == ClusterBlockId::INVALID()){
        // if here then the module did not exist in the design, so throw an error
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The router module '%s' does not exist in the design.", router_module_name.c_str());
    }

    return router_module_id;

}

void check_traffic_flow_router_module_type(std::string router_module_name, ClusterBlockId router_module_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const ClusteringContext& cluster_ctx, t_physical_tile_type_ptr noc_router_tile_type){

    // get the logical type of the provided router module
    t_logical_block_type_ptr router_module_logical_type = cluster_ctx.clb_nlist.block_type(router_module_id);

    /*
        Check whether the current router modules logical type is compatible
        with the physical type of a noc router (can the module be placed on a 
        noc router tile on the FPGA device). If not then this module is not a router so throw an error.
    */
    if (!is_tile_compatible(noc_router_tile_type, router_module_logical_type)){
        
        vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The supplied module name '%s' is not a NoC router.", router_module_name.c_str());
    }
   
    return;

}

t_physical_tile_type_ptr get_physical_type_of_noc_router_tile(const DeviceContext& device_ctx, NocContext& noc_ctx){

    // get a reference to a single physical noc router
    auto physical_noc_router = noc_ctx.noc_model.get_noc_routers().begin();

    
    //Using the routers grid position go to the device and identify the physical type of the tile located there.
    return device_ctx.grid[physical_noc_router->get_router_grid_position_x()][physical_noc_router->get_router_grid_position_y()].type;

}

bool check_that_all_router_blocks_have_an_associated_traffic_flow(NocContext& noc_ctx, t_physical_tile_type_ptr noc_router_tile_type, std::string noc_flows_file){

    bool result = true;

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

        VTR_LOG_WARN("NoC traffic flows file '%s' does not contain all router modules in the design. Every router module in the design must be part of a traffic flow (communicating to another router). Otherwise the router is unused.\n", noc_flows_file.c_str());

        result = false;
    }

    return result;

}

void check_for_duplicate_traffic_flow(ClusterBlockId source_router_id, ClusterBlockId sink_router_id, pugi::xml_node single_flow_tag, const pugiutil::loc_data& loc_data, const NocTrafficFlows& noc_traffic_flow_storage){

    const std::vector<NocTrafficFlowId>*  associated_traffic_flows_to_source_router = noc_traffic_flow_storage.get_traffic_flows_associated_to_source_router(source_router_id);

    // make sure the router is associated to atleast one traffic flow
    if (associated_traffic_flows_to_source_router != nullptr){

        /*
            Go through all the traffic flows that exist with the current source router and check to see if any of those traffic flows have a sink router
            that matches current sink router. When this happens then the two traffic flows are duplicates and an error should be thrown.
        */
        for (auto traffic_flow_id = associated_traffic_flows_to_source_router->begin(); traffic_flow_id != associated_traffic_flows_to_source_router->end(); traffic_flow_id++){
            
            ClusterBlockId curr_sink_router_id = noc_traffic_flow_storage.get_single_noc_traffic_flow(*traffic_flow_id).sink_router_cluster_id;

            // compare the current traffic flows sink router with the new traffic flows sink router
            if (curr_sink_router_id == sink_router_id){
                
                // the routers are the same, so this is a duplicate traffic flow. thrown an error
                vpr_throw(VPR_ERROR_OTHER, loc_data.filename_c_str(), loc_data.line(single_flow_tag), "The supplied traffic flow is a duplicate of another traffic flow (contain the same source and sink routers). Duplicate traffic flows are not allowed.");

            }

        }
    }

    return;
}