#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "xy_routing.h"

namespace {

constexpr double DUMMY_LATENCY = 1e-9;
constexpr double DUMMY_BANDWIDTH = 1e12;

/**
 * @brief Compares two vectors of NocLinks. These vectors represent
 * two routes between a start and destination routers. This function
 * verifies whether the two routers are the exact same or not.
 * 
 */
void compare_routes(const std::vector<NocLink>& golden_path, const std::vector<NocLinkId>& found_path, const NocStorage& noc_model) {
    // make sure that size of the found route and golden route match
    REQUIRE(found_path.size() == golden_path.size());

    // now go through the two routes and make sure that the links in the route match between both routes
    int route_size = golden_path.size();

    for (int link_index = 0; link_index < route_size; link_index++) {
        // get the current link we need to verify from the found route
        const NocLink& found_link = noc_model.get_single_noc_link(found_path[link_index]);

        // now compare the found link to the equivalent link in the golden route.
        // We are just comparing the source and destination routers of the links. We want them to be the same.
        REQUIRE(found_link.get_source_router() == golden_path[link_index].get_source_router());
        REQUIRE(found_link.get_sink_router() == golden_path[link_index].get_sink_router());
    }
}

TEST_CASE("test_route_flow", "[vpr_noc_xy_routing]") {
    /*
     * Creating a test FPGA device below. The NoC itself will be
     * a 4x4 mesh where they will span over a grid size of 4x4. So this FPGA will only consist of NoC routers.
     *
     * This is a map of what the NoC looks like, where the numbers indicate the NoC router id.
     *
     * 12   13   14   15
     * 8    9    10   11
     * 4    5    6    7
     * 0    1    2    3
     *
     */

    // Create the NoC data structure
    NocStorage noc_model;

    // store the reference to device grid with
    // this will be set to the device grid width
    noc_model.set_device_grid_width((int)4);

    // add all the routers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            noc_model.add_router((i * 4) + j, j, i, 0, DUMMY_LATENCY);
        }
    }

    noc_model.make_room_for_noc_router_link_list();

    // add all the links
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) - 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= 3) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) + 4), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= 3) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) + 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) - 4), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
        }
    }

    noc_model.finished_building_noc();

    // creating the XY routing object
    XYRouting routing_algorithm;

    SECTION("Test case where the destination router and the starting routers are the same") {
        // choosing the start and end routers as router 10
        auto start_router_id = NocRouterId(10);
        auto sink_router_id = NocRouterId(10);
        auto traffic_flow_id = NocTrafficFlowId(33);

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // make sure that a legal route was found (no error should be thrown)
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // now make sure that the found route is empty, we shouldn't be moving anywhere as the start and end routers are the same
        REQUIRE(found_path.empty() == true);
    }
    SECTION("Test case where the destination router and the starting routers are located on the same row within the FPGA device.") {
        // choose start router as 7, and choose the destination router as 4
        auto start_router_id = NocRouterId(7);
        auto sink_router_id = NocRouterId(4);
        auto traffic_flow_id = NocTrafficFlowId(34);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path horizontally from router 7 to router 4
        std::vector<NocLink> golden_path;

        for (int current_router = 7; current_router != 4; current_router--) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_router), NocRouterId(current_router - 1));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }
    SECTION("Test case where the destination router and the starting routers are located on the same column within the FPGA device.") {
        // choose start router as 2, and choose the destination router as 14
        auto start_router_id = NocRouterId(2);
        auto sink_router_id = NocRouterId(14);
        auto traffic_flow_id = NocTrafficFlowId(35);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path vertically from router 2 to router 14
        std::vector<NocLink> golden_path;

        for (int current_row = 0; current_row < 3; current_row++) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_row * 4 + 2), NocRouterId((current_row + 1) * 4 + 2));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }
    SECTION("Test case where the destination router and the starting routers are located on different columns and rows within the FPGA device. In this test the path moves left and then up.") {
        // choose start router as 3, and choose the destination router as 14
        auto start_router_id = NocRouterId(3);
        auto sink_router_id = NocRouterId(12);
        auto traffic_flow_id = NocTrafficFlowId(37);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path horizontally from router 3 to router 0 and then vertically from router 0 to 12
        std::vector<NocLink> golden_path;

        // generate the horizontal path first
        for (int current_router = 3; current_router != 0; current_router--) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_router), NocRouterId(current_router - 1));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // generate the vertical path next
        for (int current_row = 0; current_row < 3; current_row++) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_row * 4), NocRouterId((current_row + 1) * 4));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }
    SECTION("Test case where the destination router and the starting routers are located on different columns and rows within the FPGA device. In this test the path moves right and then down.") {
        // The main reason for this test is to verify that the XY routing algorithm correctly traverses in the directions going right and down.
        // These directions had not been tested yet.

        // choose start router as 12, and choose the destination router as 3
        auto start_router_id = NocRouterId(12);
        auto sink_router_id = NocRouterId(3);
        auto traffic_flow_id = NocTrafficFlowId(38);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path horizontally from router 12 to router 15 and then vertically from router 15 to 3
        std::vector<NocLink> golden_path;

        // generate the horizontal path first
        for (int current_router = 12; current_router != 15; current_router++) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_router), NocRouterId(current_router + 1));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // generate the vertical path next
        for (int current_row = 3; current_row > 0; current_row--) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_row * 4 + 3), NocRouterId((current_row - 1) * 4 + 3));
            const auto& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }
}
TEST_CASE("test_route_flow when it fails in a mesh topology.", "[vpr_noc_xy_routing]") {
    /*
     * Creating a test FPGA device below. The NoC itself will be
     * a 4x4 mesh where they will span over a grid size of 4x4. So this FPGA will only consist of NoC routers.
     *
     * This is a map of what the NoC looks like, where the numbers indicate the NoC router id.
     *
     * 12   13   14   15
     * 8    9    10   11
     * 4    5    6    7
     * 0    1    2    3
     *
     */

    // Create the NoC data structure
    NocStorage noc_model;

    // store the reference to device grid with
    // this will be set to the device grid width
    noc_model.set_device_grid_spec((int)4, 0);

    // add all the routers
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            noc_model.add_router((i * 4) + j, j, i, 0, DUMMY_LATENCY);
        }
    }

    noc_model.make_room_for_noc_router_link_list();

    // add all the links
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) - 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= 3) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) + 4), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= 3) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) + 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 4) + j), (NocRouterId)(((i * 4) + j) - 4), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
        }
    }

    noc_model.finished_building_noc();

    // creating the XY routing object
    XYRouting routing_algorithm;

    SECTION("Test case where the xy routing algorithm fails to find a horizontal link to traverse.") {
        /*
         * The route we will test will be starting at router 3 and end at
         * router 0. We will delete the link connecting router 2 to router 1.
         * So the algorithm should fail as it horizontally traverses from router 3 to get to router 0.
         */

        // store the source and destination routers
        auto start_router_id = NocRouterId(3);
        auto sink_router_id = NocRouterId(0);
        auto traffic_flow_id = NocTrafficFlowId(39);

        // routers associated to the link to remove
        auto link_to_remove_src_router_id = NocRouterId(2);
        auto link_to_remove_sink_router_id = NocRouterId(1);

        // start by deleting the link that connects router 2 to router 1
        noc_model.remove_link(link_to_remove_src_router_id, link_to_remove_sink_router_id);

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now use the XY router to find a route, we expect it to fail
        REQUIRE_THROWS_WITH(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model),
                            "No route could be found from starting router with ID:'3' and the destination router with ID:'0' using the XY-Routing algorithm.");
    }
    SECTION("Test case where the xy routing algorithm fails to find a vertical link to traverse.") {
        /*
         * The route we will test will be starting at router 3 and end at
         * router 15. We will delete the link connecting router 11 to router 15.
         * So the algorithm should fail as it vertically traverses from router 3 to get to router 15.
         */

        // store the source and destination routers
        auto start_router_id = NocRouterId(3);
        auto sink_router_id = NocRouterId(15);
        auto traffic_flow_id = NocTrafficFlowId(41);

        // routers associated to the link to remove
        auto link_to_remove_src_router_id = NocRouterId(11);
        auto link_to_remove_sink_router_id = NocRouterId(15);

        // start by deleting the link that connects router 11 to router 15
        noc_model.remove_link(link_to_remove_src_router_id, link_to_remove_sink_router_id);

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now use the XY router to find a route, we expect it to fail
        REQUIRE_THROWS_WITH(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model),
                            "No route could be found from starting router with ID:'3' and the destination router with ID:'15' using the XY-Routing algorithm.");
    }
}
TEST_CASE("test_route_flow when it fails in a non mesh topology.", "[vpr_noc_xy_routing]") {
    /*
     * Creating a test FPGA device below. The NoC itself will be
     * a 3 router design. The purpose of this design is to verify the case where the routing algorithm is stuck in a loop trying to go back and forth between two routers while trying to get to the destination.
     *
     * For example, looking at the example below, suppose we are trying to route between routers 3 and 1. The XY routing algorithm will first traverse to router 0 as it is towards the direction of router 1.
     * But then at router 0 the algorithm will go towards router 3 as its now
     * in the direction of router 1. But then the algorithm will infinitely
     * just ping-pong between routers 0 and 3.
     *
     * The purpose of this test case is to make sure that this situation is
     * appropriately handled through an error.
     *
     * This is a map of what the NoC looks like, where the numbers indicate the NoC router id.
     *
     * 2----1
     * 0------------3
     */

    // Create the NoC data structure
    NocStorage noc_model;

    // store the reference to device grid with
    // this will be set to the device grid width
    noc_model.set_device_grid_spec((int)4, 0);

    noc_model.add_router(0, 0, 0, 0, DUMMY_LATENCY);
    noc_model.add_router(1, 2, 2, 0, DUMMY_LATENCY);
    noc_model.add_router(2, 1, 2, 0, DUMMY_LATENCY);
    noc_model.add_router(3, 3, 0, 0, DUMMY_LATENCY);

    noc_model.make_room_for_noc_router_link_list();

    // add the links
    noc_model.add_link((NocRouterId)0, (NocRouterId)3, DUMMY_BANDWIDTH, DUMMY_LATENCY);
    noc_model.add_link((NocRouterId)3, (NocRouterId)0, DUMMY_BANDWIDTH, DUMMY_LATENCY);
    noc_model.add_link((NocRouterId)2, (NocRouterId)1, DUMMY_BANDWIDTH, DUMMY_LATENCY);

    noc_model.finished_building_noc();

    // now create the start and the destination routers of the route we want to test
    auto start_router_id = NocRouterId(3);
    auto sink_router_id = NocRouterId(1);
    auto traffic_flow_id = NocTrafficFlowId(40);

    // creating the XY routing object
    XYRouting routing_algorithm;

    // store the route found by the algorithm
    std::vector<NocLinkId> found_path;

    // now use the XY router to find a route. We expect this to fail to check that.
    REQUIRE_THROWS_WITH(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model),
                        "No route could be found from starting router with ID:'3' and the destination router with ID:'1' using the XY-Routing algorithm.");
}
} // namespace