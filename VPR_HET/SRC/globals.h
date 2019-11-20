extern boolean WMF_DEBUG;
extern char *OutFilePrefix;

extern float grid_logic_tile_area;
extern float ipin_mux_trans_size;

extern int num_nets;
extern struct s_net *net;

extern int num_blocks;
extern struct s_block *block;

/* Physical FPGA architecture stuff */
extern int nx, ny;

/* chan_width_x is the x-directed channel; i.e. between rows */
extern int *chan_width_x, *chan_width_y;	/* numerical form */
extern struct s_grid_tile **grid;

/* [0..num_nets-1] of linked list start pointers.  Defines the routing.  */
extern struct s_trace **trace_head, **trace_tail;

/* Structures to define the routing architecture of the FPGA.           */
extern int num_rr_nodes;
extern t_rr_node *rr_node;	/* [0..num_rr_nodes-1]          */
extern int num_rr_indexed_data;
extern t_rr_indexed_data *rr_indexed_data;	/* [0 .. num_rr_indexed_data-1] */
extern t_ivec ***rr_node_indices;
extern int **net_rr_terminals;	/* [0..num_nets-1][0..num_pins-1] */
extern struct s_switch_inf *switch_inf;	/* [0..det_routing_arch.num_switch-1] */
extern int **rr_blk_source;	/* [0..num_blocks-1][0..num_class-1] */

/* This identifies the t_type_ptr of an IO block */
extern t_type_ptr IO_TYPE;

/* This identifies the t_type_ptr of an Empty block */
extern t_type_ptr EMPTY_TYPE;

/* This identifies the t_type_ptr of the default logic block */
extern t_type_ptr FILL_TYPE;

/* Total number of type_descriptors */
extern int num_types;
extern struct s_type_descriptor *type_descriptors;
