#include <stdio.h> 
#include <string.h>
#include <math.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "graphics.h"
#include "path_delay.h"
#include "draw.h"


/*************** Types local to this module *********************************/

enum e_draw_rr_toggle {DRAW_NO_RR = 0, DRAW_ALL_RR, DRAW_ALL_BUT_BUFFERS_RR, 
          DRAW_NODES_AND_SBOX_RR, DRAW_NODES_RR};

enum e_draw_net_type {ALL_NETS, HIGHLIGHTED};

enum e_edge_dir {FROM_X_TO_Y, FROM_Y_TO_X}; /* Chanx to chany or vice versa? */


/****************** Variables local to this module. *************************/

static boolean show_nets = FALSE;  /* Show nets of placement or routing? */

/* Controls drawing of routing resources on screen, if pic_on_screen is   *
 * ROUTING.                                                               */

static enum e_draw_rr_toggle draw_rr_toggle = DRAW_NO_RR;

static enum e_route_type draw_route_type;

/* Controls if congestion is shown, when ROUTING is on screen. */

static boolean show_congestion = FALSE;  

static boolean show_graphics;  /* Graphics enabled or not? */

static char default_message[BUFSIZE];  /* Default screen message on screen */

static int gr_automode;  /* Need user input after: 0: each t,   *
                          * 1: each place, 2: never             */

static enum pic_type pic_on_screen = NO_PICTURE;  /* What do I draw? */

static float *x_clb_left, *y_clb_bottom;

/* The left and bottom coordinates of each clb in the FPGA.               *
 * x_clb_left[0..nx+1] and y_clb_bottom[0..ny+1].                         *
 * COORDINATE SYSTEM goes from (0,0) at the lower left corner to          *
 * (x_clb_left[nx+1]+clb_width, y_clb_bottom[ny+1]+clb_width) in the      *
 * upper right corner.                                                    */


static float clb_width, pin_size;
/* Drawn width (and height) of a clb, and the half-width or half-height of *
 * a clb pin, respectiviely.  Set when init_draw_coords is called.         */

static enum color_types *net_color, *block_color;
/* Color in which each block and net should be drawn.      *
 * [0..num_nets-1] and [0..num_blocks-1], respectively.    */


/********************** Subroutines local to this module ********************/

static void toggle_nets (void (*drawscreen) (void));
static void toggle_rr (void (*drawscreen) (void));
static void toggle_congestion (void (*drawscreen) (void));
static void highlight_crit_path (void (*drawscreen_ptr) (void));

static void drawscreen (void);
static void redraw_screen (void);
static void drawplace (void);
static void drawnets (void);
static void drawroute (enum e_draw_net_type draw_net_type);
static void draw_congestion (void); 

static void highlight_blocks (float x, float y);
static void get_block_center (int bnum, float *x, float *y);
static void deselect_all (void);

static void draw_rr (void); 
static void draw_rr_edges (int from_node); 
static void draw_rr_pin (int inode, enum color_types color); 
static void draw_rr_chanx (int inode, int itrack);
static void draw_rr_chany (int inode, int itrack);
static void get_rr_pin_draw_coords (int inode, int iside, float *xcen, 
            float *ycen); 
static void draw_pin_to_chan_edge (int pin_node, int chan_node, int itrack,
            boolean mark_conn); 
static void draw_x (float x, float y, float size); 
static void draw_chany_to_chany_edge (int from_node, int from_track,
            int to_node, int to_track, short switch_type);
static void draw_chanx_to_chanx_edge (int from_node, int from_track,
            int to_node, int to_track, short switch_type); 
static void draw_chanx_to_chany_edge (int chanx_node,  int chanx_track, 
             int chany_node, int chany_track, enum e_edge_dir edge_dir,
             short switch_type); 
static int get_track_num (int inode, int **chanx_track, int **chany_track); 
static void draw_rr_switch (float from_x, float from_y, float to_x, float to_y,
        boolean buffered); 



/********************** Subroutine definitions ******************************/


void set_graphics_state (boolean show_graphics_val, int gr_automode_val,
       enum e_route_type route_type) {

/* Sets the static show_graphics and gr_automode variables to the    *
 * desired values.  They control if graphics are enabled and, if so, *
 * how often the user is prompted for input.                         */

 show_graphics = show_graphics_val;
 gr_automode = gr_automode_val;
 draw_route_type = route_type;
}


void update_screen (int priority, char *msg, enum pic_type 
            pic_on_screen_val, boolean crit_path_button_enabled) {

/* Updates the screen if the user has requested graphics.  The priority  *
 * value controls whether or not the Proceed button must be clicked to   *
 * continue.  Saves the pic_on_screen_val to allow pan and zoom redraws. */

 if (!show_graphics)         /* Graphics turned off */
    return;  


/* If it's the type of picture displayed has changed, set up the proper  *
 * buttons.                                                              */

 if (pic_on_screen != pic_on_screen_val) {
    if (pic_on_screen_val == PLACEMENT && pic_on_screen == NO_PICTURE) {
       create_button ("Window", "Toggle Nets", toggle_nets);
    }

    else if (pic_on_screen_val == ROUTING && pic_on_screen == PLACEMENT) {
       create_button ("Toggle Nets", "Toggle RR", toggle_rr);
       create_button ("Toggle RR", "Congestion", toggle_congestion);

       if (crit_path_button_enabled) 
          create_button ("Congestion", "Crit. Path", highlight_crit_path);
    }

    else if (pic_on_screen_val == PLACEMENT && pic_on_screen == ROUTING) {
       destroy_button ("Toggle RR");
       destroy_button ("Congestion");

       if (crit_path_button_enabled) 
          destroy_button ("Crit. Path");
    }
 }

/* Save the main message. */

 strncpy (default_message, msg, BUFSIZE);

 pic_on_screen = pic_on_screen_val;
 update_message (msg); 
 drawscreen();
 if (priority >= gr_automode) {

    event_loop(highlight_blocks, drawscreen);
 }

 else {
    flushinput();
 }
}

 
static void drawscreen (void) {

/* This is the screen redrawing routine that event_loop assumes exists.  *
 * It erases whatever is on screen, then calls redraw_screen to redraw   *
 * it.                                                                   */

 clearscreen();
 redraw_screen ();
}


