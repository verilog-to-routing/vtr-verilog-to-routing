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
#include "path_delay.h"
#include "net_delay.h"
#include "stats.h"
#include "echo_files.h"
#include "draw.h"
#include "rr_graph.h"
#include "routing_predictor.h"

// all functions in profiling:: namespace, which are only activated if PROFILE is defined
#include "route_profiling.h"

#include "timing_info.h"
#include "timing_util.h"
#include "route_budgets.h"

#include "router_lookahead_map.h"

#define CONGESTED_SLOPE_VAL -0.04

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

struct t_timing_driven_node_costs {
    float backward_cost = 0.;
    float total_cost = 0.;
    float R_upstream = 0.;
};

/*
 * File-scope variables
 */

//Run-time flag to control when router debug information is printed
//Note only enables debug output if compiled with VTR_ENABLE_DEBUG_LOGGING defined
bool f_router_debug = true;

/******************** Subroutines local to route_timing.c ********************/

static t_rt_node* setup_routing_resources(int itry, ClusterNetId net_id, unsigned num_sinks, float pres_fac, int min_incremental_reroute_fanout,
        CBRR& incremental_rerouting_res, t_rt_node** rt_node_of_sink);

static bool timing_driven_route_sink(int itry, ClusterNetId net_id, unsigned itarget, int target_pin, float target_criticality,
        float pres_fac, float astar_fac, float bend_cost, t_rt_node* rt_root, t_rt_node** rt_node_of_sink, route_budgets &budgeting_inf,
        float max_delay, float min_delay, float target_delay, float short_path_crit, RouterStats& router_stats);

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
        float target_criticality, float astar_fac, route_budgets& budgeting_inf,
        float max_delay, float min_delay,
        float target_delay, float short_path_crit, RouterStats& router_stats);

static void timing_driven_expand_neighbours(t_heap *current,
        t_bb bounding_box, float bend_cost, float criticality_fac,
        int num_sinks, int target_node,
        float astar_fac, int highfanout_rlim, route_budgets &budgeting_inf, float max_delay, float min_delay,
        float target_delay, float short_path_crit, RouterStats& router_stats);

static void timing_driven_add_to_heap(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        const t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node, RouterStats& router_stats);

static void timing_driven_expand_node(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node);

static void timing_driven_expand_node_non_configurable_recurr(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node,
        std::set<int>& visited);

t_timing_driven_node_costs evaluate_timing_driven_node_costs(const t_timing_driven_node_costs old_costs,
    const float criticality_fac, const float bend_cost, const float astar_fac,
    const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
    const int from_node, const int to_node, const int iconn, const int target_node);

static float get_timing_driven_expected_cost(int inode, int target_node,
        float criticality_fac, float R_upstream);

static int get_expected_segs_to_target(int inode, int target_node,
        int *num_segs_ortho_dir_ptr);

static bool timing_driven_check_net_delays(vtr::vector_map<ClusterNetId, float *> &net_delay);

static int mark_node_expansion_by_bin(int source_node, int target_node,
        t_rt_node * rt_node, t_bb bounding_box, int num_sinks);

void reduce_budgets_if_congested(route_budgets &budgeting_inf,
        CBRR& connections_inf, float slope, int itry);

static bool should_route_net(ClusterNetId net_id, const CBRR& connections_inf, bool if_force_reroute);
static bool early_exit_heuristic(const WirelengthInfo& wirelength_info);

struct more_sinks_than {
    inline bool operator()(const ClusterNetId net_index1, const ClusterNetId net_index2) {
        auto& cluster_ctx = g_vpr_ctx.clustering();
        return cluster_ctx.clb_nlist.net_sinks(net_index1).size() > cluster_ctx.clb_nlist.net_sinks(net_index2).size();
    }
};

static WirelengthInfo calculate_wirelength_info();
static OveruseInfo calculate_overuse_info();

static void print_route_status_header();
static void print_route_status(int itry, double elapsed_sec, int num_bb_updated, const RouterStats& router_stats,
        const OveruseInfo& overuse_info, const WirelengthInfo& wirelength_info,
        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
        float est_success_iteration);
static void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision);
static int round_up(float x);

static std::string describe_unrouteable_connection(const int source_node, const int sink_node);

static bool is_high_fanout(int fanout);

static size_t dynamic_update_bounding_boxes();
static t_bb calc_current_bb(const t_trace* head);

static void enable_router_debug(const t_router_opts& router_opts, ClusterNetId net);

