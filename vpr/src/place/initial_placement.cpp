#include "vtr_memory.h"
#include "vtr_random.h"
#include "vtr_time.h"

#include "globals.h"
#include "read_place.h"
#include "initial_placement.h"
#include "vpr_utils.h"

/* The maximum number of tries when trying to place a carry chain at a    *
 * random location before trying exhaustive placement - find the fist     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 4

static std::vector<std::vector<std::vector<t_pl_loc>>> legal_pos; /* [0..device_ctx.num_block_types-1][0..type_tsize - 1][0..num_sub_tiles - 1] */
static std::vector<std::vector<int>> num_legal_pos;               /* [0..num_sub_tiles - 1][0..num_legal_pos-1] */

// initialize usage to 0 and blockID to EMPTY_BLOCK_ID for all place_ctx.grid_block locations
static void zero_initialize_grid_blocks();

static int get_free_sub_tile(std::vector<std::vector<int>>& free_locations, int itype, std::vector<int> possible_sub_tiles);

static int check_macro_can_be_placed(t_pl_macro pl_macro, int itype, t_pl_loc head_pos);
static int try_place_macro(int itype, int ipos, int isub_tile, t_pl_macro pl_macro);
static void initial_placement_pl_macros(int macros_max_num_tries, std::vector<std::vector<int>>& free_locations);

static void initial_placement_blocks(std::vector<std::vector<int>>& free_locations, enum e_pad_loc_type pad_loc_type);

static t_physical_tile_type_ptr pick_placement_type(t_logical_block_type_ptr logical_block,
                                                    int num_needed_types,
                                                    std::vector<std::vector<int>>& free_locations);

static void zero_initialize_grid_blocks() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Initialize all occupancy to zero. */

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            place_ctx.grid_blocks[i][j].usage = 0;
            auto tile = device_ctx.grid[i][j].type;

            for (auto sub_tile : tile->sub_tiles) {
                auto capacity = sub_tile.capacity;

                for (int k = 0; k < capacity.total(); k++) {
                    if (place_ctx.grid_blocks[i][j].blocks[k + capacity.low] != INVALID_BLOCK_ID) {
                        place_ctx.grid_blocks[i][j].blocks[k + capacity.low] = EMPTY_BLOCK_ID;
                    }
                }
            }
        }
    }
}

static int get_free_sub_tile(std::vector<std::vector<int>>& free_locations, int itype, std::vector<int> possible_sub_tiles) {
    for (int sub_tile : possible_sub_tiles) {
        if (free_locations[itype][sub_tile] > 0) {
            return sub_tile;
        }
    }

    VPR_THROW(VPR_ERROR_PLACE, "No more sub tiles available for initial placement.");
}

static std::vector<int> get_possible_sub_tile_indices(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block) {
    std::vector<int> sub_tile_indices;

    for (auto sub_tile : physical_tile->sub_tiles) {
        auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), logical_block);
        if (result != sub_tile.equivalent_sites.end()) {
            sub_tile_indices.push_back(sub_tile.index);
        }
    }

    VTR_ASSERT(sub_tile_indices.size() > 0);
    return sub_tile_indices;
}

static int check_macro_can_be_placed(t_pl_macro pl_macro, int itype, t_pl_loc head_pos) {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    // Every macro can be placed until proven otherwise
    int macro_can_be_placed = true;

    // Check whether all the members can be placed
    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

        // Check whether the location could accept block of this type
        // Then check whether the location could still accommodate more blocks
        // Also check whether the member position is valid, that is the member's location
        // still within the chip's dimemsion and the member_z is allowed at that location on the grid
        if (member_pos.x < int(device_ctx.grid.width()) && member_pos.y < int(device_ctx.grid.height())
            && device_ctx.grid[member_pos.x][member_pos.y].type->index == itype
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

static int try_place_macro(int itype, int ipos, int isub_tile, t_pl_macro pl_macro) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    int macro_placed = false;

    // Choose a random position for the head
    t_pl_loc head_pos = legal_pos[itype][isub_tile][ipos];

    // If that location is occupied, do nothing.
    if (place_ctx.grid_blocks[head_pos.x][head_pos.y].blocks[head_pos.sub_tile] != EMPTY_BLOCK_ID) {
        return (macro_placed);
    }

    int macro_can_be_placed = check_macro_can_be_placed(pl_macro, itype, head_pos);

    if (macro_can_be_placed) {
        // Place down the macro
        macro_placed = true;
        for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
            t_pl_loc member_pos = head_pos + pl_macro.members[imember].offset;

            ClusterBlockId iblk = pl_macro.members[imember].blk_index;
            place_ctx.block_locs[iblk].loc = member_pos;

            place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.sub_tile] = pl_macro.members[imember].blk_index;
            place_ctx.grid_blocks[member_pos.x][member_pos.y].usage++;

            // Could not ensure that the randomiser would not pick this location again
            // So, would have to do a lazy removal - whenever I come across a block that could not be placed,
            // go ahead and remove it from the legal_pos[][] array

        } // Finish placing all the members in the macro

    } // End of this choice of legal_pos

    return (macro_placed);
}

