#ifndef VPR_INITIAL_PLACEMENT_H
#define VPR_INITIAL_PLACEMENT_H

#include "vpr_types.h"

/**
 * @brief Used to assign each block a score for how difficult it is to place. 
 * The higher numbers indicate a block is expected to be more difficult to place.
 * Hence, initial placement tries to place blocks with higher scores earlier.
 */
struct t_block_score {
    int macro_size = 0; //How many members does the macro have, if the block is part of one - this value is zero if the block is not in a macro

    //The number of tiles NOT covered by the block's floorplan constraints.
    double tiles_outside_of_floorplan_constraints = 0;

    //The number of initial placement iterations that the block was unplaced.
    int failed_to_place_in_prev_attempts;

    //The number of placed block during initial placement that are connected to the this block.
    int number_of_placed_connections = 0;
};

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
 * If the noc optimization is enabled then after initial placement
 * the NoC is initialized by routing all the traffic
 * flows and updating the bandwidths used by the links due to the
 * traffic flows.
 *
 *   @param placer_opts Required by the function that set the status of f_placer_debug
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param constraints_file Used to read block locations if any constraints is available.
 *   @param noc_enabled Used to check whether the user turned on the noc
 * optimization during placement.
 */
void initial_placement(const t_placer_opts& placer_opts,
                       enum e_pad_loc_type pad_loc_type,
                       const char* constraints_file,
                       const t_noc_opts& noc_opts);

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
bool place_one_block(const ClusterBlockId& blk_id, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid, vtr::vector<ClusterBlockId, t_block_score>* block_scores);
#endif
