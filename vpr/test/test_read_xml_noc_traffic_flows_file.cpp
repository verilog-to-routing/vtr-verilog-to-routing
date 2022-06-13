#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "read_xml_noc_traffic_flows_file.h"

#include <random>

namespace{

    TEST_CASE("test_verify_traffic_flow_router_modules", "[vpr_noc_traffic_flows_parser]"){

        // filler data for the xml information
        // data for the xml parsing
        pugi::xml_node test;
        pugiutil::loc_data test_location;
       

        SECTION("Test case where atleast one of the input strings for the source and destination router module name is empty"){

            std::string src_router_name = "";
            std::string dst_router_name = "test";

            REQUIRE_THROWS_WITH(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location), "Invalid names for the source and destination NoC router modules.");

        }
        SECTION("Test case where the router module names for both the source and destination routers are the same"){

            std::string src_router_name = "same_router";
            std::string dst_router_name = "same_router";

            REQUIRE_THROWS_WITH(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location), "Source and destination NoC routers cannot be the same modules.");
        }
        SECTION("Test case where the source and destination router module names are legeal"){

            std::string src_router_name = "source_router";
            std::string dst_router_name = "destination_router";

            REQUIRE_NOTHROW(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location));

        }
    }
    TEST_CASE("test_verify_traffic_flow_properties", "[vpr_noc_traffic_flows_parser]"){

        // filler data for the xml information
        // data for the xml parsing
        pugi::xml_node test;
        pugiutil::loc_data test_location;

        SECTION("Test case where the noc traffic flow properties are illegal"){

            double test_traffic_flow_bandwidth = 1.5;
            // illegal value
            double test_max_traffic_flow_latency = -1.5;

            REQUIRE_THROWS_WITH(verify_traffic_flow_properties(test_traffic_flow_bandwidth, test_max_traffic_flow_latency, test, test_location), "The traffic flow bandwidth and latency constraints need to be positive values.");
        }
        SECTION("Test case where the noc traffic flows properties are legal"){

            double test_traffic_flow_bandwidth = 1.5;
            double test_max_traffic_flow_latency = 1.5;

            REQUIRE_NOTHROW(verify_traffic_flow_properties(test_traffic_flow_bandwidth, test_max_traffic_flow_latency, test, test_location));
        }
    }
    TEST_CASE("test_get_router_module_cluster_id", "[vpr_noc_traffic_flows_parser]"){
        
        // filler data for the xml information
        // data for the xml parsing
        pugi::xml_node test;
        pugiutil::loc_data test_location;

        // datastructure to keep track of blocks name to its id
        std::map<std::string, ClusterBlockId> block_id_from_name;

        // get the global netlist
        ClusteringContext& cluster_ctx = g_vpr_ctx.mutable_clustering();
        ClusteredNetlist* test_netlist = &cluster_ctx.clb_nlist;

        // create a new clustered netlist
        *test_netlist = ClusteredNetlist("test_netlist", "77");

        /* need to create the noc router physical type */
        
        t_physical_tile_type noc_router;
        
        // create a single subtile
        t_sub_tile router_tile;
        
        // there are two logical router types that are compatible with this subtile
        t_logical_block_type router_block;
        char router[] = "router";
        router_block.name = router;
        t_logical_block_type_ptr router_ref = &router_block;
        t_pb router_pb;

        // second logical router block
        t_logical_block_type router_block_2;
        char router_2[] = "router_2";
        router_block_2.name = router_2;
        t_logical_block_type_ptr router_ref_2 = &router_block_2;
        t_pb router_2_pb;
 
        // add these two logical tyes as the equivalent sites of the subtile
        router_tile.equivalent_sites.push_back(router_ref);
        router_tile.equivalent_sites.push_back(router_ref_2);

        // add the subtile to the physical noc router type
        noc_router.sub_tiles.push_back(router_tile);

        // create a reference to the physical type
        t_physical_tile_type_ptr noc_router_ref = &noc_router;

        /* finished creating noc router physical type */

        // creating an IO logical type
        t_logical_block_type i_o_block;
        char i_o[] = "IO";
        i_o_block.name = i_o;
        t_logical_block_type_ptr i_o_ref = &i_o_block;
        t_pb io_pb;

        // create some sample IO blocks in the clustered netlist
        // These will act as fillers to make sure that the find block function correctly handles a netlist with different types of blocks
        
        // the block names
        char io_port_one[] = "io_port_one";
        char io_port_two[] = "io_port_two";
        char io_port_three[] = "io_port_three";
        char io_port_four[] = "io_port_four";

        // add the io blocks to the netlist
        block_id_from_name.emplace(io_port_one, test_netlist->create_block(io_port_one, &io_pb, i_o_ref));
        block_id_from_name.emplace(io_port_two, test_netlist->create_block(io_port_two, &io_pb, i_o_ref));
        block_id_from_name.emplace(io_port_three, test_netlist->create_block(io_port_three, &io_pb, i_o_ref));
        block_id_from_name.emplace(io_port_four, test_netlist->create_block(io_port_four, &io_pb, i_o_ref));

        SECTION("Test case where the block is found in the clustered netlist"){

            // create names for some router blocks
            char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";
            char router_two[] = "router:noc_router_two|flit_out_two[0]~reg0";
            char router_three[] = "router:noc_router_three|flit_out_two[0]~reg0";
            char router_four[] = "router:noc_router_four|flit_out_two[0]~reg0";

            // add the router blocks
            block_id_from_name.emplace(router_one, test_netlist->create_block(router_one, &router_pb, router_ref));
            block_id_from_name.emplace(router_two, test_netlist->create_block(router_two, &router_pb, router_ref));
            block_id_from_name.emplace(router_three, test_netlist->create_block(router_three, &router_pb, router_ref));
            block_id_from_name.emplace(router_four, test_netlist->create_block(router_four, &router_pb, router_ref));

            // create additional router blocks
            char router_five[] = "router:noc_router_five|flit_out_two[0]~reg0";
            char router_six[] = "router:noc_router_six|flit_out_two[0]~reg0";

            block_id_from_name.emplace(router_five, test_netlist->create_block(router_five, &router_2_pb, router_ref_2));
            block_id_from_name.emplace(router_six, test_netlist->create_block(router_six, &router_2_pb, router_ref_2));

            // now find a block just knowing its instance name
            std::string test_router_module_name = ".*noc_router_five.*";

            // now get the cluster id of the block with the test router name using the function we are testing
            ClusterBlockId test_router_block_id;
            REQUIRE_NOTHROW(test_router_block_id = get_router_module_cluster_id(test_router_module_name, cluster_ctx, test, test_location, noc_router_ref));

            REQUIRE((size_t)(block_id_from_name.find("router:noc_router_five|flit_out_two[0]~reg0")->second) == (size_t)test_router_block_id);
        }
        SECTION("Test case where the block is not found in the clustered netlist"){

            // create names for some router blocks
            char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";
            char router_two[] = "router:noc_router_two|flit_out_two[0]~reg0";
            char router_three[] = "router:noc_router_three|flit_out_two[0]~reg0";
            char router_four[] = "router:noc_router_four|flit_out_two[0]~reg0";

            // add the router blocks
            block_id_from_name.emplace(router_one, test_netlist->create_block(router_one, &router_pb, router_ref));
            block_id_from_name.emplace(router_two, test_netlist->create_block(router_two, &router_pb, router_ref));
            block_id_from_name.emplace(router_three, test_netlist->create_block(router_three, &router_pb, router_ref));
            block_id_from_name.emplace(router_four, test_netlist->create_block(router_four, &router_pb, router_ref));

            // create additional router blocks
            char router_five[] = "router:noc_router_five|flit_out_two[0]~reg0";
            char router_six[] = "router:noc_router_six|flit_out_two[0]~reg0";

            block_id_from_name.emplace(router_five, test_netlist->create_block(router_five, &router_2_pb, router_ref_2));
            block_id_from_name.emplace(router_six, test_netlist->create_block(router_six, &router_2_pb, router_ref_2));

            // now find a block just knowing its name. Choosing a block name that doesn't exist
            std::string test_router_module_name = "^router:noc_router_seven|flit_out_two[0]~reg0$";

            // now get the cluster id of the block with the test router name using the function we are testing
            // This should fail, so check that it does
            REQUIRE_THROWS_WITH(get_router_module_cluster_id(test_router_module_name, cluster_ctx, test, test_location, noc_router_ref), "The router module '^router:noc_router_seven|flit_out_two[0]~reg0$' does not exist in the design.");
        }
    }
    TEST_CASE("test_check_traffic_flow_router_module_type", "[vpr_noc_traffic_flows_parser]"){
        
        // filler data for the xml information
        // data for the xml parsing
        pugi::xml_node test;
        pugiutil::loc_data test_location;

        // get the global netlist
        ClusteringContext& cluster_ctx = g_vpr_ctx.mutable_clustering();
        ClusteredNetlist* test_netlist = &cluster_ctx.clb_nlist;

        // create a new clustered netlist
        *test_netlist = ClusteredNetlist("test_netlist", "77");

        /* need to create the noc router physical type */
        
        t_physical_tile_type noc_router;
        
        // create a single subtile
        t_sub_tile router_tile;
        
        // there is a single logical type that is compatible with this subtile and it is a router
        t_logical_block_type router_block;
        char router[] = "router";
        router_block.name = router;
        t_logical_block_type_ptr router_ref = &router_block;
        t_pb router_pb;
 
        // add the logical tyes as the equivalent sites of the subtile
        router_tile.equivalent_sites.push_back(router_ref);

        // add the subtile to the physical noc router type
        noc_router.sub_tiles.push_back(router_tile);

        // create a reference to the physical type
        t_physical_tile_type_ptr noc_router_ref = &noc_router;

        /* finished creating noc router physical type */

        // need to add the physical type to the logical block types equivalent tiles
        router_block.equivalent_tiles.push_back(noc_router_ref); 

        // creating an IO logical type
        t_logical_block_type i_o_block;
        char i_o[] = "IO";
        i_o_block.name = i_o;
        t_logical_block_type_ptr i_o_ref = &i_o_block;
        t_pb io_pb;

        SECTION("Test case where the traffic flow module is of type router"){
            // create a name for a router block
            char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";

            // create a cluster block that represents a router module
            ClusterBlockId router_module_id = test_netlist->create_block(router_one, &router_pb, router_ref);

            // now run the test function to verify that the current router module has a logical type of a router
            // the function should not fail since the module is a router
            REQUIRE_NOTHROW(check_traffic_flow_router_module_type(router_one, router_module_id, test, test_location, cluster_ctx, noc_router_ref));
        }
        SECTION("Test case where the traffic flow module is not of type router"){
            // create a name for a IO block
            char io_block_one[] = "io_block_one";

            // create a cluster blcok that represents a IO block
            ClusterBlockId io_module_id = test_netlist->create_block(io_block_one, &io_pb, i_o_ref);

            // now run the test function to verify that the current IO module doesnt have a logical type of a router
            // the function should faile since the module is of type IO
            REQUIRE_THROWS_WITH(check_traffic_flow_router_module_type(io_block_one, io_module_id, test, test_location, cluster_ctx, noc_router_ref), "The supplied module name 'io_block_one' is not a NoC router.");
        }
    }
    TEST_CASE("test_check_that_all_router_blocks_have_an_associated_traffic_flow", "[vpr_noc_traffic_flows_parser]"){

        // get the global netlist
        ClusteringContext& cluster_ctx = g_vpr_ctx.mutable_clustering();
        ClusteredNetlist* test_netlist = &cluster_ctx.clb_nlist;

        // create a new clustered netlist
        *test_netlist = ClusteredNetlist("test_netlist", "77");

        // get the global device information
        DeviceContext& device_ctx = g_vpr_ctx.mutable_device();

        // get the global noc information
        NocContext& noc_ctx = g_vpr_ctx.mutable_noc();
        // delete any previously created traffic flow info
        noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();

        // create the logical type of a noc router
        // there is a single logical type that is compatible with this subtile and it is a router
        t_logical_block_type router_block;
        char router[] = "router";
        router_block.name = router;
        t_logical_block_type_ptr router_ref = &router_block;
        t_pb router_pb;

        //set the index of the logical type as 0 (its the only block type in this test device)
        router_block.index = 0;
        // now add this logical type to the device
        device_ctx.logical_block_types.push_back(router_block);

        //need to create the noc router physical type
        t_physical_tile_type noc_router;

        // indicate that the noc_router physical tile is not an input/output
        noc_router.is_input_type = false;
        noc_router.is_output_type = false;
        
        // create a single subtile
        t_sub_tile router_tile;

        // add the logical tyes as the equivalent sites of the subtile
        router_tile.equivalent_sites.push_back(router_ref);

        // add the subtile to the physical noc router type
        noc_router.sub_tiles.push_back(router_tile);

        // create a reference to the physical type
        t_physical_tile_type_ptr noc_router_ref = &noc_router;
        // need to add the physical type of the router to the list of physical tiles that match to the router logical block
        router_block.equivalent_tiles.push_back(noc_router_ref);

        // define arbritary values for traffic flow bandwidths and latency
        double traffic_flow_bandwidth = 0.0;
        double traffic_flow_latency = 0.0;

        // start by creating a set of router blocks in the design and add them to the clustered netlist

        // define the test router block names
        char router_one[] = "router_block_one";
        char router_two[] = "router_block_two";
        char router_three[] = "router_block_three";

        ClusterBlockId router_block_one_id = test_netlist->create_block(router_one, &router_pb, router_ref);
        ClusterBlockId router_block_two_id = test_netlist->create_block(router_two, &router_pb, router_ref);
        ClusterBlockId router_block_three_id = test_netlist->create_block(router_three, &router_pb, router_ref);

        // define the name of the test noc traffic flows file
        std::string test_noc_traffic_flows_file_name = "noc_traffic_flows_file.flows";

        SECTION("Test case when all router blocks in the design have an associated traffic flow"){ 
            
            // create a number of traffic flows that include all router blocks in the design
            noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_one, router_two, router_block_one_id, router_block_two_id, traffic_flow_bandwidth, traffic_flow_latency);

            noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_two, router_three, router_block_two_id, router_block_three_id, traffic_flow_bandwidth, traffic_flow_latency);

            noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_three, router_one, router_block_three_id, router_block_one_id, traffic_flow_bandwidth, traffic_flow_latency);

            // now check to see whether all router blocks in the design have an associated traffic flow (this is the function tested here)
            // we expect this to pass 
            CHECK(check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_ref, test_noc_traffic_flows_file_name) == true);
        }
        SECTION("Test case where some router blocks in the design do not have an associated traffic flow"){

            // create a number of traffic flows that includes router_one and router_twp but does not include router_three
            noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_one, router_two, router_block_one_id, router_block_two_id, traffic_flow_bandwidth, traffic_flow_latency);

            noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_two, router_one, router_block_two_id, router_block_one_id, traffic_flow_bandwidth, traffic_flow_latency);

            // now check to see whether all router blocks in the design have an associated traffic flow (this is the function tested here)
            // we expect this fail 
            CHECK(check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_ref, test_noc_traffic_flows_file_name) == false);
        }
    }
}