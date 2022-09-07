#ifndef VPR_INITIAL_PLACEMENT_H
#define VPR_INITIAL_PLACEMENT_H

#include "vpr_types.h"

/**
 * @brief keeps track of available empty locations of a specific block type during initial placement.
 * Used to densly place macros that failed to be placed in the first initial placement iteration (random placement)
 */
struct t_grid_empty_locs_block_type {
    /*
     * First available location of a block type that has not been occupied yet.
     * It will be initialized to the leftmost and lowest (x,y) location of the block type in grid.
     */
    t_pl_loc first_avail_loc;
    /*
     * Number of consecutive y-locations starting (and including) from first_avail_loc
     * that can accomadate blocks of the type at first_avail_loc.
     */
    int num_of_empty_locs_in_y_axis;
};

/**
 * @brief Tries to find an initial placement location for each block considering floorplanning constraints
 * and throws an error out if it fails after max number of attempts.
 *   
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param constraints_file Used to read block locations if any constraints is available.
 */
void initial_placement(enum e_pad_loc_type pad_loc_type, const char* constraints_file);

/**
 * @brief Looks for a valid placement location for block.
 *   
 *   @param blk_id The block that should be placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid First location (lowest y) and number of remaining blocks in each column for the blk_id type
 *   
 * 
 * @return true if the block gets placed, false if not.
 */
bool place_one_block(const ClusterBlockId& blk_id, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);
#endif
