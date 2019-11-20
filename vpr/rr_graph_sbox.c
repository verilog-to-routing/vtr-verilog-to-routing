#include "util.h"
#include "vpr_types.h"
#include "rr_graph_sbox.h"


/* Switch box:                                                             *
 *                    TOP (CHANY)                                          *
 *                    | | | | | |                                          *
 *                   +-----------+                                         *
 *                 --|           |--                                       *
 *                 --|           |--                                       *
 *           LEFT  --|           |-- RIGHT                                 *
 *          (CHANX)--|           |--(CHANX)                                *
 *                 --|           |--                                       *
 *                 --|           |--                                       *
 *                   +-----------+                                         *
 *                    | | | | | |                                          *
 *                   BOTTOM (CHANY)                                        */

/* [0..3][0..3][0..nodes_per_chan-1].  Structure below is indexed as:       *
 * [from_side][to_side][from_track].  That yields an integer vector (ivec)  *
 * of the tracks to which from_track connects in the proper to_location.    *
 * For simple switch boxes this is overkill, but it will allow complicated  *
 * switch boxes with Fs > 3, etc. without trouble.                          */

static struct s_ivec ***switch_block_conn;  



static int get_simple_switch_block_track (enum e_side from_side, enum e_side
             to_side, int from_track, enum e_switch_block_type
             switch_block_type, int nodes_per_chan); 

static enum e_side get_sbox_side (int get_i, int get_j, t_rr_type get_type,
            int comp_i, int comp_j); 





void alloc_and_load_switch_block_conn (int nodes_per_chan,
        enum e_switch_block_type switch_block_type) {

/* Allocates and loads the switch_block_conn data structure.  This structure *
 * lists which tracks connect to which at each switch block.                 */

 enum e_side from_side, to_side;
 int from_track, to_track;

 switch_block_conn = (struct s_ivec ***) alloc_matrix3 (0, 3, 0, 3, 0, 
                      nodes_per_chan - 1, sizeof (struct s_ivec));

 for (from_side=0;from_side<=3;from_side++) {
    for (to_side=0;to_side<=3;to_side++) {
       for (from_track=0;from_track<nodes_per_chan;from_track++) {

          if (from_side != to_side) {
             switch_block_conn[from_side][to_side][from_track].nelem = 1;

             switch_block_conn[from_side][to_side][from_track].list = 
                  (int *) my_malloc (sizeof (int));

             to_track = get_simple_switch_block_track (from_side, to_side, 
                        from_track, switch_block_type, nodes_per_chan);

             switch_block_conn[from_side][to_side][from_track].list[0] = 
                             to_track;
          }

          else {   /* from_side == to_side -> no connection. */

             switch_block_conn[from_side][to_side][from_track].nelem = 0;
             switch_block_conn[from_side][to_side][from_track].list = NULL;
          }

       }
    }
 }
}


void free_switch_block_conn (int nodes_per_chan) {

/* Frees the switch_block_conn data structure.                              */

 free_ivec_matrix3 (switch_block_conn, 0, 3, 0, 3, 0, nodes_per_chan - 1);
}


#define SBOX_ERROR -1

