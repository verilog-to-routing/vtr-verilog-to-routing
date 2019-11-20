#include <stdio.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph2.h"
#include "rr_graph_sbox.h"
#include <assert.h>


/* Variables below are the global variables shared only amongst the rr_graph *
 ************************************ routines. ******************************/

/* [0..num_rr_nodes-1].  TRUE if a node is already listed in the edges array *
 * that's being constructed.  This allows me to ensure that there are never  *
 *  duplicate edges (two edges between the same thing).                      */

boolean *rr_edge_done; 


/* Used to keep my own list of free linked integers, for speed reasons.     */

t_linked_edge *free_edge_list_head = NULL;



/*************************** Variables local to this module *****************/

/* Two arrays below give the rr_node_index of the channel segment at        *
 * (i,j,track) for fast index lookup.                                       */

static int ***chanx_rr_indices;  /* [1..nx][0..ny][0..nodes_per_chan-1] */
static int ***chany_rr_indices;  /* [0..nx][1..ny][0..nodes_per_chan-1] */
  


/************************** Subroutines local to this module ****************/

static int load_chanx_rr_indices (t_seg_details *seg_details_x, int 
           nodes_per_chan, int start_index, int i, int j); 

static int load_chany_rr_indices (t_seg_details *seg_details_y, int 
           nodes_per_chan, int start_index, int i, int j); 

static void get_switch_type (boolean is_from_sbox, boolean is_to_sbox, 
         short from_node_switch, short to_node_switch, short switch_types[2]); 


/******************** Subroutine definitions *******************************/

t_seg_details *alloc_and_load_seg_details (int nodes_per_chan, t_segment_inf 
              *segment_inf, int num_seg_types, int max_len) {
 
/* Allocates and loads the seg_details data structure.  Max_len gives the   *
 * maximum length of a segment (dimension of array).  The code below tries  *
 * to:                                                                      *
 * (1) stagger the start points of segments of the same type evenly;        *
 * (2) spread out the limited number of connection boxes or switch boxes    *
 *     evenly along the length of a segment, starting at the segment ends;  *
 * (3) stagger the connection and switch boxes on different long lines,     *
 *     as they will not be staggered by different segment start points.     */
 
 int tracks_left, i, next_track, ntracks, itrack, length, j, index;
 int wire_switch, opin_switch, num_cb, num_sb;
 float frac_left, start_step, sb_step, cb_step;
 float cb_off_step, sb_off_step;
 t_seg_details *seg_details;
 boolean longline;

 seg_details = (t_seg_details *) my_malloc (nodes_per_chan * sizeof 
                    (t_seg_details));

 tracks_left = nodes_per_chan;
 next_track = 0;
 frac_left = 1.;

 for (i=0;i<num_seg_types;i++) {
    if (frac_left <= 0.) {
       ntracks = 0;
    }
    else {
       ntracks = nint (segment_inf[i].frequency / frac_left * tracks_left);
    }
    frac_left -= segment_inf[i].frequency;
    tracks_left -= ntracks;

    if (ntracks == 0) 
       continue;      /* No tracks of this length.  Avoid divide by 0, etc. */

    longline = segment_inf[i].longline;
 
    if (!longline) {  
       length = min (segment_inf[i].length, max_len);
       start_step = (float) length / (float) ntracks;
    }
    else {                                 /* Is a long line. */
       length = max_len;
       start_step = 0.;
    }

    num_cb = nint (length * segment_inf[i].frac_cb);
    num_sb = nint ((length + 1.) * segment_inf[i].frac_sb);

/* Distribute connection boxes, with two endpoints covered first.           */

    if (!longline) {
       if (num_cb > 1) 
          cb_step = (float) (length-1) / (float) (num_cb - 1);
       else
          cb_step = 0.;
    }

/* Treat longlines differently.  Distribute C and S blocks as if the        *
 * longlines were circular (i.e., don't cover both endpoints first).  This  *
 * is important, as the rotation of the C and S blocks from line to line    *
 * would cause stupidities otherwise.                                       */

    else {   
       if (num_cb > 0)
          cb_step = (float) length / (float) num_cb;
       else
          cb_step = 0.;
    }


/* Now spead out switch boxes in the same way.     */

    if (!longline) {
       if (num_sb > 1) 
          sb_step = (float) length / (float) (num_sb - 1);
       else
          sb_step = 0.;
    }

    else {    /* Is a longline */
       if (num_sb > 0) 
          sb_step = (float) (length + 1) / (float) num_sb;
       else
          sb_step = 0;
    }

    if (!longline) {
       cb_off_step = 0.;
       sb_off_step = 0.;
    }

    else {                                 /* Is a long line. */
       cb_off_step = cb_step / (float) ntracks;
       sb_off_step = sb_step / (float) ntracks;
    }

    wire_switch = segment_inf[i].wire_switch;
    opin_switch = segment_inf[i].opin_switch;
 
    for (itrack=0;itrack<ntracks;itrack++) {
       seg_details[next_track].length = length;
       seg_details[next_track].longline = longline;
 
       seg_details[next_track].start = nint (itrack * start_step) % length + 1;
 
       seg_details[next_track].cb = (boolean *) my_calloc (length, 
                           sizeof (boolean));

       seg_details[next_track].sb = (boolean *) my_calloc ((length+1),
                            sizeof (boolean));

       for (j=0;j<num_cb;j++) {
          index = nint (j * cb_step + itrack * cb_off_step) % length;
          seg_details[next_track].cb[index] = TRUE;
       }    

       for (j=0;j<num_sb;j++) {
          index = nint (j * sb_step + itrack * sb_off_step) % (length + 1);
          seg_details[next_track].sb[index] = TRUE;
       }    
 
       seg_details[next_track].Rmetal = segment_inf[i].Rmetal;
       seg_details[next_track].Cmetal = segment_inf[i].Cmetal;

       seg_details[next_track].wire_switch = wire_switch;
       seg_details[next_track].opin_switch = opin_switch;

       seg_details[next_track].index = i;

       next_track++;
    }
 }   /* End for each segment type. */
 
 assert (tracks_left == 0);
 assert (next_track == nodes_per_chan);
 return (seg_details);
}
 
 
void free_seg_details (t_seg_details *seg_details, int nodes_per_chan) {
 
/* Frees all the memory allocated to an array of seg_details structures. */
 
 int i;
 
 for (i=0;i<nodes_per_chan;i++) {
    free (seg_details[i].cb);
    free (seg_details[i].sb);
 }
 free (seg_details);
}


