#ifndef VPR_WEIGHTED_CENTROID_MOVE_GEN_H
#define VPR_WEIGHTED_CENTROID_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"

/**
 * @brief Weighted Centroid move generator
 *
 * This move generator is inspired by analytical placers: model net connections as springs and 
 * calculate the force equilibrium location. 
 *
 * For more details, please refer to:
 * "Learn to Place: FPGA Placement using Reinforcement Learning and Directed Moves", ICFPT2020
 */
class WeightedCentroidMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities);
};

#endif
