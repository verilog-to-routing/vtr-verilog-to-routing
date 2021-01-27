/**
 * @file place_timing_update.cpp
 * @brief Defines the routines declared in place_timing_update.h.
 */

#include "vtr_time.h"

#include "placer_globals.h"
#include "place_timing_update.h"

/* Routines local to place_timing_update.cpp */
static double comp_td_connection_cost(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities& place_crit,
                                      ClusterNetId net,
                                      int ipin);
static double sum_td_net_cost(ClusterNetId net);
static double sum_td_costs();

///@brief Use an incremental approach to updating timing costs after re-computing criticalities
static constexpr bool INCR_COMP_TD_COSTS = true;

/**
 * @brief Initialize the timing information and structures in the placer.
 *
 * Perform first time update on the timing graph, and initialize the values within
 * PlacerCriticalities, PlacerSetupSlacks, and connection_timing_cost.
 */
void initialize_timing_info(const PlaceCritParams& crit_params,
                            const PlaceDelayModel* delay_model,
                            PlacerCriticalities* criticalities,
                            PlacerSetupSlacks* setup_slacks,
                            ClusteredPinTimingInvalidator* pin_timing_invalidator,
                            SetupTimingInfo* timing_info,
                            t_placer_costs* costs) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    //As a safety measure, for the first time update,
    //invalidate all timing edges via the pin invalidator
    //by passing in all the clb sink pins
    for (ClusterNetId net_id : clb_nlist.nets()) {
        for (ClusterPinId pin_id : clb_nlist.net_sinks(net_id)) {
            pin_timing_invalidator->invalidate_connection(pin_id, timing_info);
        }
    }

    //Perform first time update for all timing related classes
    perform_full_timing_update(crit_params,
                               delay_model,
                               criticalities,
                               setup_slacks,
                               pin_timing_invalidator,
                               timing_info,
                               costs);

    //Don't warn again about unconstrained nodes again during placement
    timing_info->set_warn_unconstrained(false);

    //Clear all update_td_costs() runtime stat variables
    auto& p_runtime_ctx = g_placer_ctx.mutable_runtime();
    p_runtime_ctx.f_update_td_costs_connections_elapsed_sec = 0.f;
    p_runtime_ctx.f_update_td_costs_nets_elapsed_sec = 0.f;
    p_runtime_ctx.f_update_td_costs_sum_nets_elapsed_sec = 0.f;
    p_runtime_ctx.f_update_td_costs_total_elapsed_sec = 0.f;
}

/**
 * @brief Updates every timing related classes, variables and structures.
 *
 * This routine exists to reduce code duplication, as the placer routines
 * often require updating every timing related stuff.
 *
 * Updates: SetupTimingInfo, PlacerCriticalities, PlacerSetupSlacks,
 *          timing_cost, connection_setup_slack.
 */
void perform_full_timing_update(const PlaceCritParams& crit_params,
                                const PlaceDelayModel* delay_model,
                                PlacerCriticalities* criticalities,
                                PlacerSetupSlacks* setup_slacks,
                                ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                SetupTimingInfo* timing_info,
                                t_placer_costs* costs) {
    /* Update all timing related classes. */
    criticalities->enable_update();
    setup_slacks->enable_update();
    update_timing_classes(crit_params,
                          timing_info,
                          criticalities,
                          setup_slacks,
                          pin_timing_invalidator);

    /* Update the timing cost with new connection criticalities. */
    update_timing_cost(delay_model,
                       criticalities,
                       &costs->timing_cost);

    /* Commit the setup slacks since they are updated. */
    commit_setup_slacks(setup_slacks);
}

/**
 * @brief Update timing information based on the current block positions.
 *
 * Run STA to update the timing info class.
 *
 * Update the values stored in PlacerCriticalities and PlacerSetupSlacks
 * if they are enabled to update. To enable updating, call their respective
 * enable_update() method. See their documentation for more detailed info.
 *
 * If criticalities are updated, the timing driven costs should be updated
 * as well by calling update_timing_cost(). Calling this routine to update
 * timing_cost will produce round-off error in the long run due to its
 * incremental nature, so the timing cost value will be recomputed once in
 * a while, via other timing driven routines.
 *
 * If setup slacks are updated, then normally they should be committed to
 * `connection_setup_slack` via commit_setup_slacks() routine. However,
 * sometimes new setup slack values are not committed immediately if we
 * expect to revert the current timing update in the near future, or if
 * we wish to compare the new slack values to the original ones.
 *
 * All the pins with changed connection delays have already been added into
 * the ClusteredPinTimingInvalidator to allow incremental STA update. These
 * changed connection delays are a direct result of moved blocks in try_swap().
 */
