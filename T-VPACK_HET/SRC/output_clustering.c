#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "vpack.h"
#include "globals.h"
#include "output_clustering.h"

#define LINELENGTH 80


/****************** Subroutines local to this module ************************/

static void print_subblock_ipin(int inet,
				int *io_net_mapping,
				int inputs_per_cluster,
				int cluster_size,
				int *column_ptr,
				FILE * fpout);

static void print_subblock_opin(int inet,
				int *io_net_mapping,
				int inputs_per_cluster,
				int *net_of_clb_pin,
				int *column_ptr,
				FILE * fpout,
				boolean muxes_to_cluster_output_pins);


/**************** Subroutine definitions ************************************/

static void
print_string(char *str_ptr,
	     int *column,
	     FILE * fpout)
{

/* Prints string without making any lines longer than LINELENGTH.  Column  *
 * points to the column in which the next character will go (both used and *
 * updated), and fpout points to the output file.                          */

    int len;

    len = strlen(str_ptr);
    if(len + 3 > LINELENGTH)
	{
	    printf
		("Error in print_string: String %s is too long for desired\n"
		 "maximum line length.\n", str_ptr);
	    exit(1);
	}

    if(*column + len + 2 > LINELENGTH)
	{
	    fprintf(fpout, "\\\n");
	    *column = 1;
	}

    fprintf(fpout, "%s ", str_ptr);
    *column += len + 1;
}


static void
print_net_name(int inet,
	       int *column,
	       FILE * fpout)
{

/* This routine prints out the net name (or open) and limits the    *
 * length of a line to LINELENGTH characters by using \ to continue *
 * lines.  net_num is the index of the net to be printed, while     *
 * column points to the current printing column (column is both     *
 * used and updated by this routine).  fpout is the output file     *
 * pointer.                                                         */

    char *str_ptr;

    if(inet == OPEN)
	str_ptr = "open";
    else
	str_ptr = net[inet].name;

    print_string(str_ptr, column, fpout);
}


static void
print_net_number(int inet,
		 int *column,
		 FILE * fpout)
{

/* Prints out either a number or "open".                     */

    char str[BUFSIZE];

    if(inet == OPEN)
	sprintf(str, "open");
    else
	sprintf(str, "%d", inet);

    print_string(str, column, fpout);
}


static void
print_pins(FILE * fpout,
	   int bnum,
	   int lut_size)
{

/* This routine is used only for netlists that have NOT been clustered.   *
 * It prints out the pinlist for a clb that contains either a LUT, a      *
 * LATCH or both, assuming that each LUT input needs its own pin, and     *
 * the output cannot be fed back to inputs via local routing.  It prints  *
 * the LUT inputs and output in the order below so that the net ordering  *
 * when parsed will be the same as that of the old combinational blif     *
 * parsing in vpr.  This allows the routing serial numbers to be compared.*/

    int ipin, column, netnum;

    fprintf(fpout, "pinlist: ");

    column = 10;		/* Next column I will write to. */

/* LUT inputs first. */

    for(ipin = 1; ipin <= lut_size; ipin++)
	{
	    netnum = block[bnum].nets[ipin];
	    print_net_name(netnum, &column, fpout);
	}

/* Output next.  */

    netnum = block[bnum].nets[0];
    print_net_name(netnum, &column, fpout);

/* Clock last. */

    netnum = block[bnum].nets[lut_size + 1];
    print_net_name(netnum, &column, fpout);

    fprintf(fpout, "\n");
}


