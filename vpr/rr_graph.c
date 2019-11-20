#include <stdio.h>
#include "util.h"
#include "pr.h"
#include "ext.h"
#include "rr_graph.h"
#include <assert.h>


struct s_ivec {int nelem; int *list;};
/* Integer vector.  nelem stores length, list[0..nelem-1] stores list of    *
 * integers.                                                                */

static struct s_linked_vptr *rr_mem_chunk_list_head = NULL;   
/* Used to free "chunked" memory.  If NULL, no rr_graph exists right now.  */

static int chunk_bytes_avail = 0;
static char *chunk_next_avail_mem = NULL;
/* Status of current chunk being dished out by calls to my_chunk_malloc.   */


static int **alloc_and_load_rr_node_indices (int nodes_per_clb, 
         int nodes_per_pad, int nodes_per_chan); 
static int ***alloc_and_load_clb_pin_to_tracks (enum e_pin_type pin_type,
         int nodes_per_chan, int Fc); 
static struct s_ivec **alloc_and_load_tracks_to_clb_ipin (int nodes_per_chan,
         int Fc, int ***clb_ipin_to_tracks); 
static int **alloc_and_load_pads_to_tracks (int nodes_per_chan, int Fc_pad); 
static struct s_ivec *alloc_and_load_tracks_to_pads (int **pads_to_tracks, 
         int nodes_per_chan, int Fc_pad); 
static int track_side (int clb_side); 
static void free_ivec_matrix (struct s_ivec **ivec_matrix, int nrmin, int
         nrmax, int ncmin, int ncmax);
static void free_ivec_vector (struct s_ivec *ivec_vector, int nrmin, 
         int nrmax); 
static void alloc_and_load_rr_graph (int **rr_node_indices,
         int ***clb_opin_to_tracks, struct s_ivec **tracks_to_clb_ipin,
         int **pads_to_tracks, struct s_ivec *tracks_to_pads, int Fc_output,
         int Fc_input, int Fc_pad, int nodes_per_chan, enum e_route_type
         route_type, enum e_switch_block_type switch_block_type); 
static void build_rr_clb (int **rr_node_indices, int Fc_output, int ***
         clb_opin_to_tracks, int nodes_per_chan, int i, int j); 
static void build_rr_pads (int **rr_node_indices, int Fc_pad,
         int **pads_to_tracks, int nodes_per_chan, int i, int j); 
static void build_rr_xchan (int **rr_node_indices, enum e_route_type
         route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
         tracks_to_pads, int i, int j, int nodes_per_chan, enum 
         e_switch_block_type switch_block_type); 
static void build_rr_ychan (int **rr_node_indices, enum e_route_type
         route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
         tracks_to_pads, int i, int j, int nodes_per_chan, enum 
         e_switch_block_type switch_block_type); 
static int add_switch_box_edge (int from_i, int from_j, int from_track,
         t_rr_type from_type, int to_i, int to_j, t_rr_type
         to_type, enum e_route_type route_type, enum e_switch_block_type
         switch_block_type, int nodes_per_chan, int **rr_node_indices); 
static int add_track_to_clb_ipin_edges (int tr_node, int tr_i, int tr_j,
         int itrack, int iside, int start_conn, struct s_ivec **
         tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices); 
static int num_track_to_pad_conn (int pad_i, int pad_j, int itrack,
         struct s_ivec *tracks_to_pads); 
static int add_track_to_pad_edges (int tr_node, int pad_i, int pad_j,
         int itrack, int start_conn, struct s_ivec *tracks_to_pads,
         int nodes_per_chan, int **rr_node_indices); 
static int get_rr_node_index (int i, int j, t_rr_type rr_type, int ioff,
         int nodes_per_chan, int **rr_node_indices); 
static void alloc_and_load_net_rr_terminals (int **rr_node_indices, 
         int nodes_per_chan); 
static int which_io_block (int iblk); 






