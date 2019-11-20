#include <stdio.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "path_delay.h"
#include "path_delay2.h"
#include "net_delay.h"
#include "vpr_utils.h"
#include <assert.h>

/* TODO: Add option for registered inputs and outputs once works, currently, outputs only */

/****************** Timing graph Structure ************************************
 *                                                                            *
 * In the timing graph I create, input pads and constant generators have no   *
 * inputs; everything else has inputs.  Every input pad and output pad is     *
 * represented by two tnodes -- an input pin and an output pin.  For an input *
 * pad the input pin comes from off chip and has no fanin, while the output   *
 * pin drives outpads and/or CLBs.  For output pads, the input node is driven *
 * by a FB or input pad, and the output node goes off chip and has no        *
 * fanout (out-edges).  I need two nodes to respresent things like pads       *
 * because I mark all delay on tedges, not on tnodes.                         *
 *                                                                            *
 * Every used (not OPEN) FB pin becomes a timing node.  As well, every used  *
 * subblock pin within a FB also becomes a timing node.  Unused (OPEN) pins  *
 * don't create any timing nodes. If a subblock is used in combinational mode *
 * (i.e. its clock pin is open), I just hook the subblock input tnodes to the *
 * subblock output tnode.  If the subblock is used in sequential mode, I      *
 * create two extra tnodes.  One is just the subblock clock pin, which is     *
 * connected to the subblock output.  This means that FFs don't generate      *
 * their output until their clock arrives.  For global clocks coming from an  *
 * input pad, the delay of the clock is 0, so the FFs generate their outputs  *
 * at T = 0, as usual.  For locally-generated or gated clocks, however, the   *
 * clock will arrive later, and the FF output will be generated later.  This  *
 * lets me properly model things like ripple counters and gated clocks.  The  *
 * other extra node is the FF storage node (i.e. a sink), which connects to   *
 * the subblock inputs and has no fanout.                                     *
 *                                                                            *
 * One other subblock that needs special attention is a constant generator.   *
 * This has no used inputs, but its output is used.  I create an extra tnode, *
 * a dummy input, in addition to the output pin tnode.  The dummy tnode has   *
 * no fanin.  Since constant generators really generate their outputs at T =  *
 * -infinity, I set the delay from the input tnode to the output to a large-  *
 * magnitude negative number.  This guarantees every block that needs the     *
 * output of a constant generator sees it available very early.               *
 *                                                                            *
 * For this routine to work properly, subblocks whose outputs are unused must *
 * be completely empty -- all their input pins and their clock pin must be    *
 * OPEN.  Check_netlist checks the input netlist to guarantee this -- don't   *
 * disable that check.                                                        *
 *                                                                            *
 * NB:  The discussion below is only relevant for circuits with multiple      *
 * clocks.  For circuits with a single clock, everything I do is exactly      *
 * correct.                                                                   *
 *                                                                            *
 * A note about how I handle FFs:  By hooking the clock pin up to the FF      *
 * output, I properly model the time at which the FF generates its output.    *
 * I don't do a completely rigorous job of modelling required arrival time at *
 * the FF input, however.  I assume every FF and outpad needs its input at    *
 * T = 0, which is when the earliest clock arrives.  This can be conservative *
 * -- a fuller analysis would be to do a fast path analysis of the clock      *
 * feeding each FF and subtract its earliest arrival time from the delay of   *
 * the D signal to the FF input.  This is too much work, so I'm not doing it. *
 * Alternatively, when one has N clocks, it might be better to just do N      *
 * separate timing analyses, with only signals from FFs clocked on clock i    *
 * being propagated forward on analysis i, and only FFs clocked on i being    *
 * considered as sinks.  This gives all the critical paths within clock       *
 * domains, but ignores interactions.  Instead, I assume all the clocks are   *
 * more-or-less synchronized (they might be gated or locally-generated, but   *
 * they all have the same frequency) and explore all interactions.  Tough to  *
 * say what's the better way.  Since multiple clocks aren't important for my  *
 * work, it's not worth bothering about much.                                 *
 *                                                                            *
 ******************************************************************************/

#define T_CONSTANT_GENERATOR -1000	/* Essentially -ve infinity */

/***************** Types local to this module ***************************/

enum e_subblock_pin_type
{ SUB_INPUT = 0, SUB_OUTPUT, SUB_CLOCK, NUM_SUB_PIN_TYPES };

/***************** Variables local to this module ***************************/

/* Variables for "chunking" the tedge memory.  If the head pointer is NULL, *
 * no timing graph exists now.                                              */

static struct s_linked_vptr *tedge_ch_list_head = NULL;
static int tedge_ch_bytes_avail = 0;
static char *tedge_ch_next_avail = NULL;


/***************** Subroutines local to this module *************************/

static int alloc_and_load_pin_mappings(int ***block_pin_to_tnode_ptr,
				       int *****snode_block_pin_to_tnode_ptr,
				       t_subblock_data subblock_data,
				       int ***num_uses_of_sblk_opin);

static void free_pin_mappings(int **block_pin_to_tnode,
			      int ****snode_block_pin_to_tnode,
			      int *num_subblocks_per_block);

static void alloc_and_load_fanout_counts(int ***num_uses_of_fb_ipin_ptr,
					 int ****num_uses_of_sblk_opin_ptr,
					 t_subblock_data subblock_data);

static void free_fanout_counts(int **num_uses_of_fb_ipin,
			       int ***num_uses_of_sblk_opin);

static float **alloc_net_slack(void);

static void compute_net_slacks(float **net_slack);

static void alloc_and_load_tnodes_and_net_mapping(int **num_uses_of_fb_ipin,
						  int
						  ***num_uses_of_sblk_opin,
						  int **block_pin_to_tnode,
						  int
						  ****snode_block_pin_to_tnode,
						  t_subblock_data
						  subblock_data,
						  t_timing_inf timing_inf);

static void build_fb_tnodes(int iblk,
			    int *n_uses_of_fb_ipin,
			    int **block_pin_to_tnode,
			    int ***sub_pin_to_tnode,
			    int num_subs,
			    t_subblock * sub_inf,
			    float T_fb_ipin_to_sblk_ipin);

static void build_subblock_tnodes(int **n_uses_of_sblk_opin,
				  int *node_block_pin_to_tnode,
				  int ***sub_pin_to_tnode,
				  int *num_subblocks_per_block,
				  t_subblock ** subblock_inf,
				  t_timing_inf timing_inf,
				  int iblk);


static boolean is_global_clock(int iblk,
			       int sub,
			       int subpin,
			       int *num_subblocks_per_block,
			       t_subblock ** subblock_inf);

static void build_block_output_tnode(int inode,
				     int iblk,
				     int ipin,
				     int **block_pin_to_tnode);


/********************* Subroutine definitions *******************************/


