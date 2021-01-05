/**
 * @file place_timing_update.h
 * @brief Timing update routines used by the VPR placer.
 */

#pragma once
#include "timing_place.h"
#include "place_util.h"

///@brief Initialize the timing information and structures in the placer.
void initialize_timing_info(const PlaceCritParams& crit_params,
                            const PlaceDelayModel* delay_model,
                            PlacerCriticalities* criticalities,
                            PlacerSetupSlacks* setup_slacks,
                            ClusteredPinTimingInvalidator* pin_timing_invalidator,
                            SetupTimingInfo* timing_info,
                            t_placer_costs* costs);

///@brief Updates every timing related classes, variables and structures.
void perform_full_timing_update(const PlaceCritParams& crit_params,
                                const PlaceDelayModel* delay_model,
                                PlacerCriticalities* criticalities,
                                PlacerSetupSlacks* setup_slacks,
                                ClusteredPinTimingInvalidator* pin_timing_invalidator,
                                SetupTimingInfo* timing_info,
                                t_placer_costs* costs);

///@brief Update timing information based on the current block positions.
void update_timing_classes(const PlaceCritParams& crit_params,
                           SetupTimingInfo* timing_info,
                           PlacerCriticalities* criticalities,
                           PlacerSetupSlacks* setup_slacks,
                           ClusteredPinTimingInvalidator* pin_timing_invalidator);

///@brief Updates the timing driven (td) costs.
void update_timing_cost(const PlaceDelayModel* delay_model,
                        const PlacerCriticalities* criticalities,
                        double* timing_cost);

///@brief Incrementally updates timing cost based on the current delays and criticality estimates.
void update_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost);

///@brief Recomputes timing cost from scratch based on the current delays and criticality estimates.
void comp_td_costs(const PlaceDelayModel* delay_model, const PlacerCriticalities& place_crit, double* timing_cost);

/**
 * @brief Commit all the setup slack values from the PlacerSetupSlacks
 *        class to `connection_setup_slack`.
 */
void commit_setup_slacks(const PlacerSetupSlacks* setup_slacks);

///@brief Verify that the values in `connection_setup_slack` matches PlacerSetupSlacks.
bool verify_connection_setup_slacks(const PlacerSetupSlacks* setup_slacks);
