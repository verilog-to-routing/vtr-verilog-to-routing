#include "catch2/catch_test_macros.hpp"

#include "noc_traffic_flows.h"

#include <random>

#define NUM_OF_ROUTERS 10

namespace {

TEST_CASE("test_adding_traffic_flows", "[vpr_noc_traffic_flows]") {
    // the traffic flows datastructure and reset it
    NocTrafficFlows traffic_flow_storage;
    traffic_flow_storage.clear_traffic_flows();

    // parameters for each traffic flow
    std::string source_router_name = "test_1";
    std::string sink_router_name = "test_2";
    double traffic_flow_bandwidth = 200;
    double traffic_flow_latency = 10;
    int traffic_flow_priority = 1;
    ClusterBlockId source_router_id;
    ClusterBlockId sink_router_id;
    NocTrafficFlowId curr_flow_id;
    // set up the test data

    // create all the routers
    std::vector<ClusterBlockId> golden_router_blocks_list;
    for (int router = 0; router < NUM_OF_ROUTERS; router++) {
        golden_router_blocks_list.push_back((ClusterBlockId)router);
    }

    // total traffic flows will be NUM_OF_ROUTERS * (NUM_OF_ROUTERS - 1)
    // create the traffic flows
    std::vector<t_noc_traffic_flow> golden_traffic_flow_list;

    vtr::vector<ClusterBlockId, std::vector<NocTrafficFlowId>> golden_list_of_associated_traffic_flows_to_routers;

    golden_list_of_associated_traffic_flows_to_routers.resize(NUM_OF_ROUTERS);

    for (int router = 0; router < NUM_OF_ROUTERS; router++) {
        for (int second_router = 0; second_router < NUM_OF_ROUTERS; second_router++) {
            // don't want the case where the source and destination routers are the same
            if (router == second_router) {
                continue;
            }

            source_router_id = (ClusterBlockId)router;
            sink_router_id = (ClusterBlockId)second_router;

            // need to match how the test function does it
            golden_traffic_flow_list.emplace_back(source_router_name, sink_router_name, source_router_id, sink_router_id, traffic_flow_bandwidth, traffic_flow_latency, traffic_flow_priority);

            curr_flow_id = (NocTrafficFlowId)(golden_traffic_flow_list.size() - 1);

            // add the current traffic flow as an associated flow to its source and sink routers
            golden_list_of_associated_traffic_flows_to_routers[source_router_id].emplace_back(curr_flow_id);
            golden_list_of_associated_traffic_flows_to_routers[sink_router_id].emplace_back(curr_flow_id);
        }
    }

    // finished setting up all the golden information, so now perform the tests
    SECTION("Verifying that all created traffic flows and their related information are stored correctly.") {
        // add all the traffic flows to the datastructure
        for (int router = 0; router < NUM_OF_ROUTERS; router++) {
            for (int second_router = 0; second_router < NUM_OF_ROUTERS; second_router++) {
                // don't want the case where the source and destination routers are the same
                if (router == second_router) {
                    continue;
                }

                source_router_id = (ClusterBlockId)router;
                sink_router_id = (ClusterBlockId)second_router;

                // create and add the traffic flow
                traffic_flow_storage.create_noc_traffic_flow(source_router_name, sink_router_name, source_router_id, sink_router_id, traffic_flow_bandwidth, traffic_flow_latency, traffic_flow_priority);
            }
        }

        int size_of_router_block_list = golden_router_blocks_list.size();

        // check the set of routers first to see that they were all added properly
        for (int router = 0; router < size_of_router_block_list; router++) {
            // every router in the golden list needs to exist in the traffic flow datastructure (this also tests cases where a router was added multiple times, this shouldn't affect it)
            REQUIRE(traffic_flow_storage.check_if_cluster_block_has_traffic_flows(golden_router_blocks_list[router]) == true);
        }

        int size_of_traffic_flow_list = golden_traffic_flow_list.size();

        // check the traffic flows (make sure they are correct)
        for (int traffic_flow = 0; traffic_flow < size_of_traffic_flow_list; traffic_flow++) {
            curr_flow_id = (NocTrafficFlowId)traffic_flow;
            const t_noc_traffic_flow& curr_traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(curr_flow_id);

            // make sure that the source and destination routers match the golden set
            REQUIRE(curr_traffic_flow.source_router_cluster_id == golden_traffic_flow_list[traffic_flow].source_router_cluster_id);
            REQUIRE(curr_traffic_flow.sink_router_cluster_id == golden_traffic_flow_list[traffic_flow].sink_router_cluster_id);
        }

        // now check that the associated traffic flows for each router is also stored correctly
        for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
            ClusterBlockId router_id = (ClusterBlockId)router_number;

            int number_of_traffic_flows_associated_with_current_router = golden_list_of_associated_traffic_flows_to_routers[router_id].size();

            // get the traffic flows associated to the current router from the test datastructure
            const std::vector<NocTrafficFlowId>& associated_traffic_flows_to_router = traffic_flow_storage.get_traffic_flows_associated_to_router_block(router_id);

            // make sure that the number of traffic flows associated to each router within the NocTrafficFlows data structure matches the golden set
            REQUIRE((int)associated_traffic_flows_to_router.size() == number_of_traffic_flows_associated_with_current_router);

            // now go through the associated traffic flows and make sure the correct ones were added to the current router
            for (int router_traffic_flow = 0; router_traffic_flow < number_of_traffic_flows_associated_with_current_router; router_traffic_flow++) {
                REQUIRE((size_t)golden_list_of_associated_traffic_flows_to_routers[router_id][router_traffic_flow] == (size_t)associated_traffic_flows_to_router[router_traffic_flow]);
            }
        }

        // make sure that the number of unique routers stored inside the NocTrafficFlows class is what we expect it should be
        REQUIRE(NUM_OF_ROUTERS == traffic_flow_storage.get_number_of_routers_used_in_traffic_flows());
    }
    SECTION("Checking to see if invalid blocks that are not routers exist in NocTrafficFlows.") {
        // create an invalid block id
        ClusterBlockId invalid_block = (ClusterBlockId)(NUM_OF_ROUTERS + 1);

        // check that this block doesn't exist in the traffic flow datastructure
        REQUIRE(traffic_flow_storage.check_if_cluster_block_has_traffic_flows(invalid_block) == false);
    }
    SECTION("Checking that when a router has no traffic flows associated to it, then the associated traffic flows vector retrieved from the NocTrafficFlows class for this router should be null.") {
        // create an invalid block id (mimics the effect where a router has no traffic flows associated with it)
        ClusterBlockId invalid_block = (ClusterBlockId)(NUM_OF_ROUTERS + 1);

        // check that this router has no traffic flows associated with it
        REQUIRE(traffic_flow_storage.get_traffic_flows_associated_to_router_block(invalid_block).empty());
    }
}
} // namespace