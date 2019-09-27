#include <cstdio>
#include <ctime>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"

#include "vpr_utils.h"
#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_export.h"
#include "route_common.h"
#include "route_tree_timing.h"
#include "route_timing.h"
#include "net_delay.h"
#include "stats.h"
#include "echo_files.h"
#include "draw.h"
#include "rr_graph.h"
#include "routing_predictor.h"
#include "VprTimingGraphResolver.h"

// all functions in profiling:: namespace, which are only activated if PROFILE is defined
#include "route_profiling.h"

#include "timing_info.h"
#include "timing_util.h"
#include "route_budgets.h"

#include "router_lookahead_map.h"

#include "tatum/TimingReporter.hpp"

#define CONGESTED_SLOPE_VAL -0.04
//#define ROUTER_DEBUG

enum class RouterCongestionMode {
    NORMAL,
    CONFLICTED
};

class WirelengthInfo {
  public:
    WirelengthInfo(size_t available = 0u, size_t used = 0u)
        : available_wirelength_(available)
        , used_wirelength_(used) {
    }

    size_t available_wirelength() const {
        return available_wirelength_;
    }

    size_t used_wirelength() const {
        return used_wirelength_;
    }

    float used_wirelength_ratio() const {
        if (available_wirelength() > 0) {
            return float(used_wirelength()) / available_wirelength();
        } else {
            VTR_ASSERT(used_wirelength() == 0);
            return 0.;
        }
    }

  private:
    size_t available_wirelength_;
    size_t used_wirelength_;
};

class OveruseInfo {
  public:
    OveruseInfo(size_t total = 0u, size_t overused = 0u, size_t total_overuse_val = 0u, size_t worst_overuse_val = 0u)
        : total_nodes_(total)
        , overused_nodes_(overused)
        , total_overuse_(total_overuse_val)
        , worst_overuse_(worst_overuse_val) {
    }

    size_t total_nodes() const {
        return total_nodes_;
    }

    size_t overused_nodes() const {
        return overused_nodes_;
    }

    float overused_node_ratio() const {
        if (total_nodes() > 0) {
            return float(overused_nodes()) / total_nodes();
        } else {
            VTR_ASSERT(overused_nodes() == 0);
            return 0.;
        }
    }

    size_t total_overuse() const {
        return total_overuse_;
    }

    size_t worst_overuse() const {
        return worst_overuse_;
    }

  private:
    size_t total_nodes_;
    size_t overused_nodes_;
    size_t total_overuse_;
    size_t worst_overuse_;
};

struct RoutingMetrics {
    size_t used_wirelength = 0;

    float sWNS = std::numeric_limits<float>::quiet_NaN();
    float sTNS = std::numeric_limits<float>::quiet_NaN();
    float hWNS = std::numeric_limits<float>::quiet_NaN();
    float hTNS = std::numeric_limits<float>::quiet_NaN();
    tatum::TimingPathInfo critical_path;
};

/*
 * File-scope variables
 */

//Run-time flag to control when router debug information is printed
//Note only enables debug output if compiled with VTR_ENABLE_DEBUG_LOGGING defined
bool f_router_debug = false;

/******************** Subroutines local to route_timing.c ********************/

static bool timing_driven_route_sink(ClusterNetId net_id,
                                     unsigned itarget,
                                     int target_pin,
                                     const t_conn_cost_params cost_params,
                                     float pres_fac,
                                     int high_fanout_threshold,
                                     t_rt_node* rt_root,
                                     t_rt_node** rt_node_of_sink,
                                     const RouterLookahead& router_lookahead,
                                     SpatialRouteTreeLookup& spatial_rt_lookup,
                                     RouterStats& router_stats);

static t_heap* timing_driven_route_connection_from_route_tree_high_fanout(t_rt_node* rt_root,
                                                                          int sink_node,
                                                                          const t_conn_cost_params cost_params,
                                                                          t_bb bounding_box,
                                                                          const RouterLookahead& router_lookahead,
                                                                          const SpatialRouteTreeLookup& spatial_rt_lookup,
                                                                          std::vector<int>& modified_rr_node_inf,
                                                                          RouterStats& router_stats);

static t_heap* timing_driven_route_connection_from_heap(int sink_node,
                                                        const t_conn_cost_params cost_params,
                                                        t_bb bounding_box,
                                                        const RouterLookahead& router_lookahead,
                                                        std::vector<int>& modified_rr_node_inf,
                                                        RouterStats& router_stats);

static std::vector<t_heap> timing_driven_find_all_shortest_paths_from_heap(const t_conn_cost_params cost_params,
                                                                           t_bb bounding_box,
                                                                           std::vector<int>& modified_rr_node_inf,
                                                                           RouterStats& router_stats);

static void timing_driven_expand_cheapest(t_heap* cheapest,
                                          int target_node,
                                          const t_conn_cost_params cost_params,
                                          t_bb bounding_box,
                                          const RouterLookahead& router_lookahead,
                                          std::vector<int>& modified_rr_node_inf,
                                          RouterStats& router_stats);

static t_rt_node* setup_routing_resources(int itry, ClusterNetId net_id, unsigned num_sinks, float pres_fac, int min_incremental_reroute_fanout, CBRR& incremental_rerouting_res, t_rt_node** rt_node_of_sink);

static void add_route_tree_to_heap(t_rt_node* rt_node,
                                   int target_node,
                                   const t_conn_cost_params cost_params,
                                   const RouterLookahead& router_lookahead,
                                   RouterStats& router_stats);

static t_bb add_high_fanout_route_tree_to_heap(t_rt_node* rt_root,
                                               int target_node,
                                               const t_conn_cost_params cost_params,
                                               const RouterLookahead& router_lookahead,
                                               const SpatialRouteTreeLookup& spatial_route_tree_lookup,
                                               t_bb net_bounding_box,
                                               RouterStats& router_stats);

static t_bb adjust_highfanout_bounding_box(t_bb highfanout_bb);

static void add_route_tree_node_to_heap(t_rt_node* rt_node,
                                        int target_node,
                                        const t_conn_cost_params cost_params,
                                        const RouterLookahead& router_lookahead,
                                        RouterStats& router_stats);

static void timing_driven_expand_neighbours(t_heap* current,
                                            const t_conn_cost_params cost_params,
                                            t_bb bounding_box,
                                            const RouterLookahead& router_lookahead,
                                            int target_node,
                                            RouterStats& router_stats);

static void timing_driven_expand_neighbour(t_heap* current,
                                           const int from_node,
                                           const t_edge_size from_edge,
                                           const int to_node,
                                           const t_conn_cost_params cost_params,
                                           const t_bb bounding_box,
                                           const RouterLookahead& router_lookahead,
                                           int target_node,
                                           const t_bb target_bb,
                                           RouterStats& router_stats);

static void timing_driven_add_to_heap(const t_conn_cost_params cost_params,
                                      const RouterLookahead& router_lookahead,
                                      const t_heap* current,
                                      const int from_node,
                                      const int to_node,
                                      const int iconn,
                                      const int target_node,
                                      RouterStats& router_stats);

static void timing_driven_expand_node(const t_conn_cost_params cost_params,
                                      const RouterLookahead& router_lookahead,
                                      t_heap* current,
                                      const int from_node,
                                      const int to_node,
                                      const int iconn,
                                      const int target_node);

static void evaluate_timing_driven_node_costs(t_heap* from,
                                              const t_conn_cost_params cost_params,
                                              const RouterLookahead& router_lookahead,
                                              const int from_node,
                                              const int to_node,
                                              const int iconn,
                                              const int target_node);

static bool timing_driven_check_net_delays(vtr::vector<ClusterNetId, float*>& net_delay);

void reduce_budgets_if_congested(route_budgets& budgeting_inf,
                                 CBRR& connections_inf,
                                 float slope,
                                 int itry);

static bool should_route_net(ClusterNetId net_id, CBRR& connections_inf, bool if_force_reroute);
static bool early_exit_heuristic(const t_router_opts& router_opts, const WirelengthInfo& wirelength_info);

struct more_sinks_than {
    inline bool operator()(const ClusterNetId net_index1, const ClusterNetId net_index2) {
        auto& cluster_ctx = g_vpr_ctx.clustering();
        return cluster_ctx.clb_nlist.net_sinks(net_index1).size() > cluster_ctx.clb_nlist.net_sinks(net_index2).size();
    }
};

static size_t calculate_wirelength_available();
static WirelengthInfo calculate_wirelength_info(size_t available_wirelength);
static OveruseInfo calculate_overuse_info();

static void print_route_status_header();
static void print_route_status(int itry,
                               double elapsed_sec,
                               float pres_fac,
                               int num_bb_updated,
                               const RouterStats& router_stats,
                               const OveruseInfo& overuse_info,
                               const WirelengthInfo& wirelength_info,
                               std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                               float est_success_iteration);

static std::string describe_unrouteable_connection(const int source_node, const int sink_node);

static bool is_high_fanout(int fanout, int fanout_threshold);

static size_t dynamic_update_bounding_boxes(const std::vector<ClusterNetId>& nets, int high_fanout_threshold);
static t_bb calc_current_bb(const t_trace* head);

static bool is_better_quality_routing(const vtr::vector<ClusterNetId, t_traceback>& best_routing,
                                      const RoutingMetrics& best_routing_metrics,
                                      const WirelengthInfo& wirelength_info,
                                      std::shared_ptr<const SetupHoldTimingInfo> timing_info);

static bool early_reconvergence_exit_heuristic(const t_router_opts& router_opts,
                                               int itry_since_last_convergence,
                                               std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                                               const RoutingMetrics& best_routing_metrics);

static void generate_route_timing_reports(const t_router_opts& router_opts,
                                          const t_analysis_opts& analysis_opts,
                                          const SetupTimingInfo& timing_info,
                                          const RoutingDelayCalculator& delay_calc);

static void prune_unused_non_configurable_nets(CBRR& connections_inf);

