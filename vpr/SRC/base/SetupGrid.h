#ifndef SETUPGRID_H
#define SETUPGRID_H

/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */
#include <vector>
#include "physical_types.h"

void alloc_and_load_grid(std::vector<t_grid_loc_def> grid_loc_defs, int *num_instances_type); /* [0..device_ctx.num_block_types-1] */
void freeGrid(void);

#endif