static void redraw_screen (void) {

/* The screen redrawing routine called by drawscreen and           *
 * highlight_blocks.  Call this routine instead of drawscreen if   *
 * you know you don't need to erase the current graphics, and want *
 * to avoid a screen "flash".                                      */

 if (pic_on_screen == PLACEMENT) {
    drawplace();
    if (show_nets) 
       drawnets ();
 }
 else {             /* ROUTING on screen */
    drawplace();

    if (show_nets) 
       drawroute (ALL_NETS);
    else
       draw_rr ();

    if (show_congestion) 
       draw_congestion ();
 }
}


static void toggle_nets (void (*drawscreen_ptr) (void)) {

/* Enables/disables drawing of nets when a the user clicks on a button.    *
 * Also disables drawing of routing resources.  See graphics.c for details *
 * of how buttons work.                                                    */

 show_nets = !show_nets;
 draw_rr_toggle = DRAW_NO_RR; 
 show_congestion = FALSE;

 update_message (default_message);
 drawscreen_ptr ();
}


static void toggle_rr (void (*drawscreen_ptr) (void)) {

/* Cycles through the options for viewing the routing resources available   *
 * in an FPGA.  If a routing isn't on screen, the routing graph hasn't been *
 * built, and this routine doesn't switch the view. Otherwise, this routine *
 * switches to the routing resource view.  Clicking on the toggle cycles    *
 * through the options:  DRAW_NO_RR, DRAW_ALL_RR, DRAW_ALL_BUT_BUFFERS_RR,  *
 * DRAW_NODES_AND_SBOX_RR, and DRAW_NODES_RR.                               */

 draw_rr_toggle = (draw_rr_toggle + 1) % (DRAW_NODES_RR + 1);
 show_nets = FALSE;
 show_congestion = FALSE;

 update_message (default_message);
 drawscreen_ptr ();
}


static void toggle_congestion (void (*drawscreen_ptr) (void)) {

/* Turns the congestion display on and off.   */

 char msg[BUFSIZE];
 int inode, num_congested;

 show_nets = FALSE;
 draw_rr_toggle = DRAW_NO_RR;
 show_congestion = !show_congestion;

 if (!show_congestion) {
    update_message (default_message);
 }
 else {
    num_congested = 0;
    for (inode=0;inode<num_rr_nodes;inode++) {
       if (rr_node[inode].occ > rr_node[inode].capacity) 
          num_congested++;
    }

    sprintf (msg, "%d routing resources are overused.", num_congested);
    update_message (msg);
 }
 
 drawscreen_ptr ();
}


static void highlight_crit_path (void (*drawscreen_ptr) (void)) {

/* Highlights all the blocks and nets on the critical path.                 */

 t_linked_int *critical_path_head, *critical_path_node;
 int inode, iblk, inet, num_nets_seen;
 static int nets_to_highlight = 1;
 char msg[BUFSIZE];

 if (nets_to_highlight == 0) {   /* Clear the display of all highlighting. */
    nets_to_highlight = 1;
    deselect_all ();
    update_message (default_message);
    drawscreen_ptr ();
    return;
 }
    
 critical_path_head = allocate_and_load_critical_path ();
 critical_path_node = critical_path_head;
 num_nets_seen = 0;

 while (critical_path_node != NULL)  {

    inode = critical_path_node->data;
    get_tnode_block_and_output_net (inode, &iblk, &inet);

    if (num_nets_seen == nets_to_highlight)            /* Last block */
       block_color[iblk] = MAGENTA;
    else if (num_nets_seen == nets_to_highlight - 1)   /* 2nd last block */
       block_color[iblk] = YELLOW;
    else if (num_nets_seen < nets_to_highlight)    /* Earlier block */
       block_color[iblk] = DARKGREEN;

    if (inet != OPEN) {
       num_nets_seen++;

       if (num_nets_seen < nets_to_highlight)   /* First nets. */
          net_color[inet] = DARKGREEN;
       else if (num_nets_seen == nets_to_highlight) 
          net_color[inet] = CYAN;               /* Last (new) net. */
    }

    critical_path_node = critical_path_node->next;
 }

 if (nets_to_highlight == num_nets_seen) {
    nets_to_highlight = 0;
    sprintf (msg, "All %d nets on the critical path highlighted.", 
            num_nets_seen);
 }
 else {
    sprintf (msg, "First %d nets on the critical path highlighted.", 
            nets_to_highlight);
    nets_to_highlight++;
 }

 free_int_list (&critical_path_head);

 update_message (msg);
 drawscreen_ptr ();
}


void alloc_draw_structs (void) {

/* Allocate the structures needed to draw the placement and routing.  Set *
 * up the default colors for blocks and nets.                             */

 x_clb_left = (float *) my_malloc ((nx+2)*sizeof(float));
 y_clb_bottom = (float *) my_malloc ((ny+2)*sizeof(float));
 net_color = (enum color_types *) my_malloc (num_nets * 
       sizeof (enum color_types));
 block_color = (enum color_types *) my_malloc (num_blocks * 
       sizeof (enum color_types));

 deselect_all();    /* Set initial colors */
}


void init_draw_coords (float clb_width_val) {

/* Load the arrays containing the left and bottom coordinates of the clbs   *
 * forming the FPGA.  clb_width_val sets the width and height of a drawn    *
 * clb.                                                                     */

 int i;

 if (!show_graphics) return;   /* -nodisp was selected. */

 clb_width = clb_width_val;
 pin_size = clb_width / (4. * pins_per_clb);
 pin_size = min (pin_size, clb_width / (4. * io_rat));
 pin_size = min (pin_size, 0.3);
 
 x_clb_left[0] = 0.;
 for (i=1;i<=nx+1;i++)
    x_clb_left[i] = x_clb_left[i-1] + clb_width + chan_width_y[i-1] + 1.;

 y_clb_bottom[0] = 0.;
 for (i=1;i<=ny+1;i++)
    y_clb_bottom[i] = y_clb_bottom[i-1] + clb_width + 
        chan_width_x[i-1] + 1.;

 init_world (0., y_clb_bottom[ny+1] + clb_width,
    x_clb_left[nx+1]+ clb_width, 0.);
}