static void
print_basic_subblock(FILE * fpout,
		     int bnum,
		     int lut_size)
{

/* Prints out the subblock connectivity.  Used only when *
 * the -no_clustering option is selected -- prints out   *
 * basically redundant connection info for a LUT + FF    *
 * logic block.                                          */

    int inet, ipin, column;

    fprintf(fpout, "subblock: ");
    column = 11;
    print_string(block[bnum].name, &column, fpout);

    for(ipin = 1; ipin <= lut_size; ipin++)
	{			/* Inputs first. */
	    inet = block[bnum].nets[ipin];
	    if(inet == OPEN)
		print_net_number(OPEN, &column, fpout);
	    else
		print_net_number(ipin - 1, &column, fpout);
	}

    print_net_number(lut_size, &column, fpout);	/* Output next */

    inet = block[bnum].nets[lut_size + 1];
    if(inet == OPEN)
	print_net_number(OPEN, &column, fpout);
    else
	print_net_number(lut_size + 1, &column, fpout);

    fprintf(fpout, "\n\n");
}

static void
print_subckt(FILE * fpout,
		     struct s_subckt *subckt,
			 int index)
{

/* PAJ - Prints out the subckt with the output and input pins
 * reconnected from what were previously primary inputs
 * and primary outputs */

	int i, j=0;
	int column;
	char *signal_name;
	char string[BUFSIZE];

	/* start with the name of the het structure */
    fprintf(fpout, ".%s %s_%d\n", subckt->name, subckt->name, index);

	/* print the pinlist */
	fprintf(fpout, "pinlist: ");
    column = 10;		/* Next column I will write to. */
	
	for (i = 0; i < subckt->num_inputs; i++)
	{
		if (subckt->input_signal_to_block[i] == -1)
		{
    		print_string("open", &column, fpout);
		}
		else
		{
			signal_name = subckt->in_blocks[subckt->input_signal_to_block[i]]->name;
			print_string(signal_name, &column, fpout);
		}
	}
	for (i = 0; i < subckt->num_outputs; i++)
	{
		if (subckt->output_signal_to_block[i] == -1)
		{
    		print_string("open", &column, fpout);
		}
		else
		{
			signal_name = subckt->out_blocks[subckt->output_signal_to_block[i]]->name;
			print_string(signal_name, &column, fpout);
		}
	}
	// add one more for an automatic clock
   	print_string("open", &column, fpout);
    fprintf(fpout, "\n");
	
	/* now print the subblock */
	fprintf(fpout, "subblock: sub0 ");
    column = 16;		/* Next column I will write to. */
	
	for (i = 0; i < subckt->num_inputs; i++)
	{
		if (subckt->input_signal_to_block[i] == -1)
		{
    		print_string("open", &column, fpout);
			j++;
		}
		else
		{
			sprintf(string, "%d", j++);
			print_string(string, &column, fpout);
		}
	}
	for (i = 0; i < subckt->num_outputs; i++)
	{
		if (subckt->output_signal_to_block[i] == -1)
		{
    		print_string("open", &column, fpout);
			j++;
		}
		else
		{
			sprintf(string, "%d", j++);
			print_string(string, &column, fpout);
		}
	}
	// add one more for an automatic clock
   	print_string("open", &column, fpout);

    fprintf(fpout, "\n\n");
}


