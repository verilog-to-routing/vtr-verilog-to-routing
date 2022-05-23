#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"


#include "clustered_netlist.h"

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

    // need to create the cluster netlist object that will hold the blocks
    ClusteredNetlist test_netlist("test_netlist", "77");

    // need to create the sample names for clusters in the design
    


    

}

}