static void drawplace (void) {

/* Draws the blocks placed on the proper clbs.  Occupied clbs are light *
 * grey, while empty ones are left white and have a dashed border.      */

 float io_step = clb_width/io_rat;
 float x1, y1, x2, y2;
 int i, j, k, bnum;

 /* Draw the IO Pads first. Want each subblock to border on core. */

 setlinewidth (0);

 for (i=1;i<=nx;i++) { 
    for (j=0;j<=ny+1;j+=ny+1) {  /* top and bottom */
       y1 = y_clb_bottom[j];
       y2 = y1 + clb_width;

       setlinestyle (SOLID);
       for (k=0;k<clb[i][j].occ;k++) {
          bnum = clb[i][j].u.io_blocks[k];
          setcolor (block_color[bnum]);
          x1 = x_clb_left[i] + k * io_step;
          x2 = x1 + io_step;
          fillrect (x1,y1,x2,y2);

          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
/* Vertically offset text so these closely spaced names don't overlap. */
          drawtext ((x1 + x2)/2., y1 + io_step * (k + 0.5), 
             block[clb[i][j].u.io_blocks[k]].name, clb_width);
       }

       setlinestyle (DASHED);
       setcolor (BLACK);
       for (k=clb[i][j].occ;k<io_rat;k++) {
          x1 = x_clb_left[i] + k * io_step;
          x2 = x1 + io_step;
          drawrect (x1,y1,x2,y2);
       }
    }
 }
        
 for (j=1;j<=ny;j++) {
    for (i=0;i<=nx+1;i+=nx+1) {  /* IOs on left and right */
       x1 = x_clb_left[i];
       x2 = x1 + clb_width;

       setlinestyle (SOLID);
       for (k=0;k<clb[i][j].occ;k++) {
          bnum = clb[i][j].u.io_blocks[k];
          setcolor (block_color[bnum]);
          y1 = y_clb_bottom[j] + k * io_step;
          y2 = y1 + io_step;
          fillrect (x1,y1,x2,y2);
 
          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
          drawtext ((x1 + x2)/2., (y1 + y2)/2.,
             block[clb[i][j].u.io_blocks[k]].name, clb_width);
       }
          
       setlinestyle (DASHED);
       setcolor (BLACK);
       for (k=clb[i][j].occ;k<io_rat;k++) {
          y1 = y_clb_bottom[j] + k * io_step;
          y2 = y1 + io_step;
          drawrect (x1,y1,x2,y2);
       }
    }     
 }     

/* Now do the CLBs in the middle. */

 for (i=1;i<=nx;i++) {
    x1 = x_clb_left[i];
    x2 = x1 + clb_width;
    for (j=1;j<=ny;j++) {
       y1 = y_clb_bottom[j];
       y2 = y1 + clb_width;
       if (clb[i][j].occ != 0) {
          setlinestyle (SOLID);
          bnum = clb[i][j].u.block;
          setcolor (block_color[bnum]);
          fillrect (x1,y1,x2,y2);

          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
          drawtext ((x1 + x2)/2., (y1 + y2)/2., block[clb[i][j].u.block].name, 
             clb_width);
       }
       else {
          setlinestyle (DASHED);
          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
       }
    }    /* end j */
 }    /* end i */
}


static void drawnets (void) {

/* This routine draws the nets on the placement.  The nets have not *
 * yet been routed, so we just draw a chain showing a possible path *
 * for each net.  This gives some idea of future congestion.        */

 int inet, ipin, b1, b2;
 float x1, y1, x2, y2;
 
 setlinestyle (SOLID);
 setlinewidth (0);
 
/* Draw the net as a star from the source to each sink. Draw from centers of *
 * blocks (or sub blocks in the case of IOs).                                */

 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet])               /* Don't draw global nets. */
       continue;

    setcolor (net_color[inet]);
    b1 = net[inet].blocks[0];
    get_block_center (b1, &x1, &y1);
       
    for (ipin=1;ipin<net[inet].num_pins;ipin++) {
       b2 = net[inet].blocks[ipin];
       get_block_center (b2, &x2, &y2);
       drawline (x1,y1,x2,y2);
 /*      x1 = x2;  */    /* Uncomment to draw a chain instead of a star. */
 /*      y1 = y2;  */
    }
 }
}


static void get_block_center (int bnum, float *x, float *y) {

/* This routine finds the center of block bnum in the current placement, *
 * and returns it in *x and *y.  This is used in routine shownets.       */

 int i, j, k;
 
 i = block[bnum].x;
 j = block[bnum].y;

 if (clb[i][j].type == CLB) {
    *x = x_clb_left[i] + clb_width/2.;
    *y = y_clb_bottom[j] + clb_width/2.;
 }

 else {    /* IO clb.  Have to figure out which subblock it is. */
    for (k=0;k<clb[i][j].occ;k++) 
       if (clb[i][j].u.io_blocks[k] == bnum) 
          break;
         
    if (i == 0 || i == nx + 1) {   /* clb split vertically */
       *x = x_clb_left[i] + clb_width/2.;
       *y = y_clb_bottom[j] + (k + 0.5) * clb_width / (float) io_rat; 
    }
    else {                         /* clb split horizontally */
       *x = x_clb_left[i] + (k + 0.5) * clb_width / (float) io_rat; 
       *y = y_clb_bottom[j] + clb_width/2.;
    }
 }
}


static void draw_congestion (void) {

/* Draws all the overused routing resources (i.e. congestion) in RED.   */

 int inode, itrack;

 setcolor (RED);
 setlinewidth (2);

 for (inode=0;inode<num_rr_nodes;inode++) {
    if (rr_node[inode].occ > rr_node[inode].capacity) { 
    
       switch (rr_node[inode].type) {

       case CHANX:
          itrack = rr_node[inode].ptc_num;
          draw_rr_chanx (inode, itrack);
          break;
          
       case CHANY:
          itrack = rr_node[inode].ptc_num;
          draw_rr_chany (inode, itrack);
          break;
 
       case IPIN: case OPIN:
          draw_rr_pin (inode, RED);
          break;
  
       default:
          break;
       }
    }
 }
}


