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
    int macro_size = 0; //how many members does the macro have, if the block is part of one, this value is zero if the block is not in a macro

    /*
     * The number of tiles NOT covered by the block's floorplan constraints. The higher this number, the more
     * difficult the block is to place.
     */
    int tiles_outside_of_floorplan_constraints = 0;
};

/* The maximum number of tries when trying to place a carry chain at a    *
 * random location before trying exhaustive placement - find the fist     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 6
#define MAX_NUM_TRIES_TO_PLACE_BLOCKS_RANDOMLY 10

static int check_macro_can_be_placed(t_pl_macro pl_macro, t_pl_loc head_pos);

static int try_place_macro(t_pl_loc head_pos, t_pl_macro pl_macro);

static void place_a_macro(int macros_max_num_tries, t_pl_macro pl_macro);

static void place_a_block(int blocks_max_num_tries, ClusterBlockId blk_id, enum e_pad_loc_type pad_loc_type);

/*
 * Assign scores to each block based on macro size, floorplanning constraints, and number of equivalent tiles.
 * Used for relative placement, so that the blocks that are more difficult to place can be placed first during initial placement.
 * A higher score indicates that the block is more difficult to place.
 */
static vtr::vector<ClusterBlockId, t_block_score> assign_block_scores();

//Sort the blocks according to how difficult they are to place, prior to initial placement
static std::vector<ClusterBlockId> sort_blocks(const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

//Get a legal placement position for the block
static bool get_legal_placement_loc(PartitionRegion& pr, t_pl_loc& loc, t_logical_block_type_ptr block_type);

static bool try_all_part_region_locations(ClusterBlockId blk_id, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type);

static bool try_all_part_region_locations_macro(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type);

void print_sorted_blocks(const std::vector<ClusterBlockId>& sorted_blocks, const vtr::vector<ClusterBlockId, t_block_score>& block_scores);

static void place_the_blocks(const std::vector<ClusterBlockId>& sorted_blocks,
                             const vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                             enum e_pad_loc_type pad_loc_type);

static bool get_legal_placement_loc(PartitionRegion& pr, t_pl_loc& loc, t_logical_block_type_ptr block_type) {
    const auto& compressed_block_grid = g_vpr_ctx.placement().compressed_block_grids[block_type->index];

    bool legal;

    int cx_from = -1;
    int cy_from = -1;

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

    legal = find_compatible_compressed_loc_in_range(block_type, min_cx, max_cx, min_cy, max_cy, delta_cx, cx_from, cy_from, cx_to, cy_to, false);
    if (!legal) {
        //No valid position found
        return false;
    }

    compressed_grid_to_loc(block_type, cx_to, cy_to, loc);

    auto& grid = g_vpr_ctx.device().grid;

    VTR_ASSERT_MSG(grid[loc.x][loc.y].width_offset == 0, "Should be at block base location");
    VTR_ASSERT_MSG(grid[loc.x][loc.y].height_offset == 0, "Should be at block base location");

    return legal;
}

static bool try_all_part_region_locations(ClusterBlockId blk_id, PartitionRegion& pr, t_logical_block_type_ptr block_type, enum e_pad_loc_type pad_loc_type) {
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

            for (int dy = 0; dy < y_range && placed == false; dy++) {
                int cy = (y_lower_iter + dy)->first;

                to_loc.x = compressed_block_grid.compressed_to_grid_x[cx];
                to_loc.y = compressed_block_grid.compressed_to_grid_y[cy];

                auto& grid = g_vpr_ctx.device().grid;
                auto tile_type = grid[to_loc.x][to_loc.y].type;

                if (regions[reg].get_sub_tile() != NO_SUBTILE) {
                    int subtile = regions[reg].get_sub_tile();

                    to_loc.sub_tile = subtile;
                    if (place_ctx.grid_blocks[to_loc.x][to_loc.y].blocks[to_loc.sub_tile] == EMPTY_BLOCK_ID && cluster_floorplanning_legal(blk_id, to_loc)) {
                        set_block_location(blk_id, to_loc);
                        placed = true;

                        if (is_io_type(pick_physical_type(block_type)) && pad_loc_type == RANDOM) {
                            place_ctx.block_locs[blk_id].is_fixed = true;
                        }
                    }
                } else {
                    for (const auto& sub_tile : tile_type->sub_tiles) {
                        if (is_sub_tile_compatible(tile_type, block_type, sub_tile.capacity.low)) {
                            int st_low = sub_tile.capacity.low;
                            int st_high = sub_tile.capacity.high;

                            for (int st = st_low; st <= st_high && placed == false; st++) {
                                to_loc.sub_tile = st;
                                if (place_ctx.grid_blocks[to_loc.x][to_loc.y].blocks[to_loc.sub_tile] == EMPTY_BLOCK_ID && cluster_floorplanning_legal(blk_id, to_loc)) {
                                    set_block_location(blk_id, to_loc);
                                    placed = true;

                                    if (is_io_type(pick_physical_type(block_type)) && pad_loc_type == RANDOM) {
                                        place_ctx.block_locs[blk_id].is_fixed = true;
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

static bool try_all_part_region_locations_macro(t_pl_macro pl_macro, PartitionRegion& pr, t_logical_block_type_ptr block_type) {
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
                        placed = try_place_macro(to_loc, pl_macro);
                    }
                } else {
                    for (const auto& sub_tile : tile_type->sub_tiles) {
                        if (is_sub_tile_compatible(tile_type, block_type, sub_tile.capacity.low)) {
                            int st_low = sub_tile.capacity.low;
                            int st_high = sub_tile.capacity.high;

                            for (int st = st_low; st <= st_high && placed == false; st++) {
                                to_loc.sub_tile = st;
                                if (place_ctx.grid_blocks[to_loc.x][to_loc.y].blocks[to_loc.sub_tile] == EMPTY_BLOCK_ID) {
                                    placed = try_place_macro(to_loc, pl_macro);
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

static int check_macro_can_be_placed(t_pl_macro pl_macro, t_pl_loc head_pos) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    //Get block type of head member
    ClusterBlockId blk_id = pl_macro.members[0].blk_index;
    auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

    // Every macro can be placed until proven otherwise
    int macro_can_be_placed = true;

    //Check whether macro contains blocks with floorplan constraints
    bool macro_constrained = is_macro_constrained(pl_macro);

    // Check whether all the members can be placed
    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

        /*
         * If the macro is constrained, check that the head member is in a legal position from
         * a floorplanning perspective. It is enough to do this check for the head member alone,
         * because constraints propagation was performed to calculate smallest floorplan region for the head
         * macro, based on the constraints on all of the blocks in the macro. So, if the head macro is in a
         * legal floorplan location, all other blocks in the macro will be as well.
         */
        if (macro_constrained && imember == 0) {
            bool member_loc_good = cluster_floorplanning_legal(pl_macro.members[imember].blk_index, member_pos);
            if (!member_loc_good) {
                macro_can_be_placed = false;
                break;
            }
        }

        // Check whether the location could accept block of this type
        // Then check whether the location could still accommodate more blocks
        // Also check whether the member position is valid, that is the member's location
        // still within the chip's dimemsion and the member_z is allowed at that location on the grid
        if (member_pos.x < int(device_ctx.grid.width()) && member_pos.y < int(device_ctx.grid.height())
            && is_tile_compatible(device_ctx.grid[member_pos.x][member_pos.y].type, block_type)
            && place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.sub_tile] == EMPTY_BLOCK_ID) {
            // Can still accommodate blocks here, check the next position
            continue;
        } else {
            // Cant be placed here - skip to the next try
            macro_can_be_placed = false;
            break;
        }
    }

    return (macro_can_be_placed);
}

static int try_place_macro(t_pl_loc head_pos, t_pl_macro pl_macro) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    int macro_placed = false;

    // If that location is occupied, do nothing.
    if (place_ctx.grid_blocks[head_pos.x][head_pos.y].blocks[head_pos.sub_tile] != EMPTY_BLOCK_ID) {
        return (macro_placed);
    }

    int macro_can_be_placed = check_macro_can_be_placed(pl_macro, head_pos);

    if (macro_can_be_placed) {
        // Place down the macro
        macro_placed = true;
        for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
            t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

            ClusterBlockId iblk = pl_macro.members[imember].blk_index;

            set_block_location(iblk, member_pos);

        } // Finish placing all the members in the macro

    } // End of this choice of legal_pos

    return (macro_placed);
}

static void place_a_macro(int macros_max_num_tries, t_pl_macro pl_macro) {
    int macro_placed;
    int itry;
    ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto& place_ctx = g_vpr_ctx.placement();

    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    macro_placed = false;

    while (!macro_placed) {
        // Assume that all the blocks in the macro are of the same type
        blk_id = pl_macro.members[0].blk_index;

        if (place_ctx.block_locs[blk_id].loc.x != -1) {
            //put VTR assert
            macro_placed = true;
            continue;
        }

        auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

        //Choose a random location for the head
        t_pl_loc head_pos;
        PartitionRegion pr;
        bool found_legal_location = false;

        if (is_cluster_constrained(blk_id)) {
            pr = floorplanning_ctx.cluster_constraints[blk_id];
        } else {
            Region reg;
            reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
            reg.set_sub_tile(NO_SUBTILE);
            pr.add_to_part_region(reg);
        }

        // Try to place the macro first, if can be placed - place them, otherwise try again
        for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
            found_legal_location = get_legal_placement_loc(pr, head_pos, block_type);

            // Try to place the macro
            if (found_legal_location) {
                macro_placed = try_place_macro(head_pos, pl_macro);
            }

        } // Finished all tries

        if (macro_placed == false) {
            // if a macro still could not be placed after macros_max_num_tries times,
            // go through the chip exhaustively to find a legal placement for the macro
            // place the macro on the first location that is legal
            // then set macro_placed = true;
            // if there are no legal positions, error out

            // Exhaustive placement of carry macros
            macro_placed = try_all_part_region_locations_macro(pl_macro, pr, block_type);

            // If macro could not be placed after exhaustive placement, error out
        }

        if (macro_placed == false) {
            std::vector<std::string> tried_types;
            for (auto tile_type : block_type->equivalent_tiles) {
                tried_types.push_back(tile_type->name);
            }
            std::string tried_types_str = "{" + vtr::join(tried_types, ", ") + "}";

            // Error out
            VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                            "Initial placement failed.\n"
                            "Could not place macro length %zu with head block %s (#%zu); not enough free locations of type(s) %s.\n"
                            "Please manually size the FPGA because VPR can't do this yet.\n",
                            pl_macro.members.size(), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), tried_types_str.c_str());
        }
    }
}

/* Place blocks that are NOT a part of any macro.
 * We'll randomly place each block in the clustered netlist, one by one. */
static void place_a_block(int blocks_max_num_tries, ClusterBlockId blk_id, enum e_pad_loc_type pad_loc_type) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    bool placed = false;
    while (!placed) {
        /* -1 is a sentinel for a non-placed block, which the code in this routine will choose a location for.
         * If the x value is not -1, we assume something else has already placed this block and we should leave it there.
         * For example, if the user constrained it to a certain location, the block has already been placed.
         */
        if (place_ctx.block_locs[blk_id].loc.x != -1) {
            //put VTR assert
            placed = true;
            continue;
        }

        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        t_pl_loc to;
        PartitionRegion pr;

        if (is_cluster_constrained(blk_id)) {
            pr = floorplanning_ctx.cluster_constraints[blk_id];
        } else {
            Region reg;
            reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
            reg.set_sub_tile(NO_SUBTILE);
            pr.add_to_part_region(reg);
        }
        bool found_legal_place = false;
        bool floorplanning_good = false;

        for (int itry = 0; itry < blocks_max_num_tries && placed == false; itry++) {
            found_legal_place = get_legal_placement_loc(pr, to, logical_block);

            if (found_legal_place) {
                floorplanning_good = cluster_floorplanning_legal(blk_id, to);
                if (place_ctx.grid_blocks[to.x][to.y].blocks[to.sub_tile] == EMPTY_BLOCK_ID && floorplanning_good) {
                    set_block_location(blk_id, to);
                    placed = true;

                    if (is_io_type(pick_physical_type(logical_block)) && pad_loc_type == RANDOM) {
                        place_ctx.block_locs[blk_id].is_fixed = true;
                    }

                    break;
                }
            }

        } // Finished all tries

        if (placed == false) {
            /*
             * If a block could still not be placed after blocks_max_num_tries attempts,
             * go through the region exhaustively to find a legal placement for the block.
             * Place the blocks on the first location that is legal and available then
             * set placed = true;
             * If there are no legal positions, error out
             */
            placed = try_all_part_region_locations(blk_id, pr, logical_block, pad_loc_type);

            // If block could not be placed after exhaustive placement, error out
            if (placed == false) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                "Initial placement failed.\n"
                                "Could not place  block %s (#%zu); not enough free locations of type(s) %s.\n",
                                cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), logical_block->name);
            }

            VTR_LOG("Placed by exhaustion \n");
        }
    }
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
    grid_tiles.initialize_grid_tile_matrices();

    /*
     * For the blocks with no floorplan constraints, and the blocks that are not part of macros,
     * the block scores will remain at their default values assigned by the constructor
     * (macro_size = 0; floorplan_constraints = 0; num_equivalent_tiles =1;
     */

    //go through all blocks and store floorplan constraints and num equivalent tiles
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
        int lhs_score = 10 * block_scores[lhs].macro_size + block_scores[lhs].tiles_outside_of_floorplan_constraints;
        int rhs_score = 10 * block_scores[rhs].macro_size + block_scores[rhs].tiles_outside_of_floorplan_constraints;

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

static void place_the_blocks(const std::vector<ClusterBlockId>& sorted_blocks,
                             const vtr::vector<ClusterBlockId, t_block_score>& block_scores,
                             enum e_pad_loc_type pad_loc_type) {
    auto& place_ctx = g_vpr_ctx.placement();

    for (auto blk_id : sorted_blocks) {
        //Check if block has already been placed
        if (place_ctx.block_locs[blk_id].loc.x != -1) {
            //put VTR assert
            continue;
        }

        if (block_scores[blk_id].macro_size > 0) {
            int imacro;
            get_imacro_from_iblk(&imacro, blk_id, place_ctx.pl_macros);
            t_pl_macro pl_macro;
            pl_macro = place_ctx.pl_macros[imacro];
            place_a_macro(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, pl_macro);
            //num_macros_placed++;
        } else {
            place_a_block(MAX_NUM_TRIES_TO_PLACE_BLOCKS_RANDOMLY, blk_id, pad_loc_type);
        }
    }
}

void initial_placement(enum e_pad_loc_type pad_loc_type, const char* constraints_file) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Go through cluster blocks to calculate the tightest placement
     * floorplan constraint for each constrained block
     */
    propagate_place_constraints();

    //Sort blocks and placement macros according to how difficult they are to place
    vtr::vector<ClusterBlockId, t_block_score> block_scores = assign_block_scores();
    std::vector<ClusterBlockId> sorted_blocks = sort_blocks(block_scores);

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

    /*Check whether the constraint file is NULL, if not, read in the block locations from the constraints file here*/
    if (strlen(constraints_file) != 0) {
        read_constraints(constraints_file);
    }

    /*Mark the blocks that have already been locked to one spot via floorplan constraints
     * as fixed so they do not get moved during initial placement or during simulated annealing*/
    mark_fixed_blocks();

    //Place the blocks in sorted order
    place_the_blocks(sorted_blocks, block_scores, pad_loc_type);

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
}
