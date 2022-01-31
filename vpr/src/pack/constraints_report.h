/* Perform a check at the end of each packing iteration to see whether any
 * floorplan regions have been packed with too many clusters.
 */

#ifndef VPR_SRC_PACK_CONSTRAINTS_REPORT_H_
#define VPR_SRC_PACK_CONSTRAINTS_REPORT_H_

#include "globals.h"
#include "grid_tile_lookup.h"
#include "place_constraints.h"

/*
 * Check if any constraints regions are overfull,
 * i.e. the region contains more clusters of a certain type
 * than it has room for.
 * If the region is overfull, a message is printed saying which
 * region is overfull, and by how many clusters.
 */
bool floorplan_constraints_regions_overfull();

#endif /* VPR_SRC_PACK_CONSTRAINTS_REPORT_H_ */
