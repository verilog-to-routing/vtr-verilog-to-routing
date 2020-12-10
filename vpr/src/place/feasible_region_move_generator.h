#ifndef VPR_FEASIBLE_REGION_MOVE_GEN_H
#define VPR_FEASIBLE_REGION_MOVE_GEN_H
#include "move_generator.h"
#include "timing_place.h"

/**
 * @brief Feasible Reion (FR) move genrator
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
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, e_move_type& /*move_type*/, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities);
};

#endif
