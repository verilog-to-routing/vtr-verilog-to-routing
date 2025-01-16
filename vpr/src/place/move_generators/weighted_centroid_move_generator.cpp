#include "weighted_centroid_move_generator.h"

WeightedCentroidMoveGenerator::WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                                             e_reward_function reward_function,
                                                             vtr::RngContainer& rng)
    : CentroidMoveGenerator(placer_state, reward_function, rng) {
    weighted_ = true;
}
