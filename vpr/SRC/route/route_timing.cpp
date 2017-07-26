#include <cstdio>
#include <ctime>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "vtr_assert.h"
#include "vtr_log.h"

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
#include "routing_success_predictor.h"

// all functions in profiling:: namespace, which are only activated if PROFILE is defined
#include "route_profiling.h" 

#include "timing_info.h"
#include "timing_util.h"

#include "router_lookahead_map.h"

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


/******************** Subroutines local to route_timing.c ********************/

static t_rt_node* setup_routing_resources(int itry, int inet, unsigned num_sinks, float pres_fac, int min_incremental_reroute_fanout,
        CBRR& incremental_rerouting_res, t_rt_node** rt_node_of_sink);

static bool timing_driven_route_sink(int itry, int inet, unsigned itarget, int target_pin, float target_criticality,
        float pres_fac, float astar_fac, float bend_cost, t_rt_node* rt_root, t_rt_node** rt_node_of_sink);

static void add_route_tree_to_heap(t_rt_node * rt_node, int target_node,
        float target_criticality, float astar_fac);

static void timing_driven_expand_neighbours(t_heap *current,
        t_bb bounding_box, float bend_cost, float criticality_fac,
        int num_sinks, int target_node,
        float astar_fac, int highfanout_rlim);

static float get_timing_driven_expected_cost(int inode, int target_node,
        float criticality_fac, float R_upstream);

static int get_expected_segs_to_target(int inode, int target_node,
        int *num_segs_ortho_dir_ptr);

static void timing_driven_check_net_delays(float **net_delay);

static int mark_node_expansion_by_bin(int source_node, int target_node,
        t_rt_node * rt_node, t_bb bounding_box, int num_sinks);


static bool should_route_net(int inet, const CBRR& connections_inf);
static bool early_exit_heuristic(const WirelengthInfo& wirelength_info);

struct more_sinks_than {

    inline bool operator()(const int& net_index1, const int& net_index2) {
        auto& cluster_ctx = g_vpr_ctx.clustering();
        return cluster_ctx.clbs_nlist.net[net_index1].num_sinks() > cluster_ctx.clbs_nlist.net[net_index2].num_sinks();
    }
};

static WirelengthInfo calculate_wirelength_info();
static OveruseInfo calculate_overuse_info();

static void print_route_status_header();
static void print_route_status(int itry, double elapsed_sec,
        const OveruseInfo& overuse_info, const WirelengthInfo& wirelength_info,
        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
        float est_success_iteration);
static int round_up(float x);

