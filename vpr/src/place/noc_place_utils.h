#ifndef NOC_PLACE_UTILS_H
#define NOC_PLACE_UTILS_H

#include "globals.h"
#include "noc_routing.h"
#include "place_util.h"
#include "vtr_assert.h"
#include "move_transactions.h"
#include "vtr_log.h"
#include "noc_routing_algorithm_creator.h"
#include "move_utils.h"
#include "vtr_random.h"
#include "place_constraints.h"
#include "fstream"

// represent the maximum values of the NoC cost normalization factors //
// we need to handle the case where the aggregate bandwidth is 0, so we set this to some arbitrary positive number that is greater than 1.e-9, since that is the range we expect the normalization factor to be (in Gbps)
constexpr double MAX_INV_NOC_AGGREGATE_BANDWIDTH_COST = 1.;
// we expect the latency costs to be in the pico-second range, and we don't expect it to go lower than that. So if the latency costs go below the pico-second range we trim the normalization value to be no higher than 1/ps
// This should be updated if the delays become lower
constexpr double MAX_INV_NOC_LATENCY_COST = 1.e12;

// we don't expect the noc_latency cost to ever go below 1 pico second.
// So this value represents the lowest possible latency cost.
constexpr double MIN_EXPECTED_NOC_LATENCY_COST = 1.e-12;

/**
 * @brief Each traffic flow cost consists of two components:
 *        1) traffic flow aggregate bandwidth (sum over all used links of the traffic flow bandwidth)
 *        2) traffic flow latency (currently unloaded/best-case latency of the flow)
 *        NoC placement code will keep an array-of-struct to easily access each
 *        traffic flow cost.
 */
struct TrafficFlowPlaceCost {
    double aggregate_bandwidth = -1;
    double latency = -1;
};

/**
 * @brief Routes all the traffic flows within the NoC and updates the link usage
 * for all links. This should be called after initial placement, where all the 
 * logical NoC router blocks have been placed for the first time and no traffic
 * flows have been routed yet. This function should also only be used once as
 * its intended use is to initialize the routes for all the traffic flows.
 * 
 * This is different from a complete re-route of the traffic flows as this
 * function assumes the traffic flows have not been routed yet, whereas a 
 * complete re-route function assumes the traffic flows have already been 
 * routed. This is why this function should only be used once.
 * 
 */
void initial_noc_routing(void);

/**
 * @brief Zeros out all link bandwidth usage an re-routes traffic flows.
 * Initializes static variables in noc_place_utils.cpp that are used to
 * keep track of NoC-related costs.
 *
 * This function should be called when a placement checkpoint is restored.
 * If the router placement in the checkpoint is different from the last
 * router placement before the checkpoint is restored, link bandwidth usage,
 * traffic flow routes, and static variable in noc_place_utils.cpp are no
 * longer valid and need to be re-initialized.
 *
 * @param noc_opts NoC-related options used to calculated NoC costs
 * @param costs Used to get aggregate bandwidth and latency costs.
 */
void reinitialize_noc_routing(const t_noc_opts& noc_opts, t_placer_costs& costs);

/**
 * @brief Goes through all the cluster blocks that were moved
 * in a single swap iteration during placement and checks to see
 * if any moved blocks were NoC routers. 
 * 
 * For each moved block that is a NoC router, all the traffic flows
 * that the router is a part of are re-routed. The individual noc placement
 * costs (latency and aggregate bandwidth) are also updated to
 * reflect the re-routed traffic flows. This update is done to the 
 * 'proposed_traffic_flow_aggregate_bandwidth_cost' and 
 * 'proposed_traffic_flow_latency_cost' datastructures found in
 * 'noc_place_utils.cpp'.
 * 
 * Finally the overall change in NoC costs for a given placement
 * iteration is computed.
 * 
 * If none of the moved blocks are NoC routers, then this function
 * does nothing.
 * 
 * This function should be used if the user enabled NoC optimization
 * during placement and after a move has been proposed.
 * 
 * @param blocks_affected Contains all the blocks that were moved in
 * the current placement iteration. This includes the cluster ids of
 * the moved blocks, their previous locations and their new locations
 * after being moved.
 * @param noc_aggregate_bandwidth_delta_c The change in the overall
 * NoC aggregate bandwidth cost caused by a placer move is stored
 * here.
 * @param noc_latency_delta_c The change in the overall
 * NoC latency cost caused by a placer move is stored
 * here.
 */