void dump_seg_details (t_seg_details *seg_details, int nodes_per_chan, char 
           *fname) {

/* Dumps out an array of seg_details structures to file fname.  Used only   *
 * for debugging.                                                           */

 FILE *fp;
 int i, j;

 fp = my_fopen (fname, "w", 0);

 for (i=0;i<nodes_per_chan;i++) {
    fprintf (fp, "Track: %d.\n", i);
    fprintf (fp, "Length: %d,  Start: %d,  Long line: %d  wire_switch: %d  "
        "opin_switch: %d.\n", seg_details[i].length, seg_details[i].start, 
        seg_details[i].longline, seg_details[i].wire_switch, 
        seg_details[i].opin_switch);

    fprintf (fp, "Rmetal: %g  Cmetal: %g\n", seg_details[i].Rmetal, 
        seg_details[i].Cmetal);

    fprintf (fp, "cb list: ");
    for (j=0;j<seg_details[i].length;j++)
       fprintf (fp, "%d ", seg_details[i].cb[j]);

    fprintf (fp, "\nsb list: ");
    for (j=0;j<=seg_details[i].length;j++)
       fprintf (fp, "%d ", seg_details[i].sb[j]);

    fprintf (fp, "\n\n");
 } 

 fclose (fp);
}


int get_closest_seg_start (t_seg_details *seg_details, int itrack, int seg_num,
        int chan_num) {

/* Returns the segment number at which the segment this track lies on        *
 * started.                                                                  */

 int closest_start, length, start;

 if (seg_details[itrack].longline) {
    closest_start = 1;
 }
 else {
    length = seg_details[itrack].length;
    start = seg_details[itrack].start;

/* Start is guaranteed to be between 1 and length.  Hence adding length to *
 * the quantity in brackets below guarantees it will be nonnegative.       */

    closest_start = seg_num - (seg_num + chan_num - start + length) % length;
    closest_start = max (closest_start, 1);
 }

 return (closest_start);
}


int get_clb_opin_connections (int ***clb_opin_to_tracks, int ipin, int i, int 
        j, int Fc_output, t_seg_details *seg_details_x, t_seg_details 
        *seg_details_y, t_linked_edge **edge_list_ptr, int nodes_per_chan, int 
        **rr_node_indices) {

/* Returns the number of tracks to which clb opin #ipin at (i,j) connects.   *
 * Also stores the nodes to which this pin connects in the linked list       *
 * pointed to by *edge_list_ptr.                                             */

 int iside, num_conn, iconn, itrack, tr_j, tr_i, to_node;
 t_rr_type to_rr_type;
 boolean cbox_exists;
 t_linked_edge *edge_list_head;
 t_seg_details *seg_details;

 edge_list_head = *edge_list_ptr;
 num_conn = 0;

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
 
       if (iside == LEFT || iside == RIGHT) {
          to_rr_type = CHANY;
          seg_details = seg_details_y;
       }
       else {
          to_rr_type = CHANX;
          seg_details = seg_details_x;
       }
                 
       for (iconn=0;iconn<Fc_output;iconn++) {
          itrack = clb_opin_to_tracks[ipin][iside][iconn]; 
 
          if (to_rr_type == CHANX) 
             cbox_exists = is_cbox (tr_i, tr_j, itrack, seg_details);
          else
             cbox_exists = is_cbox (tr_j, tr_i, itrack, seg_details);

          if (cbox_exists) {
             to_node = get_rr_node_index (tr_i, tr_j, to_rr_type, itrack,
                           nodes_per_chan, rr_node_indices);
             edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                       seg_details[itrack].opin_switch, &free_edge_list_head);
             num_conn++;
          }
       }
           
    }
 }

 if (num_conn == 0) {
    printf ("Error:  clb output pin %d at (%d,%d) does not connect to any "
            "tracks.\n", ipin, i, j);
    exit (1);
 }

 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


