/*
 * place_constraints.cpp
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#include "globals.h"
#include "place_constraints.h"

struct t_block_scores {
    int macro_size = 0; //how many member does the macro have, if it is part of one

    int num_floorplan_constraints = 0; //how many floorplan constraints does it have, if any

    int num_equivalent_tiles = 1; //how many equivalent tiles does block have to be placed at
};

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

void sort_blocks(){

	auto& cluster_ctx = g_vpr_ctx.clustering();
	auto& place_ctx = g_vpr_ctx.placement();

	auto blocks = cluster_ctx.clb_nlist.blocks();
	auto pl_macros = place_ctx.pl_macros;

	vtr::vector<ClusterBlockId, t_block_scores> block_scores;

	t_block_scores score;

	for (auto blk_id : blocks) {
		block_scores.push_back(score);
	}

	std::vector<ClusterBlockId> sorted_blocks(blocks.begin(), blocks.end());

	//go through all blocks and store floorplan constraints and num equivalent tiles
	for (auto blk_id : blocks) {
		if (is_cluster_constrained(blk_id)) {
			block_scores[blk_id].num_floorplan_constraints = 1;
		}

		auto logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

		auto num_tiles = logical_block->equivalent_tiles.size();

		block_scores[blk_id].num_equivalent_tiles = num_tiles;
	}

	//go through placement macros and store size of macro for each block
	for (auto pl_macro : pl_macros) {
		int size = pl_macro.members.size();

		for (unsigned int i = 0; i < pl_macro.members.size(); i++) {
			block_scores[pl_macro.members[i].blk_index].macro_size = size;
		}
	}

    auto criteria = [block_scores](ClusterBlockId lhs, ClusterBlockId rhs) {
        int lhs_score = 100*block_scores[lhs].macro_size +
        		        10*block_scores[lhs].num_floorplan_constraints +
						10/(block_scores[lhs].num_equivalent_tiles);

        int rhs_score = 100*block_scores[rhs].macro_size +
                		10*block_scores[rhs].num_floorplan_constraints +
        		        10/(block_scores[rhs].num_equivalent_tiles);

        return lhs_score > rhs_score;
    };

    std::stable_sort(sorted_blocks.begin(), sorted_blocks.end(), criteria);

    print_sorted_blocks(sorted_blocks);
}

void print_sorted_blocks(std::vector<ClusterBlockId> sorted_blocks) {

	VTR_LOG("\nPrinting sorted blocks: \n");

	for (unsigned int i = 0; i < sorted_blocks.size(); i++) {
		VTR_LOG("Block_Id: %zu \n", sorted_blocks[i]);
	}

}
