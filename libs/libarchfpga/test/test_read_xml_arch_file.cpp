// test framework
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

// testting statuc functions so include whole source file it is in
#include "read_xml_arch_file.cpp"

TEST_CASE( "Updating router info in arch", "[NoC Arch Tests]" ) {

    std::map<int, std::pair<int, int>> test_router_list;

    std::map<int, std::pair<int, int>>::iterator it;

    // initial conditions 
    int router_id = 1;
    bool router_is_from_connection_list = false;

    // we initially need the map to be empty
    REQUIRE(test_router_list.size() == 0);

    SECTION( "Update the number of declarations for a router for the first time " ) {

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 1);
        REQUIRE(it->second.second == 0);

    }
    SECTION( "Update the number of connections for a router for the first time" ) {

        router_is_from_connection_list = true;

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 0);
        REQUIRE(it->second.second == 1);

    }
    SECTION( "Update the number of declarations for a router when it already exists" ) {

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        // verify that a router was added
        REQUIRE(test_router_list.size() != 0);

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 2);
        REQUIRE(it->second.second == 0);

    }
    SECTION( "Update the number of connections for a router when it already exists" ) {

        router_is_from_connection_list = true;

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        // verify that a router was added
        REQUIRE(test_router_list.size() != 0);

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 0);
        REQUIRE(it->second.second == 2);

    }

}

TEST_CASE( "Verifying a parsed NoC topology", "[NoC Arch Tests]" ) {

    std::map<int, std::pair<int, int>> test_router_list;

    REQUIRE(test_router_list.size() == 0);

    SECTION( "Check the error where a router in the NoC is not connected to other routers." ) {

        // error router
        test_router_list.insert(std::pair<int,std::pair<int, int>>(1, std::pair<int, int>(1,0)));

        // sonme normal routers
        test_router_list.insert(std::pair<int,std::pair<int, int>>(2, std::pair<int, int>(1,5)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(3, std::pair<int, int>(1,6)));

        REQUIRE(test_router_list.size() == 3);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'1' is not connected to any other router in the NoC.");

    }
    SECTION( "Check the error where a router in the NoC is connected to other routers but missing a declaration in the arch file." ) {

        // normal routers
        test_router_list.insert(std::pair<int,std::pair<int, int>>(1, std::pair<int, int>(1,5)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(2, std::pair<int, int>(1,3)));

        // error router
        test_router_list.insert(std::pair<int,std::pair<int, int>>(3, std::pair<int, int>(0,5)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(4, std::pair<int, int>(1,10)));

        REQUIRE(test_router_list.size() == 4);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'3' was found to be connected to another router but missing in the architecture file. Add the router using the <router> tag.");

    }
    SECTION( "Check the error where the router is included more than once in the architecture file." ) {

        // normal routers
        test_router_list.insert(std::pair<int,std::pair<int, int>>(1, std::pair<int, int>(1,5)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(2, std::pair<int, int>(1,3)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(3, std::pair<int, int>(1,10)));

        // error routers
        test_router_list.insert(std::pair<int,std::pair<int, int>>(4, std::pair<int, int>(2,10)));

        // normal routers
        test_router_list.insert(std::pair<int,std::pair<int, int>>(5, std::pair<int, int>(1,3)));

        test_router_list.insert(std::pair<int,std::pair<int, int>>(6, std::pair<int, int>(1,10)));

        REQUIRE(test_router_list.size() == 6);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'4' was included more than once in the architecture file. Routers should only be declared once.");

    }
}