void update_timing_classes(const PlaceCritParams& crit_params,
                           SetupTimingInfo* timing_info,
                           PlacerCriticalities* criticalities,
                           PlacerSetupSlacks* setup_slacks,
                           ClusteredPinTimingInvalidator* pin_timing_invalidator) {
    /* Run STA to update slacks and adjusted/relaxed criticalities. */
    timing_info->update();

    /* Update the placer's criticalities (e.g. sharpen with crit_exponent). */
    criticalities->update_criticalities(timing_info, crit_params);

    /* Update the placer's raw setup slacks. */
    setup_slacks->update_setup_slacks(timing_info);

    /* Clear invalidation state. */
    pin_timing_invalidator->reset();
}

/**
 * @brief Update the timing driven (td) costs.
 *
 * This routine either uses incremental update_td_costs(), or updates
 * from scratch using comp_td_costs(). By default, it is incremental
 * by iterating over the set of clustered netlist connections/pins
 * returned by PlacerCriticalities::pins_with_modified_criticality().
 *
 * Hence, this routine should always be called when PlacerCriticalites
 * is enabled to be updated in update_timing_classes(). Otherwise, the
 * incremental method will no longer be correct.
 */
void update_timing_cost(const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        double* timing_cost) {
#ifdef INCR_COMP_TD_COSTS
    update_td_costs(delay_model, *criticalities, timing_cost);
#else
    comp_td_costs(delay_model, *criticalities, timing_cost);
#endif
}

/**
 * @brief Commit all the setup slack values from the PlacerSetupSlacks
 *        class to `connection_setup_slack`.
 *
 * This routine is incremental since it relies on the pins_with_modified_setup_slack()
 * to detect which pins need to be updated and which pins do not.
 *
 * Therefore, it is assumed that this routine is always called immediately after
 * each time update_timing_classes() is called with setup slack update enabled.
 * Otherwise, pins_with_modified_setup_slack() cannot accurately account for all
 * the pins that have their setup slacks changed, making this routine incorrect.
 *
 * Currently, the only exception to the rule above is when setup slack analysis is used
 * during the placement quench. The new setup slacks might be either accepted or
 * rejected, so for efficiency reasons, this routine is not called if the slacks are
 * rejected in the end. For more detailed info, see the try_swap() routine.
 */
void commit_setup_slacks(const PlacerSetupSlacks* setup_slacks) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    auto& connection_setup_slack = g_placer_ctx.mutable_timing().connection_setup_slack;

    /* Incremental: only go through sink pins with modified setup slack */
    auto clb_pins_modified = setup_slacks->pins_with_modified_setup_slack();
    for (ClusterPinId pin_id : clb_pins_modified) {
        ClusterNetId net_id = clb_nlist.pin_net(pin_id);
        size_t pin_index_in_net = clb_nlist.pin_net_index(pin_id);

        connection_setup_slack[net_id][pin_index_in_net] = setup_slacks->setup_slack(net_id, pin_index_in_net);
    }
}

/**
 * @brief Verify that the values in `connection_setup_slack` matches PlacerSetupSlacks.
 *
 * Return true if all connection values are identical. Otherwise, return false.
 *
 * Currently, this routine is called to check if the timing update has been successfully
 * reverted after a proposed move is rejected when applying setup slack analysis during
 * the placement quench. If successful, the setup slacks in PlacerSetupSlacks should be
 * the same as the values in `connection_setup_slack` without running commit_setup_slacks().
 * For more detailed info, see the try_swap() routine.
 */
