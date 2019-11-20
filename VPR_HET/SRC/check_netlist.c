#include <stdio.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "hash.h"
#include "vpr_utils.h"
#include "check_netlist.h"
#include "assert.h"


/**************** Subroutines local to this module **************************/

static int check_connections_to_global_fb_pins(int inet);

static int check_fb_conn(int iblk,
			 int num_conn);

static void check_for_multiple_sink_connections(void);

static int check_for_duplicate_block_names(void);

static int get_num_conn(int bnum);

static int check_subblocks(int iblk,
			   t_subblock_data * subblock_data_ptr,
			   int *num_uses_of_fb_pin,
			   int **num_uses_of_sblk_opin);

static int check_subblock_pin(int fb_pin,
			      int min_val,
			      int max_val,
			      enum e_pin_type pin_type,
			      int iblk,
			      int isubblk,
			      t_subblock * subblock_inf);

static int check_internal_subblock_connections(t_subblock_data
					       * subblock_data_ptr,
					       int iblk,
					       int **num_uses_of_sblk_opin);

static int check_fb_to_subblock_connections(int iblk,
					    t_subblock * subblock_inf,
					    int num_subblocks,
					    int *num_uses_of_fb_pin);



/*********************** Subroutine definitions *****************************/

void
check_netlist(t_subblock_data * subblock_data_ptr)
{

/* This routine checks that the netlist makes sense, and sets the num_ff    *
 * and num_const_gen members of subblock_data.                              */


    int i, error, num_conn, max_subblocks, max_pins, max_sub_opins;
    int *num_uses_of_fb_pin, **num_uses_of_sblk_opin,
	*num_subblocks_per_block;
	struct s_hash **net_hash_table, *h_ptr;
    
    net_hash_table = alloc_hash_table();

    num_subblocks_per_block = subblock_data_ptr->num_subblocks_per_block;
    subblock_data_ptr->num_ff = 0;
    subblock_data_ptr->num_const_gen = 0;

    error = 0;
    max_pins = 0;
    max_sub_opins = 0;

    /* Determine max number of subblocks for all types */
    max_subblocks = 0;
    for(i = 0; i < num_types; i++)
	{
	    max_subblocks =
		max(max_subblocks, type_descriptors[i].max_subblocks);
	    max_pins = max(max_pins, type_descriptors[i].num_pins);
	    max_sub_opins =
		max(max_sub_opins, type_descriptors[i].max_subblock_outputs);
	}

    /* one mem alloc to save space */
    num_uses_of_fb_pin = (int *)my_malloc(max_pins * sizeof(int));
    num_uses_of_sblk_opin =
	(int **)alloc_matrix(0, max_subblocks - 1, 0, max_sub_opins - 1,
			     sizeof(int));


/* Check that nets fanout and have a driver. */
    for(i = 0; i < num_nets; i++)
	{
	    h_ptr = insert_in_hash_table(net_hash_table, net[i].name, i);
		if(h_ptr->count != 1) {
			printf("Error:  net %s has multiple drivers.\n", net[i].name);
		    error++;
		}

	    if(net[i].num_sinks == 0)
		{
		    printf("Error:  net %s has no fanout.\n", net[i].name);
		    error++;
		}

	    error += check_connections_to_global_fb_pins(i);
	}
	free_hash_table(net_hash_table);

/* Check that each block makes sense. */
    for(i = 0; i < num_blocks; i++)
	{
	    num_conn = get_num_conn(i);
	    error += check_fb_conn(i, num_conn);
	    error += check_subblocks(i, subblock_data_ptr, num_uses_of_fb_pin,
				     num_uses_of_sblk_opin);
	}

    check_for_multiple_sink_connections();

    error += check_for_duplicate_block_names();

    free(num_uses_of_fb_pin);
    free_matrix(num_uses_of_sblk_opin, 0, max_subblocks - 1, 0, sizeof(int));

    if(error != 0)
	{
	    printf("Found %d fatal Errors in the input netlist.\n", error);
	    exit(1);
	}
}