int get_pad_opin_connections (int **pads_to_tracks, int ipad, int i, int j, 
        int Fc_pad, t_seg_details *seg_details_x, t_seg_details *seg_details_y, 
        t_linked_edge **edge_list_ptr, int nodes_per_chan, int 
        **rr_node_indices) {

/* Returns the number of tracks to which the pad opin at (i,j) connects.     *
 * If edge_ptr isn't NULL, it also loads the edge array.                     */

 int chan_i, chan_j, iconn, itrack, num_conn, to_node;
 t_rr_type chan_type;
 boolean cbox_exists;
 t_linked_edge *edge_list_head;
 t_seg_details *seg_details;

/* Find location of adjacent channel. */

 if (j == 0) {                 /* Bottom row of pads. */
    chan_i = i;
    chan_j = j;
    chan_type = CHANX;
    seg_details = seg_details_x;
 }
 else if (j == ny+1) {
    chan_i = i;
    chan_j = j-1;
    chan_type = CHANX;
    seg_details = seg_details_x;
 }
 else if (i == 0) {
    chan_i = i;
    chan_j = j;
    chan_type = CHANY;
    seg_details = seg_details_y;
 }
 else if (i == nx+1) {
    chan_i = i-1;
    chan_j = j;
    chan_type = CHANY;
    seg_details = seg_details_y;
 }
 else {
    printf ("Error in get_pad_opin_connections:  requested IO block at "
            "(%d,%d) does not exist.\n", i, j);
    exit (1);
 }

 edge_list_head = *edge_list_ptr;
 num_conn = 0;

 for (iconn=0;iconn<Fc_pad;iconn++) {
    itrack = pads_to_tracks[ipad][iconn];

    if (chan_type == CHANX)
       cbox_exists = is_cbox (chan_i, chan_j, itrack, seg_details);
    else
       cbox_exists = is_cbox (chan_j, chan_i, itrack, seg_details);

    if (cbox_exists) {
       to_node = get_rr_node_index (chan_i, chan_j, chan_type, itrack,
             nodes_per_chan, rr_node_indices);
       edge_list_head = insert_in_edge_list (edge_list_head, to_node, 
                       seg_details[itrack].opin_switch, &free_edge_list_head);
       num_conn++;
    }
 } 

 if (num_conn == 0) {
    printf ("Error:  INPAD %d at (%d,%d) does not connect to any "
            "tracks.\n", ipad, i, j);
    exit (1);
 }

 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


boolean is_cbox (int seg_num, int chan_num, int itrack, t_seg_details 
       *seg_details) {

/* Returns 1 (TRUE) if the track segment at this segment and channel         *
 * location with track number itrack contains a connection box, 0 otherwise. */

 int seg_offset, start, length;

 length = seg_details[itrack].length;
 start = seg_details[itrack].start;

 seg_offset = (seg_num + chan_num - start + length) % length;
 return (seg_details[itrack].cb[seg_offset]);
}


int **alloc_and_load_rr_node_indices (int nodes_per_clb,
    int nodes_per_pad, int nodes_per_chan, t_seg_details *seg_details_x,
    t_seg_details *seg_details_y) {

/* Allocates and loads all the structures needed for fast lookups of the   *
 * index of an rr_node.  rr_node_indices is a matrix containing the index  *
 * of the *first* rr_node at a given (i,j) location.  The chanx_rr_indices *
 * and chany_rr_indices data structures give the rr_node index for each    *
 * track in each (i,j) channel segment.                                    */ 
 
 int index, i, j;
 int **rr_node_indices;  
 
 rr_node_indices = (int **) alloc_matrix (0, nx+1, 0, ny+1, sizeof(int));
 
 chanx_rr_indices = (int ***) alloc_matrix3 (1, nx, 0, ny, 0, 
                    nodes_per_chan - 1, sizeof (int)); 
 
 chany_rr_indices = (int ***) alloc_matrix3 (0, nx, 1, ny, 0,
                    nodes_per_chan - 1, sizeof (int)); 
 
 index = 0;
 
 for (i=0;i<=nx+1;i++) { 
    for (j=0;j<=ny+1;j++) {
       rr_node_indices[i][j] = index;

       if (clb[i][j].type == CLB) { 
          index += nodes_per_clb; 
          index = load_chanx_rr_indices (seg_details_x, nodes_per_chan, index, 
                                         i, j);
          index = load_chany_rr_indices (seg_details_y, nodes_per_chan, index,
                                         i, j);
       } 

       else if (clb[i][j].type == IO) {  
          index += nodes_per_pad;

          if (j == 0)    /* Bottom row */
             index = load_chanx_rr_indices (seg_details_x, nodes_per_chan, 
                        index, i, j);

          if (i == 0)    /* Leftmost column */
             index = load_chany_rr_indices (seg_details_y, nodes_per_chan, 
                        index, i, j);
       } 

       else if (clb[i][j].type != ILLEGAL) {
          printf("Error in alloc_and_load_rr_node_indices.  Unexpected clb"
                 " type.\n");
          exit (1);
       } 
    }  
 }     
 
 return (rr_node_indices);
}


void free_rr_node_indices (int **rr_node_indices) {

/* Frees all the rr_node_indices structures allocated for fast index       *
 * computations.                                                           */

 free_matrix (rr_node_indices, 0, nx+1, 0, sizeof(int));
 free_matrix3 (chanx_rr_indices, 1, nx, 0, ny, 0, sizeof (int));
 free_matrix3 (chany_rr_indices, 0, nx, 1, ny, 0, sizeof (int));
}


static int load_chanx_rr_indices (t_seg_details *seg_details_x, int 
           nodes_per_chan, int start_index, int i, int j) {

/* Loads the chanx_rr_indices array for all track segments starting at     *
 * (i,j), assuming the first segment found has index start_index.  It also *
 * returns the next free index (i.e. the index for the next rr_node).      */

 int rr_index, istart, iend, iseg, itrack;

 rr_index = start_index;

 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    istart = get_closest_seg_start (seg_details_x, itrack, i, j);

    /* Don't do anything if this isn't the start of the segment. */
 
    if (istart != i)
       continue;
   
    iend = get_seg_end (seg_details_x, itrack, istart, j, nx);
   
    for (iseg=istart;iseg<=iend;iseg++) 
       chanx_rr_indices[iseg][j][itrack] = rr_index;

    rr_index++;
 }
 
 return (rr_index);
}


static int load_chany_rr_indices (t_seg_details *seg_details_y, int 
           nodes_per_chan, int start_index, int i, int j) {
 
/* Loads the chany_rr_indices array for all track segments starting at     *
 * (i,j), assuming the first segment found has index start_index.  It also *
 * returns the next free index (i.e. the index for the next rr_node).      */
 
 int rr_index, jstart, jend, jseg, itrack;

 rr_index = start_index;
 
 for (itrack=0;itrack<nodes_per_chan;itrack++) {
    jstart = get_closest_seg_start (seg_details_y, itrack, j, i);
 
    /* Don't do anything if this isn't the start of the segment. */
  
    if (jstart != j)
       continue; 
    
    jend = get_seg_end (seg_details_y, itrack, jstart, i, ny);
    
    for (jseg=jstart;jseg<=jend;jseg++) 
       chany_rr_indices[i][jseg][itrack] = rr_index;  
 
    rr_index++; 
 } 
  
 return (rr_index); 
} 


