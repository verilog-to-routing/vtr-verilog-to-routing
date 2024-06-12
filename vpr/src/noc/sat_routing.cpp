#ifdef ENABLE_NOC_SAT_ROUTING


#include "sat_routing.h"
#include "turn_model_routing.h"

#include "globals.h"
#include "vtr_time.h"

#include <unordered_map>

#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"

namespace orsat = operations_research::sat;

/**
 * For each traffic flow and NoC link pair, we create a boolean variable.
 * When a variable associated with traffic flow t and NoC link l is set,
 * it means that t is routed through l.
 */
typedef std::unordered_map<std::pair<NocTrafficFlowId, NocLinkId>, orsat::BoolVar> t_flow_link_var_map;


/**
 * @brief  Creates a boolean variable for each (traffic flow, link) pair.
 * It also create integer variables for latency-constrained traffic flows.
 *
 * @param cp_model The CP model builder object. Created variables are added
 * to this model builder object.
 * @param flow_link_vars The created boolean variables for (traffic flow, link) pairs
 * are stored in this container to be used for adding constraints and defining objective
 * functions.
 * @param latency_overrun_vars The created integer variables for latency-constrained
 * traffic flows are added to this container for future use, e.g. adding constraints
 * and defining objective functions.
 */
static void create_flow_link_vars(orsat::CpModelBuilder& cp_model,
                                  t_flow_link_var_map& flow_link_vars,
                                  std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars);

/**
 * @brief Translates a latency constraint for a traffic flow to the maximum number
 * of links that the traffic flow can traverse without violating the latency constraint.
 *
 * This translation is possible only when all NoC routers or links have the same latency.
 * NoC routers can have a different latency than NoC links, but all router (or links) must
 * have the same latency.
 *
 * @param traffic_flow_id The unique ID of the latency-constrained traffic flow.
 * @return The maximum number links that can travelled by the given traffic flow without
 * violating the latency constraint.
 */
static int comp_max_number_of_traversed_links(NocTrafficFlowId traffic_flow_id);

/**
 * @brief Performs an outer product between traffic_flow_ids and noc_link_ids
 * and returns all boolean variables for the resulting (traffic flow, link) pairs.
 *
 * @param map The container that stores all boolean variables for all
 * (traffic flow, link) pairs.
 * @param traffic_flow_ids Traffic flows whose boolean variables are requested.
 * @param noc_link_ids NoC links whose boolean variables are requested.
 * @return A vector of boolean variables for the requested (traffic flow, link) pairs.
 */
static std::vector<orsat::BoolVar> get_flow_link_vars(const t_flow_link_var_map& map,
                                                      const std::vector<NocTrafficFlowId>& traffic_flow_ids,
                                                      const std::vector<NocLinkId>& noc_link_ids);

/**
 * @brief Adds constraints for latency_overrun_vars variables to make sure
 * that they count the number of extra links a traffic flow is traversing
 * beyond what its latency constraint allows. For example, if a traffic flow
 * latency constraint allows the traffic flow to traverse at most 3 links,
 * but 4 (traffic flow, link) boolean variables are activated for this traffic flow,
 * the corresponding integer variable in latency_overrun_vars should take a value
 * of 1 because an extra link beyond the maximum 3 links has been activated.
 *
 * @param cp_model The CP model builder object. New constraints are added
 * to this model builder object.
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param latency_overrun_vars The integer variables that are to be constrained.
 */
static void constrain_latency_overrun_vars(orsat::CpModelBuilder& cp_model,
                                           t_flow_link_var_map& flow_link_vars,
                                           std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars);

/**
 * @brief Forbids specific turns that traffic flows can take.
 * Turn model routing algorithms forbid specific turns in a mesh topology
 * to make sure that deadlock does not happen. A turn can be determined by
 * specifying two consecutive links. A turn can be forbidden in the SAT
 * formulation by making sure that at most one of two consecutive links
 * that specify a turn is activated.
 *
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param cp_model The CP model builder object. New constraints are added
 * to this model builder object.
 */
static void forbid_illegal_turns(t_flow_link_var_map& flow_link_vars,
                                 orsat::CpModelBuilder& cp_model);

