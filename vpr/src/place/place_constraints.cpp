/*
 * place_constraints.cpp
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 *
 *  This file contains routines that help with making sure floorplanning constraints are respected throughout
 *  the placement stage of VPR.
 */

#include "globals.h"
#include "place_constraints.h"
#include "place_util.h"
#include "re_cluster_util.h"

/*checks that each block's location is compatible with its floorplanning constraints if it has any*/
int check_placement_floorplanning() {
    int error = 0;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto loc = place_ctx.block_locs[blk_id].loc;
        if (!cluster_floorplanning_legal(blk_id, loc)) {
            error++;
            VTR_LOG_ERROR("Block %zu is not in correct floorplanning region.\n", size_t(blk_id));
        }
    }

    return error;
}

/*returns true if cluster has floorplanning constraints, false if it doesn't*/
bool is_cluster_constrained(ClusterBlockId blk_id) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    const PartitionRegion& pr = floorplanning_ctx.cluster_constraints[blk_id];
    return (!pr.empty());
}

bool is_macro_constrained(const t_pl_macro& pl_macro) {
    bool is_macro_constrained = false;
    bool is_member_constrained = false;

    for (const auto& member : pl_macro.members) {
        ClusterBlockId iblk = member.blk_index;
        is_member_constrained = is_cluster_constrained(iblk);

        if (is_member_constrained) {
            is_macro_constrained = true;
            break;
        }
    }

    return is_macro_constrained;
}

/*Returns PartitionRegion of where the head of the macro could go*/
PartitionRegion update_macro_head_pr(const t_pl_macro& pl_macro, const PartitionRegion& grid_pr) {
    PartitionRegion macro_head_pr;
    bool is_member_constrained = false;
    int num_constrained_members = 0;
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    for (const auto& member : pl_macro.members) {
        ClusterBlockId iblk = member.blk_index;
        is_member_constrained = is_cluster_constrained(iblk);

        if (is_member_constrained) {
            num_constrained_members++;

            //PartitionRegion of the constrained block modified for the head according to the offset
            PartitionRegion modified_pr;

            //PartitionRegion of the constrained block
            const PartitionRegion& block_pr = floorplanning_ctx.cluster_constraints[iblk];
            const std::vector<Region>& block_regions = block_pr.get_regions();

            for (const auto& block_region : block_regions) {
                Region modified_reg;
                auto offset = member.offset;

                const auto block_reg_coord = block_region.get_region_rect();

                modified_reg.set_region_rect({block_reg_coord.xmin - offset.x,
                                              block_reg_coord.ymin - offset.y,
                                              block_reg_coord.xmax - offset.x,
                                              block_reg_coord.ymax - offset.y,
                                              block_reg_coord.layer_num});

                //check that subtile is not an invalid value before changing, otherwise it just stays -1
                if (block_region.get_sub_tile() != NO_SUBTILE) {
                    modified_reg.set_sub_tile(block_region.get_sub_tile() - offset.sub_tile);
                }

                modified_pr.add_to_part_region(modified_reg);
            }

            if (num_constrained_members == 1) {
                macro_head_pr = modified_pr;
            } else {
                macro_head_pr = intersection(macro_head_pr, modified_pr);
            }
        }
    }

    //intersect to ensure the head pr does not go outside of grid dimensions
    macro_head_pr = intersection(macro_head_pr, grid_pr);

    //if the intersection is empty, no way to place macro members together, give an error
    if (macro_head_pr.empty()) {
        print_macro_constraint_error(pl_macro);
    }

    return macro_head_pr;
}