int get_rr_node_index (int i, int j, t_rr_type rr_type, int ioff,
        int nodes_per_chan, int **rr_node_indices) {

/* Returns the index of the specified routing resource node.  (i,j) are     *
 * the location within the FPGA, rr_type specifies the type of resource,    *
 * and ioff gives the number of this resource.  ioff is the class number,   *
 * pin number or track number, depending on what type of resource this      *
 * is.  All ioffs start at 0 and go up to pins_per_clb-1 or the equivalent. *
 * The order within a clb is:  SOURCEs + SINKs (num_class of them); IPINs,  *
 * and OPINs (pins_per_clb of them); CHANX; and CHANY (nodes_per_chan of    *
 * each).  For (i,j) locations that point at pads the order is:  io_rat     *
 * occurances of SOURCE, SINK, OPIN, IPIN (one for each pad), then one      *
 * associated channel (if there is a channel at (i,j)).  All IO pads are    *
 * bidirectional, so while each will be used only as an INPAD or as an      *
 * OUTPAD, all the switches necessary to do both must be in each pad.       *
 *                                                                          *
 * Note that for segments (CHANX and CHANY) of length > 1, the segment is   *
 * given an rr_index based on the (i,j) location at which it starts (i.e.   *
 * lowest (i,j) location at which this segment exists).                     *
 * This routine also performs error checking to make sure the node in       *
 * question exists.                                                         */

 int index, iclass;

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
       index = chanx_rr_indices[i][j][ioff];
       return (index);
 
    case CHANY:
       assert (ioff < nodes_per_chan);
       index = chany_rr_indices[i][j][ioff];
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
       index += 4 * ioff;
       return (index);
 
    case SINK:
       assert (ioff < io_rat);
       index += 4 * ioff + 1;
       return (index);
 
    case OPIN:
       assert (ioff < io_rat);
       index += 4 * ioff + 2;
       return (index);
 
    case IPIN:
       assert (ioff < io_rat);
       index += 4 * ioff + 3;
       return (index);
 
    case CHANX:
       assert (ioff < nodes_per_chan);
       assert (j == 0);                 /* Only one with a channel. */
       index = chanx_rr_indices[i][j][ioff];
       return (index);
 
    case CHANY:
       assert (ioff < nodes_per_chan);
       assert (i == 0);                 /* Only one with a channel. */
       index = chany_rr_indices[i][j][ioff];
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
           "(%d, %d).\nrr_type: %d.\n", clb[i][j].type, i, j, rr_type);
    exit (1);
 }
}


int get_seg_end (t_seg_details *seg_details, int itrack, int seg_start, int
       chan_num, int max_end) {

/* Returns the segment number (coordinate along the channel) at which this *
 * segment ends.  For a segment spanning clbs from 1 to 4, seg_end is 4.   *
 * max_end is the maximum dimension of the FPGA in this direction --       *
 * either nx or ny.                                                        */

 int seg_end, length, norm_start;

 length = seg_details[itrack].length;

 if (seg_start != 1) {
    seg_end = min (seg_start + length - 1, max_end);
 }
 else if (seg_details[itrack].longline) {
    seg_end = max_end;
 }
 else {        /* Short segment, at the left edge of array. */
    norm_start = seg_details[itrack].start;
    seg_end = length - (length + 1 + chan_num - norm_start) % length;
 }
 
 return (seg_end);
}


int get_xtrack_to_clb_ipin_edges (int tr_istart, int tr_iend, int tr_j,
       int itrack, int iside, t_linked_edge **edge_list_ptr, struct s_ivec 
       **tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices, 
       t_seg_details *seg_details_x, int wire_to_ipin_switch) {

/* This routine counts how many connections should be made from this segment *
 * to the clbs above (TOP) or below (BOTTOM) it.   It also adds them to the  *
 * edge linked list and updates edge_list_ptr.                               */

 int clb_j, iconn, num_conn, max_conn, i, ipin, to_node;
 t_linked_edge *edge_list_head;

/* Side is from the *track's* perspective */

 if (iside == BOTTOM) {
    clb_j = tr_j;
 }
 else if (iside == TOP) {
    clb_j = tr_j + 1;
 }
 else {  
    printf ("Error in get_xtrack_to_clb_ipin_edges:  Unknown iside: %d.\n",
            iside);
    exit (1);
 }

 edge_list_head = *edge_list_ptr;
 num_conn = 0;
 max_conn = tracks_to_clb_ipin[itrack][iside].nelem;

 for (i=tr_istart;i<=tr_iend;i++) {
    if (is_cbox (i, tr_j, itrack, seg_details_x)) {
       for (iconn=0;iconn<max_conn;iconn++) {
          ipin = tracks_to_clb_ipin[itrack][iside].list[iconn];
          to_node = get_rr_node_index (i, clb_j, IPIN, ipin, nodes_per_chan,
                    rr_node_indices);
          edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                    wire_to_ipin_switch, &free_edge_list_head);
       }
       num_conn += max_conn;
    }
 }

 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