float **
alloc_and_load_timing_graph(t_timing_inf timing_inf,
			    t_subblock_data subblock_data)
{

/* This routine builds the graph used for timing analysis.  Every fb or    *
 * subblock pin is a timing node (tnode).  The connectivity between pins is *
 * represented by timing edges (tedges).  All delay is marked on edges, not *
 * on nodes.  This routine returns an array that will store slack values:   *
 * net_slack[0..num_nets-1][1..num_pins-1].                                 */

/* The two arrays below are valid only for FBs, not pads.                  */
    int i;
    int **num_uses_of_fb_ipin;	/* [0..num_blocks-1][0..type->num_pins-1]       */
    int ***num_uses_of_sblk_opin;	/* [0..num_blocks-1][0..type->num_subblocks][0..type->max_subblock_outputs] */

/* Array for mapping from a pin on a block to a tnode index. For pads, only *
 * the first two pin locations are used (input to pad is first, output of   *
 * pad is second).  For fbs, all OPEN pins on the fb have their mapping   *
 * set to OPEN so I won't use it by mistake.                                */

    int **block_pin_to_tnode;	/* [0..num_blocks-1][0..num_pins-1]      */


/* Array for mapping from a pin on a subblock to a tnode index.  Unused     *
 * or nonexistent subblock pins have their mapping set to OPEN.             *
 * [0..num_blocks-1][0..num_subblocks_per_block-1][0..NUM_SUB_PIN_TYPES][0..total_subblock_pins-1]  */

    int ****snode_block_pin_to_tnode;

    int num_sinks;
    float **net_slack;		/* [0..num_nets-1][1..num_pins-1]. */

/************* End of variable declarations ********************************/

    if(tedge_ch_list_head != NULL)
	{
	    printf("Error in alloc_and_load_timing_graph:\n"
		   "\tAn old timing graph still exists.\n");
	    exit(1);
	}

/* If either of the checks below ever fail, change the definition of        *
 * tnode_descript to use ints instead of shorts for isubblk or ipin.        */

    for(i = 0; i < num_types; i++)
	{
	    if(type_descriptors[i].num_pins > MAX_SHORT)
		{
		    printf
			("Error in alloc_and_load_timing_graph: pins for type %s is %d."
			 "\tWill cause short overflow in tnode_descript.\n",
			 type_descriptors[i].name,
			 type_descriptors[i].num_pins);
		    exit(1);
		}

	    if(type_descriptors[i].max_subblocks > MAX_SHORT)
		{
		    printf
			("Error in alloc_and_load_timing_graph: max_subblocks_per_block"
			 "\tis %d -- will cause short overflow in tnode_descript.\n",
			 type_descriptors[i].max_subblocks);
		    exit(1);
		}
	}

    alloc_and_load_fanout_counts(&num_uses_of_fb_ipin,
				 &num_uses_of_sblk_opin, subblock_data);

    num_tnodes = alloc_and_load_pin_mappings(&block_pin_to_tnode,
					     &snode_block_pin_to_tnode,
					     subblock_data,
					     num_uses_of_sblk_opin);

    alloc_and_load_tnodes_and_net_mapping(num_uses_of_fb_ipin,
					  num_uses_of_sblk_opin,
					  block_pin_to_tnode,
					  snode_block_pin_to_tnode,
					  subblock_data, timing_inf);

    num_sinks = alloc_and_load_timing_graph_levels();

    check_timing_graph(subblock_data.num_const_gen, subblock_data.num_ff,
		       num_sinks);

    free_fanout_counts(num_uses_of_fb_ipin, num_uses_of_sblk_opin);
    free_pin_mappings(block_pin_to_tnode, snode_block_pin_to_tnode,
		      subblock_data.num_subblocks_per_block);

    net_slack = alloc_net_slack();
    return (net_slack);
}


static float **
alloc_net_slack(void)
{

/* Allocates the net_slack structure.  Chunk allocated to save space.      */

    float **net_slack;		/* [0..num_nets-1][1..num_pins-1]  */
    float *tmp_ptr;
    int inet;

    net_slack = (float **)my_malloc(num_nets * sizeof(float *));

    for(inet = 0; inet < num_nets; inet++)
	{
	    tmp_ptr =
		(float *)my_chunk_malloc(((net[inet].num_sinks + 1) - 1) *
					 sizeof(float), &tedge_ch_list_head,
					 &tedge_ch_bytes_avail,
					 &tedge_ch_next_avail);
	    net_slack[inet] = tmp_ptr - 1;	/* [1..num_pins-1] */
	}

    return (net_slack);
}


static int
alloc_and_load_pin_mappings(int ***block_pin_to_tnode_ptr,
			    int *****snode_block_pin_to_tnode_ptr,
			    t_subblock_data subblock_data,
			    int ***num_uses_of_sblk_opin)
{

/* Allocates and loads the block_pin_to_tnode and snode_block_pin_to_tnode         *
 * structures, and computes num_tnodes.                                     */

    int iblk, isub, ipin, num_subblocks, opin, clk_pin;
    int curr_tnode;
    int ****snode_block_pin_to_tnode, **block_pin_to_tnode;
    int *num_subblocks_per_block;
    t_type_ptr type;
    t_subblock **subblock_inf;
    boolean has_inputs;

    num_subblocks_per_block = subblock_data.num_subblocks_per_block;
    subblock_inf = subblock_data.subblock_inf;


    block_pin_to_tnode = (int **)my_malloc(num_blocks * sizeof(int *));

    snode_block_pin_to_tnode =
	(int ****)my_malloc(num_blocks * sizeof(int ***));

    curr_tnode = 0;

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    type = block[iblk].type;
	    block_pin_to_tnode[iblk] =
		(int *)my_malloc(type->num_pins * sizeof(int));
	    /* First do the block mapping */
	    for(ipin = 0; ipin < block[iblk].type->num_pins; ipin++)
		{
		    if(block[iblk].nets[ipin] == OPEN)
			{
			    block_pin_to_tnode[iblk][ipin] = OPEN;
			}
		    else
			{
			    block_pin_to_tnode[iblk][ipin] = curr_tnode;
			    curr_tnode++;
			}
		}

	    /* Now do the subblock mapping. */

	    num_subblocks = num_subblocks_per_block[iblk];
	    snode_block_pin_to_tnode[iblk] = (int ***)alloc_matrix(0, num_subblocks - 1, 0, NUM_SUB_PIN_TYPES - 1, sizeof(int *));	/* [0..max_subblocks_for_type - 1][0..SUB_NUM_PIN_TYPES - 1] */

	    for(isub = 0; isub < num_subblocks; isub++)
		{
		    /* Allocate space for each type of subblock pin */
		    snode_block_pin_to_tnode[iblk][isub][SUB_INPUT] =
			(int *)my_malloc(type->max_subblock_inputs *
					 sizeof(int));
		    snode_block_pin_to_tnode[iblk][isub][SUB_OUTPUT] =
			(int *)my_malloc(type->max_subblock_outputs *
					 sizeof(int));
		    snode_block_pin_to_tnode[iblk][isub][SUB_CLOCK] =
			(int *)my_malloc(sizeof(int));

		    /* Pin ordering:  inputs, outputs, clock.   */

		    has_inputs = FALSE;
		    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
			{
			    if(subblock_inf[iblk][isub].inputs[ipin] != OPEN)
				{
				    has_inputs = TRUE;
				    snode_block_pin_to_tnode[iblk][isub]
					[SUB_INPUT][ipin] = curr_tnode;
				    curr_tnode++;
				    if(type == IO_TYPE)
					curr_tnode++;	/* Output pad needs additional dummy sink node */
				}
			    else
				{
				    snode_block_pin_to_tnode[iblk][isub]
					[SUB_INPUT][ipin] = OPEN;
				}
			}

		    /* subblock output  */

		    /* If the subblock opin is unused the subblock is empty and we    *
		     * shoudn't count it.                                             */
		    for(opin = 0; opin < type->max_subblock_outputs; opin++)
			{

			    if(num_uses_of_sblk_opin[iblk][isub][opin] != 0)
				{
				    snode_block_pin_to_tnode[iblk][isub]
					[SUB_OUTPUT][opin] = curr_tnode;

				    if(type == IO_TYPE)
					curr_tnode += 2;	/* Input pad needs a dummy source node */
				    else if(has_inputs)	/* Regular sblk */
					curr_tnode++;
				    else	/* Constant generator. Make room for dummy input */
					curr_tnode += 2;
				}
			    else
				{
				    snode_block_pin_to_tnode[iblk][isub]
					[SUB_OUTPUT][opin] = OPEN;
				}
			}

		    clk_pin = 0;

		    if(subblock_inf[iblk][isub].clock != OPEN)
			{

			    /* If this is a sequential block, we have two more pins per used output: #1: the
			     * clock input (connects to the subblock output node) and #2: the
			     * sequential sink (which the subblock LUT inputs will connect to). */
			    snode_block_pin_to_tnode[iblk][isub][SUB_CLOCK]
				[clk_pin] = curr_tnode;

			    for(opin = 0; opin < type->max_subblock_outputs;
				opin++)
				{
				    if(subblock_inf[iblk][isub].
				       outputs[opin] != OPEN)
					curr_tnode += 2;
				}
			}
		    else
			{
			    snode_block_pin_to_tnode[iblk][isub][SUB_CLOCK]
				[clk_pin] = OPEN;
			}
		}

	}			/* End for all blocks */

    *snode_block_pin_to_tnode_ptr = snode_block_pin_to_tnode;
    *block_pin_to_tnode_ptr = block_pin_to_tnode;
    return (curr_tnode);
}


