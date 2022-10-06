#include "vtr_memory.h"
#include "vtr_random.h"
#include "vtr_time.h"

#include "globals.h"
#include "read_place.h"
#include "initial_placement.h"
#include "vpr_utils.h"
#include "place_util.h"
#include "place_constraints.h"
#include "move_utils.h"
#include "region.h"

#include "echo_files.h"

#include <chrono>
#include <time.h>

#ifdef VERBOSE
void print_clb_placement(const char* fname);
#endif

/**
 * @brief Used to assign each block a score for how difficult it is to place. 
 * The higher numbers indicate a block is expected to be more difficult to place.
 * Hence, initial placement tries to place blocks with higher scores earlier.
 */
struct t_block_score {
    int macro_size = 0; //How many members does the macro have, if the block is part of one - this value is zero if the block is not in a macro

    //The number of tiles NOT covered by the block's floorplan constraints.
    int tiles_outside_of_floorplan_constraints = 0;

    //The number of initial placement iterations that the block was unplaced.
    int failed_to_place_in_prev_attempts;
};

/// @brief Sentinel value for indicating that a block does not have a valid x location, used to check whether a block has been placed
constexpr int INVALID_X = -1;

// Number of iterations that initial placement tries to place all blocks before throwing an error
#define MAX_INIT_PLACE_ATTEMPTS 2

// The amount of weight that will added to previous unplaced block scores to ensure that failed blocks would be placed earlier next iteration
#define SORT_WEIGHT_PER_FAILED_BLOCK 10

/* The maximum number of tries when trying to place a macro at a    *
 * random location before trying exhaustive placement - find the first     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 8

/// @brief

/**
 * @brief Set choosen grid locations to EMPTY block id before each placement iteration
 *   
 *   @param unplaced_blk_types_index Block types that their grid locations must be cleared.
 * 
 */
static void clear_block_type_grid_locs(std::unordered_set<int> unplaced_blk_types_index);

/**
 * @brief Places the macro if the head position passed in is legal, and all the resulting
 * member positions are legal
 *   
 *   @param pl_macro The macro to be placed.
 *   @param head_pos The location of the macro head member.
 * 
 * @return true if macro was placed, false if not.
 */
static bool try_place_macro(t_pl_macro pl_macro, t_pl_loc head_pos);

/**
 * @brief Control routine for placing a macro.
 * First iterations calls random placement for the max number of tries,then calls the exhaustive placement routine.
 * If first iteration failed, next iteration calls dense placement for specific block types.
 *  
 *   @param macros_max_num_tries Max number of tries for initial placement before switching to exhaustive placement. 
 *   @param pl_macro The macro to be placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type
 * 
 * @return true if macro was placed, false if not.
 */
