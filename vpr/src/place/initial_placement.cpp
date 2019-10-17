#include "vtr_memory.h"
#include "vtr_random.h"

#include "globals.h"
#include "read_place.h"
#include "initial_placement.h"

/* The maximum number of tries when trying to place a carry chain at a    *
 * random location before trying exhaustive placement - find the fist     *
 * legal position and place it during initial placement.                  */
#define MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY 4

static t_pl_loc** legal_pos = nullptr; /* [0..device_ctx.num_block_types-1][0..type_tsize - 1] */
static int* num_legal_pos = nullptr;   /* [0..num_legal_pos-1] */

static void alloc_legal_placements();
static void load_legal_placements();

static void free_legal_placements();

static int check_macro_can_be_placed(t_pl_macro pl_macro, int itype, t_pl_loc head_pos);
static int try_place_macro(int itype, int ipos, t_pl_macro pl_macro);
static void initial_placement_pl_macros(int macros_max_num_tries, int* free_locations);

static void initial_placement_blocks(int* free_locations, enum e_pad_loc_type pad_loc_type);
static void initial_placement_location(const int* free_locations, int& pipos, int itype, t_pl_loc& to);

static t_physical_tile_type_ptr pick_placement_type(t_logical_block_type_ptr logical_block,
                                                    int num_needed_types,
                                                    int* free_locations);

static void alloc_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    legal_pos = new t_pl_loc*[device_ctx.physical_tile_types.size()];
    num_legal_pos = (int*)vtr::calloc(device_ctx.physical_tile_types.size(), sizeof(int));

    /* Initialize all occupancy to zero. */

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            place_ctx.grid_blocks[i][j].usage = 0;

            for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] != INVALID_BLOCK_ID) {
                    place_ctx.grid_blocks[i][j].blocks[k] = EMPTY_BLOCK_ID;
                    if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                        num_legal_pos[device_ctx.grid[i][j].type->index]++;
                    }
                }
            }
        }
    }

    for (const auto& type : device_ctx.physical_tile_types) {
        legal_pos[type.index] = new t_pl_loc[num_legal_pos[type.index]];
    }
}

static void load_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    int* index = (int*)vtr::calloc(device_ctx.physical_tile_types.size(), sizeof(int));

    for (size_t i = 0; i < device_ctx.grid.width(); i++) {
        for (size_t j = 0; j < device_ctx.grid.height(); j++) {
            for (int k = 0; k < device_ctx.grid[i][j].type->capacity; k++) {
                if (place_ctx.grid_blocks[i][j].blocks[k] == INVALID_BLOCK_ID) {
                    continue;
                }
                if (device_ctx.grid[i][j].width_offset == 0 && device_ctx.grid[i][j].height_offset == 0) {
                    int itype = device_ctx.grid[i][j].type->index;
                    legal_pos[itype][index[itype]].x = i;
                    legal_pos[itype][index[itype]].y = j;
                    legal_pos[itype][index[itype]].z = k;
                    index[itype]++;
                }
            }
        }
    }
    free(index);
}

static void free_legal_placements() {
    auto& device_ctx = g_vpr_ctx.device();

    for (unsigned int i = 0; i < device_ctx.physical_tile_types.size(); i++) {
        delete[] legal_pos[i];
    }
    delete[] legal_pos; /* Free the mapping list */
    free(num_legal_pos);
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
            && place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.z] == EMPTY_BLOCK_ID) {
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

static int try_place_macro(int itype, int ipos, t_pl_macro pl_macro) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    int macro_placed = false;

    // Choose a random position for the head
    t_pl_loc head_pos = legal_pos[itype][ipos];

    // If that location is occupied, do nothing.
    if (place_ctx.grid_blocks[head_pos.x][head_pos.y].blocks[head_pos.z] != EMPTY_BLOCK_ID) {
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

            place_ctx.grid_blocks[member_pos.x][member_pos.y].blocks[member_pos.z] = pl_macro.members[imember].blk_index;
            place_ctx.grid_blocks[member_pos.x][member_pos.y].usage++;

            // Could not ensure that the randomiser would not pick this location again
            // So, would have to do a lazy removal - whenever I come across a block that could not be placed,
            // go ahead and remove it from the legal_pos[][] array

        } // Finish placing all the members in the macro

    } // End of this choice of legal_pos

    return (macro_placed);
}

