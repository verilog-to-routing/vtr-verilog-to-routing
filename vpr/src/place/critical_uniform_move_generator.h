#ifndef VPR_CRITICAL_UNIFORM_MOVE_GEN_H
#define VPR_CRITICAL_UNIFORM_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"

/**
 * @file 
 * @author M. Elgammal
 * @brief Critical uniform move type
 *
 * This move picks a random block from the the critical blocks (those with one or more critical nets)
 * and moves it (swapping with what's there if necessary) to a random location within rlim units 
 * away in the x and y dimensions in the compressed block grid.
 * 
 * Returns its choices by filling in affected_blocks.
 */
class CriticalUniformMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& /*placer_opts*/, const PlacerCriticalities* /*criticalities*/);
};

#endif
