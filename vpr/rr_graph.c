#include <stdio.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include "check_rr_graph.h"
#include "rr_graph_timing_params.h"
#include "rr_graph_indexed_data.h"
#include <assert.h>


/******************* Variables local to this module. ***********************/

static struct s_linked_vptr *rr_mem_chunk_list_head = NULL;   
/* Used to free "chunked" memory.  If NULL, no rr_graph exists right now.  */

static int chunk_bytes_avail = 0;
static char *chunk_next_avail_mem = NULL;
/* Status of current chunk being dished out by calls to my_chunk_malloc.   */



/********************* Subroutines local to this module. *******************/

static int ***alloc_and_load_clb_pin_to_tracks (enum e_pin_type pin_type,
         int nodes_per_chan, int Fc, boolean perturb_switch_pattern); 

static void load_uniform_switch_pattern (int ***tracks_connected_to_pin, 
         int num_phys_pins, int *pin_num_ordering, int *side_ordering, 
         int nodes_per_chan, int Fc, float step_size);

static void load_perturbed_switch_pattern (int ***tracks_connected_to_pin, 
         int num_phys_pins, int *pin_num_ordering, int *side_ordering, 
         int nodes_per_chan, int Fc, float step_size);

static void check_all_tracks_reach_pins (int ***tracks_connected_to_pin,
         int nodes_per_chan, int Fc, enum e_pin_type ipin_or_opin);

static struct s_ivec **alloc_and_load_tracks_to_clb_ipin (int nodes_per_chan,
         int Fc, int ***clb_ipin_to_tracks); 

static int **alloc_and_load_pads_to_tracks (int nodes_per_chan, int Fc_pad); 

static struct s_ivec *alloc_and_load_tracks_to_pads (int **pads_to_tracks, 
         int nodes_per_chan, int Fc_pad); 

static int track_side (int clb_side); 

static void alloc_and_load_rr_graph (int **rr_node_indices, 
       int ***clb_opin_to_tracks, struct s_ivec **tracks_to_clb_ipin,
       int **pads_to_tracks, struct s_ivec *tracks_to_pads, int Fc_output,
       int Fc_input, int Fc_pad, int nodes_per_chan, enum e_route_type 
       route_type, struct s_det_routing_arch det_routing_arch,
       t_seg_details *seg_details_x, t_seg_details *seg_details_y); 

static void build_rr_clb (int **rr_node_indices, int Fc_output, int ***
         clb_opin_to_tracks, int nodes_per_chan, int i, int j,
         int delayless_switch, t_seg_details *seg_details_x, 
         t_seg_details *seg_details_y);

static void build_rr_pads (int **rr_node_indices, int Fc_pad, int 
          **pads_to_tracks, int nodes_per_chan, int i, int j, int 
          delayless_switch, t_seg_details *seg_details_x, t_seg_details 
          *seg_details_y);

static void build_rr_xchan (int **rr_node_indices, enum e_route_type
          route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
          tracks_to_pads, int i, int j, int nodes_per_chan, enum 
          e_switch_block_type switch_block_type, int wire_to_ipin_switch,
          t_seg_details *seg_details_x, t_seg_details *seg_details_y,
          int cost_index_offset); 

static void build_rr_ychan (int **rr_node_indices, enum e_route_type
          route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
          tracks_to_pads, int i, int j, int nodes_per_chan, enum
          e_switch_block_type switch_block_type, int wire_to_ipin_switch,
          t_seg_details *seg_details_x, t_seg_details *seg_details_y,
          int cost_index_offset);

void alloc_and_load_edges_and_switches (int inode, int num_edges, 
          t_linked_edge *edge_list_head);

static void alloc_net_rr_terminals (void);

static void alloc_and_load_rr_clb_source (int **rr_node_indices,
          int nodes_per_chan);

static int which_io_block (int iblk); 


/*define a structure to keep track of the internal variables allocated inside */
/*build_rr_graph. This is required so that we can later free them in          */
/*free_rr_graph_internals. If this seems like a hack, it is. The original code*/
/*was able to do everything within one procedure, however since the timing    */
/*driven placer needs to continuously change the mappping of the nets to the  */
/*rr terminals, we need to keep the internal structures active until we finish*/
/*placement -Sandy Nov 10/98*/


struct s_rr_graph_internal_vars {
  int nodes_per_chan;
  int **rr_node_indices;
  int **pads_to_tracks;
  struct s_ivec **tracks_to_clb_ipin;
  struct s_ivec *tracks_to_pads;
  int ***clb_opin_to_tracks;
  int ***clb_ipin_to_tracks;
  t_seg_details *seg_details_x;
  t_seg_details *seg_details_y;
};

static struct s_rr_graph_internal_vars rr_graph_internal_vars;

/******************* Subroutine definitions *******************************/
int **get_rr_node_indices(){

  return (rr_graph_internal_vars.rr_node_indices);
}

int get_nodes_per_chan() {
  return (rr_graph_internal_vars.nodes_per_chan);
}

