/*
 * place_constraints.cpp
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#include "globals.h"
#include "place_constraints.h"

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

//returns true if cluster has floorplanning constraints, false if
//it doesn't
bool is_cluster_constrained(ClusterBlockId blk_id) {
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    PartitionRegion pr;
    pr = floorplanning_ctx.cluster_constraints[blk_id];
    if (pr.empty()) {
    	//VTR_LOG("cluster constrained returned false \n");
    	return false;
    } else {
    	//VTR_LOG("cluster constrained returned true \n");
    	return true;
    }
}

//returns true if no floorplanning issues,
//false if not
bool cluster_floorplanning_check(ClusterBlockId blk_id, t_pl_loc loc) {
	auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

	bool floorplanning_good = false;

	bool cluster_constrained = is_cluster_constrained(blk_id);

	if(!cluster_constrained) {
		//not constrained so will not have floorplanning issues
		//VTR_LOG("The block is not constrained in cluster_floorplanning_check \n");
		floorplanning_good = true;
		return floorplanning_good;
	} else {
        PartitionRegion pr;
        pr = floorplanning_ctx.cluster_constraints[blk_id];
        bool in_pr = pr.is_loc_in_part_reg(loc);

        //if location is in partitionregion, floorplanning is respected
        //if not it is not
        if(in_pr){
        	//VTR_LOG("The block is in the part_region in cluster_floorplanning_check \n");
        	floorplanning_good = true;
        	return floorplanning_good;
        }
        //VTR_LOG("The block did not pass cluster_floorplanning_check \n");
        return floorplanning_good;
	}
}

