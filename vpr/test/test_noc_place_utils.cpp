#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "noc_place_utils.h"
#include "noc_routing.h"
#include "xy_routing.h"
#include "bfs_routing.h"

// test parameters
#define NUM_OF_ROUTERS 100
#define MESH_TOPOLOGY_SIZE 10            // this should be the square root of NUM_OF_ROUTERS
#define NUM_OF_LOGICAL_ROUTER_BLOCKS 100 // should be less than or equal to NUM_OF_ROUTERS
#define NUM_OF_TRAFFIC_FLOWS 100         // this should be less than or equal to the NUM_OF_LOGICAL_ROUTER_BLOCKS
#define NUM_OF_PLACEMENT_MOVES 10000

namespace {

TEST_CASE("test_initial_noc_placement", "[noc_place_utils]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_LOGICAL_ROUTER_BLOCKS - 1);
    // this sets the range of possible bandwidths for a traffic flow
    std::uniform_int_distribution<std::mt19937::result_type> dist_2(0, 1000);

    // get global datastructures
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    std::vector<int> id_of_all_hard_routers_in_device;

    // start by creating the routers
    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number % MESH_TOPOLOGY_SIZE;
        router_grid_position_y = router_number / MESH_TOPOLOGY_SIZE;

        id_of_all_hard_routers_in_device.push_back(router_number);

        // add the router to the noc
        noc_ctx.noc_model.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);
    }

    noc_ctx.noc_model.make_room_for_noc_router_link_list();

    // now add the links, this will generate a mesh topology with the size being MESH_TOPOLOGY_SIZE
    for (int i = 0; i < MESH_TOPOLOGY_SIZE; i++) {
        for (int j = 0; j < MESH_TOPOLOGY_SIZE; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) - 1));
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= MESH_TOPOLOGY_SIZE - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) + MESH_TOPOLOGY_SIZE));
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= MESH_TOPOLOGY_SIZE - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) + 1));
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) - MESH_TOPOLOGY_SIZE));
            }
        }
    }

    // now we need to create router cluster blocks and assing them to placed at a router hard block
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS; cluster_block_number++) {
        // since the indexes for the hard router blocks start from 0, we will just place the router clusters on hard router blocks with the same id //

        // start by creating the placement grid location for the current router cluster by getting the grid location of the hard router block it will be placed on
        const NocRouter& hard_router_block = noc_ctx.noc_model.get_single_noc_router((NocRouterId)cluster_block_number);
        t_block_loc current_cluster_block_location;
        current_cluster_block_location.is_fixed = true;
        current_cluster_block_location.loc = t_pl_loc(hard_router_block.get_router_grid_position_x(), hard_router_block.get_router_grid_position_y(), -1);

        // now add the cluster and its placed location to the placement datastructures
        place_ctx.block_locs.insert(ClusterBlockId(cluster_block_number), current_cluster_block_location);
    }

    // similiar parameters for all traffic flows
    std::string source_traffic_flow_name = "test";
    std::string sink_traffic_flow_name = "test_2";
    double traffic_flow_latency = 10.0;
    int number_of_created_traffic_flows = 0;

    // now create a random number of traffic flows
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS; cluster_block_number++) {
        // the current cluster block number will act as the source router
        // and we will choose a random router to act as the sink router

        ClusterBlockId source_router_for_traffic_flow = (ClusterBlockId)cluster_block_number;
        ClusterBlockId sink_router_for_traffic_flow;

        // randomly choose sink router
        do {
            sink_router_for_traffic_flow = (ClusterBlockId)dist(rand_num_gen);
        } while (sink_router_for_traffic_flow == source_router_for_traffic_flow);

        // randomly choose a bandwidth for the traffic flow
        double traffic_flow_bandwidth_usage = (double)dist_2(rand_num_gen);

        // create and add the traffic flow
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(source_traffic_flow_name, sink_traffic_flow_name, source_router_for_traffic_flow, sink_router_for_traffic_flow, traffic_flow_bandwidth_usage, traffic_flow_latency);

        number_of_created_traffic_flows++;

        // exit when we have created the required number of traffic flows
        if (number_of_created_traffic_flows == NUM_OF_TRAFFIC_FLOWS) {
            break;
        }
    }
    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // now go and route all the traffic flows //
    // start by creating the routing alagorithm
    NocRouting* routing_algorithm = new XYRouting();
    noc_ctx.noc_flows_router = routing_algorithm;

    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);
        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        // get the source and sink routers of this traffic flow
        int source_hard_router_id = (size_t)curr_traffic_flow.source_router_cluster_id;
        int sink_hard_routed_id = (size_t)curr_traffic_flow.sink_router_cluster_id;
        // route it
        routing_algorithm->route_flow((NocRouterId)source_hard_router_id, (NocRouterId)sink_hard_routed_id, traffic_flow_route, noc_ctx.noc_model);
    }

    /* in the test function we will be routing all the traffic flows and then updating their link usages. So we will be generating a vector of all links
     * and updating their bandwidths based on the routed traffic flows. This will
     * then be compared to the link badnwdith usages updated by the test function
     */
    vtr::vector<NocLinkId, double> golden_link_bandwidths;
    golden_link_bandwidths.resize(noc_ctx.noc_model.get_noc_links().size(), 0.0);

    // now go through the routed traffic flows and update the bandwidths of the links. Once a traffic flow has been processsed, we need to clear it so that the test function can update it with the routes it finds
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);
        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        for (auto& link : traffic_flow_route) {
            golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;
        }

        traffic_flow_route.clear();
    }

    // now call the test function
    initial_noc_placement();

    // now verify the function by comparing the link bandwidths in the noc model (should have been updated by the test function) to the golden set
    int number_of_links = golden_link_bandwidths.size();

    for (int link_number = 0; link_number < number_of_links; link_number++) {
        NocLinkId current_link_id = (NocLinkId)link_number;
        const NocLink& current_link = noc_ctx.noc_model.get_single_noc_link(current_link_id);

        REQUIRE(golden_link_bandwidths[current_link_id] == current_link.get_bandwidth_usage());
    }

    // now clear all the datastructures used in this test
    noc_ctx.noc_model.clear_noc();
    place_ctx.block_locs.clear();
    noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();
    delete routing_algorithm;
    noc_ctx.noc_flows_router = nullptr;
}