/************************ Subroutine definitions *****************************/
bool try_timing_driven_route(t_router_opts router_opts,
        vtr::vector_map<ClusterNetId, float *> &net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack * slacks,
        const t_timing_inf &timing_inf,
#endif
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

    /* Set delay of global signals to zero. Non-global net delays are set by
       update_net_delays_from_route_tree() inside timing_driven_route_net(),
       which is only called for non-global nets. */
	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_global(net_id)) {
            for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
                net_delay[net_id][ipin] = 0.;
            }
        }
    }

    CBRR connections_inf{};
    VTR_ASSERT_SAFE(connections_inf.sanity_check_lookup());

    route_budgets budgeting_inf;

    /*
     * Routing parameters
     */
    float pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */
    int bb_fac = router_opts.bb_factor;

    //When routing conflicts are detected the bounding boxes are scaled
    //by BB_SCALE_FACTOR every BB_SCALE_ITER_COUNT iterations
    constexpr float BB_SCALE_FACTOR = 2;
    constexpr int BB_SCALE_ITER_COUNT = 5; 

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
    for (itry = 1; itry <= router_opts.max_router_iterations; ++itry) {

        RouterStats router_iteration_stats;

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

        /*
         * Route each net
         */
        for (auto net_id : sorted_nets) {

            enable_router_debug(router_opts, net_id);

            bool is_routable = try_timing_driven_route_net(
                    net_id,
                    itry,
                    pres_fac,
                    router_opts,
                    connections_inf,
                    router_iteration_stats,
                    route_structs.pin_criticality,
                    route_structs.rt_node_of_sink,
                    net_delay,
                    netlist_pin_lookup,
                    route_timing_info, budgeting_inf);

            if (!is_routable) {
                return (false); //Impossible to route
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
        wirelength_info = calculate_wirelength_info();
        routing_predictor.add_iteration_overuse(itry, overuse_info.overused_nodes());

        if (timing_info) {
            //Update timing based on the new routing
            //Note that the net delays have already been updated by timing_driven_route_net
#ifdef ENABLE_CLASSIC_VPR_STA
            load_timing_graph_net_delays(net_delay);

            do_timing_analysis(slacks, timing_inf, false, true);
#endif

            timing_info->update();
            timing_info->set_warn_unconstrained(false); //Don't warn again about unconstrained nodes again during routing

            critical_path = timing_info->least_slack_critical_path();

#ifdef ENABLE_CLASSIC_VPR_STA
            float cpd_diff_ns = std::abs(get_critical_path_delay() - 1e9 * critical_path.delay());
            if (cpd_diff_ns > 0.01) {
                print_classic_cpds();
                print_tatum_cpds(timing_info->critical_paths());

                vpr_throw(VPR_ERROR_TIMING, __FILE__, __LINE__, "Classic VPR and Tatum critical paths do not match (%g and %g respectively)", get_critical_path_delay(), 1e9 * critical_path.delay());
            }
#endif
            VTR_ASSERT_SAFE(timing_driven_check_net_delays(net_delay));
        }

        float iter_cumm_time = iteration_timer.elapsed_sec();
        float iter_elapsed_time = iter_cumm_time - prev_iter_cumm_time;

        //Output progress
        print_route_status(itry, iter_elapsed_time, num_net_bounding_boxes_updated, router_iteration_stats, overuse_info, wirelength_info, timing_info, est_success_iteration);

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
        routing_is_successful = routing_is_feasible; //TODO: keep going after legal routing to further optimize timing
        if (routing_is_successful) {
            break; //Finished
        }

        /*
         * Abort checks: Should we give-up because this routing problem is unlikely to converge to a legal routing?
         */
        if (itry == 1 && early_exit_heuristic(wirelength_info)) {
            //Abort
            break;
        }

        //Estimate at what iteration we will converge to a legal routing
        if (overuse_info.overused_nodes() > ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD) {
            //Only abort if we have a significant number of overused resources

            if (!std::isnan(est_success_iteration) && est_success_iteration > abort_iteration_threshold) {
                VTR_LOG("Routing aborted, the predicted iteration for a successful route (%.1f) is too high.\n", est_success_iteration);
                break; //Abort
            }
        }

        /*
         * Prepare for the next iteration
         */

        if (router_opts.route_bb_update == e_route_bb_update::DYNAMIC) {
            num_net_bounding_boxes_updated = dynamic_update_bounding_boxes();
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
            pres_fac = min(pres_fac, static_cast<float> (HUGE_POSITIVE_FLOAT / 1e5));

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
                bb_fac *= BB_SCALE_FACTOR;

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
               net_delays stay as 0 so that wirelength can be optimized. */

            for (auto net_id : cluster_ctx.clb_nlist.nets()) {
                for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
#ifdef ENABLE_CLASSIC_VPR_STA
                    slacks->timing_criticality[(size_t)net_id][ipin] = 0.;
#endif
                    net_delay[net_id][ipin] = 0.;
                }
            }
        }


        if (router_opts.congestion_analysis) profiling::congestion_analysis();
        if (router_opts.fanout_analysis) profiling::time_on_fanout_analysis();
        // profiling::time_on_criticality_analysis();
    }

    if (routing_is_successful) {
        if (timing_info) {
            timing_driven_check_net_delays(net_delay);
            VTR_LOG("Completed net delay value cross check successfully.\n");
            VTR_LOG("Critical path: %g ns\n", 1e9 * critical_path.delay());
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

bool try_timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac,
        t_router_opts router_opts,
        CBRR& connections_inf,
        RouterStats& router_stats,
        float* pin_criticality,
        t_rt_node** rt_node_of_sink, vtr::vector_map<ClusterNetId, float *> &net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<SetupTimingInfo> timing_info, route_budgets &budgeting_inf) {

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    bool is_routed = false;

    connections_inf.prepare_routing_for_net(net_id);

    if (route_ctx.net_status[net_id].is_fixed) { /* Skip pre-routed nets. */
        is_routed = true;
    } else if (cluster_ctx.clb_nlist.net_is_global(net_id)) { /* Skip global nets. */
        is_routed = true;
    } else if (should_route_net(net_id, connections_inf, true) == false) {
        is_routed = true;
    } else {
        // track time spent vs fanout
        profiling::net_fanout_start();

        is_routed = timing_driven_route_net(net_id, itry, pres_fac,
                router_opts.max_criticality, router_opts.criticality_exp,
                router_opts.astar_fac, router_opts.bend_cost,
                connections_inf,
                router_stats,
                pin_criticality, router_opts.min_incremental_reroute_fanout,
                rt_node_of_sink,
                net_delay[net_id],
                netlist_pin_lookup,
                timing_info, budgeting_inf);

        profiling::net_fanout_end(cluster_ctx.clb_nlist.net_sinks(net_id).size());

        /* Impossible to route? (disconnected rr_graph) */
        if (is_routed) {
			route_ctx.net_status[net_id].is_routed = true;
        } else {
            VTR_LOG("Routing failed.\n");
        }
    }
    return (is_routed);
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void alloc_timing_driven_route_structs(float **pin_criticality_ptr,
        int **sink_order_ptr, t_rt_node *** rt_node_of_sink_ptr) {

    /* Allocates all the structures needed only by the timing-driven router.   */

    int max_pins_per_net = get_max_pins_per_net();
    int max_sinks = std::max(max_pins_per_net - 1, 0);

    float *pin_criticality = (float *) vtr::malloc(max_sinks * sizeof (float));
    *pin_criticality_ptr = pin_criticality - 1; /* First sink is pin #1. */

    int *sink_order = (int *) vtr::malloc(max_sinks * sizeof (int));
    *sink_order_ptr = sink_order - 1;

    t_rt_node **rt_node_of_sink = (t_rt_node **) vtr::malloc(max_sinks * sizeof (t_rt_node *));
    *rt_node_of_sink_ptr = rt_node_of_sink - 1;

    alloc_route_tree_timing_structs();
}

/*
 * NOTE:
 * Suggest using a timing_driven_route_structs struct. Memory is managed for you
 */
void free_timing_driven_route_structs(float *pin_criticality, int *sink_order,
        t_rt_node ** rt_node_of_sink) {

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
    alloc_timing_driven_route_structs(
            &pin_criticality,
            &sink_order,
            &rt_node_of_sink
            );
}

timing_driven_route_structs::~timing_driven_route_structs() {
    free_timing_driven_route_structs(
            pin_criticality,
            sink_order,
            rt_node_of_sink
            );
}

void reduce_budgets_if_congested(route_budgets &budgeting_inf,
        CBRR& connections_inf, float slope, int itry) {
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
        Must be more than 9 iterations to have a valid slope*/
        if (slope > CONGESTED_SLOPE_VAL && itry >= 9) {
            budgeting_inf.lower_budgets(1e-9);
        }
    }
}

int get_max_pins_per_net() {
	int max_pins_per_net = 0;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_global(net_id))
            max_pins_per_net = max(max_pins_per_net, (int)cluster_ctx.clb_nlist.net_pins(net_id).size());
    }

    return (max_pins_per_net);
}

struct Criticality_comp {
    const float* criticality;

    Criticality_comp(const float* calculated_criticalities) : criticality{calculated_criticalities}
    {
    }

    bool operator()(int a, int b) const {
        return criticality[a] > criticality[b];
    }
};