int get_ytrack_to_clb_ipin_edges (int tr_jstart, int tr_jend, int tr_i,
       int itrack, int iside, t_linked_edge **edge_list_ptr, struct s_ivec
       **tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices,
       t_seg_details *seg_details_y, int wire_to_ipin_switch) {

/* This routine counts how many connections should be made from this segment *
 * to the clbs to the LEFT or RIGHT of it.  It also adds them to the edge    *
 * linked list and updates edge_list_ptr.                                    */

 int clb_i, iconn, num_conn, max_conn, j, ipin, to_node;
 t_linked_edge *edge_list_head;

/* Side is from the *track's* perspective */

 if (iside == LEFT) {  
    clb_i = tr_i;
 }
 else if (iside == RIGHT) {
    clb_i = tr_i + 1;   
 }
 else {  
    printf ("Error in get_ytrack_to_clb_ipin_edges:  Unknown iside: %d.\n",
            iside);
    exit (1);
 }
 
 edge_list_head = *edge_list_ptr;
 num_conn = 0;   
 max_conn = tracks_to_clb_ipin[itrack][iside].nelem;
 
 for (j=tr_jstart;j<=tr_jend;j++) {
    if (is_cbox (j, tr_i, itrack, seg_details_y)) {
       for (iconn=0;iconn<max_conn;iconn++) {
          ipin = tracks_to_clb_ipin[itrack][iside].list[iconn];
          to_node = get_rr_node_index (clb_i, j, IPIN, ipin, nodes_per_chan,
                    rr_node_indices);
          edge_list_head = insert_in_edge_list (edge_list_head, to_node, 
                        wire_to_ipin_switch, &free_edge_list_head);
       }
       num_conn += max_conn;
    }   
 }
 
 *edge_list_ptr = edge_list_head;
 return (num_conn); 
}


int get_xtrack_to_pad_edges (int tr_istart, int tr_iend, int tr_j, int pad_j,
        int itrack, t_linked_edge **edge_list_ptr, struct s_ivec 
        *tracks_to_pads, int nodes_per_chan, int **rr_node_indices, 
        t_seg_details *seg_details_x, int wire_to_ipin_switch) {

/* This routine counts how many connections should be made from this segment *
 * to the row of pads above or below it.  It also adds these edges to the    *
 * edge list and updates edge_list_ptr to point to the new list head.        */

 int iconn, ipad, to_node, num_conn, max_conn, i;
 t_linked_edge *edge_list_head;

 edge_list_head = *edge_list_ptr;
 num_conn = 0;
 max_conn = tracks_to_pads[itrack].nelem;

 for (i=tr_istart;i<=tr_iend;i++) {
    if (is_cbox (i, tr_j, itrack, seg_details_x)) {
       for (iconn=0;iconn<max_conn;iconn++) {
          ipad = tracks_to_pads[itrack].list[iconn];
          to_node = get_rr_node_index (i, pad_j, IPIN, ipad, nodes_per_chan, 
                        rr_node_indices);
          edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                           wire_to_ipin_switch, &free_edge_list_head);
       }
       num_conn += max_conn; 
    }
 }
 
 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


int get_ytrack_to_pad_edges (int tr_jstart, int tr_jend, int tr_i, int pad_i, 
        int itrack, t_linked_edge **edge_list_ptr, struct s_ivec 
        *tracks_to_pads, int nodes_per_chan, int **rr_node_indices, 
        t_seg_details *seg_details_y, int wire_to_ipin_switch) {

/* This routine counts how many connections should be made from this segment *
 * to the row of pads to the left or right of it.  Additionally, if rr_edges *
 * isn't NULL, it will also load the edges array.  Make sure rr_edges points *
 * at the next free spot to put an edge in this case.                        */

 int iconn, ipad, to_node, num_conn, max_conn, j;
 t_linked_edge *edge_list_head;

 edge_list_head = *edge_list_ptr;
 num_conn = 0;
 max_conn = tracks_to_pads[itrack].nelem;

 for (j=tr_jstart;j<=tr_jend;j++) {
    if (is_cbox (j, tr_i, itrack, seg_details_y)) {
       for (iconn=0;iconn<tracks_to_pads[itrack].nelem;iconn++) {
          ipad = tracks_to_pads[itrack].list[iconn];
          to_node = get_rr_node_index (pad_i, j, IPIN, ipad, nodes_per_chan, 
                        rr_node_indices);
          edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                           wire_to_ipin_switch, &free_edge_list_head);
       } 
       num_conn += max_conn;
    }  
 }
 
 *edge_list_ptr = edge_list_head; 
 return (num_conn);
}


