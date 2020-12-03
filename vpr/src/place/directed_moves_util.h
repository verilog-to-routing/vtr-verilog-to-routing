#ifndef VPR_DIRECTED_MOVES_UTIL_H
#define VPR_DIRECTED_MOVES_UTIL_H

#include "globals.h"
#include "timing_place.h"

///@brief Helper function that returns the x, y coordinates of a pin
void get_coordinate_of_pin(ClusterPinId pin, int& x, int& y);

/**
 * @brief Calculates the exact centroid location
 *
 * This function is very useful in centroid and weightedCentroid moves as it calculates
 * the centroid location. It returns the calculated location in centroid.
 *
 *      @param b_from: The block Id of the moving block
 *      @param timing_weights: Determines whether to calculate centroid or weighted centroid location. If true, use the timing weights (weighted centroid)
 *      @param criticalities: A pointer to the placer criticalities which is used when calculating weighted centroid (send a NULL pointer in case of centroid)
 * 
 *      @return The calculated location is returned in centroid parameter that is sent by reference
 */
void calculate_centroid_loc(ClusterBlockId b_from, bool timing_weights, t_pl_loc& centroid, const PlacerCriticalities* criticalities);

#endif
