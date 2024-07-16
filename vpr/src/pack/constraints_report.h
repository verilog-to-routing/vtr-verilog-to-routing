/* Perform a check at the end of each packing iteration to see whether any
 * floorplan regions have been packed with too many clusters.
 */

#ifndef VPR_SRC_PACK_CONSTRAINTS_REPORT_H_
#define VPR_SRC_PACK_CONSTRAINTS_REPORT_H_

#include "globals.h"
#include "grid_tile_lookup.h"
#include "place_constraints.h"

/**
 * @brief Check if any constraint partition regions are overfull,
 * i.e. the partition contains more clusters of a certain type
 * than it has room for. If the region is overfull, a message is
 * printed saying which partition is overfull, and by how many clusters.
 *
 * To reduce the complexity of this function, several assumptions are made.
 * 1) Regions of a partition do not overlap, meaning that the capacity of each
 * partition for accommodating blocks of a specific type can be calculated by
 * accumulating the capacity of its regions.
 * 2) Partitions do not overlap. This means that each physical tile is in at most
 * one partition.
 *
 * VPR can still work if these assumptions do not hold true, but for tight overlapping
 * partitions, the placement engine may fail to find a legal placement.
 *
 * @return True if there is at least one overfull partition.
 */
bool floorplan_constraints_regions_overfull();

#endif /* VPR_SRC_PACK_CONSTRAINTS_REPORT_H_ */