bool timing_driven_route_net(ClusterNetId net_id, int itry, float pres_fac, float max_criticality,
        float criticality_exp, float astar_fac, float bend_cost,
        CBRR& connections_inf,
        RouterStats& router_stats,
        float *pin_criticality, int min_incremental_reroute_fanout,
        t_rt_node ** rt_node_of_sink, float *net_delay,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        std::shared_ptr<const SetupTimingInfo> timing_info, route_budgets &budgeting_inf) {

    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    unsigned int num_sinks = cluster_ctx.clb_nlist.net_sinks(net_id).size();

    VTR_LOGV_DEBUG(f_router_debug, "Routing Net %zu (%zu sinks)\n", size_t(net_id), num_sinks);

    t_rt_node* rt_root = setup_routing_resources(itry, net_id, num_sinks, pres_fac, min_incremental_reroute_fanout, connections_inf, rt_node_of_sink);
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
            pin_criticality[ipin] = max(pin_criticality[ipin] - (1.0 - max_criticality), 0.0);

            /* Take pin criticality to some power (1 by default). */
            pin_criticality[ipin] = pow(pin_criticality[ipin], criticality_exp);

            /* Cut off pin criticality at max_criticality. */
            pin_criticality[ipin] = min(pin_criticality[ipin], max_criticality);
        } else {
            //No timing info, implies we want a min delay routing, so use criticality of 1.
            pin_criticality[ipin] = 1.;
        }
    }

    // compare the criticality of different sink nodes
    sort(begin(remaining_targets), end(remaining_targets), Criticality_comp{pin_criticality});

    /* Update base costs according to fanout and criticality rules */
    update_rr_base_costs(num_sinks);

    // explore in order of decreasing criticality (no longer need sink_order array)
    for (unsigned itarget = 0; itarget < remaining_targets.size(); ++itarget) {
        int target_pin = remaining_targets[itarget];
        float target_criticality = pin_criticality[target_pin];

        float max_delay, min_delay, target_delay, short_path_crit;

        if (budgeting_inf.if_set()) {
            max_delay = budgeting_inf.get_max_delay_budget(net_id, target_pin);
            target_delay = budgeting_inf.get_delay_target(net_id, target_pin);
            min_delay = budgeting_inf.get_min_delay_budget(net_id, target_pin);
            short_path_crit = budgeting_inf.get_crit_short_path(net_id, target_pin);
        } else {
            /*No budgets so do not add/subtract any value to the cost function*/
            max_delay = 0;
            target_delay = 0;
            min_delay = 0;
            short_path_crit = 0;
        }

        // build a branch in the route tree to the target
        if (!timing_driven_route_sink(itry, net_id, itarget, target_pin, target_criticality,
                pres_fac, astar_fac, bend_cost, rt_root, rt_node_of_sink, budgeting_inf,
                max_delay, min_delay, target_delay, short_path_crit, router_stats))
            return false;

        // need to guarentee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
        // do this by resetting all the path_costs that have been touched while routing to the current sink
        reset_path_costs();

        ++router_stats.connections_routed;
    } // finished all sinks

    ++router_stats.nets_routed;

    /* For later timing analysis. */

    // may have to update timing delay of the previously legally reached sinks since downstream capacitance could be changed
    update_net_delays_from_route_tree(net_delay, rt_node_of_sink, net_id);

    if (!cluster_ctx.clb_nlist.net_is_global(net_id)) {
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

static bool timing_driven_route_sink(int itry, ClusterNetId net_id, unsigned itarget, int target_pin, float target_criticality,
        float pres_fac, float astar_fac, float bend_cost, t_rt_node* rt_root, t_rt_node** rt_node_of_sink, route_budgets &budgeting_inf,
        float max_delay, float min_delay, float target_delay, float short_path_crit, RouterStats& router_stats) {

    /* Build a path from the existing route tree rooted at rt_root to the target_node
     * add this branch to the existing route tree and update pathfinder costs and rr_node_route_inf to reflect this */
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    profiling::sink_criticality_start();

    int source_node = route_ctx.net_rr_terminals[net_id][0];
    int sink_node = route_ctx.net_rr_terminals[net_id][target_pin];

    t_bb bounding_box = route_ctx.route_bb[net_id];

    VTR_LOGV_DEBUG(f_router_debug, "Net %zu Target %d\n", size_t(net_id), itarget);

    if (itarget > 0 && itry > 5) {
        /* Enough iterations given to determine opin, to speed up legal solution, do not let net use two opins */
        VTR_ASSERT(device_ctx.rr_nodes[rt_root->inode].type() == SOURCE);
        rt_root->re_expand = false;
    }

    VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace_head[net_id], rt_root));

    std::vector<int> modified_rr_node_inf;

    t_heap * cheapest = timing_driven_route_connection(source_node, sink_node, target_criticality,
            astar_fac, bend_cost, rt_root, bounding_box, (int)cluster_ctx.clb_nlist.net_sinks(net_id).size(), budgeting_inf,
            max_delay, min_delay, target_delay, short_path_crit, modified_rr_node_inf, router_stats);

    if (cheapest == nullptr) {
		ClusterBlockId src_block = cluster_ctx.clb_nlist.net_driver_block(net_id);
		ClusterBlockId sink_block = cluster_ctx.clb_nlist.pin_block(*(cluster_ctx.clb_nlist.net_pins(net_id).begin() + target_pin));
        VTR_LOG("Failed to route connection from '%s' to '%s' for net '%s'\n",
                    cluster_ctx.clb_nlist.block_name(src_block).c_str(),
					cluster_ctx.clb_nlist.block_name(sink_block).c_str(),
                    cluster_ctx.clb_nlist.net_name(net_id).c_str());
        return false;
    }

    profiling::sink_criticality_end(target_criticality);

    /* NB:  In the code below I keep two records of the partial routing:  the   *
     * traceback and the route_tree.  The route_tree enables fast recomputation *
     * of the Elmore delay to each node in the partial routing.  The traceback  *
     * lets me reuse all the routines written for breadth-first routing, which  *
     * all take a traceback structure as input.                                 */

    int inode = cheapest->index;
    route_ctx.rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
    t_trace* new_route_start_tptr = update_traceback(cheapest, net_id);
    VTR_ASSERT_DEBUG(validate_traceback(route_ctx.trace_head[net_id]));

    rt_node_of_sink[target_pin] = update_route_tree(cheapest);
    VTR_ASSERT_DEBUG(verify_route_tree(rt_root));
    VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace_head[net_id], rt_root));

    free_heap_data(cheapest);
    pathfinder_update_path_cost(new_route_start_tptr, 1, pres_fac);
    empty_heap();
    reset_path_costs(modified_rr_node_inf);

    // routed to a sink successfully
    return true;
}

t_heap * timing_driven_route_connection(int source_node, int sink_node, float target_criticality,
        float astar_fac, float bend_cost, t_rt_node* rt_root, t_bb bounding_box, int num_sinks, route_budgets &budgeting_inf,
        float max_delay, float min_delay, float target_delay, float short_path_crit, std::vector<int>& modified_rr_node_inf,
        RouterStats& router_stats) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    int highfanout_rlim = mark_node_expansion_by_bin(source_node, sink_node, rt_root, bounding_box, num_sinks);

    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    add_route_tree_to_heap(rt_root, sink_node, target_criticality, astar_fac, budgeting_inf,
            max_delay, min_delay, target_delay, short_path_crit, router_stats);
    heap_::build_heap(); // via sifting down everything

    VTR_ASSERT_SAFE(heap_::is_valid());

    // cheapest s_heap (gives index to device_ctx.rr_nodes) in current route tree to be expanded on
    t_heap * cheapest{get_heap_head()};
    ++router_stats.heap_pops;

    if (cheapest == nullptr) {
        VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

        reset_path_costs();
        free_route_tree(rt_root);
        return nullptr;
    }


    int inode = cheapest->index;
    VTR_LOGV_DEBUG(f_router_debug, "  Popping node %d\n", inode);
    while (inode != sink_node) {
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
            for (t_heap_prev prev : cheapest->previous) {
                VTR_LOGV_DEBUG(f_router_debug, "      Setting path costs for assicated node %d (from %d edge %d)\n", prev.to_node, prev.from_node, prev.from_edge);

                add_to_mod_list(prev.to_node, modified_rr_node_inf);

                route_ctx.rr_node_route_inf[prev.to_node].prev_node = prev.from_node;
                route_ctx.rr_node_route_inf[prev.to_node].prev_edge = prev.from_edge;
                route_ctx.rr_node_route_inf[prev.to_node].path_cost = new_total_cost;
                route_ctx.rr_node_route_inf[prev.to_node].backward_path_cost = new_back_cost;
            }

            timing_driven_expand_neighbours(cheapest, bounding_box, bend_cost,
                    target_criticality, num_sinks, sink_node, astar_fac,
                    highfanout_rlim, budgeting_inf, max_delay, min_delay,
                    target_delay, short_path_crit, router_stats);
        }

        free_heap_data(cheapest);
        cheapest = get_heap_head();
        ++router_stats.heap_pops;

        if (cheapest == nullptr) { /* Impossible routing.  No path for net. */
            VTR_LOG("%s\n", describe_unrouteable_connection(source_node, sink_node).c_str());

            reset_path_costs();
            free_route_tree(rt_root);
            return nullptr;
        }

        inode = cheapest->index;
        VTR_LOGV_DEBUG(f_router_debug, "  Popping node %d\n", inode);
    }
    VTR_LOGV_DEBUG(f_router_debug, "  Found target %d\n", inode);

    return cheapest;
}

