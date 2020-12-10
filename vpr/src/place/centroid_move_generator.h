#ifndef VPR_CENTROID_MOVE_GEN_H
#define VPR_CENTROID_MOVE_GEN_H
#include "move_generator.h"

/**
 * @file
 * @author M. Elgammal
 * @brief Centroid move type
 *
 * This move picks a random block and moves it (swapping with what's there if necessary) to the zero force location
 * It calculates forces/weighs acting on the block based on its connections to other blocks.
 *
 * Returns its choices by filling in affected_blocks.
 */
class CentroidMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* /*criticalities*/);
};

#endif
