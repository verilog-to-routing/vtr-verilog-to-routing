/*
 * place_constraints.h
 *
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */
#include "move_transactions.h"
#include "region.h"
#include "clustered_netlist_utils.h"

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
 * Returns PartitionRegion for the head of the macro based on the floorplan constraints
 * of all blocks in the macro. For example, if there was a macro of length two and each block has
 * a constraint, this routine will shift and intersect the two constraint regions to calculate
 * the tightest region constraint for the head macro.
 */
PartitionRegion update_macro_head_pr(const t_pl_macro& pl_macro, const PartitionRegion& grid_pr);

/*
 * Update the PartitionRegions of non-head members of a macro,
 * based on the constraint that was calculated for the head region, head_pr.
 * The constraint on the head region must be the tightest possible (i.e. implied by the
 * entire macro) before this routine is called.
 * For each macro member, the updated constraint is essentially the head constraint
 * with the member's offset applied.
 */
PartitionRegion update_macro_member_pr(PartitionRegion& head_pr, const t_pl_offset& offset, const PartitionRegion& grid_pr, const t_pl_macro& pl_macro);

/*
 * Updates the floorplan constraints information for all constrained macros.
 * Updates the constraints to be the tightest constraints possible while adhering
 * to the floorplan constraints of each macro member.
 * This is done at the start of initial placement to ease floorplan legality checking
 * while placing macros during initial placement.
 */
void propagate_place_constraints();

void print_macro_constraint_error(const t_pl_macro& pl_macro);

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

/*
 * Load cluster_constraints if the pack stage of VPR is skipped. The cluster_constraints
 * data structure is normally loaded during packing, so this routine is called when the packing stage is not performed.
 * If no constraints file is specified, every cluster is assigned
 * an empty PartitionRegion. If a constraints file is specified, cluster_constraints is loaded according to
 * the floorplan constraints specified in the file.
 * Load cluster_constraints according to the floorplan constraints specified in
 * the constraints XML file.
 */
void load_cluster_constraints();

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