static void
print_clbs(FILE * fpout,
	   int *cluster_occupancy,
	   int **cluster_contents,
	   int lut_size,
	   int cluster_size,
	   int inputs_per_cluster,
	   int clocks_per_cluster,
	   int num_clusters,
	   boolean muxes_to_cluster_output_pins)
{

/* Prints out one cluster (clb).  Both the external pins and the *
 * internal connections are printed out.                         */

    int ipin, icluster, sub_blk, num_clb_pins, inet, free_pin_index;
    int column, iblk;
    int redundant_outputs, used_outputs, total_used_ble_inputs;
    int total_locally_generated_ble_inputs, total_bles;

/* [0..num_nets-1].  Which clb pin connects to this net?   Need  *
 * separate data structures for input or output pins and clocks, *
 * because a clock net could connect to both a clb output and a  *
 * clock input.                                                  */

    int *io_net_mapping, *clock_net_mapping, *num_pins_in_cluster;

/* [0..num_cluster_pins].  Net connected to this clb pin. */
    int *net_of_clb_pin;


    io_net_mapping = (int *)my_malloc(num_nets * sizeof(int));
    clock_net_mapping = (int *)my_malloc(num_nets * sizeof(int));
    num_pins_in_cluster = (int *)my_malloc(num_nets * sizeof(int));

    for(inet = 0; inet < num_nets; inet++)
	{
	    io_net_mapping[inet] = OPEN;
	    clock_net_mapping[inet] = OPEN;
	    num_pins_in_cluster[inet] = 0;
	}

/* Counters used only for statistics purposes. */

    redundant_outputs = 0;
    total_used_ble_inputs = 0;
    total_locally_generated_ble_inputs = 0;

    num_clb_pins = inputs_per_cluster + cluster_size + clocks_per_cluster;
    net_of_clb_pin = my_malloc(num_clb_pins * sizeof(int));
    for(ipin = 0; ipin < num_clb_pins; ipin++)
	net_of_clb_pin[ipin] = OPEN;

    for(icluster = 0; icluster < num_clusters; icluster++)
	{

/* First I have to get a list of the pins that have to be routed to *
 * this cluster's inputs.  Also need to know which pin will supply  *
 * each net needed by the LUTs of this cluster.  Start with the     *
 * cluster output nets, as nets generated in the cluster don't need *
 * to be routed here as inputs.                                     */

	    for(iblk = 0; iblk < cluster_occupancy[icluster]; iblk++)
		{
		    sub_blk = cluster_contents[icluster][iblk];
		    inet = block[sub_blk].nets[0];	/* Output */
		    net_of_clb_pin[inputs_per_cluster + iblk] = inet;

		    if(io_net_mapping[inet] != OPEN)
			{
			    printf
				("Error in print_clbs.  Net #%d (%s) in cluster %d\n",
				 inet, net[inet].name, icluster);
			    printf("appears to be driven by two outputs.\n");
			    exit(1);
			}

		    io_net_mapping[inet] = inputs_per_cluster + iblk;
		    num_pins_in_cluster[inet]++;
		}

/* Now do the input pins. */

	    free_pin_index = 0;
	    for(iblk = 0; iblk < cluster_occupancy[icluster]; iblk++)
		{
		    sub_blk = cluster_contents[icluster][iblk];
		    for(ipin = 1; ipin <= lut_size; ipin++)
			{
			    inet = block[sub_blk].nets[ipin];
			    if(inet != OPEN)
				{
				    num_pins_in_cluster[inet]++;
				    total_used_ble_inputs++;	/* Used LUT inputs;doesn't include clk */

				    if(io_net_mapping[inet] == OPEN)
					{	/* Must get from CLB IPIN */
					    io_net_mapping[inet] =
						free_pin_index;
					    net_of_clb_pin[free_pin_index] =
						inet;
					    free_pin_index++;
					}

				    else if(io_net_mapping[inet] >=
					    inputs_per_cluster)
					{
					    /* Input is locally generated (output of a BLE). */
					    total_locally_generated_ble_inputs++;
					}
				}
			}
		}

	    if(free_pin_index > inputs_per_cluster)
		{
		    printf
			("Error in print_clbs:  cluster %d has %d input pins.\n",
			 icluster, free_pin_index);
		    exit(1);
		}

/* Now do the clocks. */

	    free_pin_index = inputs_per_cluster + cluster_size;
	    for(iblk = 0; iblk < cluster_occupancy[icluster]; iblk++)
		{
		    sub_blk = cluster_contents[icluster][iblk];
		    inet = block[sub_blk].nets[lut_size + 1];
		    if(inet != OPEN)
			{

/* No internal output - clock connection, so don't increment num_pins_in *
 * _cluster so that I can guarantee that all clock inputs and outputs    *
 * that generate clocks do come out to the cluster pins.                 */

			    if(clock_net_mapping[inet] == OPEN)
				{
				    clock_net_mapping[inet] = free_pin_index;
				    net_of_clb_pin[free_pin_index] = inet;
				    free_pin_index++;
				}
			}
		}

	    if(free_pin_index > num_clb_pins)
		{
		    printf
			("Error in print_clbs:  cluster %d has %d clock pins.\n",
			 icluster,
			 free_pin_index - inputs_per_cluster - cluster_size);
		    exit(1);
		}

/* Remove all nets that are now routed completely within the cluster from  *
 * the cluster input/output pins.                                          */

	    for(ipin = 0; ipin < num_clb_pins; ipin++)
		{
		    inet = net_of_clb_pin[ipin];
		    if(inet != OPEN)
			{
			    if(num_pins_in_cluster[inet] ==
			       net[inet].num_pins)
				{
				    net_of_clb_pin[ipin] = OPEN;	/* Remove. */
				    redundant_outputs++;
				    if(ipin < inputs_per_cluster
				       || ipin >=
				       inputs_per_cluster + cluster_size)
					{
					    printf
						("Error in print_clbs:  cluster %d pin %d is not an\n",
						 icluster, ipin);
					    printf
						("output, but it has been marked as a net that doesn't\n");
					    printf
						("fanout, and will be removed.  Net #%d (%s)\n",
						 inet, net[inet].name);
					    exit(1);
					}
				}
			}
		}

/* Do the actual printout. */

/* Name clb (arbitrarily) after the first block it contains. */

	    fprintf(fpout, ".clb %s\n",
		    block[cluster_contents[icluster][0]].name);
	    fprintf(fpout, "pinlist: ");
	    column = 10;	/* Next column I will write to. */
	    for(ipin = 0; ipin < num_clb_pins; ipin++)
		print_net_name(net_of_clb_pin[ipin], &column, fpout);
	    fprintf(fpout, "\n");

	    for(iblk = 0; iblk < cluster_occupancy[icluster]; iblk++)
		{
		    sub_blk = cluster_contents[icluster][iblk];
		    fprintf(fpout, "subblock: ");
		    column = 11;
		    print_string(block[sub_blk].name, &column, fpout);

		    for(ipin = 1; ipin <= lut_size; ipin++)
			{	/* Inputs first. */
			    inet = block[sub_blk].nets[ipin];
			    print_subblock_ipin(inet, io_net_mapping,
						inputs_per_cluster,
						cluster_size, &column, fpout);
			}

		    inet = block[sub_blk].nets[0];	/* Output */
		    print_subblock_opin(inet, io_net_mapping,
					inputs_per_cluster, net_of_clb_pin,
					&column, fpout,
					muxes_to_cluster_output_pins);

		    inet = block[sub_blk].nets[lut_size + 1];	/* Clock */
		    print_subblock_ipin(inet, clock_net_mapping,
					inputs_per_cluster, cluster_size,
					&column, fpout);

		    fprintf(fpout, "\n");
		}

	    fprintf(fpout, "\n");

/* Reset the data structures in order to do the next cluster. */

	    for(iblk = 0; iblk < cluster_occupancy[icluster]; iblk++)
		{
		    sub_blk = cluster_contents[icluster][iblk];
		    for(ipin = 0; ipin <= lut_size; ipin++)
			{
			    inet = block[sub_blk].nets[ipin];
			    if(inet != OPEN)
				{
				    io_net_mapping[inet] = OPEN;
				    num_pins_in_cluster[inet] = 0;
				}
			}
		    inet = block[sub_blk].nets[lut_size + 1];	/* Clock */
		    if(inet != OPEN)
			clock_net_mapping[inet] = OPEN;
		}

	    for(ipin = 0; ipin < num_clb_pins; ipin++)
		net_of_clb_pin[ipin] = OPEN;
	}

    used_outputs =
	num_blocks - num_p_inputs - num_p_outputs - redundant_outputs;
    total_bles = num_blocks - num_p_inputs - num_p_outputs;

    printf("\nAverage number of cluster outputs used outside cluster: %f\n",
	   (float)used_outputs / (float)num_clusters);

    printf("Fraction outputs made local: %f\n", (float)redundant_outputs /
	   (float)(total_bles));

    printf
	("Average number of used LUT inputs per BLE (excluding clocks): %f\n",
	 (float)total_used_ble_inputs / (float)total_bles);

    printf("Average fraction of BLE (LUT) inputs generated locally: %f\n",
	   (float)total_locally_generated_ble_inputs /
	   (float)total_used_ble_inputs);

    free(io_net_mapping);
    free(clock_net_mapping);
    free(num_pins_in_cluster);
    free(net_of_clb_pin);
}


