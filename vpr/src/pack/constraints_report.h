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
 * Check if any constraints regions are overfull, give an error message if they are
 */
void check_constraints_filling();

#endif /* VPR_SRC_PACK_CONSTRAINTS_REPORT_H_ */
