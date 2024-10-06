#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "bfs_routing.h"

namespace {

constexpr double DUMMY_LATENCY = 1e-9;
constexpr double DUMMY_BANDWIDTH = 1e12;

TEST_CASE("test_route_flow", "[vpr_noc_bfs_routing]") {
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
    // need to add this before routers are added
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

    // creating the XY routing object
    BFSRouting routing_algorithm;

    SECTION("Test case where the destination router and the starting routers are the same") {
        // choosing the start eand end routers as router 10
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
    SECTION("Test case where there is a single shortest path between the source and destination routers in the NoC.") {
        // choosing the starting router (source) as the top left corner of the mesh
        auto start_router_id = NocRouterId(12);
        // choosing the destination router as the bottom right corner of the mesh
        auto sink_router_id = NocRouterId(3);
        auto traffic_flow_id = NocTrafficFlowId(33);

        // store the route to be found by the algorithm
        std::vector<NocLinkId> found_path;
        // store the golden route (the fastest possible path)
        std::vector<NocLinkId> golden_path;

        // setup the fastest possible route by adding a set of diagonal links that connect
        // routers 12 9 6 3.
        // These links also represent the fastest path, so add them to the golden route

        // we need to know the ids of the new links that are being added. We know that the
        // router ids are based on the vector size, so get the size of the vector of links
        // within the NoC
        int number_of_links = noc_model.get_noc_links().size();

        // add the diagonal links and also add them to the golden path
        noc_model.add_link(NocRouterId(12), NocRouterId(9), DUMMY_BANDWIDTH, DUMMY_LATENCY);
        golden_path.emplace_back(number_of_links++);
        noc_model.add_link(NocRouterId(9), NocRouterId(6), DUMMY_BANDWIDTH, DUMMY_LATENCY);
        golden_path.emplace_back(number_of_links++);
        noc_model.add_link(NocRouterId(6), NocRouterId(3), DUMMY_BANDWIDTH, DUMMY_LATENCY);
        golden_path.emplace_back(number_of_links++);

        // now run the routinjg algorithm
        // make sure that a legal route was found (no error should be thrown)
        REQUIRE_NOTHROW(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model));

        // check that the found route has the exact same number of links as the expected path
        REQUIRE(golden_path.size() == found_path.size());
        // go through the expected path and verify that each link matches to the found path
        // by the routing algorithm
        for (int link_in_path = 0; link_in_path < (int)golden_path.size(); link_in_path++) {
            REQUIRE(golden_path[link_in_path] == found_path[link_in_path]);
        }
    }
    SECTION("Test case where no path could be found") {
        // choosing the starting router (source) as the top left corner of the mesh
        auto start_router_id = NocRouterId(12);
        // choosing the destination router as the bottom right corner of the mesh
        auto sink_router_id = NocRouterId(3);
        auto traffic_flow_id = NocTrafficFlowId(33);

        // Remove any direct links to router 3
        noc_model.remove_link(NocRouterId(2), NocRouterId(3));
        noc_model.remove_link(NocRouterId(7), NocRouterId(3));

        // store the route to be found by the algorithm
        std::vector<NocLinkId> found_path;

        // run the routing algorithm and we expect ir ro fail
        REQUIRE_THROWS_WITH(routing_algorithm.route_flow(start_router_id, sink_router_id, traffic_flow_id, found_path, noc_model),
                            "No route could be found from starting router with id:'12' and the destination router with id:'3' using the breadth-first search routing algorithm.");
    }
}

} // namespace