#include <stdio.h>
#include "pr.h"
#include "ext.h"
#include "graphics.h"
#include "util.h"

static float *x_clb_left, *y_clb_bottom;
/* The left and bottom coordinates of each clb in the FPGA.    *
 * x_clb_left[0..nx+1] and y_clb_bottom[0..ny+1].              */

static float cell_width;
/* Drawn width (and height) of a clb.  Set when init_draw_coords *
 * is called.                                                    */

static enum color_types *net_color, *block_color;
/* Color in which each block and net should be drawn. */

void update_screen (int priority, char *msg) {
/* Updates the screen if the priority of the update is high enough  *
 * and the user has requested graphics.                             */

 void highlight_blocks (float x, float y);

 if (!showgraph) return;   /* Graphics turned off */
 update_message (msg); 
/*  draw_message (); */
 drawscreen();
 if (priority >= automode) {
    event_loop(highlight_blocks);
 }
 else {
    flushinput();
 }
}

 
void drawscreen (void) {

/* This is the screen redrawing routine that event_loop assumes exists.  *
 * It erases whatever is on screen, then calls redraw_screen to redraw   *
 * it.                                                                   */

 void redraw_screen (void);
 
 clearscreen();
 redraw_screen ();
}


void redraw_screen (void) {

/* The screen redrawing routine called by drawscreen and           *
 * highlight_blocks.  Call this routine instead of drawscreen if   *
 * you know you don't need to erase the current graphics, and want *
 * to avoid a screen "flash".                                      */

 void drawplace (void);
 void drawnets (void);
 void drawroute (void);

 if (pic_on_screen == PLACEMENT) {
    drawplace();
    if (show_nets) drawnets ();
 }
 else {             /* ROUTING on screen */
    drawplace();
    if (show_nets) drawroute();
 }
}


void alloc_draw_structs (void) {
/* Allocate the structures needed to draw the placement and routing.  Set *
 * up the default colors for blocks and nets.                             */

 void deselect_all (void);

 x_clb_left = (float *) my_malloc ((nx+2)*sizeof(float));
 y_clb_bottom = (float *) my_malloc ((ny+2)*sizeof(float));
 net_color = (enum color_types *) my_malloc (num_nets * 
       sizeof (enum color_types));
 block_color = (enum color_types *) my_malloc (num_blocks * 
       sizeof (enum color_types));

 deselect_all();    /* Set initial colors */
}

void init_draw_coords (float clb_width) {
/* Load the arrays containing the left and bottom coordinates of   *
 * the clbs forming the FPGA.  clb_width sets the width and height *
 * of a drawn clb.                                                 */

 int i;

 if (!showgraph) return;   /* -nodisp was selected. */

 cell_width = clb_width;
 
 x_clb_left[0] = 0.;
 for (i=1;i<=nx+1;i++)
    x_clb_left[i] = x_clb_left[i-1] + cell_width + chan_width_y[i-1] + 1.;

 y_clb_bottom[0] = 0.;
 for (i=1;i<=ny+1;i++)
    y_clb_bottom[i] = y_clb_bottom[i-1] + cell_width + 
        chan_width_x[i-1] + 1.;

 init_world (0., y_clb_bottom[ny+1] + cell_width,
    x_clb_left[nx+1]+ cell_width, 0.);
}

void drawplace (void) {
/* Draws the blocks placed on the proper clbs.  Occupied clbs are light *
 * grey, while empty ones are left white and have a dashed border.      */

 float io_step = cell_width/io_rat;
 float x1, y1, x2, y2;
 int i, j, k, bnum;

 /* Draw the IO Pads first. Want each subblock to border on core. */

 for (i=1;i<=nx;i++) { 
    for (j=0;j<=ny+1;j+=ny+1) {  /* top and bottom */
       y1 = y_clb_bottom[j];
       y2 = y1 + cell_width;

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
             block[clb[i][j].u.io_blocks[k]].name, cell_width);
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
       x2 = x1 + cell_width;

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
             block[clb[i][j].u.io_blocks[k]].name, cell_width);
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
    x2 = x1 + cell_width;
    for (j=1;j<=ny;j++) {
       y1 = y_clb_bottom[j];
       y2 = y1 + cell_width;
       if (clb[i][j].occ != 0) {
          setlinestyle (SOLID);
          bnum = clb[i][j].u.block;
          setcolor (block_color[bnum]);
          fillrect (x1,y1,x2,y2);

          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
          drawtext ((x1 + x2)/2., (y1 + y2)/2., block[clb[i][j].u.block].name, 
             cell_width);
       }
       else {
          setlinestyle (DASHED);
          setcolor (BLACK);
          drawrect (x1,y1,x2,y2);
       }
    }    /* end j */
 }    /* end i */
}

