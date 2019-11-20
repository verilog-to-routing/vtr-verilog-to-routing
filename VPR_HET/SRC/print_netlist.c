#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "print_netlist.h"


/******************** Subroutines local to this module ***********************/

static void print_pinnum(FILE * fp,
			 int pinnum);


/********************* Subroutine definitions ********************************/

void
print_netlist(char *foutput,
	      char *net_file,
	      t_subblock_data subblock_data)
{

/* Prints out the netlist related data structures into the file    *
 * fname.                                                          */

    int i, j, ipin, max_pin;
    int num_global_nets;
    int num_p_inputs, num_p_outputs;
    FILE *fp;
    t_subblock **subblock_inf;
    int *num_subblocks_per_block;

    num_global_nets = 0;
    num_p_inputs = 0;
    num_p_outputs = 0;

    /* Count number of global nets */
    for(i = 0; i < num_nets; i++)
	{
	    if(!net[i].is_global)
		{
		    num_global_nets++;
		}
	}

    /* Count I/O input and output pads */
    for(i = 0; i < num_blocks; i++)
	{
	    if(block[i].type == IO_TYPE)
		{
		    for(j = 0; j < IO_TYPE->num_pins; j++)
			{
			    if(block[i].nets[j] != OPEN)
				{
				    if(IO_TYPE->
				       class_inf[IO_TYPE->pin_class[j]].
				       type == DRIVER)
					{
					    num_p_inputs++;
					}
				    else
					{
					    assert(IO_TYPE->
						   class_inf[IO_TYPE->
							     pin_class[j]].
						   type == RECEIVER);
					    num_p_outputs++;
					}
				}
			}
		}
	}


    fp = my_fopen(foutput, "w");

    fprintf(fp, "Input netlist file: %s\n", net_file);
    fprintf(fp, "num_p_inputs: %d, num_p_outputs: %d, num_clbs: %d\n",
	    num_p_inputs, num_p_outputs, num_blocks);
    fprintf(fp, "num_blocks: %d, num_nets: %d, num_globals: %d\n",
	    num_blocks, num_nets, num_global_nets);
    fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (block, pin)\n");

    for(i = 0; i < num_nets; i++)
	{
	    fprintf(fp, "\n%d\t%s\t", i, net[i].name);
	    if(strlen(net[i].name) < 8)
		fprintf(fp, "\t");	/* Name field is 16 chars wide */
	    fprintf(fp, "%d", net[i].num_sinks + 1);
	    for(j = 0; j <= net[i].num_sinks; j++)
		fprintf(fp, "\t(%4d,%4d)", net[i].node_block[j],
			net[i].node_block_pin[j]);
	}

    fprintf(fp, "\nBlock\tName\t\tType\tPin Connections\n\n");

    for(i = 0; i < num_blocks; i++)
	{
	    fprintf(fp, "\n%d\t%s\t", i, block[i].name);
	    if(strlen(block[i].name) < 8)
		fprintf(fp, "\t");	/* Name field is 16 chars wide */
	    fprintf(fp, "%s", block[i].type->name);

	    max_pin = block[i].type->num_pins;

	    for(j = 0; j < max_pin; j++)
		print_pinnum(fp, block[i].nets[j]);
	}

    fprintf(fp, "\n");

/* Now print out subblock info. */

    subblock_inf = subblock_data.subblock_inf;
    num_subblocks_per_block = subblock_data.num_subblocks_per_block;


    fprintf(fp, "\n\nSubblock List:\n\n");

    for(i = 0; i < num_blocks; i++)
	{
	    fprintf(fp, "\nBlock: %d (%s)\tNum_subblocks: %d\n", i,
		    block[i].name, num_subblocks_per_block[i]);

	    /* Print header. */

	    fprintf(fp, "Index\tName\t\tInputs");
	    for(j = 0; j < block[i].type->max_subblock_inputs; j++)
		fprintf(fp, "\t");
	    fprintf(fp, "Outputs");
	    for(j = 0; j < block[i].type->max_subblock_outputs; j++)
		fprintf(fp, "\t");
	    fprintf(fp, "Clock\n");

	    /* Print subblock info for block i. */

	    for(j = 0; j < num_subblocks_per_block[i]; j++)
		{
		    fprintf(fp, "%d\t%s", j, subblock_inf[i][j].name);
		    if(strlen(subblock_inf[i][j].name) < 8)
			fprintf(fp, "\t");	/* Name field is 16 characters */
		    for(ipin = 0; ipin < block[i].type->max_subblock_inputs;
			ipin++)
			print_pinnum(fp, subblock_inf[i][j].inputs[ipin]);
		    for(ipin = 0; ipin < block[i].type->max_subblock_outputs;
			ipin++)
			print_pinnum(fp, subblock_inf[i][j].outputs[ipin]);
		    print_pinnum(fp, subblock_inf[i][j].clock);
		    fprintf(fp, "\n");
		}
	}

    fclose(fp);
}


static void
print_pinnum(FILE * fp,
	     int pinnum)
{

/* This routine prints out either OPEN or the pin number, to file fp. */

    if(pinnum == OPEN)
	fprintf(fp, "\tOPEN");
    else
	fprintf(fp, "\t%d", pinnum);
}
