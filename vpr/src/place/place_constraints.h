/*
 * place_constraints.h
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#ifndef VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_
#define VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_

int check_placement_floorplanning();

bool is_cluster_constrained(ClusterBlockId blk_id);

bool cluster_floorplanning_check(ClusterBlockId blk_id, t_pl_loc loc);

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