static int
check_connections_to_global_fb_pins(int inet)
{

/* Checks that a global net (inet) connects only to global FB input pins  *
 * and that non-global nets never connects to a global FB pin.  Either    *
 * global or non-global nets are allowed to connect to pads.               */

    int ipin, num_pins, iblk, node_block_pin, error;

    num_pins = (net[inet].num_sinks + 1);
    error = 0;

/* For now global signals can be driven by an I/O pad or any FB output       *
 * although a FB output generates a warning.  I could make a global FB      *
 * output pin type to allow people to make architectures that didn't have     *
 * this warning.                                                              */

    for(ipin = 0; ipin < num_pins; ipin++)
	{
	    iblk = net[inet].node_block[ipin];

	    node_block_pin = net[inet].node_block_pin[ipin];

	    if(block[iblk].type->is_global_pin[node_block_pin] !=
	       net[inet].is_global)
		{

		    /* Allow a FB output pin to drive a global net (warning only). */

		    if(ipin == 0 && net[inet].is_global)
			{
			    printf
				("Warning in check_connections_to_global_fb_pins:\n"
				 "\tnet #%d (%s) is driven by FB output pin (#%d)\n"
				 "\ton block #%d (%s).\n", inet,
				 net[inet].name, node_block_pin, iblk,
				 block[iblk].name);
			}
		    else
			{	/* Otherwise -> Error */
			    printf
				("Error in check_connections_to_global_fb_pins:\n"
				 "\tpin %d on net #%d (%s) connects to FB input pin (#%d)\n"
				 "\ton block #%d (%s).\n", ipin, inet,
				 net[inet].name, node_block_pin, iblk,
				 block[iblk].name);
			    error++;
			}

		    if(net[inet].is_global)
			printf("\tNet is global, but FB pin is not.\n\n");
		    else
			printf("\tCLB pin is global, but net is not.\n\n");
		}
	}			/* End for all pins */

    return (error);
}


static int
check_fb_conn(int iblk,
	      int num_conn)
{

/* Checks that the connections into and out of the fb make sense.  */

    int iclass, ipin, error;
    t_type_ptr type;

    error = 0;
    type = block[iblk].type;

    if(type == IO_TYPE)
	{
	    if(num_conn != 1)
		{
		    printf(ERRTAG "io blk #%d (%s) has %d pins.\n",
			   iblk, block[iblk].name, num_conn);
		    error++;
		}
	}
    else if(num_conn < 2)
	{
	    printf("Warning:  logic block #%d (%s) has only %d pin.\n",
		   iblk, block[iblk].name, num_conn);

/* Allow the case where we have only one OUTPUT pin connected to continue. *
 * This is used sometimes as a constant generator for a primary output,    *
 * but I will still warn the user.  If the only pin connected is an input, *
 * abort.                                                                  */

	    if(num_conn == 1)
		{
		    for(ipin = 0; ipin < type->num_pins; ipin++)
			{
			    if(block[iblk].nets[ipin] != OPEN)
				{
				    iclass = type->pin_class[ipin];

				    if(type->class_inf[iclass].type != DRIVER)
					{
					    error++;
					}
				    else
					{
					    printf
						("\tPin is an output -- may be a constant generator.\n"
						 "\tNon-fatal, but check this.\n");
					}

				    break;
				}
			}
		}
	    else
		{
		    error++;
		}
	}

/* This case should already have been flagged as an error -- this is *
 * just a redundant double check.                                    */

    if(num_conn > type->num_pins)
	{
	    printf("Error:  logic block #%d with output %s has %d pins.\n",
		   iblk, block[iblk].name, num_conn);
	    error++;
	}

    return (error);
}


static int
check_for_duplicate_block_names(void)
{

/* Checks that all blocks have duplicate names.  Returns the number of     *
 * duplicate names.                                                        */

    int error, iblk;
    struct s_hash **block_hash_table, *h_ptr;
    struct s_hash_iterator hash_iterator;

    error = 0;
    block_hash_table = alloc_hash_table();

    for(iblk = 0; iblk < num_blocks; iblk++)
	h_ptr =
	    insert_in_hash_table(block_hash_table, block[iblk].name, iblk);

    hash_iterator = start_hash_table_iterator();
    h_ptr = get_next_hash(block_hash_table, &hash_iterator);

    while(h_ptr != NULL)
	{
	    if(h_ptr->count != 1)
		{
		    printf
			("Error:  %d blocks are named %s.  Block names must be unique."
			 "\n", h_ptr->count, h_ptr->name);
		    error++;
		}
	    h_ptr = get_next_hash(block_hash_table, &hash_iterator);
	}

    free_hash_table(block_hash_table);
    return (error);
}


