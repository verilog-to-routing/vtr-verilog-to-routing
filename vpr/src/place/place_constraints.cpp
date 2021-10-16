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
    PartitionRegion pr;
    pr = floorplanning_ctx.cluster_constraints[blk_id];
    return (!pr.empty());
}

bool is_macro_constrained(const t_pl_macro& pl_macro) {
    bool is_macro_constrained = false;
    bool is_member_constrained = false;

    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        ClusterBlockId iblk = pl_macro.members[imember].blk_index;
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

    for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
        ClusterBlockId iblk = pl_macro.members[imember].blk_index;
        is_member_constrained = is_cluster_constrained(iblk);

        if (is_member_constrained) {
            num_constrained_members++;
            //PartitionRegion of the constrained block
            PartitionRegion block_pr;
            //PartitionRegion of the constrained block modified for the head according to the offset
            PartitionRegion modified_pr;

            block_pr = floorplanning_ctx.cluster_constraints[iblk];
            std::vector<Region> block_regions = block_pr.get_partition_region();

            for (unsigned int i = 0; i < block_regions.size(); i++) {
                Region modified_reg;
                auto offset = pl_macro.members[imember].offset;

                vtr::Rect<int> reg_rect = block_regions[i].get_region_rect();

                modified_reg.set_region_rect(reg_rect.xmin() - offset.x, reg_rect.ymin() - offset.y, reg_rect.xmax() - offset.x, reg_rect.ymax() - offset.y);

                //check that subtile is not an invalid value before changing, otherwise it just stays -1
                if (block_regions[i].get_sub_tile() != NO_SUBTILE) {
                    modified_reg.set_sub_tile(block_regions[i].get_sub_tile() - offset.sub_tile);
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
    std::vector<Region> block_regions = head_pr.get_partition_region();
    PartitionRegion macro_pr;

    for (unsigned int i = 0; i < block_regions.size(); i++) {
        Region modified_reg;

        vtr::Rect<int> reg_rect = block_regions[i].get_region_rect();

        modified_reg.set_region_rect(reg_rect.xmin() + offset.x, reg_rect.ymin() + offset.y, reg_rect.xmax() + offset.x, reg_rect.ymax() + offset.y);

        //check that subtile is not an invalid value before changing, otherwise it just stays -1
        if (block_regions[i].get_sub_tile() != NO_SUBTILE) {
            modified_reg.set_sub_tile(block_regions[i].get_sub_tile() + offset.sub_tile);
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
    for (unsigned int i = 0; i < pl_macro.members.size(); i++) {
        std::string blk_name = cluster_ctx.clb_nlist.block_name((pl_macro.members[i].blk_index));
        VTR_LOG("Block %s (#%zu) ", blk_name.c_str(), size_t(pl_macro.members[i].blk_index));
    }
    VTR_LOG("\n");
    VPR_ERROR(VPR_ERROR_PLACE, " \n Check that the above-mentioned placement macro blocks have compatible floorplan constraints.\n");
}

void propagate_place_constraints() {
    auto& place_ctx = g_vpr_ctx.placement();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    //Create a PartitionRegion with grid dimensions
    //Will be used to check that updated PartitionRegions are within grid bounds
    int width = device_ctx.grid.width() - 1;
    int height = device_ctx.grid.height() - 1;
    Region grid_reg;
    grid_reg.set_region_rect(0, 0, width, height);
    PartitionRegion grid_pr;
    grid_pr.add_to_part_region(grid_reg);

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
    ClusterAtomsLookup atoms_lookup;

    floorplanning_ctx.cluster_constraints.resize(cluster_ctx.clb_nlist.blocks().size());

    for (auto cluster_id : cluster_ctx.clb_nlist.blocks()) {
        std::vector<AtomBlockId> atoms = atoms_lookup.atoms_in_cluster(cluster_id);
        PartitionRegion empty_pr;
        floorplanning_ctx.cluster_constraints[cluster_id] = empty_pr;

        //if there are any constrainted atoms in the cluster,
        //we update the cluster's PartitionRegion
        for (unsigned int i = 0; i < atoms.size(); i++) {
            PartitionId partid = floorplanning_ctx.constraints.get_atom_partition(atoms[i]);

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
    vtr::Rect<int> rb = reg.get_region_rect();
    int num_tiles = 0;

    for (int x = rb.xmin(); x <= rb.xmax(); x++) {
        for (int y = rb.ymin(); y <= rb.ymax(); y++) {
            auto& tile = device_ctx.grid[x][y].type;

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
 * Used when marking fixed blocks to check whether the ParitionRegion associated with a block
 * covers one tile. If it covers one tile, it is marked as fixed. If it covers 0 tiles or
 * more than one tile, it will not be marked as fixed. As soon as it is known that the
 * PartitionRegion covers more than one tile, there is no need to check further regions
 * and the routine will return false.
 */
bool is_pr_size_one(PartitionRegion& pr, t_logical_block_type_ptr block_type, t_pl_loc& loc) {
    auto& device_ctx = g_vpr_ctx.device();
    std::vector<Region> regions = pr.get_partition_region();
    bool pr_size_one;
    int pr_size = 0;
    int reg_size;

    Region intersect_reg;
    intersect_reg.set_region_rect(0, 0, device_ctx.grid.width() - 1, device_ctx.grid.height() - 1);
    Region current_reg;

    for (unsigned int i = 0; i < regions.size(); i++) {
        reg_size = region_tile_cover(regions[i], block_type, loc);

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
            current_reg.set_region_rect(loc.x, loc.y, loc.x, loc.y);
            current_reg.set_sub_tile(loc.sub_tile);
            intersect_reg = intersection(intersect_reg, current_reg);

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
    std::vector<Region> part_reg = pr.get_partition_region();
    int num_tiles = 0;

    for (unsigned int i_reg = 0; i_reg < part_reg.size(); i_reg++) {
        num_tiles += grid_tiles.region_tile_count(part_reg[i_reg], block_type);
    }

    return num_tiles;
}

int get_floorplan_score(ClusterBlockId blk_id, PartitionRegion& pr, t_logical_block_type_ptr block_type, GridTileLookup& grid_tiles) {
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

    return total_type_tiles - num_pr_tiles;
}