void find_affected_noc_routers_and_update_noc_costs(const t_pl_blocks_to_be_moved& blocks_affected, double& noc_aggregate_bandwidth_delta_c, double& noc_latency_delta_c, const t_noc_opts& noc_opts);

/**
 * @brief Updates static datastructures found in 'noc_place_utils.cpp'
 * which keep track of the aggregate bandwidth and latency costs of all
 * traffic flows in the design. 
 * 
 * Go through the 'affected_traffic_flows' datastructure which contains
 * the traffic flows which have been modified in a given placement 
 * iteration. For each traffic flow update the aggregate bandwidth and
 * latency costs within the 'traffic_flow_aggregate_bandwidth_cost' and
 * 'traffic_flow_latency_cost' datastructures with their proposed values
 * which can be found in the 'proposed_traffic_flow_aggregate_bandwidth_cost'
 * and 'proposed_traffic_flow_latency_cost' datastructures.
 * 
 * This function should be used after a proposed move which includes NoC
 * router blocks (logical) is accepted. The move needs to be accepted
 * since the affected traffic flow costs are updated here to reflect the
 * current placement and the placement is only updated when a move is
 * accepted.
 */
void commit_noc_costs();

/**
 * @brief Routes a given traffic flow within the NoC based on where the
 * logical cluster blocks in the traffic flow are currently placed. The
 * found route is stored and returned externally.
 * 
 * First, the hard routers blocks that represent the placed location of
 * the router cluster blocks are identified. Then the traffic flow
 * is routed and updated.
 * 
 * @param traffic_flow_id Represents the traffic flow that needs to be routed
 * @param noc_model Contains all the links and routers within the NoC. Used
 * to route traffic flows within the NoC.
 * @param noc_traffic_flows_storage Contains all the traffic flow information
 * within the NoC. Used to get the current traffic flow information.
 * @param noc_flows_router The packet routing algorithm used to route traffic
 * flows within the NoC.
 * @param placed_cluster_block_locations A datastructure that identifies the
 * placed grid locations of all cluster blocks.
 * @return std::vector<NocLinkId>& The found route for the traffic flow.
 */
std::vector<NocLinkId>& get_traffic_flow_route(NocTrafficFlowId traffic_flow_id, const NocStorage& noc_model, NocTrafficFlows& noc_traffic_flows_storage, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations);

/**
 * @brief Updates the bandwidth usages of links found in a routed traffic flow.
 * The link bandwidth usages are either incremented or decremented by the
 * bandwidth of the traffic flow. If the traffic flow route is being deleted,
 * then the link bandwidth needs to be decremented. If the traffic flow
 * route has just been added then the link bandwidth needs to be incremented.
 * This function needs to be called everytime a traffic flow has been newly
 * routed.
 * 
 * @param traffic_flow_route The routed path for a traffic flow. This
 * contains a collection of links in the NoC.
 * @param noc_model Contains all the links and routers within the NoC. Used
 * to update link information.
 * @param inc_or_dec Determines how the bandwidths of links found
 * in the traffic flow route are updated. If it is -1, the route flow has
 * been removed and links' bandwidth must be decremented. Otherwise, the a traffic
 * flow has been re-routed and its links' bandwidth should be incremented.
 * @param traffic_flow_bandwidth The bandwidth of a traffic flow. This will
 * be used to update bandwidth usage of the links.
 */