/************************ Subroutine definitions *****************************/
bool try_timing_driven_route(const t_router_opts& router_opts,
                             const t_analysis_opts& analysis_opts,
                             const std::vector<t_segment_inf>& segment_inf,
                             vtr::vector<ClusterNetId, float*>& net_delay,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<SetupHoldTimingInfo> timing_info,
                             std::shared_ptr<RoutingDelayCalculator> delay_calc,
                             ScreenUpdatePriority first_iteration_priority) {
    /* Timing-driven routing algorithm.  The timing graph (includes slack)   *
     * must have already been allocated, and net_delay must have been allocated. *
     * Returns true if the routing succeeds, false otherwise.                    */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    //Initially, the router runs normally trying to reduce congestion while
    //balancing other metrics (timing, wirelength, run-time etc.)
    RouterCongestionMode router_congestion_mode = RouterCongestionMode::NORMAL;

    //Initialize and properly size the lookups for profiling
    profiling::profiling_initialization(get_max_pins_per_net());

    //sort so net with most sinks is routed first.
    auto sorted_nets = std::vector<ClusterNetId>(cluster_ctx.clb_nlist.nets().begin(), cluster_ctx.clb_nlist.nets().end());
    std::sort(sorted_nets.begin(), sorted_nets.end(), more_sinks_than());

    /*
     * Configure the routing predictor
     */
    RoutingPredictor routing_predictor;
    float abort_iteration_threshold = std::numeric_limits<float>::infinity(); //Default no early abort
    if (router_opts.routing_failure_predictor == SAFE) {
        abort_iteration_threshold = ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_SAFE * router_opts.max_router_iterations;
    } else if (router_opts.routing_failure_predictor == AGGRESSIVE) {
        abort_iteration_threshold = ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_AGGRESSIVE * router_opts.max_router_iterations;
    } else {
        VTR_ASSERT_MSG(router_opts.routing_failure_predictor == OFF, "Unrecognized routing failure predictor setting");
    }

    float high_effort_congestion_mode_iteration_threshold = router_opts.congested_routing_iteration_threshold_frac * router_opts.max_router_iterations;

    /* Set delay of ignored signals to zero. Non-ignored net delays are set by
     * update_net_delays_from_route_tree() inside timing_driven_route_net(),
     * which is only called for non-ignored nets. */
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
                net_delay[net_id][ipin] = 0.;
            }
        }
    }

    CBRR connections_inf{};
    VTR_ASSERT_SAFE(connections_inf.sanity_check_lookup());

    route_budgets budgeting_inf;

    const auto* router_lookahead = get_cached_router_lookahead(
        router_opts.lookahead_type,
        router_opts.write_router_lookahead,
        router_opts.read_router_lookahead,
        segment_inf);

    /*
     * Routing parameters
     */
    float pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */
    int bb_fac = router_opts.bb_factor;

    //When routing conflicts are detected the bounding boxes are scaled
    //by BB_SCALE_FACTOR every BB_SCALE_ITER_COUNT iterations
    constexpr float BB_SCALE_FACTOR = 2;
    constexpr int BB_SCALE_ITER_COUNT = 5;

    size_t available_wirelength = calculate_wirelength_available();

    /*
     * Routing status and metrics
     */
    bool routing_is_successful = false;
    WirelengthInfo wirelength_info;
    OveruseInfo overuse_info;
    tatum::TimingPathInfo critical_path;
    int itry; //Routing iteration number
    int itry_conflicted_mode = 0;

    /*
     * Best result so far
     */
    vtr::vector<ClusterNetId, t_traceback> best_routing;
    t_clb_opins_used best_clb_opins_used_locally;
    RoutingMetrics best_routing_metrics;
    int legal_convergence_count = 0;

    /*
     * On the first routing iteration ignore congestion to get reasonable net
     * delay estimates. Set criticalities to 1 when timing analysis is on to
     * optimize timing, and to 0 when timing analysis is off to optimize routability.
     *
     * Subsequent iterations use the net delays from the previous iteration.
     */
    RouterStats router_stats;
    print_route_status_header();
    timing_driven_route_structs route_structs;
    float prev_iter_cumm_time = 0;
    vtr::Timer iteration_timer;
    int num_net_bounding_boxes_updated = 0;
    int itry_since_last_convergence = -1;
    for (itry = 1; itry <= router_opts.max_router_iterations; ++itry) {
        RouterStats router_iteration_stats;
        std::vector<ClusterNetId> rerouted_nets;

        /* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            route_ctx.net_status[net_id].is_routed = false;
            route_ctx.net_status[net_id].is_fixed = false;
        }

        std::shared_ptr<SetupTimingInfo> route_timing_info;
        if (timing_info) {
            if (itry == 1) {
                //First routing iteration, make all nets critical for a min-delay routing
                route_timing_info = make_constant_timing_info(1.);
            } else {
                //Other iterations user the true criticality
                route_timing_info = timing_info;
            }
        } else {
            //Not timing driven, force criticality to zero for a routability-driven routing
            route_timing_info = make_constant_timing_info(0.);
        }

        if (itry_since_last_convergence >= 0) {
            ++itry_since_last_convergence;
        }

        /*
         * Route each net
         */
        for (auto net_id : sorted_nets) {
            bool was_rerouted = false;
            bool is_routable = try_timing_driven_route_net(net_id,
                                                           itry,
                                                           pres_fac,
                                                           router_opts,
                                                           connections_inf,
                                                           router_iteration_stats,
                                                           route_structs.pin_criticality,
                                                           route_structs.rt_node_of_sink,
                                                           net_delay,
                                                           *router_lookahead,
                                                           netlist_pin_lookup,
                                                           route_timing_info,
                                                           budgeting_inf,
                                                           was_rerouted);

            if (!is_routable) {
                return (false); //Impossible to route
            }

            if (was_rerouted) {
                rerouted_nets.push_back(net_id);
            }
        }

        // Make sure any CLB OPINs used up by subblocks being hooked directly to them are reserved for that purpose
        bool rip_up_local_opins = (itry == 1 ? false : true);
        reserve_locally_used_opins(pres_fac, router_opts.acc_fac, rip_up_local_opins);

        /*
         * Calculate metrics for the current routing
         */
        bool routing_is_feasible = feasible_routing();
        float est_success_iteration = routing_predictor.estimate_success_iteration();

        overuse_info = calculate_overuse_info();
        wirelength_info = calculate_wirelength_info(available_wirelength);
        routing_predictor.add_iteration_overuse(itry, overuse_info.overused_nodes());

        if (timing_info) {
            //Update timing based on the new routing
            //Note that the net delays have already been updated by timing_driven_route_net
            timing_info->update();
            timing_info->set_warn_unconstrained(false); //Don't warn again about unconstrained nodes again during routing

            critical_path = timing_info->least_slack_critical_path();

            VTR_ASSERT_SAFE(timing_driven_check_net_delays(net_delay));

            if (itry == 1) {
                generate_route_timing_reports(router_opts, analysis_opts, *timing_info, *delay_calc);
            }
        }

        float iter_cumm_time = iteration_timer.elapsed_sec();
        float iter_elapsed_time = iter_cumm_time - prev_iter_cumm_time;

        //Output progress
        print_route_status(itry, iter_elapsed_time, pres_fac, num_net_bounding_boxes_updated, router_iteration_stats, overuse_info, wirelength_info, timing_info, est_success_iteration);

        prev_iter_cumm_time = iter_cumm_time;

        //Update graphics
        if (itry == 1) {
            update_screen(first_iteration_priority, "Routing...", ROUTING, timing_info);
        } else {
            update_screen(ScreenUpdatePriority::MINOR, "Routing...", ROUTING, timing_info);
        }

        if (router_opts.save_routing_per_iteration) {
            std::string filename = vtr::string_fmt("iteration_%03d.route", itry);
            print_route(nullptr, filename.c_str());
        }

        //Update router stats (total)
        router_stats.connections_routed += router_iteration_stats.connections_routed;
        router_stats.nets_routed += router_iteration_stats.nets_routed;
        router_stats.heap_pushes += router_iteration_stats.heap_pushes;
        router_stats.heap_pops += router_iteration_stats.heap_pops;

        /*
         * Are we finished?
         */
        if (routing_is_feasible) {
            auto& router_ctx = g_vpr_ctx.routing();

            if (is_better_quality_routing(best_routing, best_routing_metrics, wirelength_info, timing_info)) {
                //Save routing
                best_routing = router_ctx.trace;
                best_clb_opins_used_locally = router_ctx.clb_opins_used_locally;

                routing_is_successful = true;

                //Update best metrics
                if (timing_info) {
                    timing_driven_check_net_delays(net_delay);

                    best_routing_metrics.sTNS = timing_info->setup_total_negative_slack();
                    best_routing_metrics.sWNS = timing_info->setup_worst_negative_slack();
                    best_routing_metrics.hTNS = timing_info->hold_total_negative_slack();
                    best_routing_metrics.hWNS = timing_info->hold_worst_negative_slack();
                    best_routing_metrics.critical_path = critical_path;
                }
                best_routing_metrics.used_wirelength = wirelength_info.used_wirelength();
            }

            //Decrease pres_fac so that criticl connections will take more direct routes
            //Note that we use first_iter_pres_fac here (typically zero), and switch to
            //use initial_pres_fac on the next iteration.
            pres_fac = router_opts.first_iter_pres_fac;

            //Reduce timing tolerances to re-route more delay-suboptimal signals
            connections_inf.set_connection_criticality_tolerance(0.7);
            connections_inf.set_connection_delay_tolerance(1.01);

            ++legal_convergence_count;
            itry_since_last_convergence = 0;

            VTR_ASSERT(routing_is_successful);
        }

        if (itry_since_last_convergence == 1) {
            //We used first_iter_pres_fac when we started routing again
            //after the first routing convergence. Since that is often zero,
            //we want to set pres_fac to a reasonable (i.e. typically non-zero)
            //value afterwards -- so it grows when multiplied by pres_fac_mult
            pres_fac = router_opts.initial_pres_fac;
        }

        //Have we converged the maximum number of times, did not make any changes, or does it seem
        //unlikely additional convergences will improve QoR?
        if (legal_convergence_count >= router_opts.max_convergence_count
            || router_iteration_stats.connections_routed == 0
            || early_reconvergence_exit_heuristic(router_opts, itry_since_last_convergence, timing_info, best_routing_metrics)) {
            break; //Done routing
        }

        /*
         * Abort checks: Should we give-up because this routing problem is unlikely to converge to a legal routing?
         */
        if (itry == 1 && early_exit_heuristic(router_opts, wirelength_info)) {
            //Abort
            break;
        }

        //Estimate at what iteration we will converge to a legal routing
        if (overuse_info.overused_nodes() > ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD) {
            //Only consider aborting if we have a significant number of overused resources

            if (!std::isnan(est_success_iteration) && est_success_iteration > abort_iteration_threshold) {
                VTR_LOG("Routing aborted, the predicted iteration for a successful route (%.1f) is too high.\n", est_success_iteration);
                break; //Abort
            }
        }

        /*
         * Prepare for the next iteration
         */

        if (router_opts.route_bb_update == e_route_bb_update::DYNAMIC) {
            num_net_bounding_boxes_updated = dynamic_update_bounding_boxes(rerouted_nets, router_opts.high_fanout_threshold);
        }

        if (itry >= high_effort_congestion_mode_iteration_threshold) {
            //We are approaching the maximum number of routing iterations,
            //and still do not have a legal routing. Switch to a mode which
            //focuses more on attempting to resolve routing conflicts.
            router_congestion_mode = RouterCongestionMode::CONFLICTED;
        }

        //Update pres_fac and resource costs
        if (itry == 1) {
            pres_fac = router_opts.initial_pres_fac;
            pathfinder_update_cost(pres_fac, 0.); /* Acc_fac=0 for first iter. */
        } else {
            pres_fac *= router_opts.pres_fac_mult;

            /* Avoid overflow for high iteration counts, even if acc_cost is big */
            pres_fac = std::min(pres_fac, static_cast<float>(HUGE_POSITIVE_FLOAT / 1e5));

            pathfinder_update_cost(pres_fac, router_opts.acc_fac);
        }

        if (router_congestion_mode == RouterCongestionMode::CONFLICTED) {
            //The design appears to have routing conflicts which are difficult to resolve:
            //  1) Don't re-route legal connections due to delay. This allows
            //     the router to focus on the actual conflicts
            //  2) Increase the net bounding boxes. This potentially allows
            //     the router to route around otherwise congested regions
            //     (at the cost of high run-time).

            //Increase the size of the net bounding boxes to give the router more
            //freedom to find alternate paths.
            //
            //In the case of routing conflicts there are multiple connections competing
            //for the same resources which can not resolve the congestion themselves.
            //In normal routing mode we try to keep the bounding boxes small to minimize
            //run-time, but this can limits how far signals can detour (i.e. they can't
            //route outside the bounding box), which can cause conflicts to oscillate back
            //and forth without resolving.
            //
            //By scaling the bounding boxes here, we slowly increase the router's search
            //space in hopes of it allowing signals to move further out of the way to
            //aleviate the conflicts.
            if (itry_conflicted_mode % BB_SCALE_ITER_COUNT == 0) {
                //We scale the bounding boxes by BB_SCALE_FACTOR,
                //every BB_SCALE_ITER_COUNT iterations. This ensures
                //that we give the router some time (BB_SCALE_ITER_COUNT) to try
                //resolve/negotiate congestion at the new BB factor.
                //
                //Note that we increase the BB factor slowly to try and minimize
                //the bounding box size (since larger bounding boxes slow the router down).
                auto& grid = g_vpr_ctx.device().grid;
                int max_grid_dim = std::max(grid.width(), grid.height());

                //Scale by BB_SCALE_FACTOR but clip to grid size to avoid overflow
                bb_fac = std::min<int>(max_grid_dim, bb_fac * BB_SCALE_FACTOR);

                route_ctx.route_bb = load_route_bb(bb_fac);
            }

            ++itry_conflicted_mode;
        }

        if (timing_info) {
            if (itry == 1) {
                // first iteration sets up the lower bound connection delays since only timing is optimized for
                connections_inf.set_stable_critical_path_delay(critical_path.delay());
                connections_inf.set_lower_bound_connection_delays(net_delay);

                //load budgets using information from uncongested delay information
                budgeting_inf.load_route_budgets(net_delay, timing_info, netlist_pin_lookup, router_opts);
                /*for debugging purposes*/
                //                if (budgeting_inf.if_set()) {
                //                    budgeting_inf.print_route_budget();
                //                }

            } else {
                bool stable_routing_configuration = true;

                /*
                 * Determine if any connection need to be forcibly re-routed due to timing
                 */

                //Yes, if explicitly enabled
                bool should_ripup_for_delay = (router_opts.incr_reroute_delay_ripup == e_incr_reroute_delay_ripup::ON);

                //Or, if things are not too congested
                should_ripup_for_delay |= (router_opts.incr_reroute_delay_ripup == e_incr_reroute_delay_ripup::AUTO
                                           && router_congestion_mode == RouterCongestionMode::NORMAL);

                if (should_ripup_for_delay) {
                    if (connections_inf.critical_path_delay_grew_significantly(critical_path.delay())) {
                        // only need to forcibly reroute if critical path grew significantly
                        stable_routing_configuration = connections_inf.forcibly_reroute_connections(router_opts.max_criticality,
                                                                                                    timing_info,
                                                                                                    netlist_pin_lookup,
                                                                                                    net_delay);
                    }
                }

                // not stable if any connection needs to be forcibly rerouted
                if (stable_routing_configuration) {
                    connections_inf.set_stable_critical_path_delay(critical_path.delay());
                }

                /*Check if rate of convergence is high enough, if not lower the budgets of certain nets*/
                //reduce_budgets_if_congested(budgeting_inf, connections_inf, routing_predictor.get_slope(), itry);
            }
        } else {
            /* If timing analysis is not enabled, make sure that the criticalities and the
             * net_delays stay as 0 so that wirelength can be optimized. */

            for (auto net_id : cluster_ctx.clb_nlist.nets()) {
                for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
                    net_delay[net_id][ipin] = 0.;
                }
            }
        }

        if (router_opts.congestion_analysis) profiling::congestion_analysis();
        if (router_opts.fanout_analysis) profiling::time_on_fanout_analysis();
        // profiling::time_on_criticality_analysis();
    }

    if (routing_is_successful) {
        VTR_LOG("Restoring best routing\n");

        auto& router_ctx = g_vpr_ctx.mutable_routing();

        /* Restore congestion from best route */
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            pathfinder_update_path_cost(route_ctx.trace[net_id].head, -1, pres_fac);
            pathfinder_update_path_cost(best_routing[net_id].head, 1, pres_fac);
        }
        router_ctx.trace = best_routing;
        router_ctx.clb_opins_used_locally = best_clb_opins_used_locally;

        prune_unused_non_configurable_nets(connections_inf);

        if (timing_info) {
            VTR_LOG("Critical path: %g ns\n", 1e9 * best_routing_metrics.critical_path.delay());
        }

        VTR_LOG("Successfully routed after %d routing iterations.\n", itry);
    } else {
        VTR_LOG("Routing failed.\n");
#ifdef VTR_ENABLE_DEBUG_LOGGING
        if (f_router_debug) print_invalid_routing_info();
#endif
    }

    VTR_LOG("Router Stats: total_nets_routed: %zu total_connections_routed: %zu total_heap_pushes: %zu total_heap_pops: %zu\n",
            router_stats.nets_routed, router_stats.connections_routed, router_stats.heap_pushes, router_stats.heap_pops);

    return routing_is_successful;
}