static void
free_pin_mappings(int **block_pin_to_tnode,
		  int ****snode_block_pin_to_tnode,
		  int *num_subblocks_per_block)
{

/* Frees the arrays that map from pins to tnode coordinates. */

    int isub, iblk, isubtype, num_subblocks;

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    num_subblocks = num_subblocks_per_block[iblk];
	    for(isub = 0; isub < num_subblocks; isub++)
		{
		    for(isubtype = 0; isubtype < NUM_SUB_PIN_TYPES;
			isubtype++)
			{
			    free(snode_block_pin_to_tnode[iblk][isub]
				 [isubtype]);
			}
		}
	    free_matrix(snode_block_pin_to_tnode[iblk], 0,
			num_subblocks_per_block[iblk] - 1, 0, sizeof(int *));
	    free(block_pin_to_tnode[iblk]);
	}
    free(block_pin_to_tnode);
    free(snode_block_pin_to_tnode);
}


static void
alloc_and_load_fanout_counts(int ***num_uses_of_fb_ipin_ptr,
			     int ****num_uses_of_sblk_opin_ptr,
			     t_subblock_data subblock_data)
{

/* Allocates and loads two arrays that say how many points each fb input    *
 * pin and each subblock output fan out to.                                 */

    int iblk;
    int **num_uses_of_fb_ipin, ***num_uses_of_sblk_opin;
    int *num_subblocks_per_block;
    t_subblock **subblock_inf;

    num_subblocks_per_block = subblock_data.num_subblocks_per_block;
    subblock_inf = subblock_data.subblock_inf;

    num_uses_of_fb_ipin = (int **)my_malloc(num_blocks * sizeof(int *));

    num_uses_of_sblk_opin = (int ***)my_malloc(num_blocks * sizeof(int **));

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    num_uses_of_fb_ipin[iblk] =
		(int *)my_calloc(block[iblk].type->num_pins, sizeof(int));
	    num_uses_of_sblk_opin[iblk] =
		(int **)alloc_matrix(0, block[iblk].type->max_subblocks - 1,
				     0,
				     block[iblk].type->max_subblock_outputs -
				     1, sizeof(int));

	    load_one_fb_fanout_count(subblock_inf[iblk],
				     num_subblocks_per_block[iblk],
				     num_uses_of_fb_ipin[iblk],
				     num_uses_of_sblk_opin[iblk], iblk);

	}			/* End for all blocks */

    *num_uses_of_fb_ipin_ptr = num_uses_of_fb_ipin;
    *num_uses_of_sblk_opin_ptr = num_uses_of_sblk_opin;
}


static void
free_fanout_counts(int **num_uses_of_fb_ipin,
		   int ***num_uses_of_sblk_opin)
{

/* Frees the fanout count arrays. */

    int iblk;
    t_type_ptr type;

    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    type = block[iblk].type;
	    free(num_uses_of_fb_ipin[iblk]);
	    free_matrix(num_uses_of_sblk_opin[iblk], 0,
			type->max_subblocks - 1, 0, sizeof(int));
	}

    free(num_uses_of_fb_ipin);
    free(num_uses_of_sblk_opin);
}


static void
alloc_and_load_tnodes_and_net_mapping(int **num_uses_of_fb_ipin,
				      int ***num_uses_of_sblk_opin,
				      int **block_pin_to_tnode,
				      int ****snode_block_pin_to_tnode,
				      t_subblock_data subblock_data,
				      t_timing_inf timing_inf)
{

    int iblk;
    int *num_subblocks_per_block;
    t_subblock **subblock_inf;


    tnode = (t_tnode *) my_malloc(num_tnodes * sizeof(t_tnode));
    tnode_descript = (t_tnode_descript *) my_malloc(num_tnodes *
						    sizeof(t_tnode_descript));

    net_to_driver_tnode = (int *)my_malloc(num_nets * sizeof(int));

    subblock_inf = subblock_data.subblock_inf;
    num_subblocks_per_block = subblock_data.num_subblocks_per_block;


    for(iblk = 0; iblk < num_blocks; iblk++)
	{
	    build_fb_tnodes(iblk, num_uses_of_fb_ipin[iblk],
			    block_pin_to_tnode,
			    snode_block_pin_to_tnode[iblk],
			    num_subblocks_per_block[iblk], subblock_inf[iblk],
			    block[iblk].type->type_timing_inf.
			    T_fb_ipin_to_sblk_ipin);

	    build_subblock_tnodes(num_uses_of_sblk_opin[iblk],
				  block_pin_to_tnode[iblk],
				  snode_block_pin_to_tnode[iblk],
				  num_subblocks_per_block, subblock_inf,
				  timing_inf, iblk);
	}
}


