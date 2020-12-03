#ifndef VPR_DIRECTED_MOVES_UTIL_H
#define VPR_DIRECTED_MOVES_UTIL_H

#include "globals.h"
#include "timing_place.h"
#include "move_generator.h"

#include "static_move_generator.h"
#include "simpleRL_move_generator.h"

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

/**
 * @brief Creates the move generators that will be used by the annealer
 *
 * This function creates 2 move generators to be used by the annealer. The type of the move generators created here depends on the 
 * type selected in placer_opts.
 * It returns a unique pointer for each move generator in move_generator and move_generator2
 * move_lim: represents the num of moves per temp.
 */
void create_move_generators(std::unique_ptr<MoveGenerator>& move_generator, std::unique_ptr<MoveGenerator>& move_generator2, const t_placer_opts& placer_opts, int move_lim);
#endif
