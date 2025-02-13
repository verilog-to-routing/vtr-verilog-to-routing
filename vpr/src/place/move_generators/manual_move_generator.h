/*
 * @file 	manual_move_generator.h
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the ManualMoveGenerator class.
 */

#ifndef VPR_MANUAL_MOVE_GEN_H
#define VPR_MANUAL_MOVE_GEN_H

#include "move_generator.h"

class PlaceMacros;

/**
 * @brief Manual Moves Generator, inherits from MoveGenerator class.
 *
 * Manual Move Generator, needed for swapping blocks requested by the user.
 */
class ManualMoveGenerator : public MoveGenerator {
  public:
    ManualMoveGenerator() = delete;
    ManualMoveGenerator(PlacerState& placer_state, vtr::RngContainer& rng);

    //Evaluates if move is successful and legal or unable to do.
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& /*proposed_action*/,
                               float /*rlim*/,
                               const PlaceMacros& place_macros,
                               const t_placer_opts& /*placer_opts*/,
                               const PlacerCriticalities* /*criticalities*/) override;
};

#endif /*VPR_MANUAL_MOVE_GEN_H */