bool try_timing_driven_route_net(ClusterNetId net_id,
                                 int itry,
                                 float pres_fac,
                                 const t_router_opts& router_opts,
                                 CBRR& connections_inf,
                                 RouterStats& router_stats,
                                 float* pin_criticality,
                                 t_rt_node** rt_node_of_sink,
                                 vtr::vector<ClusterNetId, float*>& net_delay,
                                 const RouterLookahead& router_lookahead,
                                 const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                 std::shared_ptr<SetupTimingInfo> timing_info,
                                 route_budgets& budgeting_inf,
                                 bool& was_rerouted) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    bool is_routed = false;

    connections_inf.prepare_routing_for_net(net_id);

    if (route_ctx.net_status[net_id].is_fixed) { /* Skip pre-routed nets. */
        is_routed = true;
    } else if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) { /* Skip ignored nets. */
        is_routed = true;
    } else if (should_route_net(net_id, connections_inf, true) == false) {
        is_routed = true;
    } else {
        // track time spent vs fanout
        profiling::net_fanout_start();

        is_routed = timing_driven_route_net(net_id,
                                            itry,
                                            pres_fac,
                                            router_opts,
                                            connections_inf,
                                            router_stats,
                                            pin_criticality,
                                            rt_node_of_sink,
                                            net_delay[net_id],
                                            router_lookahead,
                                            netlist_pin_lookup,
                                            timing_info,
                                            budgeting_inf);

        profiling::net_fanout_end(cluster_ctx.clb_nlist.net_sinks(net_id).size());

        /* Impossible to route? (disconnected rr_graph) */
        if (is_routed) {
            route_ctx.net_status[net_id].is_routed = true;
        } else {
            VTR_LOG("Routing failed.\n");
        }

        was_rerouted = true; //Flag to record whether routing was actually changed
    }
    return (is_routed);
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void alloc_timing_driven_route_structs(float** pin_criticality_ptr,
                                       int** sink_order_ptr,
                                       t_rt_node*** rt_node_of_sink_ptr) {
    /* Allocates all the structures needed only by the timing-driven router.   */

    int max_pins_per_net = get_max_pins_per_net();
    int max_sinks = std::max(max_pins_per_net - 1, 0);

    float* pin_criticality = (float*)vtr::malloc(max_sinks * sizeof(float));
    *pin_criticality_ptr = pin_criticality - 1; /* First sink is pin #1. */

    int* sink_order = (int*)vtr::malloc(max_sinks * sizeof(int));
    *sink_order_ptr = sink_order - 1;

    t_rt_node** rt_node_of_sink = (t_rt_node**)vtr::malloc(max_sinks * sizeof(t_rt_node*));
    *rt_node_of_sink_ptr = rt_node_of_sink - 1;

    alloc_route_tree_timing_structs();
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void free_timing_driven_route_structs(float* pin_criticality, int* sink_order, t_rt_node** rt_node_of_sink) {
    /* Frees all the stuctures needed only by the timing-driven router.        */

    // coverity[offset_free : Intentional]
    free(pin_criticality + 1); /* Starts at index 1. */
    // coverity[offset_free : Intentional]
    free(sink_order + 1);
    // coverity[offset_free : Intentional]
    free(rt_node_of_sink + 1);

    free_route_tree_timing_structs();
}

timing_driven_route_structs::timing_driven_route_structs() {
    alloc_timing_driven_route_structs(&pin_criticality,
                                      &sink_order,
                                      &rt_node_of_sink);
}

timing_driven_route_structs::~timing_driven_route_structs() {
    free_timing_driven_route_structs(pin_criticality,
                                     sink_order,
                                     rt_node_of_sink);
}

void reduce_budgets_if_congested(route_budgets& budgeting_inf,
                                 CBRR& connections_inf,
                                 float slope,
                                 int itry) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    if (budgeting_inf.if_set()) {
        for (auto net_id : cluster_ctx.clb_nlist.nets()) {
            if (should_route_net(net_id, connections_inf, false)) {
                budgeting_inf.update_congestion_times(net_id);
            } else {
                budgeting_inf.not_congested_this_iteration(net_id);
            }
        }

        /*Problematic if the overuse nodes are positive or declining at a slow rate
         * Must be more than 9 iterations to have a valid slope*/
        if (slope > CONGESTED_SLOPE_VAL && itry >= 9) {
            budgeting_inf.lower_budgets(1e-9);
        }
    }
}

int get_max_pins_per_net() {
    int max_pins_per_net = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id))
            max_pins_per_net = std::max(max_pins_per_net, (int)cluster_ctx.clb_nlist.net_pins(net_id).size());
    }

    return (max_pins_per_net);
}

struct Criticality_comp {
    const float* criticality;

    Criticality_comp(const float* calculated_criticalities)
        : criticality{calculated_criticalities} {
    }

    bool operator()(int a, int b) const {
        return criticality[a] > criticality[b];
    }
};

bool timing_driven_route_net(ClusterNetId net_id,
                             int itry,
                             float pres_fac,
                             const t_router_opts& router_opts,
                             CBRR& connections_inf,
                             RouterStats& router_stats,
                             float* pin_criticality,
                             t_rt_node** rt_node_of_sink,
                             float* net_delay,
                             const RouterLookahead& router_lookahead,
                             const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                             std::shared_ptr<const SetupTimingInfo> timing_info,
                             route_budgets& budgeting_inf) {
    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    unsigned int num_sinks = cluster_ctx.clb_nlist.net_sinks(net_id).size();

    VTR_LOGV_DEBUG(f_router_debug, "Routing Net %zu (%zu sinks)\n", size_t(net_id), num_sinks);

    t_rt_node* rt_root = setup_routing_resources(itry, net_id, num_sinks, pres_fac, router_opts.min_incremental_reroute_fanout, connections_inf, rt_node_of_sink);

    bool high_fanout = is_high_fanout(num_sinks, router_opts.high_fanout_threshold);

    SpatialRouteTreeLookup spatial_route_tree_lookup;
    if (high_fanout) {
        spatial_route_tree_lookup = build_route_tree_spatial_lookup(net_id, rt_root);
    }

    // after this point the route tree is correct
    // remaining_targets from this point on are the **pin indices** that have yet to be routed
    auto& remaining_targets = connections_inf.get_remaining_targets();

    // calculate criticality of remaining target pins
    for (int ipin : remaining_targets) {
        if (timing_info) {
            auto clb_pin = cluster_ctx.clb_nlist.net_pin(net_id, ipin);
            pin_criticality[ipin] = calculate_clb_net_pin_criticality(*timing_info, netlist_pin_lookup, clb_pin);

            /* Pin criticality is between 0 and 1.
             * Shift it downwards by 1 - max_criticality (max_criticality is 0.99 by default,
             * so shift down by 0.01) and cut off at 0.  This means that all pins with small
             * criticalities (<0.01) get criticality 0 and are ignored entirely, and everything
             * else becomes a bit less critical. This effect becomes more pronounced if
             * max_criticality is set lower. */
            // VTR_ASSERT(pin_criticality[ipin] > -0.01 && pin_criticality[ipin] < 1.01);
            pin_criticality[ipin] = std::max(pin_criticality[ipin] - (1.0 - router_opts.max_criticality), 0.0);

            /* Take pin criticality to some power (1 by default). */
            pin_criticality[ipin] = std::pow(pin_criticality[ipin], router_opts.criticality_exp);

            /* Cut off pin criticality at max_criticality. */
            pin_criticality[ipin] = std::min(pin_criticality[ipin], router_opts.max_criticality);
        } else {
            //No timing info, implies we want a min delay routing, so use criticality of 1.
            pin_criticality[ipin] = 1.;
        }
    }

    // compare the criticality of different sink nodes
    sort(begin(remaining_targets), end(remaining_targets), Criticality_comp{pin_criticality});

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(num_sinks);

    t_conn_delay_budget conn_delay_budget;
    t_conn_cost_params cost_params;
    cost_params.astar_fac = router_opts.astar_fac;
    cost_params.bend_cost = router_opts.bend_cost;
    cost_params.delay_budget = ((budgeting_inf.if_set()) ? &conn_delay_budget : nullptr);

    // explore in order of decreasing criticality (no longer need sink_order array)
    for (unsigned itarget = 0; itarget < remaining_targets.size(); ++itarget) {
        int target_pin = remaining_targets[itarget];

        int sink_rr = route_ctx.net_rr_terminals[net_id][target_pin];

        enable_router_debug(router_opts, net_id, sink_rr);

        cost_params.criticality = pin_criticality[target_pin];

        if (budgeting_inf.if_set()) {
            conn_delay_budget.max_delay = budgeting_inf.get_max_delay_budget(net_id, target_pin);
            conn_delay_budget.target_delay = budgeting_inf.get_delay_target(net_id, target_pin);
            conn_delay_budget.min_delay = budgeting_inf.get_min_delay_budget(net_id, target_pin);
            conn_delay_budget.short_path_criticality = budgeting_inf.get_crit_short_path(net_id, target_pin);
        }

        // build a branch in the route tree to the target
        if (!timing_driven_route_sink(net_id,
                                      itarget,
                                      target_pin,
                                      cost_params,
                                      pres_fac,
                                      router_opts.high_fanout_threshold,
                                      rt_root, rt_node_of_sink,
                                      router_lookahead,
                                      spatial_route_tree_lookup,
                                      router_stats))
            return false;

        ++router_stats.connections_routed;
    } // finished all sinks

    ++router_stats.nets_routed;

    /* For later timing analysis. */

    // may have to update timing delay of the previously legally reached sinks since downstream capacitance could be changed
    update_net_delays_from_route_tree(net_delay, rt_node_of_sink, net_id);

    if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            if (net_delay[ipin] == 0) { // should be SOURCE->OPIN->IPIN->SINK
                VTR_ASSERT(device_ctx.rr_nodes[rt_node_of_sink[ipin]->parent_node->parent_node->inode].type() == OPIN);
            }
        }
    }

    VTR_ASSERT_MSG(route_ctx.rr_node_route_inf[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");

    // route tree is not kept persistent since building it from the traceback the next iteration takes almost 0 time
    VTR_LOGV_DEBUG(f_router_debug, "Routed Net %zu (%zu sinks)\n", size_t(net_id), num_sinks);
    free_route_tree(rt_root);
    return (true);
}

static bool timing_driven_route_sink(ClusterNetId net_id,
                                     unsigned itarget,
                                     int target_pin,
                                     const t_conn_cost_params cost_params,
                                     float pres_fac,
                                     int high_fanout_threshold,
                                     t_rt_node* rt_root,
                                     t_rt_node** rt_node_of_sink,
                                     const RouterLookahead& router_lookahead,
                                     SpatialRouteTreeLookup& spatial_rt_lookup,
                                     RouterStats& router_stats) {
    /* Build a path from the existing route tree rooted at rt_root to the target_node
     * add this branch to the existing route tree and update pathfinder costs and rr_node_route_inf to reflect this */
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    profiling::sink_criticality_start();

    int sink_node = route_ctx.net_rr_terminals[net_id][target_pin];

    VTR_LOGV_DEBUG(f_router_debug, "Net %zu Target %d (%s)\n", size_t(net_id), itarget, describe_rr_node(sink_node).c_str());

    VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace[net_id].head, rt_root));

    std::vector<int> modified_rr_node_inf;
    t_heap* cheapest = nullptr;
    t_bb bounding_box = route_ctx.route_bb[net_id];

    bool net_is_global = cluster_ctx.clb_nlist.net_is_global(net_id);
    bool high_fanout = is_high_fanout(cluster_ctx.clb_nlist.net_sinks(net_id).size(), high_fanout_threshold);
    constexpr float HIGH_FANOUT_CRITICALITY_THRESHOLD = 0.9;
    bool sink_critical = (cost_params.criticality > HIGH_FANOUT_CRITICALITY_THRESHOLD);

    //We normally route high fanout nets by only adding spatially close-by routing to the heap (reduces run-time).
    //However, if the current sink is 'critical' from a timing perspective, we put the entire route tree back onto
    //the heap to ensure it has more flexibility to find the best path.
    if (high_fanout && !sink_critical && !net_is_global) {
        cheapest = timing_driven_route_connection_from_route_tree_high_fanout(rt_root,
                                                                              sink_node,
                                                                              cost_params,
                                                                              bounding_box,
                                                                              router_lookahead,
                                                                              spatial_rt_lookup,
                                                                              modified_rr_node_inf,
                                                                              router_stats);
    } else {
        cheapest = timing_driven_route_connection_from_route_tree(rt_root,
                                                                  sink_node,
                                                                  cost_params,
                                                                  bounding_box,
                                                                  router_lookahead,
                                                                  modified_rr_node_inf,
                                                                  router_stats);
    }

    if (cheapest == nullptr) {
        ClusterBlockId src_block = cluster_ctx.clb_nlist.net_driver_block(net_id);
        ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(*(cluster_ctx.clb_nlist.net_pins(net_id).begin() + target_pin));
        VTR_LOG("Failed to route connection from '%s' to '%s' for net '%s' (#%zu)\n",
                cluster_ctx.clb_nlist.block_name(src_block).c_str(),
                cluster_ctx.clb_nlist.block_name(sink_block).c_str(),
                cluster_ctx.clb_nlist.net_name(net_id).c_str(),
                size_t(net_id));
        if (f_router_debug) {
            update_screen(ScreenUpdatePriority::MAJOR, "Unable to route connection.", ROUTING, nullptr);
        }
        return false;
    } else {
        //Record final link to target
        add_to_mod_list(cheapest->index, modified_rr_node_inf);

        route_ctx.rr_node_route_inf[cheapest->index].prev_node = cheapest->u.prev.node;
        route_ctx.rr_node_route_inf[cheapest->index].prev_edge = cheapest->u.prev.edge;
        route_ctx.rr_node_route_inf[cheapest->index].path_cost = cheapest->cost;
        route_ctx.rr_node_route_inf[cheapest->index].backward_path_cost = cheapest->backward_path_cost;
    }

    profiling::sink_criticality_end(cost_params.criticality);

    /* NB:  In the code below I keep two records of the partial routing:  the   *
     * traceback and the route_tree.  The route_tree enables fast recomputation *
     * of the Elmore delay to each node in the partial routing.  The traceback  *
     * lets me reuse all the routines written for breadth-first routing, which  *
     * all take a traceback structure as input.                                 */

    int inode = cheapest->index;
    route_ctx.rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
    t_trace* new_route_start_tptr = update_traceback(cheapest, net_id);
    VTR_ASSERT_DEBUG(validate_traceback(route_ctx.trace[net_id].head));

    rt_node_of_sink[target_pin] = update_route_tree(cheapest, ((high_fanout) ? &spatial_rt_lookup : nullptr));
    VTR_ASSERT_DEBUG(verify_route_tree(rt_root));
    VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace[net_id].head, rt_root));
    VTR_ASSERT_DEBUG(!high_fanout || validate_route_tree_spatial_lookup(rt_root, spatial_rt_lookup));
    if (f_router_debug) {
        update_screen(ScreenUpdatePriority::MAJOR, "Routed connection successfully", ROUTING, nullptr);
    }
    free_heap_data(cheapest);
    pathfinder_update_path_cost(new_route_start_tptr, 1, pres_fac);
    empty_heap();

    // need to guarentee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
    // do this by resetting all the path_costs that have been touched while routing to the current sink
    reset_path_costs(modified_rr_node_inf);

    // routed to a sink successfully
    return true;
}

