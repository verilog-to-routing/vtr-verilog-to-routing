#ifndef SETUPGRID_H
#define SETUPGRID_H

/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block g_grid for VPR.

 */

void alloc_and_load_grid(int *num_instances_type); /* [0..g_num_block_types-1] */
void freeGrid(void);

#endif