static void
build_fb_tnodes(int iblk,
		int *n_uses_of_fb_ipin,
		int **block_pin_to_tnode,
		int ***sub_pin_to_tnode,
		int num_subs,
		t_subblock * sub_inf,
		float T_fb_ipin_to_sblk_ipin)
{

/* This routine builds the tnodes corresponding to the fb pins of this     
 * block, and properly hooks them up to the rest of the graph. Note that    
 * only the snode_block_pin_to_tnode, etc. element for this block is passed in.    
 * Assumes that pins are ordered as [inputs][outputs][clk]
 */

    int isub, ipin, iedge, from_pin, opin;
    int inode, to_node, num_edges;
    t_tedge *tedge;
    int clk_pin;
    t_type_ptr type;
    int *next_ipin_edge;

    type = block[iblk].type;
    next_ipin_edge = (int *)my_malloc(type->num_pins * sizeof(int));
    clk_pin = 0;

/* Start by allocating the edge arrays, and for opins, loading them.    */

    for(ipin = 0; ipin < block[iblk].type->num_pins; ipin++)
	{
	    inode = block_pin_to_tnode[iblk][ipin];

	    if(inode != OPEN)
		{		/* Pin is used -> put in graph */
		    if(is_opin(ipin, block[iblk].type))
			{
			    build_block_output_tnode(inode, iblk, ipin,
						     block_pin_to_tnode);
			    tnode_descript[inode].type = FB_OPIN;
			}
		    else
			{	/* FB ipin */
			    next_ipin_edge[ipin] = 0;	/* Reset */
			    num_edges = n_uses_of_fb_ipin[ipin];

			    /* if clock pin, timing edges go to each subblock output used */
			    for(isub = 0; isub < num_subs; isub++)
				{
				    if(sub_inf[isub].clock == ipin)
					{
					    for(opin = 0;
						opin <
						type->max_subblock_outputs;
						opin++)
						{
						    if(sub_inf[isub].
						       outputs[opin] != OPEN)
							{
							    num_edges++;
							}
						}
					    num_edges--;	/* Remove clock_pin count, replaced by outputs */
					}
				}

			    tnode[inode].num_edges = num_edges;

			    tnode[inode].out_edges =
				(t_tedge *) my_chunk_malloc(num_edges *
							    sizeof(t_tedge),
							    &tedge_ch_list_head,
							    &tedge_ch_bytes_avail,
							    &tedge_ch_next_avail);

			    tnode_descript[inode].type = FB_IPIN;
			}

		    tnode_descript[inode].ipin = ipin;
		    tnode_descript[inode].isubblk = OPEN;
		    tnode_descript[inode].iblk = iblk;
		}
	}

/* Now load the edge arrays for the FB input pins. Do this by looking at   *
 * where the subblock input and clock pins are driven from.                 */

    for(isub = 0; isub < num_subs; isub++)
	{
	    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
		{
		    from_pin = sub_inf[isub].inputs[ipin];

		    /* Not OPEN and comes from fb ipin? */

		    if(from_pin != OPEN
		       && from_pin < block[iblk].type->num_pins)
			{
			    inode = block_pin_to_tnode[iblk][from_pin];
			    assert(inode != OPEN);
			    to_node = sub_pin_to_tnode[isub][SUB_INPUT][ipin];
			    tedge = tnode[inode].out_edges;
			    iedge = next_ipin_edge[from_pin]++;
			    tedge[iedge].to_node = to_node;
			    tedge[iedge].Tdel = T_fb_ipin_to_sblk_ipin;
			}
		}

	    from_pin = sub_inf[isub].clock;

	    if(from_pin != OPEN && from_pin < block[iblk].type->num_pins)
		{
		    inode = block_pin_to_tnode[iblk][from_pin];
		    to_node = sub_pin_to_tnode[isub][SUB_CLOCK][clk_pin];	/* Feeds seq. output */
		    /* connect to each output flip flop */
		    for(opin = 0; opin < type->max_subblock_outputs; opin++)
			{
			    if(sub_inf[isub].outputs[opin] != OPEN)
				{
				    tedge = tnode[inode].out_edges;
				    iedge = next_ipin_edge[from_pin]++;
				    tedge[iedge].to_node = to_node;

				    /* For the earliest possible clock I want this delay to be zero, so it       *
				     * * arrives at flip flops at T = 0.  For later clocks or locally generated    *
				     * * clocks that may accumulate delay (like the clocks in a ripple counter),   *
				     * * I might want to make this delay nonzero.  Not worth bothering about now.  */

				    tedge[iedge].Tdel = 0.;
				    to_node += 2;
				}
			}
		}
	}
    free(next_ipin_edge);
}


static void
build_block_output_tnode(int inode,
			 int iblk,
			 int ipin,
			 int **block_pin_to_tnode)
{

/* Sets the number of edges and the edge array for an output pin from a      *
 * block.  This pin must be hooked to something -- i.e. not OPEN.            */

    int iedge, to_blk, to_pin, to_node, num_edges, inet;
    t_tedge *tedge;

    inet = block[iblk].nets[ipin];	/* Won't be OPEN, as inode exists */
    assert(inet != OPEN);	/* Sanity check. */

    net_to_driver_tnode[inet] = inode;

    num_edges = (net[inet].num_sinks + 1) - 1;
    tnode[inode].num_edges = num_edges;

    tnode[inode].out_edges = (t_tedge *) my_chunk_malloc(num_edges *
							 sizeof(t_tedge),
							 &tedge_ch_list_head,
							 &tedge_ch_bytes_avail,
							 &tedge_ch_next_avail);

    tedge = tnode[inode].out_edges;

    for(iedge = 0; iedge < (net[inet].num_sinks + 1) - 1; iedge++)
	{
	    to_blk = net[inet].node_block[iedge + 1];

	    to_pin = net[inet].node_block_pin[iedge + 1];

	    to_node = block_pin_to_tnode[to_blk][to_pin];
	    tedge[iedge].to_node = to_node;
	    /* Set delay from net delays with a later call */
	}
}