//Finds a path from the route tree rooted at rt_root to sink_node
//
//This is used when you want to allow previous routing of the same net to serve
//as valid start locations for the current connection.
//
//Returns either the last element of the path, or nullptr if no path is found
t_heap* timing_driven_route_connection_from_route_tree(t_rt_node* rt_root,
                                                       int sink_node,
                                                       const t_conn_cost_params cost_params,
                                                       t_bb bounding_box,
                                                       const RouterLookahead& router_lookahead,
                                                       std::vector<int>& modified_rr_node_inf,
                                                       RouterStats& router_stats) {
    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    add_route_tree_to_heap(rt_root, sink_node, cost_params, router_lookahead, router_stats);
    heap_::build_heap(); // via sifting down everything

    int source_node = rt_root->inode;

    if (is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

        free_route_tree(rt_root);
        return nullptr;
    }

    VTR_LOGV_DEBUG(f_router_debug, "  Routing to %d as normal net (BB: %d,%d x %d,%d)\n", sink_node,
                   bounding_box.xmin, bounding_box.ymin,
                   bounding_box.xmax, bounding_box.ymax);

    t_heap* cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                                cost_params,
                                                                bounding_box,
                                                                router_lookahead,
                                                                modified_rr_node_inf,
                                                                router_stats);

    if (cheapest == nullptr) {
        //Found no path found within the current bounding box.
        //Try again with no bounding box (i.e. a full device grid bounding box).
        //
        //Note that the additional run-time overhead of re-trying only occurs
        //when we were otherwise going to give up -- the typical case (route
        //found with the bounding box) remains fast and never re-tries .
        VTR_LOG_WARN("No routing path for connection to sink_rr %d, retrying with full device bounding box\n", sink_node);

        auto& device_ctx = g_vpr_ctx.device();

        t_bb full_device_bounding_box;
        full_device_bounding_box.xmin = 0;
        full_device_bounding_box.ymin = 0;
        full_device_bounding_box.xmax = device_ctx.grid.width() - 1;
        full_device_bounding_box.ymax = device_ctx.grid.height() - 1;

        //
        //TODO: potential future optimization
        //      We have already explored the RR nodes accessible within the regular
        //      BB (which are stored in modified_rr_node_inf), and so already know
        //      their cost from the source. Instead of re-starting the path search
        //      from scratch (i.e. from the previous route tree as we do below), we
        //      could just re-add all the explored nodes to the heap and continue
        //      expanding.
        //

        //Reset any previously recorded node costs so that when we call
        //add_route_tree_to_heap() the nodes in the route tree actually
        //make it back into the heap.
        reset_path_costs(modified_rr_node_inf);
        modified_rr_node_inf.clear();

        //Re-initialize the heap since it was emptied by the previous call to
        //timing_driven_route_connection_from_heap()
        add_route_tree_to_heap(rt_root, sink_node, cost_params, router_lookahead, router_stats);

        //Try finding the path again with the relaxed bounding box
        cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                            cost_params,
                                                            full_device_bounding_box,
                                                            router_lookahead,
                                                            modified_rr_node_inf,
                                                            router_stats);
    }

    if (cheapest == nullptr) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

        free_route_tree(rt_root);
        return nullptr;
    }

    return cheapest;
}

//Finds a path from the route tree rooted at rt_root to sink_node for a high fanout net.
//
//Unlike timing_driven_route_connection_from_route_tree(), only part of the route tree
//which is spatially close to the sink is added to the heap.
static t_heap* timing_driven_route_connection_from_route_tree_high_fanout(t_rt_node* rt_root,
                                                                          int sink_node,
                                                                          const t_conn_cost_params cost_params,
                                                                          t_bb net_bounding_box,
                                                                          const RouterLookahead& router_lookahead,
                                                                          const SpatialRouteTreeLookup& spatial_rt_lookup,
                                                                          std::vector<int>& modified_rr_node_inf,
                                                                          RouterStats& router_stats) {
    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    t_bb high_fanout_bb = add_high_fanout_route_tree_to_heap(rt_root, sink_node, cost_params, router_lookahead, spatial_rt_lookup, net_bounding_box, router_stats);
    heap_::build_heap(); // via sifting down everything

    int source_node = rt_root->inode;

    if (is_empty_heap()) {
        VTR_LOG("No source in route tree: %s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

        free_route_tree(rt_root);
        return nullptr;
    }

    VTR_LOGV_DEBUG(f_router_debug, "  Routing to %d as high fanout net (BB: %d,%d x %d,%d)\n", sink_node,
                   high_fanout_bb.xmin, high_fanout_bb.ymin,
                   high_fanout_bb.xmax, high_fanout_bb.ymax);

    t_heap* cheapest = timing_driven_route_connection_from_heap(sink_node,
                                                                cost_params,
                                                                high_fanout_bb,
                                                                router_lookahead,
                                                                modified_rr_node_inf,
                                                                router_stats);

    if (cheapest == nullptr) {
        //Found no path, that may be due to an unlucky choice of existing route tree sub-set,
        //try again with the full route tree to be sure this is not an artifact of high-fanout routing
        VTR_LOG_WARN("No routing path found in high-fanout mode for net connection (to sink_rr %d), retrying with full route tree\n", sink_node);

        //Reset any previously recorded node costs so timing_driven_route_connection()
        //starts over from scratch.
        reset_path_costs(modified_rr_node_inf);
        modified_rr_node_inf.clear();

        cheapest = timing_driven_route_connection_from_route_tree(rt_root,
                                                                  sink_node,
                                                                  cost_params,
                                                                  net_bounding_box,
                                                                  router_lookahead,
                                                                  modified_rr_node_inf,
                                                                  router_stats);
    }

    if (cheapest == nullptr) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

        free_route_tree(rt_root);
        return nullptr;
    }

    return cheapest;
}

//Finds a path to sink_node, starting from the elements currently in the heap.
//
//This is the core maze routing routine.
//
//Returns either the last element of the path, or nullptr if no path is found
static t_heap* timing_driven_route_connection_from_heap(int sink_node,
                                                        const t_conn_cost_params cost_params,
                                                        t_bb bounding_box,
                                                        const RouterLookahead& router_lookahead,
                                                        std::vector<int>& modified_rr_node_inf,
                                                        RouterStats& router_stats) {
    VTR_ASSERT_SAFE(heap_::is_valid());

    if (is_empty_heap()) { //No source
        VTR_LOGV_DEBUG(f_router_debug, "  Initial heap empty (no source)\n");
    }

    t_heap* cheapest = nullptr;
    while (!is_empty_heap()) {
        // cheapest t_heap in current route tree to be expanded on
        cheapest = get_heap_head();
        ++router_stats.heap_pops;

        int inode = cheapest->index;
        VTR_LOGV_DEBUG(f_router_debug, "  Popping node %d (cost: %g)\n",
                       inode, cheapest->cost);

        //Have we found the target?
        if (inode == sink_node) {
            VTR_LOGV_DEBUG(f_router_debug, "  Found target %8d (%s)\n", inode, describe_rr_node(inode).c_str());
            break;
        }

        //If not, keep searching
        timing_driven_expand_cheapest(cheapest,
                                      sink_node,
                                      cost_params,
                                      bounding_box,
                                      router_lookahead,
                                      modified_rr_node_inf,
                                      router_stats);

        free_heap_data(cheapest);
        cheapest = nullptr;
    }

    if (cheapest == nullptr) { /* Impossible routing.  No path for net. */
        VTR_LOGV_DEBUG(f_router_debug, "  Empty heap (no path found)\n");
        return nullptr;
    }

    return cheapest;
}

//Find shortest paths from specified route tree to all nodes in the RR graph
std::vector<t_heap> timing_driven_find_all_shortest_paths_from_route_tree(t_rt_node* rt_root,
                                                                          const t_conn_cost_params cost_params,
                                                                          t_bb bounding_box,
                                                                          std::vector<int>& modified_rr_node_inf,
                                                                          RouterStats& router_stats) {
    //Add the route tree to the heap with no specific target node
    int target_node = OPEN;
    auto router_lookahead = make_router_lookahead(e_router_lookahead::NO_OP,
                                                  /*write_lookahead=*/"", /*read_lookahead=*/"",
                                                  /*segment_inf=*/{});
    add_route_tree_to_heap(rt_root, target_node, cost_params, *router_lookahead, router_stats);
    heap_::build_heap(); // via sifting down everything

    auto res = timing_driven_find_all_shortest_paths_from_heap(cost_params, bounding_box, modified_rr_node_inf, router_stats);

    return res;
}

//Find shortest paths from current heap to all nodes in the RR graph
//
//Since there is no single *target* node this uses Dijkstra's algorithm
//with a modified exit condition (runs until heap is empty).
//
//Note that to re-use code used for the regular A*-based router we use a
//no-operation lookahead which always returns zero.
static std::vector<t_heap> timing_driven_find_all_shortest_paths_from_heap(const t_conn_cost_params cost_params,
                                                                           t_bb bounding_box,
                                                                           std::vector<int>& modified_rr_node_inf,
                                                                           RouterStats& router_stats) {
    auto router_lookahead = make_router_lookahead(e_router_lookahead::NO_OP,
                                                  /*write_lookahead=*/"", /*read_lookahead=*/"",
                                                  /*segment_inf=*/{});

    auto& device_ctx = g_vpr_ctx.device();
    std::vector<t_heap> cheapest_paths(device_ctx.rr_nodes.size());

    VTR_ASSERT_SAFE(heap_::is_valid());

    if (is_empty_heap()) { //No source
        VTR_LOGV_DEBUG(f_router_debug, "  Initial heap empty (no source)\n");
    }

    while (!is_empty_heap()) {
        // cheapest t_heap in current route tree to be expanded on
        t_heap* cheapest = get_heap_head();
        ++router_stats.heap_pops;

        int inode = cheapest->index;
        VTR_LOGV_DEBUG(f_router_debug, "  Popping node %d (cost: %g)\n",
                       inode, cheapest->cost);

        //Since we want to find shortest paths to all nodes in the graph
        //we do not specify a target node.
        //
        //By setting the target_node to OPEN in combination with the NoOp router
        //lookahead we can re-use the node exploration code from the regular router
        int target_node = OPEN;

        timing_driven_expand_cheapest(cheapest,
                                      target_node,
                                      cost_params,
                                      bounding_box,
                                      *router_lookahead,
                                      modified_rr_node_inf,
                                      router_stats);

        if (cheapest_paths[inode].index == OPEN || cheapest_paths[inode].cost >= cheapest->cost) {
            VTR_LOGV_DEBUG(f_router_debug, "  Better cost to node %d: %g (was %g)\n", inode, cheapest->cost, cheapest_paths[inode].cost);
            cheapest_paths[inode] = *cheapest;
        } else {
            VTR_LOGV_DEBUG(f_router_debug, "  Worse cost to node %d: %g (better %g)\n", inode, cheapest->cost, cheapest_paths[inode].cost);
        }

        free_heap_data(cheapest);
    }

    return cheapest_paths;
}

static void timing_driven_expand_cheapest(t_heap* cheapest,
                                          int target_node,
                                          const t_conn_cost_params cost_params,
                                          t_bb bounding_box,
                                          const RouterLookahead& router_lookahead,
                                          std::vector<int>& modified_rr_node_inf,
                                          RouterStats& router_stats) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    int inode = cheapest->index;

    float old_total_cost = route_ctx.rr_node_route_inf[inode].path_cost;
    float old_back_cost = route_ctx.rr_node_route_inf[inode].backward_path_cost;

    float new_total_cost = cheapest->cost;
    float new_back_cost = cheapest->backward_path_cost;

    /* I only re-expand a node if both the "known" backward cost is lower  *
     * in the new expansion (this is necessary to prevent loops from       *
     * forming in the routing and causing havoc) *and* the expected total  *
     * cost to the sink is lower than the old value.  Different R_upstream *
     * values could make a path with lower back_path_cost less desirable   *
     * than one with higher cost.  Test whether or not I should disallow   *
     * re-expansion based on a higher total cost.                          */

    if (old_total_cost > new_total_cost && old_back_cost > new_back_cost) {
        VTR_LOGV_DEBUG(f_router_debug, "    Better cost to %d\n", inode);
        VTR_LOGV_DEBUG(f_router_debug, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(f_router_debug, "    New back cost: %g\n", new_back_cost);
        VTR_LOGV_DEBUG(f_router_debug, "      Setting path costs for assicated node %d (from %d edge %d)\n", cheapest->index, cheapest->u.prev.node, cheapest->u.prev.edge);

        add_to_mod_list(cheapest->index, modified_rr_node_inf);

        route_ctx.rr_node_route_inf[cheapest->index].prev_node = cheapest->u.prev.node;
        route_ctx.rr_node_route_inf[cheapest->index].prev_edge = cheapest->u.prev.edge;
        route_ctx.rr_node_route_inf[cheapest->index].path_cost = new_total_cost;
        route_ctx.rr_node_route_inf[cheapest->index].backward_path_cost = new_back_cost;

        timing_driven_expand_neighbours(cheapest, cost_params, bounding_box,
                                        router_lookahead,
                                        target_node,
                                        router_stats);
    } else {
        VTR_LOGV_DEBUG(f_router_debug, "    Worse cost to %d\n", inode);
        VTR_LOGV_DEBUG(f_router_debug, "    Old total cost: %g\n", old_total_cost);
        VTR_LOGV_DEBUG(f_router_debug, "    Old back cost: %g\n", old_back_cost);
        VTR_LOGV_DEBUG(f_router_debug, "    New total cost: %g\n", new_total_cost);
        VTR_LOGV_DEBUG(f_router_debug, "    New back cost: %g\n", new_back_cost);
    }
}

