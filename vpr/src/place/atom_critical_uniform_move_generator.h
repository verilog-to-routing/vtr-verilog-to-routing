//
// Created by amin on 5/10/23.
//

#ifndef VTR_PRIMITIVE_CRITICAL_UNIFORM_MOVE_GENERATOR_H
#define VTR_PRIMITIVE_CRITICAL_UNIFORM_MOVE_GENERATOR_H

#include "move_generator.h"
#include "timing_place.h"

/**
 * @file
 * @author Amin Mohaghegh
 * @brief Primitive critical uniform move type
 *
 * This move picks a random block from the the critical blocks (those with one or more critical nets)
 * and moves it (swapping with what's there if necessary) to a random location within rlim units
 * away in the x and y dimensions in the compressed block grid.
 *
 * Returns its choices by filling in affected_blocks.
 */
class AtomCriticalUniformMoveGenerator : public MoveGenerator {
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, t_propose_action& /* proposed_action */,
                               float rlim, const t_placer_opts& /* placer_opts */, const PlacerCriticalities* /* criticalities */) override;
};
#endif //VTR_PRIMITIVE_CRITICAL_UNIFORM_MOVE_GENERATOR_H
