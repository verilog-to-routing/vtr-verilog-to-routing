#ifndef VPR_INITIAL_PLACEMENT_H
#define VPR_INITIAL_PLACEMENT_H

class NocCostHandler;

#include <optional>

#include "place_macro.h"
#include "partition_region.h"

#include "vpr_types.h"
#include "vtr_vector_map.h"

class BlkLocRegistry;

/* The maximum number of tries when trying to place a macro at a
 * random location before trying exhaustive placement - find the first
 * legal position and place it during initial placement. */
constexpr int MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY = 8;

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
 * Used to densely place macros that failed to be placed in the first initial placement iteration (random placement)
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
 * @brief  tries to place a macro at a random location
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints,
 *   is the size of the whole chip if the macro is not constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the macro gets placed, false if not.
 */
bool try_place_macro_randomly(const t_pl_macro& pl_macro,
                              const PartitionRegion& pr,
                              t_logical_block_type_ptr block_type,
                              e_pad_loc_type pad_loc_type,
                              BlkLocRegistry& blk_loc_registry);


/**
 * @brief Looks for a valid placement location for macro exhaustively once the maximum number of random locations have been tried.
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the macro gets placed, false if not.
 */
bool try_place_macro_exhaustively(const t_pl_macro& pl_macro,
                                  const PartitionRegion& pr,
                                  t_logical_block_type_ptr block_type,
                                  e_pad_loc_type pad_loc_type,
                                  BlkLocRegistry& blk_loc_registry);

/**
 * @brief Places the macro if the head position passed in is legal, and all the resulting
 * member positions are legal
 *
 *   @param pl_macro The macro to be placed.
 *   @param head_pos The location of the macro head member.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if macro was placed, false if not.
 */
bool try_place_macro(const t_pl_macro& pl_macro,
                     t_pl_loc head_pos,
                     BlkLocRegistry& blk_loc_registry);

/**
 * @brief Checks whether the block is already placed
 *
 *   @param blk_id block id of the block to be checked
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 *
 * @return true if the block was placed, false if not.
 */
bool is_block_placed(ClusterBlockId blk_id,
                     const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

/**
 * @brief Tries to find an initial placement location for each block considering floorplanning constraints
 * and throws an error out if it fails after max number of attempts.
 * If the noc optimization is enabled then after initial placement
 * the NoC is initialized by routing all the traffic
 * flows and updating the bandwidths used by the links due to the
 * traffic flows.
 *
 *   @param placer_opts Required by the function that set the status of f_placer_debug.
 *   Also used to access pad_loc_type to see if a block needs to be marked fixed.
 *   @param constraints_file Used to read block locations if any constraints is available.
 *   @param noc_opts Contains information about if the NoC optimization is enabled
 *   and NoC-related weighting factors.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 */
void initial_placement(const t_placer_opts& placer_opts,
                       const char* constraints_file,
                       const t_noc_opts& noc_opts,
                       BlkLocRegistry& blk_loc_registry,
                       std::optional<NocCostHandler>& noc_cost_handler);

/**
 * @brief Looks for a valid placement location for block.
 *   
 *   @param blk_id The block that should be placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid First location (lowest y) and number of remaining blocks in each column for the blk_id type
 *   @param block_scores Scores assign to different blocks to determine which one should be placed first.
 *   @param blk_loc_registry Placement block location information. To be filled with the location
 *   where pl_macro is placed.
 * 
 * @return true if the block gets placed, false if not.
 */
bool place_one_block(const ClusterBlockId blk_id,
                     e_pad_loc_type pad_loc_type,
                     std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid,
                     vtr::vector<ClusterBlockId, t_block_score>* block_scores,
                     BlkLocRegistry& blk_loc_registry);



#endif