void build_rr_graph (enum e_route_type route_type, struct s_det_routing_arch
         det_routing_arch, t_segment_inf *segment_inf, t_timing_inf 
         timing_inf, enum e_base_cost_type base_cost_type) {

/* Builds the routing resource graph needed for routing.  Does all the   *
 * necessary allocation and initialization.  If route_type is DETAILED   *
 * then the routing graph built is for detailed routing, otherwise it is *
 * for GLOBAL routing.                                                   */

 int nodes_per_clb, nodes_per_pad, nodes_per_chan;
 float Fc_ratio;
 boolean perturb_ipin_switch_pattern;
 int **rr_node_indices;
 int Fc_output, Fc_input, Fc_pad;
 int **pads_to_tracks;
 struct s_ivec **tracks_to_clb_ipin, *tracks_to_pads;

/* [0..pins_per_clb-1][0..3][0..Fc_output-1].  List of tracks this pin *
 * connects to.  Second index is the side number (see pr.h).  If a pin *
 * is not an output or input, respectively, or doesn't connect to      *
 * anything on this side, the [ipin][iside][0] element is OPEN.        */

 int ***clb_opin_to_tracks, ***clb_ipin_to_tracks;
 t_seg_details *seg_details_x, *seg_details_y;  /* [0 .. nodes_per_chan-1] */


 nodes_per_clb = num_class + pins_per_clb;
 nodes_per_pad = 4 * io_rat;    /* SOURCE, SINK, OPIN, and IPIN */

 if (route_type == GLOBAL) {
    nodes_per_chan = 1;
 }
 else {
    nodes_per_chan = chan_width_x[0];
 }

 seg_details_x = alloc_and_load_seg_details (nodes_per_chan, segment_inf, 
                 det_routing_arch.num_segment, nx);

 seg_details_y = alloc_and_load_seg_details (nodes_per_chan, segment_inf, 
                 det_routing_arch.num_segment, ny);

#ifdef DEBUG
 dump_seg_details (seg_details_x, nodes_per_chan, "x.echo");
 dump_seg_details (seg_details_y, nodes_per_chan, "y.echo");
#endif

/* To set up the routing graph I need to choose an order for the rr_indices. *
 * For each (i,j) slot in the FPGA, the index order is [source+sink]/pins/   *
 * chanx/chany. The dummy source and sink are ordered by class number or pad *
 * number; the pins are ordered by pin number or pad number, and the channel *
 * segments are ordered by track number.  Track 1 is closest to the "owning" *
 * block; i.e. it is towards the left of y-chans and the bottom of x-chans.  *
 *                                                                           *
 * The maximize cache effectiveness, I put all the rr_nodes associated with  *
 * a block together (this includes the "owned" channel segments).  I start   *
 * at (0,0) (empty), go to (1,0), (2,0) ... (0,1) and so on.                 */

/* NB:  Allocates data structures for fast index computations -- more than *
 * just rr_node_indices are allocated by this routine.                     */

 rr_node_indices = alloc_and_load_rr_node_indices (nodes_per_clb, 
         nodes_per_pad, nodes_per_chan, seg_details_x, seg_details_y);

 num_rr_nodes = rr_node_indices[nx+1][ny+1];

 if (det_routing_arch.Fc_type == ABSOLUTE) {
    Fc_output = min (det_routing_arch.Fc_output, nodes_per_chan);
    Fc_input = min (det_routing_arch.Fc_input, nodes_per_chan);
    Fc_pad = min (det_routing_arch.Fc_pad, nodes_per_chan);
 }
 else {    /* FRACTIONAL */
/* Make Fc_output round up so all tracks are driven when                    *
 * Fc = W/(Nequivalent outputs), regardless of W.                           */
    Fc_output = ceil (nodes_per_chan * det_routing_arch.Fc_output - 0.005);
/*  Fc_output = nint (nodes_per_chan * det_routing_arch.Fc_output); */
    Fc_output = max (1, Fc_output);
    Fc_input = nint (nodes_per_chan * det_routing_arch.Fc_input);
    Fc_input = max (1, Fc_input);
    Fc_pad = nint (nodes_per_chan * det_routing_arch.Fc_pad);
    Fc_pad = max (1, Fc_pad);
 }

 alloc_and_load_switch_block_conn (nodes_per_chan, 
        det_routing_arch.switch_block_type);

 clb_opin_to_tracks = alloc_and_load_clb_pin_to_tracks (DRIVER, nodes_per_chan,
        Fc_output, FALSE);

/* Perturb ipin switch pattern if it will line up perfectly with the output  *
 * pin pattern, and there are not so many switches that this would cause 1   *
 * track to connect to the same pin twice.                                   */

 if (Fc_input > Fc_output) 
    Fc_ratio = (float) Fc_input / (float) Fc_output;
 else 
    Fc_ratio = (float) Fc_output / (float) Fc_input;

 if (Fc_input <= nodes_per_chan - 2 && fabs (Fc_ratio - nint (Fc_ratio)) < 
      .5 / (float) nodes_per_chan)
    perturb_ipin_switch_pattern = TRUE;
 else
    perturb_ipin_switch_pattern = FALSE;

 clb_ipin_to_tracks = alloc_and_load_clb_pin_to_tracks (RECEIVER,
        nodes_per_chan, Fc_input, perturb_ipin_switch_pattern);

 tracks_to_clb_ipin = alloc_and_load_tracks_to_clb_ipin (nodes_per_chan,
        Fc_input, clb_ipin_to_tracks);

 pads_to_tracks = alloc_and_load_pads_to_tracks (nodes_per_chan, Fc_pad);
 
 tracks_to_pads = alloc_and_load_tracks_to_pads (pads_to_tracks, 
          nodes_per_chan, Fc_pad);

 rr_edge_done = (boolean *) my_calloc (num_rr_nodes, sizeof (boolean));

 alloc_and_load_rr_graph (rr_node_indices, clb_opin_to_tracks,
       tracks_to_clb_ipin, pads_to_tracks, tracks_to_pads, Fc_output,
       Fc_input, Fc_pad, nodes_per_chan, route_type, det_routing_arch, 
       seg_details_x, seg_details_y);

 add_rr_graph_C_from_switches (timing_inf.C_ipin_cblock); 

 alloc_and_load_rr_indexed_data (segment_inf, det_routing_arch.num_segment,
        rr_node_indices, nodes_per_chan, det_routing_arch.wire_to_ipin_switch,
        base_cost_type);
        
/* dump_rr_graph ("rr_graph.echo");  */
 check_rr_graph (route_type, det_routing_arch.num_switch);

 rr_graph_internal_vars.clb_opin_to_tracks = clb_opin_to_tracks;
 rr_graph_internal_vars.clb_ipin_to_tracks = clb_ipin_to_tracks;
 rr_graph_internal_vars.tracks_to_clb_ipin = tracks_to_clb_ipin;
 rr_graph_internal_vars.tracks_to_pads = tracks_to_pads;
 rr_graph_internal_vars.pads_to_tracks = pads_to_tracks;
 rr_graph_internal_vars.nodes_per_chan = nodes_per_chan;
 rr_graph_internal_vars.seg_details_x = seg_details_x;
 rr_graph_internal_vars.seg_details_y = seg_details_y;
 rr_graph_internal_vars.rr_node_indices = rr_node_indices;
}


void free_rr_graph_internals (enum e_route_type route_type, struct s_det_routing_arch
         det_routing_arch, t_segment_inf *segment_inf, t_timing_inf 
         timing_inf, enum e_base_cost_type base_cost_type)
{
  /* frees the variables that are internal to build_rr_graph.        */
  /* this procedure should typically be called immediatley after     */
  /* build_rr_graph, except for in the timing driven placer, which   */
  /* is constantly modifying some of the rr_graph structures         */

 int nodes_per_chan;
 int **rr_node_indices;
 int **pads_to_tracks;
 struct s_ivec **tracks_to_clb_ipin, *tracks_to_pads;
 int ***clb_opin_to_tracks, ***clb_ipin_to_tracks;
 t_seg_details *seg_details_x, *seg_details_y;  /* [0 .. nodes_per_chan-1] */


 clb_opin_to_tracks = rr_graph_internal_vars.clb_opin_to_tracks;
 clb_ipin_to_tracks = rr_graph_internal_vars.clb_ipin_to_tracks;
 tracks_to_clb_ipin = rr_graph_internal_vars.tracks_to_clb_ipin;
 tracks_to_pads = rr_graph_internal_vars.tracks_to_pads;
 pads_to_tracks = rr_graph_internal_vars.pads_to_tracks;
 nodes_per_chan = rr_graph_internal_vars.nodes_per_chan;
 seg_details_x = rr_graph_internal_vars.seg_details_x;
 seg_details_y = rr_graph_internal_vars.seg_details_y;
 rr_node_indices = rr_graph_internal_vars.rr_node_indices;


 free (rr_edge_done);
 free_matrix3 (clb_opin_to_tracks, 0, pins_per_clb-1, 0, 3, 0, sizeof(int));
 free_matrix3 (clb_ipin_to_tracks, 0, pins_per_clb-1, 0, 3, 0, sizeof(int));
 free_ivec_matrix (tracks_to_clb_ipin, 0, nodes_per_chan-1, 0, 3);
 free_ivec_vector (tracks_to_pads, 0, nodes_per_chan-1);
 free_matrix (pads_to_tracks, 0, io_rat-1, 0, sizeof(int));
 free_switch_block_conn (nodes_per_chan);
 free_edge_list_hard (&free_edge_list_head);
 free_seg_details (seg_details_x, nodes_per_chan);
 free_seg_details (seg_details_y, nodes_per_chan);

 free_rr_node_indices (rr_node_indices);
}


