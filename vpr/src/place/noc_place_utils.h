#ifndef NOC_PLACE_UTILS_H
#define NOC_PLACE_UTILS_H

#include <string_view>
#include "move_utils.h"
#include "place_util.h"

// represent the maximum values of the NoC cost normalization factors //
// we need to handle the case where the aggregate bandwidth is 0, so we set this to some arbitrary positive number that is greater than 1.e-9, since that is the range we expect the normalization factor to be (in Gbps)
constexpr double MAX_INV_NOC_AGGREGATE_BANDWIDTH_COST = 1.;
// we expect the latency costs to be in the pico-second range, and we don't expect it to go lower than that. So if the latency costs go below the pico-second range we trim the normalization value to be no higher than 1/ps
// This should be updated if the delays become lower
constexpr double MAX_INV_NOC_LATENCY_COST = 1.e12;
// we don't expect the noc_latency cost to ever go below 1 pico second.
// So this value represents the lowest possible latency cost.
constexpr double MIN_EXPECTED_NOC_LATENCY_COST = 1.e-12;
// the congestion cost for a link is measured as the proportion of the overloaded BW to the link capacity
// We assume that when a link congested, it is overloaded with at least 0.1% of its BW capacity
constexpr double MAX_INV_NOC_CONGESTION_COST = 1.e3;
// If a link is overloaded by less than 0.1% of the link bandwidth capacity,
// we assume it is not congested.
constexpr double MIN_EXPECTED_NOC_CONGESTION_COST = 1.e-3;

constexpr double INVALID_NOC_COST_TERM = -1.0;

/**
 * @brief Each traffic flow cost consists of two components:
 *        1) traffic flow aggregate bandwidth (sum over all used links of the traffic flow bandwidth)
 *        2) traffic flow latency (currently unloaded/best-case latency of the flow)
 *        3) traffic flow latency overrun (how much the latency is higher than the
 *        latency constraint for a traffic flow.
 *        NoC placement code will keep an array-of-struct to easily access each
 *        traffic flow cost.
 */
struct TrafficFlowPlaceCost {
    double aggregate_bandwidth = INVALID_NOC_COST_TERM;
    double latency = INVALID_NOC_COST_TERM;
    double latency_overrun = INVALID_NOC_COST_TERM;
};

/**
 * @brief Initializes the link bandwidth usage for all NoC links.
 *
 * If traffic flow routes are not passed to this function, it uses a NoC routing algorithm
 * to route all traffic flows. The caller can prevent this function from routing traffic flows
 * by passing routes for all traffic flows. This should be called after initial placement,
 * where all the logical NoC router blocks have been placed for the first time and no traffic
 * flows have been routed yet. In this case an empty vector should be passed to the function.
 * This function can also be called after modification of traffic flow routes. For example,
 * NoC SAT routing algorithm generates new traffic flow routes to avoid congestion. The routes
 * generate by the SAT router should be passed to this function
 *
 *
 * @param new_traffic_flow_routes Traffic flow routes used to initialize link bandwidth utilization.
 * If an empty vector is passed, this function uses a routing algorithm to route traffic flows.
 */
void initial_noc_routing(const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& new_traffic_flow_routes);

/**
 * @brief Re-initializes all link bandwidth usages by either re-routing
 * all traffic flows or using the provided traffic flow routes. This functions
 * also initializes static variables in noc_place_utils.cpp that are used to
 * keep track of NoC-related costs.
 *
 * This function should be called when a placement checkpoint is restored.
 * If the router placement in the checkpoint is different from the last
 * router placement before the checkpoint is restored, link bandwidth usage,
 * traffic flow routes, and static variable in noc_place_utils.cpp are no
 * longer valid and need to be re-initialized.
 *
 * This function should be called after NoC SAT routing algorithm returns its
 * traffic flow routes.
 *
 * @param costs Used to get aggregate bandwidth and latency costs.
 * @param new_traffic_flow_routes Traffic flow routes used to initialize link bandwidth utilization.
* If an empty vector is passed, this function uses a routing algorithm to route traffic flows.
 */