static void
build_subblock_tnodes(int **n_uses_of_sblk_opin,
		      int *node_block_pin_to_tnode,
		      int ***sub_pin_to_tnode,
		      int *num_subblocks_per_block,
		      t_subblock ** subblock_inf,
		      t_timing_inf timing_inf,
		      int iblk)
{

/* This routine builds the tnodes of the subblock pins within one FB. Note *
 * that only the block_pin_to_tnode, etc. data for *this* block are passed  *
 * in.                                                                      */

    int isub, ipin, inode, to_node, from_pin, to_pin, opin, from_opin,
	clk_pin, used_opin_count;
    int num_subs, from_sub;
    t_subblock *sub_inf;
    int iedge, num_edges;
    float sink_delay;
    t_tedge *tedge;
    boolean has_inputs, has_outputs;
    int **next_sblk_opin_edge;	/* [0..max_subblocks-1][0..max_subblock_outputs-1] */
    int *num_opin_used_in_sblk;	/* [0..max_subblocks-1] */
    t_type_ptr type = block[iblk].type;

    sub_inf = subblock_inf[iblk];
    num_subs = num_subblocks_per_block[iblk];

    next_sblk_opin_edge =
	(int **)alloc_matrix(0, type->max_subblocks - 1, 0,
			     type->max_subblock_outputs - 1, sizeof(int));
    num_opin_used_in_sblk =
	(int *)my_malloc(type->max_subblocks * sizeof(int));

    clk_pin = 0;

/* Allocate memory for output pins first. */
    for(isub = 0; isub < num_subs; isub++)
	{
	    num_opin_used_in_sblk[isub] = 0;
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    inode = sub_pin_to_tnode[isub][SUB_OUTPUT][opin];

		    if(inode != OPEN)
			{	/* Output is used -> timing node exists. */
			    num_opin_used_in_sblk[isub]++;
			    next_sblk_opin_edge[isub][opin] = 0;	/* Reset */
			    num_edges = n_uses_of_sblk_opin[isub][opin];
			    tnode[inode].num_edges = num_edges;

			    tnode[inode].out_edges =
				(t_tedge *) my_chunk_malloc(num_edges *
							    sizeof(t_tedge),
							    &tedge_ch_list_head,
							    &tedge_ch_bytes_avail,
							    &tedge_ch_next_avail);

			    if(IO_TYPE == type)
				{
				    tnode_descript[inode].type = INPAD_OPIN;
				    tnode[inode + 1].num_edges = 1;
				    tnode[inode + 1].out_edges = (t_tedge *)
					my_chunk_malloc(sizeof(t_tedge),
							&tedge_ch_list_head,
							&tedge_ch_bytes_avail,
							&tedge_ch_next_avail);
				    tedge = tnode[inode + 1].out_edges;
				    tedge[0].to_node = inode;

				    /* For input pads, use sequential output for source timing delay (treat as register) */
				    if(is_global_clock
				       (iblk, isub, opin,
					num_subblocks_per_block,
					subblock_inf))
					tedge[0].Tdel = 0.;
				    else
					tedge[0].Tdel =
					    type->type_timing_inf.
					    T_subblock[isub].T_seq_out[opin];

				    tnode_descript[inode + 1].type =
					INPAD_SOURCE;
				    tnode_descript[inode + 1].ipin = OPEN;
				    tnode_descript[inode + 1].isubblk = isub;
				    tnode_descript[inode + 1].iblk = iblk;
				}
			    else
				{
				    tnode_descript[inode].type = SUBBLK_OPIN;
				}

			    tnode_descript[inode].ipin = opin;
			    tnode_descript[inode].isubblk = isub;
			    tnode_descript[inode].iblk = iblk;
			}
		}
	}

    /* First pass, load the subblock input pins to output pins without connecting edges to output pins.
     * If the subblock is used in sequential mode (i.e. is clocked), the two clock pin nodes.  
     * Connect edges to output pins and take care of constant generators in second pass
     */
    for(isub = 0; isub < num_subs; isub++)
	{
	    has_outputs = FALSE;

	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    if(sub_pin_to_tnode[isub][SUB_OUTPUT][opin] != OPEN)
			{
			    has_outputs = TRUE;
			}
		}

	    if(!has_outputs && type != IO_TYPE)
		{		/* Empty, so skip */
		    continue;
		}

	    if(sub_inf[isub].clock != OPEN)
		{		/* Sequential mode */
		    inode = sub_pin_to_tnode[isub][SUB_CLOCK][clk_pin];
		    /* Each output of a subblock has a flip-flop sink and source */
		    for(opin = 0; opin < type->max_subblock_outputs; opin++)
			{
			    if(sub_inf[isub].outputs[opin] != OPEN)
				{
				    /* First node is the clock input pin; it feeds the sequential output */
				    tnode[inode].num_edges = 1;
				    tnode[inode].out_edges = (t_tedge *)
					my_chunk_malloc(sizeof(t_tedge),
							&tedge_ch_list_head,
							&tedge_ch_bytes_avail,
							&tedge_ch_next_avail);

				    tnode_descript[inode].type = FF_SOURCE;
				    tnode_descript[inode].ipin = OPEN;
				    tnode_descript[inode].isubblk = isub;
				    tnode_descript[inode].iblk = iblk;

				    /* Now create the "sequential sink" -- i.e. the FF input node. */

				    inode++;
				    tnode[inode].num_edges = 0;
				    tnode[inode].out_edges = NULL;

				    tnode_descript[inode].type = FF_SINK;
				    tnode_descript[inode].ipin = OPEN;
				    tnode_descript[inode].isubblk = isub;
				    tnode_descript[inode].iblk = iblk;
				    inode++;
				}
			}
		}

	    /* Build and hook up subblock inputs. */

	    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
		{
		    inode = sub_pin_to_tnode[isub][SUB_INPUT][ipin];

		    if(inode != OPEN)
			{	/* tnode exists -> pin is used */
			    if(type == IO_TYPE)
				{	/* Output pad */
				    tnode[inode].num_edges = 1;
				    opin = 0;
				    tnode[inode].out_edges = (t_tedge *)
					my_chunk_malloc(sizeof(t_tedge),
							&tedge_ch_list_head,
							&tedge_ch_bytes_avail,
							&tedge_ch_next_avail);
				    tnode_descript[inode].type = OUTPAD_IPIN;
				    tnode[inode + 1].num_edges = 0;
				    tnode[inode + 1].out_edges = NULL;
				    tedge = tnode[inode].out_edges;
				    tedge[0].to_node = inode + 1;

				    /* For output pads, use subblock combinational time 
				     * and sequential in for timing (treat as register) */
				    tedge[0].Tdel =
					type->type_timing_inf.
					T_subblock[isub].T_comb[ipin][opin] +
					type->type_timing_inf.
					T_subblock[isub].T_seq_in[opin];

				    tnode_descript[inode + 1].type =
					OUTPAD_SINK;
				    tnode_descript[inode + 1].ipin = OPEN;
				    tnode_descript[inode + 1].isubblk = isub;
				    tnode_descript[inode + 1].iblk = iblk;
				}
			    else
				{
				    tnode[inode].num_edges =
					num_opin_used_in_sblk[isub];
				    tnode[inode].out_edges =
					(t_tedge *)
					my_chunk_malloc(num_opin_used_in_sblk
							[isub] *
							sizeof(t_tedge),
							&tedge_ch_list_head,
							&tedge_ch_bytes_avail,
							&tedge_ch_next_avail);
				    tnode[inode].num_edges =
					num_opin_used_in_sblk[isub];
				    tnode_descript[inode].type = SUBBLK_IPIN;
				}

			    tnode_descript[inode].ipin = ipin;
			    tnode_descript[inode].isubblk = isub;
			    tnode_descript[inode].iblk = iblk;
			}
		}
	}			/* End for each subblock */

    /* Second pass load the input pins to output pins array */
    /* Load the output pins edge arrays. */
    for(isub = 0; isub < num_subs; isub++)
	{
	    used_opin_count = 0;
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{
		    if(sub_pin_to_tnode[isub][SUB_OUTPUT][opin] == OPEN)
			{	/* Empty, so skip */
			    continue;
			}
		    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
			{	/* sblk opin to sblk ipin */

			    from_pin = sub_inf[isub].inputs[ipin];

			    /* Not OPEN and comes from local subblock output? */

			    if(from_pin >= type->num_pins)
				{
				    /* Convention for numbering netlist pins.
				     * Internal connections are numbered subblock_index + subblock_output_index + num_pins
				     */
				    from_sub =
					(from_pin -
					 type->num_pins) /
					type->max_subblock_outputs;
				    from_opin =
					(from_pin -
					 type->num_pins) %
					type->max_subblock_outputs;
				    inode =
					sub_pin_to_tnode[from_sub][SUB_OUTPUT]
					[from_opin];
				    to_node =
					sub_pin_to_tnode[isub][SUB_INPUT]
					[ipin];
				    tedge = tnode[inode].out_edges;
				    iedge = next_sblk_opin_edge[from_sub]
					[from_opin]++;
				    tedge[iedge].to_node = to_node;
				    tedge[iedge].Tdel =
					type->type_timing_inf.
					T_sblk_opin_to_sblk_ipin;
				}
			}
		    from_pin = sub_inf[isub].clock;	/* sblk opin to sblk clock */

		    /* Not OPEN and comes from local subblock output? */

		    if(from_pin >= type->num_pins)
			{
			    from_sub =
				(from_pin -
				 type->num_pins) / type->max_subblock_outputs;
			    from_opin =
				(from_pin -
				 type->num_pins) % type->max_subblock_outputs;
			    inode = sub_pin_to_tnode[from_sub][SUB_OUTPUT]
				[from_opin];
			    /* Feeds seq. output, one ff per output pin */
			    to_node =
				sub_pin_to_tnode[isub][SUB_CLOCK][clk_pin] +
				2 * used_opin_count;
			    tedge = tnode[inode].out_edges;
			    iedge =
				next_sblk_opin_edge[from_sub][from_opin]++;
			    tedge[iedge].to_node = to_node;

			    /* NB: Could make sblk opin to clk delay parameter; not worth it right now. */
			    tedge[iedge].Tdel =
				type->type_timing_inf.
				T_sblk_opin_to_sblk_ipin;
			}

		    to_pin = sub_inf[isub].outputs[opin];
		    if(to_pin != OPEN)
			{	/* sblk opin goes to fb opin? */

			    /* Check that FB pin connects to something ->     *
			     * not just a mandatory BLE to FB opin connection */

			    if(block[iblk].nets[to_pin] != OPEN)
				{
				    to_node = node_block_pin_to_tnode[to_pin];
				    inode = sub_pin_to_tnode[isub][SUB_OUTPUT]
					[opin];
				    tedge = tnode[inode].out_edges;
				    iedge = next_sblk_opin_edge[isub][opin]++;
				    tedge[iedge].to_node = to_node;
				    tedge[iedge].Tdel =
					type->type_timing_inf.
					T_sblk_opin_to_fb_opin;
				}
			}
		    used_opin_count++;
		}
	}

    /* Now load the subblock input pins and, if the subblock is used in        *
     * sequential mode (i.e. is clocked), the two clock pin nodes for the output pin.              */
    for(isub = 0; isub < num_subs; isub++)
	{
	    used_opin_count = 0;
	    for(opin = 0; opin < type->max_subblock_outputs; opin++)
		{

		    if(sub_pin_to_tnode[isub][SUB_OUTPUT][opin] == OPEN)	/* Empty, so skip */
			continue;
		    /* Begin loading */
		    if(sub_inf[isub].clock == OPEN)
			{	/* Combinational mode */
			    to_node =
				sub_pin_to_tnode[isub][SUB_OUTPUT][opin];
			    sink_delay = 0;
			}
		    else
			{	/* Sequential mode.  Load two clock nodes. */
			    inode =
				sub_pin_to_tnode[isub][SUB_CLOCK][clk_pin] +
				2 * used_opin_count;

			    /* First node is the clock input pin; it feeds the sequential output */
			    tedge = tnode[inode].out_edges;
			    tedge[0].to_node =
				sub_pin_to_tnode[isub][SUB_OUTPUT][opin];
			    tedge[0].Tdel =
				type->type_timing_inf.T_subblock[isub].
				T_seq_out[opin];

			    /* Now create the "sequential sink" -- i.e. the FF input node. */

			    inode++;
			    to_node = inode;
			    sink_delay =
				type->type_timing_inf.T_subblock[isub].
				T_seq_in[opin];
			}

		    /* Build and hook up subblock inputs. */

		    has_inputs = FALSE;
		    for(ipin = 0; ipin < type->max_subblock_inputs; ipin++)
			{
			    inode = sub_pin_to_tnode[isub][SUB_INPUT][ipin];

			    if(inode != OPEN)
				{	/* tnode exists -> pin is used */
				    has_inputs = TRUE;
				    tedge = tnode[inode].out_edges;
				    tedge[used_opin_count].to_node = to_node;
				    tedge[used_opin_count].Tdel =
					sink_delay +
					type->type_timing_inf.
					T_subblock[isub].T_comb[ipin][opin];
				}
			}

		    if(!has_inputs && type != IO_TYPE)
			{	/* Constant generator.  Give fake input. */

			    inode =
				sub_pin_to_tnode[isub][SUB_OUTPUT][opin] + 1;
			    tnode[inode].num_edges = 1;
			    tnode[inode].out_edges =
				(t_tedge *) my_chunk_malloc(sizeof(t_tedge),
							    &tedge_ch_list_head,
							    &tedge_ch_bytes_avail,
							    &tedge_ch_next_avail);
			    tedge = tnode[inode].out_edges;
			    tedge[used_opin_count].to_node = to_node;

			    /* Want constants generated early so they never affect the critical path. */

			    tedge[used_opin_count].Tdel =
				T_CONSTANT_GENERATOR;

			    tnode_descript[inode].type = CONSTANT_GEN_SOURCE;
			    tnode_descript[inode].ipin = OPEN;
			    tnode_descript[inode].isubblk = isub;
			    tnode_descript[inode].iblk = iblk;
			}
		    used_opin_count++;
		}
	}
    free_matrix(next_sblk_opin_edge, 0, type->max_subblocks - 1, 0,
		sizeof(int));
    free(num_opin_used_in_sblk);
}

