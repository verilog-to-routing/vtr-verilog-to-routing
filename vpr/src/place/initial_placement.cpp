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

//Used to assign each block a score for how difficult it is to place
struct t_block_score {
    int macro_size = 0; //how many members does the macro have, if the block is part of one - this value is zero if the block is not in a macro

    /*
     * The number of tiles NOT covered by the block's floorplan constraints. The higher this number, the more
     * difficult the block is to place.
     */
    int tiles_outside_of_floorplan_constraints = 0;

    /*
     * If random initial placement failed to place all blocks, next iteration will increase unplaced blocks' 
     * scores to place them earlier. 
     */
    int failed_to_place_in_prev_attempts;
};

/**
 * @brief keeps track of available empty locations of a specific block type during initial placement.
 * Used to densly place macros that failed to be placed in the first initial placement iteration (random placement)
 */
struct t_grid_empty_locs_block_type {
    t_pl_loc first_avail_loc; //first available location of a block type

    int num_of_empty_locs_in_y_axis; //number of consecutive locations from first_avail_loc that can accomadate blocks with same block types
};

/// @brief sentinel value for indicating that a block does not have a valid x location, used to check whether a block has been placed
constexpr int INVALID_X = -1;

// Number of iterations that initial placement tries to place all blocks before throwing an error
#define MAX_INIT_PLACE_ATTEMPTS 2

// The amount of weight that will added to previous unplaced block scores to ensure that failed blocks would be placed earlier next iteration
#define SORT_WEIGHT_PER_FAILED_BLOCKS 10

/* The maximum number of tries when trying to place a macro at a    *
 * random location before trying exhaustive placement - find the first     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 8

// Keeps the first locations and number of remained blocks in each column for a specific block type.
static std::vector<std::vector<t_grid_empty_locs_block_type>> blk_types_empty_locs_in_grid;


// initialize the grid before each placement iteration
static void init_grid(); 

/*
 * Places the macro if the head position passed in is legal, and all the resulting
 * member positions are legal
 * Returns true if macro was placed, false if not.
 */
static bool try_place_macro(t_pl_macro pl_macro, t_pl_loc head_pos);

/*
 * Control routine for placing a macro - first calls random placement for the max number of tries,
 * the calls the exhaustive placement routine. Errors out if the macro cannot be placed.
 */
static bool place_macro(int macros_max_num_tries, t_pl_macro pl_macro, enum e_pad_loc_type pad_loc_type);

/*
 * Assign scores to each block based on macro size and floorplanning constraints.
 * Used for relative placement, so that the blocks that are more difficult to place can be placed first during initial placement.
 * A higher score indicates that the block is more difficult to place.
 */
static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores();

//Sort the blocks according to how difficult they are to place, prior to initial placement
static std::vector<ClusterBlockId> sort_blocks(const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

/**
 * @brief  try to find y axis for macro head location based on macro direction
 *   
 *
 *   @param first_macro_loc The first available location that can place the macro blocks.
 *   @param pl_macro The macro to be placed.
 *
 *   Returns y axis of the location that macro head should be placed
 */
static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc, t_pl_macro pl_macro);

/**
 * @brief  try to get the first available location of a specific block type that can accomodate macro blocks
 *
 *   @param loc The first available location that can place the macro blocks.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pl_macro The macro to be placed.
 *
 */
static void get_blk_type_first_loc(t_pl_loc& loc,t_logical_block_type_ptr block_type,t_pl_macro pl_macro);

/**
 * @brief  initialize empty locations of the grid with a specific block type into vector for dense initial placement 
 *
 *   @param block_type_index block type index that failed in previous initial placement iterations
 *   @param block_type_empty_locs Vector that holds block type empty locations  
 *   
 */
static void init_blk_types_empty_locations(int block_type_index, std::vector <t_grid_empty_locs_block_type>& block_type_empty_locs);

/**
 * @brief  mark the macro members' locations fixed if necessary
 *
 *   @param pl_macro The macro to be fixed
 *   @param loc The location to check type
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 */
static inline void fix_IO_block_types(t_pl_macro pl_macro, t_pl_loc loc, enum e_pad_loc_type pad_loc_type);

/**
 * @brief  try to place a macro at a random location
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *
 *   Returns true if the macro gets placed, false if not.
 */
static bool try_random_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

/**
 * @brief  Look for a valid placement location for macro exhaustively once the maximum number of random locations have been tried.
 *
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *
 *   Returns true if the macro gets placed, false if not.
 */
static bool try_exhaustive_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

