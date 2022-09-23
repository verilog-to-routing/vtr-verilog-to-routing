#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "noc_place_utils.h"
#include "noc_routing.h"
#include "xy_routing.h"
#include "bfs_routing.h"
#include "vtr_math.h"

// test parameters
#define NUM_OF_ROUTERS_NOC_PLACE_UTILS_TEST 100
#define MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST 10            // this should be the square root of NUM_OF_ROUTERS
#define NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST 100 // should be less than or equal to NUM_OF_ROUTERS
#define NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST 100         // this should be less than or equal to the NUM_OF_LOGICAL_ROUTER_BLOCKS
#define NUM_OF_PLACEMENT_MOVES_NOC_PLACE_UTILS_TEST 10000

namespace {

TEST_CASE("test_initial_noc_placement", "[noc_place_utils]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST - 1);
    // this sets the range of possible bandwidths for a traffic flow
    std::uniform_int_distribution<std::mt19937::result_type> dist_2(0, 1000);

    // get global datastructures
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // start by deleting any global datastructures (this is so that we dont have corruption from previous tests)
    noc_ctx.noc_model.clear_noc();
    noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();
    delete noc_ctx.noc_flows_router;
    place_ctx.block_locs.clear();

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    std::vector<int> id_of_all_hard_routers_in_device;

    // start by creating the routers
    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS_NOC_PLACE_UTILS_TEST; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number % MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;
        router_grid_position_y = router_number / MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;

        id_of_all_hard_routers_in_device.push_back(router_number);

        // add the router to the noc
        noc_ctx.noc_model.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);
    }

    noc_ctx.noc_model.make_room_for_noc_router_link_list();

    // now add the links, this will generate a mesh topology with the size being MESH_TOPOLOGY_SIZE
    for (int i = 0; i < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; i++) {
        for (int j = 0; j < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - 1));
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + 1));
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
        }
    }

    // now we need to create router cluster blocks and assing them to placed at a router hard block
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
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
    int traffic_flow_priority = 1;

    // now create a random number of traffic flows
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
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
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(source_traffic_flow_name, sink_traffic_flow_name, source_router_for_traffic_flow, sink_router_for_traffic_flow, traffic_flow_bandwidth_usage, traffic_flow_latency, traffic_flow_priority);

        number_of_created_traffic_flows++;

        // exit when we have created the required number of traffic flows
        if (number_of_created_traffic_flows == NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST) {
            break;
        }
    }
    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // now go and route all the traffic flows //
    // start by creating the routing alagorithm
    NocRouting* routing_algorithm_global = new XYRouting();
    noc_ctx.noc_flows_router = routing_algorithm_global;

    // create a local routing algorithm for the unit test
    NocRouting* routing_algorithm = new XYRouting();

    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
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
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
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

    // delete the local routing algorithm
    delete routing_algorithm;
}
TEST_CASE("test_initial_comp_cost_functions", "[noc_place_utils]") {

// setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST - 1);
    // this sets the range of possible priorities
    std::uniform_int_distribution<std::mt19937::result_type> dist_1(1, 100);
    // for random double number generation for the bandwidth and latency
    std::uniform_real_distribution<double> dist_2(0,1000);
    std::uniform_real_distribution<double> dist_3(1,25);
    std::default_random_engine double_engine;

    // get global datastructures
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // start by deleting any global datastructures (this is so that we dont have corruption from previous tests)
    noc_ctx.noc_model.clear_noc();
    noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();
    delete noc_ctx.noc_flows_router;
    place_ctx.block_locs.clear();

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;

    std::vector<int> id_of_all_hard_routers_in_device;

    // start by creating the routers
    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS_NOC_PLACE_UTILS_TEST; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number % MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;
        router_grid_position_y = router_number / MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;

        id_of_all_hard_routers_in_device.push_back(router_number);

        // add the router to the noc
        noc_ctx.noc_model.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);
    }

    noc_ctx.noc_model.make_room_for_noc_router_link_list();

    // now add the links, this will generate a mesh topology with the size being MESH_TOPOLOGY_SIZE
    for (int i = 0; i < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; i++) {
        for (int j = 0; j < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - 1));
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + 1));
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
        }
    }

    // now we need to create router cluster blocks and assing them to placed at a router hard block
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
        // since the indexes for the hard router blocks start from 0, we will just place the router clusters on hard router blocks with the same id //

        // start by creating the placement grid location for the current router cluster by getting the grid location of the hard router block it will be placed on
        const NocRouter& hard_router_block = noc_ctx.noc_model.get_single_noc_router((NocRouterId)cluster_block_number);
        t_block_loc current_cluster_block_location;
        current_cluster_block_location.is_fixed = true;
        current_cluster_block_location.loc = t_pl_loc(hard_router_block.get_router_grid_position_x(), hard_router_block.get_router_grid_position_y(), -1);

        // now add the cluster and its placed location to the placement datastructures
        place_ctx.block_locs.insert(ClusterBlockId(cluster_block_number), current_cluster_block_location);
    }

    noc_ctx.noc_model.set_noc_link_latency(1);
    noc_ctx.noc_model.set_noc_router_latency(1);

    // similiar parameters for all traffic flows
    std::string source_traffic_flow_name = "test";
    std::string sink_traffic_flow_name = "test_2";
    int number_of_created_traffic_flows = 0;

    // now create a random number of traffic flows
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
        // the current cluster block number will act as the source router
        // and we will choose a random router to act as the sink router

        ClusterBlockId source_router_for_traffic_flow = (ClusterBlockId)cluster_block_number;
        ClusterBlockId sink_router_for_traffic_flow;

        // randomly choose sink router
        do {
            sink_router_for_traffic_flow = (ClusterBlockId)dist(rand_num_gen);
        } while (sink_router_for_traffic_flow == source_router_for_traffic_flow);

        // randomly choose a bandwidth, latency and priority for the traffic flow
        double traffic_flow_bandwidth_usage = (double)dist_2(double_engine);
        double traffic_flow_latency_constraint = (double)dist_3(double_engine);
        int traffic_flow_priority = dist_1(rand_num_gen);

        // create and add the traffic flow
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(source_traffic_flow_name, sink_traffic_flow_name, source_router_for_traffic_flow, sink_router_for_traffic_flow, traffic_flow_bandwidth_usage, traffic_flow_latency_constraint, traffic_flow_priority);

        number_of_created_traffic_flows++;

        // exit when we have created the required number of traffic flows
        if (number_of_created_traffic_flows == NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST) {
            break;
        }
    }
    
    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // need to route all the traffic flows so create a datstructure to store them here
    std::vector<int> golden_traffic_flow_route_sizes;
    golden_traffic_flow_route_sizes.resize(number_of_created_traffic_flows);
    
    // now go and route all the traffic flows //
    // start by creating the routing alagorithm
    NocRouting* routing_algorithm_global = new XYRouting();
    noc_ctx.noc_flows_router = routing_algorithm_global;

    // create a local routing algorithm for the unit test
    NocRouting* routing_algorithm = new XYRouting();

    // route all the traffic flows locally
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);

        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        // get the source and sink routers of this traffic flow
        int source_hard_router_id = (size_t)curr_traffic_flow.source_router_cluster_id;
        int sink_hard_routed_id = (size_t)curr_traffic_flow.sink_router_cluster_id;
        // route it
        routing_algorithm->route_flow((NocRouterId)source_hard_router_id, (NocRouterId)sink_hard_routed_id, traffic_flow_route, noc_ctx.noc_model);

        // store the number of links in the traffic flow
        golden_traffic_flow_route_sizes[traffic_flow_number] = traffic_flow_route.size();
    }

    SECTION("test_comp_noc_aggregate_bandwidth_cost"){

        //initialize all the cost calculator datastructures
        allocate_and_load_noc_placement_structs();

        // create local variable to store the bandwidth cost
        double golden_total_noc_bandwidth_costs = 0.;

        // now go through all the traffic flows and calculate the bandwidth cost
        for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
            const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);

            // calculate the bandwidth cost
            double current_bandwidth_cost = golden_traffic_flow_route_sizes[traffic_flow_number] * curr_traffic_flow.traffic_flow_bandwidth;
            current_bandwidth_cost *= curr_traffic_flow.traffic_flow_priority;

            golden_total_noc_bandwidth_costs += current_bandwidth_cost;
        }

        // run the test function and get the bandwidth calculated
        double found_bandwidth_cost = comp_noc_aggregate_bandwidth_cost();

        // compare the test function bandwidth cost to the golden value
        // since we are comparing double numbers we allow a tolerance of difference
        REQUIRE(vtr::isclose(golden_total_noc_bandwidth_costs, found_bandwidth_cost));

        // release the cost calculator datastructures
        free_noc_placement_structs();

        // need to delete the local routing algorithm
        delete routing_algorithm;
    }

    SECTION("test_comp_noc_latency_cost"){

        //initialize all the cost calculator datastructures
        allocate_and_load_noc_placement_structs();

        // create the noc options
        t_noc_opts noc_opts;
        noc_opts.noc_latency_constraints_weighting = dist_3(double_engine);
        noc_opts.noc_latency_weighting = dist_3(double_engine);

        // create local variable to store the latency cost
        double golden_total_noc_latency_costs = 0.;

        // local router and link latency parameters
        double router_latency = noc_ctx.noc_model.get_noc_router_latency();
        double link_latency = noc_ctx.noc_model.get_noc_link_latency();

        // now go through all the traffic flows and calculate the latency cost
        for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
            const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);

            double curr_traffic_flow_latency = (router_latency * (golden_traffic_flow_route_sizes[traffic_flow_number] + 1)) + (link_latency * golden_traffic_flow_route_sizes[traffic_flow_number]);

            // calculate the latency cost
            double current_latency_cost = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
            current_latency_cost *= curr_traffic_flow.traffic_flow_priority;

            golden_total_noc_latency_costs += current_latency_cost;
        }

        // run the test function and get the bandwidth calculated
        double found_latency_cost = comp_noc_latency_cost(noc_opts);

        // compare the test function bandwidth cost to the golden value
        // since we are comparing double numbers we allow a tolerance of difference
        REQUIRE(vtr::isclose(golden_total_noc_latency_costs, found_latency_cost));

        // release the cost calculator datastructures
        free_noc_placement_structs();

        // need to delete the local routing algorithm
        delete routing_algorithm;
    }
}

