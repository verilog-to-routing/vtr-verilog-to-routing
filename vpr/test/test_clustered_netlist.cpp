#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "clustered_netlist.h"

#include <map>
#include <string>

namespace {

TEST_CASE("test_find_block_with_matching_name", "[vpr_clustered_netlist]") {
    // create some sample logical types that we will use when creating new cluster blocks
    t_logical_block_type router_block;
    char router[] = "router";
    router_block.name = router;
    t_logical_block_type_ptr router_ref = &router_block;

    t_logical_block_type i_o_block;
    char i_o[] = "IO";
    i_o_block.name = i_o;
    t_logical_block_type_ptr i_o_ref = &i_o_block;

    t_pb router_pb;
    t_pb i_o_pb;

    // data structure to keep track of blocks name to its id
    std::map<std::string, ClusterBlockId> block_id_from_name;

    // need to create the cluster netlist object that will hold the blocks
    ClusteredNetlist test_netlist("test_netlist", "77");

    // creating some names for i_o_blocks
    // These will act as fillers to make sure that the matching function correctly handles a netlist with different types of blocks

    char io_port_one[] = "io_port_one";
    char io_port_two[] = "io_port_two";
    char io_port_three[] = "io_port_three";
    char io_port_four[] = "io_port_four";

    // data structure to store all the cluster block IDs of the noc router logical block type clusters
    std::vector<ClusterBlockId> noc_router_logical_type_clusters;

    // add the io blocks to the netlist
    block_id_from_name.emplace(io_port_one, test_netlist.create_block(io_port_one, &i_o_pb, i_o_ref));
    block_id_from_name.emplace(io_port_two, test_netlist.create_block(io_port_two, &i_o_pb, i_o_ref));
    block_id_from_name.emplace(io_port_three, test_netlist.create_block(io_port_three, &i_o_pb, i_o_ref));
    block_id_from_name.emplace(io_port_four, test_netlist.create_block(io_port_four, &i_o_pb, i_o_ref));

    SECTION("Test find_block_with_matching_name when the input string is the instance name of the block") {
        // create names for some router blocks
        char router_one[] = "router:noc_router_one";
        char router_two[] = "router:noc_router_two";
        char router_three[] = "router:noc_router_three";
        char router_four[] = "router:noc_router_four";

        // add the routers
        block_id_from_name.emplace(router_one, test_netlist.create_block(router_one, &router_pb, router_ref));
        block_id_from_name.emplace(router_two, test_netlist.create_block(router_two, &router_pb, router_ref));
        block_id_from_name.emplace(router_three, test_netlist.create_block(router_three, &router_pb, router_ref));
        block_id_from_name.emplace(router_four, test_netlist.create_block(router_four, &router_pb, router_ref));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_one)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_two)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_three)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_four)->second);

        // now find a block just knowing its instance name
        // the test names will have an arbritary number of characters in front and then the name of the instance and then maybe some characters after
        std::string test_router_module_name = "(.*)(noc_router_one)(.*)";

        //get the block id
        ClusterBlockId test_router_id = test_netlist.find_block_by_name_fragment(test_router_module_name, noc_router_logical_type_clusters);

        // now check the block id with what we expect to be
        REQUIRE((size_t)(block_id_from_name.find("router:noc_router_one")->second) == (size_t)test_router_id);
    }
    SECTION("Test find_block_with_matching_name when the input string is a unique port name connecting to the block") {
        // create the name of the router blocks
        // the idea here is to create test cases where all the names of the blocks have the same block name but have a unique component identifying them
        // this is a name identifying the net
        char router_one[] = "router:new_router|q_a[1]";
        char router_two[] = "router:new_router|q_a[2]";
        char router_three[] = "router:new_router|q_a[3]";
        char router_four[] = "router:new_router|q_a[4]";

        // add the routers
        block_id_from_name.emplace(router_one, test_netlist.create_block(router_one, &router_pb, router_ref));
        block_id_from_name.emplace(router_two, test_netlist.create_block(router_two, &router_pb, router_ref));
        block_id_from_name.emplace(router_three, test_netlist.create_block(router_three, &router_pb, router_ref));
        block_id_from_name.emplace(router_four, test_netlist.create_block(router_four, &router_pb, router_ref));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_one)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_two)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_three)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_four)->second);

        // now find a block just knowing its unique identifier
        // the test names will have an arbritary number of characters in front of them and the unique identifier at the end
        std::string test_router_module_name = "(.*)(q_a\\[2\\])(.*)";

        //get the block id
        ClusterBlockId test_router_id = test_netlist.find_block_by_name_fragment(test_router_module_name, noc_router_logical_type_clusters);

        // now check the block id with what we expect to be
        REQUIRE((size_t)(block_id_from_name.find("router:new_router|q_a[2]")->second) == (size_t)test_router_id);
    }
    SECTION("Test find_block_with_matching_name when multiple blocks match the input string ") {
        // create the name of the router blocks
        char router_one[] = "router:noc_router_one|flit_out_two[0]~reg0";
        char router_two[] = "router:noc_router_two|flit_out_two[0]~reg0";
        char router_three[] = "router:noc_router_three|flit_out_two[0]~reg0";
        char router_four[] = "router:noc_router_four|flit_out_two[0]~reg0";

        // need to create another block of a different type that has the same name as one of the router blocks
        char i_o_block_with_same_name[] = "io|router:noc_router_four|flit_out_two[0]~reg0";

        // add the routers and the IO block

        // add the IO block with a similiar name
        block_id_from_name.emplace(i_o_block_with_same_name, test_netlist.create_block(i_o_block_with_same_name, &i_o_pb, i_o_ref));

        // add routers
        block_id_from_name.emplace(router_one, test_netlist.create_block(router_one, &router_pb, router_ref));
        block_id_from_name.emplace(router_two, test_netlist.create_block(router_two, &router_pb, router_ref));
        block_id_from_name.emplace(router_three, test_netlist.create_block(router_three, &router_pb, router_ref));
        block_id_from_name.emplace(router_four, test_netlist.create_block(router_four, &router_pb, router_ref));

        // get the added router cluster block ids and store them
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_one)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_two)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_three)->second);
        noc_router_logical_type_clusters.push_back(block_id_from_name.find(router_four)->second);

        // now find a block just knowing its unique identifier
        // THe identifier we use will match with multiple blocks in this test case
        std::string test_router_module_name = "(.*)(noc_router_four\\|flit_out_two\\[0\\]~reg0)$";

        //get the block id
        ClusterBlockId test_router_id = test_netlist.find_block_by_name_fragment(test_router_module_name, noc_router_logical_type_clusters);

        // since we passed in the router logical type, we expect the router block to be the returned id

        // now check the block id with what we expect to be
        REQUIRE((size_t)(block_id_from_name.find("router:noc_router_four|flit_out_two[0]~reg0")->second) == (size_t)test_router_id);
    }
}
} // namespace