static t_rt_node* setup_routing_resources(int itry, ClusterNetId net_id, unsigned num_sinks, float pres_fac, int min_incremental_reroute_fanout,
        CBRR& connections_inf, t_rt_node** rt_node_of_sink) {

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
    if ((int) num_sinks < min_incremental_reroute_fanout || itry == 1) {

        profiling::net_rerouted();

        // rip up the whole net
        pathfinder_update_path_cost(route_ctx.trace_head[net_id], -1, pres_fac);
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
        VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace_head[net_id], rt_root));

        // check for edge correctness
        VTR_ASSERT_SAFE(is_valid_skeleton_tree(rt_root));
        VTR_ASSERT_SAFE(should_route_net(net_id, connections_inf, true));

        //Prune the branches of the tree that don't legally lead to sinks
        rt_root = prune_route_tree(rt_root, connections_inf);

        //Now that the tree has been pruned, we can free the old traceback
        // NOTE: this must happen *after* pruning since it changes the
        //       recorded congestion
        pathfinder_update_path_cost(route_ctx.trace_head[net_id], -1, pres_fac);
        free_traceback(net_id);

        if (rt_root) { //Partially pruned
            profiling::route_tree_preserved();

            //Since we have a valid partial routing (to at least one SINK)
            //we need to make sure the traceback is synchronized to the route tree
            traceback_from_route_tree(net_id, rt_root, reached_rt_sinks.size());

            //Sanity check the traceback for self-consistency
            VTR_ASSERT_DEBUG(validate_traceback(route_ctx.trace_head[net_id]));

            //Santiy check that route tree and traceback are equivalent after pruning
            VTR_ASSERT_DEBUG(verify_traceback_route_tree_equivalent(route_ctx.trace_head[net_id], rt_root));

            // put the updated costs of the route tree nodes back into pathfinder
            pathfinder_update_path_cost(route_ctx.trace_head[net_id], 1, pres_fac);

        } else { //Fully destroyed
            profiling::route_tree_pruned();

            //Initialize only to source
            rt_root = init_route_tree_to_source(net_id);

            //NOTE: We leave the traceback uninitiailized, so update_traceback()
            //      will correctly add the SOURCE node when the branch to
            //      the first SINK is found.
            VTR_ASSERT(route_ctx.trace_head[net_id] == nullptr);
            VTR_ASSERT(route_ctx.trace_tail[net_id] == nullptr);
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

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
        float target_criticality, float astar_fac, route_budgets& budgeting_inf,
        float max_delay, float min_delay,
        float target_delay, float short_path_crit, RouterStats& router_stats) {

    /* Puts the entire partial routing below and including rt_node onto the heap *
     * (except for those parts marked as not to be expanded) by calling itself   *
     * recursively.                                                              */

    int inode;
    t_rt_node *child_node;
    t_linked_rt_edge *linked_rt_edge;
    float tot_cost, backward_path_cost, R_upstream;

    /* Pre-order depth-first traversal */
    // IPINs and SINKS are not re_expanded
    if (rt_node->re_expand) {
        inode = rt_node->inode;
        backward_path_cost = target_criticality * rt_node->Tdel;

        R_upstream = rt_node->R_upstream;
        tot_cost = backward_path_cost
                + astar_fac
                * get_timing_driven_expected_cost(inode, target_node,
                target_criticality, R_upstream);

        float zero = 0.0;
        //after budgets are loaded, calculate delay cost as described by RCV paper
        /*R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
         * Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
         * Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.*/
        if (budgeting_inf.if_set()) {
            tot_cost += (short_path_crit + target_criticality) * max(zero, target_delay - tot_cost);
            tot_cost += pow(max(zero, tot_cost - max_delay), 2) / 100e-12;
            tot_cost += pow(max(zero, min_delay - tot_cost), 2) / 100e-12;
        }

        VTR_LOGV_DEBUG(f_router_debug, "Adding node %d to heap from init route tree\n", inode);

        heap_::push_back_node(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
                backward_path_cost, R_upstream);

        ++router_stats.heap_pushes;
    }

    linked_rt_edge = rt_node->u.child_list;

    while (linked_rt_edge != nullptr) {
        child_node = linked_rt_edge->child;
        add_route_tree_to_heap(child_node, target_node, target_criticality,
                astar_fac, budgeting_inf, max_delay, min_delay,
                target_delay, short_path_crit, router_stats);
        linked_rt_edge = linked_rt_edge->next;
    }
}

static void timing_driven_expand_neighbours(t_heap *current,
        t_bb bounding_box, float bend_cost, float criticality_fac,
        int num_sinks, int target_node,
        float astar_fac, int highfanout_rlim, route_budgets& budgeting_inf, float max_delay, float min_delay,
        float target_delay, float short_path_crit, RouterStats& router_stats) {

    /* Puts all the rr_nodes adjacent to current on the heap.  rr_nodes outside *
     * the expanded bounding box specified in bounding_box are not added to the     *
     * heap.                                                                    */

    auto& device_ctx = g_vpr_ctx.device();

    int inode = current->index;

    int target_xlow = device_ctx.rr_nodes[target_node].xlow();
    int target_ylow = device_ctx.rr_nodes[target_node].ylow();
    int target_xhigh = device_ctx.rr_nodes[target_node].xhigh();
    int target_yhigh = device_ctx.rr_nodes[target_node].yhigh();

    bool high_fanout = is_high_fanout(num_sinks);

    int num_edges = device_ctx.rr_nodes[inode].num_edges();
    for (int iconn = 0; iconn < num_edges; iconn++) {
        int to_node = device_ctx.rr_nodes[inode].edge_sink_node(iconn);

        int to_xlow = device_ctx.rr_nodes[to_node].xlow();
        int to_ylow = device_ctx.rr_nodes[to_node].ylow();
        int to_xhigh = device_ctx.rr_nodes[to_node].xhigh();
        int to_yhigh = device_ctx.rr_nodes[to_node].yhigh();

        if (high_fanout) {
            if (   to_xhigh < target_xhigh - highfanout_rlim
                || to_xlow > target_xlow + highfanout_rlim
                || to_yhigh < target_yhigh - highfanout_rlim
                || to_ylow > target_ylow + highfanout_rlim) {
                continue; /* Node is outside high fanout bin. */
            }
        } else if (to_xhigh < bounding_box.xmin //Strictly left of BB left-edge
                   || to_xlow > bounding_box.xmax //Strictly right of BB right-edge
                   || to_yhigh < bounding_box.ymin //Strictly below BB bottom-edge
                   || to_ylow > bounding_box.ymax) { //Strictly above BB top-edge
            continue; /* Node is outside (expanded) bounding box. */
        }


        /* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
         * the issue of how to cost them properly so they don't get expanded before *
         * more promising routes, but makes route-throughs (via CLBs) impossible.   *
         * Change this if you want to investigate route-throughs.                   */
        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if (to_type == IPIN) {
            //Check if this IPIN leads to the target block
            // IPIN's of the target block should be contained within it's bounding box
            if (   to_xlow < target_xlow
                || to_ylow < target_ylow
                || to_xhigh > target_xhigh
                || to_yhigh > target_yhigh) {
                continue;
            }
        }

        timing_driven_add_to_heap(criticality_fac, bend_cost, astar_fac,
                budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
                current, inode, to_node, iconn, target_node, router_stats);
    } /* End for all neighbours */
}

