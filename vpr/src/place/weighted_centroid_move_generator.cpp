#include "weighted_centroid_move_generator.h"

WeightedCentroidMoveGenerator::WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                                             e_reward_function reward_function)
    : CentroidMoveGenerator(placer_state, reward_function) {
    weighted_ = true;
}
