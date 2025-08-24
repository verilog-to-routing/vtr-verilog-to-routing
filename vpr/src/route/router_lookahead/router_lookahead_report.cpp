/**
 * @file
 * @author  Alex Singer
 * @date    June 2025
 * @brief   Definition of the router lookahead report generation methods.
 */

#include "router_lookahead_report.h"
#include <fstream>
#include <memory>
#include "connection_router_interface.h"
#include "d_ary_heap.h"
#include "globals.h"
#include "router_lookahead.h"
#include "rr_graph_view.h"
#include "serial_connection_router.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vtr_random.h"
#include "vtr_time.h"
#include "vtr_util.h"
#include "vtr_version.h"

/**
 * @brief Helper method for printing a header section into the given output stream.
 */
static void print_header(std::ofstream& os, const std::string& header) {
    os << "*****************************************************************\n";
    os << header << "\n";
    os << "*****************************************************************\n";
}

namespace {

/**
 * @brief Struct to hold overestimation information found while profiling
 *        sample routes. Here, overestimation is defined as the amount the
 *        router lookahead (heuristic_delay) overestimated the actualy delay
 *        of the path (path_delay).
 */
struct t_overestimation_info {
    /// @brief How large was the overestimation.
    float overestimation = 0.0f;
    /// @brief What was the src RRNode for the overestimating path.
    RRNodeId src_node = RRNodeId::INVALID();
    /// @brief What was the target RRNode for the overestimating path.
    RRNodeId target_node = RRNodeId::INVALID();
    /// @brief What was the delay estimated by the router lookahead.
    float heuristic_delay = 0.0f;
    /// @brief What was the actual delay of the path, reported by a router
    ///        performing a Dijkstra search.
    float path_delay = 0.0f;
};

} // namespace

/**
 * @brief Profile the router lookahead for the given sample node by performing
 *        an all-destination Dijkstra search starting at the sample rr_node.
 *        For all targets reached, the estimation reported by the router lookahead
 *        is compared to the actual delays reported by the router.
 *
 *  @param os
 *      The filestream to write status messages to.
 *  @param sample_rr_node
 *      The sample RRNode used as the source of all paths in this trial.
 *  @param sink_rr_nodes
 *      A list of all sink RR nodes in the RR graph. It is much faster to
 *      precompute this list than iterative over all RR nodes every time in this
 *      method.
 *  @param router_lookahead
 *      The router lookahead we are profiling.
 *  @param router
 *      A connection router object setup with the NOP lookahead heuristic which
 *      makes it perform a Dijkstra search.
 *  @param router_opts
 *      The options to pass into the router.
 *  @param max_difference
 *      The current maximum difference between the estimation from the lookhead
 *      and the actual cost of a given path.
 *  @param num_trials
 *      The current trial number this is. This is used for writing status logs.
 *  @param max_overestimation_per_type
 *      Information on the maximum overestimation per RR node type.
 */