/**
 * @brief  Look for a valid placement location for macro in second iteration, tries to place as many macros as possible in one column 
 * and avoids to create any fragment location in one column. 
 *   
 *   @param pl_macro The macro to be placed.
 *   @param pr The PartitionRegion of the macro - represents its floorplanning constraints, is the size of the whole chip if the macro is not
 *   constrained.
 *   @param block_type Logical block type of the macro blocks.
 *   @param pad_loc_type Used to check whether an io block needs to be marked as fixed.
 *
 *   Returns true if the macro gets placed, false if not.
 */
static bool try_dense_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

void print_sorted_blocks(const std::vector<ClusterBlockId>& sorted_blocks, const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

static int place_all_blocks(enum e_pad_loc_type pad_loc_type);

/**
 * @brief If any blocks are unplaced after each initial placement iteration, this routine
 * prints an log message showing the names, types, and IDs of the unplaced blocks.
 *  
 *  @param init_placement_iter initial placement iteration that finished.
 *  @param unplaced_blocks Number of failed blocks after init_placement_iter.
 */
static void print_unplaced_blocks(int init_placement_iter, int unplaced_blocks);

/**
 * @brief If any blocks remain unplaced after all initial placement iterations, this routine
 * throw an error indicating that initial placement can not be done with the current device size or
 * floorplanning constraints. 
 *  
 *  @param unplaced_blocks Number of failed blocks after all placement iterations.
 */
static void check_initial_placement_legality(int unplaced_blocks);

static void check_initial_placement_legality(int unplaced_blocks) {
    if (unplaced_blocks > 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "%d blocks could not be placed during initial placement, no spaces were available for them on the grid.\n"
                        "If VPR was run with floorplan constraints, the constraints may be too tight.\n",
                        unplaced_blocks);
    }
}

static void print_unplaced_blocks(int init_placement_iter, int unplaced_blocks) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    VTR_LOG("Initial placement iteration %d has finished with %d unplaced blocks\n",init_placement_iter,unplaced_blocks);

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (place_ctx.block_locs[blk_id].loc.x == INVALID_X) {
            VTR_LOG("Block %s (# %d) of type %s could not be placed during initial placement iteration %d\n", cluster_ctx.clb_nlist.block_name(blk_id).c_str(), blk_id, cluster_ctx.clb_nlist.block_type(blk_id)->name,init_placement_iter);
        }
    }
}

static bool is_block_placed(ClusterBlockId blk_id) {
    auto& place_ctx = g_vpr_ctx.placement();

    return (!(place_ctx.block_locs[blk_id].loc.x == INVALID_X));
}


static int get_y_loc_based_on_macro_direction(t_grid_empty_locs_block_type first_macro_loc, t_pl_macro pl_macro){
    int y = first_macro_loc.first_avail_loc.y;
    
    /*
     * if the macro member offset is positive, it means that macro head should be placed at the first location of first_macro_loc.
     * otherwise, macro head should be placed at the last available location to ensure macro_can_be_placed can check macro location correctly.
     * 
     */  
    if(pl_macro.members.size() > 1){ 
        if(pl_macro.members.at(1).offset.y < 0){
            y += (pl_macro.members.size()-1)*abs(pl_macro.members.at(1).offset.y);
        }
    }
    
    return y;
}

static void get_blk_type_first_loc(t_pl_loc& loc,t_logical_block_type_ptr block_type,t_pl_macro pl_macro){
    const auto& device_ctx = g_vpr_ctx.device();

    //loop over all empty locations and choose first column that can accomodate macro blocks
    for(unsigned int empty_loc_index = 0; empty_loc_index < blk_types_empty_locs_in_grid[block_type->index].size(); empty_loc_index++){
        auto first_empty_loc = blk_types_empty_locs_in_grid[block_type->index].at(empty_loc_index);
        
        //if macro size is larger than available locations in the specific column, should go to next available column
        if((unsigned)first_empty_loc.num_of_empty_locs_in_y_axis < pl_macro.members.size()){
            continue;
        }

        //set the coordinate of first location that can accomodate macro blocks
        loc.x = first_empty_loc.first_avail_loc.x;
        loc.y = get_y_loc_based_on_macro_direction(first_empty_loc,pl_macro);
        loc.sub_tile = first_empty_loc.first_avail_loc.sub_tile;
        
        //update the first available macro location in vector for the next macro 
        first_empty_loc.first_avail_loc.y += device_ctx.physical_tile_types.at(block_type->index).height * pl_macro.members.size();
        first_empty_loc.num_of_empty_locs_in_y_axis -= pl_macro.members.size();
        blk_types_empty_locs_in_grid[block_type->index][empty_loc_index] = first_empty_loc;
        
        break;
    }
}