//Add to_node to the heap, and also add any nodes which are connected by non-configurable edges
static void timing_driven_add_to_heap(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        const t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node, RouterStats& router_stats) {

    t_heap* next = alloc_heap_data();
    next->index = to_node;

    //Costs initialized to current
    next->cost = std::numeric_limits<float>::infinity(); //Not used directly
    next->backward_path_cost = current->backward_path_cost;
    next->R_upstream = current->R_upstream;

    auto& device_ctx = g_vpr_ctx.device();
    if (device_ctx.rr_nodes[to_node].num_non_configurable_edges() == 0) {
        //The common case where there are no non-configurable edges
        timing_driven_expand_node(criticality_fac, bend_cost, astar_fac,
                budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
                next, from_node, to_node, iconn, target_node);
    } else {
        //The 'to_node' which we just expanded to has non-configurable
        //out-going edges which must also be expanded.
        //
        //Note that we only call the recursive version if there *are* non-configurable
        //edges since creating/destroying the 'visited' tracker is very expensive (since
        //it is in the router's inner loop). Since non-configurable edges are relatively
        //rare this is reasonable.
        //
        //TODO: use a more efficient method of tracking visited nodes (e.g. if
        //      non-configurable edges become more common)
        std::set<int> visited;
        timing_driven_expand_node_non_configurable_recurr(criticality_fac, bend_cost, astar_fac,
                budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
                next, from_node, to_node, iconn, target_node,
                visited);
    }

    add_to_heap(next);
    ++router_stats.heap_pushes;
}

//Updates current (path step and costs) to account for the step taken to reach to_node
static void timing_driven_expand_node(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node) {


#ifdef VTR_ENABLE_DEBUG_LOGGING
    if (f_router_debug) {
        auto& device_ctx = g_vpr_ctx.device();
        bool reached_via_non_configurable_edge = !device_ctx.rr_nodes[from_node].edge_is_configurable(iconn);
        if (reached_via_non_configurable_edge) {
            VTR_LOG("        Force Expanding to node %d", to_node);
        } else {
            VTR_LOG("      Expanding to node %d", to_node);
        }
        VTR_LOG("\n");
    }
#endif

    t_timing_driven_node_costs old_costs;
    old_costs.backward_cost = current->backward_path_cost;
    old_costs.total_cost = current->cost;
    old_costs.R_upstream = current->R_upstream;

    auto new_costs = evaluate_timing_driven_node_costs(old_costs,
                        criticality_fac, bend_cost, astar_fac,
                        budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
                        from_node, to_node, iconn, target_node);

    //Record how we reached this node
    current->previous.emplace_back(to_node, from_node, iconn);

    //Since this heap element may represent multiple (non-configurably connected) nodes,
    //keep the minimum cost to the target
    if (new_costs.total_cost < current->cost) {
        current->cost = new_costs.total_cost;
        current->backward_path_cost = new_costs.backward_cost;
        current->R_upstream = new_costs.R_upstream;
        current->index = to_node;
    }
}

//Updates current (path stage and costs) to account for the step taken to reach to_node,
//and any of it's non-configurably connected nodes
static void timing_driven_expand_node_non_configurable_recurr(const float criticality_fac, const float bend_cost, const float astar_fac,
        const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
        t_heap* current, const int from_node, const int to_node, const int iconn, const int target_node,
        std::set<int>& visited
        ) {

    VTR_ASSERT(current);

    if (visited.count(to_node)) {
        return;
    }

    visited.insert(to_node);

    auto& device_ctx = g_vpr_ctx.device();

    timing_driven_expand_node(criticality_fac, bend_cost, astar_fac,
            budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
            current, from_node, to_node, iconn, target_node);

    //Consider any non-configurable edges which must be expanded for correctness
    for (int iconn_next : device_ctx.rr_nodes[to_node].non_configurable_edges()) {
        VTR_ASSERT_SAFE(!device_ctx.rr_nodes[to_node].edge_is_configurable(iconn_next)); //Forced expansion

        int to_to_node = device_ctx.rr_nodes[to_node].edge_sink_node(iconn_next);

        timing_driven_expand_node_non_configurable_recurr(criticality_fac, bend_cost, astar_fac,
                budgeting_inf, max_delay, min_delay, target_delay, short_path_crit,
                current, to_node, to_to_node, iconn_next, target_node, visited);
    }
}