static t_rt_node* setup_routing_resources(int itry,
                                          ClusterNetId net_id,
                                          unsigned num_sinks,
                                          float pres_fac,
                                          int min_incremental_reroute_fanout,
                                          CBRR& connections_inf,
                                          t_rt_node** rt_node_of_sink) {
    /* Build and return a partial route tree from the legal connections from last iteration.
     * along the way do:
     * 	update pathfinder costs to be accurate to the partial route tree
     *	update the net's traceback to be accurate to the partial route tree
     * 	find and store the pins that still need to be reached in incremental_rerouting_resources.remaining_targets
     * 	find and store the rt nodes that have been reached in incremental_rerouting_resources.reached_rt_sinks
     *	mark the rr_node sinks as targets to be reached */

    auto& route_ctx = g_vpr_ctx.routing();

    t_rt_node* rt_root;

    // for nets below a certain size (min_incremental_reroute_fanout), rip up any old routing
    // otherwise, we incrementally reroute by reusing legal parts of the previous iteration
    // convert the previous iteration's traceback into the starting route tree for this iteration
    if ((int)num_sinks < min_incremental_reroute_fanout || itry == 1) {
        profiling::net_rerouted();

        // rip up the whole net
        pathfinder_update_path_cost(route_ctx.trace[net_id].head, -1, pres_fac);
        free_traceback(net_id);

        rt_root = init_route_tree_to_source(net_id);
        for (unsigned int sink_pin = 1; sink_pin <= num_sinks; ++sink_pin)
            connections_inf.toreach_rr_sink(sink_pin);
        // since all connections will be rerouted for this net, clear all of net's forced reroute flags
        connections_inf.clear_force_reroute_for_net();

        // when we don't prune the tree, we also don't know the sink node indices
        // thus we'll use functions that act on pin indices like mark_ends instead
        // of their versions that act on node indices directly like mark_remaining_ends
        mark_ends(net_id);
    } else {
        auto& reached_rt_sinks = connections_inf.get_reached_rt_sinks();
        auto& remaining_targets = connections_inf.get_remaining_targets();

        profiling::net_rebuild_start();

        // convert the previous iteration's traceback into a route tree
        rt_root = traceback_to_route_tree(net_id);

        //Santiy check that route tree and traceback are equivalent before pruning
        VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace[net_id].head, rt_root));

        // check for edge correctness
        VTR_ASSERT_SAFE(is_valid_skeleton_tree(rt_root));
        VTR_ASSERT_SAFE(should_route_net(net_id, connections_inf, true));

        //Prune the branches of the tree that don't legally lead to sinks
        rt_root = prune_route_tree(rt_root, connections_inf);

        //Now that the tree has been pruned, we can free the old traceback
        // NOTE: this must happen *after* pruning since it changes the
        //       recorded congestion
        pathfinder_update_path_cost(route_ctx.trace[net_id].head, -1, pres_fac);
        free_traceback(net_id);

        if (rt_root) { //Partially pruned
            profiling::route_tree_preserved();

            //Since we have a valid partial routing (to at least one SINK)
            //we need to make sure the traceback is synchronized to the route tree
            traceback_from_route_tree(net_id, rt_root, reached_rt_sinks.size());

            //Sanity check the traceback for self-consistency
            VTR_ASSERT_DEBUG(validate_traceback(route_ctx.trace[net_id].head));

            //Santiy check that route tree and traceback are equivalent after pruning
            VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace[net_id].head, rt_root));

            // put the updated costs of the route tree nodes back into pathfinder
            pathfinder_update_path_cost(route_ctx.trace[net_id].head, 1, pres_fac);

        } else { //Fully destroyed
            profiling::route_tree_pruned();

            //Initialize only to source
            rt_root = init_route_tree_to_source(net_id);

            //NOTE: We leave the traceback uninitiailized, so update_traceback()
            //      will correctly add the SOURCE node when the branch to
            //      the first SINK is found.
            VTR_ASSERT(route_ctx.trace[net_id].head == nullptr);
            VTR_ASSERT(route_ctx.trace[net_id].tail == nullptr);
            VTR_ASSERT(route_ctx.trace_nodes[net_id].empty());
        }

        //Update R/C
        load_new_subtree_R_upstream(rt_root);
        load_new_subtree_C_downstream(rt_root);

        VTR_ASSERT(reached_rt_sinks.size() + remaining_targets.size() == num_sinks);

        //Record current routing
        add_route_tree_to_rr_node_lookup(rt_root);

        // give lookup on the reached sinks
        connections_inf.put_sink_rt_nodes_in_net_pins_lookup(reached_rt_sinks, rt_node_of_sink);

        profiling::net_rebuild_end(num_sinks, remaining_targets.size());

        // check for R_upstream C_downstream and edge correctness
        VTR_ASSERT_SAFE(is_valid_route_tree(rt_root));
        // congestion should've been pruned away
        VTR_ASSERT_SAFE(is_uncongested_route_tree(rt_root));

        // use the nodes to directly mark ends before they get converted to pins
        mark_remaining_ends(remaining_targets);

        // everything dealing with a net works with it in terms of its sink pins; need to convert its sink nodes to sink pins
        connections_inf.convert_sink_nodes_to_net_pins(remaining_targets);

        // still need to calculate the tree's time delay (0 Tarrival means from SOURCE)
        load_route_tree_Tdel(rt_root, 0);

        // mark the lookup (rr_node_route_inf) for existing tree elements as NO_PREVIOUS so add_to_path stops when it reaches one of them
        load_route_tree_rr_route_inf(rt_root);
    }

    // completed constructing the partial route tree and updated all other data structures to match
    return rt_root;
}

static void add_route_tree_to_heap(t_rt_node* rt_node,
                                   int target_node,
                                   const t_conn_cost_params cost_params,
                                   const RouterLookahead& router_lookahead,
                                   RouterStats& router_stats) {
    /* Puts the entire partial routing below and including rt_node onto the heap *
     * (except for those parts marked as not to be expanded) by calling itself   *
     * recursively.                                                              */

    t_rt_node* child_node;
    t_linked_rt_edge* linked_rt_edge;

    /* Pre-order depth-first traversal */
    // IPINs and SINKS are not re_expanded
    if (rt_node->re_expand) {
        add_route_tree_node_to_heap(rt_node,
                                    target_node,
                                    cost_params,
                                    router_lookahead,
                                    router_stats);
    }

    linked_rt_edge = rt_node->u.child_list;

    while (linked_rt_edge != nullptr) {
        child_node = linked_rt_edge->child;
        add_route_tree_to_heap(child_node, target_node,
                               cost_params,
                               router_lookahead,
                               router_stats);
        linked_rt_edge = linked_rt_edge->next;
    }
}

static t_bb add_high_fanout_route_tree_to_heap(t_rt_node* rt_root, int target_node, const t_conn_cost_params cost_params, const RouterLookahead& router_lookahead, const SpatialRouteTreeLookup& spatial_rt_lookup, t_bb net_bounding_box, RouterStats& router_stats) {
    //For high fanout nets we only add those route tree nodes which are spatially close
    //to the sink.
    //
    //Based on:
    //  J. Swartz, V. Betz, J. Rose, "A Fast Routability-Driven Router for FPGAs", FPGA, 1998
    //
    //We rely on a grid-based spatial look-up which is maintained for high fanout nets by
    //update_route_tree(), which allows us to add spatially close route tree nodes without traversing
    //the entire route tree (which is likely large for a high fanout net).

    auto& device_ctx = g_vpr_ctx.device();

    //Determine which bin the target node is located in
    auto& target_rr_node = device_ctx.rr_nodes[target_node];

    int target_bin_x = grid_to_bin_x(target_rr_node.xlow(), spatial_rt_lookup);
    int target_bin_y = grid_to_bin_y(target_rr_node.ylow(), spatial_rt_lookup);

    int nodes_added = 0;

    t_bb highfanout_bb;
    highfanout_bb.xmin = target_rr_node.xlow();
    highfanout_bb.xmax = target_rr_node.xhigh();
    highfanout_bb.ymin = target_rr_node.ylow();
    highfanout_bb.ymax = target_rr_node.yhigh();

    //Add existing routing starting from the target bin.
    //If the target's bin has insufficient existing routing add from the surrounding bins
    bool done = false;
    for (int dx : {0, -1, +1}) {
        size_t bin_x = target_bin_x + dx;

        if (bin_x > spatial_rt_lookup.dim_size(0) - 1) continue; //Out of range

        for (int dy : {0, -1, +1}) {
            size_t bin_y = target_bin_y + dy;

            if (bin_y > spatial_rt_lookup.dim_size(1) - 1) continue; //Out of range

            for (t_rt_node* rt_node : spatial_rt_lookup[bin_x][bin_y]) {
                if (!rt_node->re_expand) continue; //Some nodes (like IPINs) shouldn't be re-expanded

                //Put the node onto the heap
                add_route_tree_node_to_heap(rt_node, target_node, cost_params, router_lookahead, router_stats);

                //Update Bounding Box
                auto& rr_node = device_ctx.rr_nodes[rt_node->inode];
                highfanout_bb.xmin = std::min<int>(highfanout_bb.xmin, rr_node.xlow());
                highfanout_bb.ymin = std::min<int>(highfanout_bb.ymin, rr_node.ylow());
                highfanout_bb.xmax = std::max<int>(highfanout_bb.xmax, rr_node.xhigh());
                highfanout_bb.ymax = std::max<int>(highfanout_bb.ymax, rr_node.yhigh());

                ++nodes_added;
            }

            constexpr int SINGLE_BIN_MIN_NODES = 2;
            if (dx == 0 && dy == 0 && nodes_added > SINGLE_BIN_MIN_NODES) {
                //Target bin contained at least minimum amount of routing
                //
                //We require at least SINGLE_BIN_MIN_NODES to be added.
                //This helps ensure we don't end up with, for example, a single
                //routing wire running in the wrong direction which may not be
                //able to reach the target within the bounding box.
                done = true;
                break;
            }
        }
        if (done) break;
    }

    t_bb bounding_box = net_bounding_box;
    if (nodes_added == 0) { //If the target bin and it's surrounding bins were empty, just add the full route tree
        add_route_tree_to_heap(rt_root, target_node, cost_params, router_lookahead, router_stats);
    } else {
        //We found nearby routing, replace original bounding box to be localized around that routing
        bounding_box = adjust_highfanout_bounding_box(highfanout_bb);
    }

    return bounding_box;
}

static t_bb adjust_highfanout_bounding_box(t_bb highfanout_bb) {
    t_bb bb = highfanout_bb;

    constexpr int HIGH_FANOUT_BB_FAC = 3;
    bb.xmin -= HIGH_FANOUT_BB_FAC;
    bb.ymin -= HIGH_FANOUT_BB_FAC;
    bb.xmax += HIGH_FANOUT_BB_FAC;
    bb.ymax += HIGH_FANOUT_BB_FAC;

    return bb;
}

//Unconditionally adds rt_node to the heap
//
//Note that if you want to respect rt_node->re_expand that is the caller's
//responsibility.
static void add_route_tree_node_to_heap(t_rt_node* rt_node,
                                        int target_node,
                                        const t_conn_cost_params cost_params,
                                        const RouterLookahead& router_lookahead,
                                        RouterStats& router_stats) {
    int inode = rt_node->inode;
    float backward_path_cost = cost_params.criticality * rt_node->Tdel;

    float R_upstream = rt_node->R_upstream;
    float tot_cost = backward_path_cost
                     + cost_params.astar_fac
                           * router_lookahead.get_expected_cost(inode, target_node, cost_params, R_upstream);

    //after budgets are loaded, calculate delay cost as described by RCV paper
    /*R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
     * Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
     * Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.*/
    const t_conn_delay_budget* delay_budget = cost_params.delay_budget;
    if (delay_budget) {
        float zero = 0.0;
        tot_cost += (delay_budget->short_path_criticality + cost_params.criticality) * std::max(zero, delay_budget->target_delay - tot_cost);
        tot_cost += std::pow(std::max(zero, tot_cost - delay_budget->max_delay), 2) / 100e-12;
        tot_cost += std::pow(std::max(zero, delay_budget->min_delay - tot_cost), 2) / 100e-12;
    }

    VTR_LOGV_DEBUG(f_router_debug, "  Adding node %8d to heap from init route tree with cost %g (%s)\n", inode, tot_cost, describe_rr_node(inode).c_str());

    heap_::push_back_node(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
                          backward_path_cost, R_upstream);

    ++router_stats.heap_pushes;
}

