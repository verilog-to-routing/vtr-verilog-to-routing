/*
 * place_constraints.h
 *
 *
 *  Created on: Mar. 1, 2021
 *      Author: khalid88
 */

#ifndef VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_
#define VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_

/*
 * Check that placement of each block is within the floorplan constraint region of that block (if the block has any constraints).
 * Returns the number of errors (inconsistencies in adherence to floorplanning constraints).
 */
int check_placement_floorplanning();

/*
 * Check if the block has floorplanning constraints
 */
bool is_cluster_constrained(ClusterBlockId blk_id);

/*
 * Check if the placement location would respect floorplan constraints of the block, if it has any
 */
bool cluster_floorplanning_check(ClusterBlockId blk_id, t_pl_loc loc);

/*
 * Check whether any member of the macro has floorplan constraints
 */
bool is_macro_constrained(t_pl_macro pl_macro);

/*
 * Returns region of valid locations for the head of the macro based on floorplan constraints
 */
PartitionRegion constrained_macro_locs(t_pl_macro pl_macro);

/*
 * Check if the cluster_constraints data structure has been initialized.
 * If it has not, initialize with an empty PartitionRegion for each cluster in the cluster netlist.
 * Used to load cluster_constraints if the pack stage of VPR is skipped, as the cluster_constraints
 * data structure is loaded during packing.
 */
void check_cluster_constraints_init(std::string constraints_file);

void load_cluster_constraints();

#endif /* VPR_SRC_PLACE_PLACE_CONSTRAINTS_H_ */