//Calculates the cost of reaching to_node
t_timing_driven_node_costs evaluate_timing_driven_node_costs(const t_timing_driven_node_costs old_costs,
    const float criticality_fac, const float bend_cost, const float astar_fac,
    const route_budgets& budgeting_inf, const float max_delay, const float min_delay, const float target_delay, const float short_path_crit,
    const int from_node, const int to_node, const int iconn, const int target_node) {
    /* new_costs.backward_cost: is the "known" part of the cost to this node -- the
     * congestion cost of all the routing resources back to the existing route
     * plus the known delay of the total path back to the source.
     *
     * new_costs.total_cost: is this "known" backward cost + an expected cost to get to the target.
     *
     * new_costs.R_upstream: is the upstream resistance at the end of this node
     */
    auto& device_ctx = g_vpr_ctx.device();

    t_timing_driven_node_costs new_costs;

    //Switch info
    int iswitch = device_ctx.rr_nodes[from_node].edge_switch(iconn);
    bool switch_buffered = device_ctx.rr_switch_inf[iswitch].buffered();
    float switch_R = device_ctx.rr_switch_inf[iswitch].R;
    float switch_Tdel = device_ctx.rr_switch_inf[iswitch].Tdel;

    //Node info
    float node_C = device_ctx.rr_nodes[to_node].C();
    float node_R = device_ctx.rr_nodes[to_node].R();

    //Update R_upstream
    if (switch_buffered) {
        new_costs.R_upstream = 0.; //No upstream resistance
    } else {
        new_costs.R_upstream = old_costs.R_upstream; //Upstream resistance
    }
    new_costs.R_upstream += switch_R; //Switch resistance
    new_costs.R_upstream += node_R; //Node resistance

    //Calculate delay
    float Rdel = new_costs.R_upstream - 0.5 * node_R; //Only consider half node's resistance for delay
    float Tdel = switch_Tdel + Rdel * node_C;

    //Update the backward cost
    new_costs.backward_cost = old_costs.backward_cost; //Back cost to 'from_node'
    new_costs.backward_cost += (1. - criticality_fac) * get_rr_cong_cost(to_node); //Congestion cost
    new_costs.backward_cost += criticality_fac * Tdel; //Delay cost
    if (bend_cost != 0.) {
        t_rr_type from_type = device_ctx.rr_nodes[from_node].type();
        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if ((from_type == CHANX && to_type == CHANY) || (from_type == CHANY && to_type == CHANX)) {
            new_costs.backward_cost += bend_cost; //Bend cost
        }
    }

    if (budgeting_inf.if_set()) {
        //If budgets specified calculate cost as described by RCV paper:
        //    R. Fung, V. Betz and W. Chow, "Slack Allocation and Routing to Improve FPGA Timing While
        //     Repairing Short-Path Violations," in IEEE Transactions on Computer-Aided Design of
        //     Integrated Circuits and Systems, vol. 27, no. 4, pp. 686-697, April 2008.

        //TODO: Since these targets are delays, shouldn't we be using Tdel instead of new_costs.total_cost on RHS?
        new_costs.total_cost += (short_path_crit + criticality_fac) * max(0.f, target_delay - new_costs.total_cost);
        new_costs.total_cost += pow(max(0.f, new_costs.total_cost - max_delay), 2) / 100e-12;
        new_costs.total_cost += pow(max(0.f, min_delay - new_costs.total_cost), 2) / 100e-12;
    }

    //Update total cost
    float expected_cost = get_timing_driven_expected_cost(to_node, target_node, criticality_fac, new_costs.R_upstream);
    new_costs.total_cost = new_costs.backward_cost + astar_fac * expected_cost;

    return new_costs;
}

static float get_timing_driven_expected_cost(int inode, int target_node, float criticality_fac, float R_upstream) {

    /* Determines the expected cost (due to both delay and resource cost) to reach *
     * the target node from inode.  It doesn't include the cost of inode --       *
     * that's already in the "known" path_cost.                                   */

    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type = device_ctx.rr_nodes[inode].type();

    if (rr_type == CHANX || rr_type == CHANY) {

#ifdef USE_MAP_LOOKAHEAD
        return get_lookahead_map_cost(inode, target_node, criticality_fac);
#else
        int num_segs_ortho_dir = 0;
        int num_segs_same_dir = get_expected_segs_to_target(inode, target_node, &num_segs_ortho_dir);

        int cost_index = device_ctx.rr_nodes[inode].cost_index();
        int ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;

        const auto& same_data = device_ctx.rr_indexed_data[cost_index];
        const auto& ortho_data = device_ctx.rr_indexed_data[ortho_cost_index];
        const auto& ipin_data = device_ctx.rr_indexed_data[IPIN_COST_INDEX];
        const auto& sink_data = device_ctx.rr_indexed_data[SINK_COST_INDEX];

        float cong_cost =   num_segs_same_dir * same_data.base_cost
                          + num_segs_ortho_dir * ortho_data.base_cost
                          + ipin_data.base_cost
                          + sink_data.base_cost;

        float Tdel =   num_segs_same_dir * same_data.T_linear
                     + num_segs_ortho_dir * ortho_data.T_linear
                     + num_segs_same_dir * num_segs_same_dir * same_data.T_quadratic
                     + num_segs_ortho_dir * num_segs_ortho_dir * ortho_data.T_quadratic
                     + R_upstream * (  num_segs_same_dir * same_data.C_load
                                     + num_segs_ortho_dir * ortho_data.C_load)
                     + ipin_data.T_linear;

        float expected_cost = criticality_fac * Tdel + (1. - criticality_fac) * cong_cost;
        return (expected_cost);
#endif
    } else if (rr_type == IPIN) { /* Change if you're allowing route-throughs */
        return (device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost);
    } else { /* Change this if you want to investigate route-throughs */
        return (0.);
    }
}

/* Used below to ensure that fractions are rounded up, but floating   *
 * point values very close to an integer are rounded to that integer.       */
static int round_up(float x) {
    return std::ceil(x - 0.001);
}