bool verify_connection_setup_slacks(const PlacerSetupSlacks* setup_slacks) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const auto& connection_setup_slack = g_placer_ctx.timing().connection_setup_slack;

    /* Go through every single sink pin to check that the slack values are the same */
    for (ClusterNetId net_id : clb_nlist.nets()) {
        for (size_t ipin = 1; ipin < clb_nlist.net_pins(net_id).size(); ++ipin) {
            if (connection_setup_slack[net_id][ipin] != setup_slacks->setup_slack(net_id, ipin)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Incrementally updates timing cost based on the current delays and criticality estimates.
 *
 * Unlike comp_td_costs(), this only updates connections who's criticality has changed.
 * This is a superset of those connections whose connection delay has changed. For a
 * from-scratch recalculation, refer to comp_td_cost().
 *
 * We must be careful calculating the total timing cost incrementally, due to limited
 * floating point precision, so that we get a bit-identical result matching the one
 * calculated by comp_td_costs().
 *
 * In particular, we can not simply calculate the incremental delta's caused by changed
 * connection timing costs and adjust the timing cost. Due to limited precision, the results
 * of floating point math operations are order dependant and we would get a different result.
 *
 * To get around this, we calculate the timing costs hierarchically, to ensure that we
 * calculate the sum with the same order of operations as comp_td_costs().
 *
 * See PlacerTimingCosts object used to represent connection_timing_costs for details.
 */
void update_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost) {
    vtr::Timer t;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& clb_nlist = cluster_ctx.clb_nlist;

    auto& p_timing_ctx = g_placer_ctx.mutable_timing();
    auto& p_runtime_ctx = g_placer_ctx.mutable_runtime();
    auto& connection_timing_cost = p_timing_ctx.connection_timing_cost;

    //Update the modified pin timing costs
    {
        vtr::Timer timer;
        auto clb_pins_modified = place_crit.pins_with_modified_criticality();
        for (ClusterPinId clb_pin : clb_pins_modified) {
            if (clb_nlist.pin_type(clb_pin) == PinType::DRIVER) continue;

            ClusterNetId clb_net = clb_nlist.pin_net(clb_pin);
            VTR_ASSERT_SAFE(clb_net);

            if (cluster_ctx.clb_nlist.net_is_ignored(clb_net)) continue;

            int ipin = clb_nlist.pin_net_index(clb_pin);
            VTR_ASSERT_SAFE(ipin >= 1 && ipin < int(clb_nlist.net_pins(clb_net).size()));

            double new_timing_cost = comp_td_connection_cost(delay_model, place_crit, clb_net, ipin);

            //Record new value
            connection_timing_cost[clb_net][ipin] = new_timing_cost;
        }

        p_runtime_ctx.f_update_td_costs_connections_elapsed_sec += timer.elapsed_sec();
    }

    //Re-total timing costs of all nets
    {
        vtr::Timer timer;
        *timing_cost = connection_timing_cost.total_cost();
        p_runtime_ctx.f_update_td_costs_sum_nets_elapsed_sec += timer.elapsed_sec();
    }

#ifdef VTR_ASSERT_DEBUG_ENABLED
    double check_timing_cost = 0.;
    comp_td_costs(delay_model, place_crit, &check_timing_cost);
    VTR_ASSERT_DEBUG_MSG(check_timing_cost == *timing_cost,
                         "Total timing cost calculated incrementally in update_td_costs() is "
                         "not consistent with value calculated from scratch in comp_td_costs()");
#endif
    p_runtime_ctx.f_update_td_costs_total_elapsed_sec += t.elapsed_sec();
}

/**
 * @brief Recomputes timing cost from scratch based on the current delays and criticality estimates.
 *
 * Computes the cost (from scratch) from the delays and criticalities of all point to point
 * connections, we define the timing cost of each connection as criticality * delay.
 *
 * We calculate the timing cost in a hierarchical manner (first connection, then nets, then
 * sum of nets) in order to allow it to be incremental while avoiding round-off effects.
 *
 * For a more efficient incremental update, see update_td_costs().
 */
void comp_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& p_timing_ctx = g_placer_ctx.mutable_timing();

    auto& connection_timing_cost = p_timing_ctx.connection_timing_cost;
    auto& net_timing_cost = p_timing_ctx.net_timing_cost;

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) continue;

        for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++) {
            float conn_timing_cost = comp_td_connection_cost(delay_model, place_crit, net_id, ipin);

            /* Record new value */
            connection_timing_cost[net_id][ipin] = conn_timing_cost;
        }
        /* Store net timing cost for more efficient incremental updating */
        net_timing_cost[net_id] = sum_td_net_cost(net_id);
    }
    /* Make sure timing cost does not go above MIN_TIMING_COST. */
    *timing_cost = sum_td_costs();
}

/**
 * @brief Calculates the timing cost of the specified connection.
 *
 * This routine assumes that it is only called either compt_td_cost() or
 * update_td_costs(). Otherwise, various assertions below would fail.
 */
static double comp_td_connection_cost(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities& place_crit,
                                      ClusterNetId net,
                                      int ipin) {
    const auto& p_timing_ctx = g_placer_ctx.timing();

    VTR_ASSERT_SAFE_MSG(ipin > 0, "Shouldn't be calculating connection timing cost for driver pins");

    VTR_ASSERT_SAFE_MSG(p_timing_ctx.connection_delay[net][ipin] == comp_td_single_connection_delay(delay_model, net, ipin),
                        "Connection delays should already be updated");

    double conn_timing_cost = place_crit.criticality(net, ipin) * p_timing_ctx.connection_delay[net][ipin];

    VTR_ASSERT_SAFE_MSG(std::isnan(p_timing_ctx.proposed_connection_delay[net][ipin]),
                        "Propsoed connection delay should already be invalidated");

    VTR_ASSERT_SAFE_MSG(std::isnan(p_timing_ctx.proposed_connection_timing_cost[net][ipin]),
                        "Proposed connection timing cost should already be invalidated");

    return conn_timing_cost;
}

///@brief Returns the timing cost of the specified 'net' based on the values in connection_timing_cost.
static double sum_td_net_cost(ClusterNetId net) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& p_timing_ctx = g_placer_ctx.mutable_timing();
    auto& connection_timing_cost = p_timing_ctx.connection_timing_cost;

    double net_td_cost = 0;
    for (unsigned ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
        net_td_cost += connection_timing_cost[net][ipin];
    }

    return net_td_cost;
}

///@brief Returns the total timing cost accross all nets based on the values in net_timing_cost.
static double sum_td_costs() {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& p_timing_ctx = g_placer_ctx.timing();
    const auto& net_timing_cost = p_timing_ctx.net_timing_cost;

    double td_cost = 0;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            continue;
        }
        td_cost += net_timing_cost[net_id];
    }

    return td_cost;
}
