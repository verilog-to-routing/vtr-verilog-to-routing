#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "read_xml_noc_traffic_flows_file.h"

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
    













}