static void initial_placement_pl_macros(int macros_max_num_tries, std::vector<std::vector<int>>& free_locations) {
    int macro_placed;
    int itype, itry, ipos, isub_tile;
    ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    auto& pl_macros = place_ctx.pl_macros;

    // Sorting blocks to place to have most constricted ones to be placed first
    std::vector<t_pl_macro> sorted_pl_macros(pl_macros.begin(), pl_macros.end());

    auto criteria = [&cluster_ctx](const t_pl_macro lhs, t_pl_macro rhs) {
        auto lhs_logical_block = cluster_ctx.clb_nlist.block_type(lhs.members[0].blk_index);
        auto rhs_logical_block = cluster_ctx.clb_nlist.block_type(rhs.members[0].blk_index);

        auto lhs_num_tiles = lhs_logical_block->equivalent_tiles.size();
        auto rhs_num_tiles = rhs_logical_block->equivalent_tiles.size();

        return lhs_num_tiles < rhs_num_tiles;
    };

    std::stable_sort(sorted_pl_macros.begin(), sorted_pl_macros.end(), criteria);

    /* Macros are harder to place.  Do them first */
    for (auto pl_macro : sorted_pl_macros) {
        // Every macro are not placed in the beginnning
        macro_placed = false;

        // Assume that all the blocks in the macro are of the same type
        blk_id = pl_macro.members[0].blk_index;
        auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);

        for (auto tile_type : block_type->equivalent_tiles) { //Try each possible tile type
            itype = tile_type->index;

            auto possible_sub_tiles = get_possible_sub_tile_indices(tile_type, block_type);

            // Try to place the macro first, if can be placed - place them, otherwise try again
            for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
                // Choose a free sub tile for the head
                isub_tile = get_free_sub_tile(free_locations, itype, possible_sub_tiles);

                // Choose a random position for the head
                ipos = vtr::irand(free_locations[itype][isub_tile] - 1);

                // Try to place the macro
                macro_placed = try_place_macro(itype, ipos, isub_tile, pl_macro);

            } // Finished all tries

            if (macro_placed == false) {
                // if a macro still could not be placed after macros_max_num_tries times,
                // go through the chip exhaustively to find a legal placement for the macro
                // place the macro on the first location that is legal
                // then set macro_placed = true;
                // if there are no legal positions, error out

                // Exhaustive placement of carry macros
                for (isub_tile = 0; tile_type->sub_tiles.size() && macro_placed == false; isub_tile++) {
                    for (ipos = 0; ipos < free_locations[itype][isub_tile] && macro_placed == false; ipos++) {
                        // Try to place the macro
                        macro_placed = try_place_macro(itype, ipos, isub_tile, pl_macro);
                    }
                } // Exhausted all the legal placement position for this macro

                // If macro could not be placed after exhaustive placement, error out
            } else {
                // This macro has been placed successfully
                break;
            }
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

    } // Finish placing all the pl_macros successfully
}

/* Place blocks that are NOT a part of any macro.
 * We'll randomly place each block in the clustered netlist, one by one. */
static void initial_placement_blocks(std::vector<std::vector<int>>& free_locations, enum e_pad_loc_type pad_loc_type) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    auto blocks = cluster_ctx.clb_nlist.blocks();

    // Sorting blocks to place to have most constricted ones to be placed first
    std::vector<ClusterBlockId> sorted_blocks(blocks.begin(), blocks.end());

    auto criteria = [&cluster_ctx](const ClusterBlockId lhs, ClusterBlockId rhs) {
        auto lhs_logical_block = cluster_ctx.clb_nlist.block_type(lhs);
        auto rhs_logical_block = cluster_ctx.clb_nlist.block_type(rhs);

        auto lhs_num_tiles = lhs_logical_block->equivalent_tiles.size();
        auto rhs_num_tiles = rhs_logical_block->equivalent_tiles.size();

        return lhs_num_tiles < rhs_num_tiles;
    };

    if (device_ctx.has_multiple_equivalent_tiles) {
        std::sort(sorted_blocks.begin(), sorted_blocks.end(), criteria);
    }

    for (auto blk_id : sorted_blocks) {
        /* -1 is a sentinel for a non-placed block, which the code in this routine will choose a location for.
         * If the x value is not -1, we assume something else has already placed this block and we should leave it there.
         * For example, if the user constrained it to a certain location, the block has already been placed.
         */
        if (place_ctx.block_locs[blk_id].loc.x != -1) {
            //put VTR assert
            continue;
        }

        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        /* Randomly select a free location of the appropriate type for blk_id.
         * We have a linearized list of all the free locations that can
         * accommodate a block of that type in free_locations[itype].
         * Choose one randomly and put blk_id there. Then we don't want to pick
         * that location again, so remove it from the free_locations array.
         */

        auto type = pick_placement_type(logical_block, 1, free_locations);

        if (type == nullptr) {
            VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                            "Initial placement failed.\n"
                            "Could not place block %s (#%zu); no free locations of type %s (#%d).\n",
                            cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), logical_block->name, logical_block->index);
        }

        int itype = type->index;

        auto possible_sub_tiles = get_possible_sub_tile_indices(type, logical_block);

        int isub_tile = get_free_sub_tile(free_locations, itype, possible_sub_tiles);

        t_pl_loc to;
        int ipos = vtr::irand(free_locations[itype][isub_tile] - 1);

        to = legal_pos[itype][isub_tile][ipos];

        // Make sure that the position is EMPTY_BLOCK before placing the block down
        VTR_ASSERT(place_ctx.grid_blocks[to.x][to.y].blocks[to.sub_tile] == EMPTY_BLOCK_ID);

        place_ctx.grid_blocks[to.x][to.y].blocks[to.sub_tile] = blk_id;
        place_ctx.grid_blocks[to.x][to.y].usage++;

        place_ctx.block_locs[blk_id].loc = to;

        //Mark IOs as fixed if specifying a (fixed) random placement
        if (is_io_type(pick_best_physical_type(logical_block)) && pad_loc_type == RANDOM) {
            place_ctx.block_locs[blk_id].is_fixed = true;
        }

        /* Ensure randomizer doesn't pick this location again, since it's occupied. Could shift all the
         * legal positions in legal_pos to remove the entry (choice) we just used, but faster to
         * just move the last entry in legal_pos to the spot we just used and decrement the
         * count of free_locations. */
        legal_pos[itype][isub_tile][ipos] = legal_pos[itype][isub_tile][free_locations[itype][isub_tile] - 1]; /* overwrite used block position */
        free_locations[itype][isub_tile]--;
    }
}