static void initial_placement_pl_macros(int macros_max_num_tries, int* free_locations) {
    int macro_placed;
    int itype, itry, ipos;
    ClusterBlockId blk_id;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();
    auto& place_ctx = g_vpr_ctx.placement();

    auto& pl_macros = place_ctx.pl_macros;

    // The map serves to place first the most constrained block ids
    std::map<int, std::vector<t_pl_macro>> sorted_pl_macros_map;

    for (size_t imacro = 0; imacro < pl_macros.size(); imacro++) {
        blk_id = pl_macros[imacro].members[0].blk_index;
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        size_t num_equivalent_tiles = logical_block->equivalent_tiles.size();
        sorted_pl_macros_map[num_equivalent_tiles].push_back(pl_macros[imacro]);
    }

    /* Macros are harder to place.  Do them first */
    for (auto& sorted_pl_macros : sorted_pl_macros_map) {
        for (auto& pl_macro : sorted_pl_macros.second) {
            // Every macro are not placed in the beginnning
            macro_placed = false;

            // Assume that all the blocks in the macro are of the same type
            blk_id = pl_macro.members[0].blk_index;
            auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);
            auto type = pick_placement_type(logical_block, int(pl_macro.members.size()), free_locations);

            if (type == nullptr) {
                VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                "Initial placement failed.\n"
                                "Could not place macro length %zu with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
                                "VPR cannot auto-size for your circuit, please resize the FPGA manually.\n",
                                pl_macro.members.size(), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), logical_block->name, logical_block->index);
            }

            itype = type->index;

            // Try to place the macro first, if can be placed - place them, otherwise try again
            for (itry = 0; itry < macros_max_num_tries && macro_placed == false; itry++) {
                // Choose a random position for the head
                ipos = vtr::irand(free_locations[itype] - 1);

                // Try to place the macro
                macro_placed = try_place_macro(itype, ipos, pl_macro);

            } // Finished all tries

            if (macro_placed == false) {
                // if a macro still could not be placed after macros_max_num_tries times,
                // go through the chip exhaustively to find a legal placement for the macro
                // place the macro on the first location that is legal
                // then set macro_placed = true;
                // if there are no legal positions, error out

                // Exhaustive placement of carry macros
                for (ipos = 0; ipos < free_locations[itype] && macro_placed == false; ipos++) {
                    // Try to place the macro
                    macro_placed = try_place_macro(itype, ipos, pl_macro);

                } // Exhausted all the legal placement position for this macro

                // If macro could not be placed after exhaustive placement, error out
                if (macro_placed == false) {
                    // Error out
                    VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                                    "Initial placement failed.\n"
                                    "Could not place macro length %zu with head block %s (#%zu); not enough free locations of type %s (#%d).\n"
                                    "Please manually size the FPGA because VPR can't do this yet.\n",
                                    pl_macro.members.size(), cluster_ctx.clb_nlist.block_name(blk_id).c_str(), size_t(blk_id), device_ctx.physical_tile_types[itype].name, itype);
                }

            } else {
                // This macro has been placed successfully, proceed to place the next macro
                continue;
            }
        }
    } // Finish placing all the pl_macros successfully
}

/* Place blocks that are NOT a part of any macro.
 * We'll randomly place each block in the clustered netlist, one by one. */