void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route, NocStorage& noc_model, int inc_or_dec, double traffic_flow_bandwidth);

/**
 * @brief Goes through all the traffic flows associated to a moved
 * logical router cluster block (a traffic flow is associated to a router if
 * the router is either a source or sink router of the traffic flow) and
 * re-routes them. The new routes are stored and the NoC cost is updated
 * to reflect the moved logical router cluster block.
 * 
 * The associated traffic flows and their newly found costs are stored in static
 * datastructures found in 'noc_place_utils.cpp'. The size of these
 * datastructures represent the total number of affected traffic flows which is
 * also updated within here.
 * 
 * @param moved_router_block_id The logical router cluster block that was moved
 * to a new location during placement.
 * @param noc_traffic_flows_storage Contains all the traffic flow information
 * within the NoC. Used to get the traffic flows associated to logical router
 * blocks.
 * @param noc_model Contains all the links and routers within the NoC. Used
 * to route traffic flows within the NoC.  
 * @param noc_flows_router The packet routing algorithm used to route traffic
 * flows within the NoC.
 * @param placed_cluster_block_locations A datastructure that identifies the
 * placed grid locations of all cluster blocks.
 * @param updated_traffic_flows Keeps track of traffic flows that have been
 * re-routed. Used to prevent re-routing the same traffic flow multiple times.
 */
void re_route_associated_traffic_flows(ClusterBlockId moved_router_block_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations, std::unordered_set<NocTrafficFlowId>& updated_traffic_flows);

/**
 * @brief Used to re-route all the traffic flows associated to logical
 * router blocks that were supposed to be moved during placement but are
 * back to their original positions. 
 * 
 * The routing function is called to find the original traffic flow route
 * again.
 * 
 * @param blocks_affected Contains all the blocks that were moved in
 * the current placement iteration. This includes the cluster ids of
 * the moved blocks, their previous locations and their new locations
 * after being moved.
 */
void revert_noc_traffic_flow_routes(const t_pl_blocks_to_be_moved& blocks_affected);

/**
 * @brief Removes the route of a traffic flow and updates the links to indicate
 * that the traffic flow does not use them. And then finds
 * a new route for the traffic flow and updates the links in the new route to
 * indicate that the traffic flow uses them.
 * 
 * @param traffic_flow_id The traffic flow to re-route.
 * @param noc_traffic_flows_storage Contains all the traffic flow information
 * within the NoC. Used to get the current traffic flow information.
 * @param noc_model Contains all the links and routers within the NoC. Used
 * to route traffic flows within the NoC.
 * @param noc_flows_router The packet routing algorithm used to route traffic
 * flows within the NoC.
 * @param placed_cluster_block_locations A datastructure that identifies the
 * placed grid locations of all cluster blocks.
 */
void re_route_traffic_flow(NocTrafficFlowId traffic_flow_id, NocTrafficFlows& noc_traffic_flows_storage, NocStorage& noc_model, NocRouting& noc_flows_router, const vtr::vector_map<ClusterBlockId, t_block_loc>& placed_cluster_block_locations);

/**
 * @brief Recompute the NoC costs (aggregate bandwidth and latency) by
 * accumulating the individual traffic flow costs and then verify
 * whether the result is within an error tolerance of the placements
 * NoC costs.
 * 
 * During placement, the NoC aggregate bandwidth and latency costs are
 * incrementally updated by adding the change in NoC costs caused by
 * each accepted move. This function quickly verifies whether
 * the incremental changes to the NoC costs are correct by comparing
 * the current costs to newly computed costs for the current
 * placement state. This function assumes the traffic flows have
 * been routed and all the individual NoC costs of all traffic flows are
 * correct and only accumulates them to compute the new overall NoC costs.
 * 
 * If the comparison is larger than the error tolerance then it
 * implies that the incremental cost updates were incorrect and 
 * an error is thrown. 
 * 
 * This function is not very expensive and can be called regularly during
 * placement to ensure the NoC costs do not deviate too far off
 * from their correct values.
 * 
 * @param new_noc_aggregate_bandwidth_cost Will store the newly computed
 * NoC aggregate bandwidth cost for the current placement state.
 * @param new_noc_latency_cost Will store the newly computed
 * NoC latency cost for the current placement state.
 */
