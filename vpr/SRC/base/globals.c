/* Define global variables here */

#include "vpr_types.h"
#include "globals.h"


/******** General global variables ********/
int Fs_seed = -1;

int W_seed = -1;
int binary_search = -1;

float grid_logic_tile_area = 0;
float ipin_mux_trans_size = 0;

/* User netlist information begin  */
int num_logical_nets = 0, num_logical_blocks = 0;
int num_p_inputs = 0, num_p_outputs = 0;
struct s_net *vpack_net = NULL;
struct s_logical_block *logical_block = NULL;
char *blif_circuit_name = NULL;
char *default_output_name = NULL;
/* User netlist information end  */

/******** Clustered netlist to be mapped stuff ********/

int num_nets = 0;
struct s_net *clb_net = NULL;

int num_blocks = 0;
struct s_block *block = NULL;

int *clb_to_vpack_net_mapping = NULL; /* [0..num_clb_nets - 1] */
int *vpack_to_clb_net_mapping = NULL; /* [0..num_vpack_nets - 1] */

/* This identifies the t_type_ptr of an IO block */
int num_types = 0;
struct s_type_descriptor *type_descriptors = NULL;

t_type_ptr IO_TYPE = NULL;
t_type_ptr EMPTY_TYPE = NULL;
t_type_ptr FILL_TYPE = NULL;

/******** Physical architecture stuff ********/

int nx = 0;
int ny = 0;

/* TRUE if this is a global clb pin -- an input pin to which the netlist can *
 * connect global signals, but which does not connect into the normal        *
 * routing via muxes etc.  Marking pins like this (only clocks in my work)   *
 * stops them from screwing up the input switch pattern in the rr_graph      *
 * generator and from creating extra switches that the area model would      *
 * count.                                                                    */

int *chan_width_x = NULL; /* [0..ny] */
int *chan_width_y = NULL; /* [0..nx] */

struct s_grid_tile **grid = NULL; /* [0..(nx+1)][0..(ny+1)] Physical block list */

/******** Structures defining the routing ********/

/* Linked list start pointers.  Define the routing. */
struct s_trace **trace_head = NULL; /* [0..(num_nets-1)] */
struct s_trace **trace_tail = NULL; /* [0..(num_nets-1)] */

/******** Structures defining the FPGA routing architecture ********/

int num_rr_nodes = 0;
t_rr_node *rr_node = NULL; /* [0..(num_rr_nodes-1)] */
t_ivec ***rr_node_indices = NULL;

int num_rr_indexed_data = 0;
t_rr_indexed_data *rr_indexed_data = NULL; /* [0..(num_rr_indexed_data-1)] */

/* Gives the rr_node indices of net terminals. */

int **net_rr_terminals = NULL; /* [0..num_nets-1][0..num_pins-1] */

/* Gives information about all the switch types                      *
 * (part of routing architecture, but loaded in read_arch.c          */

struct s_switch_inf *switch_inf = NULL; /* [0..(det_routing_arch.num_switch-1)] */

/* Stores the SOURCE and SINK nodes of all CLBs (not valid for pads).     */

int **rr_blk_source = NULL; /* [0..(num_blocks-1)][0..(num_class-1)] */

/* primiary inputs removed from circuit */
struct s_linked_vptr *circuit_p_io_removed = NULL;

/********** Structures representing timing graph information */
float pb_max_internal_delay = UNDEFINED; /* biggest internal delay of physical block */
const t_pb_type *pbtype_max_internal_delay = NULL; /* physical block type with highest internal delay */

/********** Structures representing the global clock network */
t_clock_arch * g_clock_arch;
