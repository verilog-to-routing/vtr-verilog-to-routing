#ifndef VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_
#define VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_

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

/**
 * @brief Check that placement of each block is within the floorplan constraint region
 * of that block (if the block has any constraints).
 *
 * @return int The number of errors (inconsistencies in adherence to floorplanning constraints).
 */
int check_placement_floorplanning();


/**
 * @brief Check if the block has floorplanning constraints.
 *
 * @param blk_id The ID of the clustered block to be checked.
 * @return bool True if the block has floorplanning constraints, false otherwise.
 */
bool is_cluster_constrained(ClusterBlockId blk_id);

/**
 * @brief Check if the placement location would respect floorplan constraints of the block, if it has any.
 *
 * This function determines whether placing a block at a specified location adheres to its floorplan constraints.
 *
 * @param blk_id The ID of the clustered block to be checked.
 * @param loc The location where the block is to be placed.
 * @return bool True if the placement location respects the block's floorplan constraints, false otherwise.
 */
bool cluster_floorplanning_legal(ClusterBlockId blk_id, const t_pl_loc& loc);


/**
 * @brief Check whether any member of the macro has floorplan constraints.
 *
 * @param pl_macro The macro to be checked.
 * @return bool True if any member of the macro has floorplan constraints, false otherwise.
 */
bool is_macro_constrained(const t_pl_macro& pl_macro);

/**
 * @brief Returns PartitionRegion for the head of the macro based on the floorplan constraints
 * of all blocks in the macro.
 *
 * This function calculates the PartitionRegion for the head of a macro by considering the
 * floorplan constraints of all blocks in the macro. For example, if a macro has two blocks
 * and each block has a constraint, this routine will shift and intersect the two constraint
 * regions to determine the tightest region constraint for the macro's head.
 *
 * @param pl_macro The macro whose head's PartitionRegion is to be calculated.
 * @return PartitionRegion The calculated PartitionRegion for the head of the macro.
 */
PartitionRegion update_macro_head_pr(const t_pl_macro& pl_macro);

/**
 * @brief Update the PartitionRegions of non-head members of a macro,
 * based on the constraint that was calculated for the head region, head_pr.
 *
 * The constraint on the head region must be the tightest possible (i.e., implied by the
 * entire macro) before this routine is called. For each macro member, the updated constraint
 * is essentially the head constraint with the member's offset applied.
 *
 * @param head_pr The PartitionRegion constraint of the macro's head.
 * @param offset The offset of the macro member from the head.
 * @param pl_macro The placement macro whose members' PartitionRegions are to be updated.
 * @param grid_pr A PartitionRegion that covers the entire device.
 * @return PartitionRegion The updated PartitionRegion for the macro member.
 */
PartitionRegion update_macro_member_pr(const PartitionRegion& head_pr,
                                       const t_pl_offset& offset,
                                       const t_pl_macro& pl_macro,
                                       const PartitionRegion& grid_pr);

/**
 * @brief Updates the floorplan constraints information for all constrained macros.
 *
 * Updates the constraints to be the tightest constraints possible while adhering
 * to the floorplan constraints of each macro member. This is done at the start of
 * initial placement to ease floorplan legality checking while placing macros during
 * initial placement.
 */
void propagate_place_constraints();

void print_macro_constraint_error(const t_pl_macro& pl_macro);

inline bool floorplan_legal(const t_pl_blocks_to_be_moved& blocks_affected) {
    bool floorplan_legal;

    for (int i = 0; i < blocks_affected.num_moved_blocks; i++) {
        floorplan_legal = cluster_floorplanning_legal(blocks_affected.moved_blocks[i].block_num,
                                                      blocks_affected.moved_blocks[i].new_loc);
        if (!floorplan_legal) {
            VTR_LOGV_DEBUG(g_vpr_ctx.placement().f_placer_debug,
                           "\tMove aborted for block %zu, location tried was x: %d, y: %d, subtile: %d \n",
                           size_t(blocks_affected.moved_blocks[i].block_num),
                           blocks_affected.moved_blocks[i].new_loc.x,
                           blocks_affected.moved_blocks[i].new_loc.y,
                           blocks_affected.moved_blocks[i].new_loc.sub_tile);
            return false;
        }
    }
    return true;
}


