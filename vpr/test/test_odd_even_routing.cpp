#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "odd_even_routing.h"


namespace {

constexpr double DUMMY_LATENCY = 1e-9;
constexpr double DUMMY_BANDWIDTH = 1e12;

/**
 * @brief Compares two vectors of NocLinks. These vectors represent
 * two routes between a start and destination routers. This function
 * verifies whether the two routers are the exact same or not.
 *
 */
void compare_routes(const std::vector<NocLink>& golden_path,
                    const std::vector<NocLinkId>& found_path,
                    const NocStorage& noc_model) {
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

TEST_CASE("test_route_flow", "[vpr_noc_odd_even_routing]") {
    /* Creating a test FPGA device below. The NoC itself will be a 10x10 mesh where
     * they will span over a grid size of 10x10. So this FPGA will only consist of NoC routers */

    // Create the NoC storage
    NocStorage noc_model;

    // store the reference to device grid with
    // this will be set to the device grid width
    noc_model.set_device_grid_width((int)10);

    // add all the routers
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            noc_model.add_router((i * 10) + j, j, i, 0, DUMMY_LATENCY);
        }
    }

    noc_model.make_room_for_noc_router_link_list();

    // add all the links
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 10) + j), (NocRouterId)(((i * 10) + j) - 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= 9) {
                noc_model.add_link((NocRouterId)((i * 10) + j), (NocRouterId)(((i * 10) + j) + 10), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= 9) {
                noc_model.add_link((NocRouterId)((i * 10) + j), (NocRouterId)(((i * 10) + j) + 1), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_model.add_link((NocRouterId)((i * 10) + j), (NocRouterId)(((i * 10) + j) - 10), DUMMY_BANDWIDTH, DUMMY_LATENCY);
            }
        }
    }

    noc_model.finished_building_noc();

    // creating the Odd-even routing object
    OddEvenRouting routing_algorithm(noc_model);

    SECTION("Test case where the destination router and the starting routers are the same") {
        // choosing the start and end routers as router 17
        auto start_router_id = NocRouterId(17);
        auto sink_router_id = NocRouterId(17);
        auto traffic_flow_id = NocTrafficFlowId(33);

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // make sure that a legal route was found (no error should be thrown)
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // now make sure that the found route is empty, we shouldn't be moving anywhere as the start and end routers are the same
        REQUIRE(found_path.empty() == true);
    }

    SECTION("Test case where the destination router and the starting routers are located on the same row, and the route is westward.") {
        // choose start router as 19, and choose the destination router as 12
        auto start_router_id = NocRouterId(19);
        auto sink_router_id = NocRouterId(12);
        auto traffic_flow_id = NocTrafficFlowId(34);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path horizontally from router 7 to router 4
        std::vector<NocLink> golden_path;

        for (int current_router = 19; current_router != 12; current_router--) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_router), NocRouterId(current_router - 1));
            const NocLink& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }

    SECTION("Test case where the destination router and the starting routers are located on the same row, and the route is eastward.") {
        // choose start router as 19, and choose the destination router as 12
        auto start_router_id = NocRouterId(41);
        auto sink_router_id = NocRouterId(47);
        auto traffic_flow_id = NocTrafficFlowId(76);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path horizontally from router 7 to router 4
        std::vector<NocLink> golden_path;

        for (int current_router = 41; current_router != 47; current_router++) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_router), NocRouterId(current_router + 1));
            const NocLink& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }

    SECTION("Test case where the destination router and the starting routers are located on the same column within the FPGA device, and the route is northward") {
        // choose start router as 15, and choose the destination router as 75
        auto start_router_id = NocRouterId(15);
        auto sink_router_id = NocRouterId(75);
        auto traffic_flow_id = NocTrafficFlowId(35);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path vertically from router 15 to router 75
        std::vector<NocLink> golden_path;

        for (int current_row = 1; current_row < 7; current_row++) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_row * 10 + 5), NocRouterId((current_row + 1) * 10 + 5));
            const NocLink& link = noc_model.get_single_noc_link(link_id);
            golden_path.push_back(link);
        }

        // store the route found by the algorithm
        std::vector<NocLinkId> found_path;

        // now run the algorithm to find a route, once again we expect no errors and a legal path to be found
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // make sure that size of the found route and golden route match
        compare_routes(golden_path, found_path, noc_model);
    }

    SECTION("Test case where the destination router and the starting routers are located on the same column within the FPGA device, and the route is southward") {
        // choose start router as 82, and choose the destination router as 22
        auto start_router_id = NocRouterId(82);
        auto sink_router_id = NocRouterId(22);
        auto traffic_flow_id = NocTrafficFlowId(9);

        // build the golden route that we expect the algorithm to produce
        // the expectation is a number of links that path vertically from router 15 to router 75
        std::vector<NocLink> golden_path;

        for (int current_row = 8; current_row > 2; current_row--) {
            NocLinkId link_id = noc_model.get_single_noc_link_id(NocRouterId(current_row * 10 + 2), NocRouterId((current_row - 1) * 10 + 2));
            const NocLink& link = noc_model.get_single_noc_link(link_id);
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

}