static void draw_rr (void) {

/* Draws the routing resources that exist in the FPGA, if the user wants *
 * them drawn.                                                           */

 int inode, itrack;

 if (draw_rr_toggle == DRAW_NO_RR) {
    setlinewidth (3);
    drawroute (HIGHLIGHTED);
    setlinewidth (0);
    return;
 }

 setlinestyle (SOLID);
 setlinewidth (0);

 for (inode=0;inode<num_rr_nodes;inode++) {
 
     switch (rr_node[inode].type) {
     
     case SOURCE: case SINK:
        break;         /* Don't draw. */

     case CHANX:
       setcolor (BLACK);
       itrack = rr_node[inode].ptc_num;
       draw_rr_chanx (inode, itrack);
       draw_rr_edges (inode);
       break;

     case CHANY:
       setcolor (BLACK);
       itrack = rr_node[inode].ptc_num;
       draw_rr_chany (inode, itrack);
       draw_rr_edges (inode);
       break;

    case IPIN:
       draw_rr_pin (inode, BLUE);
       break;

    case OPIN:
       draw_rr_pin (inode, RED);
       setcolor (RED);
       draw_rr_edges (inode);
       break;

    default:
       printf("Error in draw_rr:  Unexpected rr_node type: %d.\n",
               rr_node[inode].type);
       exit (1);
    }
 }
 
 setlinewidth (3);
 drawroute (HIGHLIGHTED);
 setlinewidth (0);
}


static void draw_rr_chanx (int inode, int itrack) {

/* Draws an x-directed channel segment.                       */

 float x1, x2, y;

 /* Track 0 at bottom edge, closest to "owning" clb. */

  x1 = x_clb_left[rr_node[inode].xlow];
  x2 = x_clb_left[rr_node[inode].xhigh] + clb_width;
  y = y_clb_bottom[rr_node[inode].ylow] + 1. + itrack + clb_width;
  drawline (x1, y, x2, y);
}


static void draw_rr_chany (int inode, int itrack) {
 
/* Draws a y-directed channel segment.                       */ 
 
 float x, y1, y2;
 
 /* Track 0 at left edge, closest to "owning" clb. */

  x = x_clb_left[rr_node[inode].xlow] + 1. + itrack + clb_width;
  y1 = y_clb_bottom[rr_node[inode].ylow];
  y2 = y_clb_bottom[rr_node[inode].yhigh] + clb_width;
  drawline (x, y1, x, y2);
}


static void draw_rr_edges (int inode) {

/* Draws all the edges that the user wants shown between inode and what it *
 * connects to.  inode is assumed to be a CHANX, CHANY, or OPIN.           */

 t_rr_type from_type, to_type;
 int iedge, to_node, from_ptc_num, to_ptc_num;
 short switch_type;

 from_type = rr_node[inode].type;

 if (draw_rr_toggle == DRAW_NODES_RR || (draw_rr_toggle == 
              DRAW_NODES_AND_SBOX_RR && from_type == OPIN))
    return;                                          /* Nothing to draw. */

 from_ptc_num = rr_node[inode].ptc_num;
 
 for (iedge=0;iedge<rr_node[inode].num_edges;iedge++) {
    to_node = rr_node[inode].edges[iedge];
    to_type = rr_node[to_node].type;
    to_ptc_num = rr_node[to_node].ptc_num;
    
    switch (from_type) {

    case OPIN:
       switch (to_type) {

       case CHANX: case CHANY:
          setcolor (RED);
          draw_pin_to_chan_edge (inode, to_node, to_ptc_num, TRUE);
          break;

       default:
          printf("Error in draw_rr_edges:  node %d (type: %d) connects to \n"
             "node %d (type: %d).\n", inode, from_type, to_node, to_type);
          exit (1);
          break;
       }

       break;


    case CHANX:            /* from_type */
       switch (to_type) {
       
       case IPIN:
          if (draw_rr_toggle == DRAW_NODES_AND_SBOX_RR)
             break;
          
          setcolor (BLUE);
          draw_pin_to_chan_edge (to_node, inode, from_ptc_num, TRUE);
          break;

       case CHANX:
          setcolor (DARKGREEN);
          switch_type = rr_node[inode].switches[iedge];
          draw_chanx_to_chanx_edge (inode, from_ptc_num, to_node, 
                    to_ptc_num, switch_type);
          break;

       case CHANY:
          setcolor (DARKGREEN);
          switch_type = rr_node[inode].switches[iedge];
          draw_chanx_to_chany_edge (inode, from_ptc_num, to_node, 
                     to_ptc_num, FROM_X_TO_Y, switch_type); 
          break;

       default:
          printf("Error in draw_rr_edges:  node %d (type: %d) connects to \n"
             "node %d (type: %d).\n", inode, from_type, to_node, to_type);
          exit (1);
          break;
    
       }
       break;

     
    case CHANY:                   /* from_type */
       switch (to_type) {

       case IPIN:
          if (draw_rr_toggle == DRAW_NODES_AND_SBOX_RR)
             break;
         
          setcolor (BLUE);
          draw_pin_to_chan_edge (to_node, inode, from_ptc_num, TRUE);
          break;

       case CHANX:
          setcolor (DARKGREEN);
          switch_type = rr_node[inode].switches[iedge];
          draw_chanx_to_chany_edge (to_node, to_ptc_num, inode, 
                     from_ptc_num, FROM_Y_TO_X, switch_type); 
          break;

       case CHANY:
          setcolor (DARKGREEN);
          switch_type = rr_node[inode].switches[iedge];
          draw_chany_to_chany_edge (inode, from_ptc_num, to_node,
                    to_ptc_num, switch_type);
          break;

       default:
          printf("Error in draw_rr_edges:  node %d (type: %d) connects to \n"
             "node %d (type: %d).\n", inode, from_type, to_node, to_type);
          exit (1);
          break;
       }
       break;


    default:                     /* from_type */
       printf("Error:  draw_rr_edges called with node %d of type %d.\n",
               inode, from_type);
       exit (1);
       break;
    }

 }   /* End of for each edge loop */
}


