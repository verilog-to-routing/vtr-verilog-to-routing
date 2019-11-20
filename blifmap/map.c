#include <stdio.h>
#include <string.h>
#include "blifmap.h"
#include "ext.h"
#include "util.h"

void map_netlist (int global_clocks, int *is_clock) {
/* This routine uses a simple pattern matching algorithm to pack flip-  *
 * flops into the same clb as a LUT, where possible.  Currently the     *
 * pattern to be matched is hard-coded into the program.  It is a FF    *
 * following a LUT whose output does not fan-out.  If a FF does not     *
 * fit this pattern, it is left as in its own clb (the LUT in this      *
 * clb is unused).  This pattern corresponds to a clb with n LUT inputs * 
 * and a clock input.  It has only one output -- a multiplexer selects  *
 * whether this output comes from the LUT or the FF.  As well, all      *
 * unconnected pins in a clb are set to OPEN by this routine.           *
 * Finally, this routine also marks all the nets used to clock FFs by   *
 * setting their entry in is_clock to 1.                                */

 int bnum, in_blk, ipin, out_net, clock_net, in_net;
 int num_clocks;

/* Pin ordering for the clb blocks (1 LUT + 1 FF in each block) is      *
 * output, n LUT inputs, clock input.                                   */

 num_clocks = 0;

 for (bnum=0;bnum<num_blocks;bnum++) {
    if (block[bnum].type == LATCH) {

/* Want to identify all the clock nets.  */

       clock_net = block[bnum].nets[lut_size + 1];
       if (is_clock[clock_net] == 0) {
          is_clock[clock_net] = 1;
          num_clocks++;
       }

       in_net = block[bnum].nets[1];   /* Net driving the latch. */
       if (net[in_net].num_pins == 2) {   /* No fanout from driver */
          in_blk = net[in_net].pins[0];   /* Drives the FF input */
          if (block[in_blk].type == LUT) {    /* We match! */

/* Fold the LATCH into previous LUT block.  Update block and net data. */
             
             net[in_net].pins[0] = OPEN; /* This net disappears; mark it. */
             out_net = block[bnum].nets[0];
             block[in_blk].nets[0] = out_net;  /* New output */
             free (block[in_blk].name);
             block[in_blk].name = block[bnum].name;  /* Rename block */

/* Clock new FF location. */

             block[in_blk].nets[lut_size+1] = clock_net; 

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

/* If we have multiple clocks and we're supposed to declare them global, *
 * print a warning message, since it looks like this circuit may have    *
 * locally generated clocks.                                             */

 if (num_clocks > 1 && global_clocks == 1) {
    printf("Warning:  circuit contains %d clocks.\n", num_clocks);
    printf("          All will be marked global.\n");
 }
}


void output_netlist (char *out_name, int global_clocks, int *is_clock) {
/* This routine dumps out the output netlist in a format suitable for  *
 * input to vpr.  The clb pins are in the following order:  n LUT      *
 * inputs, output, clock input.  Pins that are unconnected are marked  *
 * as open.  If global_clocks is 1, all the clock nets (nets marked    *
 * with a 1 in is_clock) will be set to be global.                     */

 FILE *fpout;
 int bnum, netnum;
 void print_pins (FILE *fpout, int bnum);

 fpout = my_open (out_name, "w", 0);

 if (global_clocks == 1) {
    for (netnum=0;netnum<num_nets;netnum++) {
       if (is_clock[netnum]) 
          fprintf(fpout,".global %s\n\n", net[netnum].name);
    }
 }

 for (bnum=0;bnum<num_blocks;bnum++) {
    switch (block[bnum].type) {
      
    case INPAD:
       fprintf(fpout,".input %s\n", block[bnum].name);
       netnum = block[bnum].nets[0];
       fprintf(fpout,"  pinlist:  %s\n\n", net[netnum].name);
       break;

    case OUTPAD:
       fprintf(fpout,".output %s\n", block[bnum].name);
       netnum = block[bnum].nets[0];
       fprintf(fpout,"  pinlist:  %s\n\n", net[netnum].name);
       break;
  
    case LUT:
       fprintf(fpout,".clb %s  # Only LUT used.\n", block[bnum].name);
       print_pins (fpout, bnum);
       break;
 
    case LATCH:
       fprintf(fpout,".clb %s  # Only LATCH used.\n", block[bnum].name);
       print_pins (fpout, bnum);
       break;

    case LUT_AND_LATCH:
       fprintf(fpout,".clb %s  # Both LUT and LATCH used.\n", 
           block[bnum].name);
       print_pins (fpout, bnum);
       break;

    case EMPTY:
       break;

    default:
       printf("Error in output_netlist.  Unexpected type %d for block"
            "%d.\n", block[bnum].type, bnum);
    }
 }

 fclose (fpout);
}

void print_pins (FILE *fpout, int bnum) {
/* This routine prints out the pinlist for a clb that contains either   *
 * a LUT, a LATCH or both.  It prints the LUT inputs and output in the  *
 * order below so that the net ordering when parsed will be the same as *
 * that of the old combinational blif parsing in vpr.  This allows      *
 * the routing serial numbers to be compared.                           */

 int ipin, column, netnum;
 void print_net_name (int netnum, int *column, FILE *fpout);

 fprintf(fpout,"   pinlist: ");

 column = 14;  /* Next column I will write to. */

/* LUT inputs first. */

 for (ipin=1;ipin<=lut_size;ipin++) {
    netnum = block[bnum].nets[ipin];
    print_net_name (netnum, &column, fpout);
 }

/* Output next.  */

 netnum = block[bnum].nets[0];
 print_net_name (netnum, &column, fpout);

/* Clock last. */

 netnum = block[bnum].nets[lut_size+1];
 print_net_name (netnum, &column, fpout);

 fprintf(fpout,"\n\n");
}


void print_net_name (int netnum, int *column, FILE *fpout) {
/* This routine prints out a net's name.  It limits the length of a *
 * line to LINELENGTH characters by using \ to continue lines.      *
 * netnum is the number of the net to be printed, while column      *
 * points to the current printing column (column is both used and   *
 * updated by this routine).  fpout is the output file pointer.     */

 int len;
 char *str_ptr; 

#define LINELENGTH 80

 if (netnum == OPEN) 
    str_ptr = "open";
 else 
    str_ptr = net[netnum].name;

 len = strlen(str_ptr);
 if (len + 4 > LINELENGTH) {
     printf("Error in print_pins: Net %s has too long a name.\n",
        str_ptr);
     exit(1);
 }

 if (*column + len > LINELENGTH) {
    fprintf(fpout,"\\\n   ");
    *column = 4;
 }
     
 fprintf(fpout,"%s ",str_ptr);
 *column += len + 1; 
}