static void
print_subblock_ipin(int inet,
		    int *io_net_mapping,
		    int inputs_per_cluster,
		    int cluster_size,
		    int *column_ptr,
		    FILE * fpout)
{

/* Prints out where a subblock gets its input pin from. */

    char str[BUFSIZE];
    int ble_num;

    if(inet == OPEN)
	{
	    print_net_number(OPEN, column_ptr, fpout);
	}
    else if(io_net_mapping[inet] >= inputs_per_cluster && io_net_mapping[inet]
	    < inputs_per_cluster + cluster_size)
	{

	    /* Subblock input comes from a BLE output within this cluster. */

	    ble_num = io_net_mapping[inet] - inputs_per_cluster;
	    sprintf(str, "ble_%d", ble_num);
	    print_string(str, column_ptr, fpout);
	}
    else
	{			/* Regular input or clock pin */
	    print_net_number(io_net_mapping[inet], column_ptr, fpout);
	}
}


static void
print_subblock_opin(int inet,
		    int *io_net_mapping,
		    int inputs_per_cluster,
		    int *net_of_clb_pin,
		    int *column_ptr,
		    FILE * fpout,
		    boolean muxes_to_cluster_output_pins)
{

/* Prints out a subblock output pin.  If the net driven by this BLE is used *
 * only internally to the clb, then the subblock output is marked as OPEN.  */

    int clb_pin;

    if(inet == OPEN)
	{
	    printf("Error in print_subblock_opin:  inet = %d.\n", inet);
	    exit(1);
/*    print_net_number (OPEN, column_ptr, fpout);  */
	}
    else
	{
	    clb_pin = io_net_mapping[inet];
	    if(net_of_clb_pin[clb_pin] == OPEN
	       && muxes_to_cluster_output_pins)
		{
		    /* Output not used outside clb and subblock output not directly    *
		     * connected to the CLB output pin.                                */
		    print_net_number(OPEN, column_ptr, fpout);
		}
	    else
		{
		    print_net_number(clb_pin, column_ptr, fpout);
		}
	}
}