// need to add tets that checks whether this works properly when one router or two routers involved in a move dont have traffic flows associated with them

TEST_CASE("test_find_affected_noc_routers_and_update_noc_costs, test_commit_noc_costs, test_recompute_noc_costs", "[noc_place_utils]") {
    // setup random number generation
    std::random_device device;
    std::mt19937 rand_num_gen(device());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST - 1);
    // this sets the range of possible bandwidths for a traffic flow
    std::uniform_int_distribution<std::mt19937::result_type> dist_2(0, 1000);
    // this sets the range of possible priorities
    std::uniform_int_distribution<std::mt19937::result_type> dist_1(1, 100);
    // for random double number generation for the latency
    std::uniform_real_distribution<double> dist_3(1,25);
    std::default_random_engine double_engine;


    // get global datastructures
    auto& noc_ctx = g_vpr_ctx.mutable_noc();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // start by deleting any global datastructures (this is so that we dont have corruption from previous tests)
    noc_ctx.noc_model.clear_noc();
    noc_ctx.noc_traffic_flows_storage.clear_traffic_flows();
    delete noc_ctx.noc_flows_router;
    place_ctx.block_locs.clear();

    // individual router parameters
    int curr_router_id;
    int router_grid_position_x;
    int router_grid_position_y;
    
    // noc options used in this test
    // we creae these randomly
    t_noc_opts noc_opts;
    noc_opts.noc_latency_constraints_weighting = dist_3(double_engine);
    noc_opts.noc_latency_weighting = dist_3(double_engine);

    // setting the NoC parameters
    noc_ctx.noc_model.set_noc_link_latency(1);
    noc_ctx.noc_model.set_noc_router_latency(1);
    // needs to be the same as above
    double router_latency = noc_ctx.noc_model.get_noc_router_latency();
    double link_latency = noc_ctx.noc_model.get_noc_link_latency();

    // keeps track of which hard router each cluster block is placed
    vtr::vector<ClusterBlockId, NocRouterId> router_where_cluster_is_placed;

    // start by creating the routers
    // add all the routers to noc_storage and populate the golden router set
    for (int router_number = 0; router_number < NUM_OF_ROUTERS_NOC_PLACE_UTILS_TEST; router_number++) {
        // determine the current router parameters
        curr_router_id = router_number;
        router_grid_position_x = router_number % MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;
        router_grid_position_y = router_number / MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST;

        // add the router to the noc
        noc_ctx.noc_model.add_router(curr_router_id, router_grid_position_x, router_grid_position_y);
    }

    noc_ctx.noc_model.make_room_for_noc_router_link_list();

    // now add the links, this will generate a mesh topology with the size being MESH_TOPOLOGY_SIZE
    for (int i = 0; i < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; i++) {
        for (int j = 0; j < MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST; j++) {
            // add a link to the left of the router if there exists another router there
            if ((j - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - 1));
            }
            // add a link to the top of the router if there exists another router there
            if ((i + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
            // add a link to the right of the router if there exists another router there
            if ((j + 1) <= MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST - 1) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) + 1));
            }
            // add a link to the bottom of the router if there exists another router there
            if ((i - 1) >= 0) {
                noc_ctx.noc_model.add_link((NocRouterId)((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j), (NocRouterId)(((i * MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST) + j) - MESH_TOPOLOGY_SIZE_NOC_PLACE_UTILS_TEST));
            }
        }
    }

    // now we need to create router cluster blocks and assing them to placed at a router hard block as an initial position
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
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
    int number_of_created_traffic_flows = 0;

    // now create a random number of traffic flows
    for (int cluster_block_number = 0; cluster_block_number < NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST; cluster_block_number++) {
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
        // randomly choose the traffic flow latency and priority
        double traffic_flow_latency_constraint = (double)dist_3(double_engine);
        int traffic_flow_priority = dist_1(rand_num_gen);

        // create and add the traffic flow
        noc_ctx.noc_traffic_flows_storage.create_noc_traffic_flow(source_traffic_flow_name, sink_traffic_flow_name, source_router_for_traffic_flow, sink_router_for_traffic_flow, traffic_flow_bandwidth_usage, traffic_flow_latency_constraint, traffic_flow_priority);

        number_of_created_traffic_flows++;

        // exit when we have created the required number of traffic flows
        if (number_of_created_traffic_flows == NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST) {
            break;
        }
    }
    noc_ctx.noc_traffic_flows_storage.finshed_noc_traffic_flows_setup();

    // now go and route all the traffic flows //
    // start by creating the routing alagorithm
    NocRouting* routing_algorithm_global = new XYRouting();
    noc_ctx.noc_flows_router = routing_algorithm_global;

    // create a local routing algorithm for the unit test
    NocRouting* routing_algorithm = new XYRouting();

    // store the traffic flow routes found
    vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> golden_traffic_flow_routes;
    golden_traffic_flow_routes.resize(noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows());
    // store the traffic flow bandwidth costs and latency costs
    vtr::vector<NocTrafficFlowId, double> golden_traffic_flow_bandwidth_costs;
    vtr::vector<NocTrafficFlowId, double> golden_traffic_flow_latency_costs;
    golden_traffic_flow_bandwidth_costs.resize(noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows());
    golden_traffic_flow_latency_costs.resize(noc_ctx.noc_traffic_flows_storage.get_number_of_traffic_flows());

    // stores the change in bandwidth and latency costs from the test function
    double test_noc_bandwidth_costs = 0;
    double test_noc_latency_costs = 0;

    // we need to route all the traffic flows based on their initial positions
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
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
    // also initialize the bandwidth and latency costs for all traffic flows
    // and sum them up to calculate the total initial aggregate bandwidth and latency costs for the NoC
    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {
        const t_noc_traffic_flow& curr_traffic_flow = noc_ctx.noc_traffic_flows_storage.get_single_noc_traffic_flow((NocTrafficFlowId)traffic_flow_number);
        std::vector<NocLinkId>& traffic_flow_route = noc_ctx.noc_traffic_flows_storage.get_mutable_traffic_flow_route((NocTrafficFlowId)traffic_flow_number);

        for (auto& link : traffic_flow_route) {
            golden_link_bandwidths[link] += curr_traffic_flow.traffic_flow_bandwidth;

            // update the link bandwidth in the NoC datastructure
            double current_link_bandwidth = noc_ctx.noc_model.get_single_noc_link(link).get_bandwidth_usage();
            noc_ctx.noc_model.get_single_mutable_noc_link(link).set_bandwidth_usage(current_link_bandwidth + curr_traffic_flow.traffic_flow_bandwidth);
        }

        // calculate the bandwidth cost
        golden_traffic_flow_bandwidth_costs[(NocTrafficFlowId)traffic_flow_number] = traffic_flow_route.size() * curr_traffic_flow.traffic_flow_bandwidth;
        golden_traffic_flow_bandwidth_costs[(NocTrafficFlowId)traffic_flow_number] *= curr_traffic_flow.traffic_flow_priority;

        double curr_traffic_flow_latency = (router_latency * (traffic_flow_route.size() + 1)) + (link_latency * traffic_flow_route.size());

        golden_traffic_flow_latency_costs[(NocTrafficFlowId)traffic_flow_number] = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
        golden_traffic_flow_latency_costs[(NocTrafficFlowId)traffic_flow_number] *= curr_traffic_flow.traffic_flow_priority;

        test_noc_bandwidth_costs += golden_traffic_flow_bandwidth_costs[(NocTrafficFlowId)traffic_flow_number];
        test_noc_latency_costs += golden_traffic_flow_latency_costs[(NocTrafficFlowId)traffic_flow_number];

    }

    // initialize noc placement structs
    allocate_and_load_noc_placement_structs();

    // We need to run these functions as they initialize local variables needed to run the test function within this unit test. we assume thi is correct
    comp_noc_aggregate_bandwidth_cost();
    comp_noc_latency_cost(noc_opts);

    // datastructure that keeps track of moved blocks during placement
    t_pl_blocks_to_be_moved blocks_affected(NUM_OF_LOGICAL_ROUTER_BLOCKS_NOC_PLACE_UTILS_TEST);

    // datastructure that keeps track of all the traffic flows that have been re-routed
    std::unordered_set<NocTrafficFlowId> routed_traffic_flows;

    /*  Now we imitate placement here by swapping two clusters block
     * posistions. In each iteration, we first update the positions
     * of the two router cluster blocks, then we update the traffic
     * flows and then the bandwidth usages of the links. Then we call
     * the test function and then move onto the next iteration.
     */
    for (int iteration_number = 0; iteration_number < NUM_OF_PLACEMENT_MOVES_NOC_PLACE_UTILS_TEST; iteration_number++) {
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

                // update the costs now
                golden_traffic_flow_bandwidth_costs[traffic_flow] = golden_traffic_flow_routes[traffic_flow].size() * curr_traffic_flow.traffic_flow_bandwidth;
                golden_traffic_flow_bandwidth_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

                double curr_traffic_flow_latency = (router_latency * (golden_traffic_flow_routes[traffic_flow].size() + 1)) + (link_latency * golden_traffic_flow_routes[traffic_flow].size());

                golden_traffic_flow_latency_costs[traffic_flow] = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
                golden_traffic_flow_latency_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

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

                // update the costs now
                golden_traffic_flow_bandwidth_costs[traffic_flow] = golden_traffic_flow_routes[traffic_flow].size() * curr_traffic_flow.traffic_flow_bandwidth;
                golden_traffic_flow_bandwidth_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

                double curr_traffic_flow_latency = (router_latency * (golden_traffic_flow_routes[traffic_flow].size() + 1)) + (link_latency * golden_traffic_flow_routes[traffic_flow].size());

                golden_traffic_flow_latency_costs[traffic_flow] = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
                golden_traffic_flow_latency_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

                routed_traffic_flows.insert(traffic_flow);
            }
        }

        double delta_aggr_band_cost = 0.;
        double delta_laten_cost = 0.;

        // call the test function
        int number_of_affected_traffic_flows = find_affected_noc_routers_and_update_noc_costs(blocks_affected, delta_aggr_band_cost, delta_laten_cost, noc_opts);

        // update the test total noc bandwidth and latency costs based on the cost changes found by the test functions
        test_noc_bandwidth_costs += delta_aggr_band_cost;
        test_noc_latency_costs += delta_laten_cost;

        // need this function to update the local datastructures that store all the traffic flow costs
        commit_noc_costs(number_of_affected_traffic_flows);

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

        // update the costs now
        golden_traffic_flow_bandwidth_costs[traffic_flow] = golden_traffic_flow_routes[traffic_flow].size() * curr_traffic_flow.traffic_flow_bandwidth;
        golden_traffic_flow_bandwidth_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

        double curr_traffic_flow_latency = (router_latency * (golden_traffic_flow_routes[traffic_flow].size() + 1)) + (link_latency * golden_traffic_flow_routes[traffic_flow].size());

        golden_traffic_flow_latency_costs[traffic_flow] = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
        golden_traffic_flow_latency_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

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

        // update the costs now
        golden_traffic_flow_bandwidth_costs[traffic_flow] = golden_traffic_flow_routes[traffic_flow].size() * curr_traffic_flow.traffic_flow_bandwidth;
        golden_traffic_flow_bandwidth_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;

        double curr_traffic_flow_latency = (router_latency * (golden_traffic_flow_routes[traffic_flow].size() + 1)) + (link_latency * golden_traffic_flow_routes[traffic_flow].size());

        golden_traffic_flow_latency_costs[traffic_flow] = (noc_opts.noc_latency_constraints_weighting * (std::max(0., curr_traffic_flow_latency - curr_traffic_flow.max_traffic_flow_latency))) + (noc_opts.noc_latency_weighting * curr_traffic_flow_latency);
        golden_traffic_flow_latency_costs[traffic_flow] *= curr_traffic_flow.traffic_flow_priority;
    }

    double delta_aggr_band_cost = 0.;
    double delta_laten_cost = 0.;

    // call the test function
    int number_of_affected_traffic_flows = find_affected_noc_routers_and_update_noc_costs(blocks_affected, delta_aggr_band_cost, delta_laten_cost, noc_opts);

    // update the test total noc bandwidth and latency costs based on the cost changes found by the test functions
    test_noc_bandwidth_costs += delta_aggr_band_cost;
    test_noc_latency_costs += delta_laten_cost;

    // need this function to update the local datastructures that store all the traffic flow costs
    commit_noc_costs(number_of_affected_traffic_flows);

    // now verify the function by comparing the link bandwidths in the noc model (should have been updated by the test function) to the golden set
    int number_of_links = golden_link_bandwidths.size();
    for (int link_number = 0; link_number < number_of_links; link_number++) {
        NocLinkId current_link_id = (NocLinkId)link_number;
        const NocLink& current_link = noc_ctx.noc_model.get_single_noc_link(current_link_id);

        REQUIRE(golden_link_bandwidths[current_link_id] == current_link.get_bandwidth_usage());
    }

    // now find the total expected noc aggregate bandwidth and latency cost
    double golden_total_noc_aggr_bandwidth_cost = 0.;
    double golden_total_noc_latency_cost = 0.;

    for (int traffic_flow_number = 0; traffic_flow_number < NUM_OF_TRAFFIC_FLOWS_NOC_PLACE_UTILS_TEST; traffic_flow_number++) {

        golden_total_noc_aggr_bandwidth_cost += golden_traffic_flow_bandwidth_costs[(NocTrafficFlowId)traffic_flow_number];
        golden_total_noc_latency_cost += golden_traffic_flow_latency_costs[(NocTrafficFlowId)traffic_flow_number];
    }

    // now check whether the expected noc costs that we manually calculated above match the noc costs found through the test function (we allow for a tolerance of difference)
    REQUIRE(vtr::isclose(golden_total_noc_latency_cost, test_noc_latency_costs));
    REQUIRE(vtr::isclose(golden_total_noc_aggr_bandwidth_cost, test_noc_bandwidth_costs));

    // now test the recompute cost function //
    // The recompute cost function just adds up all traffic flow costs, so it match the expected noc costs that we manually calculated above by summing up all the expected individual traffic flow costs. //

    // start by resetting the test cost variables
    test_noc_bandwidth_costs = 0.;
    test_noc_latency_costs = 0.;

    // now execute the test function
    recompute_noc_costs(&test_noc_bandwidth_costs, &test_noc_latency_costs);

    // now verify 
    REQUIRE(vtr::isclose(golden_total_noc_latency_cost, test_noc_latency_costs));
    REQUIRE(vtr::isclose(golden_total_noc_aggr_bandwidth_cost, test_noc_bandwidth_costs));

    // delete local datastructures
    free_noc_placement_structs();

    // need to delete local noc routing algorithm
    delete routing_algorithm;
}
TEST_CASE("test_update_noc_normalization_factors", "[noc_place_utils]"){

    // creating local parameters needed for the test
    t_placer_costs costs;
    t_placer_opts placer_opts;

    SECTION("Test case where the bandwidth cost is 0"){
        costs.noc_aggregate_bandwidth_cost = 0.;
        costs.noc_latency_cost = 1.;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // verify the aggregate bandwidth normalized cost
        // this should not be +INF and instead trimmed
        REQUIRE(costs.noc_aggregate_bandwidth_cost_norm == 1.0);
    }
    SECTION("Test case where the latency cost is 0"){
        costs.noc_aggregate_bandwidth_cost = 1.;
        costs.noc_latency_cost = 0.;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // verify the latency normalized cost
        // this should not be +INF and instead trimmed
        REQUIRE(costs.noc_latency_cost_norm == 1.e12);
    }
    SECTION("Test case where the bandwidth cost is an expected value"){
        costs.noc_aggregate_bandwidth_cost = 1.e9;
        costs.noc_latency_cost = 0.;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // verify the aggregate bandwidth normalized cost
        // this should not be trimmed
        REQUIRE(costs.noc_aggregate_bandwidth_cost_norm == 1.e-9);
    }
    SECTION("Test case where the latency cost is an expected value"){
        costs.noc_aggregate_bandwidth_cost = 1.;
        costs.noc_latency_cost = 50.e-12;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // verify the latency normalized cost
        // this should not be trimmed
        REQUIRE(costs.noc_latency_cost_norm == 2.e10);
    }
    SECTION("Test case where the latency cost is lower than the smallest expected value"){
        costs.noc_aggregate_bandwidth_cost = 1.;
        costs.noc_latency_cost = 999.e-15;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // verify the latency normalized cost
        // this should not be trimmed
        REQUIRE(costs.noc_latency_cost_norm == 1.e12);
    }
    SECTION("Test case where the placement algorithm is timing based"){
        costs.noc_aggregate_bandwidth_cost = 1.;
        costs.noc_latency_cost = 1.;

        costs.cost = 10.;

        placer_opts.place_algorithm = e_place_algorithm::SLACK_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // cost should not be updated
        REQUIRE(costs.cost == 10.);

        // try the other timing algorithm
        placer_opts.place_algorithm = e_place_algorithm::CRITICALITY_TIMING_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // cost should not be updated
        REQUIRE(costs.cost == 10.);
    }
    SECTION("Test case where the placement algorithm is not timing based"){
        costs.noc_aggregate_bandwidth_cost = 1.;
        costs.noc_latency_cost = 1.;

        costs.cost = 10.;

        placer_opts.place_algorithm = e_place_algorithm::BOUNDING_BOX_PLACE;

        // run the test function
        update_noc_normalization_factors(costs, placer_opts);

        // cost should be updated
        REQUIRE(costs.cost == 1.);
    }
}
} // namespace