static void alloc_and_load_rr_graph (int **rr_node_indices, 
       int ***clb_opin_to_tracks, struct s_ivec **tracks_to_clb_ipin,
       int **pads_to_tracks, struct s_ivec *tracks_to_pads, int Fc_output,
       int Fc_input, int Fc_pad, int nodes_per_chan, enum e_route_type 
       route_type, struct s_det_routing_arch det_routing_arch, 
       t_seg_details *seg_details_x, t_seg_details *seg_details_y) {

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */

 int i, j;
 enum e_switch_block_type switch_block_type;
 int delayless_switch, wire_to_ipin_switch, num_segment;


/* Allocate storage for the graph nodes.  Storage for the edges will be *
 * allocated as I fill in the graph.                                    */

 if (rr_mem_chunk_list_head != NULL) {
    printf("Error in alloc_and_load_rr_graph:  rr_mem_chunk_list_head = %p.\n",
            rr_mem_chunk_list_head);
    printf("Expected NULL.  It appears an old rr_graph has not been freed.\n");
    exit (1);
 }

 alloc_net_rr_terminals ();
 load_net_rr_terminals (rr_node_indices, nodes_per_chan);

 alloc_and_load_rr_clb_source (rr_node_indices, nodes_per_chan);

 rr_node = (t_rr_node *) my_malloc (num_rr_nodes * sizeof (t_rr_node));
 
 switch_block_type = det_routing_arch.switch_block_type;
 delayless_switch = det_routing_arch.delayless_switch;
 wire_to_ipin_switch = det_routing_arch.wire_to_ipin_switch;
 num_segment = det_routing_arch.num_segment;

 for (i=0;i<=nx+1;i++) {
    for (j=0;j<=ny+1;j++) {

       if (clb[i][j].type == CLB) {
          build_rr_clb (rr_node_indices, Fc_output, clb_opin_to_tracks, 
                 nodes_per_chan, i, j, delayless_switch, seg_details_x, 
                 seg_details_y);
          build_rr_xchan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type, 
                 wire_to_ipin_switch, seg_details_x, seg_details_y,
                 CHANX_COST_INDEX_START);
          build_rr_ychan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type, 
                 wire_to_ipin_switch, seg_details_x, seg_details_y,
                 CHANX_COST_INDEX_START + num_segment);
       }

       else if (clb[i][j].type == IO) {
          build_rr_pads (rr_node_indices, Fc_pad, pads_to_tracks,
                  nodes_per_chan, i, j, delayless_switch, seg_details_x, 
                  seg_details_y);

          if (j == 0)  /* Bottom-most channel. */
             build_rr_xchan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                    tracks_to_pads, i, j, nodes_per_chan, switch_block_type, 
                    wire_to_ipin_switch, seg_details_x, seg_details_y,
                    CHANX_COST_INDEX_START);

          if (i == 0)  /* Left-most channel.   */
             build_rr_ychan (rr_node_indices, route_type, tracks_to_clb_ipin,
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type, 
                 wire_to_ipin_switch, seg_details_x, seg_details_y, 
                 CHANX_COST_INDEX_START + num_segment);
       }

       else if (clb[i][j].type != ILLEGAL) {
          printf ("Error in alloc_and_load_rr_graph.\n"
                  "Block at (%d, %d) has unknown type (%d).\n", i, j, 
                   clb[i][j].type);
          exit (1);
       }

    }
 }

}


void free_rr_graph (void) {

/* Frees all the routing graph data structures, if they have been       *
 * allocated.  I use rr_mem_chunk_list_head as a flag to indicate       *
 * whether or not the graph has been allocated -- if it is not NULL,    *
 * a routing graph exists and can be freed.  Hence, you can call this   *
 * routine even if you're not sure of whether a rr_graph exists or not. */

 if (rr_mem_chunk_list_head == NULL)   /* Nothing to free. */
    return;

 free_chunk_memory (rr_mem_chunk_list_head);  /* Frees ALL "chunked" data */
 rr_mem_chunk_list_head = NULL;               /* No chunks allocated now. */
 chunk_bytes_avail = 0;             /* 0 bytes left in current "chunk". */
 chunk_next_avail_mem = NULL;       /* No current chunk.                */

/* Before adding any more free calls here, be sure the data is NOT chunk *
 * allocated, as ALL the chunk allocated data is already free!           */

 free (net_rr_terminals);
 free (rr_node);
 free (rr_indexed_data);
 free_matrix (rr_clb_source, 0, num_blocks-1, 0, sizeof(int));

 net_rr_terminals = NULL;
 rr_node = NULL;
 rr_indexed_data = NULL;
 rr_clb_source = NULL;
}


static void alloc_net_rr_terminals (void) {

  int inet;

 net_rr_terminals = (int **) my_malloc (num_nets * sizeof(int *));

 for (inet=0;inet<num_nets;inet++) {
    net_rr_terminals[inet] = (int *) my_chunk_malloc (net[inet].num_pins * 
              sizeof (int), &rr_mem_chunk_list_head, &chunk_bytes_avail,
	     &chunk_next_avail_mem);
 }

}


void load_net_rr_terminals (int **rr_node_indices, 
        int nodes_per_chan) {

/* Allocates and loads the net_rr_terminals data structure.  For each net   *
 * it stores the rr_node index of the SOURCE of the net and all the SINKs   *
 * of the net.  [0..num_nets-1][0..num_pins-1].  Entry [inet][pnum] stores  *
 * the rr index corresponding to the SOURCE (opin) or SINK (ipin) of pnum.  */

 int inet, ipin, inode, iblk, i, j, blk_pin, iclass;
 t_rr_type rr_type;


 for (inet=0;inet<num_nets;inet++) {
    
    rr_type = SOURCE;     /* First pin only */
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       iblk = net[inet].blocks[ipin];
       i = block[iblk].x;
       j = block[iblk].y;

       if (clb[i][j].type == CLB) {
          blk_pin = net[inet].blk_pin[ipin];
          iclass = clb_pin_class[blk_pin];
       }
       else {
          iclass = which_io_block (iblk);
       }

       inode = get_rr_node_index (i, j, rr_type, iclass, nodes_per_chan, 
                  rr_node_indices);
       net_rr_terminals[inet][ipin] = inode;
 
       rr_type = SINK;    /* All pins after first are SINKs. */
    }
 }
}

static void alloc_and_load_rr_clb_source (int **rr_node_indices,
          int nodes_per_chan) {

/* Saves the rr_node corresponding to each SOURCE and SINK in each CLB      *
 * in the FPGA.  Currently only the SOURCE rr_node values are used, and     *
 * they are used only to reserve pins for locally used OPINs in the router. *
 * [0..num_blocks-1][0..num_class-1].  The values for blocks that are pads  *
 * are NOT valid.                                                           */

 int iblk, i, j, iclass, inode;
 t_rr_type rr_type;

 rr_clb_source = (int **) alloc_matrix (0, num_blocks-1, 0, num_class-1,
                  sizeof(int));

 for (iblk=0;iblk<num_blocks;iblk++) {
    for (iclass=0;iclass<num_class;iclass++) {
       if (block[iblk].type == CLB) {
          i = block[iblk].x;
          j = block[iblk].y;

          if (class_inf[iclass].type == DRIVER)
             rr_type = SOURCE;
          else
             rr_type = SINK;
 
          inode = get_rr_node_index (i, j, rr_type, iclass, nodes_per_chan,
               rr_node_indices);
          rr_clb_source[iblk][iclass] = inode;
       }
       else {  /* IO Pad; don't need any data so set to OPEN (invalid) */
          rr_clb_source[iblk][iclass] = OPEN;
       }
    }
 }
}



static int which_io_block (int iblk) {

/* Returns the subblock (pad) number at which this block was placed.  iblk *
 * must be an IO block.                                                    */

 int i, j, ipad, ifound, test_blk;

 ifound = -1;
 i = block[iblk].x;
 j = block[iblk].y;
 

 if (block[iblk].type != INPAD && block[iblk].type != OUTPAD) {
    printf ("Error in which_io_block:  block %d is not an IO block.\n", iblk);
    exit (1);
 }

 for (ipad=0;ipad<clb[i][j].occ;ipad++) {
    test_blk = clb[i][j].u.io_blocks[ipad];
    if (test_blk == iblk) {
       ifound = ipad;
       break;
    }
 }

 if (ifound < 0) {
    printf ("Error in which_io_block:  block %d not found in clb array.\n",
             iblk);
    exit (1);
 }

 return (ifound);
}


