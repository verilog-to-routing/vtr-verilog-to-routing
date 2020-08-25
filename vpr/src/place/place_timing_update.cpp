/**
 * @file place_timing_update.cpp
 * @brief Defines the routines declared in place_timing_update.h.
 */

#include "vtr_time.h"

#include "place_timing_update.h"
#include "place_global.h"

///@brief Use an incremental approach to updating timing costs after re-computing criticalities
static constexpr bool INCR_COMP_TD_COSTS = true;

///@brief File-scope variable that can be accessed via the routine get_udpate_td_costs_runtime_stats().
static t_update_td_costs_stats update_td_costs_stats;

///@brief Routines local to place_timing_update.cpp
static double comp_td_connection_cost(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities& place_crit,
                                      ClusterNetId net,
                                      int ipin);
static double sum_td_net_cost(ClusterNetId net);
static double sum_td_costs();

/**
 * @brief Initialize the timing information and structures in the placer.
 *
 * Perform first time update on the timing graph, and initialize the values within
 * PlacerCriticalities, PlacerSetupSlacks, and connection_timing_cost.
 */
void initialize_timing_info(float crit_exponent,
                            const PlaceDelayModel* delay_model,
                            PlacerCriticalities* criticalities,
                            PlacerSetupSlacks* setup_slacks,
                            ClusteredPinTimingInvalidator* pin_timing_invalidator,
                            SetupTimingInfo* timing_info,
                            t_placer_timing_update_mode* timing_update_mode,
                            t_placer_costs* costs) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& clb_nlist = cluster_ctx.clb_nlist;

    //Initialize the timing update mode. Update both
    //setup slacks and criticalities from scratch
    timing_update_mode->update_criticalities = true;
    timing_update_mode->update_setup_slacks = true;
    timing_update_mode->recompute_criticalities = true;
    timing_update_mode->recompute_setup_slacks = true;

    //As a safety measure, for the first time update,
    //invalidate all timing edges via the pin invalidator
    //by passing in all the clb sink pins
    for (ClusterNetId net_id : clb_nlist.nets()) {
        for (ClusterPinId pin_id : clb_nlist.net_sinks(net_id)) {
            pin_timing_invalidator->invalidate_connection(pin_id, timing_info);
        }
    }

    //Perform timing info update
    update_setup_slacks_and_criticalities(crit_exponent,
                                          delay_model,
                                          criticalities,
                                          setup_slacks,
                                          pin_timing_invalidator,
                                          timing_info,
                                          timing_update_mode,
                                          costs);

    //Compute timing cost from scratch
    comp_td_costs(delay_model, *criticalities, &costs->timing_cost);

    //Initialize the data structure that stores committed placer setup slacks
    commit_setup_slacks(setup_slacks);

    //Don't warn again about unconstrained nodes again during placement
    timing_info->set_warn_unconstrained(false);
}

/**
 * @brief Update timing info based on the current block positions.
 *
 * Update the values stored in PlacerCriticalities and PlacerSetupSlacks.
 * This routine tries its best to be incremental when it comes to updating
 * these values, and branching variables are stored in `timing_update_mode`.
 * For a detailed description of how these variables work, please refer to
 * the declaration documentation on t_placer_timing_update_mode.
 *
 * If criticalities are updated, the timing costs are updated as well.
 * Calling this routine to update timing_cost will produce round-off error
 * in the long run, so this value will be recomputed once in a while, via
 * other timing driven routines.
 *
 * All the pins with changed connection delays have already been added into
 * the ClusteredPinTimingInvalidator to allow incremental STA update. These
 * changed connection delays are a direct result of moved blocks in try_swap().
 *
 * @param crit_exponent            Used to calculate `sharpened` criticalities.
 *
 * @param delay_model              Used to calculate the delay between two locations.
 *
 * @param criticalities            Mapping interface between atom pin criticalities
 *                                 and clb pin criticalities.
 *
 * @param setup_slacks             Mapping interface between atom pin raw setup slacks
 *                                 and clb pin raw setup slacks.
 *
 * @param pin_timing_invalidator   Stores all the pins that have their delay value changed
 *                                 and needs to be updated in the timing graph.
 *
 * @param timing_info              Stores the timing graph and other important timing info.
 *
 * @param timing_update_mode       Determines what should be updated when this routine is
 *                                 called, and using incremental techniques is appropriate.
 *
 * @param costs                    Stores the updated timing cost for the whole placement.
 */
void update_setup_slacks_and_criticalities(float crit_exponent,
                                           const PlaceDelayModel* delay_model,
                                           PlacerCriticalities* criticalities,
                                           PlacerSetupSlacks* setup_slacks,
                                           ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                           SetupTimingInfo* timing_info,
                                           t_placer_timing_update_mode* timing_update_mode,
                                           t_placer_costs* costs) {
    //Run STA to update slacks and adjusted/relaxed criticalities
    timing_info->update();

    if (timing_update_mode->update_setup_slacks) {
        //Update placer's setup slacks
        setup_slacks->update_setup_slacks(timing_info, timing_update_mode->recompute_setup_slacks);
    }

    if (timing_update_mode->update_criticalities) {
        //Update placer's criticalities (e.g. sharpen with crit_exponent)
        criticalities->update_criticalities(timing_info, crit_exponent, timing_update_mode->recompute_criticalities);

        //Update connection, net and total timing costs based on new criticalities
        if (INCR_COMP_TD_COSTS) {
            update_td_costs(delay_model, *criticalities, &costs->timing_cost);
        } else {
            comp_td_costs(delay_model, *criticalities, &costs->timing_cost);
        }
    }

    //Setup slacks and criticalities need to be in sync with the timing_info.
    //if they are to be incrementally updated on the next iteration.
    //Otherwise, a re-computation for all clb sink pins is required.
    timing_update_mode->recompute_setup_slacks = !timing_update_mode->update_setup_slacks;
    timing_update_mode->recompute_criticalities = !timing_update_mode->update_criticalities;

    //Clear invalidation state
    pin_timing_invalidator->reset();
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

        update_td_costs_stats.connections_elapsed_sec += timer.elapsed_sec();
    }

    //Re-total timing costs of all nets
    {
        vtr::Timer timer;
        *timing_cost = connection_timing_cost.total_cost();
        update_td_costs_stats.sum_nets_elapsed_sec += timer.elapsed_sec();
    }

