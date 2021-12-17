/*
 * constraints_report.h
 *
 *  Created on: Dec. 7, 2021
 *      Author: khalid88
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
void check_constraints_filling();

#endif /* VPR_SRC_PACK_CONSTRAINTS_REPORT_H_ */