void reinitialize_noc_routing(t_placer_costs& costs,
                              const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& new_traffic_flow_routes);

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
void find_affected_noc_routers_and_update_noc_costs(const t_pl_blocks_to_be_moved& blocks_affected,
                                                    NocCostTerms& delta_c);

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
 * Note that this function does not update the link bandwidth utilization.
 * update_traffic_flow_link_usage() should be called after this function
 * to update the link utilization for the new route. If the flow is re-routed
 * because either its source or destination are moved, update_traffic_flow_link_usage()
 * should be used to reduce the bandwidth utilization for the old route.
 * 
 * @param traffic_flow_id Represents the traffic flow that needs to be routed
 * @param noc_model Contains all the links and routers within the NoC. Used
 * to route traffic flows within the NoC.
 * @param noc_traffic_flows_storage Contains all the traffic flow information
 * within the NoC. Used to get the current traffic flow information.
 * @param noc_flows_router The packet routing algorithm used to route traffic
 * flows within the NoC.
 * @return std::vector<NocLinkId>& The found route for the traffic flow.
 */
std::vector<NocLinkId>& route_traffic_flow(NocTrafficFlowId traffic_flow_id,
                                           const NocStorage& noc_model,
                                           NocTrafficFlows& noc_traffic_flows_storage,
                                           NocRouting& noc_flows_router);

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
void update_traffic_flow_link_usage(const std::vector<NocLinkId>& traffic_flow_route,
                                    NocStorage& noc_model,
                                    int inc_or_dec,
                                    double traffic_flow_bandwidth);

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
 * @param updated_traffic_flows Keeps track of traffic flows that have been
 * re-routed. Used to prevent re-routing the same traffic flow multiple times.
 */
void re_route_associated_traffic_flows(ClusterBlockId moved_router_block_id,
                                       NocTrafficFlows& noc_traffic_flows_storage,
                                       NocStorage& noc_model,
                                       NocRouting& noc_flows_router,
                                       std::unordered_set<NocTrafficFlowId>& updated_traffic_flows);

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
 */
void re_route_traffic_flow(NocTrafficFlowId traffic_flow_id,
                           NocTrafficFlows& noc_traffic_flows_storage,
                           NocStorage& noc_model,
                           NocRouting& noc_flows_router);

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
void recompute_noc_costs(NocCostTerms& new_cost);

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
double comp_noc_aggregate_bandwidth_cost();

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
std::pair<double, double> comp_noc_latency_cost();

double comp_noc_congestion_cost();

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
double calculate_traffic_flow_aggregate_bandwidth_cost(const std::vector<NocLinkId>& traffic_flow_route,
                                                       const t_noc_traffic_flow& traffic_flow_info);

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
 * @return The computed latency cost terms for the given traffic flow.
 * The first element is the total latency experience by the traffic flow.
 * The second one specifies how much the experienced latency exceeds the
 * latency constraint set for this traffic flow.
 */
std::pair<double, double> calculate_traffic_flow_latency_cost(const std::vector<NocLinkId>& traffic_flow_route,
                                                              const NocStorage& noc_model,
                                                              const t_noc_traffic_flow& traffic_flow_info);

/**
 * @brief Determines the congestion cost a NoC link. The cost
 * is calculating by measuring how much the current bandwidth
 * going through the link exceeds the link's bandwidth capacity.
 *
 * @param link The NoC link for which the congestion cost is
 * to be computed
 * @return The computed congestion cost for the given NoC link.
 */
double calculate_link_congestion_cost(const NocLink& link);

/**
 * @brief The user passes weighting factors for aggregate latency
 * and latency overrun terms. The weighting factor for aggregate
 * bandwidth is computed by subtracting two user-provided weighting
 * factor from 1. The computed aggregate bandwidth weighting factor
 * is stored in noc_opts argument.
 *
 * @param noc_opts Contains weighting factors.
 */
void normalize_noc_cost_weighting_factor(t_noc_opts& noc_opts);

/**
 * @brief Computes a weighted average of NoC cost term to determine
 * NoC's contribution to the total placement cost.
 *
 * @param cost_terms Different NoC-related cost terms.
 * @param norm_factors Normalization factors used to scale
 * different NoC-related cost term so that they have similar
 * ranges.
 * @param noc_opts Contains noc_placement_weighting factor
 * to specify the contribution of NoC-related cost to the
 * total placement cost.
 * @return  The computed total NoC-related contribution to the
 * total placement cost.
 */
double calculate_noc_cost(const NocCostTerms& cost_terms,
                          const NocCostTerms& norm_factors,
                          const t_noc_opts& noc_opts);

