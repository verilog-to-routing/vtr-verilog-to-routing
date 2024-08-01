#include "concrete_timing_info.h"
#include "connection_based_routing.h"
#include "draw.h"
#include "netlist_routers.h"
#include "place_and_route.h"
#include "read_route.h"
#include "route.h"
#include "route_common.h"
#include "route_debug.h"
#include "route_export.h"
#include "route_profiling.h"
#include "route_utils.h"
#include "vtr_time.h"

bool route(const Netlist<>& net_list,
           int width_fac,
           const t_router_opts& router_opts,
           const t_analysis_opts& analysis_opts,
           t_det_routing_arch* det_routing_arch,
           std::vector<t_segment_inf>& segment_inf,
           NetPinsMatrix<float>& net_delay,
           std::shared_ptr<SetupHoldTimingInfo> timing_info,
           std::shared_ptr<RoutingDelayCalculator> delay_calc,
           t_chan_width_dist chan_width_dist,
           t_direct_inf* directs,
           int num_directs,
           ScreenUpdatePriority first_iteration_priority,
           bool is_flat) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& route_ctx = g_vpr_ctx.mutable_routing();

    if (net_list.nets().empty()) {
        VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "No nets to route\n");
    }

    t_graph_type graph_type;
    t_graph_type graph_directionality;
    if (router_opts.route_type == GLOBAL) {
        graph_type = GRAPH_GLOBAL;
        graph_directionality = GRAPH_BIDIR;
    } else {
        graph_type = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
        graph_directionality = (det_routing_arch->directionality == BI_DIRECTIONAL ? GRAPH_BIDIR : GRAPH_UNIDIR);
    }

    /* Set the channel widths */
    t_chan_width chan_width = init_chan(width_fac, chan_width_dist, graph_directionality);

    /* Set up the routing resource graph defined by this FPGA architecture. */
    int warning_count;

    create_rr_graph(graph_type,
                    device_ctx.physical_tile_types,
                    device_ctx.grid,
                    chan_width,
                    det_routing_arch,
                    segment_inf,
                    router_opts,
                    directs,
                    num_directs,
                    &warning_count,
                    is_flat);

    //Initialize drawing, now that we have an RR graph
    init_draw_coords(width_fac);

    /* Allocate and load additional rr_graph information needed only by the router. */
    alloc_and_load_rr_node_route_structs();

    init_route_structs(net_list,
                       router_opts.bb_factor,
                       router_opts.has_choking_spot,
                       is_flat);

    IntraLbPbPinLookup intra_lb_pb_pin_lookup(device_ctx.logical_block_types);
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, intra_lb_pb_pin_lookup);

    auto choking_spots = set_nets_choking_spots(net_list,
                                                route_ctx.net_terminal_groups,
                                                route_ctx.net_terminal_group_num,
                                                router_opts.has_choking_spot,
                                                is_flat);

    //Initially, the router runs normally trying to reduce congestion while
    //balancing other metrics (timing, wirelength, run-time etc.)
    RouterCongestionMode router_congestion_mode = RouterCongestionMode::NORMAL;

    //Initialize and properly size the lookups for profiling
    profiling::profiling_initialization(get_max_pins_per_net(net_list));

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
    for (auto net_id : net_list.nets()) {
        if (net_list.net_is_ignored(net_id)) {
            for (unsigned int ipin = 1; ipin < net_list.net_pins(net_id).size(); ++ipin) {
                net_delay[net_id][ipin] = 0.;
            }
        }
    }

    CBRR connections_inf{net_list, route_ctx.net_rr_terminals, is_flat};

    route_budgets budgeting_inf(net_list, is_flat);

    // This needs to be called before filling intra-cluster lookahead maps to ensure that the intra-cluster lookahead maps are initialized.
    const RouterLookahead* router_lookahead = get_cached_router_lookahead(*det_routing_arch,
                                                                          router_opts.lookahead_type,
                                                                          router_opts.write_router_lookahead,
                                                                          router_opts.read_router_lookahead,
                                                                          segment_inf,
                                                                          is_flat);

    if (is_flat) {
        // If is_flat is true, the router lookahead maps related to intra-cluster resources should be initialized since
        // they haven't been initialized when the map related to global resources was initialized.
        auto cache_key = route_ctx.router_lookahead_cache_key_;
        std::unique_ptr<RouterLookahead> mut_router_lookahead(route_ctx.cached_router_lookahead_.release());
        VTR_ASSERT(mut_router_lookahead);
        route_ctx.cached_router_lookahead_.clear();
        if (!router_opts.read_intra_cluster_router_lookahead.empty()) {
            mut_router_lookahead->read_intra_cluster(router_opts.read_intra_cluster_router_lookahead);
        } else {
            mut_router_lookahead->compute_intra_tile();
        }
        route_ctx.cached_router_lookahead_.set(cache_key, std::move(mut_router_lookahead));
        router_lookahead = get_cached_router_lookahead(*det_routing_arch,
                                                       router_opts.lookahead_type,
                                                       router_opts.write_router_lookahead,
                                                       router_opts.read_router_lookahead,
                                                       segment_inf,
                                                       is_flat);
        if (!router_opts.write_intra_cluster_router_lookahead.empty()) {
            router_lookahead->write_intra_cluster(router_opts.write_intra_cluster_router_lookahead);
        }
    }

    VTR_ASSERT(router_lookahead != nullptr);

    /* Routing parameters */
    float pres_fac = router_opts.first_iter_pres_fac;
    update_draw_pres_fac(pres_fac); /* Typically 0 -> ignore cong. */
    int bb_fac = router_opts.bb_factor;

    /* When routing conflicts are detected the bounding boxes are scaled
     * by BB_SCALE_FACTOR every BB_SCALE_ITER_COUNT iterations */
    constexpr float BB_SCALE_FACTOR = 2;
    constexpr int BB_SCALE_ITER_COUNT = 5;

    size_t available_wirelength = calculate_wirelength_available();

    /* Routing status and metrics */
    bool success = false;
    WirelengthInfo wirelength_info;
    OveruseInfo overuse_info(device_ctx.rr_graph.num_nodes());
    tatum::TimingPathInfo critical_path;
    int itry; //Routing iteration number
    int itry_conflicted_mode = 0;

    /* Best result so far */
    vtr::vector<ParentNetId, vtr::optional<RouteTree>> best_routing;
    t_clb_opins_used best_clb_opins_used_locally;
    RoutingMetrics best_routing_metrics;
    int legal_convergence_count = 0;

    /* Get initial criticalities from the lookahead */
    if (router_opts.with_timing_analysis && router_opts.initial_timing == e_router_initial_timing::LOOKAHEAD) {
        vtr::ScopedStartFinishTimer init_timing_timer("Initializing router criticalities");
        //Estimate initial connection delays from the router lookahead
        init_net_delay_from_lookahead(*router_lookahead,
                                      net_list,
                                      route_ctx.net_rr_terminals,
                                      net_delay,
                                      device_ctx.rr_graph,
                                      is_flat);

        //Run STA to get estimated criticalities
        timing_info->update();
        VTR_LOG("Initial Net Connection Criticality Histogram:\n");
        print_router_criticality_histogram(net_list, *timing_info, netlist_pin_lookup, is_flat);
    }

    std::unique_ptr<NetPinTimingInvalidator> pin_timing_invalidator;
    pin_timing_invalidator = make_net_pin_timing_invalidator(
        router_opts.timing_update_type,
        net_list,
        netlist_pin_lookup,
        atom_ctx.nlist,
        atom_ctx.lookup,
        *timing_info->timing_graph(),
        is_flat);

    std::unique_ptr<NetlistRouter> netlist_router = make_netlist_router(
        net_list,
        router_lookahead,
        router_opts,
        connections_inf,
        net_delay,
        netlist_pin_lookup,
        timing_info,
        pin_timing_invalidator.get(),
        budgeting_inf,
        routing_predictor,
        choking_spots,
        is_flat);

    RouterStats router_stats;
    float prev_iter_cumm_time = 0;
    vtr::Timer iteration_timer;
    int num_net_bounding_boxes_updated = 0;
    int itry_since_last_convergence = -1;

    // This heap is used for reserve_locally_used_opins.
    FourAryHeap small_heap;
    small_heap.init_heap(device_ctx.grid);

    // When RCV is enabled the router will not stop unless negative hold slack is 0
    // In some cases this isn't doable, due to global nets or intracluster routing issues
    // In these cases RCV will finish early if it goes RCV_FINISH_EARLY_COUNTDOWN iterations without detecting resolvable negative hold slack
    // Increasing this will make the router fail occasionally, decreasing will sometimes not let all hold violations be resolved
    constexpr int RCV_FINISH_EARLY_COUNTDOWN = 15;

    int rcv_finished_count = RCV_FINISH_EARLY_COUNTDOWN;

    print_route_status_header();
    for (itry = 1; itry <= router_opts.max_router_iterations; ++itry) {
        /* Reset "is_routed" and "is_fixed" flags to indicate nets not pre-routed (yet) */
        for (auto net_id : net_list.nets()) {
            route_ctx.net_status.set_is_routed(net_id, false);
            route_ctx.net_status.set_is_fixed(net_id, false);
        }

        if (itry_since_last_convergence >= 0) {
            ++itry_since_last_convergence;
        }

        // Calculate this once and pass it into net routing to check if should reroute for hold
        float worst_negative_slack = 0;
        if (budgeting_inf.if_set()) {
            worst_negative_slack = timing_info->hold_total_negative_slack();
        }

        /* Initial criticalities: set to 1 on the first iter if the user asked for it */
        if (router_opts.initial_timing == e_router_initial_timing::ALL_CRITICAL && itry == 1)
            netlist_router->set_timing_info(make_constant_timing_info(1));
        else
            netlist_router->set_timing_info(timing_info);

        /* Route each net */
        RouteIterResults iter_results = netlist_router->route_netlist(itry, pres_fac, worst_negative_slack);

        if (!iter_results.is_routable) { /* Disconnected RRG */
            return false;
        }

        // Make sure any CLB OPINs used up by subblocks being hooked directly to them are reserved for that purpose
        bool rip_up_local_opins = (itry == 1 ? false : true);
        if (!is_flat) {
            reserve_locally_used_opins(&small_heap, pres_fac,
                                       router_opts.acc_fac, rip_up_local_opins, is_flat);
        }

        /*
         * Calculate metrics for the current routing
         */
        bool routing_is_feasible = feasible_routing();
        float est_success_iteration = routing_predictor.estimate_success_iteration();

        //Update resource costs and overuse info
        if (itry == 1) {
            pathfinder_update_acc_cost_and_overuse_info(0., overuse_info); /* Acc_fac=0 for first iter. */
        } else {
            pathfinder_update_acc_cost_and_overuse_info(router_opts.acc_fac, overuse_info);
        }

        wirelength_info = calculate_wirelength_info(net_list, available_wirelength);
        routing_predictor.add_iteration_overuse(itry, overuse_info.overused_nodes);

        //Update timing based on the new routing
        //Note that the net delays have already been updated by timing_driven_route_net
        timing_info->update();
        timing_info->set_warn_unconstrained(false); //Don't warn again about unconstrained nodes again during routing
        pin_timing_invalidator->reset();

        critical_path = timing_info->least_slack_critical_path();

        VTR_ASSERT_SAFE(!router_opts.with_timing_analysis || check_net_delays(net_list, net_delay));

        if (itry == 1 && router_opts.with_timing_analysis) {
            generate_route_timing_reports(router_opts, analysis_opts, *timing_info, *delay_calc, is_flat);
        }

        float iter_cumm_time = iteration_timer.elapsed_sec();
        float iter_elapsed_time = iter_cumm_time - prev_iter_cumm_time;

        //Output progress
        print_route_status(itry, iter_elapsed_time, pres_fac, num_net_bounding_boxes_updated, iter_results.stats, overuse_info, wirelength_info, timing_info, est_success_iteration);

        prev_iter_cumm_time = iter_cumm_time;

        //Update graphics
        if (itry == 1) {
            update_screen(first_iteration_priority, "Routing...", ROUTING, timing_info);
        } else {
            update_screen(ScreenUpdatePriority::MINOR, "Routing...", ROUTING, timing_info);
        }

        if (router_opts.save_routing_per_iteration) {
            std::string filename = vtr::string_fmt("iteration_%03d.route", itry);
            print_route(net_list, nullptr, filename.c_str(), is_flat);
        }

        //Update router stats (total)
        router_stats.combine(iter_results.stats);

        /*
         * Are we finished?
         */
        if (is_iteration_complete(routing_is_feasible, router_opts, itry, timing_info, rcv_finished_count == 0)) {
            auto& router_ctx = g_vpr_ctx.routing();

            if (is_better_quality_routing(best_routing, best_routing_metrics, wirelength_info, timing_info)) {
                //Save routing
                best_routing = router_ctx.route_trees;
                best_clb_opins_used_locally = router_ctx.clb_opins_used_locally;

                success = true;

                //Update best metrics
                if (router_opts.with_timing_analysis) {
                    check_net_delays(net_list, net_delay);
                    best_routing_metrics.sTNS = timing_info->setup_total_negative_slack();
                    best_routing_metrics.sWNS = timing_info->setup_worst_negative_slack();
                    best_routing_metrics.hTNS = timing_info->hold_total_negative_slack();
                    best_routing_metrics.hWNS = timing_info->hold_worst_negative_slack();
                    best_routing_metrics.critical_path = critical_path;
                }
                best_routing_metrics.used_wirelength = wirelength_info.used_wirelength();
            }

            //Decrease pres_fac so that critical connections will take more direct routes
            //Note that we use first_iter_pres_fac here (typically zero), and switch to
            //use initial_pres_fac on the next iteration.
            pres_fac = router_opts.first_iter_pres_fac;
            update_draw_pres_fac(pres_fac);

            //Reduce timing tolerances to re-route more delay-suboptimal signals
            connections_inf.set_connection_criticality_tolerance(0.7);
            connections_inf.set_connection_delay_tolerance(1.01);

            ++legal_convergence_count;
            itry_since_last_convergence = 0;

            VTR_ASSERT(success);
        }

        if (itry_since_last_convergence == 1) {
            //We used first_iter_pres_fac when we started routing again
            //after the first routing convergence. Since that is often zero,
            //we want to set pres_fac to a reasonable (i.e. typically non-zero)
            //value afterwards -- so it grows when multiplied by pres_fac_mult
            pres_fac = router_opts.initial_pres_fac;
            update_draw_pres_fac(pres_fac);
        }

        //Have we converged the maximum number of times, did not make any changes, or does it seem
        //unlikely additional convergences will improve QoR?
        if (legal_convergence_count >= router_opts.max_convergence_count
            || iter_results.stats.connections_routed == 0
            || early_reconvergence_exit_heuristic(router_opts, itry_since_last_convergence, timing_info, best_routing_metrics)) {
#ifndef NO_GRAPHICS
            update_router_info_and_check_bp(BP_ROUTE_ITER, -1);
#endif
            break; //Done routing
        }

        /*
         * Abort checks: Should we give-up because this routing problem is unlikely to converge to a legal routing?
         */
        if (itry == 1 && early_exit_heuristic(router_opts, wirelength_info)) {
#ifndef NO_GRAPHICS
            update_router_info_and_check_bp(BP_ROUTE_ITER, -1);
#endif
            //Abort
            break;
        }

        //Estimate at what iteration we will converge to a legal routing
        if (overuse_info.overused_nodes > ROUTING_PREDICTOR_MIN_ABSOLUTE_OVERUSE_THRESHOLD) {
            //Only consider aborting if we have a significant number of overused resources

            if (!std::isnan(est_success_iteration) && est_success_iteration > abort_iteration_threshold && router_opts.routing_budgets_algorithm != YOYO) {
                VTR_LOG("Routing aborted, the predicted iteration for a successful route (%.1f) is too high.\n", est_success_iteration);
#ifndef NO_GRAPHICS
                update_router_info_and_check_bp(BP_ROUTE_ITER, -1);
#endif
                break; //Abort
            }
        }

        if (itry == 1 && router_opts.exit_after_first_routing_iteration) {
            VTR_LOG("Exiting after first routing iteration as requested\n");
#ifndef NO_GRAPHICS
            update_router_info_and_check_bp(BP_ROUTE_ITER, -1);
#endif
            break;
        }

        /*
         * Prepare for the next iteration
         */

        if (router_opts.route_bb_update == e_route_bb_update::DYNAMIC) {
            num_net_bounding_boxes_updated = dynamic_update_bounding_boxes(iter_results.rerouted_nets);
        }

        if (itry >= high_effort_congestion_mode_iteration_threshold) {
            //We are approaching the maximum number of routing iterations,
            //and still do not have a legal routing. Switch to a mode which
            //focuses more on attempting to resolve routing conflicts.
            router_congestion_mode = RouterCongestionMode::CONFLICTED;
        }

        //Update pres_fac
        if (itry == 1) {
            pres_fac = router_opts.initial_pres_fac;
            update_draw_pres_fac(pres_fac);
        } else {
            pres_fac *= router_opts.pres_fac_mult;
            pres_fac = std::min(pres_fac, router_opts.max_pres_fac);
            /* Set the maximum pres_fac to the value passed by the command line argument */
            update_draw_pres_fac(pres_fac);

            // Increase short path criticality if it's having a hard time resolving hold violations due to congestion
            if (budgeting_inf.if_set()) {
                bool rcv_finished = false;

                /* This constant represents how much extra delay the budget increaser adds to the minimum and maximum delay budgets
                 * Experimentally this value delivers fast hold slack resolution, while not overwhelming the router
                 * Increasing this will make it resolve hold faster, but could result in lower circuit quality */
                constexpr float budget_increase_factor = 300e-12;

                if (itry > 5 && worst_negative_slack != 0)
                    rcv_finished = budgeting_inf.increase_min_budgets_if_struggling(budget_increase_factor, timing_info, worst_negative_slack, netlist_pin_lookup);
                if (rcv_finished)
                    rcv_finished_count--;
                else
                    rcv_finished_count = RCV_FINISH_EARLY_COUNTDOWN;
            }
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
            //alleviate the conflicts.
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

                route_ctx.route_bb = load_route_bb(net_list, bb_fac);
            }

            ++itry_conflicted_mode;
        }

        if (router_opts.with_timing_analysis) {
            if (itry == 1) {
                // first iteration sets up the lower bound connection delays since only timing is optimized for
                connections_inf.set_stable_critical_path_delay(critical_path.delay());
                connections_inf.set_lower_bound_connection_delays(net_delay);

                //load budgets using information from uncongested delay information
                budgeting_inf.load_route_budgets(net_delay, timing_info, netlist_pin_lookup, router_opts);

                if (router_opts.routing_budgets_algorithm == YOYO)
                    netlist_router->set_rcv_enabled(true);
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
            }
        } else {
            /* If timing analysis is not enabled, make sure that the criticalities and the
             * net_delays stay as 0 so that wirelength can be optimized. */

            for (auto net_id : net_list.nets()) {
                for (unsigned int ipin = 1; ipin < net_list.net_pins(net_id).size(); ++ipin) {
                    net_delay[net_id][ipin] = 0.;
                }
            }
        }

        if (router_opts.congestion_analysis) profiling::congestion_analysis();
        if (router_opts.fanout_analysis) profiling::time_on_fanout_analysis();
        // profiling::time_on_criticality_analysis();
    }

    /* Write out partition tree logs (no-op if debug option not set) */
    PartitionTreeDebug::write("partition_tree.log");

    if (success) {
        VTR_LOG("Restoring best routing\n");

        auto& router_ctx = g_vpr_ctx.mutable_routing();

        /* Restore congestion from best route */
        for (auto net_id : net_list.nets()) {
            if (route_ctx.route_trees[net_id])
                pathfinder_update_cost_from_route_tree(route_ctx.route_trees[net_id]->root(), -1);
            if (best_routing[net_id])
                pathfinder_update_cost_from_route_tree(best_routing[net_id]->root(), 1);
        }
        router_ctx.route_trees = best_routing;
        router_ctx.clb_opins_used_locally = best_clb_opins_used_locally;

        prune_unused_non_configurable_nets(connections_inf, net_list);

        if (router_opts.with_timing_analysis) {
            VTR_LOG("Critical path: %g ns\n", 1e9 * best_routing_metrics.critical_path.delay());
        }

        VTR_LOG("Successfully routed after %d routing iterations.\n", itry);
    } else {
        VTR_LOG("Routing failed.\n");

        //If the routing fails, print the overused info
        print_overused_nodes_status(router_opts, overuse_info);

#ifdef VTR_ENABLE_DEBUG_LOGGING
        if (f_router_debug)
            print_invalid_routing_info(net_list, is_flat);
#endif
    }

    if (router_opts.with_timing_analysis) {
        VTR_LOG("Final Net Connection Criticality Histogram:\n");
        print_router_criticality_histogram(net_list, *timing_info, netlist_pin_lookup, is_flat);
    }

    VTR_ASSERT(router_stats.heap_pushes >= router_stats.intra_cluster_node_pushes);
    VTR_ASSERT(router_stats.heap_pops >= router_stats.intra_cluster_node_pops);
    VTR_LOG(
        "Router Stats: total_nets_routed: %zu total_connections_routed: %zu total_heap_pushes: %zu total_heap_pops: %zu ",
        router_stats.nets_routed, router_stats.connections_routed, router_stats.heap_pushes, router_stats.heap_pops);
