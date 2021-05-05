/*
 * place_constraints.h
 *
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */
#include "move_transactions.h"

#ifndef VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_
#    define VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_

/*
 * Check that placement of each block is within the floorplan constraint region of that block (if the block has any constraints).
 * Returns the number of errors (inconsistencies in adherence to floorplanning constraints).
 */
int check_placement_floorplanning();

/*
 * Check if the block has floorplanning constraints
 */
bool is_cluster_constrained(ClusterBlockId blk_id);

/*
 * Check if the placement location would respect floorplan constraints of the block, if it has any
 */
bool cluster_floorplanning_legal(ClusterBlockId blk_id, const t_pl_loc& loc);

/*
 * Check whether any member of the macro has floorplan constraints
 */
bool is_macro_constrained(const t_pl_macro& pl_macro);

/*
 * Returns region of valid locations for the head of the macro based on floorplan constraints
 */
PartitionRegion constrained_macro_locs(const t_pl_macro& pl_macro);

inline bool floorplan_legal(const t_pl_blocks_to_be_moved& blocks_affected) {
    bool floorplan_legal;

    for (int i = 0; i < blocks_affected.num_moved_blocks; i++) {
        floorplan_legal = cluster_floorplanning_legal(blocks_affected.moved_blocks[i].block_num, blocks_affected.moved_blocks[i].new_loc);
        if (!floorplan_legal) {
#    ifdef VERBOSE
            VTR_LOG("Move aborted for block %zu, location tried was x: %d, y: %d, subtile: %d \n", size_t(blocks_affected.moved_blocks[i].block_num), blocks_affected.moved_blocks[i].new_loc.x, blocks_affected.moved_blocks[i].new_loc.y, blocks_affected.moved_blocks[i].new_loc.sub_tile);
#    endif
            return false;
        }
    }
    return true;
}

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