int get_xtrack_to_ytracks (int from_istart, int from_iend, int from_j, int 
       from_track, int to_j, t_linked_edge **edge_list_ptr, int nodes_per_chan,
       int **rr_node_indices, t_seg_details *seg_details_x, t_seg_details 
       *seg_details_y, enum e_switch_block_type switch_block_type) {

/* Counts how many connections should be made from this segment to the y-   *
 * segments in the adjacent channels at to_j.  It returns the number of     *
 * connections, and updates edge_list_ptr to point at the head of the       *
 * (extended) linked list giving the nodes to which this segment connects   *
 * and the switch type used to connect to each.                             *
 *                                                                          *
 * An edge is added from this segment to a y-segment if:                    *
 * (1) this segment should have a switch box at that location, or           *
 * (2) the y-segment to which it would connect has a switch box, and the    *
 *     switch type of that y-segment is unbuffered (bidirectional pass      *
 *     transistor).                                                         *
 *                                                                          *
 * If the switch in each direction is a pass transistor (unbuffered), both  *
 * switches are marked as being of the types of the larger (lower R) pass   *
 * transistor.  Note that this code implicitly assumes the routing is       *
 * bidirectional (a switch in each direction).                              */

 int num_conn, to_node, i, to_track, iconn;
 int from_node_switch, to_node_switch;
 short switch_types[2];
 boolean is_x_sbox, is_y_sbox, yconn_to_above;
 t_linked_edge *edge_list_head;
 struct s_ivec conn_tracks;

 num_conn = 0;
 edge_list_head = *edge_list_ptr;

 if (to_j <= from_j) 
    yconn_to_above = TRUE;  /* The connection goes up from ychan to xchan. */
 else
    yconn_to_above = FALSE;

/* Recall:  this is the type of switch to use on switches that go *to* this *
 * node (i.e. the output side is on the from_node).                         */

 from_node_switch = seg_details_x[from_track].wire_switch;

/* For each unit-length piece of wire in the segment, I add the things it    *
 * would connect to diagonally to the right and diagonally to the left of    *
 * it.  This is important -- for some switch boxes, the connection from      *
 * (i,j) to (i,j+1) and the connection from (i+1,j) to (i,j+1), for example, *
 * would lead to connections to different tracks in the (i,j+1) channel.  In *
 * this case, the code below will connect to ALL the tracks that two unit    *
 * length segments at (i,j) and (i+1,j) would have connected to in (i,j+1).  */

 for (i=from_istart;i<=from_iend;i++) { 

    /* Diagonal connection to left (from xchan to ychan) */

    is_x_sbox = is_sbox (i, from_j, from_track, seg_details_x, FALSE);

    conn_tracks = get_switch_box_tracks (i, from_j, from_track, CHANX, i-1,
               to_j, CHANY, switch_block_type, nodes_per_chan);

    /* For all the tracks we connect to in that channel ... */

    for (iconn=0;iconn<conn_tracks.nelem;iconn++) {
       to_track = conn_tracks.list[iconn];
       is_y_sbox = is_sbox (to_j, i-1, to_track, seg_details_y, yconn_to_above);
       to_node_switch = seg_details_y[to_track].wire_switch;
       
       get_switch_type (is_x_sbox, is_y_sbox, from_node_switch, to_node_switch,                         switch_types);

       if (switch_types[0] != OPEN) {
          to_node = get_rr_node_index (i-1, to_j, CHANY, to_track, 
                  nodes_per_chan, rr_node_indices);

          if (!rr_edge_done[to_node]) {   /* Not a repeat edge. */
             num_conn++;
             rr_edge_done[to_node] = TRUE;
             edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                            switch_types[0], &free_edge_list_head);

             if (switch_types[1] != OPEN) {   /* A second edge. */
                edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                            switch_types[1], &free_edge_list_head);
                num_conn++;
             }
          }
       }
    }

    /* Diagonal connection to right (from xchan to ychan) */

    is_x_sbox = is_sbox (i, from_j, from_track, seg_details_x, TRUE);

    conn_tracks = get_switch_box_tracks (i, from_j, from_track, CHANX, i,
               to_j, CHANY, switch_block_type, nodes_per_chan);
   
    /* For all the tracks we connect to in that channel ... */

    for (iconn=0;iconn<conn_tracks.nelem;iconn++) {
       to_track = conn_tracks.list[iconn];
       is_y_sbox = is_sbox (to_j, i, to_track, seg_details_y, yconn_to_above);
       to_node_switch = seg_details_y[to_track].wire_switch;
       
       get_switch_type (is_x_sbox, is_y_sbox, from_node_switch, to_node_switch,
                        switch_types);

       if (switch_types[0] != OPEN) {
          to_node = get_rr_node_index (i, to_j, CHANY, to_track, 
                  nodes_per_chan, rr_node_indices);

          if (!rr_edge_done[to_node]) {   /* Not a repeat edge. */
             num_conn++;
             rr_edge_done[to_node] = TRUE;
             edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                             switch_types[0], &free_edge_list_head);

             if (switch_types[1] != OPEN) {    /* A second edge */
                edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                             switch_types[1], &free_edge_list_head);
                num_conn++;
             }
          }
       }
    }

 }  /* End for length of segment. */
 
 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