void build_rr_graph (enum e_route_type route_type, struct s_det_routing_arch
         det_routing_arch) {

/* Builds the routing resource graph needed for routing.  Does all the   *
 * necessary allocation and initialization.  If route_type is DETAILED   *
 * then the routing graph built is for detailed routing, otherwise it is *
 * for GLOBAL routing.                                                   */

 int nodes_per_clb, nodes_per_pad, nodes_per_chan;
 int inode, num_segments, W;
 int **rr_node_indices;
 int Fc_output, Fc_input, Fc_pad;
 int **pads_to_tracks;
 struct s_ivec **tracks_to_clb_ipin, *tracks_to_pads;

/* [0..pins_per_clb-1][0..3][0..Fc_output-1].  List of tracks this pin *
 * connects to.  Second index is the side number (see pr.h).  If a pin *
 * is not an output or input, respectively, or doesn't connect to      *
 * anything on this side, the [ipin][iside][0] element is OPEN.        */

 int ***clb_opin_to_tracks, ***clb_ipin_to_tracks;


 W = chan_width_x[0];   /* Channel width.  Used only for detailed routing. */

/* Compute the total number of routing resource nodes needed. */

 inode = nx * ny * num_class;   /* SOURCE and SINK dummy nodes (logic) */
 inode += 2 * (nx + ny) * io_rat;  /* SOURCE and SINK nodes (IO) */
 inode += nx * ny * pins_per_clb;  /* Physical pins (logic) */
 inode += 2 * (nx + ny) * io_rat;  /* Physical pins (IO) */

/* y-directed segments, assuming all segments of length 1. Generalize later. */
 num_segments = (nx + 1) * ny; 
 num_segments += (ny + 1) * nx;   /* x-directed segments. */

/* All channels must have the same width to do detailed routing.  Checked *
 * in read_arch.c.                                                        */

 if (route_type == DETAILED) 
    num_segments *= W;

 num_rr_nodes = inode + num_segments;

/* To set up the routing graph I need to choose an order for the rr_indices. *
 * For each (i,j) slot in the FPGA, the index order is [source+sink]/pins/   *
 * chanx/chany.  The dummy source and sink are order by class number or pad  *
 * number; the pins are ordered by pin number or pad number, and the channel *
 * segments are ordered by track number.  Track 1 is closest to the "owning" *
 * block; i.e. it is towards the left of y-chans and the bottom of x-chans.  *
 *                                                                           *
 * The maximize cache effectiveness, I put all the rr_nodes associated with  *
 * a block together (this includes the "owned" channel segments).  I start   *
 * at (0,0) (empty), go to (1,0), (2,0) ... (0,1) and so on.                 */

 nodes_per_clb = num_class + pins_per_clb;
 nodes_per_pad = io_rat + io_rat;    /* Dummy source/sink + pin */

 if (route_type == GLOBAL) {
    nodes_per_chan = 1;
 }
 else {
    nodes_per_chan = W;
 }

 rr_node_indices = alloc_and_load_rr_node_indices (nodes_per_clb, 
    nodes_per_pad, nodes_per_chan);

 if (route_type == DETAILED) {
    if (det_routing_arch.Fc_type == ABSOLUTE) {
       Fc_output = min (det_routing_arch.Fc_output, W);
       Fc_input = min (det_routing_arch.Fc_input, W);
       Fc_pad = min (det_routing_arch.Fc_pad, W);
    }
    else {    /* FRACTIONAL */
       Fc_output = (int) (W * det_routing_arch.Fc_output + 0.5);
       Fc_output = max (1, Fc_output);
       Fc_input = (int) (W * det_routing_arch.Fc_input + 0.5);
       Fc_input = max (1, Fc_input);
       Fc_pad = (int) (W * det_routing_arch.Fc_pad + 0.5);
       Fc_pad = max (1, Fc_pad);
    }
 }
 else {   /* GLOBAL */
    Fc_output = 1;
    Fc_input = 1;
    Fc_pad = 1;
 }

 clb_opin_to_tracks = alloc_and_load_clb_pin_to_tracks (DRIVER, nodes_per_chan,
        Fc_output);

 clb_ipin_to_tracks = alloc_and_load_clb_pin_to_tracks (RECEIVER,
        nodes_per_chan, Fc_input);
 
 tracks_to_clb_ipin = alloc_and_load_tracks_to_clb_ipin (nodes_per_chan,
        Fc_input, clb_ipin_to_tracks);

 pads_to_tracks = alloc_and_load_pads_to_tracks (nodes_per_chan, Fc_pad);
 
 tracks_to_pads = alloc_and_load_tracks_to_pads (pads_to_tracks, 
          nodes_per_chan, Fc_pad);

 alloc_and_load_rr_graph (rr_node_indices, clb_opin_to_tracks,
       tracks_to_clb_ipin, pads_to_tracks, tracks_to_pads, Fc_output,
       Fc_input, Fc_pad, nodes_per_chan, route_type, 
       det_routing_arch.switch_block_type);

 free_matrix (rr_node_indices, 0, nx+1, 0, sizeof(int));
 free_matrix3 (clb_opin_to_tracks, 0, pins_per_clb-1, 0, 3, 0, sizeof(int));
 free_matrix3 (clb_ipin_to_tracks, 0, pins_per_clb-1, 0, 3, 0, sizeof(int));
 free_ivec_matrix (tracks_to_clb_ipin, 0, nodes_per_chan-1, 0, 3);
 free_ivec_vector (tracks_to_pads, 0, nodes_per_chan-1);
 free_matrix (pads_to_tracks, 0, io_rat-1, 0, sizeof(int));
}