static int get_expected_segs_to_target(int inode, int target_node,
        int *num_segs_ortho_dir_ptr) {

    /* Returns the number of segments the same type as inode that will be needed *
     * to reach target_node (not including inode) in each direction (the same    *
     * direction (horizontal or vertical) as inode and the orthogonal direction).*/

    auto& device_ctx = g_vpr_ctx.device();

    t_rr_type rr_type;
    int target_x, target_y, num_segs_same_dir, cost_index, ortho_cost_index;
    int no_need_to_pass_by_clb;
    float inv_length, ortho_inv_length, ylow, yhigh, xlow, xhigh;


    target_x = device_ctx.rr_nodes[target_node].xlow();
    target_y = device_ctx.rr_nodes[target_node].ylow();
    cost_index = device_ctx.rr_nodes[inode].cost_index();
    inv_length = device_ctx.rr_indexed_data[cost_index].inv_length;
    ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;
    ortho_inv_length = device_ctx.rr_indexed_data[ortho_cost_index].inv_length;
    rr_type = device_ctx.rr_nodes[inode].type();

    if (rr_type == CHANX) {
        ylow = device_ctx.rr_nodes[inode].ylow();
        xhigh = device_ctx.rr_nodes[inode].xhigh();
        xlow = device_ctx.rr_nodes[inode].xlow();

        /* Count vertical (orthogonal to inode) segs first. */

        if (ylow > target_y) { /* Coming from a row above target? */
            *num_segs_ortho_dir_ptr = round_up((ylow - target_y + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (ylow < target_y - 1) { /* Below the CLB bottom? */
            *num_segs_ortho_dir_ptr = round_up((target_y - ylow) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a row that passes by target CLB */
            *num_segs_ortho_dir_ptr = 0;
            no_need_to_pass_by_clb = 0;
        }


        /* Now count horizontal (same dir. as inode) segs. */

        if (xlow > target_x + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((xlow - no_need_to_pass_by_clb -
                    target_x) * inv_length);
        } else if (xhigh < target_x - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_x - no_need_to_pass_by_clb -
                    xhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }
    } else { /* inode is a CHANY */
        ylow = device_ctx.rr_nodes[inode].ylow();
        yhigh = device_ctx.rr_nodes[inode].yhigh();
        xlow = device_ctx.rr_nodes[inode].xlow();

        /* Count horizontal (orthogonal to inode) segs first. */

        if (xlow > target_x) { /* Coming from a column right of target? */
            *num_segs_ortho_dir_ptr = round_up((xlow - target_x + 1.) * ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else if (xlow < target_x - 1) { /* Left of and not adjacent to the CLB? */
            *num_segs_ortho_dir_ptr = round_up((target_x - xlow) *
                    ortho_inv_length);
            no_need_to_pass_by_clb = 1;
        } else { /* In a column that passes by target CLB */
            *num_segs_ortho_dir_ptr = 0;
            no_need_to_pass_by_clb = 0;
        }

        /* Now count vertical (same dir. as inode) segs. */

        if (ylow > target_y + no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((ylow - no_need_to_pass_by_clb -
                    target_y) * inv_length);
        } else if (yhigh < target_y - no_need_to_pass_by_clb) {
            num_segs_same_dir = round_up((target_y - no_need_to_pass_by_clb -
                    yhigh) * inv_length);
        } else {
            num_segs_same_dir = 0;
        }

    }

    return (num_segs_same_dir);
}

void update_rr_base_costs(int fanout) {

    /* Changes the base costs of different types of rr_nodes according to the  *
     * criticality, fanout, etc. of the current net being routed (net_id).       */
    auto& device_ctx = g_vpr_ctx.device();

    float factor;
    int index;

    /* Other reasonable values for factor include fanout and 1 */
    factor = sqrt(fanout);

    for (index = CHANX_COST_INDEX_START; index < device_ctx.num_rr_indexed_data; index++) {
        if (device_ctx.rr_indexed_data[index].T_quadratic > 0.) { /* pass transistor */
            device_ctx.rr_indexed_data[index].base_cost =
                    device_ctx.rr_indexed_data[index].saved_base_cost * factor;
        } else {
            device_ctx.rr_indexed_data[index].base_cost =
                    device_ctx.rr_indexed_data[index].saved_base_cost;
        }
    }
}

/* Nets that have high fanout can take a very long time to route. Routing for sinks that are part of high-fanout nets should be
   done within a rectangular 'bin' centered around a target (as opposed to the entire net bounding box) the size of which is returned by this function. */
static int mark_node_expansion_by_bin(int source_node, int target_node,
        t_rt_node * rt_node, t_bb bounding_box, int num_sinks) {

    auto& device_ctx = g_vpr_ctx.device();

    int target_xlow, target_ylow, target_xhigh, target_yhigh;
    int rlim = 1;
    int max_grid_dim = max(device_ctx.grid.width(), device_ctx.grid.height());
    int inode;
    float area;
    bool success;
    t_linked_rt_edge *linked_rt_edge;
    t_rt_node * child_node;

    target_xlow = device_ctx.rr_nodes[target_node].xlow();
    target_ylow = device_ctx.rr_nodes[target_node].ylow();
    target_xhigh = device_ctx.rr_nodes[target_node].xhigh();
    target_yhigh = device_ctx.rr_nodes[target_node].yhigh();

    if (!is_high_fanout(num_sinks)) {
        /* This algorithm only applies to high fanout nets */
        return 1;
    }
    if (rt_node == nullptr || rt_node->u.child_list == nullptr) {
        /* If unknown traceback, set radius of bin to be size of chip */
        rlim = max_grid_dim;
        return rlim;
    }

    area = (bounding_box.xmax - bounding_box.xmin)
            * (bounding_box.ymax - bounding_box.ymin);
    if (area <= 0) {
        area = 1;
    }

    VTR_ASSERT(num_sinks > 0);

    rlim = (int) (ceil(sqrt((float) area / (float) num_sinks)));

    success = false;
    /* determine quickly a feasible bin radius to route sink for high fanout nets
     this is necessary to prevent super long runtimes for high fanout nets; in best case, a reduction in complexity from O(N^2logN) to O(NlogN) (Swartz fast router)
     */
    linked_rt_edge = rt_node->u.child_list;
    while (success == false && linked_rt_edge != nullptr) {
        while (linked_rt_edge != nullptr && success == false) {
            child_node = linked_rt_edge->child;
            inode = child_node->inode;
            if (!(device_ctx.rr_nodes[inode].type() == IPIN || device_ctx.rr_nodes[inode].type() == SINK)) {
                if (device_ctx.rr_nodes[inode].xlow() <= target_xhigh + rlim
                        && device_ctx.rr_nodes[inode].xhigh() >= target_xhigh - rlim
                        && device_ctx.rr_nodes[inode].ylow() <= target_yhigh + rlim
                        && device_ctx.rr_nodes[inode].yhigh() >= target_yhigh - rlim) {
                    success = true;
                }
            }
            linked_rt_edge = linked_rt_edge->next;
        }

        if (success == false) {
            if (rlim > max_grid_dim) {
                vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                        "VPR internal error, the source node %d has paths that are not found in traceback.\n", source_node);
            }
            /* if sink not in bin, increase bin size until fit */
            rlim *= 2;
        } else {
            /* Sometimes might just catch a wire in the end segment, need to give it some channel space to explore */
            rlim += 4;
        }
        linked_rt_edge = rt_node->u.child_list;
    }

    /* adjust rlim to account for width/height of block containing the target sink */
    int target_span = max(target_xhigh - target_xlow, target_yhigh - target_ylow);
    rlim += target_span;

    /* redetermine expansion based on rlim */
    linked_rt_edge = rt_node->u.child_list;
    while (linked_rt_edge != nullptr) {
        child_node = linked_rt_edge->child;
        inode = child_node->inode;
        if (!(device_ctx.rr_nodes[inode].type() == IPIN || device_ctx.rr_nodes[inode].type() == SINK)) {
            if (device_ctx.rr_nodes[inode].xlow() <= target_xhigh + rlim
                    && device_ctx.rr_nodes[inode].xhigh() >= target_xhigh - rlim
                    && device_ctx.rr_nodes[inode].ylow() <= target_yhigh + rlim
                    && device_ctx.rr_nodes[inode].yhigh() >= target_yhigh - rlim) {
                child_node->re_expand = true;
            } else {
                child_node->re_expand = false;
            }
        }
        linked_rt_edge = linked_rt_edge->next;
    }
    return rlim;
}

static bool timing_driven_check_net_delays(vtr::vector_map<ClusterNetId, float *> &net_delay) {
    constexpr float ERROR_TOL = 0.0001;

    /* Checks that the net delays computed incrementally during timing driven    *
     * routing match those computed from scratch by the net_delay.c module.      */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    unsigned int ipin;
	vtr::vector_map<ClusterNetId, float *> net_delay_check;

    vtr::t_chunk list_head_net_delay_check_ch;

    net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch);
    load_net_delay_from_routing(net_delay_check);

	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            if (net_delay_check[net_id][ipin] == 0.) { /* Should be only GLOBAL nets */
                if (fabs(net_delay[net_id][ipin]) > ERROR_TOL) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "in timing_driven_check_net_delays: net %lu pin %d.\n"
                            "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
							size_t(net_id), ipin, net_delay[net_id][ipin], net_delay_check[net_id][ipin]);
                }
            } else {
                float error = fabs(1.0 - net_delay[net_id][ipin] / net_delay_check[net_id][ipin]);
                if (error > ERROR_TOL) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
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
static bool should_route_net(ClusterNetId net_id, const CBRR& connections_inf, bool if_force_reroute) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

    t_trace * tptr = route_ctx.trace_head[net_id];

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

    return false; /* Current route has no overuse */
}

static bool early_exit_heuristic(const WirelengthInfo& wirelength_info) {
    /* Early exit code for cases where it is obvious that a successful route will not be found
       Heuristic: If total wirelength used in first routing iteration is X% of total available wirelength, exit */

    if (wirelength_info.used_wirelength_ratio() > FIRST_ITER_WIRELENTH_LIMIT) {
        VTR_LOG("Wire length usage ratio %g exceeds limit of %g, fail routing.\n",
                wirelength_info.used_wirelength_ratio(),
                FIRST_ITER_WIRELENTH_LIMIT);
        return true;
    }
    return false;
}

// incremental rerouting resources class definitions
Connection_based_routing_resources::Connection_based_routing_resources() :
current_inet(NO_PREVIOUS), // not routing to a specific net yet (note that NO_PREVIOUS is not unsigned, so will be largest unsigned)
last_stable_critical_path_delay{0.0f},
critical_path_growth_tolerance{1.001f},
connection_criticality_tolerance{0.9f},
connection_delay_optimality_tolerance{1.1f}
{

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
        net_node_to_pin.reserve(num_pins - 1); // not looking up on the SOURCE pin
        net_lower_bound_connection_delay.reserve(num_pins - 1); // will be filled in after the 1st iteration's
        net_forcible_reroute_connection_flag.reserve(num_pins - 1); // all false to begin with

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[net_id][ipin];

            net_node_to_pin.insert({rr_sink_node, ipin});
            net_forcible_reroute_connection_flag.insert({rr_sink_node, false});
        }
    }
}

void Connection_based_routing_resources::convert_sink_nodes_to_net_pins(vector<int>& rr_sink_nodes) const {

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

void Connection_based_routing_resources::put_sink_rt_nodes_in_net_pins_lookup(const vector<t_rt_node*>& sink_rt_nodes,
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

void Connection_based_routing_resources::set_lower_bound_connection_delays(vtr::vector_map<ClusterNetId, float *> &net_delay) {

    /* Set the lower bound connection delays after first iteration, which only optimizes for timing delay.
       This will be used later to judge the optimality of a connection, with suboptimal ones being candidates
       for forced reroute */

    auto& cluster_ctx = g_vpr_ctx.clustering();

	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[net_id];

        for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            net_lower_bound_connection_delay.push_back(net_delay[net_id][ipin]);
        }
    }
}

/* Run through all non-congested connections of all nets and see if any need to be forcibly rerouted.
       The connection must satisfy all following criteria:
                    1. the connection is critical enough
                    2. the connection is suboptimal, in comparison to lower_bound_connection_delay  */
bool Connection_based_routing_resources::forcibly_reroute_connections(float max_criticality,
        std::shared_ptr<const SetupTimingInfo> timing_info,
        const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
        vtr::vector_map<ClusterNetId, float *> &net_delay) {
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
    auto& route_ctx = g_vpr_ctx.routing();

    size_t overused_nodes = 0;
    size_t total_overuse = 0;
    size_t worst_overuse = 0;
    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        int overuse = route_ctx.rr_node_route_inf[inode].occ() - device_ctx.rr_nodes[inode].capacity();
        if (overuse > 0) {
            overused_nodes += 1;

            total_overuse += overuse;
            worst_overuse = std::max(worst_overuse, size_t(overuse));
        }
    }
    return OveruseInfo(device_ctx.rr_nodes.size(), overused_nodes, total_overuse, worst_overuse);
}

