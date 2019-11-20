#include <assert.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "vpr_utils.h"

/* This module contains subroutines that are used in several unrelated parts *
 * of VPR.  They are VPR-specific utility routines.                          */


/******************** Subroutine definitions ********************************/

/* Points the grid structure back to the blocks list */
void
sync_grid_to_blocks(IN int num_blocks,
		    IN const struct s_block block_list[],
		    IN int nx,
		    IN int ny,
		    INOUT struct s_grid_tile **grid)
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

/* This function updates the nets list to point back to blocks list */
void
sync_nets_to_blocks(IN int num_blocks,
		    IN const struct s_block block_list[],
		    IN int num_nets,
		    INOUT struct s_net net_list[])
{
    int i, j, k, l;
    t_type_ptr cur_type;

    /* Count the number of sinks for each net */
    for(i = 0; i < num_nets; ++i)
	{
	    for(j = 0; j < num_blocks; ++j)
		{
		    cur_type = block_list[j].type;
		    for(k = 0; k < cur_type->num_pins; ++k)
			{
			    if(block_list[j].nets[k] == i)
				{
				    if(RECEIVER ==
				       cur_type->class_inf[cur_type->
							   pin_class[k]].type)
					{
					    ++net_list[i].num_sinks;
					}
				}
			}
		}
	}

    /* Alloc and load block lists of nets */
    for(i = 0; i < num_nets; ++i)
	{
	    /* The list should be num_sinks + 1 driver. Re-alloc if already allocated. */
	    if(net_list[i].node_block)
		{
		    free(net_list[i].node_block);
		}
	    net_list[i].node_block =
		(int *)my_malloc(sizeof(int) * (net_list[i].num_sinks + 1));
	    if(net_list[i].node_block_pin)
		{
		    free(net_list[i].node_block_pin);
		}
	    net_list[i].node_block_pin =
		(int *)my_malloc(sizeof(int) * (net_list[i].num_sinks + 1));

	    l = 1;		/* First sink goes at position 1, since 0 is for driver */

	    for(j = 0; j < num_blocks; ++j)
		{
		    cur_type = block_list[j].type;
		    for(k = 0; k < cur_type->num_pins; ++k)
			{
			    if(block_list[j].nets[k] == i)
				{
				    if(RECEIVER ==
				       cur_type->class_inf[cur_type->
							   pin_class[k]].type)
					{
					    net_list[i].node_block[l] = j;
					    net_list[i].node_block_pin[l] = k;
					    ++l;
					}
				    else
					{
					    assert(DRIVER ==
						   cur_type->
						   class_inf[cur_type->
							     pin_class[k]].
						   type);
					    net_list[i].node_block[0] = j;
					    net_list[i].node_block_pin[0] = k;
					}
				}
			}
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
get_class_range_for_block(IN int iblk,
			  OUT int *class_low,
			  OUT int *class_high)
{
/* Assumes that the placement has been done so each block has a set of pins allocated to it */
    t_type_ptr type;

    type = block[iblk].type;
    assert(type->num_class % type->capacity == 0);
    *class_low = block[iblk].z * (type->num_class / type->capacity);
    *class_high =
	(block[iblk].z + 1) * (type->num_class / type->capacity) - 1;
}


void
load_one_fb_fanout_count(t_subblock * subblock_inf,
			 int num_subblocks,
			 int *num_uses_of_fb_ipin,
			 int **num_uses_of_sblk_opin,
			 int iblk)
{

/* Loads the fanout counts for one block (iblk).  */
    t_type_ptr type = block[iblk].type;
    int isub, ipin, conn_pin, opin;
    int internal_sub, internal_pin;

	/* Reset ipin counts */
	for(ipin = 0; ipin < type->num_pins; ipin++)
	{
	    num_uses_of_fb_ipin[ipin] = 0;
	}

    /* First pass, reset fanout counts */
    for(isub = 0; isub < num_subblocks; isub++)
	{
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    num_uses_of_sblk_opin[isub][opin] = 0;
		}
	}

    for(isub = 0; isub < num_subblocks; isub++)
	{
	    /* Is the subblock output connected to a FB opin that actually goes *
	     * somewhere?  Necessary to check that the FB opin connects to      *
	     * something because some logic blocks result in netlists where      *
	     * subblock outputs being automatically hooked to a FB opin under   *
	     * all conditions.                                                   */
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    conn_pin = subblock_inf[isub].outputs[opin];
		    if(conn_pin != OPEN)
			{
			    if(block[iblk].nets[conn_pin] != OPEN)
				{	/* FB output is used */
				    num_uses_of_sblk_opin[isub][opin]++;
				}
			}
		}

	    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
		{
		    conn_pin = subblock_inf[isub].inputs[ipin];

		    if(conn_pin != OPEN)
			{
			    if(conn_pin < type->num_pins)
				{	/* Driven by FB ipin */
				    num_uses_of_fb_ipin[conn_pin]++;
				}
			    else
				{	/* Driven by sblk output in same fb */
				    internal_sub =
					(conn_pin -
					 type->num_pins) /
					type->max_subblock_outputs;
				    internal_pin =
					(conn_pin -
					 type->num_pins) %
					type->max_subblock_outputs;
				    num_uses_of_sblk_opin[internal_sub]
					[internal_pin]++;
				}
			}
		}		/* End for each sblk ipin */

	    conn_pin = subblock_inf[isub].clock;	/* Now do clock pin */

	    if(conn_pin != OPEN)
		{
		    if(conn_pin < type->num_pins)
			{	/* Driven by FB ipin */
			    num_uses_of_fb_ipin[conn_pin]++;
			}
		    else
			{	/* Driven by sblk output in same clb */
			    internal_sub =
				(conn_pin -
				 type->num_pins) / type->max_subblock_outputs;
			    internal_pin =
				(conn_pin -
				 type->num_pins) % type->max_subblock_outputs;
			    num_uses_of_sblk_opin[internal_sub]
				[internal_pin]++;
			}
		}

	}			/* End for each subblock */
}