static void alloc_and_load_rr_graph (int **rr_node_indices, 
       int ***clb_opin_to_tracks, struct s_ivec **tracks_to_clb_ipin,
       int **pads_to_tracks, struct s_ivec *tracks_to_pads, int Fc_output,
       int Fc_input, int Fc_pad, int nodes_per_chan, enum e_route_type 
       route_type, enum e_switch_block_type switch_block_type) {

/* Does the actual work of allocating the rr_graph and filling all the *
 * appropriate values.  Everything up to this was just a prelude!      */

 int i, j;

/* Allocate storage for the graph nodes.  Storage for the edges will be *
 * allocated as I fill in the graph.                                    */

 if (rr_mem_chunk_list_head != NULL) {
    printf("Error in alloc_and_load_rr_graph:  rr_mem_chunk_list_head = %p.\n",
            rr_mem_chunk_list_head);
    printf("Expected NULL.  It appears an old rr_graph has not been freed.\n");
    exit (1);
 }

 alloc_and_load_net_rr_terminals (rr_node_indices, nodes_per_chan);

 rr_node = (struct s_rr_node *) my_malloc (num_rr_nodes * sizeof (struct 
               s_rr_node));
 rr_node_cost_inf = (struct s_rr_node_cost_inf *) my_malloc (num_rr_nodes *
               sizeof (struct s_rr_node_cost_inf));
 rr_node_draw_inf = (struct s_rr_node_draw_inf *) my_malloc (num_rr_nodes *
               sizeof (struct s_rr_node_draw_inf));

 for (i=0;i<=nx+1;i++) {
    for (j=0;j<=ny+1;j++) {

       if (clb[i][j].type == CLB) {
          build_rr_clb (rr_node_indices, Fc_output, clb_opin_to_tracks, 
                 nodes_per_chan, i, j);
          build_rr_xchan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type);
          build_rr_ychan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type);
       }

       else if (clb[i][j].type == IO) {
          build_rr_pads (rr_node_indices, Fc_pad, pads_to_tracks,
                  nodes_per_chan, i, j);

          if (j == 0)  /* Bottom-most channel. */
             build_rr_xchan (rr_node_indices, route_type, tracks_to_clb_ipin, 
                    tracks_to_pads, i, j, nodes_per_chan, switch_block_type);

          if (i == 0)  /* Left-most channel.   */
             build_rr_ychan (rr_node_indices, route_type, tracks_to_clb_ipin,
                 tracks_to_pads, i, j, nodes_per_chan, switch_block_type);
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
 free (rr_node_cost_inf);
 free (rr_node_draw_inf);
}


static void alloc_and_load_net_rr_terminals (int **rr_node_indices, 
        int nodes_per_chan) {

/* Allocates and loads the net_rr_terminals data structure.  For each net   *
 * it stores the rr_node index of the SOURCE of the net and all the SINKs   *
 * of the net.  [0..num_nets-1][0..num_pins-1].  Entry [inet][pnum] stores  *
 * the rr index corresponding to the SOURCE (opin) or SINK (ipin) of pnum.  */

 int inet, ipin, inode, iblk, i, j, iclass;
 t_rr_type rr_type;

 net_rr_terminals = (int **) my_malloc (num_nets * sizeof(int *));

 for (inet=0;inet<num_nets;inet++) {
    net_rr_terminals[inet] = (int *) my_chunk_malloc (net[inet].num_pins * 
              sizeof (int), &rr_mem_chunk_list_head, &chunk_bytes_avail,
              &chunk_next_avail_mem);
    
    rr_type = SOURCE;     /* First pin only */
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       iblk = net[inet].pins[ipin];
       i = block[iblk].x;
       j = block[iblk].y;

       if (clb[i][j].type == CLB) 
          iclass = net_pin_class[inet][ipin];
       else 
          iclass = which_io_block (iblk);

       inode = get_rr_node_index (i, j, rr_type, iclass, nodes_per_chan, 
                  rr_node_indices);
       net_rr_terminals[inet][ipin] = inode;
 
       rr_type = SINK;    /* All pins after first are SINKs. */
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
       clb_opin_to_tracks, int nodes_per_chan, int i, int j) {

/* Load up the rr_node structures for the clb at location (i,j).  I both  *
 * fill in fields that shouldn't change during the entire routing and     *
 * initialize fields that will change to the proper starting value.       */

 int ipin, itrack, iclass, inode, iside, pin_num, to_node, num_edges;
 int iconn, tr_i, tr_j, next_edge;
 t_rr_type to_rr_type;

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

       for (ipin=0;ipin<class_inf[iclass].num_pins;ipin++) {
          pin_num = class_inf[iclass].pinlist[ipin];
          to_node = get_rr_node_index (i, j, OPIN, pin_num, nodes_per_chan,
                  rr_node_indices);
          rr_node[inode].edges[ipin] = to_node;
       }

       rr_node_cost_inf[inode].capacity = class_inf[iclass].num_pins;
       rr_node_cost_inf[inode].base_cost = 1.;   /* Test !! */
       rr_node_draw_inf[inode].type = SOURCE;
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
       rr_node_cost_inf[inode].capacity = class_inf[iclass].num_pins;
       rr_node_cost_inf[inode].base_cost = 0.;   /* Test !! */
       rr_node_draw_inf[inode].type = SINK;
    }

/* Things common to both SOURCEs and SINKs.   */

/*    rr_node_cost_inf[inode].base_cost = 1.; */
    rr_node_cost_inf[inode].acc_cost = 0.;
    rr_node_cost_inf[inode].occ = 0;

    rr_node[inode].prev_node = NO_PREVIOUS;
    rr_node[inode].cost = rr_node_cost_inf[inode].base_cost;
    rr_node[inode].path_cost = HUGE_FLOAT;
    rr_node[inode].target_flag = FALSE;
    rr_node[inode].xlow = i;
    rr_node[inode].xhigh = i;
    rr_node[inode].ylow = j;
    rr_node[inode].yhigh = j;

    rr_node_draw_inf[inode].ptc_num = iclass;
 }

/* Now do the pins.  */

 for (ipin=0;ipin<pins_per_clb;ipin++) {
    iclass = clb_pin_class[ipin];
    if (class_inf[iclass].type == DRIVER) {  /* OPIN */
       inode = get_rr_node_index (i, j, OPIN, ipin, nodes_per_chan,
                 rr_node_indices);

       num_edges = 0;
       for (iside=0;iside<=3;iside++) {
          if (clb_opin_to_tracks[ipin][iside][0] != OPEN) {
             num_edges += Fc_output;
          }
       }

       rr_node[inode].num_edges = num_edges;
       rr_node[inode].edges = (int *) my_chunk_malloc (num_edges * 
              sizeof (int), &rr_mem_chunk_list_head, &chunk_bytes_avail,
              &chunk_next_avail_mem);

       next_edge = 0;
       for (iside=0;iside<=3;iside++) {
          if (clb_opin_to_tracks[ipin][iside][0] != OPEN) {

/* This track may be at a different (i,j) location than the clb.  Tracks *
 * above and to the right of a clb are at the same (i,j) -- the channels *
 * on the other two sides "belong" to different clbs.                    */
 
             if (iside == BOTTOM)
                tr_j = j-1;
             else
                tr_j = j;

             if (iside == LEFT)
                tr_i = i-1;
             else
                tr_i = i;

             if (iside == LEFT || iside == RIGHT) 
                to_rr_type = CHANY;
             else 
                to_rr_type = CHANX;

             for (iconn=0;iconn<Fc_output;iconn++) {
                itrack = clb_opin_to_tracks[ipin][iside][iconn];

                to_node = get_rr_node_index (tr_i, tr_j, to_rr_type, itrack, 
                        nodes_per_chan, rr_node_indices);
                rr_node[inode].edges[next_edge] = to_node;
                next_edge++;
             }
                
          }
       }

       rr_node_cost_inf[inode].base_cost = 1.;    /* Test !! */
       rr_node_draw_inf[inode].type = OPIN;
    }
    else {                                   /* IPIN */
       inode = get_rr_node_index (i, j, IPIN, ipin, nodes_per_chan,
                 rr_node_indices);

       rr_node[inode].num_edges = 1;
       rr_node[inode].edges = (int *) my_chunk_malloc (sizeof(int), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);

       to_node = get_rr_node_index (i, j, SINK, iclass, nodes_per_chan,
                  rr_node_indices);
       rr_node[inode].edges[0] = to_node;

       rr_node_cost_inf[inode].base_cost = 0.95;    /* Test !! */
       rr_node_draw_inf[inode].type = IPIN;
    }

/* Things that are common to both OPINs and IPINs.  */

    rr_node_cost_inf[inode].capacity = 1;
/*    rr_node_cost_inf[inode].base_cost = 1.; */
    rr_node_cost_inf[inode].acc_cost = 0.;
    rr_node_cost_inf[inode].occ = 0;
 
    rr_node[inode].prev_node = NO_PREVIOUS;
    rr_node[inode].cost = rr_node_cost_inf[inode].base_cost;
    rr_node[inode].path_cost = HUGE_FLOAT;
    rr_node[inode].target_flag = FALSE;
    rr_node[inode].xlow = i;
    rr_node[inode].xhigh = i;
    rr_node[inode].ylow = j;
    rr_node[inode].yhigh = j;
 
    rr_node_draw_inf[inode].ptc_num = ipin;
 }
}