void recompute_noc_costs(double& new_noc_aggregate_bandwidth_cost, double& new_noc_latency_cost);

/**
 * @brief Updates all the cost normalization factors relevant to the NoC.
 * Handles exceptional cases so that the normalization factors do not
 * reach INF.
 * This is intended to be used to initialize the normalization factors of
 * the NoC and also at the outer loop iteration of placement to 
 * balance the NoC costs with other placement cost parameters.
 * 
 * @param costs Contains the normalization factors which need to be updated
 */
void update_noc_normalization_factors(t_placer_costs& costs);

/**
 * @brief Calculates the aggregate bandwidth of each traffic flow in the NoC
 * and initializes local variables that keep track of the traffic flow 
 * aggregate bandwidths cost.
 * Then the total aggregate bandwidth cost is determines by summing up all
 * the individual traffic flow aggregate bandwidths.
 * 
 * This should be used after initial placement to determine the starting
 * aggregate bandwidth cost of the NoC.
 * 
 * @return double The aggregate bandwidth cost of the NoC.
 */
double comp_noc_aggregate_bandwidth_cost(void);

/**
 * @brief Calculates the latency cost of each traffic flow in the NoC
 * and initializes local variables that keep track of the traffic flow
 * latency costs. Then the total latency cost is determined by summing up all
 * the individual traffic flow latency costs.
 * 
 * This should be used after initial placement to determine the starting latency
 * cost of the NoC.
 * 
 * @return double The latency cost of the NoC.
 */
double comp_noc_latency_cost(const t_noc_opts& noc_opts);

/**
 * @brief Given a placement state the NoC costs are re-computed
 * from scratch and compared to the current NoC placement costs.
 * This involves finding new routes for all traffic 
 * flows and then computing the aggregate bandwidth and latency costs
 * for the traffic flows using the newly found routes. Then the 
 * overall NoC costs are computed by accumulating the newly found traffic flow
 * costs. 
 * 
 * THe newly computed NoC costs are compared to the current NoC costs to
 * check if they are within an error tolerance. If the comparison is
 * larger than the error tolerance then an error is thrown and it indicates
 * that the incremental NoC costs updates are incorrect. 
 * 
 * This function is similar to 'recompute_noc_costs' but the traffic flow
 * routes and costs are computed as well. As a result this function is very
 * expensive and should be used in larger intervals (number of moves) within
 * the placer.
 * 
 * @param costs Contains the current NoC aggregate bandwidth and latency costs
 * which are computed incrementally after each logical router move during
 * placement.
 * @param error_tolerance The maximum allowable difference between the current
 * NoC costs and the newly computed NoC costs. 
 * @param noc_opts Contains information necessary to compute the NoC costs
 * from scratch. For example this would include the routing algorithm and
 * weights for the difference components of the NoC costs.
 * @return An integer which represents the status of the comparison. 0
 * indicates that the current NoC costs are within the error tolerance and
 * a non-zero values indicates the current NoC costs are above the error
 * tolerance.
 */
int check_noc_placement_costs(const t_placer_costs& costs, double error_tolerance, const t_noc_opts& noc_opts);

/**
 * @brief Determines the aggregate bandwidth cost of a routed traffic flow.
 * The cost is calculated as the number of links in the traffic flow multiplied
 * by the traffic flow bandwidth. This is then scaled by the priority of the
 * traffic flow. THis assumes the provided traffic flow has been routed.
 * 
 * @param traffic_flow_route The routed path for a traffic flow. This
 * contains a collection of links in the NoC.
 * @param traffic_flow_info  Contains the traffic flow bandwidth and
 * its priority.
 * @return The computed aggregate bandwidth for the provided traffic flow
 */