int get_ytrack_to_xtracks (int from_jstart, int from_jend, int from_i, int 
       from_track, int to_i, t_linked_edge **edge_list_ptr, int nodes_per_chan, 
       int **rr_node_indices, t_seg_details *seg_details_x, t_seg_details 
       *seg_details_y, enum e_switch_block_type switch_block_type) {
 
/* Counts how many connections should be made from this segment to the x-   *
 * segments in the adjacent channels at to_i.  It returns the number of     *
 * connections, and updates edge_list_ptr to point at the head of the       *
 * (extended) linked list giving the nodes to which this segment connects   *
 * and the switch type used to connect to each.                             *
 *                                                                          *
 * An edge is added from this segment to an x-segment if:                   *
 * (1) this segment should have a switch box at that location, or           *
 * (2) the x-segment to which it would connect has a switch box, and the    *
 *     switch type of that x-segment is unbuffered (bidirectional pass      *
 *     transistor).                                                         *
 *                                                                          *
 * If the switch in each direction is a pass transistor (unbuffered), both  *
 * switches are marked as being of the types of the larger (lower R) pass   *
 * transistor.  Note that this code implicitly assumes the routing is       *
 * bidirectional (a switch in each direction).                              */

 
 int num_conn, to_node, j, to_track, iconn;
 int from_node_switch, to_node_switch;
 short switch_types[2];
 boolean is_x_sbox, is_y_sbox, xconn_to_right;
 t_linked_edge *edge_list_head;
 struct s_ivec conn_tracks;

 num_conn = 0;
 edge_list_head = *edge_list_ptr;

 if (to_i <= from_i) 
    xconn_to_right = TRUE;  /* The connection goes RIGHT from xchan to ychan */
 else
    xconn_to_right = FALSE;

/* Recall:  this is the type of switch to use on switches that go *to* this *
 * node (i.e. the output side is on the from_node).                         */

 from_node_switch = seg_details_y[from_track].wire_switch;

/* For each unit-length piece of wire in the segment, I add the things it    *
 * would connect to diagonally above and diagonally below it.  This is       *
 * important -- for some switch boxes, the connection from (i,j) to (i+1,j)  *
 * and the connection from (i,j+1) to (i+1,j), for example, would lead to    *
 * connections to different tracks in the (i+1,j) channel.  In this case,    *
 * the code below will connect to ALL the tracks that two unit length        *
 * segments at (i,j) and (i,j+1) would have connected to in (i+1,j).         */
 
 for (j=from_jstart;j<=from_jend;j++) {
   
    /* Diagonal connection to below (from ychan to xchan) */

    is_y_sbox = is_sbox (j, from_i, from_track, seg_details_y, FALSE);

    conn_tracks = get_switch_box_tracks (from_i, j, from_track, CHANY, to_i,
               j-1, CHANX, switch_block_type, nodes_per_chan);

    /* For all the tracks we connect to in that channel ... */

    for (iconn=0;iconn<conn_tracks.nelem;iconn++) {
       to_track = conn_tracks.list[iconn];
       is_x_sbox = is_sbox (to_i, j-1, to_track, seg_details_x, xconn_to_right);
       to_node_switch = seg_details_x[to_track].wire_switch;
     
       get_switch_type (is_y_sbox, is_x_sbox, from_node_switch, to_node_switch,
                        switch_types);

       if (switch_types[0] != OPEN) { 
          to_node = get_rr_node_index (to_i, j-1, CHANX, to_track, 
                    nodes_per_chan, rr_node_indices);

          if (!rr_edge_done[to_node]) {    /* Not a repeat edge. */
             num_conn++; 
             rr_edge_done[to_node] = TRUE;
             edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                                switch_types[0], &free_edge_list_head);

             if (switch_types[1] != OPEN) {   /* A second edge. */
                edge_list_head = insert_in_edge_list (edge_list_head, to_node,
                                   switch_types[1], &free_edge_list_head);
                num_conn++;
             }
          }
       }   
    }

    /* Diagonal connection to above (from ychan to xchan) */

    is_y_sbox = is_sbox (j, from_i, from_track, seg_details_y, TRUE);

    conn_tracks = get_switch_box_tracks (from_i, j, from_track, CHANY, to_i,
               j, CHANX, switch_block_type, nodes_per_chan);

    /* For all the tracks we connect to in that channel ... */ 

    for (iconn=0;iconn<conn_tracks.nelem;iconn++) { 
       to_track = conn_tracks.list[iconn];
       is_x_sbox = is_sbox (to_i, j, to_track, seg_details_x, xconn_to_right);
       to_node_switch = seg_details_x[to_track].wire_switch;

       get_switch_type (is_y_sbox, is_x_sbox, from_node_switch, to_node_switch,
                        switch_types);

       if (switch_types[0] != OPEN) { 
          to_node = get_rr_node_index (to_i, j, CHANX, to_track, 
                    nodes_per_chan, rr_node_indices);

          if (!rr_edge_done[to_node]) {    /* Not a repeat edge. */
             num_conn++; 
             rr_edge_done[to_node] = TRUE;
             edge_list_head = insert_in_edge_list (edge_list_head, to_node, 
                            switch_types[0], &free_edge_list_head);

             if (switch_types[1] != OPEN) {  /* A second edge */
                edge_list_head = insert_in_edge_list (edge_list_head, to_node, 
                               switch_types[1], &free_edge_list_head);
                num_conn++;
             }
          }
       }   
    }

 }  /* End for length of segment. */
    
 
 *edge_list_ptr = edge_list_head;
 return (num_conn);
}


int get_xtrack_to_xtrack (int from_i, int j, int from_track, int to_i, 
       t_linked_edge **edge_list_ptr, int nodes_per_chan, int 
       **rr_node_indices, t_seg_details *seg_details_x, enum 
       e_switch_block_type switch_block_type) {

/* Returns the number of edges between the specified channel segments.      *
 * Also updates edge_list_ptr to point at the new (extended) linked list    *
 * of edges and switch types.                                               */
 
 boolean is_from_sbox, is_to_sbox, from_goes_right, to_goes_right;
 int to_track, to_node, iconn, num_conn;
 int from_node_switch, to_node_switch;
 short switch_types[2];
 struct s_ivec conn_tracks;

 if (from_i < to_i) {
    from_goes_right = TRUE;
    to_goes_right = FALSE;
 }
 else {
    from_goes_right = FALSE;
    to_goes_right = TRUE;
 }

 num_conn = 0;
 from_node_switch = seg_details_x[from_track].wire_switch;

 is_from_sbox = is_sbox (from_i, j, from_track, seg_details_x, from_goes_right);
 conn_tracks = get_switch_box_tracks (from_i, j, from_track, CHANX, to_i,
            j, CHANX, switch_block_type, nodes_per_chan);

 /* For all the tracks we connect to in that channel ... */
 
 for (iconn=0;iconn<conn_tracks.nelem;iconn++) {
    to_track = conn_tracks.list[iconn];
    is_to_sbox = is_sbox (to_i, j, to_track, seg_details_x, to_goes_right);
    to_node_switch = seg_details_x[to_track].wire_switch;
  
    get_switch_type (is_from_sbox, is_to_sbox, from_node_switch,
                     to_node_switch, switch_types);
 
    if (switch_types[0] != OPEN) {
       to_node = get_rr_node_index (to_i, j, CHANX, to_track, nodes_per_chan,
               rr_node_indices);

       /* No need to check for repeats with the current switch boxes. */

       *edge_list_ptr = insert_in_edge_list (*edge_list_ptr, to_node, 
                        switch_types[0], &free_edge_list_head);
       num_conn++;

       if (switch_types[1] != OPEN) {
          *edge_list_ptr = insert_in_edge_list (*edge_list_ptr, to_node, 
                           switch_types[1], &free_edge_list_head);
          num_conn++;
       }
    }
 }       
 
 return (num_conn);
}