static void build_rr_clb (int **rr_node_indices, int Fc_output, int ***
       clb_opin_to_tracks, int nodes_per_chan, int i, int j, int 
       delayless_switch, t_seg_details *seg_details_x, t_seg_details 
       *seg_details_y) {

/* Load up the rr_node structures for the clb at location (i,j).  I both  *
 * fill in fields that shouldn't change during the entire routing and     *
 * initialize fields that will change to the proper starting value.       */

 int ipin, iclass, inode, pin_num, to_node, num_edges;
 t_linked_edge *edge_list_head;

/* SOURCES and SINKS first.   */

 for (iclass=0;iclass<num_class;iclass++) {
    if (class_inf[iclass].type == DRIVER) {    /* SOURCE */
       inode = get_rr_node_index (i, j, SOURCE, iclass, nodes_per_chan,
                 rr_node_indices);

       num_edges = class_inf[iclass].num_pins;
       rr_node[inode].num_edges = num_edges;
       rr_node[inode].edges = (int *) my_chunk_malloc (num_edges * 
              sizeof (int), &rr_mem_chunk_list_head, &chunk_bytes_avail, 
              &chunk_next_avail_mem);

       rr_node[inode].switches = (short *) my_chunk_malloc (num_edges *
              sizeof (short), &rr_mem_chunk_list_head, &chunk_bytes_avail,
              &chunk_next_avail_mem);

       for (ipin=0;ipin<class_inf[iclass].num_pins;ipin++) {
          pin_num = class_inf[iclass].pinlist[ipin];
          to_node = get_rr_node_index (i, j, OPIN, pin_num, nodes_per_chan,
                  rr_node_indices);
          rr_node[inode].edges[ipin] = to_node;
          rr_node[inode].switches[ipin] = delayless_switch;
       }

       rr_node[inode].capacity = class_inf[iclass].num_pins;
       rr_node[inode].cost_index = SOURCE_COST_INDEX;
       rr_node[inode].type = SOURCE;
    }

    else {    /* SINK */
       inode = get_rr_node_index (i, j, SINK, iclass, nodes_per_chan,
                 rr_node_indices);

/* Note:  To allow route throughs through clbs, change the lines below to  *
 * make an edge from the input SINK to the output SOURCE.  Do for just the *
 * special case of INPUTS = class 0 and OUTPUTS = class 1 and see what it  *
 * leads to.  If route throughs are allowed, you may want to increase the  *
 * base cost of OPINs and/or SOURCES so they aren't used excessively.      */

       rr_node[inode].num_edges = 0; 
       rr_node[inode].edges = NULL;
       rr_node[inode].switches = NULL;
       rr_node[inode].capacity = class_inf[iclass].num_pins;
       rr_node[inode].cost_index = SINK_COST_INDEX;
       rr_node[inode].type = SINK;
    }

/* Things common to both SOURCEs and SINKs.   */

    rr_node[inode].occ = 0;

    rr_node[inode].xlow = i;
    rr_node[inode].xhigh = i;
    rr_node[inode].ylow = j;
    rr_node[inode].yhigh = j;
    rr_node[inode].R = 0;
    rr_node[inode].C = 0;

    rr_node[inode].ptc_num = iclass;
 }

/* Now do the pins.  */

 for (ipin=0;ipin<pins_per_clb;ipin++) {
    iclass = clb_pin_class[ipin];
    if (class_inf[iclass].type == DRIVER) {  /* OPIN */
       inode = get_rr_node_index (i, j, OPIN, ipin, nodes_per_chan,
                 rr_node_indices);

       edge_list_head = NULL;
       num_edges = get_clb_opin_connections (clb_opin_to_tracks, ipin, i, j,
                   Fc_output, seg_details_x, seg_details_y, &edge_list_head, 
                   nodes_per_chan, rr_node_indices);

       alloc_and_load_edges_and_switches (inode, num_edges, edge_list_head);

       rr_node[inode].cost_index = OPIN_COST_INDEX;
       rr_node[inode].type = OPIN;
    }
    else {                                   /* IPIN */
       inode = get_rr_node_index (i, j, IPIN, ipin, nodes_per_chan,
                 rr_node_indices);

       rr_node[inode].num_edges = 1;
       rr_node[inode].edges = (int *) my_chunk_malloc (sizeof(int), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);

       rr_node[inode].switches = (short *) my_chunk_malloc (sizeof(short), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);

       to_node = get_rr_node_index (i, j, SINK, iclass, nodes_per_chan,
                  rr_node_indices);
       rr_node[inode].edges[0] = to_node;
       rr_node[inode].switches[0] = delayless_switch;

       rr_node[inode].cost_index = IPIN_COST_INDEX;
       rr_node[inode].type = IPIN;
    }

/* Things that are common to both OPINs and IPINs.  */

    rr_node[inode].capacity = 1;
    rr_node[inode].occ = 0;
 
    rr_node[inode].xlow = i;
    rr_node[inode].xhigh = i;
    rr_node[inode].ylow = j;
    rr_node[inode].yhigh = j;
    rr_node[inode].C = 0;
    rr_node[inode].R = 0;
 
    rr_node[inode].ptc_num = ipin;
 }
}


