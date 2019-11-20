#include <stdio.h>
extern char blif_file[], output_file[];
extern FILE *blif, *output;
extern int lut_size;

extern char model[];

/* Netlist to be placed stuff. */
extern num_nets, num_blocks;     
extern num_p_inputs, num_p_outputs, num_luts, num_latches;
extern struct s_net *net;
extern struct s_block *block;