static boolean
is_global_clock(int iblk,
		int sub,
		int subpin,
		int *num_subblocks_per_block,
		t_subblock ** subblock_inf)
{

/* Returns TRUE if the net driven by this block (which must be an INPAD) is  *
   (1) a global signal, and (2) used as a clock input to at least one block. 
   Assumes that there is only one subblock in an IO
 */

    int inet, ipin, to_blk, to_pin, isub;
    t_type_ptr type = block[iblk].type;

    assert(type == IO_TYPE);

    inet = block[iblk].nets[subblock_inf[iblk][sub].outputs[subpin]];
    assert(inet != OPEN);

    if(!net[inet].is_global)
	return (FALSE);

    for(ipin = 1; ipin < (net[inet].num_sinks + 1); ipin++)
	{
	    to_blk = net[inet].node_block[ipin];
	    to_pin = net[inet].node_block_pin[ipin];

	    for(isub = 0; isub < num_subblocks_per_block[to_blk]; isub++)
		{
		    if(subblock_inf[to_blk][isub].clock == to_pin)
			return (TRUE);
		}
	}

    return (FALSE);
}

void
load_timing_graph_net_delays(float **net_delay)
{

/* Sets the delays of the inter-FB nets to the values specified by          *
 * net_delay[0..num_nets-1][1..num_pins-1].  These net delays should have    *
 * been allocated and loaded with the net_delay routines.  This routine      *
 * marks the corresponding edges in the timing graph with the proper delay.  */

    int inet, ipin, inode;
    t_tedge *tedge;

    for(inet = 0; inet < num_nets; inet++)
	{
	    inode = net_to_driver_tnode[inet];
	    tedge = tnode[inode].out_edges;

/* Note that the edges of a tnode corresponding to a FB or INPAD opin must  *
 * be in the same order as the pins of the net driven by the tnode.          */

	    for(ipin = 1; ipin < (net[inet].num_sinks + 1); ipin++)
		tedge[ipin - 1].Tdel = net_delay[inet][ipin];
	}
}