/************************ Subroutine definitions *****************************/
bool try_timing_driven_route(t_router_opts router_opts,
        float **net_delay,
        const IntraLbPbPinLookup& pb_gpin_lookup,
        std::shared_ptr<SetupHoldTimingInfo> timing_info,
#ifdef ENABLE_CLASSIC_VPR_STA
        t_slack * slacks,
        const t_timing_inf &timing_inf,
#endif
        t_clb_opins_used& clb_opins_used_locally,
        ScreenUpdatePriority first_iteration_priority) {

    /* Timing-driven routing algorithm.  The timing graph (includes slack)   *
     * must have already been allocated, and net_delay must have been allocated. *
     * Returns true if the routing succeeds, false otherwise.                    */

    //Initialize and properly size the lookups for profiling
    profiling::profiling_initialization(get_max_pins_per_net());

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    //sort so net with most sinks is first.
    auto sorted_nets = vector<int>(cluster_ctx.clbs_nlist.net.size());
    for (size_t i = 0; i < cluster_ctx.clbs_nlist.net.size(); ++i) {
        sorted_nets[i] = i;
    }
    std::sort(sorted_nets.begin(), sorted_nets.end(), more_sinks_than());

    /*
     * Configure the routing predictor
     */
    RoutingSuccessPredictor routing_success_predictor;
    float abort_iteration_threshold = std::numeric_limits<float>::infinity(); //Default no early abort
    if (router_opts.routing_failure_predictor == SAFE) {
        abort_iteration_threshold = ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_SAFE * router_opts.max_router_iterations;
    } else if (router_opts.routing_failure_predictor == AGGRESSIVE) {
        abort_iteration_threshold = ROUTING_PREDICTOR_ITERATION_ABORT_FACTOR_AGGRESSIVE * router_opts.max_router_iterations;
    } else {
        VTR_ASSERT_MSG(router_opts.routing_failure_predictor == OFF, "Unrecognized routing failure predictor setting");
    }

    /* Set delay of global signals to zero. Non-global net delays are set by
       update_net_delays_from_route_tree() inside timing_driven_route_net(), 
       which is only called for non-global nets. */
    for (unsigned int inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); ++inet) {
        if (cluster_ctx.clbs_nlist.net[inet].is_global) {
            for (unsigned int ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ++ipin) {
                net_delay[inet][ipin] = 0.;
            }
        }
    }

    CBRR connections_inf{};
    VTR_ASSERT_SAFE(connections_inf.sanity_check_lookup());

    /*
     * Routing parameters
     */
    float pres_fac = router_opts.first_iter_pres_fac; /* Typically 0 -> ignore cong. */

    /*
     * Routing status and metrics
     */
    bool routing_is_successful = false;
    WirelengthInfo wirelength_info;
    OveruseInfo overuse_info;
    tatum::TimingPathInfo critical_path;
    int itry; //Routing iteration number

    /*
     * On the first routing iteration ignore congestion to get reasonable net 
     * delay estimates. Set criticalities to 1 when timing analysis is on to 
     * optimize timing, and to 0 when timing analysis is off to optimize routability. 
     * 
     * Subsequent iterations use the net delays from the previous iteration. 
     */
    print_route_status_header();
    timing_driven_route_structs route_structs;
    for (itry = 1; itry <= router_opts.max_router_iterations; ++itry) {
        clock_t begin = clock();

        /* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
        for (unsigned int inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); ++inet) {
            cluster_ctx.clbs_nlist.net[inet].is_routed = false;
            cluster_ctx.clbs_nlist.net[inet].is_fixed = false;
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
        for (unsigned int i = 0; i < cluster_ctx.clbs_nlist.net.size(); ++i) {
            bool is_routable = try_timing_driven_route_net(
                    sorted_nets[i],
                    itry,
                    pres_fac,
                    router_opts,
                    connections_inf,
                    route_structs.pin_criticality,
                    route_structs.rt_node_of_sink,
                    net_delay,
                    pb_gpin_lookup,
                    route_timing_info);

            if (!is_routable) {
                return (false); //Impossible to route
            }
        }

        // Make sure any CLB OPINs used up by subblocks being hooked directly to them are reserved for that purpose
        bool rip_up_local_opins = (itry == 1 ? false : true);
        reserve_locally_used_opins(pres_fac, router_opts.acc_fac, rip_up_local_opins, clb_opins_used_locally);

        float time = static_cast<float> (clock() - begin) / CLOCKS_PER_SEC;

        /*
         * Calculate metrics for the current routing
         */
        bool routing_is_feasible = feasible_routing();
        float est_success_iteration = routing_success_predictor.estimate_success_iteration();

        overuse_info = calculate_overuse_info();
        wirelength_info = calculate_wirelength_info();
        routing_success_predictor.add_iteration_overuse(itry, overuse_info.overused_nodes());

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
        }

        //Output progress
        print_route_status(itry, time, overuse_info, wirelength_info, timing_info, est_success_iteration);

        //Update graphics
        if (itry == 1) {
            update_screen(first_iteration_priority, "Routing...", ROUTING, timing_info);
        } else {
            update_screen(ScreenUpdatePriority::MINOR, "Routing...", ROUTING, timing_info);
        }

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
                vtr::printf_info("Routing aborted, the predicted iteration for a successful route (%.1f) is too high.\n", est_success_iteration);
                break; //Abort
            }
        }

        /*
         * Prepare for the next iteration
         */

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

        if (timing_info) {
            /*
             * Determine if any connectsion need to be forcibly re-routed due to timing
             */
            if (itry == 1) {
                // first iteration sets up the lower bound connection delays since only timing is optimized for
                connections_inf.set_stable_critical_path_delay(critical_path.delay());
                connections_inf.set_lower_bound_connection_delays(net_delay);
            } else {
                bool stable_routing_configuration = true;
                // only need to forcibly reroute if critical path grew significantly
                if (connections_inf.critical_path_delay_grew_significantly(critical_path.delay()))
                    stable_routing_configuration = connections_inf.forcibly_reroute_connections(router_opts.max_criticality,
                        timing_info,
                        pb_gpin_lookup,
                        net_delay);

                // not stable if any connection needs to be forcibly rerouted
                if (stable_routing_configuration)
                    connections_inf.set_stable_critical_path_delay(critical_path.delay());
            }
        } else {
            /* If timing analysis is not enabled, make sure that the criticalities and the
               net_delays stay as 0 so that wirelength can be optimized. */

            for (unsigned int inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); ++inet) {
                for (unsigned int ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ++ipin) {
#ifdef ENABLE_CLASSIC_VPR_STA
                    slacks->timing_criticality[inet][ipin] = 0.;
#endif
                    net_delay[inet][ipin] = 0.;
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
            vtr::printf_info("Critical path: %g ns\n", 1e9 * critical_path.delay());
        }

        vtr::printf_info("Successfully routed after %d routing iterations.\n", itry);        
    } else {
        vtr::printf_info("Routing failed.\n");
    }

    return routing_is_successful;
}

