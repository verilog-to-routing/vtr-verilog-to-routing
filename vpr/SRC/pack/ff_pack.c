#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "ff_pack.h"

void absorb_buffer_luts() {
	/* This routine uses a simple pattern matching algorithm to remove buffer LUTs where possible (single-input LUTs that are programmed to be a wire) */

	int bnum, in_blk, out_blk, ipin, out_net, in_net;
	int removed = 0;

	/* Pin ordering for the clb blocks (1 VPACK_LUT + 1 FF in each logical_block) is      *
	* output, n VPACK_LUT inputs, clock input.                                   */

	for (bnum=0;bnum<num_logical_blocks;bnum++) {
		if (strcmp(logical_block[bnum].model->name, "names") == 0) {
			if(logical_block[bnum].truth_table != NULL && logical_block[bnum].truth_table->data_vptr) {
				if(strcmp("0 0", (char*)logical_block[bnum].truth_table->data_vptr) == 0 ||
					strcmp("1 1", (char*)logical_block[bnum].truth_table->data_vptr) == 0)
				{
					for(ipin = 0; ipin < logical_block[bnum].model->inputs->size; ipin++) {
						if(logical_block[bnum].input_nets[0][ipin] == OPEN)
							break;
					}
					assert(ipin == 1);

					assert(logical_block[bnum].clock_net == OPEN);
					assert(logical_block[bnum].model->inputs->next == NULL);
					assert(logical_block[bnum].model->outputs->size == 1);
					assert(logical_block[bnum].model->outputs->next == NULL);
					
					in_net = logical_block[bnum].input_nets[0][0];   /* Net driving the buffer */
					out_net = logical_block[bnum].output_nets[0][0];   /* Net the buffer us driving */
					out_blk = vpack_net[out_net].node_block[1];
					in_blk = vpack_net[in_net].node_block[0];

					assert(in_net != OPEN);
					assert(out_net != OPEN);
					assert(out_blk != OPEN);
					assert(in_blk != OPEN);
					
					/* TODO: Make this handle general cases, due to time reasons I can only handle buffers with single outputs */
					if (vpack_net[out_net].num_sinks == 1) {
						for(ipin = 1; ipin <= vpack_net[in_net].num_sinks; ipin++) {
							if(vpack_net[in_net].node_block[ipin] == bnum) {
								break;
							}
						}
						assert(ipin <= vpack_net[in_net].num_sinks);
						 
						vpack_net[in_net].node_block[ipin] = vpack_net[out_net].node_block[1];  /* New output */
						vpack_net[in_net].node_block_port[ipin] = vpack_net[out_net].node_block_port[1];
						vpack_net[in_net].node_block_pin[ipin] = vpack_net[out_net].node_block_pin[1];

						assert(logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]] == out_net);
						logical_block[out_blk].input_nets[vpack_net[out_net].node_block_port[1]][vpack_net[out_net].node_block_pin[1]] = in_net;
						
						vpack_net[out_net].node_block[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_pin[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].node_block_port[0] = OPEN; /* This vpack_net disappears; mark. */
						vpack_net[out_net].num_sinks = 0; /* This vpack_net disappears; mark. */
						
						logical_block[bnum].type = VPACK_EMPTY;   /* Mark logical_block that had LUT */
						
						/* error checking */
						for (ipin=0;ipin<=vpack_net[out_net].num_sinks;ipin++) {
							assert(vpack_net[out_net].node_block[ipin] != bnum);
						}
						removed++;
					}
				}
			}
		}
	}
	printf("Removed %d LUT buffers \n", removed);
}