// need to add tets that checks whether this works properly when one router or two routers involved in a move dont have traffic flows associated with them

TEST_CASE("test_find_affected_noc_routers_and_update_noc_costs", "[noc_place_utils]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_LOGICAL_ROUTER_BLOCKS - 1);
    // this sets the range of possible bandwidths for a traffic flow
    std::uniform_int_distribution<std::mt19937::result_type> dist_2(0, 1000);

    // get global datastructures
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    // keeps track of which hard router each cluster block is placed
    vtr::vector<ClusterBlockId, NocRouterId> router_where_cluster_is_placed;

    // start by creating the routers
    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number % MESH_TOPOLOGY_SIZE;
        router_grid_position_y = router_number / MESH_TOPOLOGY_SIZE;

        // add the router to the noc
        noc_ctx.noc_model.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);
    }

    noc_ctx.noc_model.make_room_for_noc_router_link_list();

    // now add the links, this will generate a mesh topology with the size being MESH_TOPOLOGY_SIZE
    for (int i = 0; i < MESH_TOPOLOGY_SIZE; i++) {
        for (int j = 0; j < MESH_TOPOLOGY_SIZE; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) - 1));
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= MESH_TOPOLOGY_SIZE - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) + MESH_TOPOLOGY_SIZE));
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= MESH_TOPOLOGY_SIZE - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) + 1));
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE) + j) - MESH_TOPOLOGY_SIZE));
            }
        }
    }

    // now we need to create router cluster blocks and assing them to placed at a router hard block as an initial position
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS; cluster_block_number++) {
        // since the indexes for the hard router blocks start from 0, we will just place the router clusters on hard router blocks with the same id //

        // start by creating the placement grid location for the current router cluster by getting the grid location of the hard router block it will be placed on
        const NocRouter& hard_router_block = noc_ctx.noc_model.get_single_noc_router((NocRouterId)cluster_block_number);
        t_block_loc current_cluster_block_location;
        current_cluster_block_location.is_fixed = true;
        current_cluster_block_location.loc = t_pl_loc(hard_router_block.get_router_grid_position_x(), hard_router_block.get_router_grid_position_y(), -1);

        router_where_cluster_is_placed.push_back((NocRouterId)cluster_block_number);

        // now add the cluster and its placed location to the placement datastructures
        place_ctx.block_locs.insert(ClusterBlockId(cluster_block_number), current_cluster_block_location);
    }

    // similiar parameters for all traffic flows
    std::string source_traffic_flow_name = "test";
    std::string sink_traffic_flow_name = "test_2";
    double traffic_flow_latency = 10.0;
    int number_of_created_traffic_flows = 0;

    // now create a random number of traffic flows
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS; cluster_block_number++) {
        // the current cluster block number will act as the source router
        // and we will choose a random router to act as the sink router

        ClusterBlockId source_router_for_traffic_flow = (ClusterBlockId)cluster_block_number;
        ClusterBlockId sink_router_for_traffic_flow;

        // randomly choose sink router
        do {
            sink_router_for_traffic_flow = (ClusterBlockId)dist(rand_num_gen);
        } while (sink_router_for_traffic_flow == source_router_for_traffic_flow);

        // randomly choose a bandwidth for the traffic flow
        double traffic_flow_bandwidth_usage = (double)dist_2(rand_num_gen);

        // create and add the traffic flow
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(source_traffic_flow_name, sink_traffic_flow_name, source_router_for_traffic_flow, sink_router_for_traffic_flow, traffic_flow_bandwidth_usage, traffic_flow_latency);

        number_of_created_traffic_flows++;

        // exit when we have created the required number of traffic flows
        if (number_of_created_traffic_flows == NUM_OF_TRAFFIC_FLOWS) {
            break;
        }
    }
    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // now go and route all the traffic flows //
    // start by creating the routing alagorithm
    NocRouting* routing_algorithm = new XYRouting();
    noc_ctx.noc_flows_router = routing_algorithm;

    // store the traffic flow routes found
    vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> golden_traffic_flow_routes;
    golden_traffic_flow_routes.resize(noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows());

    // we need to route all the traffic flows based on their initial positions
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);
        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        // get the source and sink routers of this traffic flow
        int source_hard_router_id = (size_t)curr_traffic_flow.source_router_cluster_id;
        int sink_hard_routed_id = (size_t)curr_traffic_flow.sink_router_cluster_id;
        // route it
        routing_algorithm->route_flow((NocRouterId)source_hard_router_id, (NocRouterId)sink_hard_routed_id, traffic_flow_route, noc_ctx.noc_model);

        // store the traffic flow route locally
        for (auto& link : traffic_flow_route) {
            golden_traffic_flow_routes[(NocTrafficFlowId)traffic_flow_number].push_back(link);
        }
    }
    // datastructure below will store the bandwidth usages of all the links
    // and will be updated throughout this test.
    // These link bandwidths will then be compared to link bandwidths in the NoC datastructure (these will be updated in the test function)
    vtr::vector<NocLinkId, double> golden_link_bandwidths;
    golden_link_bandwidths.resize(noc_ctx.noc_model.get_noc_links().size(), 0.0);

    // now store update the bandwidths used by all the links based on the initial traffic flow routes
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);
        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        for (auto& link : traffic_flow_route) {
            golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;

            // update the link bandwidth in the NoC datastructure
            double current_link_bandwidth = noc_ctx.noc_model.get_single_noc_link(link).get_bandwidth_usage();
            noc_ctx.noc_model.get_single_mutable_noc_link(link).set_bandwidth_usage(current_link_bandwidth + curr_traffic_flow.traffic_flow_bandwidth);
        }
    }

    // datastructure that keeps track of moved blocks during placement
    t_pl_blocks_to_be_moved blocks_affected(NUM_OF_LOGICAL_ROUTER_BLOCKS);

    // datastructure that keeps track of all the traffic flows that have been re-routed
    std::unordered_set<NocTrafficFlowId> routed_traffic_flows;

    /*  Now we imitate placement here by swapping two clusters block
     * posistions. In each iteration, we first update the positions
     * of the two router cluster blocks, then we update the traffic
     * flows and then the bandwidth usages of the links. Then we call
     * the test function and then move onto the next iteration.
     */
    for (int iteration_number = 0; iteration_number < NUM_OF_PLACEMENT_MOVES; iteration_number++) {
        // get the two cluster blocks to swap first
        ClusterBlockId swap_router_block_one = (ClusterBlockId)dist(rand_num_gen);
        ClusterBlockId swap_router_block_two;
        do {
            swap_router_block_two = (ClusterBlockId)dist(rand_num_gen);
        } while (swap_router_block_one == swap_router_block_two);

        //setup the moved blocks datastructure for the test function
        blocks_affected.num_moved_blocks = 2;

        blocks_affected.moved_blocks[0].block_num = swap_router_block_one;
        blocks_affected.moved_blocks[0].old_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_y(), -1);
        blocks_affected.moved_blocks[0].new_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_y(), -1);

        blocks_affected.moved_blocks[1].block_num = swap_router_block_two;
        blocks_affected.moved_blocks[1].old_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_y(), -1);
        blocks_affected.moved_blocks[1].new_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_y(), -1);

        // swap the hard router blocks where the two cluster blocks are placed on
        NocRouterId router_first_swap_cluster_location = router_where_cluster_is_placed[swap_router_block_one];
        router_where_cluster_is_placed[swap_router_block_one] = router_where_cluster_is_placed[swap_router_block_two];
        router_where_cluster_is_placed[swap_router_block_two] = router_first_swap_cluster_location;

        // now move the blocks in the placement datastructures
        place_ctx.block_locs[swap_router_block_one].loc = blocks_affected.moved_blocks[0].new_loc;
        place_ctx.block_locs[swap_router_block_two].loc = blocks_affected.moved_blocks[1].new_loc;

        // get all the associated traffic flows of the moved cluster blocks
        const std::vector<NocTrafficFlowId>* assoc_traffic_flows_block_one = noc_ctx.noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(swap_router_block_one);
        const std::vector<NocTrafficFlowId>* assoc_traffic_flows_block_two = noc_ctx.noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(swap_router_block_two);

        // now go through the traffic flows and update the link bandwidths and traffic flow routes locally
        for (auto& traffic_flow : *assoc_traffic_flows_block_one) {
            if (routed_traffic_flows.find(traffic_flow) == routed_traffic_flows.end()) {
                // get the current traffic flow
                const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow);

                // go through the current traffic flow and reduce the bandwidths of the links
                for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
                    golden_link_bandwidths[link] -= curr_traffic_flow.traffic_flow_bandwidth;
                }

                // re-route the traffic flow
                noc_ctx.noc_flows_router->route_flow(router_where_cluster_is_placed[curr_traffic_flow.source_router_cluster_id], router_where_cluster_is_placed[curr_traffic_flow.sink_router_cluster_id], golden_traffic_flow_routes[traffic_flow], noc_ctx.noc_model);

                // go through the current traffic flow and increase the bandwidths of the links
                for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
                    golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;
                }

                routed_traffic_flows.insert(traffic_flow);
            }
        }

        // this is for the second swapped block
        for (auto& traffic_flow : *assoc_traffic_flows_block_two) {
            if (routed_traffic_flows.find(traffic_flow) == routed_traffic_flows.end()) {
                // get the current traffic flow
                const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow);

                // go through the current traffic flow and reduce the bandwidths of the links
                for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
                    golden_link_bandwidths[link] -= curr_traffic_flow.traffic_flow_bandwidth;
                }

                // re-route the traffic flow
                noc_ctx.noc_flows_router->route_flow(router_where_cluster_is_placed[curr_traffic_flow.source_router_cluster_id], router_where_cluster_is_placed[curr_traffic_flow.sink_router_cluster_id], golden_traffic_flow_routes[traffic_flow], noc_ctx.noc_model);

                // go through the current traffic flow and increase the bandwidths of the links
                for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
                    golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;
                }

                routed_traffic_flows.insert(traffic_flow);
            }
        }

        // call the test function
        find_affected_noc_routers_and_update_noc_costs(blocks_affected);

        // clear the affected blocks
        clear_move_blocks(blocks_affected);

        // clear the routed traffic flows
        routed_traffic_flows.clear();
    }

    /*
     * Now we will run a test where the two routers that are moved share a 
     * traffic flow with each other. This is used to verify whether the
     * function under test correctly gets the positions of the moved blocks.
     * It also checks that the test function correctly handles the situatio
     * where the moved router cluster block is a sink router in its associated traffic flows.
     */
    // start by choosing a random traffic flow
    NocTrafficFlowId random_traffic_flow = (NocTrafficFlowId)dist(rand_num_gen);

    // get the current traffic flow
    const t_noc_traffic_flow& chosen_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow(random_traffic_flow);

    // now swap the two blocks within this traffic flow
    ClusterBlockId swap_router_block_one = chosen_traffic_flow.sink_router_cluster_id;
    ClusterBlockId swap_router_block_two = chosen_traffic_flow.source_router_cluster_id;

    // now perform the swap
    //setup the moved blocks datastructure for the test function
    blocks_affected.num_moved_blocks = 2;

    blocks_affected.moved_blocks[0].block_num = swap_router_block_one;

    blocks_affected.moved_blocks[0].old_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_y(), -1);
    blocks_affected.moved_blocks[0].new_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_y(), -1);

    blocks_affected.moved_blocks[1].block_num = swap_router_block_two;
    blocks_affected.moved_blocks[1].old_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_two]).get_router_grid_position_y(), -1);
    blocks_affected.moved_blocks[1].new_loc = t_pl_loc(noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_x(), noc_ctx.noc_model.get_single_noc_router(router_where_cluster_is_placed[swap_router_block_one]).get_router_grid_position_y(), -1);

    // swap the hard router blocks where the two cluster blocks are placed on
    NocRouterId router_first_swap_cluster_location = router_where_cluster_is_placed[swap_router_block_one];
    router_where_cluster_is_placed[swap_router_block_one] = router_where_cluster_is_placed[swap_router_block_two];
    router_where_cluster_is_placed[swap_router_block_two] = router_first_swap_cluster_location;

    // now move the blocks in the placement datastructures
    place_ctx.block_locs[swap_router_block_one].loc = blocks_affected.moved_blocks[0].new_loc;
    place_ctx.block_locs[swap_router_block_two].loc = blocks_affected.moved_blocks[1].new_loc;

    // get all the associated traffic flows of the moved cluster blocks
    const std::vector<NocTrafficFlowId>* assoc_traffic_flows_block_one = noc_ctx.noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(swap_router_block_one);
    const std::vector<NocTrafficFlowId>* assoc_traffic_flows_block_two = noc_ctx.noc_traffic_flows_storage.get_traffic_flows_associated_to_router_block(swap_router_block_two);

    // now go through the traffic flows and update the link bandwidths and traffic flow routes locally
    for (auto& traffic_flow : *assoc_traffic_flows_block_one) {
        // get the current traffic flow
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow);

        // go through the current traffic flow and reduce the bandwidths of the links
        for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
            golden_link_bandwidths[link] -= curr_traffic_flow.traffic_flow_bandwidth;
        }

        // re-route the traffic flow
        noc_ctx.noc_flows_router->route_flow(router_where_cluster_is_placed[curr_traffic_flow.source_router_cluster_id], router_where_cluster_is_placed[curr_traffic_flow.sink_router_cluster_id], golden_traffic_flow_routes[traffic_flow], noc_ctx.noc_model);

        // go through the current traffic flow and increase the bandwidths of the links
        for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
            golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;
        }
    }

    // this is for the second swapped block
    for (auto& traffic_flow : *assoc_traffic_flows_block_two) {
        // get the current traffic flow
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow(traffic_flow);

        // go through the current traffic flow and reduce the bandwidths of the links
        for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
            golden_link_bandwidths[link] -= curr_traffic_flow.traffic_flow_bandwidth;
        }

        // re-route the traffic flow
        noc_ctx.noc_flows_router->route_flow(router_where_cluster_is_placed[curr_traffic_flow.source_router_cluster_id], router_where_cluster_is_placed[curr_traffic_flow.sink_router_cluster_id], golden_traffic_flow_routes[traffic_flow], noc_ctx.noc_model);

        // go through the current traffic flow and increase the bandwidths of the links
        for (auto& link : golden_traffic_flow_routes[traffic_flow]) {
            golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;
        }
    }

    // call the test function
    find_affected_noc_routers_and_update_noc_costs(blocks_affected);

    // now verify the function by comparing the link bandwidths in the noc model (should have been updated by the test function) to the golden set
    int number_of_links = golden_link_bandwidths.size();
    for (int link_number = 0; link_number < number_of_links; link_number++) {
        NocLinkId current_link_id = (NocLinkId)link_number;
        const NocLink& current_link = noc_ctx.noc_model.get_single_noc_link(current_link_id);

        REQUIRE(golden_link_bandwidths[current_link_id] == current_link.get_bandwidth_usage());
    }

    // now clear all the datastructures used in this test
    noc_ctx.noc_model.clear_noc();
    place_ctx.block_locs.clear();
    noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();
    delete routing_algorithm;
    noc_ctx.noc_flows_router = nullptr;
}
} // namespace
