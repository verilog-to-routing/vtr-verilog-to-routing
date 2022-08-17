#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "read_xml_noc_traffic_flows_file.h"

#include <random>

namespace {

/*
 * Delete all the blocks in the global clustered netlist.
 * Then create an empty clustered netlist and assign it
 * to the globabl clustered netlist.
 */
void free_clustered_netlist(void) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        cluster_ctx.clb_nlist.remove_block(blk_id);
    }

    cluster_ctx.clb_nlist = ClusteredNetlist();
}

/*
 * Delete all the logical block types within the global device.
 */
void free_device(void) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    device_ctx.logical_block_types.clear();
}

TEST_CASE("test_verify_traffic_flow_router_modules", "[vpr_noc_traffic_flows_parser]") {
    // filler data for the xml information
    // data for the xml parsing
    pugi::xml_node test;
    pugiutil::loc_data test_location;

    SECTION("Test case where input string for the source router module name is empty") {
        std::string src_router_name = "";
        std::string dst_router_name = "test";

        REQUIRE_THROWS_WITH(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location), "Invalid name for the source NoC router module.");
    }
    SECTION("Test case where input string for the sink router module name is empty") {
        std::string src_router_name = "test";
        std::string dst_router_name = "";

        REQUIRE_THROWS_WITH(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location), "Invalid name for the sink NoC router module.");
    }
    SECTION("Test case where the router module names for both the source and destination routers are the same") {
        std::string src_router_name = "same_router";
        std::string dst_router_name = "same_router";

        REQUIRE_THROWS_WITH(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location), "Source and sink NoC routers cannot be the same modules.");
    }
    SECTION("Test case where the source and destination router module names are legeal") {
        std::string src_router_name = "source_router";
        std::string dst_router_name = "destination_router";

        REQUIRE_NOTHROW(verify_traffic_flow_router_modules(src_router_name, dst_router_name, test, test_location));
    }
}
TEST_CASE("test_verify_traffic_flow_properties", "[vpr_noc_traffic_flows_parser]") {
    // filler data for the xml information
    // data for the xml parsing
    pugi::xml_node test;
    pugiutil::loc_data test_location;

    SECTION("Test case where the noc traffic flow properties are illegal") {
        double test_traffic_flow_bandwidth = 1.5;
        // illegal value
        double test_max_traffic_flow_latency = -1.5;

        REQUIRE_THROWS_WITH(verify_traffic_flow_properties(test_traffic_flow_bandwidth, test_max_traffic_flow_latency, test, test_location), "The traffic flow bandwidth and latency constraints need to be positive values.");
    }
    SECTION("Test case where the noc traffic flows properties are legal") {
        double test_traffic_flow_bandwidth = 1.5;
        double test_max_traffic_flow_latency = 1.5;

        REQUIRE_NOTHROW(verify_traffic_flow_properties(test_traffic_flow_bandwidth, test_max_traffic_flow_latency, test, test_location));
    }
}
TEST_CASE("test_get_router_module_cluster_id", "[vpr_noc_traffic_flows_parser]") {
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

    // second logical router block
    t_logical_block_type router_block_2;
    char router_2[] = "router_2";
    router_block_2.name = router_2;
    t_logical_block_type_ptr router_ref_2 = &router_block_2;

    // add these two logical tyes as the equivalent sites of the subtile
    router_tile.equivalent_sites.push_back(router_ref);
    router_tile.equivalent_sites.push_back(router_ref_2);

    // add the subtile to the physical noc router type
    noc_router.sub_tiles.push_back(router_tile);

    /* finished creating noc router physical type */

    // creating an IO logical type
    t_logical_block_type i_o_block;
    char i_o[] = "IO";
    i_o_block.name = i_o;
    t_logical_block_type_ptr i_o_ref = &i_o_block;

    // create some sample IO blocks in the clustered netlist
    // These will act as fillers to make sure that the find block function correctly handles a netlist with different types of blocks

    // the block names
    char io_port_one[] = "io_port_one";
    char io_port_two[] = "io_port_two";
    char io_port_three[] = "io_port_three";
    char io_port_four[] = "io_port_four";

    // add the io blocks to the netlist
    block_id_from_name.emplace(io_port_one, test_netlist->create_block(io_port_one, nullptr, i_o_ref));
    block_id_from_name.emplace(io_port_two, test_netlist->create_block(io_port_two, nullptr, i_o_ref));
    block_id_from_name.emplace(io_port_three, test_netlist->create_block(io_port_three, nullptr, i_o_ref));
    block_id_from_name.emplace(io_port_four, test_netlist->create_block(io_port_four, nullptr, i_o_ref));

    // datastructure to store all the cluster block IDs of the noc router logical block type clusters
    std::vector<ClusterBlockId> noc_router_logical_type_clusters;

    SECTION("Test case where the block is found in the clustered netlist") {
        // create names for some router blocks
        char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";
        char router_two[] = "router:noc_router_two|flit_out_two[0]~reg0";
        char router_three[] = "router:noc_router_three|flit_out_two[0]~reg0";
        char router_four[] = "router:noc_router_four|flit_out_two[0]~reg0";

        // add the router blocks
        block_id_from_name.emplace(router_one, test_netlist->create_block(router_one, nullptr, router_ref));
        block_id_from_name.emplace(router_two, test_netlist->create_block(router_two, nullptr, router_ref));
        block_id_from_name.emplace(router_three, test_netlist->create_block(router_three, nullptr, router_ref));
        block_id_from_name.emplace(router_four, test_netlist->create_block(router_four, nullptr, router_ref));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_one)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_two)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_three)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_four)->second);

        // create additional router blocks
        char router_five[] = "router:noc_router_five|flit_out_two[0]~reg0";
        char router_six[] = "router:noc_router_six|flit_out_two[0]~reg0";

        block_id_from_name.emplace(router_five, test_netlist->create_block(router_five, nullptr, router_ref_2));
        block_id_from_name.emplace(router_six, test_netlist->create_block(router_six, nullptr, router_ref_2));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_five)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_six)->second);

        // now find a block just knowing its instance name
        std::string test_router_module_name = ".*noc_router_five.*";

        // now get the cluster id of the block with the test router name using the function we are testing
        ClusterBlockId test_router_block_id;
        REQUIRE_NOTHROW(test_router_block_id = get_router_module_cluster_id(test_router_module_name, cluster_ctx, test, test_location, noc_router_logical_type_clusters));

        REQUIRE((size_t)(block_id_from_name.find("router:noc_router_five|flit_out_two[0]~reg0")->second) == (size_t)test_router_block_id);

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
    SECTION("Test case where the block is not found in the clustered netlist") {
        // create names for some router blocks
        char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";
        char router_two[] = "router:noc_router_two|flit_out_two[0]~reg0";
        char router_three[] = "router:noc_router_three|flit_out_two[0]~reg0";
        char router_four[] = "router:noc_router_four|flit_out_two[0]~reg0";

        // add the router blocks
        block_id_from_name.emplace(router_one, test_netlist->create_block(router_one, nullptr, router_ref));
        block_id_from_name.emplace(router_two, test_netlist->create_block(router_two, nullptr, router_ref));
        block_id_from_name.emplace(router_three, test_netlist->create_block(router_three, nullptr, router_ref));
        block_id_from_name.emplace(router_four, test_netlist->create_block(router_four, nullptr, router_ref));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_one)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_two)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_three)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_four)->second);

        // create additional router blocks
        char router_five[] = "router:noc_router_five|flit_out_two[0]~reg0";
        char router_six[] = "router:noc_router_six|flit_out_two[0]~reg0";

        block_id_from_name.emplace(router_five, test_netlist->create_block(router_five, nullptr, router_ref_2));
        block_id_from_name.emplace(router_six, test_netlist->create_block(router_six, nullptr, router_ref_2));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_five)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_six)->second);

        // now find a block just knowing its name. Choosing a block name that doesn't exist
        std::string test_router_module_name = "^router:noc_router_seven|flit_out_two[0]~reg0$";

        // now get the cluster id of the block with the test router name using the function we are testing
        // This should fail, so check that it does
        REQUIRE_THROWS_WITH(get_router_module_cluster_id(test_router_module_name, cluster_ctx, test, test_location, noc_router_logical_type_clusters), "The router module '^router:noc_router_seven|flit_out_two[0]~reg0$' does not exist in the design.");

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
}
TEST_CASE("test_check_traffic_flow_router_module_type", "[vpr_noc_traffic_flows_parser]") {
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

    SECTION("Test case where the traffic flow module is of type router") {
        // create a name for a router block
        char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";

        // create a cluster block that represents a router module
        ClusterBlockId router_module_id = test_netlist->create_block(router_one, nullptr, router_ref);

        // now run the test function to verify that the current router module has a logical type of a router
        // the function should not fail since the module is a router
        REQUIRE_NOTHROW(check_traffic_flow_router_module_type(router_one, router_module_id, test, test_location, cluster_ctx, noc_router_ref));

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
    SECTION("Test case where the traffic flow module is not of type router") {
        // create a name for a IO block
        char io_block_one[] = "io_block_one";

        // create a cluster blcok that represents a IO block
        ClusterBlockId io_module_id = test_netlist->create_block(io_block_one, nullptr, i_o_ref);

        // now run the test function to verify that the current IO module doesnt have a logical type of a router
        // the function should faile since the module is of type IO
        REQUIRE_THROWS_WITH(check_traffic_flow_router_module_type(io_block_one, io_module_id, test, test_location, cluster_ctx, noc_router_ref), "The supplied module name 'io_block_one' is not a NoC router.");

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
}
TEST_CASE("test_check_that_all_router_blocks_have_an_associated_traffic_flow", "[vpr_noc_traffic_flows_parser]") {
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

    ClusterBlockId router_block_one_id = test_netlist->create_block(router_one, nullptr, router_ref);
    ClusterBlockId router_block_two_id = test_netlist->create_block(router_two, nullptr, router_ref);
    ClusterBlockId router_block_three_id = test_netlist->create_block(router_three, nullptr, router_ref);

    // define the name of the test noc traffic flows file
    std::string test_noc_traffic_flows_file_name = "noc_traffic_flows_file.flows";

    SECTION("Test case when all router blocks in the design have an associated traffic flow") {
        // create a number of traffic flows that include all router blocks in the design
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_one, router_two, router_block_one_id, router_block_two_id, traffic_flow_bandwidth, traffic_flow_latency);

        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_two, router_three, router_block_two_id, router_block_three_id, traffic_flow_bandwidth, traffic_flow_latency);

        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_three, router_one, router_block_three_id, router_block_one_id, traffic_flow_bandwidth, traffic_flow_latency);

        // now check to see whether all router blocks in the design have an associated traffic flow (this is the function tested here)
        // we expect this to pass
        CHECK(check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_ref, test_noc_traffic_flows_file_name) == true);

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();

        // clear the global device
        free_device();
    }
    SECTION("Test case where some router blocks in the design do not have an associated traffic flow") {
        // create a number of traffic flows that includes router_one and router_twp but does not include router_three
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_one, router_two, router_block_one_id, router_block_two_id, traffic_flow_bandwidth, traffic_flow_latency);

        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(router_two, router_one, router_block_two_id, router_block_one_id, traffic_flow_bandwidth, traffic_flow_latency);

        // now check to see whether all router blocks in the design have an associated traffic flow (this is the function tested here)
        // we expect this fail
        CHECK(check_that_all_router_blocks_have_an_associated_traffic_flow(noc_ctx, noc_router_ref, test_noc_traffic_flows_file_name) == false);

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();

        // clear the global device
        free_device();
    }
}
TEST_CASE("test_get_cluster_blocks_compatible_with_noc_router_tiles", "[vpr_noc_traffic_flows_parser]") {
    // get the global netlist
    ClusteringContext& cluster_ctx = g_vpr_ctx.mutable_clustering();
    ClusteredNetlist* test_netlist = &cluster_ctx.clb_nlist;

    // create a new clustered netlist
    *test_netlist = ClusteredNetlist("test_netlist", "77");

    // create the logical type of a noc router
    // there is a single logical type that is compatible with this subtile and it is a router
    t_logical_block_type router_block;
    char router[] = "router";
    router_block.name = router;
    t_logical_block_type_ptr router_ref = &router_block;

    //need to create the noc router physical type
    t_physical_tile_type noc_router;

    // indicate that the noc_router physical tile is not an input/output
    noc_router.is_input_type = false;
    noc_router.is_output_type = false;

    // create a reference to the physical type
    t_physical_tile_type_ptr noc_router_ref = &noc_router;
    // need to add the physical type of the router to the list of physical tiles that match to the router logical block
    router_block.equivalent_tiles.push_back(noc_router_ref);

    // create a second tupe of router logical block
    t_logical_block_type router_block_type_two;
    char router_two_name[] = "router_2";
    router_block_type_two.name = router_two_name;
    t_logical_block_type_ptr router_ref_two = &router_block_type_two;

    // set the router blocks physical type as a noc router
    router_block_type_two.equivalent_tiles.push_back(noc_router_ref);

    // creating an IO logical type
    t_logical_block_type i_o_block;
    char i_o[] = "IO";
    i_o_block.name = i_o;
    t_logical_block_type_ptr i_o_ref = &i_o_block;

    // create a physical tile for I/O blocks
    t_physical_tile_type i_o_block_tile;

    // indicate that the io physical tile is not an input/output. Just for this test
    i_o_block_tile.is_input_type = false;
    i_o_block_tile.is_output_type = false;

    // create a reference to the physical type
    t_physical_tile_type_ptr i_o_block_ref = &i_o_block_tile;
    // need to add the physical type of the io block to the list of physical tiles that match to the io logical block
    router_block.equivalent_tiles.push_back(i_o_block_ref);

    // stores all router cluster blocks within the netlist
    // this is a golden set that the output of this test will be compared to
    std::vector<ClusterBlockId> golden_set_of_router_cluster_blocks_in_netlist;

    SECTION("Test case where all router cluster blocks are correctly identified within the netlist and stored.") {
        // create some sample router blocks and add them to the clustered netlsit

        // create names for some router blocks
        char router_one[] = "noc_router_one";
        char router_two[] = "noc_router_two";
        char router_three[] = "noc_router_three";
        char router_four[] = "noc_router_four";

        // add the router blocks
        golden_set_of_router_cluster_blocks_in_netlist.emplace_back(test_netlist->create_block(router_one, nullptr, router_ref));
        golden_set_of_router_cluster_blocks_in_netlist.emplace_back(test_netlist->create_block(router_two, nullptr, router_ref));
        golden_set_of_router_cluster_blocks_in_netlist.emplace_back(test_netlist->create_block(router_three, nullptr, router_ref_two));
        golden_set_of_router_cluster_blocks_in_netlist.emplace_back(test_netlist->create_block(router_four, nullptr, router_ref_two));

        // stores the found cluster blocks in the netlist that are router blocks which are compatible with a NoC router tile
        // executing the test function here
        std::vector<ClusterBlockId> found_cluster_blocks_that_are_noc_router_compatible = get_cluster_blocks_compatible_with_noc_router_tiles(cluster_ctx, noc_router_ref);

        // check that the correct number of router blocks were found
        REQUIRE(golden_set_of_router_cluster_blocks_in_netlist.size() == found_cluster_blocks_that_are_noc_router_compatible.size());

        // now go through the golden set and check that the router blocks in the golden set were correctly found by the test function
        for (auto golden_set_router_block_id = golden_set_of_router_cluster_blocks_in_netlist.begin(); golden_set_router_block_id != golden_set_of_router_cluster_blocks_in_netlist.end(); golden_set_router_block_id++) {
            // no check that the current router block in the golden set was also found by the test and recognized as being a router logical block
            REQUIRE(std::find(found_cluster_blocks_that_are_noc_router_compatible.begin(), found_cluster_blocks_that_are_noc_router_compatible.end(), *golden_set_router_block_id) != found_cluster_blocks_that_are_noc_router_compatible.end());
        }

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
    SECTION("Test case where non router blocks are correctly identified within the netlist and ignored.") {
        // add some I/O blocks which are not compatible with a physical noc router tile

        // create names for some io blocks
        char io_one[] = "io_one";
        char io_two[] = "io_two";
        char io_three[] = "io_three";
        char io_four[] = "io_four";

        // add the io blocks
        // Note: we do not add these cluster blocks to the golden set since they are not router blocks and incompatible with physical noc router tiles
        test_netlist->create_block(io_one, nullptr, i_o_ref);
        test_netlist->create_block(io_two, nullptr, i_o_ref);
        test_netlist->create_block(io_three, nullptr, i_o_ref);
        test_netlist->create_block(io_four, nullptr, i_o_ref);

        // stores the found cluster blocks in the netlist that are router blocks which are compatible with a NoC router tile
        // execute the test function
        std::vector<ClusterBlockId> found_cluster_blocks_that_are_noc_router_compatible = get_cluster_blocks_compatible_with_noc_router_tiles(cluster_ctx, noc_router_ref);

        // since there were no router blocks in this netlist, check that the test found function 0 blocks that were compatible with a noc router tile
        REQUIRE(found_cluster_blocks_that_are_noc_router_compatible.size() == 0);

        // clear the global netlist datastructure so other unit tests that rely on dont use a corrupted netlist
        free_clustered_netlist();
    }
}
} // namespace