static bool place_macro(int macros_max_num_tries, t_pl_macro pl_macro, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/*
 * Assign scores to each block based on macro size and floorplanning constraints.
 * Used for relative placement, so that the blocks that are more difficult to place can be placed first during initial placement.
 * A higher score indicates that the block is more difficult to place.
 */
static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores();

//Sort the blocks according to how difficult they are to place, prior to initial placement
static std::vector<ClusterBlockId> sort_blocks(const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

/**
 * @brief Tries to find y coordinate for macro head location based on macro direction
 *   
 *
 *   @param first_macro_loc The first available location that can place the macro blocks.
 *   @param pl_macro The macro to be placed.
 *
 * @return y coordinate of the location that macro head should be placed
 */
static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc, t_pl_macro pl_macro);

/**
 * @brief Tries to get the first available location of a specific block type that can accomodate macro blocks
 *
 *   @param loc The first available location that can place the macro blocks.
 *   @param pl_macro The macro to be placed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type 
 *
 * @return index to a column of blk_types_empty_locs_in_grid that can accomodate pl_macro and location of first available location returned by reference
 */
static int get_blk_type_first_loc(t_pl_loc& loc, t_pl_macro pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/**
 * @brief Updates the first available location (lowest y) and number of remaining blocks in the column that dense placement used to place the macro.
 *
 *   @param blk_type_column_index Index to a column in blk_types_empty_locs_in_grid that placed pl_macro in itself.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pl_macro The macro to be placed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type 
 * 
 */
static void update_blk_type_first_loc(int blk_type_column_index, t_logical_block_type_ptr block_type, t_pl_macro pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/**
 * @brief  Initializes empty locations of the grid with a specific block type into vector for dense initial placement 
 *
 *   @param block_type_index block type index that failed in previous initial placement iterations
 *   
 * @return first location (lowest y) and number of remaining blocks in each column for the block_type_index
 */
static std::vector<t_grid_empty_locs_block_type> init_blk_types_empty_locations(int block_type_index);

/**
 * @brief  Helper function used when IO locations are to be randomly locked
 *
 *   @param pl_macro The macro to be fixed.
 *   @param loc The location at which the head of the macro is placed.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 */
static inline void fix_IO_block_types(t_pl_macro pl_macro, t_pl_loc loc, enum e_pad_loc_type pad_loc_type);

/**
 * @brief  tries to place a macro at a random location
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_random_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

/**
 * @brief Looks for a valid placement location for macro exhaustively once the maximum number of random locations have been tried.
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_exhaustive_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

/**
 * @brief Looks for a valid placement location for macro in second iteration, tries to place as many macros as possible in one column 
 * and avoids fragmenting the available locations in one column. 
 *   
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param blk_types_empty_locs_in_grid first location (lowest y) and number of remaining blocks in each column for the blk_id type
 *
 * @return true if the macro gets placed, false if not.
 */
static bool try_dense_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid);

/**
 * @brief Print all blocks based on how hard they are to place; Blocks with higher scores will be printed first. 
 *   
 *   @param sorted_blocks List of all blocks in desecending order of their scores.
 *   @param block_scores scores assigned to each blocks based on how difficult they are to place.
 */
void print_sorted_blocks(const std::vector<ClusterBlockId>& sorted_blocks, const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

/**
 * @brief Tries for MAX_INIT_PLACE_ATTEMPTS times to place all blocks considering their floorplanning constraints and the device size
 *   
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *   @param constraints_file Used to read block locations if any constraints is available.
 */
static void place_all_blocks(enum e_pad_loc_type pad_loc_type, const char* constraints_file);

/**
 * @brief If any blocks remain unplaced after all initial placement iterations, this routine
 * throws an error indicating that initial placement can not be done with the current device size or
 * floorplanning constraints. 
 */
static void check_initial_placement_legality();

static void check_initial_placement_legality() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    int unplaced_blocks = 0;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (place_ctx.block_locs[blk_id].loc.x == INVALID_X) {
            VTR_LOG("Block %s (# %d) of type %s could not be placed during initial placement iteration %d\n", cluster_ctx.clb_nlist.block_name(blk_id).c_str(), blk_id, cluster_ctx.clb_nlist.block_type(blk_id)->name, MAX_INIT_PLACE_ATTEMPTS - 1);
            unplaced_blocks++;
        }
    }

    if (unplaced_blocks > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "%d blocks could not be placed during initial placement; no spaces were available for them on the grid.\n"
                        "If VPR was run with floorplan constraints, the constraints may be too tight.\n",
                        unplaced_blocks);
    }
}

static bool is_block_placed(ClusterBlockId blk_id) {
    auto& place_ctx = g_vpr_ctx.placement();

    return (!(place_ctx.block_locs[blk_id].loc.x == INVALID_X));
}

static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc, t_pl_macro pl_macro) {
    int y = first_macro_loc.first_avail_loc.y;

    /*
     * if the macro member offset is positive, it means that macro head should be placed at the first location of first_macro_loc.
     * otherwise, macro head should be placed at the last available location to ensure macro_can_be_placed can check macro location correctly.
     * 
     */
    if (pl_macro.members.size() > 1) {
        if (pl_macro.members.at(1).offset.y < 0) {
            y += (pl_macro.members.size() - 1) * abs(pl_macro.members.at(1).offset.y);
        }
    }

    return y;
}

