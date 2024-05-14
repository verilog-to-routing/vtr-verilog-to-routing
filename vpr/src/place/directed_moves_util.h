#ifndef VPR_DIRECTED_MOVES_UTIL_H
#define VPR_DIRECTED_MOVES_UTIL_H

#include "globals.h"
#include "timing_place.h"

/**
 * @brief enum represents the different reward functions
 */
enum e_reward_function {
    BASIC,                  ///@ directly uses the change of the annealing cost function
    NON_PENALIZING_BASIC,   ///@ same as basic reward function but with 0 reward if it's a hill-climbing one
    RUNTIME_AWARE,          ///@ same as NON_PENALIZING_BASIC but with normalizing with the runtime factor of each move type
    WL_BIASED_RUNTIME_AWARE ///@ same as RUNTIME_AWARE but more biased to WL cost (the factor of the bias is REWARD_BB_TIMING_RELATIVE_WEIGHT)
};

e_reward_function string_to_reward(const std::string& st);

///@brief Helper function that returns the x, y coordinates of a pin
void get_coordinate_of_pin(ClusterPinId pin, t_physical_tile_loc& tile_loc);

/**
 * @brief Calculates the exact centroid location
 *
 * This function is very useful in centroid and weightedCentroid moves as it calculates
 * the centroid location. It returns the calculated location in centroid.
 *
 * When NoC attraction is enabled, the computed centroid is slightly adjusted towards the location
 * of NoC routers that are in the same NoC group b_from.
 *
 * @param b_from The block Id of the moving block
 * @param timing_weights Determines whether to calculate centroid or
 * weighted centroid location. If true, use the timing weights (weighted centroid).
 * @param criticalities A pointer to the placer criticalities which is used when
 * calculating weighted centroid (send a NULL pointer in case of centroid)
 * @param noc_attraction_enabled Indicates whether the computed centroid location
 * should be adjusted towards NoC routers in the NoC group of the given block.
 * @param noc_attraction_weight When NoC attraction is enabled, this weight
 * specifies to which extent the computed centroid should be adjusted. A value
 * in range [0, 1] is expected.
 * 
 * @return The calculated location is returned in centroid parameter that is sent by reference
 */
void calculate_centroid_loc(ClusterBlockId b_from,
                            bool timing_weights,
                            t_pl_loc& centroid,
                            const PlacerCriticalities* criticalities,
                            bool noc_attraction_enabled,
                            float noc_attraction_weight);

inline void calculate_centroid_loc(ClusterBlockId b_from,
                                   bool timing_weights,
                                   t_pl_loc& centroid,
                                   const PlacerCriticalities* criticalities) {
    calculate_centroid_loc(b_from, timing_weights, centroid, criticalities, false, 0.0f);
}

#endif