void compress_netlist () {

/* This routine removes all the VPACK_EMPTY blocks and OPEN nets that *
 * may have been created by the ff_pack routine.  After this    *
 * routine, all the VPACK_LUT+FF blocks that exist in the netlist     *
 * are in a contiguous list with no unused spots.  The same     *
 * goes for the list of nets.  This means that blocks and nets  *
 * have to be renumbered somewhat.                              */

 int inet, iblk, index, ipin, new_num_nets, new_num_blocks, i;
 int *net_remap, *block_remap;
 int num_nets;
 t_model_ports *port;
 struct s_linked_vptr *tvptr, *next;

 new_num_nets = 0;
 new_num_blocks = 0;
 net_remap = my_malloc (num_logical_nets * sizeof(int));
 block_remap = my_malloc (num_logical_blocks * sizeof(int));

 for (inet=0;inet<num_logical_nets;inet++) {
	 if (vpack_net[inet].node_block[0] != OPEN) {
       net_remap[inet] = new_num_nets;
       new_num_nets++;
    }
    else {
       net_remap[inet] = OPEN;
    }
 }

 for (iblk=0;iblk<num_logical_blocks;iblk++) {
    if (logical_block[iblk].type != VPACK_EMPTY) {
       block_remap[iblk] = new_num_blocks;
       new_num_blocks++;
    }
    else {
       block_remap[iblk] = OPEN;
    }
 }

 
 if (new_num_nets != num_logical_nets || new_num_blocks != num_logical_blocks) {

    for (inet=0;inet<num_logical_nets;inet++) {
		if (vpack_net[inet].node_block[0] != OPEN) {
          index = net_remap[inet];
          vpack_net[index] = vpack_net[inet];
		  for (ipin = 0; ipin <= vpack_net[index].num_sinks; ipin++) 
		  {
			  vpack_net[index].node_block[ipin] = block_remap[vpack_net[index].node_block[ipin]];
		  }
       }
       else {
          free (vpack_net[inet].name);
		  free (vpack_net[inet].node_block);
		  free (vpack_net[inet].node_block_port);
		  free (vpack_net[inet].node_block_pin);
       }
    }
  
    num_logical_nets = new_num_nets;
    vpack_net = (struct s_net *) my_realloc (vpack_net, 
             num_logical_nets * sizeof (struct s_net));

    for (iblk=0;iblk<num_logical_blocks;iblk++) {
       if (logical_block[iblk].type != VPACK_EMPTY) {
          index = block_remap[iblk];
		  if(index != iblk) {
			logical_block[index] = logical_block[iblk];
			logical_block[index].index = index; /* array index moved */
		  }

		  num_nets = 0;
		  port = logical_block[index].model->inputs;
		  while(port) {
			  for(ipin = 0; ipin < port->size; ipin++) {
				  if(port->is_clock) {
					  assert(port->size == 1 && port->index == 0 && ipin == 0);
					  if(logical_block[index].clock_net == OPEN)
						continue;
					  logical_block[index].clock_net =
						net_remap[logical_block[index].clock_net];
				  } else {
					if(logical_block[index].input_nets[port->index][ipin] == OPEN)
					  continue;
					logical_block[index].input_nets[port->index][ipin] =
						net_remap[logical_block[index].input_nets[port->index][ipin]];
				  }
				num_nets++;
    		  }
			  port = port->next;
		  }

		  port = logical_block[index].model->outputs;
		  while(port) {
			  for(ipin = 0; ipin < port->size; ipin++) {
				  if(logical_block[index].output_nets[port->index][ipin] == OPEN)
					  continue;
				logical_block[index].output_nets[port->index][ipin] =
					net_remap[logical_block[index].output_nets[port->index][ipin]];
				num_nets++;
    		  }
			  port = port->next;
		  }
       }

       else {
			free (logical_block[iblk].name);
			port = logical_block[iblk].model->inputs;
			i = 0;
			while(port) {
				if(!port->is_clock) {
					if(logical_block[iblk].input_nets) {
						if(logical_block[iblk].input_nets[i]) {
							free(logical_block[iblk].input_nets[i]);
							logical_block[iblk].input_nets[i] = NULL;
						}
					}
					i++;
				}
				port = port->next;
			}
			if(logical_block[iblk].input_nets)
				free(logical_block[iblk].input_nets);
			port = logical_block[iblk].model->outputs;
			i = 0;
			while(port) {
				if(logical_block[iblk].output_nets) {
					if(logical_block[iblk].output_nets[i]) {
						free(logical_block[iblk].output_nets[i]);
						logical_block[iblk].output_nets[i] = NULL;
					}
				}
				i++;
				port = port->next;
			}
			if(logical_block[iblk].output_nets)
				free(logical_block[iblk].output_nets);
			tvptr = logical_block[iblk].truth_table;
			while(tvptr != NULL) {
				if(tvptr->data_vptr)
					free(tvptr->data_vptr);
				next = tvptr->next;
				free(tvptr);
				tvptr = next;
			}
       }
    }

	printf("Sweeped away %d nodes\n", num_logical_blocks - new_num_blocks);

    num_logical_blocks = new_num_blocks;
    logical_block = (struct s_logical_block *) my_realloc (logical_block, 
              num_logical_blocks * sizeof (struct s_logical_block));
 }

/* Now I have to recompute the number of primary inputs and outputs, since *
 * some inputs may have been unused and been removed.  No real need to     *
 * recount primary outputs -- it's just done as defensive coding.          */

 num_p_inputs = 0;
 num_p_outputs = 0;
 
 for (iblk=0;iblk<num_logical_blocks;iblk++) {
    if (logical_block[iblk].type == VPACK_INPAD) 
       num_p_inputs++;
    else if (logical_block[iblk].type == VPACK_OUTPAD) 
       num_p_outputs++;
 }
 
 
 free (net_remap);
 free (block_remap);
}

boolean *alloc_and_load_is_clock (boolean global_clocks) {

	/* Looks through all the logical_block to find and mark all the clocks, by setting *
	 * the corresponding entry in is_clock to true.  global_clocks is used     *
	 * only for an error check.                                                */

	int num_clocks, bnum, clock_net;
	boolean *is_clock;

	num_clocks = 0;

	is_clock = (boolean *) my_calloc (num_logical_nets, sizeof(boolean));

	/* Want to identify all the clock nets.  */

	for (bnum=0;bnum<num_logical_blocks;bnum++) {
		if(logical_block[bnum].type == VPACK_LATCH) {
			clock_net = logical_block[bnum].clock_net;
			assert(clock_net != OPEN);
			if (is_clock[clock_net] == FALSE) {
				is_clock[clock_net] = TRUE;
				num_clocks++;
			}
		} else {
			if(logical_block[bnum].clock_net != OPEN) {
				clock_net = logical_block[bnum].clock_net;
				if (is_clock[clock_net] == FALSE) {
					is_clock[clock_net] = TRUE;
					num_clocks++;
				}
			}
		}
	}

	/* If we have multiple clocks and we're supposed to declare them global, *
	* print a warning message, since it looks like this circuit may have    *
	* locally generated clocks.                                             */

	if (num_clocks > 1 && global_clocks) {
		printf("Warning:  circuit contains %d clocks.\n", num_clocks);
		printf("          All will be marked global.\n");
	}

	return (is_clock);
}