/**
 * @brief Creates a boolean variable for each link to indicate
 * whether it is congested. This function adds some constraints
 * to enforce congested_link_vars boolean variables to be set to 1
 * when their corresponding link congested.
 *
 * Since the SAT solver cannot work with floating point numbers,
 * all link and traffic flow bandwidths are quantized.
 *
 * @param congested_link_vars To be filled with created boolean variables
 * to indicate whether links are congested.
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param cp_model The CP model builder object. Created variables are added
 * to this model builder object. Constraints are also added to this object.
 * @param bandwidth_resolution Specifies the resolution by which bandwidth
 * values are quantized.
 */
static void create_congested_link_vars(vtr::vector<NocLinkId , orsat::BoolVar>& congested_link_vars,
                                       t_flow_link_var_map& flow_link_vars,
                                       orsat::CpModelBuilder& cp_model,
                                       int bandwidth_resolution);

/**
 * @brief Quantize traffic flow bandwidths. The maximum NoC link bandwidth is
 * quantized to the specified bandwidth resolution, and traffic flow bandwidths
 * are quantized accordingly.
 *
 * @param bandwidth_resolution The resolution by which traffic flow bandwidth are
 * quantized. Quantized bandwidths increment by link_bandwidth/resolution.
 * @return A vector of quantized traffic flow bandwidths.
 */
static vtr::vector<NocTrafficFlowId, int> quantize_traffic_flow_bandwidths(int bandwidth_resolution);

/**
 * @brief Adds constraints to ensure that activated (traffic flow, link)
 * boolean variables for a specific traffic flow create a continuous route
 * starting from the source router and arriving at the destination.
 *
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param cp_model The CP model builder object. New constraints are added
 * to this model builder object.
 */
static void add_continuity_constraints(t_flow_link_var_map& flow_link_vars,
                                       orsat::CpModelBuilder& cp_model);

/**
 * @brief Creates a linear expression to be minimized by the SAT solver.
 * This objective function is a linear combination of latency overrun,
 * the number of congested links, and the quantized aggregate bandwidth.
 *
 * @param cp_model The CP model builder object. New constraints are added
 * to this model builder object.
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param latency_overrun_vars Integer variables for latency-constrained
 * traffic flows. Each integer variable shows how many extra links a constrained
 * traffic flow has traversed beyond what its latency constraint allows.
 * @param congested_link_vars Boolean variables indicating whether a link is
 * congested or not.
 * @param bandwidth_resolution The resolution by which traffic flow bandwidths
 * are quantized.
 * @param latency_overrun_weight Specifies the importance of minimizing latency overrun
 * for latency-constrained traffic flows.
 * @param congestion_weight Specifies the importance of avoiding congestion in links.
 * @param minimize_aggregate_bandwidth Specifies whether the objective includes an
 * aggregate bandwidth term.
 *
 * @return A linear expression including latency overrun, the number of congested links,
 * and the aggregate bandwidth;
 */
static orsat::LinearExpr create_objective(orsat::CpModelBuilder& cp_model,
                                          t_flow_link_var_map& flow_link_vars,
                                          std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars,
                                          vtr::vector<NocLinkId , orsat::BoolVar>& congested_link_vars,
                                          int bandwidth_resolution,
                                          int latency_overrun_weight,
                                          int congestion_weight,
                                          bool minimize_aggregate_bandwidth);

/**
 * @brief Converts the activated (traffic flow, link) boolean variables to
 * traffic flow routes.
 *
 * Note that traffic flow routes are not sorted in traversal order. Each
 * traffic flow route contains NoC links that constitute the route, but these
 * links may not stored in the route traversal order. For re-ordering traffic flow
 * routes in route traversal order, call sort_noc_links_in_chain_order() function.
 *
 * @param flow_link_vars Boolean variable container for (traffic flow, link) pairs.
 * @param response The SAT solver's response. This object is used to query the values
 * of (traffic flow, link) variables.
 * @return Traffic flow routes. NoC links may not be stored in the route traversal order.
 */
static vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> convert_vars_to_routes(t_flow_link_var_map& flow_link_vars,
                                                                                    const orsat::CpSolverResponse& response);