static void update_blk_type_first_loc(int blk_type_column_index, t_logical_block_type_ptr block_type, t_pl_macro pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    //check if dense placement could place macro successfully
    if (blk_type_column_index == -1 || blk_types_empty_locs_in_grid->size() <= abs(blk_type_column_index)) {
        return;
    }

    const auto& device_ctx = g_vpr_ctx.device();

    //update the first available macro location in a specific column for the next macro
    blk_types_empty_locs_in_grid->at(blk_type_column_index).first_avail_loc.y += device_ctx.physical_tile_types.at(block_type->index).height * pl_macro.members.size();
    blk_types_empty_locs_in_grid->at(blk_type_column_index).num_of_empty_locs_in_y_axis -= pl_macro.members.size();
}

static int get_blk_type_first_loc(t_pl_loc& loc, t_pl_macro pl_macro, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    //loop over all empty locations and choose first column that can accomodate macro blocks
    for (unsigned int empty_loc_index = 0; empty_loc_index < blk_types_empty_locs_in_grid->size(); empty_loc_index++) {
        auto first_empty_loc = blk_types_empty_locs_in_grid->at(empty_loc_index);

        //if macro size is larger than available locations in the specific column, should go to next available column
        if ((unsigned)first_empty_loc.num_of_empty_locs_in_y_axis < pl_macro.members.size()) {
            continue;
        }

        //set the coordinate of first location that can accomodate macro blocks
        loc.x = first_empty_loc.first_avail_loc.x;
        loc.y = get_y_loc_based_on_macro_direction(first_empty_loc, pl_macro);
        loc.sub_tile = first_empty_loc.first_avail_loc.sub_tile;

        return empty_loc_index;
    }

    return -1;
}

static std::vector<t_grid_empty_locs_block_type> init_blk_types_empty_locations(int block_type_index) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type_index];
    const auto& device_ctx = g_vpr_ctx.device();

    //create a vector to store all columns containing block_type_index with their lowest y and number of remaining blocks
    std::vector<t_grid_empty_locs_block_type> block_type_empty_locs;

    //create a region the size of grid to find out first location with a specific block type
    Region reg;
    reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
    reg.set_sub_tile(NO_SUBTILE);

    int min_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, reg.get_region_rect().xmin());
    int max_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, reg.get_region_rect().xmax());

    //traverse all column and store their empty locations in block_type_empty_locs
    for (int x_loc = min_cx; x_loc <= max_cx; x_loc++) {
        t_grid_empty_locs_block_type empty_loc;
        empty_loc.first_avail_loc.x = compressed_block_grid.grid[x_loc].at(0).x;
        empty_loc.first_avail_loc.y = compressed_block_grid.grid[x_loc].at(0).y;
        empty_loc.first_avail_loc.sub_tile = 0;
        empty_loc.num_of_empty_locs_in_y_axis = compressed_block_grid.grid[x_loc].size();
        block_type_empty_locs.push_back(empty_loc);
    }

    return block_type_empty_locs;
}

static inline void fix_IO_block_types(t_pl_macro pl_macro, t_pl_loc loc, enum e_pad_loc_type pad_loc_type) {
    const auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    //If the user marked the IO block pad_loc_type as RANDOM, that means it should be randomly
    //placed and then stay fixed to that location, which is why the macro members are marked as fixed.
    if (is_io_type(device_ctx.grid[loc.x][loc.y].type) && pad_loc_type == RANDOM) {
        for (unsigned int imember = 0; imember < pl_macro.members.size(); imember++) {
            place_ctx.block_locs[pl_macro.members[imember].blk_index].is_fixed = true;
        }
    }
}