static void build_rr_pads (int **rr_node_indices, int Fc_pad, int 
        **pads_to_tracks, int nodes_per_chan, int i, int j, int 
        delayless_switch, t_seg_details *seg_details_x, t_seg_details 
        *seg_details_y) {

/* Load up the rr_node structures for the pads at location (i,j).  I both *
 * fill in fields that shouldn't change during the entire routing and     *
 * initialize fields that will change to the proper starting value.       *
 * Empty pad locations have their type set to EMPTY.                      */

 int s_node, p_node, ipad, iloop;
 int inode, num_edges;
 t_linked_edge *edge_list_head;


/* Each pad contains both a SOURCE + OPIN, and a SINK + IPIN, since each  *
 * pad is bidirectional.  The fact that it will only be used as either an *
 * input or an output makes no difference.                                */

 for (ipad=0;ipad<io_rat;ipad++) {   /* For each pad at this (i,j) */

   /* Do SOURCE first.   */

    s_node = get_rr_node_index (i, j, SOURCE, ipad, nodes_per_chan,
             rr_node_indices);
       
    rr_node[s_node].num_edges = 1;
    p_node = get_rr_node_index (i, j, OPIN, ipad, nodes_per_chan, 
              rr_node_indices);

    rr_node[s_node].edges = (int *) my_chunk_malloc (sizeof(int), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    rr_node[s_node].edges[0] = p_node;

    rr_node[s_node].switches = (short *) my_chunk_malloc (sizeof(short),
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    rr_node[s_node].switches[0] = delayless_switch;
    
    rr_node[s_node].cost_index = SOURCE_COST_INDEX;
    rr_node[s_node].type = SOURCE;


   /* Now do OPIN */
       
    edge_list_head = NULL;
    num_edges = get_pad_opin_connections (pads_to_tracks, ipad, i, j, 
              Fc_pad, seg_details_x, seg_details_y, &edge_list_head, 
              nodes_per_chan, rr_node_indices);

    alloc_and_load_edges_and_switches (p_node, num_edges, edge_list_head);
 
    rr_node[p_node].cost_index = OPIN_COST_INDEX;
    rr_node[p_node].type = OPIN;

   /* Code common to both SOURCE and OPIN. */

    inode = s_node;
    for (iloop=1;iloop<=2;iloop++) {    /* for both SOURCE or SINK and pin */
       rr_node[inode].occ = 0;
       rr_node[inode].capacity = 1;

       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;
       rr_node[inode].R = 0.;
       rr_node[inode].C = 0.;

       rr_node[inode].ptc_num = ipad;

       inode = p_node;
    }


   /* Now do SINK */

    s_node = get_rr_node_index (i, j, SINK, ipad, nodes_per_chan,
             rr_node_indices);

    rr_node[s_node].num_edges = 0;
    rr_node[s_node].edges = NULL;
    rr_node[s_node].switches = NULL;

    rr_node[s_node].cost_index = SINK_COST_INDEX;
    rr_node[s_node].type = SINK;


    /* Now do IPIN */
    
    p_node = get_rr_node_index (i, j, IPIN, ipad, nodes_per_chan,
             rr_node_indices);

    rr_node[p_node].num_edges = 1;
    rr_node[p_node].edges = (int *) my_chunk_malloc (sizeof(int), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    rr_node[p_node].edges[0] = s_node;

    rr_node[p_node].switches = (short *) my_chunk_malloc (sizeof(short), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    rr_node[p_node].switches[0] = delayless_switch;

    rr_node[p_node].cost_index = IPIN_COST_INDEX;
    rr_node[p_node].type = IPIN;

  /* Code common to both SINK and IPIN. */

    inode = s_node;
    for (iloop=1;iloop<=2;iloop++) {    /* for both SOURCE or SINK and pin */
       rr_node[inode].occ = 0;
       rr_node[inode].capacity = 1;

       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;
       rr_node[inode].R = 0.;
       rr_node[inode].C = 0.;

       rr_node[inode].ptc_num = ipad;

       inode = p_node;
    }

 }   /* End for each pad. */
}


static void build_rr_xchan (int **rr_node_indices, enum e_route_type 
      route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
      tracks_to_pads, int i, int j, int nodes_per_chan, enum 
      e_switch_block_type switch_block_type, int wire_to_ipin_switch, 
      t_seg_details *seg_details_x, t_seg_details *seg_details_y,
      int cost_index_offset) {

/* Loads up all the routing resource nodes in the x-directed channel      *
 * segments starting at (i,j).                                            */

 int itrack, num_edges, inode, istart, iend, length, seg_index;
 t_linked_edge *edge_list_head;

 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    istart = get_closest_seg_start (seg_details_x, itrack, i, j);
    if (istart != i)
       continue;        /* Not the start of this segment. */

    iend = get_seg_end (seg_details_x, itrack, istart, j, nx);
    edge_list_head = NULL;

/* First count number of edges and put the edges in a linked list. */

    if (j == 0) {                   /* Between bottom pads and clbs.  */
       num_edges = get_xtrack_to_clb_ipin_edges (istart, iend, j, itrack, TOP, 
                   &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_x, wire_to_ipin_switch);

       num_edges += get_xtrack_to_pad_edges (istart, iend, j, j, itrack, 
                    &edge_list_head, tracks_to_pads, nodes_per_chan, 
                    rr_node_indices, seg_details_x, wire_to_ipin_switch);

       /* Only channels above exist. */

       num_edges += get_xtrack_to_ytracks (istart, iend, j, itrack, j+1,
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x,  seg_details_y, switch_block_type); 
    }

    else if (j == ny) {           /* Between top clbs and pads. */
       num_edges = get_xtrack_to_clb_ipin_edges (istart, iend, j, itrack, 
                   BOTTOM, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_x, wire_to_ipin_switch);

       num_edges += get_xtrack_to_pad_edges (istart, iend, j, j+1, itrack, 
                    &edge_list_head, tracks_to_pads, nodes_per_chan, 
                    rr_node_indices, seg_details_x, wire_to_ipin_switch);

       /* Only channels below exist. */

       num_edges += get_xtrack_to_ytracks (istart, iend, j, itrack, j,
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x,  seg_details_y, switch_block_type); 
    }

    else {                          /* Between two rows of clbs. */
       num_edges = get_xtrack_to_clb_ipin_edges (istart, iend, j, itrack, 
                   BOTTOM, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_x, wire_to_ipin_switch);

       num_edges += get_xtrack_to_clb_ipin_edges (istart, iend, j, itrack, 
                    TOP, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                    rr_node_indices, seg_details_x, wire_to_ipin_switch);

       /* Channels above and below both exist. */

       num_edges += get_xtrack_to_ytracks (istart, iend, j, itrack, j+1,
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x,  seg_details_y, switch_block_type); 

       num_edges += get_xtrack_to_ytracks (istart, iend, j, itrack, j,
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x,  seg_details_y, switch_block_type); 

    }
    
    if (istart != 1) {   /* x-chan to left exists. */
       num_edges += get_xtrack_to_xtrack (istart, j, itrack, istart - 1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, switch_block_type); 
    }

    if (iend != nx) {      /* x-chan to right exists. */
       num_edges += get_xtrack_to_xtrack (iend, j, itrack, iend + 1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, switch_block_type); 
    }


    inode = get_rr_node_index (i, j, CHANX, itrack, nodes_per_chan, 
             rr_node_indices);

    alloc_and_load_edges_and_switches (inode, num_edges, edge_list_head);

/* Edge arrays have now been built up.  Do everything else.  */

    seg_index = seg_details_x[itrack].index;
    rr_node[inode].cost_index = cost_index_offset + seg_index;
    rr_node[inode].occ = 0;

    if (route_type == DETAILED) 
       rr_node[inode].capacity = 1;
    else                                    /* GLOBAL routing */
       rr_node[inode].capacity = chan_width_x[j];
 
    rr_node[inode].xlow = istart;
    rr_node[inode].xhigh = iend;
    rr_node[inode].ylow = j;
    rr_node[inode].yhigh = j;

    length = iend - istart + 1;
    rr_node[inode].R = length * seg_details_x[itrack].Rmetal;
    rr_node[inode].C = length * seg_details_y[itrack].Cmetal;

    rr_node[inode].ptc_num = itrack;
    rr_node[inode].type = CHANX;
 }
}


static void build_rr_ychan (int **rr_node_indices, enum e_route_type 
      route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
      tracks_to_pads, int i, int j, int nodes_per_chan, enum 
      e_switch_block_type switch_block_type, int wire_to_ipin_switch,
      t_seg_details *seg_details_x, t_seg_details *seg_details_y,
      int cost_index_offset) {

/* Loads up all the routing resource nodes in the y-directed channel      *
 * segments starting at (i,j).                                            */

 int itrack, num_edges, inode, jstart, jend, length, seg_index;
 t_linked_edge *edge_list_head;

 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    jstart = get_closest_seg_start (seg_details_y, itrack, j, i);
    if (jstart != j)
       continue;        /* Not the start of this segment. */

    jend = get_seg_end (seg_details_y, itrack, jstart, i, ny);
    edge_list_head = NULL;

/* First count number of edges and put the edges in a linked list. */

    if (i == 0) {                   /* Between leftmost pads and clbs.  */
       num_edges = get_ytrack_to_clb_ipin_edges (jstart, jend, i, itrack, 
                   RIGHT, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_y, wire_to_ipin_switch);

       num_edges += get_ytrack_to_pad_edges (jstart, jend, i, i, itrack, 
                    &edge_list_head, tracks_to_pads, nodes_per_chan, 
                    rr_node_indices, seg_details_y, wire_to_ipin_switch);

       /* Only channel to right exists. */

       num_edges += get_ytrack_to_xtracks (jstart, jend, i, itrack, i+1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, seg_details_y, switch_block_type);
    }

    else if (i == nx) {           /* Between rightmost clbs and pads. */
       num_edges = get_ytrack_to_clb_ipin_edges (jstart, jend, i, itrack, 
                   LEFT, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_y, wire_to_ipin_switch);

       num_edges += get_ytrack_to_pad_edges (jstart, jend, i, i+1, itrack, 
                    &edge_list_head, tracks_to_pads, nodes_per_chan, 
                    rr_node_indices, seg_details_y, wire_to_ipin_switch);

       /* Only channel to left exists. */

       num_edges += get_ytrack_to_xtracks (jstart, jend, i, itrack, i, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, seg_details_y, switch_block_type);
    }

    else {                          /* Between two rows of clbs. */
       num_edges = get_ytrack_to_clb_ipin_edges (jstart, jend, i, itrack, 
                   LEFT, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                   rr_node_indices, seg_details_y, wire_to_ipin_switch);

       num_edges += get_ytrack_to_clb_ipin_edges (jstart, jend, i, itrack, 
                    RIGHT, &edge_list_head, tracks_to_clb_ipin, nodes_per_chan, 
                    rr_node_indices, seg_details_y, wire_to_ipin_switch);

       /* Channels on both sides. */

       num_edges += get_ytrack_to_xtracks (jstart, jend, i, itrack, i+1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, seg_details_y, switch_block_type);

       num_edges += get_ytrack_to_xtracks (jstart, jend, i, itrack, i,
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_x, seg_details_y, switch_block_type);
    }

    if (jstart != 1) {   /* y-chan below exists. */
       num_edges += get_ytrack_to_ytrack (i, jstart, itrack, jstart - 1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_y, switch_block_type); 
    }

    if (jend != ny) {      /* y-chan above exists. */
       num_edges += get_ytrack_to_ytrack (i, jend, itrack, jend + 1, 
                    &edge_list_head, nodes_per_chan, rr_node_indices, 
                    seg_details_y, switch_block_type); 
    }


    inode = get_rr_node_index (i, j, CHANY, itrack, nodes_per_chan, 
             rr_node_indices);

    alloc_and_load_edges_and_switches (inode, num_edges, edge_list_head);

/* Edge arrays have now been built up.  Do everything else.  */

    seg_index = seg_details_y[itrack].index;
    rr_node[inode].cost_index = cost_index_offset + seg_index;
    rr_node[inode].occ = 0;

    if (route_type == DETAILED) 
       rr_node[inode].capacity = 1;
    else                                    /* GLOBAL routing */
       rr_node[inode].capacity = chan_width_y[i];
 
    rr_node[inode].xlow = i;
    rr_node[inode].xhigh = i;
    rr_node[inode].ylow = jstart;
    rr_node[inode].yhigh = jend;

    length = jend - jstart + 1;
    rr_node[inode].R = length * seg_details_y[itrack].Rmetal;
    rr_node[inode].C = length * seg_details_y[itrack].Cmetal;
 
    rr_node[inode].ptc_num = itrack;
    rr_node[inode].type = CHANY;
 }
}


void alloc_and_load_edges_and_switches (int inode, int num_edges, 
        t_linked_edge *edge_list_head) {
 
/* Sets up all the edge related information for rr_node inode (num_edges,  * 
 * the edges array and the switches array).  The edge_list_head points to  *
 * a list of the num_edges edges and switches to put in the arrays.  This  *
 * linked list is freed by this routine. This routine also resets the      *
 * rr_edge_done array for the next rr_node (i.e. set it so that no edges   *
 * are marked as having been seen before).                                 */
 
 t_linked_edge *list_ptr, *next_ptr;
 int i, to_node;


 rr_node[inode].num_edges = num_edges; 
 
 rr_node[inode].edges = (int *) my_chunk_malloc (num_edges * sizeof(int), 
       &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem); 
 
 rr_node[inode].switches = (short *) my_chunk_malloc (num_edges * 
       sizeof(short), &rr_mem_chunk_list_head, &chunk_bytes_avail, 
       &chunk_next_avail_mem); 
     
/* Load the edge and switch arrays, deallocate the linked list and reset   *
 * the rr_edge_done flags.                                                 */ 
  
 list_ptr = edge_list_head;
 i = 0;

 while (list_ptr != NULL) {
    to_node = list_ptr->edge;
    rr_node[inode].edges[i] = to_node;
    rr_edge_done[to_node] = FALSE;
    rr_node[inode].switches[i] = list_ptr->iswitch;
    next_ptr = list_ptr->next;

    /* Add to free list */
    free_linked_edge_soft (list_ptr, &free_edge_list_head); 

    list_ptr = next_ptr;
    i++;
 }
}
 

static int ***alloc_and_load_clb_pin_to_tracks (enum e_pin_type pin_type, 
          int nodes_per_chan, int Fc, boolean perturb_switch_pattern) {

/* Allocates and loads an array that contains a list of which tracks each  *
 * output or input pin of a clb should connect to on each side of the clb. *
 * The pin_type flag is either DRIVER or RECEIVER, and determines whether  *
 * information for output C blocks or input C blocks is generated.         *
 * Set Fc and nodes_per_chan to 1 if you're doing a global routing graph.  */


 int *num_dir;   /* [0..3] Number of *physical* pins on each clb side.      */
 int **dir_list; /* [0..3][0..pins_per_clb-1] list of pins of correct type  *
                  * on each side. Max possible space alloced for simplicity */

 int i, j, iside, ipin, iclass, num_phys_pins, pindex;
 int *num_done_per_dir, *pin_num_ordering, *side_ordering;
 int ***tracks_connected_to_pin;  /* [0..pins_per_clb-1][0..3][0..Fc-1] */
 float step_size;

/* NB:  This wastes some space.  Could set tracks_..._pin[ipin][iside] =   *
 * NULL if there is no pin on that side, or that pin is of the wrong type. *
 * Probably not enough memory to worry about, esp. as it's temporary.      *
 * If pin ipin on side iside does not exist or is of the wrong type,       *
 * tracks_connected_to_pin[ipin][iside][0] = OPEN.                         */

 tracks_connected_to_pin = (int ***) alloc_matrix3 (0, pins_per_clb-1, 0, 3,
                  0, Fc-1, sizeof(int));

 for (ipin=0;ipin<pins_per_clb;ipin++) 
    for (iside=0;iside<=3;iside++)
       tracks_connected_to_pin[ipin][iside][0] = OPEN;  /* Unconnected. */


 num_dir = (int *) my_calloc (4, sizeof(int));
 dir_list = (int **) alloc_matrix (0, 3, 0, pins_per_clb-1, sizeof(int));

/* Defensive coding.  Try to crash hard if I use an unset entry.  */
 
 for (i=0;i<4;i++) 
    for (j=0;j<pins_per_clb;j++) 
       dir_list[i][j] = -1;
 

 for (ipin=0;ipin<pins_per_clb;ipin++) {
    iclass = clb_pin_class[ipin];
    if (class_inf[iclass].type != pin_type) /* Doing either ipins OR opins */
       continue;
     
/* Pins connecting only to global resources get no switches -> keeps the    *
 * area model accurate.                                                     */

    if (is_global_clb_pin[ipin])   
       continue;
    
    for (iside=0;iside<=3;iside++) {
       if (pinloc[iside][ipin] == 1) {
          dir_list[iside][num_dir[iside]] = ipin;
          num_dir[iside]++;
       }
    }
 }

 num_phys_pins = 0;
 for (iside=0;iside<4;iside++) 
    num_phys_pins += num_dir[iside];  /* Num. physical output pins per clb */

 num_done_per_dir = (int *) my_calloc (4, sizeof(int));
 pin_num_ordering = (int *) my_malloc (num_phys_pins * sizeof(int));
 side_ordering = (int *) my_malloc (num_phys_pins * sizeof(int));

/* Connection block I use distributes pins evenly across the tracks      *
 * of ALL sides of the clb at once.  Ensures that each pin connects      *
 * to spaced out tracks in its connection block, and that the other      *
 * pins (potentially in other C blocks) connect to the remaining tracks  *
 * first.  Doesn't matter for large Fc, but should make a fairly         *
 * good low Fc block that leverages the fact that usually lots of pins   *
 * are logically equivalent.                                             */

 iside = -1;
 ipin = 0;
 pindex = 0;

 while (ipin < num_phys_pins) {
    iside = (iside + 1);
    if (iside > 3) {
       pindex++;
       iside = 0;
    }
    if (num_done_per_dir[iside] >= num_dir[iside])
       continue;
    pin_num_ordering[ipin] = dir_list[iside][pindex]; 
    side_ordering[ipin] = iside;
    num_done_per_dir[iside]++;
    ipin++;
 }


 step_size = (float) nodes_per_chan / (float) (Fc * num_phys_pins);
 if (step_size > 1.) {
    if (pin_type == DRIVER) 
       printf("Warning:  some tracks are never driven by clb outputs.\n");
    else 
       printf("Warning:  some tracks cannot reach any inputs.\n");
 }

 if (perturb_switch_pattern) {
    load_perturbed_switch_pattern (tracks_connected_to_pin, num_phys_pins, 
          pin_num_ordering, side_ordering, nodes_per_chan, Fc, step_size);

    check_all_tracks_reach_pins (tracks_connected_to_pin, nodes_per_chan, Fc, 
          RECEIVER);
 }
 else {
    load_uniform_switch_pattern (tracks_connected_to_pin, num_phys_pins, 
          pin_num_ordering, side_ordering, nodes_per_chan, Fc, step_size);
 }

/* Free all temporary storage. */

 free (num_dir);
 free_matrix (dir_list, 0, 3, 0, sizeof(int));
 free (num_done_per_dir);
 free (pin_num_ordering);
 free (side_ordering);

 return (tracks_connected_to_pin);
}


static void load_uniform_switch_pattern (int ***tracks_connected_to_pin, 
       int num_phys_pins, int *pin_num_ordering, int *side_ordering, 
       int nodes_per_chan, int Fc, float step_size) {

/* Loads the tracks_connected_to_pin array with an even distribution of     *
 * switches across the tracks for each pin.  For example, each pin connects *
 * to every 4.3rd track in a channel, with exactly which tracks a pin       *
 * connects to staggered from pin to pin.                                   */

 int i, j, ipin, iside, itrack;
 float f_track;


 for (i=0;i<num_phys_pins;i++) {
    ipin = pin_num_ordering[i];
    iside = side_ordering[i];
    for (j=0;j<Fc;j++) {
       f_track = i*step_size + j * (float) nodes_per_chan / (float) Fc;
       itrack = (int) f_track;

  /* In case of weird roundoff effects */
       itrack = min(itrack, nodes_per_chan-1);
       tracks_connected_to_pin[ipin][iside][j] = itrack;
    }
 }
}


static void load_perturbed_switch_pattern (int ***tracks_connected_to_pin, 
       int num_phys_pins, int *pin_num_ordering, int *side_ordering, 
       int nodes_per_chan, int Fc, float step_size) {

/* Loads the tracks_connected_to_pin array with an unevenly distributed     *
 * set of switches across the channel.  This is done for inputs when        *
 * Fc_input = Fc_output to avoid creating "pin domains" -- certain output   *
 * pins being able to talk only to certain input pins because their switch  *
 * patterns exactly line up.  Distribute Fc/2 + 1 switches over half the    *
 * channel and Fc/2 - 1 switches over the other half to make the switch     * 
 * pattern different from the uniform one of the outputs.  Also, have half  *
 * the pins put the "dense" part of their connections in the first half of  *
 * the channel and the other half put the "dense" part in the second half,  *
 * to make sure each track can connect to about the same number of ipins.   */

 int i, j, ipin, iside, itrack, ihalf, iconn;
 int Fc_dense, Fc_sparse, Fc_half[2];
 float f_track, spacing_dense, spacing_sparse, spacing[2];
 
 Fc_dense = Fc/2 + 1;
 Fc_sparse = Fc - Fc_dense;  /* Works for even or odd Fc */

 spacing_dense = (float) nodes_per_chan / (float) (2 * Fc_dense);
 spacing_sparse = (float) nodes_per_chan / (float) (2 * Fc_sparse);

 for (i=0;i<num_phys_pins;i++) {
    ipin = pin_num_ordering[i];
    iside = side_ordering[i];

    /* Flip every pin to balance switch density */

    spacing[i%2] = spacing_dense;
    Fc_half[i%2] = Fc_dense;
    spacing[(i+1)%2] = spacing_sparse;
    Fc_half[(i+1)%2] = Fc_sparse;

    f_track = i * step_size;    /* Start point.  Staggered from pin to pin */
    iconn = 0;

    for (ihalf=0;ihalf<2;ihalf++) {  /* For both dense and sparse halves. */

       for (j=0;j<Fc_half[ihalf];j++) {
          itrack = (int) f_track;

     /* Can occasionally get wraparound. */
          itrack = itrack%nodes_per_chan;
          tracks_connected_to_pin[ipin][iside][iconn] = itrack;

          f_track += spacing[ihalf];
          iconn++;
       }

    }
 }   /* End for all physical pins. */
}


static void check_all_tracks_reach_pins (int ***tracks_connected_to_pin,
         int nodes_per_chan, int Fc, enum e_pin_type ipin_or_opin) {

/* Checks that all tracks can be reached by some pin.   */

 int iconn, iside, itrack, ipin;
 int *num_conns_to_track;  /* [0..nodes_per_chan-1] */

 num_conns_to_track = (int *) my_calloc (nodes_per_chan, sizeof(int));
 
 for (ipin=0;ipin<pins_per_clb;ipin++) {
    for (iside=0;iside<=3;iside++) {
       if (tracks_connected_to_pin[ipin][iside][0] != OPEN) { /* Pin exists */
          for (iconn=0;iconn<Fc;iconn++) {
             itrack = tracks_connected_to_pin[ipin][iside][iconn];
             num_conns_to_track[itrack]++;
          }
       }
    }
 }

 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    if (num_conns_to_track[itrack] <= 0) {
       printf ("Warning (check_all_tracks_reach_pins):  track %d does not \n"
               "\tconnect to any CLB ", itrack);

       if (ipin_or_opin == DRIVER)
          printf ("OPINs.\n");
       else
          printf ("IPINs.\n");
    }
 }
 
 free (num_conns_to_track);
}
 

static struct s_ivec **alloc_and_load_tracks_to_clb_ipin (int nodes_per_chan,
      int Fc, int ***clb_ipin_to_tracks) {

/* The routing graph will connect tracks to input pins on the clbs.   *
 * This routine converts the list of which tracks each ipin connects  *
 * to into a list of the ipins each track connects to.                */

 int ipin, iside, itrack, iconn, ioff, tr_side;

/* [0..nodes_per_chan-1][0..3].  For each track number it stores a vector  *
 * for each of the four sides.  x-directed channels will use the TOP and   *
 * BOTTOM vectors to figure out what clb input pins they connect to above  *
 * and below them, respectively, while y-directed channels use the LEFT    *
 * and RIGHT vectors.  Each vector contains an nelem field saying how many *
 * ipins it connects to.  The list[0..nelem-1] array then gives the pin    *
 * numbers.                                                                */

/* Note that a clb pin that connects to a channel on its RIGHT means that  *
 * that channel connects to a clb pin on its LEFT.  I have to convert the  *
 * sides so that the new structure lists the sides of pins from the        *
 * perspective of the track.                                               */

 struct s_ivec **tracks_to_clb_ipin;
 
 tracks_to_clb_ipin = (struct s_ivec **) alloc_matrix (0, nodes_per_chan-1, 0,
      3, sizeof(struct s_ivec));

 for (itrack=0;itrack<nodes_per_chan;itrack++)
    for (iside=0;iside<=3;iside++) 
       tracks_to_clb_ipin[itrack][iside].nelem = 0;

/* Counting pass.  */

 for (ipin=0;ipin<pins_per_clb;ipin++) {
    for (iside=0;iside<=3;iside++) {
       if (clb_ipin_to_tracks[ipin][iside][0] == OPEN) 
          continue;
       
       tr_side = track_side (iside);
       for (iconn=0;iconn<Fc;iconn++) {
          itrack = clb_ipin_to_tracks[ipin][iside][iconn];
          tracks_to_clb_ipin[itrack][tr_side].nelem++;
       }
    }
 }

/* Allocate space.  */

 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    for (iside=0;iside<=3;iside++) {
       if (tracks_to_clb_ipin[itrack][iside].nelem != 0) {
          tracks_to_clb_ipin[itrack][iside].list = (int *) my_malloc (
                    tracks_to_clb_ipin[itrack][iside].nelem * sizeof (int));
          tracks_to_clb_ipin[itrack][iside].nelem = 0;
       }
       else {
          tracks_to_clb_ipin[itrack][iside].list = NULL;  /* Defensive code */
       }
    }
 } 

/* Loading pass. */

 for (ipin=0;ipin<pins_per_clb;ipin++) {
    for (iside=0;iside<=3;iside++) {
       if (clb_ipin_to_tracks[ipin][iside][0] == OPEN) 
          continue;
       
       tr_side = track_side (iside);
       for (iconn=0;iconn<Fc;iconn++) {
          itrack = clb_ipin_to_tracks[ipin][iside][iconn];
          ioff = tracks_to_clb_ipin[itrack][tr_side].nelem;
          tracks_to_clb_ipin[itrack][tr_side].list[ioff] = ipin;
          tracks_to_clb_ipin[itrack][tr_side].nelem++;
       }
    }
 }
 
 return (tracks_to_clb_ipin);
}