static void init_blk_types_empty_locations(int block_type_index, std::vector <t_grid_empty_locs_block_type>& block_type_empty_locs){
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type_index];
    const auto& device_ctx = g_vpr_ctx.device();
    
    //create a region the size of grid to find out first location with a specific block type
    Region reg;
    reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
    reg.set_sub_tile(NO_SUBTILE);

    int min_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x,reg.get_region_rect().xmin());
    int max_cx = grid_to_compressed_approx(compressed_block_grid.compressed_to_grid_x,reg.get_region_rect().xmax());

    //traverse all column and store their empty locations in block_type_empty_locs
    for(int x_loc = min_cx; x_loc <= max_cx; x_loc++){
        t_grid_empty_locs_block_type empty_loc;
        empty_loc.first_avail_loc.x = compressed_block_grid.grid[x_loc].at(0).x; 
        empty_loc.first_avail_loc.y = compressed_block_grid.grid[x_loc].at(0).y;
        empty_loc.first_avail_loc.sub_tile = 0;
        empty_loc.num_of_empty_locs_in_y_axis = compressed_block_grid.grid[x_loc].size();
        block_type_empty_locs.push_back(empty_loc);
    }
}

static inline void fix_IO_block_types(t_pl_macro pl_macro, t_pl_loc loc, enum e_pad_loc_type pad_loc_type){
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

    if(legal){
        fix_IO_block_types(pl_macro,loc,pad_loc_type);
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

                        if(placed){
                            fix_IO_block_types(pl_macro,to_loc,pad_loc_type);
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
                                    if(placed){
                                        fix_IO_block_types(pl_macro,to_loc,pad_loc_type);
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

static bool try_dense_placement(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type){

    t_pl_loc loc;
    get_blk_type_first_loc(loc,block_type,pl_macro);

    bool legal = false;
    
    //check if first available location is within the chip and macro's partition region, otherwise placement is not legal
    if(!is_loc_on_chip(loc.x,loc.y) || !pr.is_loc_in_part_reg(loc)){
        return legal;
    }
    
    auto& device_ctx = g_vpr_ctx.device();

    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].width_offset == 0);
    VTR_ASSERT(device_ctx.grid[loc.x][loc.y].height_offset == 0);
    
    legal = try_place_macro(pl_macro, loc);
    
    if(legal){
        fix_IO_block_types(pl_macro,loc,pad_loc_type);
    }

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

static bool place_macro(int macros_max_num_tries, t_pl_macro pl_macro, enum e_pad_loc_type pad_loc_type) {
    bool macro_placed;
    int itry;
    ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    macro_placed = false;

    while (!macro_placed) {
        blk_id = pl_macro.members[0].blk_index;

        if (is_block_placed(blk_id)) {
            macro_placed = true;
            continue;
        }

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

        //if the block type has failed in previous iteration, we need to start place densly to be able to find a legal initial placement solution
        if(blk_types_empty_locs_in_grid[block_type->index].size() != 0){
            macro_placed = try_dense_placement(pl_macro, pr, block_type, pad_loc_type);
        }

        if (!macro_placed) {
            // Try to place the macro randomly for the max number of random tries
            for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
                macro_placed = try_random_placement(pl_macro, pr, block_type, pad_loc_type);

                // Try to place the macro
                if (macro_placed) {
                    break;
                }

            } // Finished all tries
        }
        if (!macro_placed) {
            // if a macro still could not be placed after macros_max_num_tries times,
            // go through the chip exhaustively to find a legal placement for the macro
            // place the macro on the first location that is legal
            // then set macro_placed = true;
            // if there are no legal positions, error out

            // Exhaustive placement of carry macros
            macro_placed = try_exhaustive_placement(pl_macro, pr, block_type, pad_loc_type);

            // If macro could not be placed after exhaustive placement, error out
        }

        if (!macro_placed) {
            std::vector<std::string> tried_types;
            for (auto tile_type : block_type->equivalent_tiles) {
                tried_types.push_back(tile_type->name);
            }
            std::string tried_types_str = "{" + vtr::join(tried_types, ", ") + "}";
            break;
        }
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
        int lhs_score = 10 * block_scores[lhs].macro_size + block_scores[lhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCKS * block_scores[lhs].failed_to_place_in_prev_attempts;
        int rhs_score = 10 * block_scores[rhs].macro_size + block_scores[rhs].tiles_outside_of_floorplan_constraints + SORT_WEIGHT_PER_FAILED_BLOCKS * block_scores[rhs].failed_to_place_in_prev_attempts;

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

static int place_all_blocks(enum e_pad_loc_type pad_loc_type) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& device_ctx = g_vpr_ctx.device();

    int number_of_unplaced_blks_in_curr_itr;
    //keep tracks of which block types can not be placed in each iteration
    std::unordered_set<int> unplaced_blk_type_in_curr_itr;

    for(auto iter_no = 0; iter_no < MAX_INIT_PLACE_ATTEMPTS; iter_no++){
        //clear grid for a new placement iteration
        init_grid();
        
        //Sort blocks and placement macros according to how difficult they are to place
        vtr::vector<ClusterBlockId, t_block_score> block_scores = assign_block_scores();
        std::vector<ClusterBlockId> sorted_blocks = sort_blocks(block_scores); 

        //resize the vector to store unplaced block types empty locations
        blk_types_empty_locs_in_grid.resize(device_ctx.logical_block_types.size());

        number_of_unplaced_blks_in_curr_itr = 0;

        for (auto blk_id : sorted_blocks) {
            bool block_placed = place_one_block(blk_id, pad_loc_type);
            if(!block_placed){
                //add current block to list to ensure it will be placed sooner in the next iteration in initial placement
                number_of_unplaced_blks_in_curr_itr++;
                block_scores[blk_id].failed_to_place_in_prev_attempts++;
                int imacro;
                get_imacro_from_iblk(&imacro, blk_id, place_ctx.pl_macros);
                if(imacro != -1){ //the block belongs to macro that contain a chain, we need to turn on dense placement in next iteration
                    unplaced_blk_type_in_curr_itr.insert(cluster_ctx.clb_nlist.block_type(blk_id)->index);
                }
            }
        }

        //current iteration could place all of design's blocks, initial placement succeed
        if(number_of_unplaced_blks_in_curr_itr == 0){
            VTR_LOG("Initial placement iteration %d has finished successfully\n",iter_no);
            return 0;
        }

        //loop over block types that have been failed to be placed, and add their locations in grid for the next iteration
        for(auto itype : unplaced_blk_type_in_curr_itr){
            init_blk_types_empty_locations(itype,blk_types_empty_locs_in_grid[itype]);
        } 
        
        unplaced_blk_type_in_curr_itr.clear();        
        
        //print unplaced blocks in the current iteration
        print_unplaced_blocks(iter_no,number_of_unplaced_blks_in_curr_itr);
    }

    return number_of_unplaced_blks_in_curr_itr;
}

static void init_grid(){
    
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    
    // Set every grid location to an INVALID block id
    zero_initialize_grid_blocks();

    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    int itype;

    /* We'll use the grid to record where everything goes. Initialize to the grid has no
     * blocks placed anywhere.
     */
    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            place_ctx.grid_blocks[i][j].usage = 0;
            itype = device_ctx.grid[i][j].type->index;
            for (int k = 0; k < device_ctx.physical_tile_types[itype].capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                    place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID;
                }
            }
        }
    }

    /* Similarly, mark all blocks as not being placed yet. */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        place_ctx.block_locs[blk_id].loc = t_pl_loc();
    }

}