/**
 * @brief Sorts the given NoC links so that they can traversed one after another.
 *
 * After the SAT solver returns a solution, boolean (traffic flow, link) variables
 * can be translated into traffic flow routes. The SAT solver does not have a notion
 * traversal order. Therefore, activated NoC links for a traffic flow are stored in
 * an arbitrary order. This function can reorder the activated links in the route
 * traversal order.
 *
 * @param links NoC links that form a continuous route. NoC links can be stored
 * in an order different than the traversal order.
 * @return Sorted links in traversal order.
 */
static std::vector<NocLinkId> sort_noc_links_in_chain_order(const std::vector<NocLinkId>& links);


static std::vector<orsat::BoolVar> get_flow_link_vars(const t_flow_link_var_map& map,
                                                      const std::vector<NocTrafficFlowId>& traffic_flow_ids,
                                                      const std::vector<NocLinkId>& noc_link_ids) {
    std::vector<orsat::BoolVar> results;
    for (auto traffic_flow_id : traffic_flow_ids) {
        for (auto noc_link_id : noc_link_ids) {
            auto it = map.find({traffic_flow_id, noc_link_id});
            if (it != map.end()) {
                results.push_back(it->second);
            }
        }
    }

    return results;
}

static void forbid_illegal_turns(t_flow_link_var_map& flow_link_vars,
                                 orsat::CpModelBuilder& cp_model) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    auto noc_routing_alg = dynamic_cast<const TurnModelRouting*> (noc_ctx.noc_flows_router.get());
    // ensure that the routing algorithm is a turn model algorithm
    VTR_ASSERT(noc_routing_alg != nullptr);

    // forbid illegal turns based on the routing algorithm
    // this includes 180 degree turns
    for (const auto& [link1, link2] : noc_routing_alg->get_all_illegal_turns(noc_ctx.noc_model)) {
        for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
            auto& first_var = flow_link_vars[{traffic_flow_id, link1}];
            auto& second_var = flow_link_vars[{traffic_flow_id, link2}];
            // at most one of two consecutive links that form a turn can be activated
            cp_model.AddBoolOr({first_var.Not(), second_var.Not()});
        }
    }
}

static vtr::vector<NocTrafficFlowId, int> quantize_traffic_flow_bandwidths(int bandwidth_resolution) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    //TODO: support heterogeneous bandwidth
    const auto& noc_links = noc_ctx.noc_model.get_noc_links();
    const double link_bandwidth = noc_links.front().get_bandwidth();
    auto it = std::adjacent_find(noc_links.begin(), noc_links.end(), [](const NocLink& a, const NocLink& b){
        return a.get_bandwidth() != b.get_bandwidth();
    });

    if (it != noc_links.end()) {
        const NocLink& first_link = *it;
        const NocLink& second_link = *(it + 1);
        VTR_LOG_ERROR(
            "SAT router assumes all NoC links have the same bandwidth. "
            "NoC links %d and %d have different bandwidth: %g and %g",
            (size_t)first_link.get_link_id(), (size_t)second_link.get_link_id(),
            first_link.get_bandwidth(), second_link.get_bandwidth());
    }

    vtr::vector<NocTrafficFlowId, int> rescaled_traffic_flow_bandwidths;
    rescaled_traffic_flow_bandwidths.resize(traffic_flow_storage.get_number_of_traffic_flows());

    // rescale traffic flow bandwidths
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);
        double bandwidth = traffic_flow.traffic_flow_bandwidth;
        int rescaled_bandwidth = (int)std::floor((bandwidth / link_bandwidth) * bandwidth_resolution);
        rescaled_traffic_flow_bandwidths[traffic_flow_id] = rescaled_bandwidth;
    }

    return  rescaled_traffic_flow_bandwidths;
}

