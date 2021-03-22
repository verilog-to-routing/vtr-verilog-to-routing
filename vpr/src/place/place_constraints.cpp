/*
 * place_constraints.cpp
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#include "globals.h"
#include "place_constraints.h"

/*checks that each block's location is compatible with its floorplanning constraints if it has any*/
int check_placement_floorplanning() {
    int error = 0;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        auto& loc = place_ctx.block_locs[blk_id].loc;
        if (cluster_floorplanning_check(blk_id, loc)) {
            continue;
        } else {
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
    if (pr.empty()) {
        return false;
    } else {
        return true;
    }
}

bool is_macro_constrained(t_pl_macro pl_macro) {
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

std::vector<t_pl_loc> constrained_macro_locs(t_pl_macro pl_macro) {
	std::vector<t_pl_loc> locations;
	PartitionRegion macro_pr;
	bool is_member_constrained = false;
	int num_constrained_members;
	auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

	for (size_t imember = 0; imember < pl_macro.members.size(); imember++) {
		ClusterBlockId iblk = pl_macro.members[imember].blk_index;
		is_member_constrained = is_cluster_constrained(iblk);

		if (is_member_constrained) {
			num_constrained_members++;
			//PartitionRegion of the block
			PartitionRegion block_pr;
			//PartitionRegion of the constrained cluster modified for the head according to the offset
			PartitionRegion modified_pr;

			block_pr = floorplanning_ctx.cluster_constraints[iblk];
			std::vector<Region> block_regions = block_pr.get_partition_region();

			for (unsigned int i = 0; i < block_regions.size(); i++) {
				Region modified_reg;
				auto offset = pl_macro.members[imember].offset;

				vtr::Rect<int> reg_rect = block_regions[i].region_bounds;

				int m_xmin = reg_rect.xmin() + offset;
				int m_ymin = reg_rect.ymin() + offset;
				int m_xmax = reg_rect.xmax() + offset;
				int m_ymax = reg_rect.ymax() + offset;

				int m_subtile = block_regions[i].sub_tile + offset;
			}

			if (num_constrained_members == 1) {
				macro_pr = modified_pr;
			} else {
				macro_pr = intersection(macro_pr, modified_pr);
			}
		}


	}

	return locations;
}

/*returns true if location is compatible with floorplanning constraints, false if not*/
bool cluster_floorplanning_check(ClusterBlockId blk_id, t_pl_loc loc) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    bool floorplanning_good = false;

    bool cluster_constrained = is_cluster_constrained(blk_id);

    if (!cluster_constrained) {
        //not constrained so will not have floorplanning issues
        floorplanning_good = true;
        return floorplanning_good;
    } else {
        PartitionRegion pr;
        pr = floorplanning_ctx.cluster_constraints[blk_id];
        bool in_pr = pr.is_loc_in_part_reg(loc);

        //if location is in partitionregion, floorplanning is respected
        //if not it is not
        if (in_pr) {
            floorplanning_good = true;
            return floorplanning_good;
        }
        VTR_LOG("The block %zu did not pass cluster_floorplanning_check \n", size_t(blk_id));
        VTR_LOG("Loc is x: %d, y: %d, subtile: %d \n", loc.x, loc.y, loc.sub_tile);
        return floorplanning_good;
    }
}
