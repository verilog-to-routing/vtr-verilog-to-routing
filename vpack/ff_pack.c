#include <stdio.h>
#include <string.h>
#include "vpack.h"
#include "ext.h"
#include "util.h"
#include "ff_pack.h"

void pack_luts_and_ffs (int lut_size) {

/* This routine uses a simple pattern matching algorithm to pack flip-  *
 * flops into the same clb as a LUT, where possible.  Currently the     *
 * pattern to be matched is hard-coded into the program.  It is a FF    *
 * following a LUT whose output does not fan-out.  If a FF does not     *
 * fit this pattern, it is left as in its own clb (the LUT in this      *
 * clb is unused).  This pattern corresponds to a clb with n LUT inputs * 
 * and a clock input.  It has only one output -- a multiplexer selects  *
 * whether this output comes from the LUT or the FF.  As well, all      *
 * unconnected pins in a clb are set to OPEN by this routine.           */

 int bnum, in_blk, ipin, out_net, clock_net, in_net;

/* Pin ordering for the clb blocks (1 LUT + 1 FF in each block) is      *
 * output, n LUT inputs, clock input.                                   */

 for (bnum=0;bnum<num_blocks;bnum++) {
    if (block[bnum].type == LATCH) {

       clock_net = block[bnum].nets[lut_size + 1];
       in_net = block[bnum].nets[1];   /* Net driving the latch. */

       if (net[in_net].num_pins == 2) {   /* No fanout from driver */
          in_blk = net[in_net].pins[0];   /* Drives the FF input */
          if (block[in_blk].type == LUT) {    /* We match! */

/* Fold the LATCH into previous LUT block.  Update block and net data. */
             
             net[in_net].pins[0] = OPEN; /* This net disappears; mark. */
             out_net = block[bnum].nets[0];
             block[in_blk].nets[0] = out_net;  /* New output */
             free (block[in_blk].name);
             block[in_blk].name = block[bnum].name;  /* Rename block */

/* Clock new FF location. */

             block[in_blk].nets[lut_size+1] = clock_net; 
             block[in_blk].num_nets++;

             block[bnum].type = EMPTY;   /* Mark block that had FF */
             block[in_blk].type = LUT_AND_LATCH;

/* Nets that connected to old LATCH move from block bnum to block in_blk */

             for (ipin=0;ipin<net[out_net].num_pins;ipin++) {
                if (net[out_net].pins[ipin] == bnum) 
                   net[out_net].pins[ipin] = in_blk; 
             }

             for (ipin=0;ipin<net[clock_net].num_pins;ipin++) {
                if (net[clock_net].pins[ipin] == bnum) 
                   net[clock_net].pins[ipin] = in_blk; 
             }
          }             
       }
    }
 }

}


void compress_netlist (int lut_size) {

/* This routine removes all the EMPTY blocks and OPEN nets that *
 * may have been created by the ff_pack routine.  After this    *
 * routine, all the LUT+FF blocks that exist in the netlist     *
 * are in a contiguous list with no unused spots.  The same     *
 * goes for the list of nets.  This means that blocks and nets  *
 * have to be renumbered somewhat.                              */

 int inet, iblk, index, ipin, new_num_nets, new_num_blocks, max_pin;
 int *net_remap, *block_remap;

 new_num_nets = 0;
 new_num_blocks = 0;
 net_remap = my_malloc (num_nets * sizeof(int));
 block_remap = my_malloc (num_blocks * sizeof(int));

 for (inet=0;inet<num_nets;inet++) {
    if (net[inet].pins[0] != OPEN) {
       net_remap[inet] = new_num_nets;
       new_num_nets++;
    }
    else {
       net_remap[inet] = OPEN;
    }
 }

 for (iblk=0;iblk<num_blocks;iblk++) {
    if (block[iblk].type != EMPTY) {
       block_remap[iblk] = new_num_blocks;
       new_num_blocks++;
    }
    else {
       block_remap[iblk] = -1;
    }
 }

 
 if (new_num_nets != num_nets || new_num_blocks != num_blocks) {

    for (inet=0;inet<num_nets;inet++) {
       if (net[inet].pins[0] != OPEN) {
          index = net_remap[inet];
          net[index] = net[inet];
          for (ipin=0;ipin<net[index].num_pins;ipin++) 
             net[index].pins[ipin] = block_remap[net[index].pins[ipin]];
       }
       else {
          free (net[inet].name);
          free (net[inet].pins);
       }
    }
  
    num_nets = new_num_nets;
    net = (struct s_net *) my_realloc (net, 
             num_nets * sizeof (struct s_net));

    for (iblk=0;iblk<num_blocks;iblk++) {
       if (block[iblk].type != EMPTY) {
          index = block_remap[iblk];
          block[index] = block[iblk];

          if (block[index].type == INPAD || block[index].type == OUTPAD) 
             max_pin = 1;
          else
             max_pin = lut_size+2;

          for (ipin=0;ipin<max_pin;ipin++) {

              if (block[index].nets[ipin] != OPEN) 
                 block[index].nets[ipin] =
                     net_remap[block[index].nets[ipin]];
              else 
                 block[index].nets[ipin] = OPEN;
          }

/* Note:  I don't deallocate an empty block's name here because *
 * that was ALREADY done in ff_pack.  As well, the nets element *
 * is statically allocated, so I can't deallocate it.           */

       }
    }

    num_blocks = new_num_blocks;
    block = (struct s_block *) my_realloc (block, 
              num_blocks * sizeof (struct s_block));
 }

/* Now I have to recompute the number of primary inputs and outputs, since *
 * some inputs may have been unused and been removed.  No real need to     *
 * recount primary outputs -- it's just done as defensive coding.          */

 num_p_inputs = 0;
 num_p_outputs = 0;
 
 for (iblk=0;iblk<num_blocks;iblk++) {
    if (block[iblk].type == INPAD) 
       num_p_inputs++;
    else if (block[iblk].type == OUTPAD) 
       num_p_outputs++;
 }
 
 
 free (net_remap);
 free (block_remap);
}


boolean *alloc_and_load_is_clock (boolean global_clocks, int lut_size) {

/* Looks through all the block to find and mark all the clocks, by setting *
 * the corresponding entry in is_clock to true.  global_clocks is used     *
 * only for an error check.                                                */

 int num_clocks, bnum, clock_net;
 boolean *is_clock;

 num_clocks = 0;

 is_clock = (boolean *) my_calloc (num_nets, sizeof(boolean));

/* Want to identify all the clock nets.  */

 for (bnum=0;bnum<num_blocks;bnum++) {
    if (block[bnum].type == LATCH || block[bnum].type == LUT_AND_LATCH) {

       clock_net = block[bnum].nets[lut_size + 1];
       if (is_clock[clock_net] == FALSE) {
          is_clock[clock_net] = TRUE;
          num_clocks++;
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
