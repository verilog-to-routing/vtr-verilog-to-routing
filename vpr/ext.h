/* Netlist to be placed stuff. */
extern int num_nets, num_blocks;     
extern int num_p_inputs, num_p_outputs, num_clbs, num_globals;
extern struct s_net *net;
extern struct s_block *block;
extern int *is_global;
extern int **net_pin_class;

/* Physical FPGA architecture stuff */
extern int nx, ny, io_rat, pins_per_clb;
extern float chan_width_io;
extern struct s_chan chan_x_dist, chan_y_dist;   /* functional form */
extern int **pinloc;
extern int *clb_pin_class;
extern struct s_class *class_inf;
extern int num_class;

/* chan_width_x is the x-directed channel; i.e. between rows */
extern int *chan_width_x, *chan_width_y; /* numerical form */
extern struct s_clb **clb;

/* Contents of blocks in the netlist (used only for timing analysis) */
extern struct s_subblock **subblock_inf;
extern int *num_subblocks_per_block;
extern int max_subblocks_per_block;
extern int subblock_lut_size;

/* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
extern struct s_trace **trace_head, **trace_tail;   

/* Structures to define the routing architecture of the FPGA.           */
extern int num_rr_nodes;
extern struct s_rr_node *rr_node;                    /* [0..num_rr_nodes-1] */
extern struct s_rr_node_cost_inf *rr_node_cost_inf;  /* [0..num_rr_nodes-1] */
extern struct s_rr_node_draw_inf *rr_node_draw_inf;  /* [0..num_rr_nodes-1] */
extern int **net_rr_terminals;            /* [0..num_nets-1][0..num_pins-1] */