PartitionRegion update_macro_member_pr(PartitionRegion& head_pr, const t_pl_offset& offset, const PartitionRegion& grid_pr, const t_pl_macro& pl_macro) {
    const std::vector<Region>& block_regions = head_pr.get_regions();
    PartitionRegion macro_pr;

    for (const auto& block_region : block_regions) {
        Region modified_reg;

        const auto block_reg_coord = block_region.get_region_rect();

        modified_reg.set_region_rect({block_reg_coord.xmin + offset.x,
                                      block_reg_coord.ymin + offset.y,
                                      block_reg_coord.xmax + offset.x,
                                      block_reg_coord.ymax + offset.y,
                                      block_reg_coord.layer_num});

        //check that subtile is not an invalid value before changing, otherwise it just stays -1
        if (block_region.get_sub_tile() != NO_SUBTILE) {
            modified_reg.set_sub_tile(block_region.get_sub_tile() + offset.sub_tile);
        }

        macro_pr.add_to_part_region(modified_reg);
    }

    //intersect to ensure the macro pr does not go outside of grid dimensions
    macro_pr = intersection(macro_pr, grid_pr);

    //if the intersection is empty, no way to place macro members together, give an error
    if (macro_pr.empty()) {
        print_macro_constraint_error(pl_macro);
    }

    return macro_pr;
}

void print_macro_constraint_error(const t_pl_macro& pl_macro) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    VTR_LOG(
        "Feasible floorplanning constraints could not be calculated for the placement macro. \n"
        "The placement macro contains the following blocks: \n");
    for (const auto& member : pl_macro.members) {
        std::string blk_name = cluster_ctx.clb_nlist.block_name((member.blk_index));
        VTR_LOG("Block %s (#%zu) ", blk_name.c_str(), size_t(member.blk_index));
    }
    VTR_LOG("\n");
    VPR_ERROR(VPR_ERROR_PLACE, " \n Check that the above-mentioned placement macro blocks have compatible floorplan constraints.\n");
}

void propagate_place_constraints() {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    int num_layers = device_ctx.grid.get_num_layers();
    Region grid_reg;
    PartitionRegion grid_pr;

    for (int layer_num = 0; layer_num < num_layers; layer_num++) {
        //Create a PartitionRegion with grid dimensions
        //Will be used to check that updated PartitionRegions are within grid bounds
        int width = device_ctx.grid.width() - 1;
        int height = device_ctx.grid.height() - 1;

        grid_reg.set_region_rect({0, 0, width, height, layer_num});
        grid_pr.add_to_part_region(grid_reg);
    }

    for (auto pl_macro : place_ctx.pl_macros) {
        if (is_macro_constrained(pl_macro)) {
            /*
             * Get the PartitionRegion for the head of the macro
             * based on the constraints of all blocks contained in the macro
             */
            PartitionRegion macro_head_pr = update_macro_head_pr(pl_macro, grid_pr);

            //Update PartitionRegions of all members of the macro
            for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
                ClusterBlockId iblk = pl_macro.members[imember].blk_index;
                auto offset = pl_macro.members[imember].offset;

                if (imember == 0) { //Update head PR
                    floorplanning_ctx.cluster_constraints[iblk] = macro_head_pr;
                } else { //Update macro member PR
                    PartitionRegion macro_pr = update_macro_member_pr(macro_head_pr, offset, grid_pr, pl_macro);
                    floorplanning_ctx.cluster_constraints[iblk] = macro_pr;
                }
            }
        }
    }
}

/*returns true if location is compatible with floorplanning constraints, false if not*/
/*
 * Even if the block passed in is from a macro, it will work because of the constraints
 * propagation that was done during initial placement.
 */
bool cluster_floorplanning_legal(ClusterBlockId blk_id, const t_pl_loc& loc) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    bool floorplanning_good = false;

    bool cluster_constrained = is_cluster_constrained(blk_id);

    if (!cluster_constrained) {
        //not constrained so will not have floorplanning issues
        floorplanning_good = true;
    } else {
        PartitionRegion pr = floorplanning_ctx.cluster_constraints[blk_id];
        bool in_pr = pr.is_loc_in_part_reg(loc);

        //if location is in partitionregion, floorplanning is respected
        //if not it is not
        if (in_pr) {
            floorplanning_good = true;
        } else {
#ifdef VERBOSE
            VTR_LOG("Block %zu did not pass cluster_floorplanning_check \n", size_t(blk_id));
            VTR_LOG("Loc is x: %d, y: %d, subtile: %d \n", loc.x, loc.y, loc.sub_tile);
#endif
        }
    }

    return floorplanning_good;
}