static void create_congested_link_vars(vtr::vector<NocLinkId , orsat::BoolVar>& congested_link_vars,
                                       t_flow_link_var_map& flow_link_vars,
                                       orsat::CpModelBuilder& cp_model,
                                       int bandwidth_resolution) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    // quantize traffic flow bandwidth
    vtr::vector<NocTrafficFlowId, int> rescaled_traffic_flow_bandwidths = quantize_traffic_flow_bandwidths(bandwidth_resolution);

    // go over all NoC links and create a boolean variable for each one to indicate if it is congested
    for (const auto& noc_link : noc_ctx.noc_model.get_noc_links()) {
        const NocLinkId noc_link_id = noc_link.get_link_id();
        orsat::LinearExpr bandwidth_load;

        // compute the total bandwidth routed through this link
        for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
            orsat::BoolVar binary_var = flow_link_vars[{traffic_flow_id, noc_link_id}];
            bandwidth_load += orsat::LinearExpr::Term(binary_var, rescaled_traffic_flow_bandwidths[traffic_flow_id]);
        }

        orsat::BoolVar congested = cp_model.NewBoolVar();
        cp_model.AddLessOrEqual(bandwidth_load, bandwidth_resolution).OnlyEnforceIf(congested.Not());
        cp_model.AddGreaterThan(bandwidth_load, bandwidth_resolution).OnlyEnforceIf(congested);
        congested_link_vars.push_back(congested);
    }
}

static void add_continuity_constraints(t_flow_link_var_map& flow_link_vars,
                                       orsat::CpModelBuilder& cp_model) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;
    const auto& place_ctx = g_vpr_ctx.placement();

    // constrain the links that can be activated for each traffic flow in a way that they
    // form a continuous route
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // get the source and destination logical router blocks in the current traffic flow
        ClusterBlockId logical_source_router_block_id = traffic_flow.source_router_cluster_id;
        ClusterBlockId logical_sink_router_block_id = traffic_flow.sink_router_cluster_id;

        // get the ids of the hard router blocks where the logical router cluster blocks have been placed
        NocRouterId source_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_source_router_block_id].loc);
        NocRouterId sink_router_id = noc_ctx.noc_model.get_router_at_grid_location(place_ctx.block_locs[logical_sink_router_block_id].loc);

        // exactly one outgoing link of the source must be selected
        const auto& src_outgoing_link_ids = noc_ctx.noc_model.get_noc_router_outgoing_links(source_router_id);
        auto src_outgoing_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, src_outgoing_link_ids);
        cp_model.AddExactlyOne(src_outgoing_vars);

        // exactly one incoming link of the sink must be selected
        const auto& dst_incoming_link_ids = noc_ctx.noc_model.get_noc_router_incoming_links(sink_router_id);
        auto dst_incoming_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, dst_incoming_link_ids);
        cp_model.AddExactlyOne(dst_incoming_vars);

        // each NoC router has at most one incoming and one outgoing link activated
        for (const auto& noc_router : noc_ctx.noc_model.get_noc_routers()) {
            const int noc_router_user_id = noc_router.get_router_user_id();
            const NocRouterId noc_router_id = noc_ctx.noc_model.convert_router_id(noc_router_user_id);

            // the links connected to source and destination routers have already been constrained
            if (noc_router_id == source_router_id || noc_router_id == sink_router_id) {
                continue;
            }


            // for each intermediate router, at most one incoming link can be activated to route this traffic flow
            const auto& incoming_links = noc_ctx.noc_model.get_noc_router_incoming_links(noc_router_id);
            auto incoming_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, incoming_links);
            cp_model.AddAtMostOne(incoming_vars);

            // for each intermediate router, at most one outgoing link can be activated to route this traffic flow
            const auto& outgoing_links = noc_ctx.noc_model.get_noc_router_outgoing_links(noc_router_id);
            auto outgoing_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id}, outgoing_links);
            cp_model.AddAtMostOne(outgoing_vars);

            // count the number of activated incoming links for this traffic flow
            orsat::LinearExpr incoming_vars_sum = orsat::LinearExpr::Sum(incoming_vars);

            // count the number of activated outgoing links for this traffic flow
            orsat::LinearExpr outgoing_vars_sum = orsat::LinearExpr::Sum(outgoing_vars);

            /* the number activated incoming and outgoing links must be equal/
             * Either they are both 0, meaning that this NoC routers is not on the
             * traffic flow router, or they are both 1, implying the traffic flow route
             * goes through this NoC router.*/
            cp_model.AddEquality(incoming_vars_sum, outgoing_vars_sum);
        }
    }
}

