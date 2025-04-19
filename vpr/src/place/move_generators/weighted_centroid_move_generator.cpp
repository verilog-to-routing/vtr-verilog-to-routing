#include "weighted_centroid_move_generator.h"
#include "place_macro.h"

WeightedCentroidMoveGenerator::WeightedCentroidMoveGenerator(PlacerState& placer_state,
                                                             const PlaceMacros& place_macros,
                                                             const NetCostHandler& net_cost_handler,
                                                             e_reward_function reward_function,
                                                             vtr::RngContainer& rng)
    : CentroidMoveGenerator(placer_state, place_macros, net_cost_handler, reward_function, rng) {
    weighted_ = true;
}