void
free_timing_graph(float **net_slack)
{

/* Frees the timing graph data. */

    if(tedge_ch_list_head == NULL)
	{
	    printf("Error in free_timing_graph: No timing graph to free.\n");
	    exit(1);
	}

    free_chunk_memory(tedge_ch_list_head);
    free(tnode);
    free(tnode_descript);
    free(net_to_driver_tnode);
    free_ivec_vector(tnodes_at_level, 0, num_tnode_levels - 1);
    free(net_slack);

    tedge_ch_list_head = NULL;
    tedge_ch_bytes_avail = 0;
    tedge_ch_next_avail = NULL;

    tnode = NULL;
    tnode_descript = NULL;
    num_tnodes = 0;
    net_to_driver_tnode = NULL;
    tnodes_at_level = NULL;
    num_tnode_levels = 0;
}


void
print_net_slack(char *fname,
		float **net_slack)
{

/* Prints the net slacks into a file.                                     */

    int inet, ipin;
    FILE *fp;

    fp = my_fopen(fname, "w");

    fprintf(fp, "Net #\tSlacks\n\n");

    for(inet = 0; inet < num_nets; inet++)
	{
	    fprintf(fp, "%5d", inet);
	    for(ipin = 1; ipin < (net[inet].num_sinks + 1); ipin++)
		{
		    fprintf(fp, "\t%g", net_slack[inet][ipin]);
		}
	    fprintf(fp, "\n");
	}
}


void
print_timing_graph(char *fname)
{

/* Prints the timing graph into a file.           */

    FILE *fp;
    int inode, iedge, ilevel, i;
    t_tedge *tedge;
    t_tnode_type itype;
    char *tnode_type_names[] = { "INPAD_SOURCE", "INPAD_OPIN",
	"OUTPAD_IPIN", "OUTPAD_SINK", "FB_IPIN", "FB_OPIN",
	"SUBBLK_IPIN", "SUBBLK_OPIN", "FF_SINK", "FF_SOURCE",
	"CONSTANT_GEN_SOURCE"
    };


    fp = my_fopen(fname, "w");

    fprintf(fp, "num_tnodes: %d\n", num_tnodes);
    fprintf(fp, "Node #\tType\t\tipin\tisubblk\tiblk\t# edges\t"
	    "Edges (to_node, Tdel)\n\n");

    for(inode = 0; inode < num_tnodes; inode++)
	{
	    fprintf(fp, "%d\t", inode);

	    itype = tnode_descript[inode].type;
	    fprintf(fp, "%-15.15s\t", tnode_type_names[itype]);

	    fprintf(fp, "%d\t%d\t%d\t", tnode_descript[inode].ipin,
		    tnode_descript[inode].isubblk,
		    tnode_descript[inode].iblk);

	    fprintf(fp, "%d\t", tnode[inode].num_edges);
	    tedge = tnode[inode].out_edges;
	    for(iedge = 0; iedge < tnode[inode].num_edges; iedge++)
		{
		    fprintf(fp, "\t(%4d,%7.3g)", tedge[iedge].to_node,
			    tedge[iedge].Tdel);
		}
	    fprintf(fp, "\n");
	}

    fprintf(fp, "\n\nnum_tnode_levels: %d\n", num_tnode_levels);

    for(ilevel = 0; ilevel < num_tnode_levels; ilevel++)
	{
	    fprintf(fp, "\n\nLevel: %d  Num_nodes: %d\nNodes:", ilevel,
		    tnodes_at_level[ilevel].nelem);
	    for(i = 0; i < tnodes_at_level[ilevel].nelem; i++)
		fprintf(fp, "\t%d", tnodes_at_level[ilevel].list[i]);
	}

    fprintf(fp, "\n");
    fprintf(fp, "\n\nNet #\tNet_to_driver_tnode\n");

    for(i = 0; i < num_nets; i++)
	fprintf(fp, "%4d\t%6d\n", i, net_to_driver_tnode[i]);

    fprintf(fp, "\n\nNode #\t\tT_arr\t\tT_req\n\n");

    for(inode = 0; inode < num_tnodes; inode++)
	fprintf(fp, "%d\t%12g\t%12g\n", inode, tnode[inode].T_arr,
		tnode[inode].T_req);

    fclose(fp);
}


float
load_net_slack(float **net_slack,
	       float target_cycle_time)
{

/* Determines the slack of every source-sink pair of block pins in the      *
 * circuit.  The timing graph must have already been built.  target_cycle_  *
 * time is the target delay for the circuit -- if 0, the target_cycle_time  *
 * is set to the critical path found in the timing graph.  This routine     *
 * loads net_slack, and returns the current critical path delay.            */

    float T_crit, T_arr, Tdel, T_cycle, T_req;
    int inode, ilevel, num_at_level, i, num_edges, iedge, to_node;
    t_tedge *tedge;

/* Reset all arrival times to -ve infinity.  Can't just set to zero or the   *
 * constant propagation (constant generators work at -ve infinity) won't     *
 * work.                                                                     */

    for(inode = 0; inode < num_tnodes; inode++)
	tnode[inode].T_arr = T_CONSTANT_GENERATOR;

/* Compute all arrival times with a breadth-first analysis from inputs to   *
 * outputs.  Also compute critical path (T_crit).                           */

    T_crit = 0.;

/* Primary inputs arrive at T = 0. */

    num_at_level = tnodes_at_level[0].nelem;
    for(i = 0; i < num_at_level; i++)
	{
	    inode = tnodes_at_level[0].list[i];
	    tnode[inode].T_arr = 0.;
	}

    for(ilevel = 0; ilevel < num_tnode_levels; ilevel++)
	{
	    num_at_level = tnodes_at_level[ilevel].nelem;

	    for(i = 0; i < num_at_level; i++)
		{
		    inode = tnodes_at_level[ilevel].list[i];
		    T_arr = tnode[inode].T_arr;
		    num_edges = tnode[inode].num_edges;
		    tedge = tnode[inode].out_edges;
		    T_crit = max(T_crit, T_arr);

		    for(iedge = 0; iedge < num_edges; iedge++)
			{
			    to_node = tedge[iedge].to_node;
			    Tdel = tedge[iedge].Tdel;
			    tnode[to_node].T_arr =
				max(tnode[to_node].T_arr, T_arr + Tdel);
			}
		}

	}

    if(target_cycle_time > 0.)	/* User specified target cycle time */
	T_cycle = target_cycle_time;
    else			/* Otherwise, target = critical path */
	T_cycle = T_crit;

/* Compute the required arrival times with a backward breadth-first analysis *
 * from sinks (output pads, etc.) to primary inputs.                         */

    for(ilevel = num_tnode_levels - 1; ilevel >= 0; ilevel--)
	{
	    num_at_level = tnodes_at_level[ilevel].nelem;

	    for(i = 0; i < num_at_level; i++)
		{
		    inode = tnodes_at_level[ilevel].list[i];
		    num_edges = tnode[inode].num_edges;

		    if(num_edges == 0)
			{	/* sink */
			    tnode[inode].T_req = T_cycle;
			}
		    else
			{
			    tedge = tnode[inode].out_edges;
			    to_node = tedge[0].to_node;
			    Tdel = tedge[0].Tdel;
			    T_req = tnode[to_node].T_req - Tdel;

			    for(iedge = 1; iedge < num_edges; iedge++)
				{
				    to_node = tedge[iedge].to_node;
				    Tdel = tedge[iedge].Tdel;
				    T_req =
					min(T_req,
					    tnode[to_node].T_req - Tdel);
				}

			    tnode[inode].T_req = T_req;
			}
		}
	}

    compute_net_slacks(net_slack);

    return (T_crit);
}