static std::vector<NocLinkId> sort_noc_links_in_chain_order(const std::vector<NocLinkId>& links) {
    std::vector<NocLinkId> route;
    if (links.empty()) {
        return route;
    }

    const auto& noc_model = g_vpr_ctx.noc().noc_model;

    // Create a map to find pairs by their first element
    std::unordered_map<NocRouterId , NocLinkId> src_map;
    std::unordered_map<NocRouterId , bool> is_dst;
    for (const auto l : links) {
        NocRouterId src_router_id = noc_model.get_single_noc_link(l).get_source_router();
        NocRouterId dst_router_id = noc_model.get_single_noc_link(l).get_sink_router();
        src_map[src_router_id] = l;
        is_dst[dst_router_id] = true;
    }

    // Find the starting pair (whose first element is not a second element of any pair)
    auto it = links.begin();
    for (; it != links.end(); ++it) {
        NocRouterId src_router_id = noc_model.get_single_noc_link(*it).get_source_router();
        if (is_dst.find(src_router_id) == is_dst.end()) {
            break;
        }
    }

    // Reconstruct the chain starting from the found starting pair
    auto current = *it;
    while (true) {
        route.push_back(current);
        NocRouterId dst_router_id = noc_model.get_single_noc_link(current).get_sink_router();
        auto nextIt = src_map.find(dst_router_id);
        if (nextIt == src_map.end()) {
            break; // End of chain
        }
        current = nextIt->second;
    }

    VTR_ASSERT(route.size() == links.size());

    return route;
}

static vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> convert_vars_to_routes(t_flow_link_var_map& flow_link_vars,
                                                                                    const orsat::CpSolverResponse& response) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    VTR_ASSERT(response.status() == orsat::CpSolverStatus::FEASIBLE ||
               response.status() == orsat::CpSolverStatus::OPTIMAL);

    vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> routes;
    routes.resize(traffic_flow_storage.get_number_of_traffic_flows());

    for (auto& [key, var] : flow_link_vars) {
        auto [traffic_flow_id, noc_link_id] = key;
        bool value = orsat::SolutionBooleanValue(response, var);
        if (value) {
            routes[traffic_flow_id].push_back(noc_link_id);
        }
    }

    for (auto& route : routes) {
        route = sort_noc_links_in_chain_order(route);
    }

    return routes;
}

static void create_flow_link_vars(orsat::CpModelBuilder& cp_model,
                                  t_flow_link_var_map& flow_link_vars,
                                  std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;
    // used to access NoC compressed grid
    const auto& place_ctx = g_vpr_ctx.placement();
    // used to get NoC logical block type
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    // Get the logical block type for router
    const auto router_block_type = cluster_ctx.clb_nlist.block_type(noc_ctx.noc_traffic_flows_storage.get_router_clusters_in_netlist()[0]);

    // Get the compressed grid for NoC
    const auto& compressed_noc_grid = place_ctx.compressed_block_grids[router_block_type->index];

    size_t max_n_cols = std::max_element(compressed_noc_grid.compressed_to_grid_x.begin(), compressed_noc_grid.compressed_to_grid_x.end(),
                                         [](const std::vector<int>& a, const std::vector<int>& b) {
                                             return a.size() < b.size();
                                         })->size();

    size_t max_n_rows = std::max_element(compressed_noc_grid.compressed_to_grid_y.begin(), compressed_noc_grid.compressed_to_grid_y.end(),
                                         [](const std::vector<int>& a, const std::vector<int>& b) {
                                             return a.size() < b.size();
                                         })->size();

    /* For specifying the domain, assume that the longest traffic flow route starts from
     * one corner and terminates at the opposite corner. Assuming minimal routing, such a
     * route traversed around H+W links, where H and W are the number of rows and columns
     * of the mesh grid.
     */
    operations_research::Domain latency_overrun_domain(0, (int)(max_n_rows + max_n_cols));

    // create boolean variables for each traffic flow and link pair
    // create integer variables for traffic flows with constrained latency
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

        // create an integer variable for each latency-constrained traffic flow
        if (traffic_flow.max_traffic_flow_latency < NocTrafficFlows::DEFAULT_MAX_TRAFFIC_FLOW_LATENCY) {
            latency_overrun_vars[traffic_flow_id] = cp_model.NewIntVar(latency_overrun_domain);
        }

        // create (traffic flow, NoC link) pair boolean variables
        for (const auto& noc_link : noc_model.get_noc_links()) {
            const NocLinkId noc_link_id = noc_link.get_link_id();
            flow_link_vars[{traffic_flow_id, noc_link_id}] = cp_model.NewBoolVar();
        }
    }
}

