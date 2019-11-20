/* 
Global variables 

Key global variables that are used everywhere are: 
clb_net, vpack_net, block, and logical_block
These variables represent the user netlist in various stages of the CAD flow
*/

#ifndef GLOBALS_H
#define GLOBALS_H

/********************************************************************
User Netlist Globals
********************************************************************/

/* external-to-complex block nets in the user netlist */
extern int num_nets;
extern struct s_net *clb_net;

/* blocks in the user netlist */
extern int num_blocks;
extern struct s_block *block;


/********************************************************************
Physical FPGA architecture globals 
*********************************************************************/

/* x and y dimensions of the FPGA itself, the core of the FPGA is from [1..nx][1..ny], the I/Os form a perimeter surrounding the core */
extern int nx, ny;
extern struct s_grid_tile **grid; /* FPGA complex blocks grid [0..nx+1][0..ny+1] */

/* Special pointers to identify special blocks on an FPGA: I/Os, unused, and default */
extern t_type_ptr IO_TYPE;
extern t_type_ptr EMPTY_TYPE;
extern t_type_ptr FILL_TYPE;

/* type_descriptors are blocks that can be moved by the placer
   such as: I/Os, CLBs, memories, multipliers, etc
   Different types of physical block are contained in type descriptors
*/
extern int num_types;
extern struct s_type_descriptor *type_descriptors;

/* Default prefix string for output files */
extern char *OutFilePrefix;

/* name of the blif circuit */
extern char *blif_circuit_name;

/* Default area of a 1x1 logic tile (excludes routing) on the FPGA */
extern float grid_logic_tile_area;

/* Area of a mux transistor for the input connection block */
extern float ipin_mux_trans_size;

/*******************************************************************
Packing related globals
********************************************************************/

/* Netlist description data structures. */

/* User netlist information */
extern int num_logical_nets, num_logical_blocks, num_saved_logical_blocks, num_saved_logical_nets;
extern int num_p_inputs, num_p_outputs;
extern struct s_net *vpack_net, *saved_logical_nets;
extern struct s_logical_block *logical_block, *saved_logical_blocks;
extern struct s_subckt *subckt;

/* primiary inputs removed from circuit */
extern struct s_linked_vptr *circuit_p_io_removed;

/* Relationship between external-to-complex block nets and internal-to-complex block nets */
extern int *clb_to_vpack_net_mapping; /* [0..num_clb_nets - 1] */
extern int *vpack_to_clb_net_mapping; /* [0..num_vpack_nets - 1] */

/* Number in original netlist, before FF packing. */
extern int num_luts, num_latches, num_subckts; 

/*******************************************************************
Routing related globals
********************************************************************/

/* chan_width_x is the x-directed channel; i.e. between rows */
extern int *chan_width_x, *chan_width_y;	/* numerical form */

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


/*******************************************************************
Timing related globals
********************************************************************/
extern t_tnode *tnode;			/* [0..num_tnodes - 1] */
extern int num_tnodes;			/* Number of nodes (pins) in the timing graph */
extern float pb_max_internal_delay;  /* biggest internal delay of block */
extern const t_pb_type *pbtype_max_internal_delay;  /* block type with highest internal delay */


#endif