void
output_clustering(int **cluster_contents,
		  int *cluster_occupancy,
		  int cluster_size,
		  int inputs_per_cluster,
		  int clocks_per_cluster,
		  int num_clusters,
		  int lut_size,
		  boolean global_clocks,
		  boolean muxes_to_cluster_output_pins,
		  boolean * is_clock,
		  char *out_fname)
{

/* This routine dumps out the output netlist in a format suitable for  *
 * input to vpr.  The clb pins are in the following order:             *
 * inputs_per_cluster inputs, cluster_size outputs, and                *
 * clocks_per_cluster clocks.  For a single LUT cluster, this reduces  *
 * to N LUT inputs, 1 output, followed by the clock input.  Pins that  *
 * are unconnected are marked as open.  If global_clocks is TRUE, all  *
 * the clock nets (nets marked with TRUE in is_clock) will be set to   *
 * be global.  This routine also dumps out the internal structure of   *
 * the cluster, in essentially a graph based format.  For each LUT +   * 
 * FF subblock, I write out which cluster pin connects to each of the  *
 * LUT pins.  The LUT + FF pins are ordered as N LUT inputs, LUT       *
 * output, then clock.  If a pin is not connected, it is marked as     *
 * OPEN.  Otherwise, it is given the number of the cluster pin to      *
 * which it connects -- the cluster pins begin with inputs (starting   *
 * from 0), then list outputs, then clocks.  Hence all cluster pins    *
 * have an index from 0 to inputs_per_cluster + cluster_size +         *
 * clocks_per_cluster - 1.                                             *
 * If cluster_contents is NULL, this routine prints out an unclustered *
 * netlist -- i.e. it just prints out the LUT+FF logic blocks with all *
 * pins distint.                                                       */

    FILE *fpout;
    int bnum, netnum, column;
	int i, j;
    boolean skip_clustering;

    if(cluster_contents == NULL)	/* No clustering was performed. */
		skip_clustering = TRUE;
    else
		skip_clustering = FALSE;

    fpout = my_fopen(out_fname, "w", 0);

    if(global_clocks)
	{
	    for(netnum = 0; netnum < num_nets; netnum++)
		{
		    if(is_clock[netnum])
			fprintf(fpout, ".global %s\n\n", net[netnum].name);
		}
	}

/* PAJ - mark all the primary inputs and outputs of the subckts as subckt pins so they;re not prinmted out */
	for (i = 0; i < num_subckts; i++)
	{
		for (j = 0; j < subckt[i].num_out_blocks; j++)
		{
			if (subckt[i].output_signal_to_block[j] != -1)
				subckt[i].out_blocks[j]->type = SUBCKT;
		}
		for (j = 0; j < subckt[i].num_in_blocks; j++)
		{
			if (subckt[i].input_signal_to_block[j] != -1)
				subckt[i].in_blocks[j]->type = SUBCKT;
		}
	}

/* Print out all input and output pads. */

    for(bnum = 0; bnum < num_blocks; bnum++)
	{
	    switch (block[bnum].type)
		{

		case INPAD:
		    fprintf(fpout, ".input %s\n", block[bnum].name);
		    netnum = block[bnum].nets[0];
		    fprintf(fpout, "pinlist: ");
		    column = 10;
		    print_net_name(netnum, &column, fpout);
		    fprintf(fpout, "\n\n");
		    break;

		case OUTPAD:
		    fprintf(fpout, ".output %s\n", block[bnum].name);
		    netnum = block[bnum].nets[0];
		    fprintf(fpout, "pinlist: ");
		    column = 10;
		    print_net_name(netnum, &column, fpout);
		    fprintf(fpout, "\n\n");
		    break;

		case LUT:
		    if(skip_clustering)
			{
			    fprintf(fpout, ".clb %s  # Only LUT used.\n",
				    block[bnum].name);
			    print_pins(fpout, bnum, lut_size);
			    print_basic_subblock(fpout, bnum, lut_size);
			}
		    break;

		case LATCH:
		    if(skip_clustering)
			{
			    fprintf(fpout, ".clb %s  # Only LATCH used.\n",
				    block[bnum].name);
			    print_pins(fpout, bnum, lut_size);
			    print_basic_subblock(fpout, bnum, lut_size);
			}
		    break;

		case LUT_AND_LATCH:
		    if(skip_clustering)
			{
			    fprintf(fpout,
				    ".clb %s  # Both LUT and LATCH used.\n",
				    block[bnum].name);
			    print_pins(fpout, bnum, lut_size);
			    print_basic_subblock(fpout, bnum, lut_size);
			}
		    break;

			/* PAJ - do nothing for these blocks */
		case SUBCKT:
			break;

		case EMPTY:
		    printf("Error in output_netlist -- block %d is EMPTY.\n",
			   bnum);
		    exit(1);
		    break;

		default:
		    printf
			("Error in output_netlist.  Unexpected type %d for block"
			 "%d.\n", block[bnum].type, bnum);
		}
	}

    if(skip_clustering == FALSE)
	print_clbs(fpout, cluster_occupancy, cluster_contents, lut_size,
		   cluster_size, inputs_per_cluster, clocks_per_cluster,
		   num_clusters, muxes_to_cluster_output_pins);

/* PAJ - now we're going to print the subckts */
	for (i = 0; i < num_subckts; i++)
	{
		print_subckt(fpout, &subckt[i], i);
	}

    fclose(fpout);
}