/**
 * @brief Load cluster_constraints if the pack stage of VPR is skipped.
 *
 * The cluster_constraints data structure is normally loaded during packing,
 * so this routine is called when the packing stage is not performed.
 * If no constraints file is specified, every cluster is assigned an empty PartitionRegion.
 * If a constraints file is specified, cluster_constraints is loaded according to
 * the floorplan constraints specified in the file.
 *
 * @note Load cluster_constraints according to the floorplan constraints specified in the constraints XML file.
 */
void load_cluster_constraints();

/**
 * @brief Marks blocks as fixed if they have a constraint region that
 * specifies exactly one x, y, subtile location as legal.
 *
 * Marking them as fixed indicates that they cannot be moved
 * during initial placement and simulated annealing.
 */
void mark_fixed_blocks();

/**
 * @brief Converts the floorplanning constraints from grid location to
 * compressed grid locations and store them in FloorplanningContext.
 */
void alloc_and_load_compressed_cluster_constraints();

/**
 * @brief Returns the number of tiles covered by a floorplan region.
 *
 * The return value of this routine will either be 0, 1, or 2. This
 * is because this routine is used to check whether the region covers no tile,
 * one tile, or more than one tile, and so as soon as it is seen that the number of tiles
 * covered is 2, no further information is needed.
 *
 * @param reg The region to be checked.
 * @param block_type The type of logical block.
 * @param loc The location of the tile, if only one tile is covered.
 * @return int The number of tiles covered by the region.
 */
int region_tile_cover(const Region& reg, t_logical_block_type_ptr block_type, t_pl_loc& loc);

/**
 * @brief Returns a bool that indicates if the PartitionRegion covers exactly one compatible location.
 *
 * Used to decide whether to mark a block with the .is_fixed flag based on its floorplan region.
 * block_type is used to determine whether the PartitionRegion is compatible with the cluster block type
 * and loc is updated with the location covered by the PartitionRegion.
 *
 * @param pr The PartitionRegion to be checked.
 * @param block_type The type of logical block.
 * @param loc The location covered by the PartitionRegion, if it covers exactly one compatible location.
 * @return bool True if the PartitionRegion covers exactly one compatible location, false otherwise.
 */
bool is_pr_size_one(const PartitionRegion& pr, t_logical_block_type_ptr block_type, t_pl_loc& loc);

/**
 * @brief Returns the number of grid tiles that are covered by the partition region
 * and compatible with the cluster's block type.
 *
 * Used prior to initial placement to help sort blocks based on how difficult they are to place.
 *
 * @param pr The PartitionRegion to be checked.
 * @param block_type The type of logical block.
 * @param grid_tiles The GridTileLookup containing information about the number of available subtiles
 * compatible the given block_type.
 * @return int The number of compatible grid tiles covered by the PartitionRegion.
 */
int get_part_reg_size(const PartitionRegion& pr,
                      t_logical_block_type_ptr block_type,
                      const GridTileLookup& grid_tiles);


/**
 * @brief Return the floorplan score that will be used for sorting blocks during initial placement.
 *
 * This score is the total number of subtiles for the block type in the grid, minus the number of subtiles
 * in the block's floorplan PartitionRegion. The resulting number is the number of tiles outside the block's
 * floorplan region, meaning the higher it is, the more difficult the block is to place.
 *
 * @param blk_id The ID of the cluster block.
 * @param pr The PartitionRegion representing the floorplan region of the block.
 * @param block_type The type of logical block.
 * @param grid_tiles The GridTileLookup containing information about the number of available subtiles
 * compatible the given block_type.
 * @return double The floorplan score for the block.
 */
double get_floorplan_score(ClusterBlockId blk_id,
                           const PartitionRegion& pr,
                           t_logical_block_type_ptr block_type,
                           const GridTileLookup& grid_tiles);


#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