static int comp_max_number_of_traversed_links(NocTrafficFlowId traffic_flow_id) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    const auto& traffic_flow = traffic_flow_storage.get_single_noc_traffic_flow(traffic_flow_id);

    const auto& noc_links = noc_model.get_noc_links();
    const auto& noc_routers = noc_model.get_noc_routers();
    const double noc_link_latency = noc_model.get_noc_link_latency();
    const double noc_router_latency = noc_model.get_noc_router_latency();

    auto router_it = std::find_if(noc_routers.begin(), noc_routers.end(), [noc_router_latency](const NocRouter& r) {
        return (noc_router_latency != r.get_latency());
    });

    if (router_it != noc_routers.end()) {
        VTR_LOG_ERROR(
            "SAT router assumes all NoC routers have the same latency. "
            "NoC router with the user if %d has a different latency (%g) than the NoC-wide router latency (%g).\n",
            router_it->get_router_user_id(), router_it->get_latency());
    }

    auto link_it = std::find_if(noc_links.begin(), noc_links.end(), [noc_link_latency](const NocLink& l) {
       return (noc_link_latency != l.get_latency());
    });

    if (link_it != noc_links.end()) {
        VTR_LOG_ERROR(
            "SAT router assumes all NoC links have the same latency. "
            "NoC link %d has a different latency (%g) than the NoC-wide link latency (%g).\n",
            (size_t)link_it->get_link_id(), link_it->get_latency());
    }

    const double traffic_flow_latency_constraint = traffic_flow.max_traffic_flow_latency;

    VTR_ASSERT(traffic_flow_latency_constraint < NocTrafficFlows::DEFAULT_MAX_TRAFFIC_FLOW_LATENCY);

    int n_max_links = std::floor((traffic_flow_latency_constraint - noc_router_latency) / (noc_link_latency + noc_router_latency));

    return n_max_links;
}

static void constrain_latency_overrun_vars(orsat::CpModelBuilder& cp_model,
                                           t_flow_link_var_map& flow_link_vars,
                                           std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& noc_model = noc_ctx.noc_model;

    for (auto& [traffic_flow_id, latency_overrun_var] : latency_overrun_vars) {
        int n_max_links = comp_max_number_of_traversed_links(traffic_flow_id);
        // get all boolean variables for this traffic flow
        auto link_vars = get_flow_link_vars(flow_link_vars, {traffic_flow_id},
                                            {noc_model.get_noc_links().keys().begin(), noc_model.get_noc_links().keys().end()});

        orsat::LinearExpr latency_overrun_expr;
        // count the number of activated links for this traffic flow
        latency_overrun_expr += orsat::LinearExpr::Sum(link_vars);
        // subtract the maximum number of permissible links from the number of activated links
        latency_overrun_expr -= n_max_links;

        // if latency_overrun_expr is non-positive, the latency constraint is met and
        // latency_overrun_var should be zero. Otherwise, should be equal to latency_overrun_expr
        // This is like pushing latency_overrun_expr through a ReLU function to get latency_overrun_var.
        cp_model.AddMaxEquality(latency_overrun_var, {latency_overrun_expr, 0});
    }
}

