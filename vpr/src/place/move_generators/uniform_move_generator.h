#pragma once

#include "move_generator.h"

class PlaceMacros;

/**
 * @brief The classic VPR move generator
 *
 * randomly picks a from_block with equal probabilities for all blocks, and then moves it randomly within 
 * a range limit centered on from_block in the compressed block grid space
 */
class UniformMoveGenerator : public MoveGenerator {
  public:
    UniformMoveGenerator() = delete;
    UniformMoveGenerator(PlacerState& placer_state,
                         const PlaceMacros& place_macros,
                         const NetCostHandler& net_cost_handler,
                         e_reward_function reward_function,
                         vtr::RngContainer& rng);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& /*placer_opts*/,
                               const PlacerCriticalities* /*criticalities*/) override;
};
