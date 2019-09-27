#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"

/* Stores the information of the move for a block that is       *
 * moved during placement                                       *
 * block_num: the index of the moved block                      *
 * xold: the x_coord that the block is moved from               *
 * xnew: the x_coord that the block is moved to                 *
 * yold: the y_coord that the block is moved from               *
 * xnew: the x_coord that the block is moved to                 */
struct t_pl_moved_block {
    ClusterBlockId block_num;
    t_pl_loc old_loc;
    t_pl_loc new_loc;
};

/* Stores the list of blocks to be moved in a swap during       *
 * placement.                                                   *
 * num_moved_blocks: total number of blocks moved when          *
 *                   swapping two blocks.                       *
 * moved blocks: a list of moved blocks data structure with     *
 *               information on the move.                       *
 *               [0...num_moved_blocks-1]                       */
struct t_pl_blocks_to_be_moved {
    int num_moved_blocks;
    std::vector<t_pl_moved_block> moved_blocks;
    std::unordered_set<t_pl_loc> moved_from;
    std::unordered_set<t_pl_loc> moved_to;
};

enum class e_find_affected_blocks_result {
    VALID,       //Move successful
    ABORT,       //Unable to perform move
    INVERT,      //Try move again but with from/to inverted
    INVERT_VALID //Completed inverted move
};

e_find_affected_blocks_result record_block_move(ClusterBlockId blk, t_pl_loc to);

void apply_move_blocks();

void commit_move_blocks();

void revert_move_blocks();

void clear_move_blocks();

#endif