static void draw_pin_to_chan_edge (int pin_node, int chan_node, int itrack,
             boolean mark_conn) {

/* This routine draws an edge from the pin_node to the chan_node (CHANX or   *
 * CHANY).  The track number of the channel is passed in itrack, rather than *
 * taken from chan_node's ptc_num to allow this routine to draw global       *
 * routings (where the track number isn't in the rr_graph).  If mark_conn is *
 * TRUE, draw a box where the pin connects to the track (useful for drawing  *
 * the rr graph).                                                            */

 t_rr_type chan_type;
 int pin_x, pin_y, pin_num, chan_xlow, chan_ylow;
 float x1, x2, y1, y2;

 pin_x = rr_node[pin_node].xlow;
 pin_y = rr_node[pin_node].ylow;
 pin_num = rr_node[pin_node].ptc_num;
 chan_type = rr_node[chan_node].type;

 switch (chan_type) {

 case CHANX:
    chan_ylow = rr_node[chan_node].ylow;
    if (pin_y == chan_ylow) {
       get_rr_pin_draw_coords (pin_node, TOP, &x1, &y1);
       y1 += pin_size;                        /* Don't overdraw pin number. */
    }
    else {
       get_rr_pin_draw_coords (pin_node, BOTTOM, &x1, &y1);
       y1 -= pin_size;
    }
     
    y2 = y_clb_bottom[chan_ylow] + clb_width + 1 + itrack;
    x2 = x1;
    break;
 
 case CHANY:
    chan_xlow = rr_node[chan_node].xlow;
    if (pin_x == chan_xlow) {
       get_rr_pin_draw_coords (pin_node, RIGHT, &x1, &y1);
       x1 += pin_size;
    }
    else {
       get_rr_pin_draw_coords (pin_node, LEFT, &x1, &y1);
       x1 -= pin_size;
    }
 
    x2 = x_clb_left[chan_xlow] + clb_width + 1 + itrack;
    y2 = y1;
    break;

 default:
    printf ("Error in draw_pin_to_chan_edge:  invalid channel node %d.\n",
            chan_node);
    exit (1);
 }

 drawline (x1, y1, x2, y2);
 if (mark_conn)
    draw_x (x2, y2, 0.7 * pin_size);
}


static void draw_x (float x, float y, float size) {

/* Draws an X centered at (x,y).  The width and height of the X are each    *
 * 2 * size.                                                                */

 drawline (x-size, y+size, x+size, y-size);
 drawline (x-size, y-size, x+size, y+size);
}


static void draw_chanx_to_chany_edge (int chanx_node,  int chanx_track, 
         int chany_node, int chany_track, enum e_edge_dir edge_dir, short 
         switch_type) {

/* Draws an edge (SBOX connection) between an x-directed channel and a    *
 * y-directed channel.                                                    */

 float x1, y1, x2, y2;
 int chanx_y, chany_x, chanx_xlow, chany_ylow;

 chanx_y = rr_node[chanx_node].ylow;
 chanx_xlow = rr_node[chanx_node].xlow;
 chany_x = rr_node[chany_node].xlow;
 chany_ylow = rr_node[chany_node].ylow;

/* (x1,y1): point on CHANX segment, (x2,y2): point on CHANY segment. */

 y1 = y_clb_bottom[chanx_y] + clb_width + 1. + chanx_track;
 x2 = x_clb_left[chany_x] + clb_width + 1. + chany_track;

 if (chanx_xlow <= chany_x) {   /* Can draw connection going right */
    x1 = x_clb_left[chany_x] + clb_width;
 }
 else {            /* Must draw connection going left. */
    x1 = x_clb_left[chanx_xlow]; 
 }

 if (chany_ylow <= chanx_y) {   /* Can draw connection going up. */
    y2 = y_clb_bottom[chanx_y] + clb_width;
 }
 else {     /* Must draw connection going down. */
    y2 = y_clb_bottom[chany_ylow];
 }

 drawline (x1, y1, x2, y2);

 if (draw_rr_toggle != DRAW_ALL_RR) 
    return;

 if (edge_dir == FROM_X_TO_Y) 
    draw_rr_switch (x1, y1, x2, y2, switch_inf[switch_type].buffered);
 else
    draw_rr_switch (x2, y2, x1, y1, switch_inf[switch_type].buffered);
}


static void draw_chanx_to_chanx_edge (int from_node, int from_track, 
        int to_node, int to_track, short switch_type) {

/* Draws a connection between two x-channel segments.  Passing in the track *
 * numbers allows this routine to be used for both rr_graph and routing     *
 * drawing.                                                                 */

 float x1, x2, y1, y2;
 int from_y, to_y, from_xlow, to_xlow, from_xhigh, to_xhigh;

 from_y = rr_node[from_node].ylow;
 from_xlow = rr_node[from_node].xlow;
 from_xhigh = rr_node[from_node].xhigh;
 to_y = rr_node[to_node].ylow;
 to_xlow = rr_node[to_node].xlow;
 to_xhigh = rr_node[to_node].xhigh;

/* (x1, y1) point on from_node, (x2, y2) point on to_node. */

 y1 = y_clb_bottom[from_y] + clb_width + 1 + from_track;
 y2 = y_clb_bottom[to_y] + clb_width + 1 + to_track;
 
 if (to_xhigh < from_xlow) {   /* From right to left */
    x1 = x_clb_left[from_xlow];
    x2 = x_clb_left[to_xhigh] + clb_width;
 }
 else if (to_xlow > from_xhigh) { /* From left to right */
    x1 = x_clb_left[from_xhigh] + clb_width;
    x2 = x_clb_left[to_xlow];
 }

/* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
 * make sure the drawing is symmetric in the from rr and to rr so the edges *
 * will be drawn on top of each other for bidirectional connections.        */

 else if (to_xlow < from_xlow) {   /* Draw from left edge of one to other */
    x1 = x_clb_left[from_xlow];
    x2 = x_clb_left[from_xlow - 1] + clb_width;
 }
 else if (from_xlow < to_xlow) {
    x1 = x_clb_left[to_xlow - 1] + clb_width;
    x2 = x_clb_left[to_xlow];
 }
 else if (to_xhigh > from_xhigh) {  /* Draw from right edge of one to other */
    x1 = x_clb_left[from_xhigh] + clb_width;
    x2 = x_clb_left[from_xhigh + 1];
 }
 else if (from_xhigh > to_xhigh) {
    x1 = x_clb_left[to_xhigh + 1];
    x2 = x_clb_left[to_xhigh] + clb_width;
 }

 else { /* Complete overlap: start and end both align. Draw outside the sbox */
    x1 = x_clb_left[from_xlow];
    x2 = x_clb_left[from_xlow] + clb_width;
 }
 
 drawline (x1, y1, x2, y2);
 
 if (draw_rr_toggle == DRAW_ALL_RR)
    draw_rr_switch (x1, y1, x2, y2, switch_inf[switch_type].buffered);
}


