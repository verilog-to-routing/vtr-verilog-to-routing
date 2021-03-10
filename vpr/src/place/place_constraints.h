/*
 * place_constraints.h
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#ifndef VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_
#define VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_

//used to check that floorplan constraints were respected at the end of placement for all blocks
int check_placement_floorplanning();

//check if the block has floorplanning constraints
bool is_cluster_constrained(ClusterBlockId blk_id);

//check if the placement location would respect floorplan constraints of the block, if it has any
bool cluster_floorplanning_check(ClusterBlockId blk_id, t_pl_loc loc);

//sort the blocks for initial_placement
void sort_blocks();

void print_sorted_blocks(std::vector<ClusterBlockId> sorted_blocks);

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