static void profile_sample_routes(std::ofstream& os,
                                  RRNodeId sample_rr_node,
                                  const std::vector<RRNodeId>& sink_rr_nodes,
                                  const RouterLookahead* router_lookahead,
                                  SerialConnectionRouter<FourAryHeap>& router,
                                  const t_router_opts& router_opts,
                                  float& max_difference,
                                  size_t num_trials,
                                  vtr::array<e_rr_type, t_overestimation_info, (size_t)e_rr_type::NUM_RR_TYPES>& max_overestimation_per_type) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    RoutingContext& route_ctx = g_vpr_ctx.mutable_routing();

    // Create a bounding box the size of the device.
    t_bb bounding_box;
    bounding_box.xmin = 0;
    bounding_box.xmax = device_ctx.grid.width() + 1;
    bounding_box.ymin = 0;
    bounding_box.ymax = device_ctx.grid.height() + 1;
    bounding_box.layer_min = 0;
    bounding_box.layer_max = device_ctx.grid.get_num_layers() - 1;

    // Set the cost params used during the all-destination dijkstra search,
    // Since the NOP router lookahead is used during this search, the astar
    // and post-target prune facs do not matter; however the bend cost does
    // matter.
    // These connection cost params are also used in the lookahead; where the
    // criticality does, in fact, matter. Here we set the criticality to one to
    // focus only on delay cost.
    // TODO: Should also investigate the effect of changing the criticality on
    //       the overestimation.
    t_conn_cost_params cost_params;
    cost_params.criticality = 1.;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.astar_offset = router_opts.astar_offset;
    cost_params.post_target_prune_fac = router_opts.post_target_prune_fac;
    cost_params.post_target_prune_offset = router_opts.post_target_prune_offset;
    cost_params.bend_cost = router_opts.bend_cost;

    // Get all the path delays from this sample node.
    RouteTree tree(sample_rr_node);
    e_rr_type sample_rr_node_type = rr_graph.node_type(sample_rr_node);
    RouterStats router_stats;
    ConnectionParameters conn_params(ParentNetId::INVALID(), OPEN, false, std::unordered_map<RRNodeId, int>());
    vtr::vector<RRNodeId, RTExploredNode> shortest_paths = router.timing_driven_find_all_shortest_paths_from_route_tree(tree.root(),
                                                                                                                        cost_params,
                                                                                                                        bounding_box,
                                                                                                                        router_stats,
                                                                                                                        conn_params);

    // Get the max difference between the heuristic delay and the actual delay
    // for each possible sink node on the device.
    size_t num_targets = 0;
    size_t num_overestimations = 0;
    float total_overestimation = 0.0f;
    float total_path_cost = 0.0f;
    float total_heur_cost = 0.0f;
    float max_overestimation_this_iter = 0.0f;
    double squared_error_sum = 0.0;
    for (RRNodeId rr_node_id : sink_rr_nodes) {
        VTR_ASSERT_SAFE(rr_graph.node_type(rr_node_id) == e_rr_type::SINK);
        VTR_ASSERT_SAFE(rr_node_id != sample_rr_node);

        // Get the delay of this path. The delay of the path is just equal
        // to the backward path cost at the target node.
        float path_delay = route_ctx.rr_node_route_inf[rr_node_id].backward_path_cost;
        // If this target does not have a path delay (still set to infinity)
        // this means that no paths could be found to this target. Skip.
        if (std::isnan(path_delay) || std::isinf(path_delay))
            continue;

        // Compute the lookahead from the source to this node.
        float heuristic_delay = router_lookahead->get_expected_cost(sample_rr_node,
                                                                    rr_node_id,
                                                                    cost_params,
                                                                    0);
        // If the heuristic estimate of the path cost is higher than the actual
        // path cost, this means that the router lookahead overestimated on this
        // path.
        if (heuristic_delay > path_delay) {
            // Collect information for the average overestimation.
            num_overestimations++;
            float overestimation = heuristic_delay - path_delay;
            total_overestimation += overestimation;
            max_difference = std::max(max_difference, overestimation);
            max_overestimation_this_iter = std::max(max_overestimation_this_iter, overestimation);

            // Keep track of the worst overestimation for this node type.
            t_overestimation_info& overestimation_info = max_overestimation_per_type[sample_rr_node_type];
            if (overestimation > overestimation_info.overestimation) {
                overestimation_info.overestimation = overestimation;
                overestimation_info.src_node = sample_rr_node;
                overestimation_info.target_node = rr_node_id;
                overestimation_info.heuristic_delay = heuristic_delay;
                overestimation_info.path_delay = path_delay;
            }
        }

        // Keep track of the sum of the squared errors.
        double error = heuristic_delay - path_delay;
        squared_error_sum += (error * error);

        // Keep track of the total path and heuristic costs of each path.
        total_path_cost += path_delay;
        total_heur_cost += heuristic_delay;
        num_targets++;
    }

    // Reset the global path costs to prepare for the next route.
    router.reset_path_costs();
    router.clear_modified_rr_node_info();

    // Print the status of this sample trial. This helps debug this profiling
    // method to see what nodes are being tested and what their overestimations
    // look like.
    std::string source_node_type = rr_node_typename[sample_rr_node_type];
    float average_overestimation = 0.0f;
    if (num_overestimations > 0)
        average_overestimation = total_overestimation / (float)num_overestimations;
    float average_path_cost = 0.0f;
    float average_heur_cost = 0.0f;
    double mean_squared_error = 0.0f;
    if (num_targets > 0) {
        average_path_cost = total_path_cost / (float)num_targets;
        average_heur_cost = total_heur_cost / (float)num_targets;
        mean_squared_error = squared_error_sum / (double)num_targets;
    }
    os << vtr::string_fmt("%10zu  %20.3g  %11s  %12zu  %14.3g  %15.3g  %12.3g  %18zu  %14.3g  %14.3g\n",
                          num_trials,
                          max_difference,
                          source_node_type.c_str(),
                          num_targets,
                          average_path_cost,
                          average_heur_cost,
                          mean_squared_error,
                          num_overestimations,
                          average_overestimation,
                          max_overestimation_this_iter);
}

