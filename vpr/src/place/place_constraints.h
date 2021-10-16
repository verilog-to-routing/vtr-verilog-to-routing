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
#include "partition_region.h"
#include "place_macro.h"
#include "grid_tile_lookup.h"

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

/*
 * Marks blocks as fixed if they have a constraint region that specifies exactly one x, y,
 * subtile location as legal.
 * Marking them as fixed indicates that they cannot be moved during initial placement and simulated annealing
 */
void mark_fixed_blocks();

/*
 * Returns the number of tiles covered by a floorplan region.
 * The return value of this routine will either be 0, 1, or 2. This
 * is because this routine is used to check whether the region covers no tile,
 * one tile, or more than one tile, and so as soon as it is seen that the number of tiles
 * covered is 2, no further information is needed.
 */
int region_tile_cover(const Region& reg, t_logical_block_type_ptr block_type, t_pl_loc& loc);

/*
 * Returns a bool that indicates if the PartitionRegion covers exactly one compatible location.
 * Used to decide whether to mark a block with the .is_fixed flag based on its floorplan
 * region.
 * block_type is used to determine whether the PartitionRegion is compatible with the cluster block type
 * and loc is updated with the location covered by the PartitionRegion
 */
bool is_pr_size_one(PartitionRegion& pr, t_logical_block_type_ptr block_type, t_pl_loc& loc);

/*
 * Returns the number of grid tiles that are covered by the partition region and
 * compatible with the cluster's block type.
 * Used prior to initial placement to help sort blocks based on how difficult they
 * are to place.
 */
int get_part_reg_size(PartitionRegion& pr, t_logical_block_type_ptr block_type, GridTileLookup& grid_tiles);

/*
 * Return the floorplan score that will be used for sorting blocks during initial placement. This score is the
 * total number of subtilesfor the block type in the grid, minus the number of subtiles in the block's floorplan PartitionRegion.
 * The resulting number is the number of tiles outside the block's floorplan region, meaning the higher
 * it is, the more difficult the block is to place.
 */
int get_floorplan_score(ClusterBlockId blk_id, PartitionRegion& pr, t_logical_block_type_ptr block_type, GridTileLookup& grid_tiles);

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