static int track_side (int clb_side) {

/* Converts a side from referring to the world from a clb's perspective *
 * to a channel's perspective.  That is, a connection from a clb to the *
 * track above (TOP) it, is to the clb below (BOTTOM) the track.        */
 
 switch (clb_side) {
 
 case TOP:
    return (BOTTOM);

 case BOTTOM:
    return (TOP);
 
 case LEFT:
    return (RIGHT);

 case RIGHT:
    return (LEFT);

 default:
    printf("Error:  unexpected clb_side (%d) in track_side.\n", clb_side);
    exit (1);
 }
}


static int **alloc_and_load_pads_to_tracks (int nodes_per_chan, int Fc_pad) {

/* Allocates and loads up a 2D array ([0..io_rat-1][0..Fc_pad-1]) where *
 * each entry gives a track number to which that pad connects if it is  *
 * an INPUT pad.  Code below should work for both GLOBAL and DETAILED.  */

 int **pads_to_tracks;
 float step_size;
 int ipad, iconn, itrack;

 pads_to_tracks = (int **) alloc_matrix (0, io_rat-1, 0, Fc_pad-1, sizeof(int));
 step_size = (float) nodes_per_chan / (float) (Fc_pad * io_rat);
 
 for (ipad=0;ipad<io_rat;ipad++) {
    for (iconn=0;iconn<Fc_pad;iconn++) {

       itrack = (int) (ipad * step_size + iconn * (float) nodes_per_chan / 
                 (float) Fc_pad);

/* Watch for weird round off effects.  */
       itrack = min(itrack, nodes_per_chan-1);
       pads_to_tracks[ipad][iconn] = itrack;
    }
 }

 return (pads_to_tracks);
}