/**
 * @brief This method profiles the given router lookahead by trying several
 *        routes with no lookahead and comparing the "ideal" path costs to the
 *        costs estimated by the router lookahead. This information can be
 *        valuable since over-estimations in the router lookahead cause the
 *        heuristic to be inadmissible which can hurt quality.
 */
static void profile_lookahead_overestimation(std::ofstream& os,
                                             const RouterLookahead* router_lookahead,
                                             const t_router_opts& router_opts) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    RoutingContext& route_ctx = g_vpr_ctx.mutable_routing();

    print_header(os, "Lookeahead Overestimation Report");

    // To make the report file itself self-documenting, adding a preamble section
    // describing what is happening and why.
    os << "\n";
    os << "The following section of the report will profile the router lookahead\n";
    os << "by performing a set of trial routes from different source RR nodes to\n";
    os << "all target nodes. These routes are performed using no heuristic, thus\n";
    os << "they perform a Dijkstra path search.\n";
    os << "\n";
    os << "Each \'trial\' below performs a single-source-all-destination Dijkstra\n";
    os << "search from a random source node to all reachable targets. The routes\n";
    os << "produced by this search are analyzed to see how good the router lookahead\n";
    os << "is at estimating the cost of the paths (the heuristic cost) compared to\n";
    os << "the cost returned by the Dijkstra search (the \'actual\' path cost).\n";
    os << "\n";
    os << "Important metrics in the data below include the Mean Squared Error (MSE)\n";
    os << "between the estimated cost of the path and the actual cost of the path, and\n";
    os << "the worst overestimation of the cost of the path. The MSE measures how\n";
    os << "acccurate the router lookahead is. The more accurate the router lookahead\n";
    os << "is, the faster the router will be while maintaining good quality. The\n";
    os << "max overestimation is a measure of how admissible the router lookahead is\n";
    os << "as a heuristic in the router. The higher this number is, the worse the\n";
    os << "router results may be (in theory).\n";
    os << "\n";
    os << "The routes performed by this profiling use a fixed criticality of 1.0, so\n";
    os << "be aware that these results entirely focus on the delay component of the\n";
    os << "costs of the paths.\n";
    os << "\n";

    // Variables for the profiling.
    //  The target number of random source (sample) nodes to use.
    constexpr size_t target_num_trials = 100;
    //  The maximum number of attempts.
    constexpr size_t max_attempts = 10000;

    // Create a NOP router lookahead to be used during the all-destination dijkstra search.
    t_det_routing_arch temp_det_routing_arch;
    std::unique_ptr<RouterLookahead> temp_router_lookahead = make_router_lookahead(temp_det_routing_arch, e_router_lookahead::NO_OP,
                                                                                   /*write_lookahead=*/"", /*read_lookahead=*/"",
                                                                                   /*segment_inf=*/{},
                                                                                   false /*is_flat*/,
                                                                                   1 /*route_verbosity*/);

    // Create the router to perform the all-destination dijkstra search,
    // TODO: The parallel connection router would be ideal for this use case.
    //       Should use it instead.
    SerialConnectionRouter<FourAryHeap> router(
        device_ctx.grid,
        *temp_router_lookahead,
        rr_graph.rr_nodes(),
        &rr_graph,
        device_ctx.rr_rc_data,
        rr_graph.rr_switch(),
        route_ctx.rr_node_route_inf,
        false /*is_flat*/,
        router_opts.route_verbosity);

    // Collect the sink RR nodes. Only sink RR nodes can be the target of the
    // heuristic, so they are the only ones that we care about.
    std::vector<RRNodeId> sink_rr_nodes;
    sink_rr_nodes.reserve(rr_graph.num_nodes());
    for (RRNodeId rr_node_id : rr_graph.nodes()) {
        if (rr_graph.node_type(rr_node_id) == e_rr_type::SINK)
            sink_rr_nodes.push_back(rr_node_id);
    }

    vtr::array<e_rr_type, t_overestimation_info, (size_t)e_rr_type::NUM_RR_TYPES> max_overestimation_per_type;

    // Print some header information on what each column of the status bar means.
    os << "----------  --------------------  -----------  ------------  --------------  ---------------  ------------  ------------------  --------------  --------------\n";
    os << "Trial Num.  Worst Overestimation  Source Node  Num. Targets  Avg. Path Cost  Avg. Heur. Cost  Mean Squared  Num. Overestimated  Avg.            Max           \n";
    os << "            So Far                Type         Found                                          Error         Paths               Overestimation  Overestimation\n";
    os << "----------  --------------------  -----------  ------------  --------------  ---------------  ------------  ------------------  --------------  --------------\n";

    // Perform random trial routes. These routes are single-source-all-destination
    // Dijkstra's algorithm. After all of these routes are found, the cost of each
    // path is compared with the estimated cost from the sample node to the target
    // and this is used to estimate the worst overestimation of the router lookahead.
    // Since it is infeasible to check every possible route on even a small device,
    // this method randomly tries routes; therefore it will likely not find the
    // absolute worst overestimation.
    size_t num_trials = 0;
    std::set<RRNodeId> tried_nodes;
    float max_difference = 0.0f;
    auto rng = vtr::RandomNumberGenerator(0);
    for (unsigned i = 0; i < max_attempts; i++) {
        // If the number of successful trials is equal to our target number of
        // trials, stop.
        if (num_trials == target_num_trials)
            break;

        // Pick a random RRNode
        // TODO: it may be better to create different groups of nodes and pick
        //       random ones from those groups.
        RRNodeId sample_rr_node(rng.irand(rr_graph.num_nodes()));

        // The sample node cannot be a sink or an ipin since these nodes will
        // always terminate a path, not be on the path while running the connection
        // router.
        if (rr_graph.node_type(sample_rr_node) == e_rr_type::SINK || rr_graph.node_type(sample_rr_node) == e_rr_type::IPIN) {
            continue;
        }

        // If this node has already been tried, do not do it again.
        if (tried_nodes.count(sample_rr_node) != 0) {
            continue;
        }
        // Mark this node as tried.
        tried_nodes.insert(sample_rr_node);

        profile_sample_routes(os,
                              sample_rr_node,
                              sink_rr_nodes,
                              router_lookahead,
                              router,
                              router_opts,
                              max_difference,
                              num_trials,
                              max_overestimation_per_type);

        num_trials++;
    }

    os << "\n";

    // Print a summary of the maximum overestimation.
    os << "=================================================================\n";

    // Print the total maximum difference.
    os << vtr::string_fmt("Worst overestimation between heuristic and actual: %.3g\n", max_difference);

    // Print the overestimation per node type.
    for (size_t l = 0; l < max_overestimation_per_type.size(); l++) {
        os << vtr::string_fmt("\t%6s: %.3g\n",
                              rr_node_typename[(e_rr_type)l],
                              max_overestimation_per_type[(e_rr_type)l].overestimation);
    }
    os << "\n";

    // Print details about the max overestimating path per node type.
    os << "Overestimation Details:\n";
    for (size_t l = 0; l < max_overestimation_per_type.size(); l++) {
        const t_overestimation_info& overestimation_info = max_overestimation_per_type[(e_rr_type)l];
        if (overestimation_info.overestimation == 0.0f)
            continue;
        os << vtr::string_fmt("\t%s:\n", rr_node_typename[(e_rr_type)l]);
        os << vtr::string_fmt("\t\t%zu -> %zu (%s)\n",
                              (size_t)overestimation_info.src_node,
                              (size_t)overestimation_info.target_node,
                              rr_node_typename[device_ctx.rr_graph.node_type(overestimation_info.target_node)]);
        os << vtr::string_fmt("\t\t\tHeuristic Cost: %g\n", overestimation_info.heuristic_delay);
        os << vtr::string_fmt("\t\t\tActual Path Cost: %g\n", overestimation_info.path_delay);
    }

    os << "=================================================================\n";
    os << "\n";
}

void generate_router_lookahead_report(const RouterLookahead* router_lookahead,
                                      const t_router_opts& router_opts) {
    vtr::ScopedStartFinishTimer timer("Generating Router Lookahead Report");

    std::string file_name = "report_router_lookahead.rpt";
    std::ofstream os(file_name);

    // Print some information on the report.
    os << file_name << "\n";
    os << "\n";
    os << "Report generated in VTR Version: " << vtr::VERSION << "\n";
    os << "\n";

    // Profile the router lookahead for overestimation and write the results
    // into the report.
    profile_lookahead_overestimation(os, router_lookahead, router_opts);
}