static int get_simple_switch_block_track (enum e_side from_side, enum e_side 
             to_side, int from_track, enum e_switch_block_type 
             switch_block_type, int nodes_per_chan) {

/* This routine returns the track number to which the from_track should     *
 * connect.  It supports three simple, Fs = 3, switch blocks.               */

 int to_track;
 
 to_track = SBOX_ERROR;   /* Can check to see if it's not set later. */

 if (switch_block_type == SUBSET) {   /* NB:  Global routing uses SUBSET too */
    to_track = from_track;
 }
 

/* See S. Wilton Phd thesis, U of T, 1996 p. 103 for details on following. */

 else if (switch_block_type == WILTON) {

    if (from_side == LEFT) {

       if (to_side == RIGHT) {        /* CHANX to CHANX */
          to_track = from_track;
       } 
       else if (to_side == TOP) {     /* from CHANX to CHANY */
          to_track = (nodes_per_chan - from_track) % nodes_per_chan;
       }
       else if (to_side == BOTTOM) {
          to_track = (nodes_per_chan + from_track -1) % nodes_per_chan;
       }
    }

    else if (from_side == RIGHT) {
       if (to_side == LEFT) {         /* CHANX to CHANX */
          to_track = from_track;
       }
       else if (to_side == TOP) {     /* from CHANX to CHANY */
          to_track = (nodes_per_chan + from_track - 1) % nodes_per_chan;
       }  
       else if (to_side == BOTTOM) { 
          to_track = (2 * nodes_per_chan - 2 - from_track) % nodes_per_chan;
       }  
    }

    else if (from_side == BOTTOM) {
       if (to_side == TOP) {         /* CHANY to CHANY */   
          to_track = from_track;
       }   
       else if (to_side == LEFT) {     /* from CHANY to CHANX */
          to_track = (from_track + 1) % nodes_per_chan;
       } 
       else if (to_side == RIGHT) { 
          to_track = (2 * nodes_per_chan - 2 - from_track) % nodes_per_chan;
       } 
    }

    else if (from_side == TOP) { 
       if (to_side == BOTTOM) {         /* CHANY to CHANY */
          to_track = from_track;
       }   
       else if (to_side == LEFT) {     /* from CHANY to CHANX */
          to_track = (nodes_per_chan - from_track) % nodes_per_chan;
       }
       else if (to_side == RIGHT) {
          to_track = (from_track + 1) % nodes_per_chan;
       }
    }
        
 }    /* End switch_block_type == WILTON case. */
 
 else if (switch_block_type == UNIVERSAL) {

    if (from_side == LEFT) {
 
       if (to_side == RIGHT) {        /* CHANX to CHANX */
          to_track = from_track;
       } 
       else if (to_side == TOP) {     /* from CHANX to CHANY */
          to_track = nodes_per_chan - 1 - from_track;
       } 
       else if (to_side == BOTTOM) {
          to_track = from_track;
       } 
    }

    else if (from_side == RIGHT) {
       if (to_side == LEFT) {         /* CHANX to CHANX */
          to_track = from_track;
       } 
       else if (to_side == TOP) {     /* from CHANX to CHANY */
          to_track = from_track;
       } 
       else if (to_side == BOTTOM) {
          to_track = nodes_per_chan - 1 - from_track;
       } 
    }
 
    else if (from_side == BOTTOM) {
       if (to_side == TOP) {         /* CHANY to CHANY */
          to_track = from_track;
       }  
       else if (to_side == LEFT) {     /* from CHANY to CHANX */
          to_track = from_track;
       } 
       else if (to_side == RIGHT) {
          to_track = nodes_per_chan - 1 - from_track;
       } 
    }
 
    else if (from_side == TOP) { 
       if (to_side == BOTTOM) {         /* CHANY to CHANY */
          to_track = from_track;
       }   
       else if (to_side == LEFT) {     /* from CHANY to CHANX */
          to_track = nodes_per_chan - 1 - from_track;
       } 
       else if (to_side == RIGHT) {
          to_track = from_track;
       }
    }     
 }    /* End switch_block_type == UNIVERSAL case. */
 

 if (to_track == SBOX_ERROR) {
    printf("Error in get_simple_switch_block_track.  Unexpected connection.\n"
           "from_side: %d  to_side: %d  switch_block_type: %d.\n", from_side,
           to_side, switch_block_type);
    exit (1);
 }
 
 return (to_track);
}


struct s_ivec get_switch_box_tracks (int from_i, int from_j, int from_track,
             t_rr_type from_type, int to_i, int to_j, t_rr_type to_type, enum
             e_switch_block_type switch_block_type, int nodes_per_chan) {

/* Returns a vector of the tracks to which from_track at (from_i, from_j)   *
 * should connect at (to_i, to_j).                                          */

 enum e_side from_side, to_side;

 from_side = get_sbox_side (from_i, from_j, from_type, to_i, to_j);
 to_side = get_sbox_side (to_i, to_j, to_type, from_i, from_j);
 
 return (switch_block_conn[from_side][to_side][from_track]);
}


static enum e_side get_sbox_side (int get_i, int get_j, t_rr_type get_type, 
            int comp_i, int comp_j) {

/* Returns the side of the switch box that the get_node is on, as compared *
 * to the comp (comparison) node.                                          */

 enum e_side side;

 if (get_type == CHANX) { 
    if (get_i > comp_i) {
       side = RIGHT; 
    } 
    else { 
       side = LEFT; 
    } 
 }
 
 else if (get_type == CHANY) {
    if (get_j > comp_j) {
       side = TOP;
    }
    else {
       side = BOTTOM;
    }
 }

 else {
    printf ("Error in get_sbox_side.  Unexpected get_type: %d.\n", get_type);
    exit (1);
 }

 return (side);
}