bool place_one_block(const ClusterBlockId& blk_id,
                     enum e_pad_loc_type pad_loc_type) {
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
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type);
    } else {
        //If it does not belong to a macro, create a macro with the one block and then pass to the placement routines
        //This is done so that the initial placement flow can be the same whether the block belongs to a macro or not
        t_pl_macro_member macro_member;
        t_pl_offset block_offset(0, 0, 0);

        macro_member.blk_index = blk_id;
        macro_member.offset = block_offset;
        pl_macro.members.push_back(macro_member);
        placed_macro = place_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro, pad_loc_type);
    }
    
    return placed_macro;
}

void initial_placement(enum e_pad_loc_type pad_loc_type, const char* constraints_file) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");

    /* Go through cluster blocks to calculate the tightest placement
     * floorplan constraint for each constrained block
     */
    propagate_place_constraints();

    /*Check whether the constraint file is NULL, if not, read in the block locations from the constraints file here*/
    if (strlen(constraints_file) != 0) {
        read_constraints(constraints_file);
    }

    /*Mark the blocks that have already been locked to one spot via floorplan constraints
     * as fixed so they do not get moved during initial placement or later during the simulated annealing stage of placement*/
    mark_fixed_blocks();

    //Place all blocks
    int number_of_failed_blocks = place_all_blocks(pad_loc_type);

    //if any blocks remain unplaced, print an error
    check_initial_placement_legality(number_of_failed_blocks);

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
}