#ifdef VTR_ASSERT_DEBUG_ENABLED
    double check_timing_cost = 0.;
    comp_td_costs(delay_model, place_crit, &check_timing_cost);
    VTR_ASSERT_DEBUG_MSG(check_timing_cost == *timing_cost,
                         "Total timing cost calculated incrementally in update_td_costs() is "
                         "not consistent with value calculated from scratch in comp_td_costs()");
#endif
    update_td_costs_stats.total_elapsed_sec += t.elapsed_sec();
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
    VTR_ASSERT_SAFE_MSG(ipin > 0, "Shouldn't be calculating connection timing cost for driver pins");

    VTR_ASSERT_SAFE_MSG(connection_delay[net][ipin] == comp_td_single_connection_delay(delay_model, net, ipin),
                        "Connection delays should already be updated");

    double conn_timing_cost = place_crit.criticality(net, ipin) * connection_delay[net][ipin];

    VTR_ASSERT_SAFE_MSG(std::isnan(proposed_connection_delay[net][ipin]),
                        "Propsoed connection delay should already be invalidated");

    VTR_ASSERT_SAFE_MSG(std::isnan(proposed_connection_timing_cost[net][ipin]),
                        "Proposed connection timing cost should already be invalidated");

    return conn_timing_cost;
}

///@brief Returns the timing cost of the specified 'net' based on the values in connection_timing_cost.
static double sum_td_net_cost(ClusterNetId net) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    double net_td_cost = 0;
    for (unsigned ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net).size(); ipin++) {
        net_td_cost += connection_timing_cost[net][ipin];
    }

    return net_td_cost;
}

///@brief Returns the total timing cost accross all nets based on the values in net_timing_cost.
static double sum_td_costs() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    double td_cost = 0;
    for (auto net_id : cluster_ctx.clb_nlist.nets()) { /* For each net ... */

        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) continue;

        td_cost += net_timing_cost[net_id];
    }

    return td_cost;
}

/**
 * @brief Commit all the setup slack values from the PlacerSetupSlacks
 *        class to a vtr matrix.
 *
 * This routine is incremental since it relies on the pins_with_modified_setup_slack()
 * to detect which pins need to be updated and which pins do not.
 *
 * Therefore, it is assumed that this routine is always called immediately after
 * each time update_setup_slacks_and_criticalities() updates the setup slacks
 * (i.e. t_placer_timing_update_mode::update_setup_slacks = true). Otherwise,
 * pins_with_modified_setup_slack() cannot accurately account for all the pins
 * that have their setup slacks changed, making this routine incorrect.
 *
 * Currently, the only exception to the rule above is when setup slack analysis is used
 * during the placement quench. The new setup slacks might be either accepted or
 * rejected, so for efficiency reasons, this routine is not called if the slacks are
 * rejected in the end. For more detailed info, see the try_swap() routine.
 */
void commit_setup_slacks(const PlacerSetupSlacks* setup_slacks) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    //Incremental: only go through sink pins with modified setup slack
    auto clb_pins_modified = setup_slacks->pins_with_modified_setup_slack();
    for (ClusterPinId pin_id : clb_pins_modified) {
        ClusterNetId net_id = clb_nlist.pin_net(pin_id);
        size_t pin_index_in_net = clb_nlist.pin_net_index(pin_id);

        connection_setup_slack[net_id][pin_index_in_net] = setup_slacks->setup_slack(net_id, pin_index_in_net);
    }
}

/**
 * @brief Verify that the values in the vtr matrix matches the PlacerSetupSlacks class.
 *
 * Return true if all values are identical. Otherwise, return false.
 * Used to check if the timing update has been succesfully revereted if a proposed move
 * is rejected when applying setup slack analysis during the placement quench.
 * If successful, the setup slacks in the timing analyzer should be the same as
 * the setup slacks in connection_setup_slack matrix without running commit_setup_slacks().
 *
 * For more detailed info, see the try_swap() routine.
 */
bool verify_connection_setup_slacks(const PlacerSetupSlacks* setup_slacks) {
    const auto& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    //Go through every single sink pin to check that the slack values are the same
    for (ClusterNetId net_id : clb_nlist.nets()) {
        for (size_t ipin = 1; ipin < clb_nlist.net_pins(net_id).size(); ++ipin) {
            if (connection_setup_slack[net_id][ipin] != setup_slacks->setup_slack(net_id, ipin)) {
                return false;
            }
        }
    }
    return true;
}

///@brief Fetch the file-scope variable update_td_costs_stats in timing_place.cpp.
t_update_td_costs_stats get_update_td_costs_runtime_stats() {
    return update_td_costs_stats;
}