void drawnets (void) {
/* This routine draws the nets on the placement.  The nets have not *
 * yet been routed, so we just draw a chain showing a possible path *
 * for each net.  This gives some idea of future congestion.        */

 int inet, ipin, b1, b2;
 float x1, y1, x2, y2;
 void get_block_center (int bnum, float *x, float *y);
 
 setlinestyle (SOLID);
 
/* Draw the net as a chain from pin 0 to pin n. Draw from centers of *
 * blocks (or sub blocks in the case of IOs).                        */

 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet])               /* Don't draw global nets. */
       continue;

    setcolor (net_color[inet]);
    b1 = net[inet].pins[0];
    get_block_center (b1, &x1, &y1);
       
    for (ipin=1;ipin<net[inet].num_pins;ipin++) {
       b2 = net[inet].pins[ipin];
       get_block_center (b2, &x2, &y2);
       drawline (x1,y1,x2,y2);
       x1 = x2;
       y1 = y2;
    }
 }
}


void get_block_center (int bnum, float *x, float *y) {
/* This routine finds the center of block bnum in the current placement, *
 * and returns it in *x and *y.  This is used in routine shownets.       */

 int i, j, k;
 
 i = block[bnum].x;
 j = block[bnum].y;

 if (clb[i][j].type == CLB) {
    *x = x_clb_left[i] + cell_width/2.;
    *y = y_clb_bottom[j] + cell_width/2.;
 }

 else {    /* IO clb.  Have to figure out which subblock it is. */
    for (k=0;k<clb[i][j].occ;k++) 
       if (clb[i][j].u.io_blocks[k] == bnum) 
          break;
         
    if (i == 0 || i == nx + 1) {   /* clb split vertically */
       *x = x_clb_left[i] + cell_width/2.;
       *y = y_clb_bottom[j] + (k + 0.5) * cell_width / (float) io_rat; 
    }
    else {                         /* clb split horizontally */
       *x = x_clb_left[i] + (k + 0.5) * cell_width / (float) io_rat; 
       *y = y_clb_bottom[j] + cell_width/2.;
    }
 }
}


void drawroute (void) {
/* Draws the nets in the positions fixed by the router.  */

 static float **x_chan_y_coord = NULL;
 /* [1..nx][0..ny]  The y coordinate for the next wire in this segment */

 static float **y_chan_x_coord = NULL;
 /* [0..nx][1..ny]  The x coordinate for the next wire in this segment */

 float x1, x2, y1, y2, pin_step, io_step;
 int inet, i, j;
 struct s_trace *tptr, *prev_ptr;
 void get_io_pin_loc (struct s_trace *tptr, float *x, float *y);


/* Allocate some temporary storage if it's not already available. */
 if (x_chan_y_coord == NULL) 
    x_chan_y_coord = (float **) alloc_matrix (1,nx,0,ny,sizeof(float));

 if (y_chan_x_coord == NULL) 
    y_chan_x_coord = (float **) alloc_matrix (0,nx,1,ny,sizeof(float));

/* Wires in channels are drawn 1 unit apart.  The first wire is at     *
 * the top of horizontal channels and the right side of vertical ones. */

 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++)
       x_chan_y_coord[i][j] = y_clb_bottom[j+1] - 1.;

 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++)
       y_chan_x_coord[i][j] = x_clb_left[i+1] - 1.;

/* Spacing between CLB pins. */
 pin_step = cell_width / (clb_size + 1.); 

/* spacing between IO pins in the same clb. */
 io_step = cell_width / (float) io_rat;

 setlinestyle (SOLID);