static int
check_subblocks(int iblk,
		t_subblock_data * subblock_data_ptr,
		int *num_uses_of_fb_pin,
		int **num_uses_of_sblk_opin)
{

/* This routine checks the subblocks of iblk (which must be a FB).  It    *
 * returns the number of errors found.                                     */

    int error, isub, ipin, fb_pin, num_pins_fb;
    int num_subblocks, max_subblocks_per_block, subblock_lut_size,
	num_outputs;
    t_subblock *subblock_inf;

    num_pins_fb = block[iblk].type->num_pins;
    error = 0;
    subblock_inf = subblock_data_ptr->subblock_inf[iblk];
    num_subblocks = subblock_data_ptr->num_subblocks_per_block[iblk];
    max_subblocks_per_block = block[iblk].type->max_subblocks;
    subblock_lut_size = block[iblk].type->max_subblock_inputs;
    num_outputs = block[iblk].type->max_subblock_outputs;

    if(num_subblocks < 1 || num_subblocks > max_subblocks_per_block)
	{
	    printf("Error:  block #%d (%s) contains %d subblocks.\n",
		   iblk, block[iblk].name, num_subblocks);
	    error++;
	}

/* Check that all pins connect to the proper type of FB pin and are in the *
 * correct range.                                                           */

    for(isub = 0; isub < num_subblocks; isub++)
	{

	    for(ipin = 0; ipin < subblock_lut_size; ipin++)
		{		/* Input pins */
		    fb_pin = subblock_inf[isub].inputs[ipin];
		    error += check_subblock_pin(fb_pin, 0,
						(num_pins_fb +
						 (num_subblocks *
						  num_outputs) - 1), RECEIVER,
						iblk, isub, subblock_inf);

		}

	    for(ipin = 0; ipin < num_outputs; ++ipin)
		{		/* Output pins. */
		    fb_pin = subblock_inf[isub].outputs[ipin];
		    error += check_subblock_pin(fb_pin, 0, (num_pins_fb - 1),
						DRIVER, iblk, isub,
						subblock_inf);
		}

	    /* Subblock clock pin. */
	    fb_pin = subblock_inf[isub].clock;
	    error += check_subblock_pin(fb_pin, 0,
					(num_pins_fb +
					 (num_subblocks * num_outputs) - 1),
					RECEIVER, iblk, isub, subblock_inf);

	}			/* End subblock for loop. */

/* If pins out of range, return.  Could get seg faults otherwise. */

    if(error != 0)
	return (error);

    load_one_fb_fanout_count(subblock_inf,
			     num_subblocks, num_uses_of_fb_pin,
			     num_uses_of_sblk_opin, iblk);

    error += check_fb_to_subblock_connections(iblk, subblock_inf,
					      num_subblocks,
					      num_uses_of_fb_pin);

    error += check_internal_subblock_connections(subblock_data_ptr, iblk,
						 num_uses_of_sblk_opin);

    return (error);
}


static int
check_subblock_pin(int fb_pin,
		   int min_val,
		   int max_val,
		   enum e_pin_type pin_type,
		   int iblk,
		   int isubblk,
		   t_subblock * subblock_inf)
{

/* Checks that this subblock pin connects to a valid clb pin or BLE output *
 * within the clb.  Returns the number of errors found.                    */

    int iclass;
    t_type_ptr type;

    type = block[iblk].type;

    if(fb_pin != OPEN)
	{
	    if(fb_pin < min_val || fb_pin > max_val)
		{
		    printf("Error:  Block #%d (%s) subblock #%d (%s)"
			   "connects to nonexistent clb pin #%d.\n", iblk,
			   block[iblk].name, isubblk,
			   subblock_inf[isubblk].name, fb_pin);
		    return (1);
		}

	    if(fb_pin < type->num_pins)
		{		/* clb pin */
		    iclass = type->pin_class[fb_pin];
		    if(type->class_inf[iclass].type != pin_type)
			{
			    printf
				("Error:  Block #%d (%s) subblock #%d (%s) pin connects\n"
				 "\tto clb pin (#%d) of wrong input/output type.\n",
				 iblk, block[iblk].name, isubblk,
				 subblock_inf[isubblk].name, fb_pin);
			    return (1);
			}
		}
	}

    return (0);
}