static void build_rr_pads (int **rr_node_indices, int Fc_pad, 
          int **pads_to_tracks, int nodes_per_chan, int i, int j) {

/* Load up the rr_node structures for the pads at location (i,j).  I both *
 * fill in fields that shouldn't change during the entire routing and     *
 * initialize fields that will change to the proper starting value.       *
 * Empty pad locations have their type set to EMPTY.                      */

 int s_node, p_node, ipad, iblk, iconn, chan_i, chan_j, to_node, iloop;
 int itrack, inode;
 t_rr_type chan_type;

/* Find location of adjacent channel. */

 if (j == 0) {                 /* Bottom row of pads. */
    chan_i = i;
    chan_j = j;
    chan_type = CHANX;
 }
 else if (j == ny+1) {
    chan_i = i;
    chan_j = j-1;
    chan_type = CHANX;
 }
 else if (i == 0) {
    chan_i = i;
    chan_j = j;
    chan_type = CHANY;
 }
 else if (i == nx+1) {
    chan_i = i-1;
    chan_j = j;
    chan_type = CHANY;
 }
 else {
    printf ("Error in build_rr_pads:  requested IO block at (%d,%d) does "
            "not exist.\n", i, j);
    exit (1);
 }

/* Do occupied pads first. */

 for (ipad=0;ipad<clb[i][j].occ;ipad++) {
    iblk = clb[i][j].u.io_blocks[ipad];

    if (block[iblk].type == INPAD) {

      /* Do SOURCE first.   */

       s_node = get_rr_node_index (i, j, SOURCE, ipad, nodes_per_chan,
                rr_node_indices);
          
       rr_node[s_node].num_edges = 1;
       p_node = get_rr_node_index (i, j, OPIN, ipad, nodes_per_chan, 
                 rr_node_indices);
       rr_node[s_node].edges = (int *) my_chunk_malloc (sizeof(int), 
           &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
       rr_node[s_node].edges[0] = p_node;
    
       rr_node_cost_inf[s_node].base_cost = 1.;  /* Test !! */
       rr_node_draw_inf[s_node].type = SOURCE;

      /* Now do OPIN */

       rr_node[p_node].num_edges = Fc_pad;
       rr_node[p_node].edges = (int *) my_chunk_malloc (Fc_pad *
            sizeof (int), &rr_mem_chunk_list_head, &chunk_bytes_avail, 
            &chunk_next_avail_mem);
          
       for (iconn=0;iconn<Fc_pad;iconn++) {
          itrack = pads_to_tracks[ipad][iconn];
          to_node = get_rr_node_index (chan_i, chan_j, chan_type, itrack,
                    nodes_per_chan, rr_node_indices);
          rr_node[p_node].edges[iconn] = to_node;
       }

       rr_node_cost_inf[p_node].base_cost = 1.;  /* Test !! */
       rr_node_draw_inf[p_node].type = OPIN;
    }

    else {         /* OUTPAD */

      /* Do SINK first. */

       s_node = get_rr_node_index (i, j, SINK, ipad, nodes_per_chan,
                rr_node_indices);

       rr_node[s_node].num_edges = 0;
       rr_node[s_node].edges = NULL;

       rr_node_cost_inf[s_node].base_cost = 0.;  /* Test !! */
       rr_node_draw_inf[s_node].type = SINK;

     /* Now do IPIN */
    
       p_node = get_rr_node_index (i, j, IPIN, ipad, nodes_per_chan,
                rr_node_indices);

       rr_node[p_node].num_edges = 1;
       rr_node[p_node].edges = (int *) my_chunk_malloc (sizeof(int), 
           &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
       rr_node[p_node].edges[0] = s_node;

       rr_node_cost_inf[p_node].base_cost = 0.95;  /* Test !! */
       rr_node_draw_inf[p_node].type = IPIN;
    }

/* Code common to INPADs and OUTPADs. */

    inode = s_node;
    for (iloop=1;iloop<=2;iloop++) {    /* for both SOURCE or SINK and pin */
/*       rr_node_cost_inf[inode].base_cost = 1.;  */
       rr_node_cost_inf[inode].acc_cost = 0.;
       rr_node_cost_inf[inode].occ = 0;
       rr_node_cost_inf[inode].capacity = 1;

       rr_node[inode].prev_node = NO_PREVIOUS;
       rr_node[inode].cost = rr_node_cost_inf[inode].base_cost;
       rr_node[inode].path_cost = HUGE_FLOAT;
       rr_node[inode].target_flag = FALSE;
       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;

       rr_node_draw_inf[inode].ptc_num = ipad;

       inode = p_node;
    }
 }

/* Now do empty pad locations. */

 for (ipad=clb[i][j].occ;ipad<io_rat;ipad++) {   

/* Could use type = SOURCE or SINK.  Use SINK arbitrarily.  Similarly, *
 * the choice of IPIN is arbitrary.                                    */

    s_node = get_rr_node_index (i, j, SINK, ipad, nodes_per_chan,
             rr_node_indices);
    p_node = get_rr_node_index (i, j, IPIN, ipad, nodes_per_chan, 
             rr_node_indices);

    inode = s_node;
    for (iloop=1;iloop<=2;iloop++) {    /* For both SOURCE or SINK and pin */

/* This node is empty and doesn't connect into the rest of the routing     *
 * graph.  The router will never get to it; I just allocate stoarge for it *
 * to keep the calculation of rr_node indices simple and I fill it in for  *
 * consistency.                                                            */

       rr_node[inode].num_edges = 0;
       rr_node[inode].edges = NULL;

       rr_node_draw_inf[inode].type = EMPTY_PAD;
       rr_node_cost_inf[inode].base_cost = 1.;
       rr_node_cost_inf[inode].acc_cost = 0.;
       rr_node_cost_inf[inode].occ = 0;
       rr_node_cost_inf[inode].capacity = 0;

       rr_node[inode].prev_node = NO_PREVIOUS;
       rr_node[inode].cost = 1.;
       rr_node[inode].path_cost = HUGE_FLOAT;
       rr_node[inode].target_flag = FALSE;
       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;
 

       rr_node_draw_inf[inode].ptc_num = ipad;
        
       inode = p_node;
    }
 }
}


static void build_rr_xchan (int **rr_node_indices, enum e_route_type 
      route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
      tracks_to_pads, int i, int j, int nodes_per_chan, enum 
      e_switch_block_type switch_block_type) {

/* Loads up all the routing resource nodes in the x-directed channel     *
 * segments located at (i,j).                                            */

 int itrack, num_edges, next_edge, inode;

 for (itrack=0;itrack<nodes_per_chan;itrack++) {

/* First count number of edges. */

    if (j == 0) {                   /* Between bottom pads and clbs.  */
       num_edges = tracks_to_clb_ipin[itrack][TOP].nelem;
       num_edges += num_track_to_pad_conn (i, j, itrack, tracks_to_pads);
       num_edges += 2;           /* 2 adjacent ychans */
    }

    else if (j == ny) {           /* Between top clbs and pads. */
       num_edges = tracks_to_clb_ipin[itrack][BOTTOM].nelem;
       num_edges += num_track_to_pad_conn (i, j+1, itrack, tracks_to_pads);
       num_edges += 2;           /* 2 adjacent ychans */
    }

    else {                          /* Between two rows of clbs. */
       num_edges = tracks_to_clb_ipin[itrack][BOTTOM].nelem;
       num_edges += tracks_to_clb_ipin[itrack][TOP].nelem;
       num_edges += 4;           /* 4 adjacent ychans */
    }
    
    if (i != 1) 
       num_edges++;      /* x-chan to left exists. */

    if (i != nx)
       num_edges++;      /* x-chan to right exists. */

    inode = get_rr_node_index (i, j, CHANX, itrack, nodes_per_chan, 
             rr_node_indices);

    rr_node[inode].num_edges = num_edges;
    rr_node[inode].edges = (int *) my_chunk_malloc (num_edges * sizeof(int), 
          &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    
/* Now load the edge array.   */

    next_edge = 0;
    if (j == 0) {                   /* Between bottom pads and clbs.  */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   TOP, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_pad_edges (inode, i, j, itrack, next_edge,
                   tracks_to_pads, nodes_per_chan, rr_node_indices); 

      /* 2 adjacent ychans (above) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i, j+1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i-1, j+1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;
    }

    else if (j == ny) {           /* Between top clbs and pads. */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   BOTTOM, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_pad_edges (inode, i, j+1, itrack, next_edge,
                   tracks_to_pads, nodes_per_chan, rr_node_indices); 

      /* 2 adjacent ychans (below) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i, j, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i-1, j, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;
    }

    else {                          /* Between two rows of clbs. */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   BOTTOM, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   TOP, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);

      /* 4 adjacent ychans (above and below) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i, j, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i-1, j, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i, j+1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i-1, j+1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }
    
    if (i != 1) {    /* x-chan to left exists. */
       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i-1, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }

    if (i != nx) {   /* x-chan to right exists. */
       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANX, i+1, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }

/* Edge arrays have now been built up.  Do everything else.  */

       rr_node_cost_inf[inode].base_cost = 1.;
       rr_node_cost_inf[inode].acc_cost = 0.;
       rr_node_cost_inf[inode].occ = 0;

       if (route_type == DETAILED) 
          rr_node_cost_inf[inode].capacity = 1;
       else                                    /* GLOBAL routing */
          rr_node_cost_inf[inode].capacity = chan_width_x[j];
 
       rr_node[inode].prev_node = NO_PREVIOUS;
       rr_node[inode].cost = rr_node_cost_inf[inode].base_cost;
       rr_node[inode].path_cost = HUGE_FLOAT;
       rr_node[inode].target_flag = FALSE;
       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;
 
       rr_node_draw_inf[inode].ptc_num = itrack;
       rr_node_draw_inf[inode].type = CHANX;
 }
}


static void build_rr_ychan (int **rr_node_indices, enum e_route_type 
      route_type, struct s_ivec **tracks_to_clb_ipin, struct s_ivec *
      tracks_to_pads, int i, int j, int nodes_per_chan, enum
      e_switch_block_type switch_block_type) {

/* Loads up all the routing resource nodes in the y-directed channel     *
 * segments located at (i,j).                                            */

 int itrack, num_edges, next_edge, inode;

 for (itrack=0;itrack<nodes_per_chan;itrack++) {

/* First count number of edges. */

    if (i == 0) {                   /* Between leftmost pads and clbs.  */
       num_edges = tracks_to_clb_ipin[itrack][RIGHT].nelem;
       num_edges += num_track_to_pad_conn (i, j, itrack, tracks_to_pads);
       num_edges += 2;           /* 2 adjacent xchans */
    }

    else if (i == nx) {           /* Between right clbs and pads. */
       num_edges = tracks_to_clb_ipin[itrack][LEFT].nelem;
       num_edges += num_track_to_pad_conn (i+1, j, itrack, tracks_to_pads);
       num_edges += 2;           /* 2 adjacent xchans */
    }

    else {                          /* Between two rows of clbs. */
       num_edges = tracks_to_clb_ipin[itrack][LEFT].nelem;
       num_edges += tracks_to_clb_ipin[itrack][RIGHT].nelem;
       num_edges += 4;           /* 4 adjacent xchans */
    }
    
    if (j != 1) 
       num_edges++;      /* y-chan below exists. */

    if (j != ny)
       num_edges++;      /* y-chan above exists. */

    inode = get_rr_node_index (i, j, CHANY, itrack, nodes_per_chan, 
             rr_node_indices);

    rr_node[inode].num_edges = num_edges;
    rr_node[inode].edges = (int *) my_chunk_malloc (num_edges * sizeof(int), 
         &rr_mem_chunk_list_head, &chunk_bytes_avail, &chunk_next_avail_mem);
    
/* Now load the edge array.   */

    next_edge = 0;
    if (i == 0) {                   /* Between left pads and clbs.  */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   RIGHT, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_pad_edges (inode, i, j, itrack, next_edge,
                   tracks_to_pads, nodes_per_chan, rr_node_indices); 

      /* 2 adjacent xchans (to right) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i+1, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i+1, j-1, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;
    }

    else if (i == nx) {           /* Between rightmost clbs and pads. */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   LEFT, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_pad_edges (inode, i+1, j, itrack, next_edge,
                   tracks_to_pads, nodes_per_chan, rr_node_indices); 

      /* 2 adjacent xchans (to the left) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j-1, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;
    }

    else {                          /* Between two rows of clbs. */
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   LEFT, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);
       next_edge = add_track_to_clb_ipin_edges (inode, i, j, itrack,
                   RIGHT, next_edge, tracks_to_clb_ipin, nodes_per_chan,
                   rr_node_indices);

      /* 4 adjacent xchans (to left and to right) */

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j-1, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i+1, j, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++;

       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i+1, j-1, CHANX, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }
    
    if (j != 1) {    /* y-chan below exists. */
       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j-1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }

    if (j != ny) {   /* y-chan above exists. */
       rr_node[inode].edges[next_edge] = add_switch_box_edge (i, j, itrack,
                 CHANY, i, j+1, CHANY, route_type, switch_block_type, 
                 nodes_per_chan, rr_node_indices);
       next_edge++; 
    }

/* Edge arrays have now been built up.  Do everything else.  */

       rr_node_cost_inf[inode].base_cost = 1.;
       rr_node_cost_inf[inode].acc_cost = 0.;
       rr_node_cost_inf[inode].occ = 0;

       if (route_type == DETAILED) 
          rr_node_cost_inf[inode].capacity = 1;
       else                                    /* GLOBAL routing */
          rr_node_cost_inf[inode].capacity = chan_width_y[i];
 
       rr_node[inode].prev_node = NO_PREVIOUS;
       rr_node[inode].cost = rr_node_cost_inf[inode].base_cost;
       rr_node[inode].path_cost = HUGE_FLOAT;
       rr_node[inode].target_flag = FALSE;
       rr_node[inode].xlow = i;
       rr_node[inode].xhigh = i;
       rr_node[inode].ylow = j;
       rr_node[inode].yhigh = j;
 
       rr_node_draw_inf[inode].ptc_num = itrack;
       rr_node_draw_inf[inode].type = CHANY;
 }
}


static int add_switch_box_edge (int from_i, int from_j, int from_track,
             t_rr_type from_type, int to_i, int to_j, t_rr_type
             to_type, enum e_route_type route_type, enum e_switch_block_type
             switch_block_type, int nodes_per_chan, int **rr_node_indices) {

/* This routine returns the index of the rr_node to which the desired *
 * track should connect.                                              */

 int to_track, to_node;

 if (route_type == GLOBAL) {
    to_track = 0;
 }

 else if (switch_block_type == SUBSET) {  
    to_track = from_track;
 }

/* See S. Wilton Phd thesis, U of T, 1996 p. 103 for details on following. */

 else if (switch_block_type == WILTON) {

    if (from_type == CHANX) {
       if (to_type == CHANX) {
          to_track = from_track;
       }
       else {     /* from CHANX to CHANY */

     /* Four possible cases.  Figure out which one we're in. */

          if (from_i > to_i) {
             if (from_j < to_j) {   /* t2 to t1 (RIGHT to TOP) connection. */
                to_track = (nodes_per_chan + from_track - 1) % nodes_per_chan;
             }
             else {               /* t2 to t3 (RIGHT to BOTTOM) connection */
                to_track = (2 * nodes_per_chan - 2 - from_track) %
                            nodes_per_chan;
             }
          }
          else {
             if (from_j < to_j) {   /* t0 to t1 (LEFT to TOP) connection */
                to_track = (nodes_per_chan - from_track) % nodes_per_chan;
             }
             else {              /* t0 to t3 (LEFT to BOTTOM) connection */
                to_track = (nodes_per_chan + from_track -1) % nodes_per_chan;
             }
          }
       }
    }

    else {    /* from_type == CHANY */
       if (to_type == CHANY) {
          to_track = from_track;
       }
       else {     /* from CHANY to CHANX */

     /* Four possible cases.  Figure out which one we're in. */ 
 
          if (from_j > to_j) {
             if (from_i < to_i) {   /* t1 to t2 (TOP to RIGHT) connection */ 
                to_track = (from_track + 1) % nodes_per_chan;
             }
             else {               /* t1 to t0 (TOP to LEFT) connection */
                to_track = (nodes_per_chan - from_track) % nodes_per_chan; 
             } 
          }  
          else {    
             if (from_i < to_i) {   /* t3 to t2 (BOTTOM to RIGHT) connection */
                to_track = (2 * nodes_per_chan - 2 - from_track) % 
                           nodes_per_chan;
             } 
             else {              /* t3 to t0 (BOTTOM to LEFT) connection */ 
                to_track = (from_track + 1) % nodes_per_chan; 
             } 
          }  

       }
    }
 }    /* End switch_block_type == WILTON case. */

 else if (switch_block_type == UNIVERSAL) {

    if (from_type == CHANX) {
       if (to_type == CHANX) {
          to_track = from_track;
       }
       else {   /* from CHANX to CHANY */

          if (from_i > to_i) {
             if (from_j < to_j) {   /* t2 to t1 (RIGHT to TOP) connection. */
                to_track = from_track; 
             }   
             else {               /* t2 to t3 (RIGHT to BOTTOM) connection */
                to_track = nodes_per_chan - 1 - from_track;
             }   
          }
          else { 
             if (from_j < to_j) {   /* t0 to t1 (LEFT to TOP) connection */
                to_track = nodes_per_chan - 1 - from_track;
             }   
             else {              /* t0 to t3 (LEFT to BOTTOM) connection */
                to_track = from_track; 
             }   
          }
       }
    }

    else {     /* from_type == CHANY */

       if (to_type == CHANY) {
          to_track = from_track;
       }
       else {   /* from CHANY to CHANX */

          if (from_j > to_j) {
             if (from_i < to_i) {   /* t1 to t2 (TOP to RIGHT) connection */ 
                to_track = from_track;
             }
             else {               /* t1 to t0 (TOP to LEFT) connection */
                to_track = nodes_per_chan - 1 - from_track; 
             } 
          }  
          else {    
             if (from_i < to_i) {   /* t3 to t2 (BOTTOM to RIGHT) connection */
                to_track = nodes_per_chan - 1 - from_track; 
             } 
             else {              /* t3 to t0 (BOTTOM to LEFT) connection */ 
                to_track = from_track;
             } 
          }  
       }
    }
 }    /* End switch_block_type == UNIVERSAL case. */

 else {
    printf("Error in add_switch_box_edge.  Unknown switch_block_type: %d.\n",
            switch_block_type);
    exit (1);
 }

 to_node = get_rr_node_index (to_i, to_j, to_type, to_track, nodes_per_chan,
             rr_node_indices);
 return (to_node);
}


static int add_track_to_clb_ipin_edges (int tr_node, int tr_i, int tr_j,
            int itrack, int iside, int start_conn, struct s_ivec **
            tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices) {

/* This routine adds all the edges from tr_node, which is track itrack  *
 * at (tr_i, tr_j), to the clb on one side of it, where the side is     *
 * given by iside.  start_conn is the index at which it should add the  *
 * first edge.  The routine returns the index of the next free edge.    */

 int iconn, clb_i, clb_j, max_conn, to_node, ipin;

/* Side is from the *track's* perspective */

 if (iside == BOTTOM || iside == LEFT) {  
    clb_i = tr_i;
    clb_j = tr_j;
 }
 else if (iside == TOP) {
    clb_i = tr_i;
    clb_j = tr_j + 1;
 }
 else if (iside == RIGHT) {
    clb_i = tr_i + 1;
    clb_j = tr_j;
 }
 else {
    printf ("Error in add_track_to_clb_ipin_edges:  Unknown iside: %d.\n",
            iside);
    exit (1);
 }
  
 max_conn = tracks_to_clb_ipin[itrack][iside].nelem;

 for (iconn=0;iconn<max_conn;iconn++) {
    ipin = tracks_to_clb_ipin[itrack][iside].list[iconn];
    to_node = get_rr_node_index (clb_i, clb_j, IPIN, ipin, nodes_per_chan,
               rr_node_indices);
    rr_node[tr_node].edges[start_conn + iconn] = to_node;
 }

 return (max_conn + start_conn);
}


static int num_track_to_pad_conn (int pad_i, int pad_j, int itrack,
        struct s_ivec *tracks_to_pads) {

/* Returns the number of connections from this track to the set of pads   *
 * bordering on it.  Note that connections are only made to OCCUPIED      *
 * OUTPADs.                                                               */

 int iconn, num_conn, ipad, iblk;

 num_conn = 0;
 for (iconn=0;iconn<tracks_to_pads[itrack].nelem;iconn++) {
    ipad = tracks_to_pads[itrack].list[iconn];
    if (ipad < clb[pad_i][pad_j].occ) {
       iblk = clb[pad_i][pad_j].u.io_blocks[ipad];
       if (block[iblk].type == OUTPAD)
          num_conn++;
    }
 }

 return (num_conn);
}


static int add_track_to_pad_edges (int tr_node, int pad_i, int pad_j,
            int itrack, int start_conn, struct s_ivec *tracks_to_pads,
            int nodes_per_chan, int **rr_node_indices) {

/* Adds all the connections to OUTPADs that this track should have.  The   *
 * first connection will go into the edges array at index start_conn.  The *
 * index of the next free edge element is returned by the routine.         */

 int iconn, ipad, next_conn, iblk, to_node;

 next_conn = start_conn;

 for (iconn=0;iconn<tracks_to_pads[itrack].nelem;iconn++) {
    ipad = tracks_to_pads[itrack].list[iconn];
    if (ipad < clb[pad_i][pad_j].occ) {
       iblk = clb[pad_i][pad_j].u.io_blocks[ipad];
       if (block[iblk].type == OUTPAD) {
          to_node = get_rr_node_index (pad_i, pad_j, IPIN, ipad, 
                    nodes_per_chan, rr_node_indices);
          rr_node[tr_node].edges[next_conn] = to_node;
          next_conn++;
       }
    }    
 }
 
 return (next_conn);
}


static int get_rr_node_index (int i, int j, t_rr_type rr_type, int ioff, 
        int nodes_per_chan, int **rr_node_indices) {

/* Returns the index of the specified routing resource node.  (i,j) are     *
 * the location within the FPGA, rr_type specifies the type of resource,    *
 * and ioff gives the number of this resource.  ioff is the class number,   *
 * pin number or track number, depending on what type of resource this      *
 * is.  All ioffs start at 0 and go up to pins_per_clb-1 or the equivalent. *
 * The order within a clb is:  SOURCEs + SINKs (num_class of them); IPINs,  *
 * and OPINs (pins_per_clb of them); CHANX; and CHANY (nodes_per_chan of    *
 * each).  For (i,j) locations that point at pads the order is:  SOURCES or *
 * SINKs (io_rat of them); IPINs or OPINs (io_rat of them -- these are pad  *
 * pins); and any associated channel (if there is a channel at (i,j)).      *
 * This routine also performs error checking to make sure the node in       *
 * question exists.                                                         */

 int index, iclass, iblk;

 assert (ioff >= 0);
 assert (i >= 0 && i < nx + 2);
 assert (j >= 0 && j < ny + 2);

 index = rr_node_indices[i][j];  /* Start of that block */

 switch (clb[i][j].type) {

 case CLB:
    switch (rr_type) {

    case SOURCE: 
       assert (ioff < num_class);
       assert (class_inf[ioff].type == DRIVER);

       index += ioff;
       return (index);
       
    case SINK:
       assert (ioff < num_class);
       assert (class_inf[ioff].type == RECEIVER);
   
       index += ioff;
       return (index);

    case OPIN:
       assert (ioff < pins_per_clb);
       iclass = clb_pin_class[ioff];
       assert (class_inf[iclass].type == DRIVER);

       index += num_class + ioff;
       return (index);

    case IPIN:
       assert (ioff < pins_per_clb);
       iclass = clb_pin_class[ioff];
       assert (class_inf[iclass].type == RECEIVER);

       index += num_class + ioff;
       return (index);

    case CHANX:
       assert (ioff < nodes_per_chan);
       index += num_class + pins_per_clb + ioff;
       return (index);

    case CHANY:
       assert (ioff < nodes_per_chan);
       index += num_class + pins_per_clb + nodes_per_chan + ioff;
       return (index);

    default:
       printf ("Error:  Bad rr_node passed to get_rr_node_index.\n"
               "Request for type %d number %d at (%d, %d).\n", rr_type,
               ioff, i, j);
       exit (1);
    }
    break;

 case IO:
    switch (rr_type) {   

    case SOURCE:
       assert (ioff < io_rat);
       if (ioff < clb[i][j].occ) {           /* Not empty */
          iblk = clb[i][j].u.io_blocks[ioff];
           assert (block[iblk].type == INPAD);
       }

       index += ioff;
       return (index);

    case SINK:
       assert (ioff < io_rat);
       if (ioff < clb[i][j].occ) {            /* Not empty */
          iblk = clb[i][j].u.io_blocks[ioff];
           assert (block[iblk].type == OUTPAD);
       }

       index += ioff;
       return (index);

    case OPIN:
       assert (ioff < io_rat); 
       if (ioff < clb[i][j].occ) {            /* Not empty */
          iblk = clb[i][j].u.io_blocks[ioff];
           assert (block[iblk].type == INPAD);
       }

       index += io_rat + ioff; 
       return (index);
 
    case IPIN: 
       assert (ioff < io_rat); 
       if (ioff < clb[i][j].occ) {            /* Not empty */
          iblk = clb[i][j].u.io_blocks[ioff];
           assert (block[iblk].type == OUTPAD);
       }

       index += io_rat + ioff; 
       return (index);
 
    case CHANX: 
       assert (ioff < nodes_per_chan); 
       assert (j == 0);                 /* Only one with a channel. */
       index += 2 * io_rat + ioff;
       return (index);

    case CHANY:
       assert (ioff < nodes_per_chan); 
       assert (i == 0);                 /* Only one with a channel. */
       index += 2 * io_rat + ioff;      /* No x-chan here. */
       return (index);
 
    default: 
       printf ("Error:  Bad rr_node passed to get_rr_node_index.\n" 
               "Request for type %d number %d at (%d, %d).\n", rr_type, 
               ioff, i, j); 
       exit (1); 
    } 
    break;

 default:
    printf("Error in get_rr_node_index:  unexpected block type (%d) at "
           "(%d, %d).\n", clb[i][j].type, i, j);
    exit (1);
 }
}


static void free_ivec_vector (struct s_ivec *ivec_vector, int nrmin, 
         int nrmax) {

/* Frees a 1D array of integer vectors.                              */

 int i;
 
 for (i=nrmin;i<=nrmax;i++) 
    if (ivec_vector[i].nelem != 0)
       free (ivec_vector[i].list);

 free (ivec_vector + nrmin);
}


static void free_ivec_matrix (struct s_ivec **ivec_matrix, int nrmin, int
     nrmax, int ncmin, int ncmax) {

/* Frees a 2D matrix of integer vectors (ivecs).                     */

 int i, j;

 for (i=nrmin;i<=nrmax;i++) {
    for (j=ncmin;j<=ncmax;j++) {
       if (ivec_matrix[i][j].nelem != 0) {
          free (ivec_matrix[i][j].list);
       }
    }
 }

 free_matrix (ivec_matrix, nrmin, nrmax, ncmin, sizeof(struct s_ivec)); 
}


static int **alloc_and_load_rr_node_indices (int nodes_per_clb, 
    int nodes_per_pad, int nodes_per_chan) {

/* Loads a matrix containing the index of the *first* rr_node at that    *
 * (i,j) location.                                                       */

 int index, i, j;
 int **rr_node_indices;

 rr_node_indices = (int **) alloc_matrix (0, nx+1, 0, ny+1, sizeof(int));

 index = 0;

 for (i=0;i<=nx+1;i++) {
    for (j=0;j<=ny+1;j++) {
       rr_node_indices[i][j] = index;
       if (clb[i][j].type == CLB) {
          index += nodes_per_clb + 2 * nodes_per_chan;
       }
       else if (clb[i][j].type == IO) {
          index += nodes_per_pad;
          if (j == 0)    /* Bottom row */
             index += nodes_per_chan;
          if (i == 0)    /* Leftmost column */
             index += nodes_per_chan;
       }
       else if (clb[i][j].type != ILLEGAL) {
          printf("Error in alloc_and_load_rr_node_indices.  Unexpected clb"
                 " type.\n");
          exit (1);
       }
    }
 }

 if (rr_node_indices[nx+1][ny+1] != num_rr_nodes) {
    printf ("Error in alloc_and_load_rr_node_indices:  num_rr_nodes does \n"
            "not agree with index array.\n");
    exit (1);
 }
 return (rr_node_indices);
}


static int ***alloc_and_load_clb_pin_to_tracks (enum e_pin_type pin_type, 
          int nodes_per_chan, int Fc) {

/* Allocates and loads an array that contains a list of which tracks each  *
 * output or input pin of a clb should connect to on each side of the clb. *
 * The pin_type flag is either DRIVER or RECEIVER, and determines whether  *
 * information for output C blocks or input C blocks is generated.         *
 * Set Fc and nodes_per_chan to 1 if you're doing a global routing graph.  */


 int *num_dir;   /* [0..3] Number of *physical* pins on each clb side.    */
 int **dir_list; /* [0..3][0..pins_per_clb-1] list of output pins on each *
                  * side.  Max possible space allocated for simplicity.   */

 int i, j, iside, ipin, iclass, num_phys_pins, itrack, pindex;
 int *num_done_per_dir, *pin_num_ordering, *side_ordering;
 int ***tracks_connected_to_pin;  /* [0..pins_per_clb-1][0..3][0..Fc-1] */
 float step_size, f_track;

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

/* if (route_type == GLOBAL) {
    for (ipin=0;ipin<pins_per_clb;ipin++) {
       iclass = clb_pin_class[ipin];
       if (class_inf[iclass].type != pin_type) 
          continue;
       for (iside=0;iside<=3;iside++) {  */

/* GLOBAL routing only has only track 0. */

/*          if (pinloc[iside][ipin] == 1) 
             tracks_connected_to_pin[ipin][iside][0] = 0;  
       }
    }
    return (tracks_connected_to_pin);
 } */

/* Only DETAILED routing will reach here.  (Test to see if global works) */

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

/* Connection block I use distributes outputs evenly across the tracks   *
 * of ALL sides of the clb at once.  Ensures that each opin connects     *
 * to spaced out tracks in its connection block, and that the other      *
 * opins (potentially in other C blocks) connect to the remaining tracks *
 * first.  Doesn't matter for large Fc_output, but should make a fairly  *
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
 
/* Free all temporary storage. */

 free (num_dir);
 free_matrix (dir_list, 0, 3, 0, sizeof(int));
 free (num_done_per_dir);
 free (pin_num_ordering);
 free (side_ordering);

 return (tracks_connected_to_pin);
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