/* Now draw each net, one by one.      */

 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet])           /* Don't draw global nets. */
       continue;

    setcolor (net_color[inet]);
    prev_ptr = trace_head[inet];   /* OPIN to start */
    tptr = prev_ptr;         

    do {
       prev_ptr = tptr;
       tptr = tptr->next;
       i = tptr->i;
       j = tptr->j;

       switch (tptr->type) {

       case CHANX:
          x1 = x_clb_left[i];
          x2 = x1 + cell_width;
          y1 = x_chan_y_coord[i][j];
          y2 = y1;
          drawline (x1,y1,x2,y2);

/* Now draw the connection to the previous traceback element. */
          if (prev_ptr->type == OPIN) {
             if (clb[prev_ptr->i][prev_ptr->j].type == CLB) 
                x1 = x_clb_left[i] + pin_step * (prev_ptr->pinnum + 1.);
             else    /* IO cell, must be on top or bottom */
                x1 = x_clb_left[i] + io_step * (prev_ptr->pinnum + 0.5);

             x2 = x1;
             if (prev_ptr->j == j)        /* clb below channel */
                y1 = y_clb_bottom[prev_ptr->j] + cell_width; 
             else 
                y1 = y_clb_bottom[prev_ptr->j];
          }
           

          else if (prev_ptr->type == CHANY) {
             if (prev_ptr->i == i)   /* To right of current */
                x1 = x_clb_left[i] + cell_width;

/* The plus 1 below accounts for the previous decrementing of the  *
 * channel drawing location for the prev_ptr segment.              */
             x2 = y_chan_x_coord[prev_ptr->i][prev_ptr->j] + 1.;   
             if (prev_ptr->j == j)   /* Below current channel */
                y2 = y_clb_bottom[j] + cell_width;
             else
                y2 = y_clb_bottom[j+1];
          }

          else if (prev_ptr->type == CHANX) {
             if (prev_ptr->i == i-1) {  /* To left of current */
                x2 = x_clb_left[prev_ptr->i] + cell_width;
                y2 = x_chan_y_coord[prev_ptr->i][prev_ptr->j] + 1.;
             }
             else {                    /* To the right of current */
                x1 = x_clb_left[prev_ptr->i];
                y1 = x_chan_y_coord[prev_ptr->i][prev_ptr->j] + 1.;
             }
          }
          
          else {
             printf("Error in drawroute.  Net %d has an",inet);
             printf(" unexpected predecessor to CHANX.\n"); 
             exit (1);
          }

          drawline (x1,y1,x2,y2);
          x_chan_y_coord[i][j]--;    /* Next available track */
          break;

       case CHANY:
          x1 = y_chan_x_coord[i][j];
          x2 = x1;
          y1 = y_clb_bottom[j];
          y2 = y1 + cell_width;
          drawline (x1,y1,x2,y2);

/* Now draw the connection to the previous traceback element. */
          if (prev_ptr->type == OPIN) {
             if (clb[prev_ptr->i][prev_ptr->j].type == CLB) 
                y1 = y_clb_bottom[j] + pin_step * (prev_ptr->pinnum + 1.);
             else
                y1 = y_clb_bottom[j] + io_step * (prev_ptr->pinnum + 0.5);

             y2 = y1;
             if (prev_ptr->i == i)        /* clb to left of channel */
                x1 = x_clb_left[prev_ptr->i] + cell_width; 
             else 
                x1 = x_clb_left[prev_ptr->i];
          }

          else if (prev_ptr->type == CHANX) {
             if (prev_ptr->j == j)       /* Above current */
                y1 = y_clb_bottom[j] + cell_width;
             
/* The plus 1 below accounts for the previous decrementing of the  *
 * channel drawing location for the prev_ptr segment.              */
             y2 = x_chan_y_coord[prev_ptr->i][prev_ptr->j] + 1.;
             if (prev_ptr->i == i)       /* Channel to left */
                x2 = x_clb_left[prev_ptr->i] + cell_width;
             else
                x2 = x_clb_left[prev_ptr->i];
          }

          else if (prev_ptr->type == CHANY) {
             if (prev_ptr->j == j-1) {  /* Below current */
                y2 = y_clb_bottom[prev_ptr->j] + cell_width;
                x2 = y_chan_x_coord[prev_ptr->i][prev_ptr->j] + 1.;
             }
             else {
                y1 = y_clb_bottom[prev_ptr->j];
                x1 = y_chan_x_coord[prev_ptr->i][prev_ptr->j] + 1.;
             }
          }

          
          else {
             printf("Error in drawroute.  Net %d has an",inet);
             printf(" unexpected predecessor to CHANY.\n"); 
             exit (1);
          }

          drawline (x1,y1,x2,y2);
          y_chan_x_coord[i][j]--;  /* Next available track. */
          break;
       
       case IPIN:
          if (prev_ptr->type == CHANX) {
             if (clb[i][j].type == CLB) 
                x1 = x_clb_left[i] + pin_step * (tptr->pinnum + 1.);
             else
                x1 = x_clb_left[i] + io_step * (tptr->pinnum + 0.5);

             x2 = x1;
             y1 = x_chan_y_coord[prev_ptr->i][prev_ptr->j] + 1.;
             if (prev_ptr->j == j)            /* Channel above */ 
                y2 = y_clb_bottom[j] + cell_width;
             else
                y2 = y_clb_bottom[j];
          }
          
          else if (prev_ptr->type == CHANY) {
             x1 = y_chan_x_coord[prev_ptr->i][prev_ptr->j] + 1.;
             if (prev_ptr->i == i)            /* Channel to right */
                x2 = x_clb_left[i] + cell_width;
             else 
                x2 = x_clb_left[i];

             if (clb[i][j].type == CLB) 
                y1 = y_clb_bottom[j] + pin_step * (tptr->pinnum + 1.);
             else 
                y1 = y_clb_bottom[j] + io_step * (tptr->pinnum + 0.5);

             y2 = y1;
          }

          else {
             printf("Error in drawroute.  Net %d has an",inet);
             printf(" unexpected predecessor to IPIN.\n");
             exit (1);
          }

          drawline (x1,y1,x2,y2);
          break;

       default:
          printf("Error in drawroute:  Illegal traceback type %d\n",
             tptr->type);
          printf("found in traceback of net %d.\n",inet);
          exit (1);
       }   /* End switch (segment type) */
      
       if (tptr->type == IPIN) {   /* Skip the next segment */
          prev_ptr = tptr;
          tptr = tptr->next;
       }
       
    } while (tptr != NULL);  /* While traceback continues */
 } /* End for (each net) */
}