static void draw_chany_to_chany_edge (int from_node, int from_track,
        int to_node, int to_track, short switch_type) {
 
/* Draws a connection between two y-channel segments.  Passing in the track *
 * numbers allows this routine to be used for both rr_graph and routing     *
 * drawing.                                                                 */
 
 float x1, x2, y1, y2;
 int from_x, to_x, from_ylow, to_ylow, from_yhigh, to_yhigh;
 
 from_x = rr_node[from_node].xlow;
 from_ylow = rr_node[from_node].ylow;
 from_yhigh = rr_node[from_node].yhigh;
 to_x = rr_node[to_node].xlow;
 to_ylow = rr_node[to_node].ylow;
 to_yhigh = rr_node[to_node].yhigh;

/* (x1, y1) point on from_node, (x2, y2) point on to_node. */
 
 x1 = x_clb_left[from_x] + clb_width + 1 + from_track;
 x2 = x_clb_left[to_x] + clb_width + 1 + to_track;

 if (to_yhigh < from_ylow) {   /* From upper to lower */
    y1 = y_clb_bottom[from_ylow];
    y2 = y_clb_bottom[to_yhigh] + clb_width;
 }
 else if (to_ylow > from_yhigh) {    /* From lower to upper */
    y1 = y_clb_bottom[from_yhigh] + clb_width;
    y2 = y_clb_bottom[to_ylow];
 }

/* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
 * make sure the drawing is symmetric in the from rr and to rr so the edges *
 * will be drawn on top of each other for bidirectional connections.        */
 
 else if (to_ylow < from_ylow) {  /* Draw from bottom edge of one to other. */
    y1 = y_clb_bottom[from_ylow];
    y2 = y_clb_bottom[from_ylow - 1] + clb_width;
 }
 else if (from_ylow < to_ylow) { 
    y1 = y_clb_bottom[to_ylow - 1] + clb_width;
    y2 = y_clb_bottom[to_ylow];
 }
 else if (to_yhigh > from_yhigh) {  /* Draw from top edge of one to other. */
    y1 = y_clb_bottom[from_yhigh] + clb_width;
    y2 = y_clb_bottom[from_yhigh + 1];
 }
 else if (from_yhigh > to_yhigh) {
    y1 = y_clb_bottom[to_yhigh + 1];
    y2 = y_clb_bottom[to_yhigh] + clb_width;
 }

 else { /* Complete overlap: start and end both align. Draw outside the sbox */
    y1 = y_clb_bottom[from_ylow];
    y2 = y_clb_bottom[from_ylow] + clb_width;
 }
 
 drawline (x1, y1, x2, y2);

 if (draw_rr_toggle == DRAW_ALL_RR)
    draw_rr_switch (x1, y1, x2, y2, switch_inf[switch_type].buffered);
}


static void draw_rr_switch (float from_x, float from_y, float to_x, float to_y,
        boolean buffered) {

/* Draws a buffer (triangle) or pass transistor (circle) on the edge        *
 * connecting from to to, depending on the status of buffered.  The drawing *
 * is closest to the from_node, since it reflects the switch type of from.  */

 const float switch_rad = 0.15;
 float magnitude, xcen, ycen, xdelta, ydelta, xbaseline, ybaseline;
 float xunit, yunit;
 t_point poly[3];
 
 xcen = from_x + (to_x - from_x) / 10.;
 ycen = from_y + (to_y - from_y) / 10.;

 if (!buffered) {    /* Draw a circle for a pass transistor */
    drawarc (xcen, ycen, switch_rad, 0., 360.);
 }
 else {  /* Buffer */
    xdelta = to_x - from_x;
    ydelta = to_y - from_y;
    magnitude = sqrt (xdelta * xdelta + ydelta * ydelta);
    xunit = xdelta / magnitude;
    yunit = ydelta / magnitude;
    poly[0].x = xcen + xunit * switch_rad;
    poly[0].y = ycen + yunit * switch_rad;
    xbaseline = xcen - xunit * switch_rad;
    ybaseline = ycen - yunit * switch_rad;

/* Recall: perpendicular vector to the unit vector along the switch (xv, yv) *
 * is (yv, -xv).                                                             */
 
    poly[1].x = xbaseline + yunit * switch_rad;
    poly[1].y = ybaseline - xunit * switch_rad;
    poly[2].x = xbaseline - yunit * switch_rad;
    poly[2].y = ybaseline + xunit * switch_rad;
    fillpoly (poly, 3);
 }
}