static void
check_for_multiple_sink_connections(void)
{

/* The check is for nets that connect more than once to the same class of  *
 * pins on the same block.  For LUTs and cluster-based logic blocks that   *
 * doesn't make sense, although for some logic blocks it does.  The router *
 * can now handle this case, so maybe I should get rid of this check.      */

    int iblk, ipin, inet, iclass, class_pin;
    int *num_pins_connected;
    t_type_ptr type;

    num_pins_connected = my_calloc(num_nets, sizeof(int));

/* Have to do the check block by block, rather than net by net, for speed. *
 * This makes the code a bit messy.                                        */

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    type = block[iblk].type;
	    for(iclass = 0; iclass < type->num_class; iclass++)
		{

/* Two DRIVER pins can never connect to the same net (already checked by    *
 * the multiple driver check) so skip that check.                           */

		    if(type->class_inf[iclass].type == DRIVER)
			continue;

		    for(class_pin = 0;
			class_pin < type->class_inf[iclass].num_pins;
			class_pin++)
			{
			    ipin = type->class_inf[iclass].pinlist[class_pin];
			    inet = block[iblk].nets[ipin];

			    if(inet != OPEN)
				num_pins_connected[inet]++;
			}

		    for(class_pin = 0;
			class_pin < type->class_inf[iclass].num_pins;
			class_pin++)
			{

			    ipin = type->class_inf[iclass].pinlist[class_pin];
			    inet = block[iblk].nets[ipin];

			    if(inet != OPEN)
				{
				    if(num_pins_connected[inet] > 1)
					{
					    printf
						("Warning:  block %d (%s) connects %d pins of class "
						 "%d to net %d (%s).\n", iblk,
						 block[iblk].name,
						 num_pins_connected[inet],
						 iclass, inet,
						 net[inet].name);

					    printf
						("\tThis does not make sense for many logic blocks "
						 "(e.g. LUTs).\n"
						 "\tBe sure you really want this.\n");
					}

				    num_pins_connected[inet] = 0;
				}
			}
		}
	}

    free(num_pins_connected);
}


static int
get_num_conn(int bnum)
{

/* This routine returns the number of connections to a block. */

    int i, num_conn;
    t_type_ptr type;

    type = block[bnum].type;

    num_conn = 0;

    for(i = 0; i < type->num_pins; i++)
	{
	    if(block[bnum].nets[i] != OPEN)
		num_conn++;
	}

    return (num_conn);
}


static int
check_internal_subblock_connections(t_subblock_data * subblock_data_ptr,
				    int iblk,
				    int **num_uses_of_sblk_opin)
{

/* This routine checks that all subblocks in this block are either           *
 * completely empty (no pins hooked to anything) or have their output used   *
 * somewhere.  It also counts the number of constant generators (no input    *
 * sblks) and the number of FFs used in the circuit.                         */

    int num_const_gen, num_ff, isub, ipin, error, opin, i;
    boolean has_inputs, has_outputs;
    int subblock_lut_size;
    int num_subblocks;
    t_subblock *subblock_inf;
    t_type_ptr type;

    type = block[iblk].type;
    subblock_lut_size = type->max_subblock_inputs;
    num_subblocks = subblock_data_ptr->num_subblocks_per_block[iblk];
    subblock_inf = subblock_data_ptr->subblock_inf[iblk];

    num_const_gen = 0;
    num_ff = 0;
    error = 0;

    for(isub = 0; isub < num_subblocks; isub++)
	{

	    has_inputs = FALSE;
	    for(ipin = 0; ipin < subblock_lut_size; ipin++)
		{
		    if(subblock_inf[isub].inputs[ipin] != OPEN)
			{
			    has_inputs = TRUE;
			    break;
			}
		}
	    has_outputs = FALSE;
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    if(num_uses_of_sblk_opin[isub][opin] != 0)
			{
			    has_outputs = TRUE;
			    break;
			}
		}

	    if(type == IO_TYPE)
		{
		    /* Subblock inputs and outputs are faked for I/O's so this is an internal error */
		    assert((has_inputs && !has_outputs)
			   || (!has_inputs && has_outputs));
		}
	    else if(!has_outputs)
		{		/* Output unused */

		    if(has_inputs || subblock_inf[isub].clock != OPEN)
			{
			    printf
				("Error:  output of subblock #%d (%s) of block #%d (%s) is "
				 "never used.\n", isub,
				 subblock_inf[isub].name, iblk,
				 block[iblk].name);
			    error++;
			}
		}


	    /* End if output unused */
	    /* Check that subblocks whose output is used have inputs. */
	    else
		{		/* Subblock output is used. */

		    if(!has_inputs)
			{	/* No inputs are used */
			    if(subblock_inf[isub].clock == OPEN)
				{
				    printf
					("Warning:  block #%d (%s), subblock #%d (%s) is a "
					 "constant generator.\n\t(Has no inputs.)\n",
					 iblk, block[iblk].name, isub,
					 subblock_inf[isub].name);
				    num_const_gen++;
				}
			    else
				{
				    printf
					("Error:  block #%d (%s), subblock #%d (%s) is a CLOCKED "
					 "\n\tconstant generator.\n\t(Has no inputs but is clocked.)\n",
					 iblk, block[iblk].name, isub,
					 subblock_inf[isub].name);
				    num_const_gen++;
				    error++;
				}
			}

		    else
			{	/* Both input and output are used */
			    if(subblock_inf[isub].clock != OPEN)
				{
				    for(i = 0; i < type->max_subblock_outputs;
					i++)
					{
					    if(subblock_inf[isub].
					       outputs[i] != OPEN)
						{
						    num_ff++;
						}
					}
				}
			}
		}
	}			/* End for all subblocks */

    subblock_data_ptr->num_const_gen += num_const_gen;
    subblock_data_ptr->num_ff += num_ff;
    return (error);
}