void highlight_blocks (float x, float y) {
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
 void deselect_all (void);
 void redraw_screen (void);

 io_step = cell_width/io_rat;
 deselect_all ();

 hit = 0;
 for (i=0;i<=nx+1;i++) {
    if (x <= x_clb_left[i]+cell_width) {
       if (x >= x_clb_left[i]) 
          hit = 1;
       break;
    }
 }
 if (!hit) {
    drawscreen();
    return;
 }

 hit = 0;
 for (j=0;j<=ny+1;j++) {
    if (y <= y_clb_bottom[j]+cell_width) {
       if (y >= y_clb_bottom[j])
          hit = 1;
       break;
    }
 }
 if (!hit) {
    drawscreen();
    return;
 }

/* The user selected the clb at location (i,j). */
 if (clb[i][j].type == CLB) {
    if (clb[i][j].occ == 0) {
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
       drawscreen();
       return;
    }
    bnum = clb[i][j].u.io_blocks[k];
 }

 block_color[bnum] = GREEN;

/* Do the output net(s) first.  */

 if (block[bnum].type == OUTPAD) {
    netnum = block[bnum].nets[0];    /* Only net. */
    net_color[netnum] = BLUE;        /* Outpad drives nothing */
    fanblk = net[netnum].pins[0];    /* Net driver */
    block_color[fanblk] = BLUE;
 }

 else if (block[bnum].type == INPAD) {
    netnum = block[bnum].nets[0];    /* Only net. */
    net_color[netnum] = RED;         /* Driven by INPAD */

/* Highlight fanout blocks in RED */

    for (ipin=1;ipin<net[netnum].num_pins;ipin++) { 
       fanblk = net[netnum].pins[ipin];
       block_color[fanblk] = RED;
    }
 }

 else {       /* CLB block. */
    for (k=0;k<clb_size;k++) {     /* Each pin on a CLB */
       netnum = block[bnum].nets[k];

       if (netnum == OPEN)
          continue;

       class = clb_pin_class[k];

       if (class_inf[class].type == DRIVER) {  /* Fanout */
          net_color[netnum] = RED;
          for (ipin=1;ipin<net[netnum].num_pins;ipin++) { 
             fanblk = net[netnum].pins[ipin];
             block_color[fanblk] = RED;
          }
       }

       else {         /* This net is fanin to the block. */
          net_color[netnum] = BLUE;
          fanblk = net[netnum].pins[0];
          block_color[fanblk] = BLUE;
       }
    }
 }

 redraw_screen ();
}


void deselect_all (void) {
/* Sets the color of all clbs and nets to the default.  */

 int i;

 for (i=0;i<num_blocks;i++) 
    block_color[i] = LIGHTGREY;

 for (i=0;i<num_nets;i++)
    net_color[i] = BLACK;
} 