static void draw_rr_pin (int inode, enum color_types color) {

/* Draws an IPIN or OPIN rr_node.  Note that the pin can appear on more    *
 * than one side of a clb.  Also note that this routine can change the     *
 * current color to BLACK.                                                 */

 int ipin, i, j, iside, iclass, iblk;
 float xcen, ycen;
 char str[BUFSIZE];

 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 ipin = rr_node[inode].ptc_num;
 setcolor (color);

 if (clb[i][j].type == CLB) {
    iclass = clb_pin_class[ipin];

    for (iside=0;iside<=3;iside++) {
       if (pinloc[iside][ipin] == 1) {   /* Pin exists on this side. */
          get_rr_pin_draw_coords (inode, iside, &xcen, &ycen);
          fillrect (xcen-pin_size, ycen-pin_size, xcen+pin_size, ycen+pin_size);
          sprintf (str, "%d", ipin);
          setcolor (BLACK);
          drawtext (xcen, ycen, str, 2*pin_size);
          setcolor (color);
       }
    }
 }

 else {               /* IO pad. */
    iblk = clb[i][j].u.io_blocks[ipin];

    if (i == 0) 
       iside = RIGHT;
    else if (j == 0)
       iside = TOP;
    else if (i == nx+1)
       iside = LEFT;
    else
       iside = BOTTOM;

    get_rr_pin_draw_coords (inode, iside, &xcen, &ycen);
    fillrect (xcen-pin_size, ycen-pin_size, xcen+pin_size, ycen+pin_size);
 }
}


static void get_rr_pin_draw_coords (int inode, int iside, float *xcen, 
            float *ycen) {

/* Returns the coordinates at which the center of this pin should be drawn. *
 * inode gives the node number, and iside gives the side of the clb or pad  *
 * the physical pin is on.                                                  */

 int i, j, ipin, ipad;
 float step_size, offset, xc, yc;

 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 
 xc = x_clb_left[i];
 yc = y_clb_bottom[j];

 if (clb[i][j].type == CLB) {
    ipin = rr_node[inode].ptc_num; 
    step_size = clb_width / (pins_per_clb + 1.);
    offset = (ipin + 1.) * step_size;
 }
 else {                                        /* IO pad. */  
    ipad = rr_node[inode].ptc_num; 
    step_size = clb_width / (float) io_rat;
    offset = ipad * step_size + clb_width / (3. * io_rat);

   /* Pads have both an IPIN and an OPIN.  Stagger them. */

    if (rr_node[inode].type == IPIN) 
       offset += clb_width / (3. * io_rat);
 }

 switch (iside) {

 case LEFT:  
    yc += offset;
    break;

  case RIGHT:
     xc += clb_width;
     yc += offset;
     break;
  
  case BOTTOM:
     xc += offset;
     break;

  case TOP:
     xc += offset;
     yc += clb_width;
     break;

  default:
     printf ("Error in get_rr_pin_draw_coords:  Unexpected iside %d.\n",
             iside);
     exit (1);
     break;
 }

 *xcen = xc;
 *ycen = yc;
}


static void drawroute (enum e_draw_net_type draw_net_type) {

/* Draws the nets in the positions fixed by the router.  If draw_net_type is *
 * ALL_NETS, draw all the nets.  If it is HIGHLIGHTED, draw only the nets    *
 * that are not coloured black (useful for drawing over the rr_graph).       */

 /* Next free track in each channel segment if routing is GLOBAL */

 static int **chanx_track = NULL;           /* [1..nx][0..ny] */
 static int **chany_track = NULL;           /* [0..nx][1..ny] */

 int inet, i, j, inode, prev_node, prev_track, itrack;
 short switch_type;
 struct s_trace *tptr;
 t_rr_type rr_type, prev_type;


 if (draw_route_type == GLOBAL) {
   /* Allocate some temporary storage if it's not already available. */
    if (chanx_track == NULL) 
       chanx_track = (int **) alloc_matrix (1, nx, 0, ny, sizeof(int));

    if (chany_track == NULL) 
       chany_track = (int **) alloc_matrix (0, nx, 1, ny, sizeof(int));

    for (i=1;i<=nx;i++)
       for (j=0;j<=ny;j++)
          chanx_track[i][j] = -1;

    for (i=0;i<=nx;i++)
       for (j=1;j<=ny;j++)
          chany_track[i][j] = -1;
 }

 setlinestyle (SOLID);

/* Now draw each net, one by one.      */

 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet])           /* Don't draw global nets. */
       continue;

    if (trace_head[inet] == NULL)  /* No routing.  Skip.  (Allows me to draw */
       continue;                   /* partially complete routes).            */

    if (draw_net_type == HIGHLIGHTED && net_color[inet] == BLACK) 
       continue;
 
    setcolor (net_color[inet]);
    tptr = trace_head[inet];   /* SOURCE to start */
    inode = tptr->index;
    rr_type = rr_node[inode].type;
    

    while (1) {
       prev_node = inode;
       prev_type = rr_type;
       switch_type = tptr->iswitch;
       tptr = tptr->next;
       inode = tptr->index;
       rr_type = rr_node[inode].type;

       switch (rr_type) {

       case OPIN:
          draw_rr_pin (inode, net_color[inet]);
          break;

       case IPIN:
          draw_rr_pin (inode, net_color[inet]);
          prev_track = get_track_num (prev_node, chanx_track, chany_track);
          draw_pin_to_chan_edge (inode, prev_node, prev_track, FALSE);
          break;

       case CHANX:
          if (draw_route_type == GLOBAL)
             chanx_track[rr_node[inode].xlow][rr_node[inode].ylow]++;

          itrack = get_track_num (inode, chanx_track, chany_track);
          draw_rr_chanx (inode, itrack);

          switch (prev_type) {

          case CHANX:
             prev_track = get_track_num (prev_node, chanx_track, chany_track);
             draw_chanx_to_chanx_edge (prev_node, prev_track, inode, itrack,
                         switch_type);
             break;
 
          case CHANY:
             prev_track = get_track_num (prev_node, chanx_track, chany_track);
             draw_chanx_to_chany_edge (inode, itrack, prev_node, prev_track,
                      FROM_Y_TO_X, switch_type);
             break;

          case OPIN:
             draw_pin_to_chan_edge (prev_node, inode, itrack, FALSE);
             break;

          default:
             printf ("Error in drawroute:  Unexpected connection from an \n"
                "rr_node of type %d to one of type %d.\n", prev_type, rr_type);
             exit (1);
          }

          break;

       case CHANY:
          if (draw_route_type == GLOBAL) 
             chany_track[rr_node[inode].xlow][rr_node[inode].ylow]++;

          itrack = get_track_num (inode, chanx_track, chany_track);
          draw_rr_chany (inode, itrack);
 
          switch (prev_type) {
 
          case CHANX:
             prev_track = get_track_num (prev_node, chanx_track, chany_track);
             draw_chanx_to_chany_edge (prev_node, prev_track, inode, itrack,
                       FROM_X_TO_Y, switch_type);
             break;
 
          case CHANY:
             prev_track = get_track_num (prev_node, chanx_track, chany_track);
             draw_chany_to_chany_edge (prev_node, prev_track, inode, itrack,
                       switch_type);
             break;
 
          case OPIN:
             draw_pin_to_chan_edge (prev_node, inode, itrack, FALSE);
             break;
 
          default:
             printf ("Error in drawroute:  Unexpected connection from an \n"
                "rr_node of type %d to one of type %d.\n", prev_type, rr_type);
             exit (1);
          }
 
          break; 

       default:
          break;
      
       }
      
       if (rr_type == SINK) {   /* Skip the next segment */
          tptr = tptr->next;
          if (tptr == NULL) 
             break;
          inode = tptr->index;
          rr_type = rr_node[inode].type;
       }
       
    } /* End loop over traceback. */
 } /* End for (each net) */
}


