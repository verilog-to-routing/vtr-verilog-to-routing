/* Netlist description data structures. */
extern int num_nets, num_blocks;     
extern int num_p_inputs, num_p_outputs;
extern struct s_net *net;
extern struct s_block *block;

/* Number in original netlist, before FF packing. */
extern int num_luts, num_latches; 