static WirelengthInfo calculate_wirelength_info() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t used_wirelength = 0;
    size_t available_wirelength = 0;

    for (size_t i = 0; i < device_ctx.rr_nodes.size(); ++i) {
        if (device_ctx.rr_nodes[i].type() == CHANX || device_ctx.rr_nodes[i].type() == CHANY) {
            available_wirelength += device_ctx.rr_nodes[i].capacity() +
                    device_ctx.rr_nodes[i].xhigh() - device_ctx.rr_nodes[i].xlow() +
                    device_ctx.rr_nodes[i].yhigh() - device_ctx.rr_nodes[i].ylow();
        }
    }

	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (!cluster_ctx.clb_nlist.net_is_global(net_id)
                && cluster_ctx.clb_nlist.net_sinks(net_id).size() != 0) { /* Globals don't count. */
            int bends, wirelength, segments;
            get_num_bends_and_length(net_id, &bends, &wirelength, &segments);

            used_wirelength += wirelength;
        }
    }
    VTR_ASSERT(available_wirelength > 0);

    return WirelengthInfo(available_wirelength, used_wirelength);
}

static void print_route_status_header() {
    VTR_LOG("---- ------ ---- ------- ------- ------- ----------------- --------------- -------- ---------- ---------- ---------- ---------- --------\n");
    VTR_LOG("Iter   Time  BBs    Heap  Re-Rtd  Re-Rtd Overused RR Nodes      Wirelength      CPD       sTNS       sWNS       hTNS       hWNS Est Succ\n");
    VTR_LOG("      (sec) Updt    push    Nets   Conns                                       (ns)       (ns)       (ns)       (ns)       (ns)     Iter\n");
    VTR_LOG("---- ------ ---- ------- ------- ------- ----------------- --------------- -------- ---------- ---------- ---------- ---------- --------\n");
}

static void print_route_status(int itry, double elapsed_sec, int num_bb_updated, const RouterStats& router_stats,
        const OveruseInfo& overuse_info, const WirelengthInfo& wirelength_info,
        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
        float est_success_iteration) {

    //Iteration
    VTR_LOG("%4d", itry);

    //Elapsed Time
    VTR_LOG(" %6.1f", elapsed_sec);

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
    VTR_LOG(" (%6.3f%)", overuse_info.overused_node_ratio()*100);

    //Wirelength
    constexpr int WL_DIGITS = 7;
    constexpr int WL_SCI_PRECISION = 2;
    pretty_print_uint(" ", wirelength_info.used_wirelength(), WL_DIGITS, WL_SCI_PRECISION);
    VTR_LOG(" (%4.1f%)", wirelength_info.used_wirelength_ratio()*100);

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

static void pretty_print_uint(const char* prefix, size_t value, int num_digits, int scientific_precision) {
    //Print as integer if it will fit in the width, other wise scientific
    if (value <= std::pow(10, num_digits) - 1) {
        //Direct
        VTR_LOG("%s%*zu", prefix, num_digits, value);
    } else {
        //Scientific
        VTR_LOG("%s%#*.*g", prefix, num_digits, scientific_precision, float(value));
    }
}

static std::string describe_unrouteable_connection(const int source_node, const int sink_node) {
    std::string msg = vtr::string_fmt("Cannot route from %s (%s) to "
                                      "%s (%s) -- no possible path",
                                      rr_node_arch_name(source_node).c_str(), describe_rr_node(source_node).c_str(),
                                      rr_node_arch_name(sink_node).c_str(), describe_rr_node(sink_node).c_str());

    return msg;
}

//Returns true if the specified net fanout is classified as high fanout
static bool is_high_fanout(int fanout) {
    return (fanout >= HIGH_FANOUT_NET_LIM);
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
static size_t dynamic_update_bounding_boxes() {
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

    for (ClusterNetId net : clb_nlist.nets()) {
        t_trace* routing_head = route_ctx.trace_head[net];

        if (routing_head == nullptr) continue; //Skip if no routing

        //We do not adjust the boundig boxes of high fanout nets, since they
        //use different bounding boxes based on the target location.
        //
        //This ensures that the delta values calculated below are always non-negative
        if (is_high_fanout(clb_nlist.net_sinks(net).size())) continue;

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

static void enable_router_debug(const t_router_opts& router_opts, ClusterNetId net) {
    f_router_debug = (router_opts.router_debug_net == -1 || net == ClusterNetId(router_opts.router_debug_net));

#ifndef VTR_ENABLE_DEBUG_LOGGING
    if (f_router_debug) VPR_THROW(VPR_ERROR_ROUTE, "Can not enable router debug logging since VPR was compiled without debug logging enabled");
#endif
}