void load_cluster_constraints() {
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    floorplanning_ctx.cluster_constraints.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto cluster_id : cluster_ctx.clb_nlist.blocks()) {
        std::unordered_set<AtomBlockId>* atoms = cluster_to_atoms(cluster_id);
        PartitionRegion empty_pr;
        floorplanning_ctx.cluster_constraints[cluster_id] = empty_pr;

        //if there are any constrained atoms in the cluster,
        //we update the cluster's PartitionRegion
        for (auto atom : *atoms) {
            PartitionId partid = floorplanning_ctx.constraints.get_atom_partition(atom);

            if (partid != PartitionId::INVALID()) {
                PartitionRegion pr = floorplanning_ctx.constraints.get_partition_pr(partid);
                if (floorplanning_ctx.cluster_constraints[cluster_id].empty()) {
                    floorplanning_ctx.cluster_constraints[cluster_id] = pr;
                } else {
                    PartitionRegion intersect_pr = intersection(pr, floorplanning_ctx.cluster_constraints[cluster_id]);
                    if (intersect_pr.empty()) {
                        VTR_LOG_ERROR("Cluster block %zu has atoms with incompatible floorplan constraints.\n", size_t(cluster_id));
                    } else {
                        floorplanning_ctx.cluster_constraints[cluster_id] = intersect_pr;
                    }
                }
            }
        }
    }
}

void mark_fixed_blocks() {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (!is_cluster_constrained(blk_id)) {
            continue;
        }
        PartitionRegion pr = floorplanning_ctx.cluster_constraints[blk_id];
        auto block_type = cluster_ctx.clb_nlist.block_type(blk_id);
        t_pl_loc loc;

        /*
         * If the block can be placed in exactly one
         * legal (x, y, subtile) location, place it now
         * and mark it as fixed.
         */
        if (is_pr_size_one(pr, block_type, loc)) {
            set_block_location(blk_id, loc);

            place_ctx.block_locs[blk_id].is_fixed = true;
        }
    }
}

/*
 * Returns 0, 1, or 2 depending on the number of tiles covered.
 * Will not return a value above 2 because as soon as num_tiles is above 1,
 * it is known that the block that is assigned to this region will not be fixed, and so
 * num_tiles is immediately returned.
 * Updates the location passed in because if num_tiles turns out to be 1 after checking the
 * region, the location that was set will be used as the location to which the block
 * will be fixed.
 */
int region_tile_cover(const Region& reg, t_logical_block_type_ptr block_type, t_pl_loc& loc) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto reg_coord = reg.get_region_rect();
    const int layer_num = reg.get_layer_num();
    int num_tiles = 0;

    for (int x = reg_coord.xmin; x <= reg_coord.xmax; x++) {
        for (int y = reg_coord.ymin; y <= reg_coord.ymax; y++) {
            const auto& tile = device_ctx.grid.get_physical_type({x, y, reg_coord.layer_num});

            /*
             * If the tile at the grid location is not compatible with the cluster block
             * type, do not count this tile for num_tiles
             */
            if (!is_tile_compatible(tile, block_type)) {
                continue;
            }

            /*
             * If the region passed has a specific subtile set, increment
             * the number of tiles set the location using the x, y, subtile
             * values if the subtile is compatible at this location
             */
            if (reg.get_sub_tile() != NO_SUBTILE) {
                if (is_sub_tile_compatible(tile, block_type, reg.get_sub_tile())) {
                    num_tiles++;
                    loc.x = x;
                    loc.y = y;
                    loc.sub_tile = reg.get_sub_tile();
                    loc.layer = layer_num;
                    if (num_tiles > 1) {
                        return num_tiles;
                    }
                }

                /*
                 * If the region passed in does not have a subtile set, set the
                 * subtile to the first possible slot found at this location.
                 */
            } else if (reg.get_sub_tile() == NO_SUBTILE) {
                int num_compatible_st = 0;

                for (int z = 0; z < tile->capacity; z++) {
                    if (is_sub_tile_compatible(tile, block_type, z)) {
                        num_tiles++;
                        num_compatible_st++;
                        if (num_compatible_st == 1) { //set loc.sub_tile to the first compatible subtile value found
                            loc.x = x;
                            loc.y = y;
                            loc.sub_tile = z;
                            loc.layer = layer_num;
                        }
                        if (num_tiles > 1) {
                            return num_tiles;
                        }
                    }
                }
            }
        }
    }

    return num_tiles;
}