static t_physical_tile_type_ptr pick_placement_type(t_logical_block_type_ptr logical_block,
                                                    int num_needed_types,
                                                    std::vector<std::vector<int>>& free_locations) {
    // Loop through the ordered map to get tiles in a decreasing priority order
    for (auto& tile : logical_block->equivalent_tiles) {
        auto possible_sub_tiles = get_possible_sub_tile_indices(tile, logical_block);
        for (auto sub_tile_index : possible_sub_tiles) {
            if (free_locations[tile->index][sub_tile_index] >= num_needed_types) {
                return tile;
            }
        }
    }

    return nullptr;
}

void initial_placement(enum e_pad_loc_type pad_loc_type, const char* constraints_file) {
    vtr::ScopedStartFinishTimer timer("Initial Placement");

    /* Randomly places the blocks to create an initial placement. We rely on
     * the legal_pos array already being loaded.  That legal_pos[itype] is an
     * array that gives every legal value of (x,y,z) that can accommodate a block.
     * The number of such locations is given by num_legal_pos[itype].
     */

    // Loading legal placement locations
    zero_initialize_grid_blocks();
    alloc_legal_placement_locations(legal_pos, num_legal_pos);
    load_legal_placement_locations(legal_pos);

    int itype, ipos;
    std::vector<std::vector<int>> free_locations; /* [0..device_ctx.num_block_types-1].
                                                   * Stores how many locations there are for this type that *might* still be free.
                                                   * That is, this stores the number of entries in legal_pos[itype] that are worth considering
                                                   * as you look for a free location.
                                                   */
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    free_locations.resize(device_ctx.physical_tile_types.size());
    for (const auto& type : device_ctx.physical_tile_types) {
        itype = type.index;
        free_locations[itype].resize(type.sub_tiles.size());

        for (auto sub_tile : type.sub_tiles) {
            free_locations[itype][sub_tile.index] = num_legal_pos[itype][sub_tile.index];
        }
    }

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

    initial_placement_pl_macros(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, free_locations);

    // All the macros are placed, update the legal_pos[][] array
    for (const auto& type : device_ctx.physical_tile_types) {
        itype = type.index;

        for (auto sub_tile : type.sub_tiles) {
            int isub_tile = sub_tile.index;

            VTR_ASSERT(free_locations[itype][isub_tile] >= 0);
            for (ipos = 0; ipos < free_locations[itype][isub_tile]; ipos++) {
                t_pl_loc pos = legal_pos[itype][isub_tile][ipos];

                // Check if that location is occupied.  If it is, remove from legal_pos
                if (place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.sub_tile] != EMPTY_BLOCK_ID && place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.sub_tile] != INVALID_BLOCK_ID) {
                    legal_pos[itype][isub_tile][ipos] = legal_pos[itype][isub_tile][free_locations[itype][isub_tile] - 1];
                    free_locations[itype][isub_tile]--;

                    // After the move, I need to check this particular entry again
                    ipos--;
                    continue;
                }
            }
        }
    } // Finish updating the legal_pos[][] and free_locations[] array

    initial_placement_blocks(free_locations, pad_loc_type);

    /* Restore legal_pos */
    load_legal_placement_locations(legal_pos);

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
}