static bool try_random_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    t_pl_loc loc;

    /*
     * Getting various values needed for the find_compatible_compressed_loc_in_range() routine called below.
     * Need to pass the from/to coords of the block, as well as the min/max x and y coords of where it can
     * be placed in the compressed grid.
     */

    //Block has not been placed yet, so the "from" coords will be (-1, -1)
    int cx_from = -1;
    int cy_from = -1;

    //If the block has more than one floorplan region, pick a random region to get the min/max x and y values
    int region_index;
    std::vector<Region> regions = pr.get_partition_region();
    if (regions.size() > 1) {
        region_index = vtr::irand(regions.size() - 1);
    } else {
        region_index = 0;
    }
    Region reg = regions[region_index];

    vtr::Rect<int> rect = reg.get_region_rect();

    int min_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, rect.xmin());
    int min_cy = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_y, rect.ymin());

    int max_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, rect.xmax());
    int max_cy = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_y, rect.ymax());

    int delta_cx = max_cx - min_cx;

    int cx_to;
    int cy_to;

    bool legal;
    legal = find_compatible_compressed_loc_in_range(block_type, min_cx, max_cx, min_cy, max_cy, delta_cx, cx_from, cy_from, cx_to, cy_to, false);
    if (!legal) {
        //No valid position found
        return false;
    }

    compressed_grid_to_loc(block_type, cx_to, cy_to, loc);

    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].width_offset == 0);
    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].height_offset == 0);

    legal = try_place_macro(pl_macro, loc);

    if (legal) {
        fix_IO_block_types(pl_macro, loc, pad_loc_type);
    }

    return legal;
}

static bool try_exhaustive_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    std::vector<Region> regions = pr.get_partition_region();

    bool placed = false;

    t_pl_loc to_loc;

    for (unsigned int reg = 0; reg < regions.size() && placed == false; reg++) {
        vtr::Rect<int> rect = regions[reg].get_region_rect();

        int min_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, rect.xmin());
        int max_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x, rect.xmax());

        for (int cx = min_cx; cx <= max_cx && placed == false; cx++) {
            auto y_lower_iter = compressed_block_grid.grid[cx].begin();
            auto y_upper_iter = compressed_block_grid.grid[cx].end();

            int y_range = std::distance(y_lower_iter, y_upper_iter);

            VTR_ASSERT(y_range >= 0);

            for (int dy = 0; dy < y_range && placed == false; dy++) {
                int cy = (y_lower_iter + dy)->first;

                to_loc.x = compressed_block_grid.compressed_to_grid_x[cx];
                to_loc.y = compressed_block_grid.compressed_to_grid_y[cy];

                auto& grid = g_vpr_ctx.device().grid;
                auto tile_type = grid[to_loc.x][to_loc.y].type;

                if (regions[reg].get_sub_tile() != NO_SUBTILE) {
                    int subtile = regions[reg].get_sub_tile();

                    to_loc.sub_tile = subtile;
                    if (place_ctx.grid_blocks[to_loc.x][to_loc.y].blocks[to_loc.sub_tile] == EMPTY_BLOCK_ID) {
                        placed = try_place_macro(pl_macro, to_loc);

                        if (placed) {
                            fix_IO_block_types(pl_macro, to_loc, pad_loc_type);
                        }
                    }
                } else {
                    for (const auto& sub_tile : tile_type->sub_tiles) {
                        if (is_sub_tile_compatible(tile_type, block_type, sub_tile.capacity.low)) {
                            int st_low = sub_tile.capacity.low;
                            int st_high = sub_tile.capacity.high;

                            for (int st = st_low; st <= st_high && placed == false; st++) {
                                to_loc.sub_tile = st;
                                if (place_ctx.grid_blocks[to_loc.x][to_loc.y].blocks[to_loc.sub_tile] == EMPTY_BLOCK_ID) {
                                    placed = try_place_macro(pl_macro, to_loc);
                                    if (placed) {
                                        fix_IO_block_types(pl_macro, to_loc, pad_loc_type);
                                    }
                                }
                            }
                        }

                        if (placed) {
                            break;
                        }
                    }
                }
            }
        }
    }

    return placed;
}

static bool try_dense_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    t_pl_loc loc;
    int column_index = get_blk_type_first_loc(loc, pl_macro, blk_types_empty_locs_in_grid);

    //check if first available location is within the chip and macro's partition region, otherwise placement is not legal
    if (!is_loc_on_chip(loc.x, loc.y) || !pr.is_loc_in_part_reg(loc)) {
        return false;
    }

    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].width_offset == 0);
    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].height_offset == 0);

    bool legal = false;
    legal = try_place_macro(pl_macro, loc);

    if (legal) {
        fix_IO_block_types(pl_macro, loc, pad_loc_type);
    }

    //Dense placement found a legal position for pl_macro;
    //We need to update first available location (lowest y) and number of remaining blocks for the next macro.
    update_blk_type_first_loc(column_index, block_type, pl_macro, blk_types_empty_locs_in_grid);
    return legal;
}