static orsat::LinearExpr create_objective(orsat::CpModelBuilder& cp_model,
                                          t_flow_link_var_map& flow_link_vars,
                                          std::map<NocTrafficFlowId , orsat::IntVar>& latency_overrun_vars,
                                          vtr::vector<NocLinkId , orsat::BoolVar>& congested_link_vars,
                                          int bandwidth_resolution,
                                          int latency_overrun_weight,
                                          int congestion_weight,
                                          bool minimize_aggregate_bandwidth) {
    const auto& noc_ctx = g_vpr_ctx.noc();
    const auto& traffic_flow_storage = noc_ctx.noc_traffic_flows_storage;

    // use the current routing solution as a hint for the SAT solver
    // This will help the solver by giving a good starting point and tighter initial lower bound on the objective function
    for (auto traffic_flow_id : traffic_flow_storage.get_all_traffic_flow_id()) {
        for (auto route_link_id : traffic_flow_storage.get_traffic_flow_route(traffic_flow_id)) {
            cp_model.AddHint(flow_link_vars[{traffic_flow_id, route_link_id}], true);
        }
    }

    orsat::LinearExpr latency_overrun_sum;
    for (auto& [traffic_flow_id, latency_overrun_var] : latency_overrun_vars) {
        latency_overrun_sum += latency_overrun_var;
    }
    latency_overrun_sum *= latency_overrun_weight;

    auto rescaled_traffic_flow_bandwidths = quantize_traffic_flow_bandwidths(bandwidth_resolution);
    orsat::LinearExpr agg_bw_expr;
    if (minimize_aggregate_bandwidth) {
        for (auto& [key, var] : flow_link_vars) {
            auto [traffic_flow_id, noc_link_id] = key;
            agg_bw_expr += orsat::LinearExpr::Term(var, rescaled_traffic_flow_bandwidths[traffic_flow_id]);
        }
    } else {
        agg_bw_expr = 0;
    }


    orsat::LinearExpr congested_link_sum = orsat::LinearExpr::Sum(congested_link_vars);
    congested_link_sum *= congestion_weight;

    orsat::LinearExpr objective = latency_overrun_sum + agg_bw_expr + congested_link_sum;
    return objective;
}


vtr::vector<NocTrafficFlowId, std::vector<NocLinkId>> noc_sat_route(bool minimize_aggregate_bandwidth,
                                                                    const t_noc_opts& noc_opts,
                                                                    int seed) {
    vtr::ScopedStartFinishTimer timer("NoC SAT Routing");

    // Used to add variables and constraints to a CP-SAT model
    orsat::CpModelBuilder cp_model;

    /* For each traffic flow and NoC link pair, we create a boolean variable.
     * When a variable associated with traffic flow t and NoC link l is set,
     * it means that t is routed through l.*/
    t_flow_link_var_map flow_link_vars;

    /* A boolean variable is associated with each NoC link to indicate
     * whether it is congested.*/
    vtr::vector<NocLinkId, orsat::BoolVar> link_congested_vars;

    /* Each traffic flow latency constraint is translated to how many NoC links
     * the traffic flow can traverse without violating the constraint.
     * These integer variables specify the number of additional links traversed
     * beyond the maximum allowed number of links.
     */
    std::map<NocTrafficFlowId, orsat::IntVar> latency_overrun_vars;

    create_flow_link_vars(cp_model, flow_link_vars, latency_overrun_vars);

    constrain_latency_overrun_vars(cp_model, flow_link_vars, latency_overrun_vars);

    forbid_illegal_turns(flow_link_vars, cp_model);

    create_congested_link_vars(link_congested_vars, flow_link_vars, cp_model, noc_opts.noc_sat_routing_bandwidth_resolution);

    add_continuity_constraints(flow_link_vars, cp_model);

    auto objective = create_objective(cp_model, flow_link_vars, latency_overrun_vars, link_congested_vars,
                                      noc_opts.noc_sat_routing_bandwidth_resolution,
                                      noc_opts.noc_sat_routing_latency_overrun_weighting,
                                      noc_opts.noc_sat_routing_congestion_weighting,
                                      minimize_aggregate_bandwidth);

    cp_model.Minimize(objective);

    orsat::SatParameters sat_params;
    if (noc_opts.noc_sat_routing_num_workers > 0) {
        sat_params.set_num_workers(noc_opts.noc_sat_routing_num_workers);
    }
    sat_params.set_random_seed(seed);
    sat_params.set_log_search_progress(noc_opts.noc_sat_routing_log_search_progress);

    orsat::Model model;
    model.Add(NewSatParameters(sat_params));

    orsat::CpSolverResponse response = orsat::SolveCpModel(cp_model.Build(), &model);

    if (response.status() == orsat::CpSolverStatus::FEASIBLE ||
        response.status() == orsat::CpSolverStatus::OPTIMAL) {
        auto routes = convert_vars_to_routes(flow_link_vars, response);
        return routes;
    }

    // when no feasible solution was found, return an empty vector
    return {};
}

#endif //ENABLE_NOC_SAT_ROUTING