/*
 * Used when marking fixed blocks to check whether the PartitionRegion associated with a block
 * covers one tile. If it covers one tile, it is marked as fixed. If it covers 0 tiles or
 * more than one tile, it will not be marked as fixed. As soon as it is known that the
 * PartitionRegion covers more than one tile, there is no need to check further regions
 * and the routine will return false.
 */
bool is_pr_size_one(PartitionRegion& pr, t_logical_block_type_ptr block_type, t_pl_loc& loc) {
    auto& device_ctx = g_vpr_ctx.device();
    const std::vector<Region>& regions = pr.get_regions();
    bool pr_size_one;
    int pr_size = 0;
    int reg_size;
    int num_layers = device_ctx.grid.get_num_layers();

    std::vector<Region> intersect_reg(num_layers);
    for (int layer_num = 0; layer_num < num_layers; ++layer_num) {
        intersect_reg[layer_num].set_region_rect({0,
                                                  0,
                                                  (int)device_ctx.grid.width() - 1,
                                                  (int)device_ctx.grid.height() - 1,
                                                  layer_num});
    }
    std::vector<Region> current_reg(num_layers);

    for (unsigned int i = 0; i < regions.size(); i++) {
        reg_size = region_tile_cover(regions[i], block_type, loc);
        int layer_num = regions[i].get_layer_num();

        /*
         * If multiple regions in the PartitionRegion all have size 1,
         * the block may still be marked as locked, in the case that
         * they all cover the exact same x, y, subtile location. To check whether this
         * is the case, whenever there is a size 1 region, it is intersected
         * with the previous size 1 regions to see whether it covers the same location.
         * If there is an intersection, it does cover the same location, and so pr_size is
         * not incremented (unless this is the first size 1 region encountered).
         */
        if (reg_size == 1) {
            //get the exact x, y, subtile location covered by the current region (regions[i])
            current_reg[layer_num].set_region_rect({loc.x, loc.y, loc.x, loc.y, layer_num});
            current_reg[layer_num].set_sub_tile(loc.sub_tile);
            intersect_reg[layer_num] = intersection(intersect_reg[layer_num], current_reg[layer_num]);

            if (i == 0 || intersect_reg.empty()) {
                pr_size = pr_size + reg_size;
                if (pr_size > 1) {
                    break;
                }
            }
        } else {
            pr_size = pr_size + reg_size;
            if (pr_size > 1) {
                break;
            }
        }
    }

    if (pr_size == 1) {
        pr_size_one = true;
    } else { //pr_size = 0 or pr_size > 1
        pr_size_one = false;
    }

    return pr_size_one;
}

int get_part_reg_size(PartitionRegion& pr, t_logical_block_type_ptr block_type, GridTileLookup& grid_tiles) {
    const std::vector<Region>& regions = pr.get_regions();
    int num_tiles = 0;

    for (const auto& region : regions) {
        num_tiles += grid_tiles.region_tile_count(region, block_type);
    }

    return num_tiles;
}

double get_floorplan_score(ClusterBlockId blk_id, PartitionRegion& pr, t_logical_block_type_ptr block_type, GridTileLookup& grid_tiles) {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int num_pr_tiles = get_part_reg_size(pr, block_type, grid_tiles);

    if (num_pr_tiles == 0) {
        VPR_FATAL_ERROR(VPR_ERROR_PLACE,
                        "Initial placement failed.\n"
                        "The specified floorplan region for block %s (# %d) has no available locations for its type. \n"
                        "Please specify a different floorplan region for the block. Note that if the region has a specified subtile, "
                        "an incompatible subtile location may be the cause of the floorplan region failure. \n",
                        cluster_ctx.clb_nlist.block_name(blk_id).c_str(), blk_id);
    }

    int total_type_tiles = grid_tiles.total_type_tiles(block_type);

    return (double)(total_type_tiles - num_pr_tiles) / total_type_tiles;
}
