#pragma once

#include "move_generator.h"

class PlaceMacros;

/**
 * @brief Feasible Region (FR) move generator
 *
 * This move was originally defined by Chen et al . in "Simultaneous timing-driven placement and duplication", FPGA 2005
 *
 * A feasible region is the location/area where a block can be placed to minimize the critical path delay.
 * The move is designed to choose one of the highly critical blocks (a block with one or more critical nets) and move it 
 * to a random location inside this feasible region.
 * 
 * The FR is calculated as follows:
 *      - Choose a random block from the highly critical blocks
 *      - Identify the highly critical inputs to the block and the most critical output
 *      - Follow the algorithm proposed in Chen et al.'s work
 *
 */
class FeasibleRegionMoveGenerator : public MoveGenerator {
  public:
    FeasibleRegionMoveGenerator() = delete;
    FeasibleRegionMoveGenerator(PlacerState& placer_state,
                                const PlaceMacros& place_macros,
                                const NetCostHandler& net_cost_handler,
                                e_reward_function reward_function,
                                vtr::RngContainer& rng);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* criticalities) override;
};
