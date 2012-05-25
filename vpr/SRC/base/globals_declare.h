/* Netlist to be placed stuff. */
int num_nets, num_blocks;
struct s_net *net;
struct s_block *block;
boolean *is_global;

/* Physical FPGA architecture stuff */
int nx, ny;

/* chan_width_x is the x-directed channel; i.e. between rows */
int *chan_width_x, *chan_width_y; /* numerical form */
struct s_grid_tile **grid;

/* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
struct s_trace **trace_head, **trace_tail;

/* Structures to define the routing architecture of the FPGA.           */
int num_rr_nodes;
t_rr_node *rr_node; /* [0..num_rr_nodes-1]          */
t_ivec ***rr_node_indices;
int num_rr_indexed_data;
t_rr_indexed_data *rr_indexed_data; /* [0 .. num_rr_indexed_data-1] */
int **net_rr_terminals; /* [0..num_nets-1][0..num_pins-1] */
struct s_switch_inf *switch_inf; /* [0..det_routing_arch.num_switch-1] */
int **rr_blk_source; /* [0..num_blocks-1][0..num_class-1] */