static bool try_place_macro(t_pl_macro pl_macro, t_pl_loc head_pos) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    bool macro_placed = false;

    // If that location is occupied, do nothing.
    if (place_ctx.grid_blocks[head_pos.x][head_pos.y].blocks[head_pos.sub_tile] != EMPTY_BLOCK_ID) {
        return (macro_placed);
    }

    bool mac_can_be_placed = macro_can_be_placed(pl_macro, head_pos, false);

    if (mac_can_be_placed) {
        // Place down the macro
        macro_placed = true;
        for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
            t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

            ClusterBlockId iblk = pl_macro.members[imember].blk_index;

            set_block_location(iblk, member_pos);

        } // Finish placing all the members in the macro
    }

    return (macro_placed);
}

static bool place_macro(int macros_max_num_tries, t_pl_macro pl_macro, enum e_pad_loc_type pad_loc_type, std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    ClusterBlockId blk_id;
    blk_id = pl_macro.members[0].blk_index;

    if (is_block_placed(blk_id)) {
        return true;
    }

    bool macro_placed = false;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    // Assume that all the blocks in the macro are of the same type
    auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

    PartitionRegion pr;

    //Enough to check head member of macro to see if its constrained because
    //constraints propagation was done earlier in initial placement.
    if (is_cluster_constrained(blk_id)) {
        pr = floorplanning_ctx.cluster_constraints[blk_id];
    } else { //If the block is not constrained, assign a region the size of the grid to its PartitionRegion
        Region reg;
        reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
        reg.set_sub_tile(NO_SUBTILE);
        pr.add_to_part_region(reg);
    }

    //If blk_types_empty_locs_in_grid is not NULL, means that initial placement has been failed in first iteration for this block type
    //We need to place densely in second iteration to be able to find a legal initial placement solution
    if (blk_types_empty_locs_in_grid != NULL && blk_types_empty_locs_in_grid->size() != 0) {
        macro_placed = try_dense_placement(pl_macro, pr, block_type, pad_loc_type, blk_types_empty_locs_in_grid);
    }

    // If macro is not placed yet, try to place the macro randomly for the max number of random tries
    for (int itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
        macro_placed = try_random_placement(pl_macro, pr, block_type, pad_loc_type);
    } // Finished all tries

    if (!macro_placed) {
        // if a macro still could not be placed after macros_max_num_tries times,
        // go through the chip exhaustively to find a legal placement for the macro
        // place the macro on the first location that is legal
        // then set macro_placed = true;
        // if there are no legal positions, error out

        // Exhaustive placement of carry macros
        macro_placed = try_exhaustive_placement(pl_macro, pr, block_type, pad_loc_type);
    }
    return macro_placed;
}

static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& floorplan_ctx = g_vpr_ctx.floorplanning();

    auto& pl_macros = place_ctx.pl_macros;

    t_block_score score;

    vtr::vector<ClusterBlockId, t_block_score> block_scores;

    block_scores.resize(cluster_ctx.clb_nlist.blocks().size());

    //GridTileLookup class provides info needed for calculating number of tiles covered by a region
    GridTileLookup grid_tiles;

    /*
     * For the blocks with no floorplan constraints, and the blocks that are not part of macros,
     * the block scores will remain at their default values assigned by the constructor
     * (macro_size = 0; floorplan_constraints = 0;
     */

    //go through all blocks and store floorplan constraints score
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (is_cluster_constrained(blk_id)) {
            PartitionRegion pr = floorplan_ctx.cluster_constraints[blk_id];
            auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);
            int floorplan_score = get_floorplan_score(blk_id, pr, block_type, grid_tiles);
            block_scores[blk_id].tiles_outside_of_floorplan_constraints = floorplan_score;
        }
    }

    //go through placement macros and store size of macro for each block
    for (auto pl_macro : pl_macros) {
        int size = pl_macro.members.size();
        for (unsigned int i = 0; i < pl_macro.members.size(); i++) {
            block_scores[pl_macro.members[i].blk_index].macro_size = size;
        }
    }

    return block_scores;
}

