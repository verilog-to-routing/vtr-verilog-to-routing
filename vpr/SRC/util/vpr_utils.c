#include <assert.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */

/******************** Subroutine definitions ********************************/

/**
 * print tabs given number of tabs to file
 */
void
print_tabs(FILE * fpout, int num_tab) {
	int i;
	for(i = 0; i < num_tab; i++) {
		fprintf(fpout, "\t");
	}
}


/* Points the grid structure back to the blocks list */
void
sync_grid_to_blocks(INP int num_blocks,
		    INP const struct s_block block_list[],
		    INP int nx,
		    INP int ny,
		    INOUTP struct s_grid_tile **grid)
{
    int i, j, k;

    /* Reset usage and allocate blocks list if needed */
    for(j = 0; j <= (ny + 1); ++j)
	{
	    for(i = 0; i <= (nx + 1); ++i)
		{
		    grid[i][j].usage = 0;
		    if(grid[i][j].type)
			{
			    /* If already allocated, leave it since size doesn't change */
			    if(NULL == grid[i][j].blocks)
				{
				    grid[i][j].blocks =
					(int *)my_malloc(sizeof(int) *
							 grid[i][j].type->
							 capacity);

				    /* Set them as unconnected */
				    for(k = 0; k < grid[i][j].type->capacity;
					++k)
					{
					    grid[i][j].blocks[k] = OPEN;
					}
				}
			}
		}
	}

    /* Go through each block */
    for(i = 0; i < num_blocks; ++i)
	{
	    /* Check range of block coords */
	    if(block[i].x < 0 || block[i].x > (nx + 1) ||
	       block[i].y < 0
	       || (block[i].y + block[i].type->height - 1) > (ny + 1)
	       || block[i].z < 0 || block[i].z > (block[i].type->capacity))
		{
		    printf(ERRTAG
			   "Block %d is at invalid location (%d, %d, %d)\n",
			   i, block[i].x, block[i].y, block[i].z);
		    exit(1);
		}

	    /* Check types match */
	    if(block[i].type != grid[block[i].x][block[i].y].type)
		{
		    printf(ERRTAG "A block is in a grid location "
			   "(%d x %d) with a conflicting type.\n", block[i].x,
			   block[i].y);
		    exit(1);
		}

	    /* Check already in use */
	    if(OPEN != grid[block[i].x][block[i].y].blocks[block[i].z])
		{
		    printf(ERRTAG
			   "Location (%d, %d, %d) is used more than once\n",
			   block[i].x, block[i].y, block[i].z);
		    exit(1);
		}

	    if(grid[block[i].x][block[i].y].offset != 0)
		{
		    printf(ERRTAG
			   "Large block not aligned in placment for block %d at (%d, %d, %d)",
			   i, block[i].x, block[i].y, block[i].z);
		    exit(1);
		}

	    /* Set the block */
	    for(j = 0; j < block[i].type->height; j++)
		{
		    grid[block[i].x][block[i].y + j].blocks[block[i].z] = i;
		    grid[block[i].x][block[i].y + j].usage++;
		    assert(grid[block[i].x][block[i].y + j].offset == j);
		}
	}
}


boolean
is_opin(int ipin,
	t_type_ptr type)
{

/* Returns TRUE if this clb pin is an output, FALSE otherwise. */

    int iclass;

    iclass = type->pin_class[ipin];

    if(type->class_inf[iclass].type == DRIVER)
	return (TRUE);
    else
	return (FALSE);
}

void
get_class_range_for_block(INP int iblk,
			  OUTP int *class_low,
			  OUTP int *class_high)
{
/* Assumes that the placement has been done so each block has a set of pins allocated to it */
    t_type_ptr type;

    type = block[iblk].type;
    assert(type->num_class % type->capacity == 0);
    *class_low = block[iblk].z * (type->num_class / type->capacity);
    *class_high =
	(block[iblk].z + 1) * (type->num_class / type->capacity) - 1;
}