static void initial_placement_blocks(int* free_locations, enum e_pad_loc_type pad_loc_type) {
    int itype, ipos;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // The map serves to place first the most constrained block ids
    std::map<int, std::vector<ClusterBlockId>> sorted_block_map;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        size_t num_equivalent_tiles = logical_block->equivalent_tiles.size();
        sorted_block_map[num_equivalent_tiles].push_back(blk_id);
    }

    for (auto& sorted_blocks : sorted_block_map) {
        for (auto blk_id : sorted_blocks.second) {
            if (place_ctx.block_locs[blk_id].loc.x != -1) { // -1 is a sentinel for an empty block
                // block placed.
                continue;
            }

            auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

            /* Don't do IOs if the user specifies IOs; we'll read those locations later. */
            if (!(is_io_type(pick_random_physical_type(logical_block)) && pad_loc_type == USER)) {
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

                itype = type->index;

                t_pl_loc to;
                initial_placement_location(free_locations, ipos, itype, to);

                // Make sure that the position is EMPTY_BLOCK before placing the block down
                VTR_ASSERT(place_ctx.grid_blocks[to.x][to.y].blocks[to.z] == EMPTY_BLOCK_ID);

                place_ctx.grid_blocks[to.x][to.y].blocks[to.z] = blk_id;
                place_ctx.grid_blocks[to.x][to.y].usage++;

                place_ctx.block_locs[blk_id].loc = to;

                //Mark IOs as fixed if specifying a (fixed) random placement
                if (is_io_type(pick_random_physical_type(logical_block)) && pad_loc_type == RANDOM) {
                    place_ctx.block_locs[blk_id].is_fixed = true;
                }

                /* Ensure randomizer doesn't pick this location again, since it's occupied. Could shift all the
                 * legal positions in legal_pos to remove the entry (choice) we just used, but faster to
                 * just move the last entry in legal_pos to the spot we just used and decrement the
                 * count of free_locations. */
                legal_pos[itype][ipos] = legal_pos[itype][free_locations[itype] - 1]; /* overwrite used block position */
                free_locations[itype]--;
            }
        }
    }
}

static void initial_placement_location(const int* free_locations, int& ipos, int itype, t_pl_loc& to) {
    ipos = vtr::irand(free_locations[itype] - 1);
    to = legal_pos[itype][ipos];
}

static t_physical_tile_type_ptr pick_placement_type(t_logical_block_type_ptr logical_block,
                                                    int num_needed_types,
                                                    int* free_locations) {
    // Loop through the ordered map to get tiles in a decreasing priority order
    for (auto& tile : logical_block->equivalent_tiles) {
        if (free_locations[tile->index] >= num_needed_types) {
            return tile;
        }
    }

    return nullptr;
}

void initial_placement(enum e_pad_loc_type pad_loc_type,
                       const char* pad_loc_file) {
    /* Randomly places the blocks to create an initial placement. We rely on
     * the legal_pos array already being loaded.  That legal_pos[itype] is an
     * array that gives every legal value of (x,y,z) that can accommodate a block.
     * The number of such locations is given by num_legal_pos[itype].
     */

    // Loading legal placement locations
    alloc_legal_placements();
    load_legal_placements();

    int itype, ipos;
    int* free_locations; /* [0..device_ctx.num_block_types-1].
                          * Stores how many locations there are for this type that *might* still be free.
                          * That is, this stores the number of entries in legal_pos[itype] that are worth considering
                          * as you look for a free location.
                          */
    auto& device_ctx = g_vpr_ctx.device();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    free_locations = (int*)vtr::malloc(device_ctx.physical_tile_types.size() * sizeof(int));
    for (const auto& type : device_ctx.physical_tile_types) {
        itype = type.index;
        free_locations[itype] = num_legal_pos[itype];
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

    initial_placement_pl_macros(MAX_NUM_TRIES_TO_PLACE_MACROS_RANDOMLY, free_locations);

    // All the macros are placed, update the legal_pos[][] array
    for (const auto& type : device_ctx.physical_tile_types) {
        itype = type.index;
        VTR_ASSERT(free_locations[itype] >= 0);
        for (ipos = 0; ipos < free_locations[itype]; ipos++) {
            t_pl_loc pos = legal_pos[itype][ipos];

            // Check if that location is occupied.  If it is, remove from legal_pos
            if (place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.z] != EMPTY_BLOCK_ID && place_ctx.grid_blocks[pos.x][pos.y].blocks[pos.z] != INVALID_BLOCK_ID) {
                legal_pos[itype][ipos] = legal_pos[itype][free_locations[itype] - 1];
                free_locations[itype]--;

                // After the move, I need to check this particular entry again
                ipos--;
                continue;
            }
        }
    } // Finish updating the legal_pos[][] and free_locations[] array

    initial_placement_blocks(free_locations, pad_loc_type);

    if (pad_loc_type == USER) {
        read_user_pad_loc(pad_loc_file);
    }

    /* Restore legal_pos */
    load_legal_placements();

#ifdef VERBOSE
    VTR_LOG("At end of initial_placement.\n");
    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_INITIAL_CLB_PLACEMENT)) {
        print_clb_placement(getEchoFileName(E_ECHO_INITIAL_CLB_PLACEMENT));
    }
#endif
    free(free_locations);
    free_legal_placements();
}