static void timing_driven_expand_neighbours(t_heap* current,
                                            const t_conn_cost_params cost_params,
                                            t_bb bounding_box,
                                            const RouterLookahead& router_lookahead,
                                            int target_node,
                                            RouterStats& router_stats) {
    /* Puts all the rr_nodes adjacent to current on the heap.
     */

    auto& device_ctx = g_vpr_ctx.device();

    t_bb target_bb;
    if (target_node != OPEN) {
        target_bb.xmin = device_ctx.rr_nodes[target_node].xlow();
        target_bb.ymin = device_ctx.rr_nodes[target_node].ylow();
        target_bb.xmax = device_ctx.rr_nodes[target_node].xhigh();
        target_bb.ymax = device_ctx.rr_nodes[target_node].yhigh();
    }

    //For each node associated with the current heap element, expand all of it's neighbours
    int num_edges = device_ctx.rr_nodes[current->index].num_edges();
    for (int iconn = 0; iconn < num_edges; iconn++) {
        int to_node = device_ctx.rr_nodes[current->index].edge_sink_node(iconn);
        timing_driven_expand_neighbour(current,
                                       current->index, iconn, to_node,
                                       cost_params,
                                       bounding_box,
                                       router_lookahead,
                                       target_node,
                                       target_bb,
                                       router_stats);
    }
}

//Conditionally adds to_node to the router heap (via path from from_node via from_edge).
//RR nodes outside the expanded bounding box specified in bounding_box are not added
//to the heap.
static void timing_driven_expand_neighbour(t_heap* current,
                                           const int from_node,
                                           const t_edge_size from_edge,
                                           const int to_node,
                                           const t_conn_cost_params cost_params,
                                           const t_bb bounding_box,
                                           const RouterLookahead& router_lookahead,
                                           int target_node,
                                           const t_bb target_bb,
                                           RouterStats& router_stats) {
    auto& device_ctx = g_vpr_ctx.device();

    int to_xlow = device_ctx.rr_nodes[to_node].xlow();
    int to_ylow = device_ctx.rr_nodes[to_node].ylow();
    int to_xhigh = device_ctx.rr_nodes[to_node].xhigh();
    int to_yhigh = device_ctx.rr_nodes[to_node].yhigh();

    if (to_xhigh < bounding_box.xmin      //Strictly left of BB left-edge
        || to_xlow > bounding_box.xmax    //Strictly right of BB right-edge
        || to_yhigh < bounding_box.ymin   //Strictly below BB bottom-edge
        || to_ylow > bounding_box.ymax) { //Strictly above BB top-edge
        VTR_LOGV_DEBUG(f_router_debug,
                       "      Pruned expansion of node %d edge %d -> %d"
                       " (to node location %d,%dx%d,%d outside of expanded"
                       " net bounding box %d,%dx%d,%d)\n",
                       from_node, from_edge, to_node,
                       to_xlow, to_ylow, to_xhigh, to_yhigh,
                       bounding_box.xmin, bounding_box.ymin, bounding_box.xmax, bounding_box.ymax);
        return; /* Node is outside (expanded) bounding box. */
    }

    /* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
     * the issue of how to cost them properly so they don't get expanded before *
     * more promising routes, but makes route-throughs (via CLBs) impossible.   *
     * Change this if you want to investigate route-throughs.                   */
    if (target_node != OPEN) {
        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if (to_type == IPIN) {
            //Check if this IPIN leads to the target block
            // IPIN's of the target block should be contained within it's bounding box
            if (to_xlow < target_bb.xmin
                || to_ylow < target_bb.ymin
                || to_xhigh > target_bb.xmax
                || to_yhigh > target_bb.ymax) {
                VTR_LOGV_DEBUG(f_router_debug,
                               "      Pruned expansion of node %d edge %d -> %d"
                               " (to node is IPIN at %d,%dx%d,%d which does not"
                               " lead to target block %d,%dx%d,%d)\n",
                               from_node, from_edge, to_node,
                               to_xlow, to_ylow, to_xhigh, to_yhigh,
                               target_bb.xmin, target_bb.ymin, target_bb.xmax, target_bb.ymax);
                return;
            }
        }
    }

    VTR_LOGV_DEBUG(f_router_debug, "      Expanding node %d edge %d -> %d\n",
                   from_node, from_edge, to_node);

    timing_driven_add_to_heap(cost_params,
                              router_lookahead,
                              current, from_node, to_node, from_edge, target_node, router_stats);
}

//Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
static void timing_driven_add_to_heap(const t_conn_cost_params cost_params,
                                      const RouterLookahead& router_lookahead,
                                      const t_heap* current,
                                      const int from_node,
                                      const int to_node,
                                      const int iconn,
                                      const int target_node,
                                      RouterStats& router_stats) {
    t_heap* next = alloc_heap_data();
    next->index = to_node;

    //Costs initialized to current
    next->cost = std::numeric_limits<float>::infinity(); //Not used directly
    next->backward_path_cost = current->backward_path_cost;
    next->R_upstream = current->R_upstream;

    timing_driven_expand_node(cost_params,
                              router_lookahead,
                              next, from_node, to_node, iconn, target_node);

    auto& route_ctx = g_vpr_ctx.routing();

    float old_next_total_cost = route_ctx.rr_node_route_inf[to_node].path_cost;
    float old_next_back_cost = route_ctx.rr_node_route_inf[to_node].backward_path_cost;

    float new_next_total_cost = next->cost;
    float new_next_back_cost = next->backward_path_cost;

    if (old_next_total_cost > new_next_total_cost && old_next_back_cost > new_next_back_cost) {
        //Add node to the heap only if the current cost is less than its historic cost, since
        //there is no point in for the router to expand more expensive paths.
        add_to_heap(next);
        ++router_stats.heap_pushes;
    }

    else {
        free_heap_data(next);
    }
}

//Updates current (path step and costs) to account for the step taken to reach to_node
static void timing_driven_expand_node(const t_conn_cost_params cost_params,
                                      const RouterLookahead& router_lookahead,
                                      t_heap* current,
                                      const int from_node,
                                      const int to_node,
                                      const int iconn,
                                      const int target_node) {
    VTR_LOGV_DEBUG(f_router_debug, "      Expanding to node %d (%s)\n", to_node, describe_rr_node(to_node).c_str());

    evaluate_timing_driven_node_costs(current,
                                      cost_params,
                                      router_lookahead,
                                      from_node, to_node, iconn, target_node);

    //Record how we reached this node
    current->index = to_node;
    current->u.prev.edge = iconn;
    current->u.prev.node = from_node;
}

//Calculates the cost of reaching to_node
static void evaluate_timing_driven_node_costs(t_heap* to,
                                              const t_conn_cost_params cost_params,
                                              const RouterLookahead& router_lookahead,
                                              const int from_node,
                                              const int to_node,
                                              const int iconn,
                                              const int target_node) {
    /* new_costs.backward_cost: is the "known" part of the cost to this node -- the
     * congestion cost of all the routing resources back to the existing route
     * plus the known delay of the total path back to the source.
     *
     * new_costs.total_cost: is this "known" backward cost + an expected cost to get to the target.
     *
     * new_costs.R_upstream: is the upstream resistance at the end of this node
     */
    auto& device_ctx = g_vpr_ctx.device();

    //Info for the switch connecting from_node to_node
    int iswitch = device_ctx.rr_nodes[from_node].edge_switch(iconn);
    bool switch_buffered = device_ctx.rr_switch_inf[iswitch].buffered();
    float switch_R = device_ctx.rr_switch_inf[iswitch].R;
    float switch_Tdel = device_ctx.rr_switch_inf[iswitch].Tdel;
    float switch_Cinternal = device_ctx.rr_switch_inf[iswitch].Cinternal;

    //To node info
    float node_C = device_ctx.rr_nodes[to_node].C();
    float node_R = device_ctx.rr_nodes[to_node].R();

    //From node info
    float from_node_R = device_ctx.rr_nodes[from_node].R();

    //Once a connection has been made between from_node and to_node, depending on the switch, Tdel may increase due to the internal capacitance of that switch. Even though this delay physically affects from_node, we are making the adjustment on the to_node, because we know the connection used.

    //To adjust for the time delay, we will need to compute the product of the Rdel that is associated with from_node and the internal capacitance of the switch. First, we will calculate Rdel_adjust. Just like in the computation for Rdel, we consider only half of from_node's resistance.

    float Rdel_adjust = to->R_upstream - 0.5 * from_node_R;

    //Update R_upstream
    if (switch_buffered) {
        to->R_upstream = 0.; //No upstream resistance
    } else {
        //R_Upstream already initialized
    }

    to->R_upstream += switch_R; //Switch resistance
    to->R_upstream += node_R;   //Node resistance

    //Calculate delay
    float Rdel = to->R_upstream - 0.5 * node_R; //Only consider half node's resistance for delay
    float Tdel = switch_Tdel + Rdel * node_C;

    //Now we will adjust the Tdel to account for the delay caused by the internal capacitance.
    Tdel += Rdel_adjust * switch_Cinternal;

    //Update the backward cost (upstream already included)
    to->backward_path_cost += (1. - cost_params.criticality) * get_rr_cong_cost(to_node); //Congestion cost
    to->backward_path_cost += cost_params.criticality * Tdel;                             //Delay cost
    if (cost_params.bend_cost != 0.) {
        t_rr_type from_type = device_ctx.rr_nodes[from_node].type();
        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if ((from_type == CHANX && to_type == CHANY) || (from_type == CHANY && to_type == CHANX)) {
            to->backward_path_cost += cost_params.bend_cost; //Bend cost
        }
    }

    float total_cost = 0.;
    const t_conn_delay_budget* delay_budget = cost_params.delay_budget;
    if (delay_budget) {
        //If budgets specified calculate cost as described by RCV paper:
        //    R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
        //     Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
        //     Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.

        //TODO: Since these targets are delays, shouldn't we be using Tdel instead of new_costs.total_cost on RHS?
        total_cost += (delay_budget->short_path_criticality + cost_params.criticality) * std::max(0.f, delay_budget->target_delay - total_cost);
        total_cost += std::pow(std::max(0.f, total_cost - delay_budget->max_delay), 2) / 100e-12;
        total_cost += std::pow(std::max(0.f, delay_budget->min_delay - total_cost), 2) / 100e-12;
    }

    //Update total cost
    float expected_cost = router_lookahead.get_expected_cost(to_node, target_node, cost_params, to->R_upstream);
    VTR_LOGV_DEBUG(f_router_debug && !std::isfinite(expected_cost),
                   "        Lookahead from %s (%s) to %s (%s) is non-finite, expected_cost = %f, to->R_upstream = %f\n",
                   rr_node_arch_name(to_node).c_str(), describe_rr_node(to_node).c_str(),
                   rr_node_arch_name(target_node).c_str(), describe_rr_node(target_node).c_str(),
                   expected_cost, to->R_upstream);
    total_cost = to->backward_path_cost + cost_params.astar_fac * expected_cost;

    to->cost = total_cost;
}

void update_rr_base_costs(int fanout) {
    /* Changes the base costs of different types of rr_nodes according to the  *
     * criticality, fanout, etc. of the current net being routed (net_id).       */
    auto& device_ctx = g_vpr_ctx.mutable_device();

    float factor;
    size_t index;

    /* Other reasonable values for factor include fanout and 1 */
    factor = sqrt(fanout);

    for (index = CHANX_COST_INDEX_START; index < device_ctx.rr_indexed_data.size(); index++) {
        if (device_ctx.rr_indexed_data[index].T_quadratic > 0.) { /* pass transistor */
            device_ctx.rr_indexed_data[index].base_cost = device_ctx.rr_indexed_data[index].saved_base_cost * factor;
        } else {
            device_ctx.rr_indexed_data[index].base_cost = device_ctx.rr_indexed_data[index].saved_base_cost;
        }
    }
}

static bool timing_driven_check_net_delays(vtr::vector<ClusterNetId, float*>& net_delay) {
    constexpr float ERROR_TOL = 0.0001;

    /* Checks that the net delays computed incrementally during timing driven    *
     * routing match those computed from scratch by the net_delay.c module.      */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    unsigned int ipin;
    vtr::vector<ClusterNetId, float*> net_delay_check;

    vtr::t_chunk list_head_net_delay_check_ch;

    net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch);
    load_net_delay_from_routing(net_delay_check);

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            if (net_delay_check[net_id][ipin] == 0.) { /* Should be only GLOBAL nets */
                if (fabs(net_delay[net_id][ipin]) > ERROR_TOL) {
                    VPR_ERROR(VPR_ERROR_ROUTE,
                              "in timing_driven_check_net_delays: net %lu pin %d.\n"
                              "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
                              size_t(net_id), ipin, net_delay[net_id][ipin], net_delay_check[net_id][ipin]);
                }
            } else {
                float error = fabs(1.0 - net_delay[net_id][ipin] / net_delay_check[net_id][ipin]);
                if (error > ERROR_TOL) {
                    VPR_ERROR(VPR_ERROR_ROUTE,
                              "in timing_driven_check_net_delays: net %d pin %lu.\n"
                              "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
                              size_t(net_id), ipin, net_delay[net_id][ipin], net_delay_check[net_id][ipin]);
                }
            }
        }
    }

    free_net_delay(net_delay_check, &list_head_net_delay_check_ch);
    return true;
}

/* Detect if net should be routed or not */
static bool should_route_net(ClusterNetId net_id, CBRR& connections_inf, bool if_force_reroute) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

    t_trace* tptr = route_ctx.trace[net_id].head;

    if (tptr == nullptr) {
        /* No routing yet. */
        return true;
    }

    for (;;) {
        int inode = tptr->index;
        int occ = route_ctx.rr_node_route_inf[inode].occ();
        int capacity = device_ctx.rr_nodes[inode].capacity();

        if (occ > capacity) {
            return true; /* overuse detected */
        }

        if (tptr->iswitch == OPEN) { //End of a branch
            // even if net is fully routed, not complete if parts of it should get ripped up (EXPERIMENTAL)
            if (if_force_reroute) {
                if (connections_inf.should_force_reroute_connection(inode)) {
                    return true;
                }
            }
            tptr = tptr->next; /* Skip next segment (duplicate of original branch node). */
            if (tptr == nullptr)
                break;
        }

        tptr = tptr->next;

    } /* End while loop -- did an entire traceback. */

    VTR_ASSERT(connections_inf.get_remaining_targets().empty());

    return false; /* Current route has no overuse */
}

