#include "weighted_centroid_move_generator.h"
#include "place_macro.h"

WeightedCentroidMoveGenerator::WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                                             const PlaceMacros& place_macros,
                                                             e_reward_function reward_function,
                                                             vtr::RngContainer& rng)
    : CentroidMoveGenerator(placer_state, place_macros, reward_function, rng) {
    weighted_ = true;
}
