// test framework
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

// testting statuc functions so include whole source file it is in
#include "read_xml_arch_file.cpp"

// for comparing floats
#include "vtr_math.h"

TEST_CASE("Updating router info in arch", "[NoC Arch Tests]") {
    std::map<int, std::pair<int, int>> test_router_list;

    std::map<int, std::pair<int, int>>::iterator it;

    // initial conditions
    int router_id = 1;
    bool router_is_from_connection_list = false;

    // we initially need the map to be empty
    REQUIRE(test_router_list.size() == 0);

    SECTION("Update the number of declarations for a router for the first time ") {
        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 1);
        REQUIRE(it->second.second == 0);
    }
    SECTION("Update the number of connections for a router for the first time") {
        router_is_from_connection_list = true;

        update_router_info_in_arch(router_id, router_is_from_connection_list, test_router_list);

        it = test_router_list.find(router_id);

        // check first that the router was newly added to the router databse
        REQUIRE(it != test_router_list.end());

        // no verify the components of the router parameter
        REQUIRE(it->second.first == 0);
        REQUIRE(it->second.second == 1);
    }
    SECTION("Update the number of declarations for a router when it already exists") {
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
    SECTION("Update the number of connections for a router when it already exists") {
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

TEST_CASE("Verifying a parsed NoC topology", "[NoC Arch Tests]") {
    std::map<int, std::pair<int, int>> test_router_list;

    REQUIRE(test_router_list.size() == 0);

    SECTION("Check the error where a router in the NoC is not connected to other routers.") {
        // error router
        test_router_list.insert(std::pair<int, std::pair<int, int>>(1, std::pair<int, int>(1, 0)));

        // sonme normal routers
        test_router_list.insert(std::pair<int, std::pair<int, int>>(2, std::pair<int, int>(1, 5)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(3, std::pair<int, int>(1, 6)));

        REQUIRE(test_router_list.size() == 3);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'1' is not connected to any other router in the NoC.");
    }
    SECTION("Check the error where a router in the NoC is connected to other routers but missing a declaration in the arch file.") {
        // normal routers
        test_router_list.insert(std::pair<int, std::pair<int, int>>(1, std::pair<int, int>(1, 5)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(2, std::pair<int, int>(1, 3)));

        // error router
        test_router_list.insert(std::pair<int, std::pair<int, int>>(3, std::pair<int, int>(0, 5)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(4, std::pair<int, int>(1, 10)));

        REQUIRE(test_router_list.size() == 4);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'3' was found to be connected to another router but missing in the architecture file. Add the router using the <router> tag.");
    }
    SECTION("Check the error where the router is included more than once in the architecture file.") {
        // normal routers
        test_router_list.insert(std::pair<int, std::pair<int, int>>(1, std::pair<int, int>(1, 5)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(2, std::pair<int, int>(1, 3)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(3, std::pair<int, int>(1, 10)));

        // error routers
        test_router_list.insert(std::pair<int, std::pair<int, int>>(4, std::pair<int, int>(2, 10)));

        // normal routers
        test_router_list.insert(std::pair<int, std::pair<int, int>>(5, std::pair<int, int>(1, 3)));

        test_router_list.insert(std::pair<int, std::pair<int, int>>(6, std::pair<int, int>(1, 10)));

        REQUIRE(test_router_list.size() == 6);

        REQUIRE_THROWS_WITH(verify_noc_topology(test_router_list), "The router with id:'4' was included more than once in the architecture file. Routers should only be declared once.");
    }
}

TEST_CASE("Verifying mesh topology creation", "[NoC Arch Tests]") {
    // data for the xml parsing
    pugi::xml_node test;
    pugiutil::loc_data test_location;

    // the noc storage
    t_noc_inf test_noc;

    // mesh parameters
    double mesh_start_x = 10;
    double mesh_start_y = 10;
    double mesh_end_x = 5;
    double mesh_end_y = 56;
    double mesh_size = 0;

    SECTION("Check the error where a mesh size was illegal.") {
        REQUIRE_THROWS_WITH(generate_noc_mesh(test, test_location, &test_noc, mesh_start_x, mesh_end_x, mesh_start_y, mesh_end_y, mesh_size), "The NoC mesh size cannot be 0.");
    }
    SECTION("Check the error where a mesh region size was invalid.") {
        mesh_size = 3;

        REQUIRE_THROWS_WITH(generate_noc_mesh(test, test_location, &test_noc, mesh_start_x, mesh_end_x, mesh_start_y, mesh_end_y, mesh_size), "The NoC region is invalid.");
    }
    SECTION("Check the mesh creation for integer precision coordinates.") {
        // define test parameters
        mesh_size = 3;

        mesh_start_x = 0;
        mesh_start_y = 0;

        mesh_end_x = 4;
        mesh_end_y = 4;

        // create the golden golden results
        double golden_results_x[9];
        double golden_results_y[9];

        // first row of the mesh
        golden_results_x[0] = 0;
        golden_results_y[0] = 0;
        golden_results_x[1] = 2;
        golden_results_y[1] = 0;
        golden_results_x[2] = 4;
        golden_results_y[2] = 0;

        // second row of the mesh
        golden_results_x[3] = 0;
        golden_results_y[3] = 2;
        golden_results_x[4] = 2;
        golden_results_y[4] = 2;
        golden_results_x[5] = 4;
        golden_results_y[5] = 2;

        // third row of the mesh
        golden_results_x[6] = 0;
        golden_results_y[6] = 4;
        golden_results_x[7] = 2;
        golden_results_y[7] = 4;
        golden_results_x[8] = 4;
        golden_results_y[8] = 4;

        generate_noc_mesh(test, test_location, &test_noc, mesh_start_x, mesh_end_x, mesh_start_y, mesh_end_y, mesh_size);

        // go through all the expected routers
        for (int expected_router_id = 0; expected_router_id < (mesh_size * mesh_size); expected_router_id++) {
            // make sure the router ids match
            REQUIRE(test_noc.router_list[expected_router_id].id == expected_router_id);

            // make sure the position of the routers are correct
            // x position
            REQUIRE(golden_results_x[expected_router_id] == test_noc.router_list[expected_router_id].device_x_position);
            // y position
            REQUIRE(golden_results_y[expected_router_id] == test_noc.router_list[expected_router_id].device_y_position);
        }
    }
    SECTION("Check the mesh creation for double precision coordinates.") {
        // define test parameters
        mesh_size = 3;

        mesh_start_x = 3.5;
        mesh_start_y = 5.7;

        mesh_end_x = 10.8;
        mesh_end_y = 6.4;

        // create the golden golden results
        double golden_results_x[9];
        double golden_results_y[9];

        // first row of the mesh
        golden_results_x[0] = 3.5;
        golden_results_y[0] = 5.7;
        golden_results_x[1] = 7.15;
        golden_results_y[1] = 5.7;
        golden_results_x[2] = 10.8;
        golden_results_y[2] = 5.7;

        // second row of the mesh
        golden_results_x[3] = 3.5;
        golden_results_y[3] = 6.05;
        golden_results_x[4] = 7.15;
        golden_results_y[4] = 6.05;
        golden_results_x[5] = 10.8;
        golden_results_y[5] = 6.05;

        // third row of the mesh
        golden_results_x[6] = 3.5;
        golden_results_y[6] = 6.4;
        golden_results_x[7] = 7.15;
        golden_results_y[7] = 6.4;
        golden_results_x[8] = 10.8;
        golden_results_y[8] = 6.4;

        generate_noc_mesh(test, test_location, &test_noc, mesh_start_x, mesh_end_x, mesh_start_y, mesh_end_y, mesh_size);

        // go through all the expected routers
        for (int expected_router_id = 0; expected_router_id < (mesh_size * mesh_size); expected_router_id++) {
            // make sure the router ids match
            REQUIRE(test_noc.router_list[expected_router_id].id == expected_router_id);

            // make sure the position of the routers are correct
            // x position
            REQUIRE(vtr::isclose(golden_results_x[expected_router_id], test_noc.router_list[expected_router_id].device_x_position));
            // y position
            REQUIRE(vtr::isclose(golden_results_y[expected_router_id], test_noc.router_list[expected_router_id].device_y_position));
        }
    }
}