int get_ytrack_to_ytrack (int i, int from_j, int from_track, int to_j,
       t_linked_edge **edge_list_ptr, int nodes_per_chan, int 
       **rr_node_indices, t_seg_details *seg_details_y, enum 
       e_switch_block_type switch_block_type) {
 
/* Returns the number of edges between the specified channel segments.      *
 * Also updates edge_list_ptr to point at the new (extended) linked list    *
 * of edges and switch types.                                               */

 
 boolean is_from_sbox, is_to_sbox, from_goes_up, to_goes_up; 
 int to_track, to_node, iconn, num_conn;
 int from_node_switch, to_node_switch;
 short switch_types[2];
 struct s_ivec conn_tracks;

if (from_j < to_j) { 
    from_goes_up = TRUE; 
    to_goes_up = FALSE; 
 } 
 else {
    from_goes_up = FALSE; 
    to_goes_up = TRUE; 
 } 

 num_conn = 0;
 from_node_switch = seg_details_y[from_track].wire_switch;
 
 is_from_sbox = is_sbox (from_j, i, from_track, seg_details_y, from_goes_up); 
 conn_tracks = get_switch_box_tracks (i, from_j, from_track, CHANY, i, to_j, 
             CHANY, switch_block_type, nodes_per_chan);
 
 /* For all the tracks we connect to in that channel ... */
 
 for (iconn=0;iconn<conn_tracks.nelem;iconn++) {
    to_track = conn_tracks.list[iconn];
    is_to_sbox = is_sbox (to_j, i, to_track, seg_details_y, to_goes_up); 
    to_node_switch = seg_details_y[to_track].wire_switch;
 
    get_switch_type (is_from_sbox, is_to_sbox, from_node_switch,
                     to_node_switch, switch_types);

    if (switch_types[0] != OPEN) { 
       to_node = get_rr_node_index (i, to_j, CHANY, to_track, nodes_per_chan,
                rr_node_indices); 

       /* No need to check for repeats with the current switch boxes. */

       *edge_list_ptr = insert_in_edge_list (*edge_list_ptr, to_node,
                        switch_types[0], &free_edge_list_head);
       num_conn++;

       if (switch_types[1] != OPEN) {
          *edge_list_ptr = insert_in_edge_list (*edge_list_ptr, to_node,
                           switch_types[1], &free_edge_list_head);
          num_conn++;
       }
    }
 }
 
 return (num_conn);
}


boolean is_sbox (int seg_num, int chan_num, int itrack, t_seg_details 
          *seg_details, boolean above_or_right) {

/* Returns TRUE if the specified segment has a switch box at the specified  *
 * location, FALSE otherwise.  The switch box is from the specified segment *
 * to the left (for chanx) or below (for chany) unless above_or_right is    *
 * TRUE.                                                                    */

 int seg_offset, start, length;
 boolean longline;
 
 length = seg_details[itrack].length;
 start = seg_details[itrack].start;
 longline = seg_details[itrack].longline;

/* NB: Periodicity is length for normal segments, length + 1 for long lines. */
 
 if (!longline) {
    seg_offset = (seg_num + chan_num - start + length) % length;
    seg_offset += above_or_right;    /* Add one if conn. is above or to right */
 }

 else {    /* Is a longline */
    seg_offset = (seg_num + chan_num - start + above_or_right) % (length + 1);
 }

 return (seg_details[itrack].sb[seg_offset]);
}


static void get_switch_type (boolean is_from_sbox, boolean is_to_sbox, 
           short from_node_switch, short to_node_switch, short switch_types[2])
           {

/* This routine looks at whether the from_node and to_node want a switch,  *
 * and what type of switch is used to connect *to* each type of node       *
 * (from_node_switch and to_node_switch).  It decides what type of switch, *
 * if any, should be used to go from from_node to to_node.  If no switch   *
 * should be inserted (i.e. no connection), it returns OPEN.  Its returned *
 * values are in the switch_types array.  It needs to return an array      *
 * because one topology (a buffer in the forward direction and a pass      *
 * transistor in the backward direction) results in *two* switches.        */

 switch_types[0] = OPEN;  /* No switch */
 switch_types[1] = OPEN;

 if (!is_from_sbox && !is_to_sbox) {  /* No connection wanted in either dir */
    switch_types[0] = OPEN;
 }

 else if (is_from_sbox && !is_to_sbox) {  /* Only forward connection wanted */
    switch_types[0] = to_node_switch;  /* Type of switch to go *to* to_node */
 }
 
 else if (!is_from_sbox && is_to_sbox) {

/* Only backward connection desired.  We're deciding whether or not to put *
 * in the forward connection.  Put it in if the backward connection uses   *
 * a bidirectional (pass transistor) switch.  Remember that the backward   *
 * connection uses a from_node_switch type of switch.                      */

    if (switch_inf[from_node_switch].buffered == FALSE) {
       switch_types[0] = from_node_switch;
    }
 }

 else {

/* Both a forward and a backward connection desired.  If the switch types   *
 * desired for the two connection are different, we have to reconcile them. */

    if (from_node_switch == to_node_switch) {
       switch_types[0] = to_node_switch;
    }
    else {    /* Different switch types.  Reconcile. */
       if (switch_inf[to_node_switch].buffered) {
          switch_types[0] = to_node_switch;

          if (switch_inf[from_node_switch].buffered == FALSE) {

         /* Buffer in forward direction, pass transistor in backward.  Put *
          * in *two* edges.                                                */
              
             switch_types[1] = from_node_switch;
          }
       }
       else {   /* Forward connection is a pass transistor. */
          if (switch_inf[from_node_switch].buffered) {
             switch_types[0] = to_node_switch;
          }
          else {  

          /* Both forward and backward connections use pass transistors. *
           * use whichever one is larger, since you'll only physically   *
           * build one switch.                                           */
     
             if (switch_inf[to_node_switch].R < 
                            switch_inf[from_node_switch].R) {
                switch_types[0] = to_node_switch;
             }
             else if (switch_inf[from_node_switch].R < 
                            switch_inf[to_node_switch].R) {
                switch_types[0] = from_node_switch;
             }
             else {

             /* Two pass transistors have the same R, but are have different *
              * switch indices.  Use the one with lower index (arbitrarily), *
              * to ensure both switches are of the same type (since you can  *
              * only physically build one).  I'm being pretty dogmatic here. */

                if (to_node_switch < from_node_switch) {
                   switch_types[0] = to_node_switch;
                }
                else {
                   switch_types[0] = from_node_switch;
                }
             }
          }
       }
    }     /* End switch types are different */
 }   /* End both forward and backward connection desired. */
}
