/**
 * @file place_timing_update.h
 * @brief Stores timing update routines declarations used by the VPR placer.
 */
#pragma once
#include "timing_place.h"
#include "place_util.h"

///<Forward declarations.
struct t_placer_timing_update_mode;
struct t_update_td_costs_stats;

///@brief Initialize the timing information and structures in the placer.
void initialize_timing_info(float crit_exponent,
                            const PlaceDelayModel* delay_model,
                            PlacerCriticalities* criticalities,
                            PlacerSetupSlacks* setup_slacks,
                            ClusteredPinTimingInvalidator* pin_timing_invalidator,
                            SetupTimingInfo* timing_info,
                            t_placer_timing_update_mode* timing_update_mode,
                            t_placer_costs* costs);

///@brief Update the timing information and structures in the placer.
void update_setup_slacks_and_criticalities(float crit_exponent,
                                           const PlaceDelayModel* delay_model,
                                           PlacerCriticalities* criticalities,
                                           PlacerSetupSlacks* setup_slacks,
                                           ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                           SetupTimingInfo* timing_info,
                                           t_placer_timing_update_mode* timing_update_mode,
                                           t_placer_costs* costs);

///@brief Incrementally updates timing cost based on the current delays and criticality estimates.
void update_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost);

///@brief Recomputes timing cost from scratch based on the current delays and criticality estimates.
void comp_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost);

///@brief Commit all the setup slack values from the PlacerSetupSlacks class to a vtr matrix.
void commit_setup_slacks(const PlacerSetupSlacks* setup_slacks);

///@brief Verify that the values in the vtr matrix matches the PlacerSetupSlacks class.
bool verify_connection_setup_slacks(const PlacerSetupSlacks* setup_slacks);

///@brief Fetch the file-scope variable update_td_costs_stats in timing_place.cpp.
t_update_td_costs_stats get_update_td_costs_runtime_stats();

/**
 * @brief Structure that determines the scope and the method of the timing update.
 *
 * This structure determines whether criticalities/setup slacks
 * values in PlacerCriticalities and PlacerSetupSlacks should be
 * updated in this iteration of timing update.
 *
 *   @param update_criticalities Update the values in PlacerCriticalities
 *   @param update_setup_slacks Update the values in PlacerSetupSlacks
 *
 * It also enables detecting whether incremental updates can be performed.
 * Each time incremental STA updates the timing graph, the timing analyzer
 * can be queried for a set of ATOM pins that have modified slack/criticality
 * values. If we keep feeding these modified pin ids into the two classes
 * above, then the updates will be incremental and correct. We can treat this
 * as PlacerCriticalities and PlacerSetupSlacks being `in sync` with the
 * timing graph.
 *
 * However, for example, if the timing graph is updated twice before we update
 * PlacerCriticalities, then we are only able to query the most recent set
 * of ATOM pins with modified values rather than all two sets of pins. Under
 * this circumstance, PlacerCriticalities is `out of sync` with the timing
 * graph, and we must calculate criticality for every sink pin in the netlist.
 * The case is the same for PlacerSetupSlacks.
 *
 * Fortunately, we know what does and what doesn't get updated in each iteration
 * of timing update, so we can also tell if we should perform re-computation
 * on the next iteration The following two variables are used and updated in
 * the routine `update_setup_slacks_and_criticalities()`.
 *
 *   @param recompute_criticalities Recompute criticalities for every sink pin
 *   @param recompute_setup_slacks Recompute setup slacks for every sink pin
 */
struct t_placer_timing_update_mode {
    bool update_criticalities;
    bool update_setup_slacks;
    bool recompute_criticalities;
    bool recompute_setup_slacks;
};

///@brief Structure that stores CPU runtime stats for the update_td_costs() routine.
struct t_update_td_costs_stats {
    float connections_elapsed_sec = 0.;
    float nets_elapsed_sec = 0.;
    float sum_nets_elapsed_sec = 0.;
    float total_elapsed_sec = 0.;
};