double calculate_traffic_flow_aggregate_bandwidth_cost(const std::vector<NocLinkId>& traffic_flow_route, const t_noc_traffic_flow& traffic_flow_info);

/**
 * @brief Determines the latency cost of a routed traffic flow.
 * The cost is calculated as the combination of the traffic flow
 * latency and its gap to the traffic flow latency constraint. This
 * assumes that the provided traffic flow has been routed. Each
 * parameter above is scaled by a user-provided weighting factor. 
 * This is then scaled by the priority of the traffic flow.
 *  
 * @param traffic_flow_route The routed path for a traffic flow. This
 * contains a collection of links in the NoC.
 * @param noc_model Contains noc information such as the router and link
 * latencies.
 * @param traffic_flow_info Contains the traffic flow priority.
 * @param noc_opts Contains the user provided weightings of the traffic flow 
 * latency and its constraint parameters for the cost calculation.
 * @return THe computed latency for the provided traffic flow
 */
double calculate_traffic_flow_latency_cost(const std::vector<NocLinkId>& traffic_flow_route, const NocStorage& noc_model, const t_noc_traffic_flow& traffic_flow_info, const t_noc_opts& noc_opts);

/**
 * @brief Goes through all the traffic flows and determines whether the
 * latency constraints have been met for each traffic flow. 
 * 
 * @return The total number of traffic flows with latency constraints being met
 */
int get_number_of_traffic_flows_with_latency_cons_met(void);

/**
 * @brief There are a number of static datastructures which are local
 * to 'noc_place_utils.cpp'. THe purpose of these datastructures is
 * to keep track of the NoC costs for all traffic flows and temporarily
 * store the status of the NoC placement after each move. We create and
 * initialize the datastructures here.
 * 
 * This should be called before starting the simulated annealing placement.
 * 
 */
void allocate_and_load_noc_placement_structs(void);

/**
 * @brief We delete the static datastructures which were created in
 * 'allocate_and_load_noc_placement_structs'.
 * 
 * This should be called after placement is finished.
 */
void free_noc_placement_structs(void);

/* Below are functions related to the feature that forces to the placer to swap router blocks for a certain percentage of the total number of swaps */

/**
 * @brief User supplied a number that represents the additional percentage of 
 * router block swaps relative to the total number of swaps. 
 * 
 * @param user_supplied_noc_router_swap_percentage The expected number of 
 * noc router block swaps as a percentage of the total number of swaps
 * attempted by the placer.
 * @return true This indicates that a router block should be swapped
 * @return false This indicates that a router block should not be swapped
 */
bool check_for_router_swap(int user_supplied_noc_router_swap_percentage);

/**
 * @brief Generates a placement move by choosing two router cluster blocks to 
 * swap. First, a random router cluster block is chosen and then another router
 * cluster block is chosen that can be swapped with the initial block such that
 * the distance travelled by either block does not exceed rlim. 
 * 
 * @param blocks_affected The two router cluster blocks that are proposed to be
 * swapped
 * @param rlim The maximum distance in the x and y direction that a router 
 * cluster block can travel (this is within the compressed block space) 
 * @return e_create_move Result of proposing the move
 */
e_create_move propose_router_swap(t_pl_blocks_to_be_moved& blocks_affected, float rlim);

/**
 * @brief Writes out the locations of the router cluster blocks in the
 * final placement. This file contains only NoC routers and the 
 * information is written out in a format that is compatible with the RADSim
 * imulator place file. The output of this function is a text file.
 * 
 * Sample placement file output:
 *      router_cluster_name layer_number physical_router_id
 *      ...
 *      ...
 *      ...
 * 
 * @param file_name The name of the output file that contain the NoC placement
 * information.
 * 
 */
void write_noc_placement_file(std::string file_name);
#endif