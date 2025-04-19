#pragma once

#include "centroid_move_generator.h"

class PlaceMacros;

/**
 * @brief Weighted Centroid move generator
 *
 * This move generator is inspired by analytical placers: model net connections as springs and 
 * calculate the force equilibrium location.
 *
 * @details This class inherits from CentroidMoveGenerator to avoid code duplication.
 *
 * For more details, please refer to:
 * "Learn to Place: FPGA Placement using Reinforcement Learning and Directed Moves", ICFPT2020
 */
class WeightedCentroidMoveGenerator : public CentroidMoveGenerator {
  public:
    WeightedCentroidMoveGenerator() = delete;
    WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                  const PlaceMacros& place_macros,
                                  const NetCostHandler& net_cost_handler,
                                  e_reward_function reward_function,
                                  vtr::RngContainer& rng);
};
