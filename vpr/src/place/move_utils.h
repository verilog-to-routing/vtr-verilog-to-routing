#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"

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