#ifdef VTR_ENABLE_DEBUG_LOGGING
    VTR_LOG(
        "total_internal_heap_pushes: %zu total_internal_heap_pops: %zu total_external_heap_pushes: %zu total_external_heap_pops: %zu ",
        router_stats.intra_cluster_node_pushes, router_stats.intra_cluster_node_pops,
        router_stats.inter_cluster_node_pushes, router_stats.inter_cluster_node_pops);
    for (int node_type_idx = 0; node_type_idx < t_rr_type::NUM_RR_TYPES; node_type_idx++) {
        VTR_LOG("total_external_%s_pushes: %zu ", rr_node_typename[node_type_idx], router_stats.inter_cluster_node_type_cnt_pushes[node_type_idx]);
        VTR_LOG("total_external_%s_pops: %zu ", rr_node_typename[node_type_idx], router_stats.inter_cluster_node_type_cnt_pops[node_type_idx]);
        VTR_LOG("total_internal_%s_pushes: %zu ", rr_node_typename[node_type_idx], router_stats.intra_cluster_node_type_cnt_pushes[node_type_idx]);
        VTR_LOG("total_internal_%s_pops: %zu ", rr_node_typename[node_type_idx], router_stats.intra_cluster_node_type_cnt_pops[node_type_idx]);
        VTR_LOG("rt_node_%s_pushes: %zu ", rr_node_typename[node_type_idx], router_stats.rt_node_pushes[node_type_idx]);
    }
#endif
    VTR_LOG("\n");

    return success;
}