/**
 * @brief Goes through all the traffic flows and determines whether the
 * latency constraints have been met for each traffic flow. 
 * 
 * @return The total number of traffic flows with latency constraints being met
 */
int get_number_of_traffic_flows_with_latency_cons_met();

/**
 * @brief Goes through all NoC links and counts the congested ones.
 * A congested NoC link is a link whose used bandwidth exceeds its
 * bandwidth capacity.
 *
 * @return The total number of congested NoC links.
 */
int get_number_of_congested_noc_links();

/**
 * @brief Goes through all NoC links and determines whether they
 * are congested or not. Then adds up the congestion ratio of all
 * congested links.
 *
 * @return The total congestion ratio
 */
double get_total_congestion_bandwidth_ratio();

/**
 * @brief Goes through all NoC links and determines whether they
 * are congested or not. Then finds n links that are most congested.
 *
 * @return n links with highest congestion ratio
 */
std::vector<NocLink> get_top_n_congested_links(int n);

/**
 * @brief Goes through all NoC links and determines whether they
 * are congested or not. Then finds n links that are most congested.
 *
 * @return n highest congestion ratios
 */
std::vector<double> get_top_n_congestion_ratios(int n);

/**
 * @brief There are a number of static datastructures which are local
 * to 'noc_place_utils.cpp'. THe purpose of these datastructures is
 * to keep track of the NoC costs for all traffic flows and temporarily
 * store the status of the NoC placement after each move. We create and
 * initialize the datastructures here.
 * 
 * This should be called before starting the simulated annealing placement.
 */
void allocate_and_load_noc_placement_structs();

/**
 * @brief We delete the static datastructures which were created in
 * 'allocate_and_load_noc_placement_structs'.
 * 
 * This should be called after placement is finished.
 */
void free_noc_placement_structs();

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
 * @brief Generates a placement move by first choosing a random router cluster
 * and then choosing a random physical router where the selected router cluster
 * can be moved to. If the selected physical router is already occupied,
 * the proposed move requires swapping two router clusters. If the selected
 * physical router is empty, the proposed move only requires changing the location
 * of the random router cluster. The range in which the physical router is selected
 * is limited such that them maximum distance travelled by the random router cluster
 * does not exceed rlim.
 * 
 * @param blocks_affected Contains one or two router clusters that are proposed
 * to be moved or swapped.
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
void write_noc_placement_file(const std::string& file_name);

/**
 * @brief This function checks whether the routing configuration for NoC traffic flows
 * can cause a deadlock in NoC. Assume we create a graph where NoC routers are vertices,
 * and traffic flow routes represent edges. This graph is a sub-graph of the NoC topology
 * as it contain a subset of its edges. If such a graph contains a cycle, we can argue
 * that deadlock is possible.
 *
 * This functions performs a DFS over the mentioned graph and tries to find out whether
 * the graph has any back edges, i.e. whether a node points to one of its ancestors
 * during depth-first search traversal.
 *
 * @return bool Indicates whether NoC traffic flow routes form a cycle.
 */
bool noc_routing_has_cycle();

/**
 * @brief Check if the channel dependency graph created from the given traffic flow routes
 * has any cycles.
 * @param routes The user provided traffic flow routes.
 * @return True if there is any cycles in the channel dependency graph.
 */
bool noc_routing_has_cycle(const vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>>& routes);

/**
 * @brief Invokes NoC SAT router and print new NoC cost terms after SAT router
 * solved the NoC routing problem.
 *
 * @param costs To be updated with new NoC-related cost terms after traffic flow routes
 * generated by the SAT router replace the old traffic flow routes.
 * @param noc_opts Contains NoC-related cost weighting factor used in the SAT router.
 * @param seed The initialization seed used in the SAT solver.
 */
#ifdef ENABLE_NOC_SAT_ROUTING
void invoke_sat_router(t_placer_costs& costs, const t_noc_opts& noc_opts, int seed);
#endif

/**
 * @brief Prints NoC related costs terms and metrics.
 *
 * @param header The string with which the report starts.
 * @param costs Contains NoC-related cost terms.
 * @param noc_opts Used to compute total NoC cost.
 */
void print_noc_costs(std::string_view header, const t_placer_costs& costs, const t_noc_opts& noc_opts);

#endif