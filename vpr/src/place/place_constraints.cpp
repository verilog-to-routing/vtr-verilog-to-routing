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