static std::vector<ClusterBlockId> sort_blocks(const vtr::vector<ClusterBlockId, t_block_score>& block_scores) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto blocks = cluster_ctx.clb_nlist.blocks();

    std::vector<ClusterBlockId> sorted_blocks(blocks.begin(), blocks.end());

    /*
     * The criteria considers blocks that belong to a macro or to a floorplan region more difficult to place.
     * The bigger the macro, and/or the tighter the floorplan constraint, the earlier the block will be in
     * the list of sorted blocks.
     * The tiles_outside_of_floorplan_constraints will dominate the criteria, since the number of tiles will
     * likely be significantly bigger than the macro size. This is okay since the floorplan constraints give
     * a more accurate picture of how difficult a block is to place.
     */
    auto criteria = [block_scores](ClusterBlockId lhs, ClusterBlockId rhs) {
        int lhs_score = 10 * block_scores[lhs].macro_size + block_scores[lhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCK * block_scores[lhs].failed_to_place_in_prev_attempts;
        int rhs_score = 10 * block_scores[rhs].macro_size + block_scores[rhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCK * block_scores[rhs].failed_to_place_in_prev_attempts;

        return lhs_score > rhs_score;
    };

    std::stable_sort(sorted_blocks.begin(), sorted_blocks.end(), criteria);
    //print_sorted_blocks(sorted_blocks, block_scores);

    return sorted_blocks;
}

void print_sorted_blocks(const std::vector<ClusterBlockId>& sorted_blocks, const vtr::vector<ClusterBlockId, t_block_score>& block_scores) {
    VTR_LOG("\nPrinting sorted blocks: \n");
    for (unsigned int i = 0; i < sorted_blocks.size(); i++) {
        VTR_LOG("Block_Id: %zu, Macro size: %d, Num tiles outside floorplan constraints: %d\n", sorted_blocks[i], block_scores[sorted_blocks[i]].macro_size, block_scores[sorted_blocks[i]].tiles_outside_of_floorplan_constraints);
    }
}

static void place_all_blocks(enum e_pad_loc_type pad_loc_type, const char* constraints_file) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    int number_of_unplaced_blks_in_curr_itr;
    //keep tracks of which block types can not be placed in each iteration
    std::unordered_set<int> unplaced_blk_type_in_curr_itr;

    // Keeps the first locations and number of remained blocks in each column for a specific block type.
    //[0..device_ctx.logical_block_types.size()-1][0..num_of_grid_columns_containing_this_block_type-1]
    std::vector<std::vector<t_grid_empty_locs_block_type>> blk_types_empty_locs_in_grid;

    for (auto iter_no = 0; iter_no < MAX_INIT_PLACE_ATTEMPTS; iter_no++) {
        //clear grid for a new placement iteration
        clear_block_type_grid_locs(unplaced_blk_type_in_curr_itr);
        unplaced_blk_type_in_curr_itr.clear();

        //Check whether the constraint file is NULL, if not, read in the block locations from the constraints file here
        if (strlen(constraints_file) != 0) {
            read_constraints(constraints_file);
        }

        //Sort blocks and placement macros according to how difficult they are to place
        vtr::vector<ClusterBlockId, t_block_score> block_scores = assign_block_scores();
        std::vector<ClusterBlockId> sorted_blocks = sort_blocks(block_scores);

        //resize the vector to store unplaced block types empty locations
        blk_types_empty_locs_in_grid.resize(device_ctx.logical_block_types.size());

        number_of_unplaced_blks_in_curr_itr = 0;

        for (auto blk_id : sorted_blocks) {
            auto blk_id_type = cluster_ctx.clb_nlist.block_type(blk_id);
            bool block_placed = place_one_block(blk_id, pad_loc_type, &blk_types_empty_locs_in_grid[blk_id_type->index]);

            if (!block_placed) {
                //add current block to list to ensure it will be placed sooner in the next iteration in initial placement
                number_of_unplaced_blks_in_curr_itr++;
                block_scores[blk_id].failed_to_place_in_prev_attempts++;
                int imacro;
                get_imacro_from_iblk(&imacro, blk_id, place_ctx.pl_macros);
                if (imacro != -1) { //the block belongs to macro that contain a chain, we need to turn on dense placement in next iteration for that type of block
                    unplaced_blk_type_in_curr_itr.insert(blk_id_type->index);
                }
            }
        }

        //current iteration could place all of design's blocks, initial placement succeed
        if (number_of_unplaced_blks_in_curr_itr == 0) {
            VTR_LOG("Initial placement iteration %d has finished successfully\n", iter_no);
            return;
        }

        //loop over block types with macro that have failed to be placed, and add their locations in grid for the next iteration
        for (auto itype : unplaced_blk_type_in_curr_itr) {
            blk_types_empty_locs_in_grid[itype] = init_blk_types_empty_locations(itype);
        }

        //print unplaced blocks in the current iteration
        VTR_LOG("Initial placement iteration %d has finished with %d unplaced blocks\n", iter_no, number_of_unplaced_blks_in_curr_itr);
    }
}