static int get_track_num (int inode, int **chanx_track, int **chany_track) {

/* Returns the track number of this routing resource node.   */

 int i, j;
 t_rr_type rr_type;

 if (draw_route_type == DETAILED)
    return (rr_node[inode].ptc_num);

/* GLOBAL route stuff below. */

 rr_type = rr_node[inode].type;
 i = rr_node[inode].xlow;       /* NB: Global rr graphs must have only unit */
 j = rr_node[inode].ylow;       /* length channel segments.                 */

 switch (rr_type) {
 case CHANX:
    return (chanx_track[i][j]);

 case CHANY:
    return (chany_track[i][j]);

 default:
    printf ("Error in get_track_num:  unexpected node type %d for node %d."
            "\n", rr_type, inode);
    exit (1);
 }
}


static void highlight_blocks (float x, float y) {

/* This routine is called when the user clicks in the graphics area. *
 * It determines if a clb was clicked on.  If one was, it is         *
 * highlighted in green, it's fanin nets and clbs are highlighted in *
 * blue and it's fanout is highlighted in red.  If no clb was        *
 * clicked on (user clicked on white space) any old highlighting is  *
 * removed.  Note that even though global nets are not drawn, their  *
 * fanins and fanouts are highlighted when you click on a block      *
 * attached to them.                                                 */

 int i, j, k, hit, bnum, ipin, netnum, fanblk;
 int class;
 float io_step;
 char msg[BUFSIZE];

 io_step = clb_width/io_rat;
 deselect_all ();

 hit = 0;
 for (i=0;i<=nx+1;i++) {
    if (x <= x_clb_left[i]+clb_width) {
       if (x >= x_clb_left[i]) 
          hit = 1;
       break;
    }
 }
 if (!hit) {
    update_message (default_message);
    drawscreen();
    return;
 }

 hit = 0;
 for (j=0;j<=ny+1;j++) {
    if (y <= y_clb_bottom[j]+clb_width) {
       if (y >= y_clb_bottom[j])
          hit = 1;
       break;
    }
 }
 if (!hit) {
    update_message (default_message); 
    drawscreen();
    return;
 }

/* The user selected the clb at location (i,j). */
 if (clb[i][j].type == CLB) {
    if (clb[i][j].occ == 0) {
       update_message (default_message); 
       drawscreen ();
       return;
    }
    bnum = clb[i][j].u.block;
 }
 else {   /* IO block clb */
    if (i == 0 || i == nx+1)      /* Vertical columns of IOs */
       k = (int) ((y - y_clb_bottom[j]) / io_step);
    else 
       k = (int) ((x - x_clb_left[i]) / io_step);

    if (k >= clb[i][j].occ) {   /* Empty spot */
       update_message (default_message); 
       drawscreen();
       return;
    }
    bnum = clb[i][j].u.io_blocks[k];
 }

/* Highlight fanin and fanout. */

 if (block[bnum].type == OUTPAD) {
    netnum = block[bnum].nets[0];    /* Only net. */
    net_color[netnum] = BLUE;        /* Outpad drives nothing */
    fanblk = net[netnum].blocks[0];    /* Net driver */
    block_color[fanblk] = BLUE;
 }

 else if (block[bnum].type == INPAD) {
    netnum = block[bnum].nets[0];    /* Only net. */
    net_color[netnum] = RED;         /* Driven by INPAD */

/* Highlight fanout blocks in RED */

    for (ipin=1;ipin<net[netnum].num_pins;ipin++) { 
       fanblk = net[netnum].blocks[ipin];
       block_color[fanblk] = RED;
    }
 }

 else {       /* CLB block. */
    for (k=0;k<pins_per_clb;k++) {     /* Each pin on a CLB */
       netnum = block[bnum].nets[k];

       if (netnum == OPEN)
          continue;

       class = clb_pin_class[k];

       if (class_inf[class].type == DRIVER) {  /* Fanout */
          net_color[netnum] = RED;
          for (ipin=1;ipin<net[netnum].num_pins;ipin++) { 
             fanblk = net[netnum].blocks[ipin];
             block_color[fanblk] = RED;
          }
       }

       else {         /* This net is fanin to the block. */
          net_color[netnum] = BLUE;
          fanblk = net[netnum].blocks[0];
          block_color[fanblk] = BLUE;
       }
    }
 }

 block_color[bnum] = GREEN;   /* Selected block. */

 sprintf (msg, "Block %d (%s) at (%d, %d) selected.", bnum, block[bnum].name,
               i, j);
 update_message (msg);
 drawscreen ();      /* Need to erase screen. */
}


static void deselect_all (void) {

/* Sets the color of all clbs and nets to the default.  */

 int i;

 for (i=0;i<num_blocks;i++) 
    block_color[i] = LIGHTGREY;

 for (i=0;i<num_nets;i++)
    net_color[i] = BLACK;
} 