static bool early_exit_heuristic(const t_router_opts& router_opts, const WirelengthInfo& wirelength_info) {
    /* Early exit code for cases where it is obvious that a successful route will not be found
     * Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit */

    if (wirelength_info.used_wirelength_ratio() > router_opts.init_wirelength_abort_threshold) {
        VTR_LOG("Wire length usage ratio %g exceeds limit of %g, fail routing.\n",
                wirelength_info.used_wirelength_ratio(),
                router_opts.init_wirelength_abort_threshold);
        return true;
    }
    return false;
}

// incremental rerouting resources class definitions
Connection_based_routing_resources::Connection_based_routing_resources()
    : current_inet(NO_PREVIOUS)
    , // not routing to a specific net yet (note that NO_PREVIOUS is not unsigned, so will be largest unsigned)
    last_stable_critical_path_delay{0.0f}
    , critical_path_growth_tolerance{1.001f}
    , connection_criticality_tolerance{0.9f}
    , connection_delay_optimality_tolerance{1.1f} {
    /* Initialize the persistent data structures for incremental rerouting
     * this includes rr_sink_node_to_pin, which provides pin lookup given a
     * sink node for a specific net.
     *
     * remaining_targets will reserve enough space to ensure it won't need
     * to grow while storing the sinks that still need routing after pruning
     *
     * reached_rt_sinks will also reserve enough space, but instead of
     * indices, it will store the pointers to route tree nodes */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    // can have as many targets as sink pins (total number of pins - SOURCE pin)
    // supposed to be used as persistent vector growing with push_back and clearing at the start of each net routing iteration
    auto max_sink_pins_per_net = std::max(get_max_pins_per_net() - 1, 0);
    remaining_targets.reserve(max_sink_pins_per_net);
    reached_rt_sinks.reserve(max_sink_pins_per_net);

    size_t routing_num_nets = cluster_ctx.clb_nlist.nets().size();
    rr_sink_node_to_pin.resize(routing_num_nets);
    lower_bound_connection_delay.resize(routing_num_nets);
    forcible_reroute_connection_flag.resize(routing_num_nets);

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        // unordered_map<int,int> net_node_to_pin;
        auto& net_node_to_pin = rr_sink_node_to_pin[net_id];
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[net_id];
        auto& net_forcible_reroute_connection_flag = forcible_reroute_connection_flag[net_id];

        unsigned int num_pins = cluster_ctx.clb_nlist.net_pins(net_id).size();
        net_node_to_pin.reserve(num_pins - 1);                      // not looking up on the SOURCE pin
        net_lower_bound_connection_delay.reserve(num_pins - 1);     // will be filled in after the 1st iteration's
        net_forcible_reroute_connection_flag.reserve(num_pins - 1); // all false to begin with

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[net_id][ipin];

            net_node_to_pin.insert({rr_sink_node, ipin});
            net_forcible_reroute_connection_flag.insert({rr_sink_node, false});
        }
    }
}

void Connection_based_routing_resources::convert_sink_nodes_to_net_pins(std::vector<int>& rr_sink_nodes) const {
    /* Turn a vector of device_ctx.rr_nodes indices, assumed to be of sinks for a net *
     * into the pin indices of the same net. */

    VTR_ASSERT(current_inet != ClusterNetId::INVALID()); // not uninitialized

    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (size_t s = 0; s < rr_sink_nodes.size(); ++s) {
        auto mapping = node_to_pin_mapping.find(rr_sink_nodes[s]);
        if (mapping != node_to_pin_mapping.end()) {
            rr_sink_nodes[s] = mapping->second;
        } else {
            VTR_ASSERT_SAFE_MSG(false, "Should always expect it find a pin mapping for its own net");
        }
    }
}

void Connection_based_routing_resources::put_sink_rt_nodes_in_net_pins_lookup(const std::vector<t_rt_node*>& sink_rt_nodes,
                                                                              t_rt_node** rt_node_of_sink) const {
    /* Load rt_node_of_sink (which maps a PIN index to a route tree node)
     * with a vector of route tree sink nodes. */

    VTR_ASSERT(current_inet != ClusterNetId::INVALID());

    // a net specific mapping from node index to pin index
    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (t_rt_node* rt_node : sink_rt_nodes) {
        auto mapping = node_to_pin_mapping.find(rt_node->inode);

        if (mapping != node_to_pin_mapping.end()) {
            rt_node_of_sink[mapping->second] = rt_node;
        } else {
            VTR_ASSERT_SAFE_MSG(false, "element should be able to find itself");
        }
    }
}

bool Connection_based_routing_resources::sanity_check_lookup() const {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        const auto& net_node_to_pin = rr_sink_node_to_pin[net_id];

        for (auto mapping : net_node_to_pin) {
            auto sanity = net_node_to_pin.find(mapping.first);
            if (sanity == net_node_to_pin.end()) {
                VTR_LOG("%d cannot find itself (net %lu)\n", mapping.first, size_t(net_id));
                return false;
            }
            VTR_ASSERT(route_ctx.net_rr_terminals[net_id][mapping.second] == mapping.first);
        }
    }
    return true;
}

void Connection_based_routing_resources::set_lower_bound_connection_delays(vtr::vector<ClusterNetId, float*>& net_delay) {
    /* Set the lower bound connection delays after first iteration, which only optimizes for timing delay.
     * This will be used later to judge the optimality of a connection, with suboptimal ones being candidates
     * for forced reroute */

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[net_id];

        for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            net_lower_bound_connection_delay.push_back(net_delay[net_id][ipin]);
        }
    }
}

/* Run through all non-congested connections of all nets and see if any need to be forcibly rerouted.
 * The connection must satisfy all following criteria:
 * 1. the connection is critical enough
 * 2. the connection is suboptimal, in comparison to lower_bound_connection_delay  */
bool Connection_based_routing_resources::forcibly_reroute_connections(float max_criticality,
                                                                      std::shared_ptr<const SetupTimingInfo> timing_info,
                                                                      const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                                                      vtr::vector<ClusterNetId, float*>& net_delay) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    bool any_connection_rerouted = false; // true if any connection has been marked for rerouting

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
            int ipin = cluster_ctx.clb_nlist.pin_net_index(pin_id);

            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[net_id][ipin];

            //Clear any forced re-routing from the previuos iteration
            forcible_reroute_connection_flag[net_id][rr_sink_node] = false;

            // skip if connection is internal to a block such that SOURCE->OPIN->IPIN->SINK directly, which would have 0 time delay
            if (lower_bound_connection_delay[net_id][ipin - 1] == 0)
                continue;

            // update if more optimal connection found
            if (net_delay[net_id][ipin] < lower_bound_connection_delay[net_id][ipin - 1]) {
                lower_bound_connection_delay[net_id][ipin - 1] = net_delay[net_id][ipin];
                continue;
            }

            // skip if connection criticality is too low (not a problem connection)
            float pin_criticality = calculate_clb_net_pin_criticality(*timing_info, netlist_pin_lookup, pin_id);
            if (pin_criticality < (max_criticality * connection_criticality_tolerance))
                continue;

            // skip if connection's delay is close to optimal
            if (net_delay[net_id][ipin] < (lower_bound_connection_delay[net_id][ipin - 1] * connection_delay_optimality_tolerance))
                continue;

            forcible_reroute_connection_flag[net_id][rr_sink_node] = true;
            // note that we don't set forcible_reroute_connection_flag to false when the converse is true
            // resetting back to false will be done during tree pruning, after the sink has been legally reached
            any_connection_rerouted = true;

            profiling::mark_for_forced_reroute();
        }
    }

    // non-stable configuration if any connection has to be rerouted, otherwise stable
    return !any_connection_rerouted;
}

void Connection_based_routing_resources::clear_force_reroute_for_connection(int rr_sink_node) {
    forcible_reroute_connection_flag[current_inet][rr_sink_node] = false;
    profiling::perform_forced_reroute();
}

void Connection_based_routing_resources::clear_force_reroute_for_net() {
    VTR_ASSERT(current_inet != ClusterNetId::INVALID());

    auto& net_flags = forcible_reroute_connection_flag[current_inet];
    for (auto& force_reroute_flag : net_flags) {
        if (force_reroute_flag.second) {
            force_reroute_flag.second = false;
            profiling::perform_forced_reroute();
        }
    }
}

static OveruseInfo calculate_overuse_info() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    std::unordered_set<int> checked_nodes;

    size_t overused_nodes = 0;
    size_t total_overuse = 0;
    size_t worst_overuse = 0;

    //We walk through the entire routing calculating the overuse for each node.
    //Since in the presence of overuse multiple nets could be using a single node
    //(and also since branch nodes show up multiple times in the traceback) we use
    //checked_nodes to avoid double counting the overuse.
    //
    //Note that we walk through the entire routing and *not* the RR graph, which
    //should be more efficient (since usually only a portion of the RR graph is
    //used by routing, particularly on large devices).
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (t_trace* tptr = route_ctx.trace[net_id].head; tptr != nullptr; tptr = tptr->next) {
            int inode = tptr->index;

            auto result = checked_nodes.insert(inode);
            if (!result.second) { //Already counted
                continue;
            }

            int overuse = route_ctx.rr_node_route_inf[inode].occ() - device_ctx.rr_nodes[inode].capacity();
            if (overuse > 0) {
                overused_nodes += 1;

                total_overuse += overuse;
                worst_overuse = std::max(worst_overuse, size_t(overuse));
            }
        }
    }

    return OveruseInfo(device_ctx.rr_nodes.size(), overused_nodes, total_overuse, worst_overuse);
}

static size_t calculate_wirelength_available() {
    auto& device_ctx = g_vpr_ctx.device();

    size_t available_wirelength = 0;
    for (size_t i = 0; i < device_ctx.rr_nodes.size(); ++i) {
        if (device_ctx.rr_nodes[i].type() == CHANX || device_ctx.rr_nodes[i].type() == CHANY) {
            size_t length_x = device_ctx.rr_nodes[i].xhigh() - device_ctx.rr_nodes[i].xlow();
            size_t length_y = device_ctx.rr_nodes[i].yhigh() - device_ctx.rr_nodes[i].ylow();

            available_wirelength += device_ctx.rr_nodes[i].capacity() * (length_x + length_y + 1);
        }
    }
    return available_wirelength;
}

static WirelengthInfo calculate_wirelength_info(size_t available_wirelength) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t used_wirelength = 0;
    VTR_ASSERT(available_wirelength > 0);

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_ignored(net_id)
            && cluster_ctx.clb_nlist.net_sinks(net_id).size() != 0) { /* Globals don't count. */
            int bends, wirelength, segments;
            get_num_bends_and_length(net_id, &bends, &wirelength, &segments);

            used_wirelength += wirelength;
        }
    }

    return WirelengthInfo(available_wirelength, used_wirelength);
}

static void print_route_status_header() {
    VTR_LOG("---- ------ ------- ---- ------- ------- ------- ----------------- --------------- -------- ---------- ---------- ---------- ---------- --------\n");
    VTR_LOG("Iter   Time    pres  BBs    Heap  Re-Rtd  Re-Rtd Overused RR Nodes      Wirelength      CPD       sTNS       sWNS       hTNS       hWNS Est Succ\n");
    VTR_LOG("      (sec)     fac Updt    push    Nets   Conns                                       (ns)       (ns)       (ns)       (ns)       (ns)     Iter\n");
    VTR_LOG("---- ------ ------- ---- ------- ------- ------- ----------------- --------------- -------- ---------- ---------- ---------- ---------- --------\n");
}

static void print_route_status(int itry, double elapsed_sec, float pres_fac, int num_bb_updated, const RouterStats& router_stats, const OveruseInfo& overuse_info, const WirelengthInfo& wirelength_info, std::shared_ptr<const SetupHoldTimingInfo> timing_info, float est_success_iteration) {
    //Iteration
    VTR_LOG("%4d", itry);

    //Elapsed Time
    VTR_LOG(" %6.1f", elapsed_sec);

    //pres_fac
    constexpr int PRES_FAC_DIGITS = 7;
    constexpr int PRES_FAC_SCI_PRECISION = 1;
    pretty_print_float(" ", pres_fac, PRES_FAC_DIGITS, PRES_FAC_SCI_PRECISION);
    //VTR_LOG(" %5.1f", pres_fac);

    //Number of bounding boxes updated
    VTR_LOG(" %4d", num_bb_updated);

    //Heap push/pop
    constexpr int HEAP_OP_DIGITS = 7;
    constexpr int HEAP_OP_SCI_PRECISION = 2;
    pretty_print_uint(" ", router_stats.heap_pushes, HEAP_OP_DIGITS, HEAP_OP_SCI_PRECISION);
    VTR_ASSERT(router_stats.heap_pops <= router_stats.heap_pushes);

    //Rerouted nets
    constexpr int NET_ROUTED_DIGITS = 7;
    constexpr int NET_ROUTED_SCI_PRECISION = 2;
    pretty_print_uint(" ", router_stats.nets_routed, NET_ROUTED_DIGITS, NET_ROUTED_SCI_PRECISION);

    //Rerouted connections
    constexpr int CONN_ROUTED_DIGITS = 7;
    constexpr int CONN_ROUTED_SCI_PRECISION = 2;
    pretty_print_uint(" ", router_stats.connections_routed, CONN_ROUTED_DIGITS, CONN_ROUTED_SCI_PRECISION);

    //Overused RR nodes
    constexpr int OVERUSE_DIGITS = 7;
    constexpr int OVERUSE_SCI_PRECISION = 2;
    pretty_print_uint(" ", overuse_info.overused_nodes(), OVERUSE_DIGITS, OVERUSE_SCI_PRECISION);
    VTR_LOG(" (%6.3f%)", overuse_info.overused_node_ratio() * 100);

    //Wirelength
    constexpr int WL_DIGITS = 7;
    constexpr int WL_SCI_PRECISION = 2;
    pretty_print_uint(" ", wirelength_info.used_wirelength(), WL_DIGITS, WL_SCI_PRECISION);
    VTR_LOG(" (%4.1f%)", wirelength_info.used_wirelength_ratio() * 100);

    //CPD
    if (timing_info) {
        float cpd = timing_info->least_slack_critical_path().delay();
        VTR_LOG(" %#8.3f", 1e9 * cpd);
    } else {
        VTR_LOG(" %8s", "N/A");
    }

    //sTNS
    if (timing_info) {
        float sTNS = timing_info->setup_total_negative_slack();
        VTR_LOG(" % #10.4g", 1e9 * sTNS);
    } else {
        VTR_LOG(" %10s", "N/A");
    }

    //sWNS
    if (timing_info) {
        float sWNS = timing_info->setup_worst_negative_slack();
        VTR_LOG(" % #10.3f", 1e9 * sWNS);
    } else {
        VTR_LOG(" %10s", "N/A");
    }

    //hTNS
    if (timing_info) {
        float hTNS = timing_info->hold_total_negative_slack();
        VTR_LOG(" % #10.4g", 1e9 * hTNS);
    } else {
        VTR_LOG(" %10s", "N/A");
    }

    //hWNS
    if (timing_info) {
        float hWNS = timing_info->hold_worst_negative_slack();
        VTR_LOG(" % #10.3f", 1e9 * hWNS);
    } else {
        VTR_LOG(" %10s", "N/A");
    }

    //Estimated success iteration
    if (std::isnan(est_success_iteration)) {
        VTR_LOG(" %8s", "N/A");
    } else {
        VTR_LOG(" %8.0f", est_success_iteration);
    }

    VTR_LOG("\n");

    fflush(stdout);
}