static int
check_fb_to_subblock_connections(int iblk,
				 t_subblock * subblock_inf,
				 int num_subblocks,
				 int *num_uses_of_fb_pin)
{

/* This routine checks that each non-OPEN clb input pin connects to some    *
 * subblock inputs, and that each non-OPEN clb output pin is driven by      *
 * exactly one subblock output. It returns the number of errors found.      *
 * Note that num_uses_of_fb_pin is used to store the number of out-edges   *
 * (fanout) for a FB ipin, and the number of in-edges (fanin) for a FB    *
 * opin.                                                                    */

    int ipin, isub, fb_pin, error;
    int num_outputs;

    error = 0;
    num_outputs = block[iblk].type->max_subblock_outputs;

/* Count how many things connect to each clb output pin. */

    for(isub = 0; isub < num_subblocks; isub++)
	{
	    for(ipin = 0; ipin < num_outputs; ++ipin)
		{
		    fb_pin = subblock_inf[isub].outputs[ipin];

		    if(fb_pin != OPEN)	/* Guaranteed to connect to DRIVER pin only */
			num_uses_of_fb_pin[fb_pin]++;
		}
	}

    for(ipin = 0; ipin < block[iblk].type->num_pins; ipin++)
	{

	    if(block[iblk].nets[ipin] != OPEN)
		{
		    if(is_opin(ipin, block[iblk].type))
			{	/* FB output */
			    if(num_uses_of_fb_pin[ipin] == 0)
				{	/* No driver? */
				    printf
					("Error:  output pin %d on block #%d (%s) is not driven "
					 "by any subblock.\n", ipin, iblk,
					 block[iblk].name);
				    error++;
				}
			    else if(num_uses_of_fb_pin[ipin] > 1)
				{	/* Multiple drivers? */
				    printf
					("Error:  output pin %d on block #%d (%s) is driven "
					 "by %d subblocks.\n", ipin, iblk,
					 block[iblk].name,
					 num_uses_of_fb_pin[ipin]);
				    error++;
				}
			}

		    else
			{	/* FB ipin */
			    if(num_uses_of_fb_pin[ipin] <= 0)
				{	/* Fans out? */
				    printf
					("Error:  pin %d on block #%d (%s) does not fanout to any "
					 "subblocks.\n", ipin, iblk,
					 block[iblk].name);
				    error++;
				}
			}
		}
	    /* End if not OPEN */
	    else if(is_opin(ipin, block[iblk].type))
		{		/* OPEN FB output pin */
		    if(num_uses_of_fb_pin[ipin] > 1)
			{
			    printf
				("Error:  pin %d on block #%d (%s) is driven by %d "
				 "subblocks.\n", ipin, iblk, block[iblk].name,
				 num_uses_of_fb_pin[ipin]);
			    error++;
			}
		}
	}

    return (error);
}
