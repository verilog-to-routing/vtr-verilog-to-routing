#include "catch2/catch_test_macros.hpp"

#include "noc_storage.h"

#include <random>

// controls how many routers are in the noc storage, when testing it
#define NUM_OF_ROUTERS 100


namespace {

    TEST_CASE("test_adding_routers_to_noc_storage", "[vpr_noc]") {

        // setup random number generation
        std::random_device device;
        std::mt19937 rand_num_gen(device());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0,NUM_OF_ROUTERS);

        // create a vector routers that we will use as the golden vector set
        std::vector<NocRouter> golden_set;

        // individual router parameters
        int curr_router_id;
        int router_grid_position_x;
        int router_grid_position_y;

        // testing datastructure
        NocStorage test_noc;

        NocRouterId converted_id;


        // add all the routers to noc_storage and populate the golden router set
        for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++)
        {
            // determine the current router parameters
            curr_router_id = router_number;
            router_grid_position_x = router_number + dist(rand_num_gen);
            router_grid_position_y = router_number + dist(rand_num_gen);

            // add router to the golden vector
            golden_set.emplace_back(router_number, router_grid_position_x, router_grid_position_y);

            // add tje router to the noc
            test_noc.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);

        }

        // now verify that the routers were added properly by reading the routers back from the noc and comparing them to the golden set
        for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++)
        {
            // get the converted router id
            converted_id = (NocRouterId)router_number;

            // compare all the router properties
            REQUIRE(golden_set[router_number].get_router_id() == test_noc.get_noc_router_id(converted_id));

            REQUIRE(golden_set[router_number].get_router_grid_position_x() == test_noc.get_noc_router_grid_position_x(converted_id));

            REQUIRE(golden_set[router_number].get_router_grid_position_y() == test_noc.get_noc_router_grid_position_y(converted_id));
        }

    }
    TEST_CASE("test_router_id_conversion", "[vpr_noc]") {

        // setup random number generation
        std::random_device device;
        std::mt19937 rand_num_gen(device());
        std::uniform_int_distribution<std::mt19937::result_type> dist(0,NUM_OF_ROUTERS);

        // create a vector routers that we will use as the golden vector set
        std::vector<NocRouter> golden_set;

        // individual router parameters
        int curr_router_id;
        int router_grid_position_x;
        int router_grid_position_y;

        // testing datastructure
        NocStorage test_noc;

        NocRouterId converted_id;


        // add all the routers to noc_storage and populate the golden router set
        for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++)
        {
            // determine the current router parameters
            curr_router_id = router_number;
            router_grid_position_x = router_number + dist(rand_num_gen);
            router_grid_position_y = router_number + dist(rand_num_gen);

            // add router to the golden vector
            golden_set.emplace_back(router_number, router_grid_position_x, router_grid_position_y);

            // add tje router to the noc
            test_noc.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);

        }

        // now verify that the routers were added properly by reading the routers back from the noc and comparing them to the golden set
        for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++)
        {
            // get the converted router id
            converted_id = test_noc.convert_router_id(router_number);

            // compare all the router properties
            REQUIRE(golden_set[router_number].get_router_id() == test_noc.get_noc_router_id(converted_id));

            REQUIRE(golden_set[router_number].get_router_grid_position_x() == test_noc.get_noc_router_grid_position_x(converted_id));

            REQUIRE(golden_set[router_number].get_router_grid_position_y() == test_noc.get_noc_router_grid_position_y(converted_id));
        }

    }











}