static struct s_ivec *alloc_and_load_tracks_to_pads (int **pads_to_tracks, 
          int nodes_per_chan, int Fc_pad) {

/* Converts the list of tracks each IO pad connects to into a list of  *
 * pads each track should connect to.  Allocates and fills in an array *
 * of vectors [0..nodes_per_chan-1].  Each vector specifies the number *
 * of pads to which that track connects and gives a list of the pads.  */

 int itrack, ipad, i, iconn, ioff;
 struct s_ivec *tracks_to_pads;

 tracks_to_pads = (struct s_ivec *) my_malloc (nodes_per_chan * sizeof (
           struct s_ivec));
 
/* Counting pass. */

 for (i=0;i<nodes_per_chan;i++) 
    tracks_to_pads[i].nelem = 0;

 for (ipad=0;ipad<io_rat;ipad++) {
    for (iconn=0;iconn<Fc_pad;iconn++) {
       itrack = pads_to_tracks[ipad][iconn];
       tracks_to_pads[itrack].nelem++;
    }
 }

 for (i=0;i<nodes_per_chan;i++) {
    if (tracks_to_pads[i].nelem != 0) {
       tracks_to_pads[i].list = (int *) my_malloc (tracks_to_pads[i].nelem *
                        sizeof(int));
       tracks_to_pads[i].nelem = 0;
    }
    else {
       tracks_to_pads[i].list = NULL;  /* For safety only. */
    }
 }

/* Load pass. */

 for (ipad=0;ipad<io_rat;ipad++) {
    for (iconn=0;iconn<Fc_pad;iconn++) {
       itrack = pads_to_tracks[ipad][iconn];
       ioff = tracks_to_pads[itrack].nelem;
       tracks_to_pads[itrack].list[ioff] = ipad;
       tracks_to_pads[itrack].nelem++;
    }
 }

 return (tracks_to_pads);
}