static std::string describe_unrouteable_connection(const int source_node, const int sink_node) {
    std::string msg = vtr::string_fmt(
        "Cannot route from %s (%s) to "
        "%s (%s) -- no possible path",
        rr_node_arch_name(source_node).c_str(), describe_rr_node(source_node).c_str(),
        rr_node_arch_name(sink_node).c_str(), describe_rr_node(sink_node).c_str());

    return msg;
}

//Returns true if the specified net fanout is classified as high fanout
static bool is_high_fanout(int fanout, int fanout_threshold) {
    if (fanout_threshold < 0 || fanout < fanout_threshold) return false;
    return true;
}

//In heavily congested designs a static bounding box (BB) can
//become problematic for routability (it effectively enforces a
//hard blockage restricting where a net can route).
//
//For instance, the router will try to route non-critical connections
//away from congested regions, but may end up hitting the edge of the
//bounding box. Limiting how far out-of-the-way it can be routed, and
//preventing congestion from resolving.
//
//To alleviate this, we dynamically expand net bounding boxes if the net's
//*current* routing uses RR nodes 'close' to the edge of it's bounding box.
//
//The result is that connections trying to move out of the way and hitting
//their BB will have their bounding boxes will expand slowly in that direction.
//This helps spread out regions of heavy congestion (over several routing
//iterations).
//
//By growing the BBs slowly and only as needed we minimize the size of the BBs.
//This helps keep the router's graph search fast.
//
//Typically, only a small minority of nets (typically > 10%) have their BBs updated
//each routing iteration.
static size_t dynamic_update_bounding_boxes(const std::vector<ClusterNetId>& updated_nets, int high_fanout_threshold) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    auto& clb_nlist = cluster_ctx.clb_nlist;
    auto& grid = device_ctx.grid;

    //Controls how close a net's routing needs to be to it's bounding box
    //before the bounding box is expanded.
    //
    //A value of zero indicates that the routing needs to be at the bounding box
    //edge
    constexpr int DYNAMIC_BB_DELTA_THRESHOLD = 0;

    //Walk through each net, calculating the bounding box of its current routing,
    //and then increase the router's bounding box if the two are close together

    int grid_xmax = grid.width() - 1;
    int grid_ymax = grid.height() - 1;

    size_t num_bb_updated = 0;

    for (ClusterNetId net : updated_nets) {
        t_trace* routing_head = route_ctx.trace[net].head;

        if (routing_head == nullptr) continue; //Skip if no routing

        //We do not adjust the boundig boxes of high fanout nets, since they
        //use different bounding boxes based on the target location.
        //
        //This ensures that the delta values calculated below are always non-negative
        if (is_high_fanout(clb_nlist.net_sinks(net).size(), high_fanout_threshold)) continue;

        t_bb curr_bb = calc_current_bb(routing_head);

        t_bb& router_bb = route_ctx.route_bb[net];

        //Calculate the distances between the net's used RR nodes and
        //the router's bounding box
        int delta_xmin = curr_bb.xmin - router_bb.xmin;
        int delta_xmax = router_bb.xmax - curr_bb.xmax;
        int delta_ymin = curr_bb.ymin - router_bb.ymin;
        int delta_ymax = router_bb.ymax - curr_bb.ymax;

        //Note that if the net uses non-configurable switches it's routing
        //may end-up outside the bounding boxes, so the delta values may be
        //negative. The code below will expand the bounding box in those
        //cases.

        //Expand each dimension by one if within DYNAMIC_BB_DELTA_THRESHOLD threshold
        bool updated_bb = false;
        if (delta_xmin <= DYNAMIC_BB_DELTA_THRESHOLD && router_bb.xmin > 0) {
            --router_bb.xmin;
            updated_bb = true;
        }

        if (delta_ymin <= DYNAMIC_BB_DELTA_THRESHOLD && router_bb.ymin > 0) {
            --router_bb.ymin;
            updated_bb = true;
        }

        if (delta_xmax <= DYNAMIC_BB_DELTA_THRESHOLD && router_bb.xmax < grid_xmax) {
            ++router_bb.xmax;
            updated_bb = true;
        }

        if (delta_ymax <= DYNAMIC_BB_DELTA_THRESHOLD && router_bb.ymax < grid_ymax) {
            ++router_bb.ymax;
            updated_bb = true;
        }

        if (updated_bb) {
            ++num_bb_updated;
            //VTR_LOG("Expanded net %6zu router BB to (%d,%d)x(%d,%d) based on net RR node BB (%d,%d)x(%d,%d)\n", size_t(net),
            //router_bb.xmin, router_bb.ymin, router_bb.xmax, router_bb.ymax,
            //curr_bb.xmin, curr_bb.ymin, curr_bb.xmax, curr_bb.ymax);
        }
    }
    return num_bb_updated;
}

//Returns the bounding box of a net's used routing resources
static t_bb calc_current_bb(const t_trace* head) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& grid = device_ctx.grid;

    t_bb bb;
    bb.xmin = grid.width() - 1;
    bb.ymin = grid.height() - 1;
    bb.xmax = 0;
    bb.ymax = 0;

    for (const t_trace* elem = head; elem != nullptr; elem = elem->next) {
        const t_rr_node& node = device_ctx.rr_nodes[elem->index];
        //The router interprets RR nodes which cross the boundary as being
        //'within' of the BB. Only thos which are *strictly* out side the
        //box are exluded, hence we use the nodes xhigh/yhigh for xmin/xmax,
        //and xlow/ylow for xmax/ymax calculations
        bb.xmin = std::min<int>(bb.xmin, node.xhigh());
        bb.ymin = std::min<int>(bb.ymin, node.yhigh());
        bb.xmax = std::max<int>(bb.xmax, node.xlow());
        bb.ymax = std::max<int>(bb.ymax, node.ylow());
    }

    VTR_ASSERT(bb.xmin <= bb.xmax);
    VTR_ASSERT(bb.ymin <= bb.ymax);

    return bb;
}

void enable_router_debug(const t_router_opts& router_opts, ClusterNetId net, int sink_rr) {
    bool all_net_debug = (router_opts.router_debug_net == -1);

    bool specific_net_debug = (router_opts.router_debug_net >= 0);
    bool specific_sink_debug = (router_opts.router_debug_sink_rr >= 0);

    bool match_net = (ClusterNetId(router_opts.router_debug_net) == net);
    bool match_sink = (router_opts.router_debug_sink_rr == sink_rr);

    if (all_net_debug) {
        VTR_ASSERT(!specific_net_debug);

        if (specific_sink_debug) {
            //Specific sink specified, debug if sink matches
            f_router_debug = match_sink;
        } else {
            //Debug all nets (no specific sink specified)
            f_router_debug = true;
        }
    } else if (specific_net_debug) {
        if (specific_sink_debug) {
            //Debug if both net and sink match
            f_router_debug = match_net && match_sink;
        } else {
            f_router_debug = match_net;
        }
    } else if (specific_sink_debug) {
        VTR_ASSERT(!all_net_debug);
        VTR_ASSERT(!specific_net_debug);

        //Debug if match sink
        f_router_debug = match_sink;
    } else {
        VTR_ASSERT(!all_net_debug);
        VTR_ASSERT(!specific_net_debug);
        VTR_ASSERT(!specific_sink_debug);
        //Disable debug output
        f_router_debug = false;
    }

#ifndef VTR_ENABLE_DEBUG_LOGGING
    VTR_LOGV_WARN(f_router_debug, "Limited router debug output provided since compiled without VTR_ENABLE_DEBUG_LOGGING defined\n");
#endif
}

static bool is_better_quality_routing(const vtr::vector<ClusterNetId, t_traceback>& best_routing,
                                      const RoutingMetrics& best_routing_metrics,
                                      const WirelengthInfo& wirelength_info,
                                      std::shared_ptr<const SetupHoldTimingInfo> timing_info) {
    if (best_routing.empty()) {
        return true; //First legal routing
    }

    //Rank first based on sWNS, followed by other timing metrics
    if (timing_info) {
        if (timing_info->setup_worst_negative_slack() > best_routing_metrics.sWNS) {
            return true;
        } else if (timing_info->setup_worst_negative_slack() < best_routing_metrics.sWNS) {
            return false;
        }

        if (timing_info->setup_total_negative_slack() > best_routing_metrics.sTNS) {
            return true;
        } else if (timing_info->setup_total_negative_slack() < best_routing_metrics.sTNS) {
            return false;
        }

        if (timing_info->hold_worst_negative_slack() > best_routing_metrics.hWNS) {
            return true;
        } else if (timing_info->hold_worst_negative_slack() > best_routing_metrics.hWNS) {
            return false;
        }

        if (timing_info->hold_total_negative_slack() > best_routing_metrics.hTNS) {
            return true;
        } else if (timing_info->hold_total_negative_slack() > best_routing_metrics.hTNS) {
            return false;
        }
    }

    //Finally, wirelength tie breaker
    return wirelength_info.used_wirelength() < best_routing_metrics.used_wirelength;
}

static bool early_reconvergence_exit_heuristic(const t_router_opts& router_opts,
                                               int itry_since_last_convergence,
                                               std::shared_ptr<const SetupHoldTimingInfo> timing_info,
                                               const RoutingMetrics& best_routing_metrics) {
    //Give-up on reconvergent routing if the CPD improvement after the
    //first iteration since convergence is small, compared to the best
    //CPD seen so far
    if (itry_since_last_convergence == 1) {
        float cpd_ratio = timing_info->setup_worst_negative_slack() / best_routing_metrics.sWNS;

        //Give up if we see less than a 1% CPD improvement,
        //after reducing pres_fac. Typically larger initial
        //improvements are needed to see an actual improvement
        //in final legal routing quality.
        if (cpd_ratio >= router_opts.reconvergence_cpd_threshold) {
            VTR_LOG("Giving up routing since additional routing convergences seem unlikely to improve quality (CPD ratio: %g)\n", cpd_ratio);
            return true; //Potential CPD improvement is small, don't spend run-time trying to improve it
        }
    }

    return false; //Don't give up
}

static void generate_route_timing_reports(const t_router_opts& router_opts,
                                          const t_analysis_opts& analysis_opts,
                                          const SetupTimingInfo& timing_info,
                                          const RoutingDelayCalculator& delay_calc) {
    auto& timing_ctx = g_vpr_ctx.timing();
    auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph, delay_calc);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph, *timing_ctx.constraints);

    timing_reporter.report_timing_setup(router_opts.first_iteration_timing_report_file, *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
}

// If a route is ripped up during routing, non-configurable sets are left
// behind.  As a result, the final routing may have stubs at
// non-configurable sets.  This function tracks non-configurable set usage,
// and if the sets are unused, prunes them.
static void prune_unused_non_configurable_nets(CBRR& connections_inf) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    std::vector<int> non_config_node_set_usage(device_ctx.rr_non_config_node_sets.size(), 0);
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        connections_inf.prepare_routing_for_net(net_id);
        connections_inf.clear_force_reroute_for_net();

        std::fill(non_config_node_set_usage.begin(), non_config_node_set_usage.end(), 0);
        t_rt_node* rt_root = traceback_to_route_tree(net_id, &non_config_node_set_usage);
        if (rt_root == nullptr) {
            continue;
        }

        //Santiy check that route tree and traceback are equivalent before pruning
        VTR_ASSERT(verify_traceback_route_tree_equivalent(
            route_ctx.trace[net_id].head, rt_root));

        // check for edge correctness
        VTR_ASSERT_SAFE(is_valid_skeleton_tree(rt_root));

        //Prune the branches of the tree that don't legally lead to sinks
        rt_root = prune_route_tree(rt_root, connections_inf,
                                   &non_config_node_set_usage);

        // Free old traceback.
        free_traceback(net_id);

        // Update traceback with pruned tree.
        auto& reached_rt_sinks = connections_inf.get_reached_rt_sinks();
        traceback_from_route_tree(net_id, rt_root, reached_rt_sinks.size());
        VTR_ASSERT(verify_traceback_route_tree_equivalent(route_ctx.trace[net_id].head, rt_root));

        free_route_tree(rt_root);
    }
}