static void
compute_net_slacks(float **net_slack)
{

/* Puts the slack of each source-sink pair of block pins in net_slack.     */

    int inet, iedge, inode, to_node, num_edges;
    t_tedge *tedge;
    float T_arr, Tdel, T_req;

    for(inet = 0; inet < num_nets; inet++)
	{
	    inode = net_to_driver_tnode[inet];
	    T_arr = tnode[inode].T_arr;
	    num_edges = tnode[inode].num_edges;
	    tedge = tnode[inode].out_edges;

	    for(iedge = 0; iedge < num_edges; iedge++)
		{
		    to_node = tedge[iedge].to_node;
		    Tdel = tedge[iedge].Tdel;
		    T_req = tnode[to_node].T_req;
		    net_slack[inet][iedge + 1] = T_req - T_arr - Tdel;
		}
	}
}


void
print_critical_path(char *fname,
		    t_subblock_data subblock_data)
{

/* Prints out the critical path to a file.  */

    t_linked_int *critical_path_head, *critical_path_node;
    FILE *fp;
    int non_global_nets_on_crit_path, global_nets_on_crit_path;
    int tnodes_on_crit_path, inode, iblk, inet;
    t_tnode_type type;
    float total_net_delay, total_logic_delay, Tdel;

    critical_path_head = allocate_and_load_critical_path();
    critical_path_node = critical_path_head;

    fp = my_fopen(fname, "w");

    non_global_nets_on_crit_path = 0;
    global_nets_on_crit_path = 0;
    tnodes_on_crit_path = 0;
    total_net_delay = 0.;
    total_logic_delay = 0.;

    while(critical_path_node != NULL)
	{
	    Tdel =
		print_critical_path_node(fp, critical_path_node,
					 subblock_data);
	    inode = critical_path_node->data;
	    type = tnode_descript[inode].type;
	    tnodes_on_crit_path++;

	    if(type == INPAD_OPIN || type == FB_OPIN)
		{
		    get_tnode_block_and_output_net(inode, &iblk, &inet);

		    if(!net[inet].is_global)
			non_global_nets_on_crit_path++;
		    else
			global_nets_on_crit_path++;

		    total_net_delay += Tdel;
		}
	    else
		{
		    total_logic_delay += Tdel;
		}

	    critical_path_node = critical_path_node->next;
	}

    fprintf(fp,
	    "\nTnodes on crit. path: %d  Non-global nets on crit. path: %d."
	    "\n", tnodes_on_crit_path, non_global_nets_on_crit_path);
    fprintf(fp, "Global nets on crit. path: %d.\n", global_nets_on_crit_path);
    fprintf(fp, "Total logic delay: %g (s)  Total net delay: %g (s)\n",
	    total_logic_delay, total_net_delay);

    printf("Nets on crit. path: %d normal, %d global.\n",
	   non_global_nets_on_crit_path, global_nets_on_crit_path);

    printf("Total logic delay: %g (s)  Total net delay: %g (s)\n",
	   total_logic_delay, total_net_delay);

    fclose(fp);
    free_int_list(&critical_path_head);
}


t_linked_int *
allocate_and_load_critical_path(void)
{

/* Finds the critical path and puts a list of the tnodes on the critical    *
 * path in a linked list, from the path SOURCE to the path SINK.            */

    t_linked_int *critical_path_head, *curr_crit_node, *prev_crit_node;
    int inode, iedge, to_node, num_at_level, i, crit_node, num_edges;
    float min_slack, slack;
    t_tedge *tedge;

    num_at_level = tnodes_at_level[0].nelem;
    min_slack = HUGE_FLOAT;
    crit_node = OPEN;		/* Stops compiler warnings. */

    for(i = 0; i < num_at_level; i++)
	{			/* Path must start at SOURCE (no inputs) */
	    inode = tnodes_at_level[0].list[i];
	    slack = tnode[inode].T_req - tnode[inode].T_arr;

	    if(slack < min_slack)
		{
		    crit_node = inode;
		    min_slack = slack;
		}
	}

    critical_path_head = (t_linked_int *) my_malloc(sizeof(t_linked_int));
    critical_path_head->data = crit_node;
    prev_crit_node = critical_path_head;
    num_edges = tnode[crit_node].num_edges;

    while(num_edges != 0)
	{			/* Path will end at SINK (no fanout) */
	    curr_crit_node = (t_linked_int *) my_malloc(sizeof(t_linked_int));
	    prev_crit_node->next = curr_crit_node;
	    tedge = tnode[crit_node].out_edges;
	    min_slack = HUGE_FLOAT;

	    for(iedge = 0; iedge < num_edges; iedge++)
		{
		    to_node = tedge[iedge].to_node;
		    slack = tnode[to_node].T_req - tnode[to_node].T_arr;

		    if(slack < min_slack)
			{
			    crit_node = to_node;
			    min_slack = slack;
			}
		}

	    curr_crit_node->data = crit_node;
	    prev_crit_node = curr_crit_node;
	    num_edges = tnode[crit_node].num_edges;
	}

    prev_crit_node->next = NULL;
    return (critical_path_head);
}


void
get_tnode_block_and_output_net(int inode,
			       int *iblk_ptr,
			       int *inet_ptr)
{

/* Returns the index of the block that this tnode is part of.  If the tnode *
 * is a FB_OPIN or INPAD_OPIN (i.e. if it drives a net), the net index is  *
 * returned via inet_ptr.  Otherwise inet_ptr points at OPEN.               */

    int inet, ipin, iblk;
    t_tnode_type tnode_type;

    iblk = tnode_descript[inode].iblk;
    tnode_type = tnode_descript[inode].type;

    if(tnode_type == FB_OPIN || tnode_type == INPAD_OPIN)
	{
	    ipin = tnode_descript[inode].ipin;
	    inet = block[iblk].nets[ipin];
	}
    else
	{
	    inet = OPEN;
	}

    *iblk_ptr = iblk;
    *inet_ptr = inet;
}


void
do_constant_net_delay_timing_analysis(t_timing_inf timing_inf,
				      t_subblock_data subblock_data,
				      float constant_net_delay_value)
{

/* Does a timing analysis (simple) where it assumes that each net has a      *
 * constant delay value.  Used only when operation == TIMING_ANALYSIS_ONLY.  */

    struct s_linked_vptr *net_delay_chunk_list_head;
    float **net_delay, **net_slack;

    float T_crit;

    net_slack = alloc_and_load_timing_graph(timing_inf, subblock_data);
    net_delay = alloc_net_delay(&net_delay_chunk_list_head);

    load_constant_net_delay(net_delay, constant_net_delay_value);
    load_timing_graph_net_delays(net_delay);
    T_crit = load_net_slack(net_slack, 0);

    printf("\n");
    printf("\nCritical Path: %g (s)\n", T_crit);

#ifdef CREATE_ECHO_FILES
    print_critical_path("critical_path.echo", subblock_data);
    print_timing_graph("timing_graph.echo");
    print_net_slack("net_slack.echo", net_slack);
    print_net_delay(net_delay, "net_delay.echo");
#endif /* CREATE_ECHO_FILES */

    free_timing_graph(net_slack);
    free_net_delay(net_delay, &net_delay_chunk_list_head);
}