void dump_rr_graph (char *file_name) {

/* A utility routine to dump the contents of the routing resource graph   *
 * (everything -- connectivity, occupancy, cost, etc.) into a file.  Used *
 * only for debugging.                                                    */

 int inode, index;
 FILE *fp;

 fp = my_fopen (file_name, "w", 0);

 for (inode=0;inode<num_rr_nodes;inode++) {
    print_rr_node (fp, inode);
    fprintf (fp, "\n");
 }

 fprintf (fp, "\n\n%d rr_indexed_data entries.\n\n", num_rr_indexed_data);
 
 for (index=0;index<num_rr_indexed_data;index++) {
    print_rr_indexed_data (fp, index);
    fprintf (fp, "\n");
 }
     
 fclose (fp);
}


void print_rr_node (FILE *fp, int inode) {

/* Prints all the data about node inode to file fp.                    */

 static char *name_type[] = {"SOURCE", "SINK", "IPIN", "OPIN", "CHANX",
       "CHANY"};
 t_rr_type rr_type;
 int iconn;

 rr_type = rr_node[inode].type;
 if (rr_node[inode].xlow == rr_node[inode].xhigh && rr_node[inode].ylow ==
            rr_node[inode].yhigh) {
    fprintf (fp, "Node: %d.  %s (%d, %d)  Ptc_num: %d\n", inode,
       name_type[rr_type], rr_node[inode].xlow, rr_node[inode].ylow,
       rr_node[inode].ptc_num);
 }
 else {
    fprintf (fp, "Node: %d.  %s (%d, %d) to (%d, %d)  Ptc_num: %d\n", inode,
       name_type[rr_type], rr_node[inode].xlow, rr_node[inode].ylow,
       rr_node[inode].xhigh, rr_node[inode].yhigh,
       rr_node[inode].ptc_num);
 }
 
 fprintf (fp, "%4d edge(s):", rr_node[inode].num_edges);
 
 for (iconn=0;iconn<rr_node[inode].num_edges;iconn++)
    fprintf (fp," %d", rr_node[inode].edges[iconn]);

 fprintf (fp,"\nSwitch types:");

 for (iconn=0;iconn<rr_node[inode].num_edges;iconn++)
    fprintf (fp, " %d", rr_node[inode].switches[iconn]);
     
 fprintf (fp, "\n");
 fprintf (fp, "Occ: %d  Capacity: %d.\n", rr_node[inode].occ,
     rr_node[inode].capacity);
 fprintf (fp,"R: %g  C: %g\n", rr_node[inode].R, rr_node[inode].C);
 fprintf (fp, "Cost_index: %d\n", rr_node[inode].cost_index);
}


void print_rr_indexed_data (FILE *fp, int index) {

/* Prints all the rr_indexed_data of index to file fp.   */

 fprintf (fp, "Index: %d\n", index);
 fprintf (fp, "ortho_cost_index: %d  base_cost: %g  saved_base_cost: %g\n",
               rr_indexed_data[index].ortho_cost_index, 
               rr_indexed_data[index].base_cost,
               rr_indexed_data[index].saved_base_cost);
 fprintf (fp, "Seg_index: %d  inv_length: %g\n", 
       rr_indexed_data[index].seg_index, rr_indexed_data[index].inv_length);
 fprintf (fp, "T_linear: %g  T_quadratic: %g  C_load: %g\n", 
       rr_indexed_data[index].T_linear, rr_indexed_data[index].T_quadratic,
       rr_indexed_data[index].C_load);
}