static void clear_block_type_grid_locs(std::unordered_set<int> unplaced_blk_types_index) {
    auto& device_ctx = g_vpr_ctx.device();
    bool clear_all_block_types = false;

    /* check if all types should be cleared
     * logical_block_types contain empty type, needs to be ignored.
     * Not having any type in unplaced_blk_types_index means that it is the first iteration, hence all grids needs to be cleared
     */
    if (unplaced_blk_types_index.size() == device_ctx.logical_block_types.size() - 1 || unplaced_blk_types_index.size() == 0) {
        clear_all_block_types = true;
    }

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    int itype;

    /* We'll use the grid to record where everything goes. Initialize to the grid has no
     * blocks placed anywhere.
     */
    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            itype = device_ctx.grid[i][j].type->index;
            if (clear_all_block_types || unplaced_blk_types_index.count(itype)) {
                place_ctx.grid_blocks[i][j].usage = 0;
                for (int k = 0; k < device_ctx.physical_tile_types[itype].capacity; k++) {
                    if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                        place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID;
                    }
                }
            }
        }
    }

    /* Similarly, mark all blocks as not being placed yet. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto blk_type = cluster_ctx.clb_nlist.block_type(blk_id)->index;
        if (clear_all_block_types || unplaced_blk_types_index.count(blk_type)) {
            place_ctx.block_locs[blk_id].loc = t_pl_loc();
        }
    }
}

bool place_one_block(const ClusterBlockId& blk_id,
                     enum e_pad_loc_type pad_loc_type,
                     std::vector<t_grid_empty_locs_block_type>* blk_types_empty_locs_in_grid) {
    auto& place_ctx = g_vpr_ctx.placement();

    //Check if block has already been placed
    if (is_block_placed(blk_id)) {
        return true;
    }

    bool placed_macro = false;

    //Lookup to see if the block is part of a macro
    t_pl_macro pl_macro;
    int imacro;
    get_imacro_from_iblk(&imacro, blk_id, place_ctx.pl_macros);

    if (imacro != -1) { //If the block belongs to a macro, pass that macro to the placement routines
        pl_macro = place_ctx.pl_macros[imacro];
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type, blk_types_empty_locs_in_grid);
    } else {
        //If it does not belong to a macro, create a macro with the one block and then pass to the placement routines
        //This is done so that the initial placement flow can be the same whether the block belongs to a macro or not
        t_pl_macro_member macro_member;
        t_pl_offset block_offset(0, 0, 0);

        macro_member.blk_index = blk_id;
        macro_member.offset = block_offset;
        pl_macro.members.push_back(macro_member);
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type, blk_types_empty_locs_in_grid);
    }

    return placed_macro;
}

void initial_placement(enum e_pad_loc_type pad_loc_type, const char* constraints_file) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");

    /* Go through cluster blocks to calculate the tightest placement
     * floorplan constraint for each constrained block
     */
    propagate_place_constraints();

    /*Mark the blocks that have already been locked to one spot via floorplan constraints
     * as fixed so they do not get moved during initial placement or later during the simulated annealing stage of placement*/
    mark_fixed_blocks();

    //Place all blocks
    place_all_blocks(pad_loc_type, constraints_file);

    //if any blocks remain unplaced, print an error
    check_initial_placement_legality();

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
}
