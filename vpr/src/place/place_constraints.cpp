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
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();

    //for each cluster block
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        //check if it has floorplanning constraints
        PartitionRegion pr;
        pr = floorplanning_ctx.cluster_constraints[blk_id];
        if (pr.empty()) {
            //no floorplanning constraints
            continue;
        } else {
            //get location of the block
            auto& loc = place_ctx.block_locs[blk_id].loc;

            //check if the location is in the PartitionRegion
            bool in_pr = pr.is_loc_in_part_reg(loc);

            if (!in_pr) {
                error++;
                VTR_LOG_ERROR("Block %zu is not in correct floorplanning region.\n", size_t(blk_id));
            }
        }
    }

    return error;
}
