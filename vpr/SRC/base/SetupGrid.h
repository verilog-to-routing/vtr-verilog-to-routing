#ifndef SETUPGRAD_H
#define SETUPGRID_H

/*
 Author: Jason Luu
 Date: October 8, 2008

 Initializes and allocates the physical logic block grid for VPR.

 */

void alloc_and_load_grid(INOUTP int *num_instances_type); /* [0..num_types-1] */
void freeGrid(void);

#endif