bool try_timing_driven_route_net(int inet, int itry, float pres_fac,
        t_router_opts router_opts,
        CBRR& connections_inf,
        float* pin_criticality,
        t_rt_node** rt_node_of_sink, float** net_delay,
        const IntraLbPbPinLookup& pb_gpin_lookup,
        std::shared_ptr<SetupTimingInfo> timing_info) {

    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    bool is_routed = false;

    connections_inf.prepare_routing_for_net(inet);

    if (cluster_ctx.clbs_nlist.net[inet].is_fixed) { /* Skip pre-routed nets. */
        is_routed = true;
    } else if (cluster_ctx.clbs_nlist.net[inet].is_global) { /* Skip global nets. */
        is_routed = true;
    } else if (should_route_net(inet, connections_inf) == false) {
        is_routed = true;
    } else {
        // track time spent vs fanout
        profiling::net_fanout_start();

        is_routed = timing_driven_route_net(inet, itry, pres_fac,
                router_opts.max_criticality, router_opts.criticality_exp,
                router_opts.astar_fac, router_opts.bend_cost,
                connections_inf,
                pin_criticality, router_opts.min_incremental_reroute_fanout,
                rt_node_of_sink,
                net_delay[inet],
                pb_gpin_lookup,
                timing_info);

        profiling::net_fanout_end(cluster_ctx.clbs_nlist.net[inet].num_sinks());

        /* Impossible to route? (disconnected rr_graph) */
        if (is_routed) {
            cluster_ctx.clbs_nlist.net[inet].is_routed = true;
        } else {
            vtr::printf_info("Routing failed.\n");
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

int get_max_pins_per_net(void) {

    /* Returns the largest number of pins on any non-global net.    */

    unsigned int inet;
    int max_pins_per_net;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    max_pins_per_net = 0;
    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        if (cluster_ctx.clbs_nlist.net[inet].is_global == false) {
            max_pins_per_net = max(max_pins_per_net,
                    (int) cluster_ctx.clbs_nlist.net[inet].pins.size());
        }
    }

    return (max_pins_per_net);
}

struct Criticality_comp {
	const float* criticality;

	Criticality_comp(const float* calculated_criticalities) : criticality{calculated_criticalities} {}
	bool operator()(int a, int b) const {return criticality[a] > criticality[b];}
};

bool timing_driven_route_net(int inet, int itry, float pres_fac, float max_criticality,
        float criticality_exp, float astar_fac, float bend_cost,
        CBRR& connections_inf,
        float *pin_criticality, int min_incremental_reroute_fanout,
        t_rt_node ** rt_node_of_sink, float *net_delay,
        const IntraLbPbPinLookup& pb_gpin_lookup,
        std::shared_ptr<const SetupTimingInfo> timing_info) {

    /* Returns true as long as found some way to hook up this net, even if that *
     * way resulted in overuse of resources (congestion).  If there is no way   *
     * to route this net, even ignoring congestion, it returns false.  In this  *
     * case the rr_graph is disconnected and you can give up.                   */
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    unsigned int num_sinks = cluster_ctx.clbs_nlist.net[inet].num_sinks();

    t_rt_node* rt_root = setup_routing_resources(itry, inet, num_sinks, pres_fac, min_incremental_reroute_fanout, connections_inf, rt_node_of_sink);
    // after this point the route tree is correct
    // remaining_targets from this point on are the **pin indices** that have yet to be routed 
    auto& remaining_targets = connections_inf.get_remaining_targets();

    // should always have targets to route to since some parts of the net are still congested
    VTR_ASSERT(!remaining_targets.empty());


    // calculate criticality of remaining target pins
    for (int ipin : remaining_targets) {
        if (timing_info) {
            pin_criticality[ipin] = calculate_clb_net_pin_criticality(*timing_info, pb_gpin_lookup, inet, ipin);

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

        // build a branch in the route tree to the target
        if (!timing_driven_route_sink(itry, inet, itarget, target_pin, target_criticality,
                pres_fac, astar_fac, bend_cost, rt_root, rt_node_of_sink))
            return false;

        // need to gurantee ALL nodes' path costs are HUGE_POSITIVE_FLOAT at the start of routing to a sink
        // do this by resetting all the path_costs that have been touched while routing to the current sink
        reset_path_costs();
    } // finished all sinks

    /* For later timing analysis. */

    // may have to update timing delay of the previously legally reached sinks since downstream capacitance could be changed
    update_net_delays_from_route_tree(net_delay, rt_node_of_sink, inet);

    if (!cluster_ctx.clbs_nlist.net[inet].is_global) {
        for (unsigned ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ++ipin) {
            if (net_delay[ipin] == 0) { // should be SOURCE->OPIN->IPIN->SINK
                VTR_ASSERT(device_ctx.rr_nodes[rt_node_of_sink[ipin]->parent_node->parent_node->inode].type() == OPIN);
            }
        }
    }

    VTR_ASSERT_MSG(route_ctx.rr_node_state[rt_root->inode].occ() <= device_ctx.rr_nodes[rt_root->inode].capacity(), "SOURCE should never be congested");

    // route tree is not kept persistent since building it from the traceback the next iteration takes almost 0 time
    free_route_tree(rt_root);
    return (true);
}

static bool timing_driven_route_sink(int itry, int inet, unsigned itarget, int target_pin, float target_criticality,
        float pres_fac, float astar_fac, float bend_cost, t_rt_node* rt_root, t_rt_node** rt_node_of_sink) {

    /* Build a path from the existing route tree rooted at rt_root to the target_node
     * add this branch to the existing route tree and update pathfinder costs and rr_node_route_inf to reflect this */
    auto& route_ctx = g_vpr_ctx.mutable_routing();
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    
    profiling::sink_criticality_start();

    int source_node = route_ctx.net_rr_terminals[inet][0];
    int sink_node = route_ctx.net_rr_terminals[inet][target_pin];

    t_bb bounding_box = route_ctx.route_bb[inet];

    if (itarget > 0 && itry > 5) {
        /* Enough iterations given to determine opin, to speed up legal solution, do not let net use two opins */
        VTR_ASSERT(device_ctx.rr_nodes[rt_root->inode].type() == SOURCE);
        rt_root->re_expand = false;
    }

    t_heap * cheapest = timing_driven_route_connection(source_node, sink_node, target_criticality,
            astar_fac, bend_cost, rt_root, bounding_box, cluster_ctx.clbs_nlist.net[inet].num_sinks());

    if (cheapest == NULL) {
        const t_net_pin* src_net_pin = &cluster_ctx.clbs_nlist.net[inet].pins[0];
        const t_net_pin* sink_net_pin = &cluster_ctx.clbs_nlist.net[inet].pins[target_pin];
        const t_block* src_blk = &cluster_ctx.blocks[src_net_pin->block];
        const t_block* sink_blk = &cluster_ctx.blocks[sink_net_pin->block];
        vtr::printf("Failed to route connection from '%s' to '%s' for net '%s'\n",
                    src_blk->name,
                    sink_blk->name,
                    cluster_ctx.clbs_nlist.net[inet].name);
        return false;
    }
    
    profiling::sink_criticality_end(target_criticality);

    /* NB:  In the code below I keep two records of the partial routing:  the   *
     * traceback and the route_tree.  The route_tree enables fast recomputation *
     * of the Elmore delay to each node in the partial routing.  The traceback  *
     * lets me reuse all the routines written for breadth-first routing, which  *
     * all take a traceback structure as input.  Before this routine exits the  *
     * route_tree structure is destroyed; only the traceback is needed at that  *
     * point.                                                                   */

    int inode = cheapest->index;
    route_ctx.rr_node_route_inf[inode].target_flag--; /* Connected to this SINK. */
    t_trace* new_route_start_tptr = update_traceback(cheapest, inet);
    rt_node_of_sink[target_pin] = update_route_tree(cheapest);
    free_heap_data(cheapest);
    pathfinder_update_path_cost(new_route_start_tptr, 1, pres_fac);
    empty_heap();

    // routed to a sink successfully
    return true;
}

t_heap * timing_driven_route_connection(int source_node, int sink_node, float target_criticality,
        float astar_fac, float bend_cost, t_rt_node* rt_root, t_bb bounding_box, int num_sinks) {
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    int highfanout_rlim = mark_node_expansion_by_bin(source_node, sink_node, rt_root, bounding_box, num_sinks);

    // re-explore route tree from root to add any new nodes (buildheap afterwards)
    // route tree needs to be repushed onto the heap since each node's cost is target specific
    add_route_tree_to_heap(rt_root, sink_node, target_criticality, astar_fac);
    heap_::build_heap(); // via sifting down everything

    VTR_ASSERT_SAFE(heap_::is_valid());

    // cheapest s_heap (gives index to device_ctx.rr_nodes) in current route tree to be expanded on
    t_heap * cheapest{get_heap_head()};

    if (cheapest == NULL) {
        auto& device_ctx = g_vpr_ctx.device();
        const t_rr_node* source_rr_node = &device_ctx.rr_nodes[source_node];
        const t_rr_node* sink_rr_node = &device_ctx.rr_nodes[sink_node];

        vtr::printf_info("Cannot route from rr_node %d (type %s, ptc: %d) to rr_node %d (type %s, ptc: %d) -- no possible path\n", 
                source_node, source_rr_node->type_string(), source_rr_node->ptc_num(),
                sink_node, sink_rr_node->type_string(), sink_rr_node->ptc_num());

        reset_path_costs();
        free_route_tree(rt_root);
        return NULL;
    }

    float old_total_cost, new_total_cost, old_back_cost, new_back_cost;
    int inode = cheapest->index;
    while (inode != sink_node) {
        old_total_cost = route_ctx.rr_node_route_inf[inode].path_cost;
        new_total_cost = cheapest->cost;

        if (old_total_cost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
            old_back_cost = HUGE_POSITIVE_FLOAT;
        else
            old_back_cost = route_ctx.rr_node_route_inf[inode].backward_path_cost;

        new_back_cost = cheapest->backward_path_cost;

        /* I only re-expand a node if both the "known" backward cost is lower  *
         * in the new expansion (this is necessary to prevent loops from       *
         * forming in the routing and causing havoc) *and* the expected total  *
         * cost to the sink is lower than the old value.  Different R_upstream *
         * values could make a path with lower back_path_cost less desirable   *
         * than one with higher cost.  Test whether or not I should disallow   *
         * re-expansion based on a higher total cost.                          */

        if (old_total_cost > new_total_cost && old_back_cost > new_back_cost) {
            route_ctx.rr_node_route_inf[inode].prev_node = cheapest->u.prev_node;
            route_ctx.rr_node_route_inf[inode].prev_edge = cheapest->prev_edge;
            route_ctx.rr_node_route_inf[inode].path_cost = new_total_cost;
            route_ctx.rr_node_route_inf[inode].backward_path_cost = new_back_cost;

            // tag this node's path cost to be reset to HUGE_POSITIVE_FLOAT by reset_path_costs after routing to this sink
            // path_cost is specific for each sink (different expected cost)
            if (old_total_cost > 0.99 * HUGE_POSITIVE_FLOAT) /* First time touched. */
                add_to_mod_list(&route_ctx.rr_node_route_inf[inode].path_cost);

            timing_driven_expand_neighbours(cheapest, bounding_box, bend_cost,
                    target_criticality, num_sinks, sink_node, astar_fac,
                    highfanout_rlim);
        }

        free_heap_data(cheapest);
        cheapest = get_heap_head();

        if (cheapest == NULL) { /* Impossible routing.  No path for net. */
            auto& device_ctx = g_vpr_ctx.device();
            const t_rr_node* source_rr_node = &device_ctx.rr_nodes[source_node];
            const t_rr_node* sink_rr_node = &device_ctx.rr_nodes[sink_node];

            vtr::printf_info("Cannot route from rr_node %d (type %s, ptc: %d) to rr_node %d (type %s, ptc: %d) -- no possible path\n", 
                source_node, source_rr_node->type_string(), source_rr_node->ptc_num(),
                sink_node, sink_rr_node->type_string(), sink_rr_node->ptc_num());

            reset_path_costs();
            free_route_tree(rt_root);
            return NULL;
        }

        inode = cheapest->index;
    }
    return cheapest;
}

static t_rt_node* setup_routing_resources(int itry, int inet, unsigned num_sinks, float pres_fac, int min_incremental_reroute_fanout,
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
        pathfinder_update_path_cost(route_ctx.trace_head[inet], -1, pres_fac);
        free_traceback(inet);

        rt_root = init_route_tree_to_source(inet);
        for (unsigned int sink_pin = 1; sink_pin <= num_sinks; ++sink_pin)
            connections_inf.toreach_rr_sink(sink_pin);
        // since all connections will be rerouted for this net, clear all of net's forced reroute flags
        connections_inf.clear_force_reroute_for_net();

        // when we don't prune the tree, we also don't know the sink node indices 
        // thus we'll use functions that act on pin indices like mark_ends instead
        // of their versions that act on node indices directly like mark_remaining_ends
        mark_ends(inet);
    } else {
        auto& reached_rt_sinks = connections_inf.get_reached_rt_sinks();
        auto& remaining_targets = connections_inf.get_remaining_targets();

        profiling::net_rebuild_start();

        // convert the previous iteration's traceback into the partial route tree for this iteration
        rt_root = traceback_to_route_tree(inet);

        // check for edge correctness
        VTR_ASSERT_SAFE(is_valid_skeleton_tree(rt_root));
        VTR_ASSERT_SAFE(should_route_net(inet, connections_inf));

        // prune the branches of the tree that don't legally lead to sinks
        // destroyed represents whether the entire tree is illegal
        bool destroyed = prune_route_tree(rt_root, pres_fac, connections_inf);

        // entire traceback is still freed since the pruned tree will need to have its pres_cost updated
        pathfinder_update_path_cost(route_ctx.trace_head[inet], -1, pres_fac);
        free_traceback(inet);

        // if entirely pruned, have just the source (not removed from pathfinder costs)
        if (destroyed) {
            profiling::route_tree_pruned();
            // traceback remains empty for just the root and no need to update pathfinder costs
        } else {
            profiling::route_tree_preserved();

            // sync traceback into a state that matches the route tree
            traceback_from_route_tree(inet, rt_root, num_sinks - remaining_targets.size());
            // put the updated costs of the route tree nodes back into pathfinder
            pathfinder_update_path_cost(route_ctx.trace_head[inet], 1, pres_fac);
        }

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
        float target_criticality, float astar_fac) {

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
        heap_::push_back_node(inode, tot_cost, NO_PREVIOUS, NO_PREVIOUS,
                backward_path_cost, R_upstream);

    }

    linked_rt_edge = rt_node->u.child_list;

    while (linked_rt_edge != NULL) {
        child_node = linked_rt_edge->child;
        add_route_tree_to_heap(child_node, target_node, target_criticality,
                astar_fac);
        linked_rt_edge = linked_rt_edge->next;
    }
}

static void timing_driven_expand_neighbours(t_heap *current,
        t_bb bounding_box, float bend_cost, float criticality_fac,
        int num_sinks, int target_node,
        float astar_fac, int highfanout_rlim) {

    /* Puts all the rr_nodes adjacent to current on the heap.  rr_nodes outside *
     * the expanded bounding box specified in bounding_box are not added to the     *
     * heap.                                                                    */

    auto& device_ctx = g_vpr_ctx.device();

    float new_R_upstream;

    int inode = current->index;
    float old_back_pcost = current->backward_path_cost;
    float R_upstream = current->R_upstream;
    int num_edges = device_ctx.rr_nodes[inode].num_edges();

    int target_x = device_ctx.rr_nodes[target_node].xhigh();
    int target_y = device_ctx.rr_nodes[target_node].yhigh();
    bool high_fanout = num_sinks >= HIGH_FANOUT_NET_LIM;

    for (int iconn = 0; iconn < num_edges; iconn++) {
        int to_node = device_ctx.rr_nodes[inode].edge_sink_node(iconn);

        if (high_fanout) {
            // since target node is an IPIN, xhigh and xlow are the same (same for y values)
            if (device_ctx.rr_nodes[to_node].xhigh() < target_x - highfanout_rlim
                    || device_ctx.rr_nodes[to_node].xlow() > target_x + highfanout_rlim
                    || device_ctx.rr_nodes[to_node].yhigh() < target_y - highfanout_rlim
                    || device_ctx.rr_nodes[to_node].ylow() > target_y + highfanout_rlim) {
                continue; /* Node is outside high fanout bin. */
            }
        } else if (device_ctx.rr_nodes[to_node].xhigh() < bounding_box.xmin
                || device_ctx.rr_nodes[to_node].xlow() > bounding_box.xmax
                || device_ctx.rr_nodes[to_node].yhigh() < bounding_box.ymin
                || device_ctx.rr_nodes[to_node].ylow() > bounding_box.ymax)
            continue; /* Node is outside (expanded) bounding box. */


        /* Prune away IPINs that lead to blocks other than the target one.  Avoids  *
         * the issue of how to cost them properly so they don't get expanded before *
         * more promising routes, but makes route-throughs (via CLBs) impossible.   *
         * Change this if you want to investigate route-throughs.                   */

        t_rr_type to_type = device_ctx.rr_nodes[to_node].type();
        if (to_type == IPIN
                && (device_ctx.rr_nodes[to_node].xhigh() != target_x
                || device_ctx.rr_nodes[to_node].yhigh() != target_y))
            continue;

        /* new_back_pcost stores the "known" part of the cost to this node -- the   *
         * congestion cost of all the routing resources back to the existing route  *
         * plus the known delay of the total path back to the source.  new_tot_cost *
         * is this "known" backward cost + an expected cost to get to the target.   */

        float new_back_pcost = old_back_pcost
                + (1. - criticality_fac) * get_rr_cong_cost(to_node);

        int iswitch = device_ctx.rr_nodes[inode].edge_switch(iconn);
        if (device_ctx.rr_switch_inf[iswitch].buffered) {
            new_R_upstream = device_ctx.rr_switch_inf[iswitch].R;
        } else {
            new_R_upstream = R_upstream + device_ctx.rr_switch_inf[iswitch].R;
        }

        float Tdel = device_ctx.rr_nodes[to_node].C() * (new_R_upstream + 0.5 * device_ctx.rr_nodes[to_node].R());
        Tdel += device_ctx.rr_switch_inf[iswitch].Tdel;
        new_R_upstream += device_ctx.rr_nodes[to_node].R();
        new_back_pcost += criticality_fac * Tdel;

        if (bend_cost != 0.) {
            t_rr_type from_type = device_ctx.rr_nodes[inode].type();
            to_type = device_ctx.rr_nodes[to_node].type();
            if ((from_type == CHANX && to_type == CHANY)
                    || (from_type == CHANY && to_type == CHANX))
                new_back_pcost += bend_cost;
        }

        float expected_cost = get_timing_driven_expected_cost(to_node, target_node,
                criticality_fac, new_R_upstream);
        float new_tot_cost = new_back_pcost + astar_fac * expected_cost;

        node_to_heap(to_node, new_tot_cost, inode, iconn, new_back_pcost,
                new_R_upstream);

    } /* End for all neighbours */
}

static float get_timing_driven_expected_cost(int inode, int target_node, float criticality_fac, float R_upstream) {

    /* Determines the expected cost (due to both delay and resouce cost) to reach *
     * the target node from inode.  It doesn't include the cost of inode --       *
     * that's already in the "known" path_cost.                                   */

    t_rr_type rr_type;
    int cost_index, ortho_cost_index, num_segs_same_dir, num_segs_ortho_dir;
    float expected_cost, cong_cost, Tdel;

    auto& device_ctx = g_vpr_ctx.device();

    rr_type = device_ctx.rr_nodes[inode].type();

    if (rr_type == CHANX || rr_type == CHANY) {

#ifdef USE_MAP_LOOKAHEAD
        return get_lookahead_map_cost(inode, target_node, criticality_fac);
#else
        num_segs_same_dir = get_expected_segs_to_target(inode, target_node, &num_segs_ortho_dir);
        cost_index = device_ctx.rr_nodes[inode].cost_index();
        ortho_cost_index = device_ctx.rr_indexed_data[cost_index].ortho_cost_index;

        cong_cost = num_segs_same_dir * device_ctx.rr_indexed_data[cost_index].base_cost
                + num_segs_ortho_dir * device_ctx.rr_indexed_data[ortho_cost_index].base_cost;
        cong_cost += device_ctx.rr_indexed_data[IPIN_COST_INDEX].base_cost
                + device_ctx.rr_indexed_data[SINK_COST_INDEX].base_cost;

        Tdel = num_segs_same_dir * device_ctx.rr_indexed_data[cost_index].T_linear
                + num_segs_ortho_dir * device_ctx.rr_indexed_data[ortho_cost_index].T_linear
                + num_segs_same_dir * num_segs_same_dir * device_ctx.rr_indexed_data[cost_index].T_quadratic
                + num_segs_ortho_dir * num_segs_ortho_dir * device_ctx.rr_indexed_data[ortho_cost_index].T_quadratic
                + R_upstream * (num_segs_same_dir * device_ctx.rr_indexed_data[cost_index].C_load + num_segs_ortho_dir * device_ctx.rr_indexed_data[ortho_cost_index].C_load);

        Tdel += device_ctx.rr_indexed_data[IPIN_COST_INDEX].T_linear;

        expected_cost = criticality_fac * Tdel + (1. - criticality_fac) * cong_cost;
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
     * criticality, fanout, etc. of the current net being routed (inet).       */
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
    int inode;
    float area;
    bool success;
    t_linked_rt_edge *linked_rt_edge;
    t_rt_node * child_node;

    target_xlow = device_ctx.rr_nodes[target_node].xlow();
    target_ylow = device_ctx.rr_nodes[target_node].ylow();
    target_xhigh = device_ctx.rr_nodes[target_node].xhigh();
    target_yhigh = device_ctx.rr_nodes[target_node].yhigh();

    if (num_sinks < HIGH_FANOUT_NET_LIM) {
        /* This algorithm only applies to high fanout nets */
        return 1;
    }
    if (rt_node == NULL || rt_node->u.child_list == NULL) {
        /* If unknown traceback, set radius of bin to be size of chip */
        rlim = max(device_ctx.nx + 2, device_ctx.ny + 2);
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
    while (success == false && linked_rt_edge != NULL) {
        while (linked_rt_edge != NULL && success == false) {
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
            if (rlim > max(device_ctx.nx + 2, device_ctx.ny + 2)) {
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
    while (linked_rt_edge != NULL) {
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
#define ERROR_TOL 0.0001

static void timing_driven_check_net_delays(float **net_delay) {

    /* Checks that the net delays computed incrementally during timing driven    *
     * routing match those computed from scratch by the net_delay.c module.      */
    auto& cluster_ctx = g_vpr_ctx.clustering();

    unsigned int inet, ipin;
    float **net_delay_check;

    vtr::t_chunk list_head_net_delay_check_ch = {NULL, 0, NULL};

    /*t_linked_vptr *ch_list_head_net_delay_check;*/

    net_delay_check = alloc_net_delay(&list_head_net_delay_check_ch, cluster_ctx.clbs_nlist.net,
            cluster_ctx.clbs_nlist.net.size());
    load_net_delay_from_routing(net_delay_check, cluster_ctx.clbs_nlist.net, cluster_ctx.clbs_nlist.net.size());

    for (inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); inet++) {
        for (ipin = 1; ipin < cluster_ctx.clbs_nlist.net[inet].pins.size(); ipin++) {
            if (net_delay_check[inet][ipin] == 0.) { /* Should be only GLOBAL nets */
                if (fabs(net_delay[inet][ipin]) > ERROR_TOL) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "in timing_driven_check_net_delays: net %d pin %d.\n"
                            "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
                            inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
                }
            } else {
                if (fabs(1.0 - net_delay[inet][ipin] / net_delay_check[inet][ipin]) > ERROR_TOL) {
                    vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__,
                            "in timing_driven_check_net_delays: net %d pin %d.\n"
                            "\tIncremental calc. net_delay is %g, but from scratch net delay is %g.\n",
                            inet, ipin, net_delay[inet][ipin], net_delay_check[inet][ipin]);
                }
            }
        }
    }

    free_net_delay(net_delay_check, &list_head_net_delay_check_ch);
    vtr::printf_info("Completed net delay value cross check successfully.\n");
}

/* Detect if net should be routed or not */
static bool should_route_net(int inet, const CBRR& connections_inf) {
    auto& route_ctx = g_vpr_ctx.routing();
    auto& device_ctx = g_vpr_ctx.device();

    t_trace * tptr = route_ctx.trace_head[inet];

    if (tptr == NULL) {
        /* No routing yet. */
        return true;
    }

    for (;;) {
        int inode = tptr->index;
        int occ = route_ctx.rr_node_state[inode].occ();
        int capacity = device_ctx.rr_nodes[inode].capacity();

        if (occ > capacity) {
            return true; /* overuse detected */
        }

        if (device_ctx.rr_nodes[inode].type() == SINK) {
            // even if net is fully routed, not complete if parts of it should get ripped up (EXPERIMENTAL)
            if (connections_inf.should_force_reroute_connection(inode)) {
                return true;
            }
            tptr = tptr->next; /* Skip next segment. */
            if (tptr == NULL)
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
        vtr::printf_info("Wire length usage ratio %g exceeds limit of %g, fail routing.\n",
                wirelength_info.used_wirelength_ratio(),
                FIRST_ITER_WIRELENTH_LIMIT);
        return true;
    }
    return false;
}

// incremental rerouting resources class definitions
Connection_based_routing_resources::Connection_based_routing_resources() : 
	current_inet (NO_PREVIOUS), 	// not routing to a specific net yet (note that NO_PREVIOUS is not unsigned, so will be largest unsigned)
	last_stable_critical_path_delay {0.0f},
	critical_path_growth_tolerance {1.001f},
	connection_criticality_tolerance {0.9f},
	connection_delay_optimality_tolerance {1.1f}	{	

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

    size_t routing_num_nets = cluster_ctx.clbs_nlist.net.size();
    rr_sink_node_to_pin.resize(routing_num_nets);
    lower_bound_connection_delay.resize(routing_num_nets);
    forcible_reroute_connection_flag.resize(routing_num_nets);

    for (unsigned int inet = 0; inet < routing_num_nets; ++inet) {
        // unordered_map<int,int> net_node_to_pin;
        auto& net_node_to_pin = rr_sink_node_to_pin[inet];
        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[inet];
        auto& net_forcible_reroute_connection_flag = forcible_reroute_connection_flag[inet];

        unsigned int num_pins = cluster_ctx.clbs_nlist.net[inet].pins.size();
        net_node_to_pin.reserve(num_pins - 1); // not looking up on the SOURCE pin
        net_lower_bound_connection_delay.reserve(num_pins - 1); // will be filled in after the 1st iteration's
        net_forcible_reroute_connection_flag.reserve(num_pins - 1); // all false to begin with

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[inet][ipin];

            net_node_to_pin.insert({rr_sink_node, ipin});
            net_forcible_reroute_connection_flag.insert({rr_sink_node, false});
        }
    }
}

void Connection_based_routing_resources::convert_sink_nodes_to_net_pins(vector<int>& rr_sink_nodes) const {

    /* Turn a vector of device_ctx.rr_nodes indices, assumed to be of sinks for a net *
     * into the pin indices of the same net. */

    VTR_ASSERT(current_inet != (unsigned) NO_PREVIOUS); // not uninitialized

    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (size_t s = 0; s < rr_sink_nodes.size(); ++s) {

        auto mapping = node_to_pin_mapping.find(rr_sink_nodes[s]);
        // should always expect it find a pin mapping for its own net
        VTR_ASSERT_SAFE(mapping != node_to_pin_mapping.end());

        rr_sink_nodes[s] = mapping->second;
    }
}

void Connection_based_routing_resources::put_sink_rt_nodes_in_net_pins_lookup(const vector<t_rt_node*>& sink_rt_nodes,
        t_rt_node** rt_node_of_sink) const {

    /* Load rt_node_of_sink (which maps a PIN index to a route tree node)
     * with a vector of route tree sink nodes. */

    VTR_ASSERT(current_inet != (unsigned) NO_PREVIOUS);

    // a net specific mapping from node index to pin index
    const auto& node_to_pin_mapping = rr_sink_node_to_pin[current_inet];

    for (t_rt_node* rt_node : sink_rt_nodes) {
        auto mapping = node_to_pin_mapping.find(rt_node->inode);
        // element should be able to find itself
        VTR_ASSERT_SAFE(mapping != node_to_pin_mapping.end());

        rt_node_of_sink[mapping->second] = rt_node;
    }
}

bool Connection_based_routing_resources::sanity_check_lookup() const {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    for (unsigned int inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); ++inet) {
        const auto& net_node_to_pin = rr_sink_node_to_pin[inet];

        for (auto mapping : net_node_to_pin) {
            auto sanity = net_node_to_pin.find(mapping.first);
            if (sanity == net_node_to_pin.end()) {
                vtr::printf_info("%d cannot find itself (net %d)\n", mapping.first, inet);
                return false;
            }
            VTR_ASSERT(route_ctx.net_rr_terminals[inet][mapping.second] == mapping.first);
        }
    }
    return true;
}

void Connection_based_routing_resources::set_lower_bound_connection_delays(const float* const * net_delay) {

    /* Set the lower bound connection delays after first iteration, which only optimizes for timing delay.
       This will be used later to judge the optimality of a connection, with suboptimal ones being candidates
       for forced reroute */

    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t routing_num_nets = cluster_ctx.clbs_nlist.net.size();

    for (unsigned int inet = 0; inet < routing_num_nets; ++inet) {

        auto& net_lower_bound_connection_delay = lower_bound_connection_delay[inet];

        unsigned int num_pins = cluster_ctx.clbs_nlist.net[inet].pins.size();

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            net_lower_bound_connection_delay.push_back(net_delay[inet][ipin]);
        }
    }
}

bool Connection_based_routing_resources::forcibly_reroute_connections(float max_criticality,
        std::shared_ptr<const SetupTimingInfo> timing_info,
        const IntraLbPbPinLookup& pb_gpin_lookup,
        const float* const * net_delay) {

    /* Run through all non-congested connections of all nets and see if any need to be forcibly rerouted.
       The connection must satisfy all following criteria:
                    1. the connection is critical enough
                    2. the connection is suboptimal, in comparison to lower_bound_connection_delay  
     */

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    bool any_connection_rerouted = false; // true if any connection has been marked for rerouting

    size_t routing_num_nets = cluster_ctx.clbs_nlist.net.size();

    for (unsigned int inet = 0; inet < routing_num_nets; ++inet) {

        unsigned int num_pins = cluster_ctx.clbs_nlist.net[inet].pins.size();

        for (unsigned int ipin = 1; ipin < num_pins; ++ipin) {
            // rr sink node index corresponding to this connection terminal
            auto rr_sink_node = route_ctx.net_rr_terminals[inet][ipin];

            // should always be left unset or cleared by rerouting before the end of the iteration
            VTR_ASSERT(forcible_reroute_connection_flag[inet][rr_sink_node] == false);


            // skip if connection is internal to a block such that SOURCE->OPIN->IPIN->SINK directly, which would have 0 time delay
            if (lower_bound_connection_delay[inet][ipin - 1] == 0)
                continue;

            // update if more optimal connection found
            if (net_delay[inet][ipin] < lower_bound_connection_delay[inet][ipin - 1]) {
                lower_bound_connection_delay[inet][ipin - 1] = net_delay[inet][ipin];
                continue;
            }

            // skip if connection criticality is too low (not a problem connection)
            float pin_criticality = calculate_clb_net_pin_criticality(*timing_info, pb_gpin_lookup, inet, ipin);
            if (pin_criticality < (max_criticality * connection_criticality_tolerance))
                continue;

            // skip if connection's delay is close to optimal
            if (net_delay[inet][ipin] < (lower_bound_connection_delay[inet][ipin - 1] * connection_delay_optimality_tolerance))
                continue;

            forcible_reroute_connection_flag[inet][rr_sink_node] = true;
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

    VTR_ASSERT(current_inet != (unsigned) NO_PREVIOUS);

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
    int inode;
    for (inode = 0; inode < device_ctx.num_rr_nodes; inode++) {
        int overuse = route_ctx.rr_node_state[inode].occ() - device_ctx.rr_nodes[inode].capacity();
        if (overuse > 0) {
            overused_nodes += 1;

            total_overuse += overuse;
            worst_overuse = std::max(worst_overuse, size_t(overuse));
        }
    }
    return OveruseInfo(device_ctx.num_rr_nodes, overused_nodes, total_overuse, worst_overuse);
}

static WirelengthInfo calculate_wirelength_info() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    size_t used_wirelength = 0;
    size_t available_wirelength = 0;

    for (int i = 0; i < device_ctx.num_rr_nodes; ++i) {
        if (device_ctx.rr_nodes[i].type() == CHANX || device_ctx.rr_nodes[i].type() == CHANY) {
            available_wirelength += device_ctx.rr_nodes[i].capacity() +
                    device_ctx.rr_nodes[i].xhigh() - device_ctx.rr_nodes[i].xlow() +
                    device_ctx.rr_nodes[i].yhigh() - device_ctx.rr_nodes[i].ylow();
        }
    }

    for (unsigned int inet = 0; inet < cluster_ctx.clbs_nlist.net.size(); ++inet) {
        if (cluster_ctx.clbs_nlist.net[inet].is_global == false
                && cluster_ctx.clbs_nlist.net[inet].num_sinks() != 0) { /* Globals don't count. */
            int bends, wirelength, segments;
            get_num_bends_and_length(inet, &bends, &wirelength, &segments);

            used_wirelength += wirelength;
        }
    }
    VTR_ASSERT(available_wirelength > 0);

    return WirelengthInfo(available_wirelength, used_wirelength);
}

static void print_route_status_header() {
    vtr::printf_info("----- ---------- ------------------- ----------------- -------- ---------- ---------- ---------- ---------- ----------------\n");
    vtr::printf_info("Iter. Time (sec)   Overused RR Nodes       Wirelength  CPD (ns)  sTNS (ns)  sWNS (ns)  hTNS (ns)  hWNS (ns) Est. Succ. Iter.\n");
    vtr::printf_info("----- ---------- ------------------- ----------------- -------- ---------- ---------- ---------- ---------- ----------------\n");
}

static void print_route_status(int itry, double elapsed_sec,
        const OveruseInfo& overuse_info, const WirelengthInfo& wirelength_info,
        std::shared_ptr<const SetupHoldTimingInfo> timing_info,
        float est_success_iteration) {

    //Iteration
    vtr::printf("%5d", itry);

    //Elapsed Time
    vtr::printf(" %10.1f", elapsed_sec);

    //Overused RR nodes
    vtr::printf(" %8.3g (%7.4f%)", float(overuse_info.overused_nodes()), overuse_info.overused_node_ratio()*100);

    //Wirelength
    vtr::printf(" %9.4g (%4.1f%)", float(wirelength_info.used_wirelength()), wirelength_info.used_wirelength_ratio()*100);

    //CPD
    if (timing_info) {
        float cpd = timing_info->least_slack_critical_path().delay();
        vtr::printf(" %8.3f", 1e9 * cpd);
    } else {
        vtr::printf(" %8s", "N/A");
    }

    //sTNS
    if (timing_info) {
        float sTNS = timing_info->setup_total_negative_slack();
        vtr::printf(" % 10.4g", 1e9 * sTNS);
    } else {
        vtr::printf(" %10s", "N/A");
    }

    //sWNS
    if (timing_info) {
        float sWNS = timing_info->setup_worst_negative_slack();
        vtr::printf(" % 10.3f", 1e9 * sWNS);
    } else {
        vtr::printf(" %10s", "N/A");
    }

    //hTNS
    if (timing_info) {
        float hTNS = timing_info->hold_total_negative_slack();
        vtr::printf(" % 10.4g", 1e9 * hTNS);
    } else {
        vtr::printf(" %10s", "N/A");
    }

    //hWNS
    if (timing_info) {
        float hWNS = timing_info->hold_worst_negative_slack();
        vtr::printf(" % 10.3f", 1e9 * hWNS);
    } else {
        vtr::printf(" %10s", "N/A");
    }

    //Estimated success iteration
    if (std::isnan(est_success_iteration)) {
        vtr::printf(" %16s", "N/A");
    } else {
        vtr::printf(" %16.0f", est_success_iteration);
    }

    vtr::printf("\n");

    fflush(stdout);
}
