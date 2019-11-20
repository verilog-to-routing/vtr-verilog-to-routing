#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "util.h"
#include "pr.h"
#include "ext.h" 

/* Contains the cost and occupancy of each input pin for each CLB clb. *
 * pin_inf[1..nx][1..ny][0..clb_size-1].occ or .cost.                  */
static struct s_pin ***pin_inf;

static int *pin_done;

static struct s_heap **heap;  /* Indexed from [1..heap_size] */
static int heap_size;   /* Number of slots in the heap array */
static int heap_tail;   /* Index of first unused slot in the heap array */

/* For managing my own list of currently free heap data structures.     */
static struct s_heap *heap_free_head=NULL; 

/* For managing my own list of currently free trace data structures.    */
static struct s_trace *trace_free_head=NULL;

/* From [0..num_nets-1].  Stores the EXPANDED bounding box requested *
 * by the user, properly clipped at the FPGA edges.  The router will *
 * not search for routings outside of this box.                      */

#ifdef DEBUG
 static int num_trace_allocated = 0;   /* To watch for memory leaks. */
 static int num_heap_allocated = 0;
 static int num_linked_f_pointer_allocated = 0;
#endif

static struct s_linked_f_pointer *chan_modified_head;
static struct s_linked_f_pointer *linked_f_pointer_free_head = NULL;

/*  The numbering relation between the channels and clbs is:              *
*                                                                         *
*  |   IO    | chan_   |   CLB      | chan_   |   CLB     |               *
*  |clb[0][2]| y[0][2] | clb[1][2]  | y[1][2] |  clb[2][2]|               *
*  +-------- +         +------------+         +-----------+               *
*                                                           } capacity in *
*   No channel          chan_x[1][1]          chan_x[2][1]  } chan_width  *
                                                            } _x[1]       *
*  +---------+         +------------+         +-----------+               *
*  |         | chan_   |            | chan_   |           |               *
*  |  IO     | y[0][1] |    CLB     | y[1][1] |   CLB     |               *
*  |clb[0][1]|         |  clb[1][1] |         | clb[2][1] |               *
*  |         |         |            |         |           |               *
*  +---------+         +------------+         +-----------+               *
*                                                           } capacity in *
*                      chan_x[1][0]           chan_x[2][0]  } chan_width  * 
*                                                           } _x[0]       *
*                      +------------+         +-----------+               *
*              No      |            | No      |           |               *
*            Channel   |    IO      | Channel |   IO      |               *
*                      |  clb[1][0] |         | clb[2][0] |               *
*                      |            |         |           |               *
*                      +------------+         +-----------+               *
*                                                                         *
*             {=======}              {=======}                            *
*            Capacity in            Capacity in                           *
*          chan_width_y[0]        chan_width_y[1]                         *
*                                                                         */


void save_routing (struct s_trace **best_routing) {
/* This routing frees any routing currently held in best routing,    *
 * then copies over the current routing (held in trace_head), and    *
 * finally sets trace_head and trace_tail to all NULLs so that the   *
 * connection to the saved routing is broken.  This is necessary so  *
 * that the next iteration of the router does not free the saved     *
 * routing elements.                                                 */

 int inet;
 struct s_trace *tptr, *tempptr;
 void free_trace_data (struct s_trace *tptr);

 for (inet=0;inet<num_nets;inet++) {

/* Free any previously saved routing.  It is no longer best. */
    tptr = best_routing[inet];
    while (tptr != NULL) {
       tempptr = tptr->next;
       free_trace_data (tptr);
       tptr = tempptr;
    }    

/* Save a pointer to the current routing in best_routing. */
    best_routing[inet] = trace_head[inet];

/* Set the current (working) routing to NULL the current trace elements *
 * won't be reused by the memory allocator.                             */
    trace_head[inet] = NULL;
    trace_tail[inet] = NULL;
 }
}

void restore_routing (struct s_trace **best_routing) {
/* Deallocates any current routing in trace_head, and replaces it with    *
 * the routing in best_routing.  Best_routing is set to NULL to show that *
 * it no longer points to a valid routing.  NOTE:  trace_tail is not      *
 * restored -- it is set to all NULLs since it is only used in            *
 * update_traceback.  If you need trace_tail restored, modify this        *
 * routine.                                                               */

 int inet;
 void free_traceback (int inet);

 for (inet=0;inet<num_nets;inet++) {

   /* Free any current routing. */
    free_traceback (inet);

  /* Set the current routing to the saved one. */
   trace_head[inet] = best_routing[inet];
   best_routing[inet] = NULL;          /* No stored routing. */
 }
}

void get_serial_num (void) {
/* This routine finds a "magic cookie" for the routing and prints it.    *
 * Use this number as a routing serial number to ensure that programming *
 * changes do not break the router.                                      */

 int inet, serial_num;
 struct s_trace *tptr;
 serial_num = 0;

 for (inet=0;inet<num_nets;inet++) {

/* Global nets will have null trace_heads (never routed) so they *
 * are not included in the serial number calculation.            */

    tptr = trace_head[inet];
    while (tptr != NULL) {
       serial_num += inet* (tptr->i * (nx+1) - tptr->j);

       if (tptr->type == IPIN || tptr-> type == OPIN) 
          serial_num -= tptr->pinnum * inet * 10;

       serial_num -= tptr->type * inet * 100;
       serial_num %= 2000000000;  /* Prevent overflow */
       tptr = tptr->next;
    }
 }
 printf ("Serial number (magic cookie) for the routing is: %d\n",
    serial_num);
 
}

int try_route (int width_fac, int initial_cost_type, 
   float initial_pres_fac, float pres_fac_mult, float acc_fac_mult,
   float bend_cost, int block_update_type, int max_block_update,
   int max_immediate_update) {

/* Attempts a routing via an iterated maze router algorithm.  Width_fac *
 * specifies the relative widtsh of the channels, while the rest of the *
 * options determine the value of the costs assigned to each channel    *
 * segment, whether an initial routing based on expected occupancy is   *
 * performed, etc.                                                      */

 int success, inet, itry;
 float pres_fac, acc_fac;
 void init_route_structs (int initial_cost_type);
 void route_net (int inet, float bend_cost);
 void free_trace_data (struct s_trace *tptr);
 void block_update_cost (float pres_fac, float acc_fac,
       int block_update_type);
 void immediate_update_cost (float pres_fac);
 void update_one_cost (int inet, int add_or_sub, float pres_fac,
       int resources_to_update);
 void pathfinder_update_one_cost (int inet, int add_or_sub, float pres_fac,
       float acc_fac);
 void pathfinder_update_cost (float pres_fac, float acc_fac);
 void update_occupancy (void);
 int check_routing (void);
 void init_chan (int width_fac);

 printf("\nAttempting routing with a width factor (usually maximum channel ");
 printf("width) of %d.\n",width_fac);

/* Set the channel widths */

 init_chan (width_fac);

/* Get initial channel cost estimate */

 init_route_structs(initial_cost_type);

/* Do a full routing with this cost estimate, unless the user specified  *
 * that this should not be done.                                         */

 if (initial_cost_type != NONE) {

    for (inet=0;inet<num_nets;inet++) 
       if (is_global[inet] == 0) {      /* Skip global nets. */
          route_net(inet, bend_cost);
       }

    update_occupancy ();
    success = check_routing ();
    if (success) {
       printf("Initial routing was successful!\n");
       return (1);
    }
 }     /* End skip_initial test. */


/* Now iterate to improve the routing.                             *
 * First do routings where the cost function is updated only after *
 * all nets have been routed (ala PathFinder, FPGA 95 p. 111).     */

 if (block_update_type == PATHFINDER) {  /* Use true Pathfinder algorithm */

/*    pres_fac = initial_pres_fac; */
    pres_fac = 0.;           /* First iteration finds shortest path. */
    acc_fac = acc_fac_mult;

    for (itry=1;itry<=max_block_update;itry++) {
      
       pathfinder_update_cost (pres_fac, acc_fac);

       for (inet=0;inet<num_nets;inet++) {
          if (is_global[inet] == 0) {       /* Skip global nets. */
 
             pathfinder_update_one_cost (inet, -1, pres_fac, acc_fac);
             route_net(inet, bend_cost);
             pathfinder_update_one_cost (inet, 1, pres_fac, acc_fac);

          } 
       } 
       
       success = check_routing ();
       if (success) {
          printf("Successfully routed after %d block update routings.\n", 
                   itry);
          return (1);
       }   
       
       if (itry == 1) 
          pres_fac = initial_pres_fac;
       else 
          pres_fac *= pres_fac_mult; 

      /* May want to try updating acc_fac too */
    }
 }

 else {                      /* Use my initial ideas on Pathfinder. */

    pres_fac = initial_pres_fac;
    acc_fac = pres_fac * acc_fac_mult;


    for (itry=1;itry<=max_block_update;itry++) {

       /* occupancy is still valid */

       block_update_cost (pres_fac, acc_fac, block_update_type); 

       for (inet=0;inet<num_nets;inet++) {
          if (is_global[inet] == 0) {       /* Skip global nets. */

             if (block_update_type == MIXED)    /* Delete pins of net */
                update_one_cost (inet, -1, pres_fac, PINS_ONLY);

             route_net(inet, bend_cost);
          
             if (block_update_type == MIXED)  /* Recompute pin costs */
                update_one_cost (inet, 1, pres_fac, PINS_ONLY);
  
          }
       }

       update_occupancy ();
       success = check_routing ();
       if (success) {
          printf("Successfully routed after %d block update routings.\n",
                   itry);
          return (1);
       }

       pres_fac *= pres_fac_mult;
       acc_fac = pres_fac * acc_fac_mult;
    }
 }


/* Can now do immediate cost update routings to remove logically equivalent *
 * pin sharing that some types of block update routing cannot handle.       */
 

 for (itry=1;itry<=max_immediate_update;itry++) {

 /* Necessary every time because pres_fac changes. */

    immediate_update_cost (pres_fac);      /* Occupancy already valid. */

    for (inet=0;inet<num_nets;inet++) {
       
       if (is_global[inet])    /* Skip global nets. */
          continue;

       update_one_cost (inet, -1, pres_fac, ALL);  /* Rip up net. */
       route_net (inet, bend_cost);                /* Re-route */
       update_one_cost (inet, 1, pres_fac, ALL);   /* Update cost */
    }

    success = check_routing ();
    if (success) {
       printf("Successfully routed after %d immedate update routings.\n",itry);
       printf("(following %d block update routings).\n",max_block_update);
       return (1);
    }

    pres_fac *= pres_fac_mult;
 }

 printf("Routing failed.\n");
 return(0);
}


int check_routing (void) {
/* This routine checks to see if this is a resource-feasible routing. *
 * That is, are all channel capacity limitations respected, and is    *
 * there at most one signal per pin?  It assumes that the occupancy   *
 * arrays are up to date when it is called.  It returns 0 if the      *
 * routing is illegal, and 1 if it is valid.                          */

 int i, j, ipin;

 for (i=1;i<=nx;i++)
    for (j=1;j<=ny;j++)
       for (ipin=0;ipin<clb_size;ipin++) 
          if (pin_inf[i][j][ipin].occ > 1) 
             return (0);

 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++) 
       if (chan_x[i][j].occ > chan_width_x[j])
          return (0);
     
 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) 
       if (chan_y[i][j].occ > chan_width_y[i])
          return (0);

 return (1);
}

void check_connectivity (void) {
/* This routine checks that a routing:  (1) Describes a properly    *
 * connected path for each net, (2) this path connects all the      *
 * pins spanned by that net, and (3) that no routing resources are  *
 * oversubscribed (the occupancy of everything is recomputed from   *
 * scratch).  The .flag elements of the channels are used for       *
 * scratch space.  These .flag elements must initially all be zero. */

 int inet, ipin, bnum, valid, class, connects; 
 struct s_trace *tptr, *prev_ptr;
 void check_segment (struct s_trace *tptr);
 int check_adjacent (struct s_trace *tptr, struct s_trace *prev_ptr);
 void check_linked_in_path (struct s_trace *tptr, int inet);
 void reset_flags (int inet);
 void update_occupancy (void);
 int check_routing (void);

/* Recompute the occupancy from scratch and check for overuse of routing *
 * resources.  This was already checked in order to determine that this  *
 * is a successful routing, but I want to double check it here.          */

 update_occupancy ();
 valid = check_routing ();
 if (valid == 0) {
    printf("Error in check_connectivity -- routing resources are overused.\n");
    exit(1);
 }

/* Now check that all nets are indeed connected. */

 for (inet=0;inet<num_nets;inet++) {

    if (is_global[inet])               /* Skip global nets. */
       continue;

    for (ipin=0;ipin<net[inet].num_pins;ipin++) 
       pin_done[ipin] = 0;

/* Check the output pin of the net. */

    tptr = trace_head[inet];   
    check_segment (tptr);

    if (tptr->type != OPIN) {
       printf("Error in check_connectivity:  First segment of net %d\n",inet);
       printf("was of type %d.  Expected type OPIN.\n",tptr->type);
       exit(1);
    }

    check_linked_in_path (tptr, inet);
    pin_done[0] = 1;
    prev_ptr = tptr;
    tptr = prev_ptr->next;

/* Check the rest of the net */

    while (tptr != NULL) {
       check_segment (tptr);
       if (prev_ptr->type == IPIN) {
          check_linked_in_path (tptr, inet);
       }
       else {
          connects = check_adjacent (tptr, prev_ptr);
          if (connects != 1) {
             printf("Error in check_connectivity while checking net %d.\n",
                inet);
             printf("Non-adjacent segments in traceback.\n");
             exit (1);
          }

          if (tptr->type == CHANX) 
             chan_x[tptr->i][tptr->j].flag = 1;
          if (tptr->type == CHANY) 
             chan_y[tptr->i][tptr->j].flag = 1;
          if (tptr->type == IPIN) {
             for (ipin=0;ipin<net[inet].num_pins;ipin++) {
                if (pin_done[ipin] == 0) {
                   bnum = net[inet].pins[ipin];
                   if (clb[tptr->i][tptr->j].type == CLB) {
                      if (clb[tptr->i][tptr->j].u.block == bnum) {
                         class = clb_pin_class[tptr->pinnum];
                         if (class == net_pin_class[inet][ipin]) {
                            pin_done[ipin] = 1;
                            break;
                         }
                      }
                   }
                   else {   /* IO block */
                      if (clb[tptr->i][tptr->j].u.io_blocks[tptr->pinnum] ==
                            bnum) {
                         pin_done[ipin] = 1;
                         break;
                      }
                   }
                }
             }
          }    /* End of tptr-> type == IPIN */
       }    /* End of prev_ptr-> != IPIN */
       prev_ptr = tptr;
       tptr = prev_ptr->next;
    }  /* End while */

    if (prev_ptr->type != IPIN) {
       printf("Error in check_connectivity.  Net %d does not end\n", inet);
       printf("with an IPIN.\n");
       exit(1);
    }

    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       if (pin_done[ipin] == 0) {
          printf("Error in check_connectivity.  Net %d does not \n",inet);
          printf("connect to pin %d.\n",ipin);
          exit(1);
       }
    }
    
    reset_flags (inet);

 }  /* End for each net */

 printf("\n\nCompleted routing consistency check successfully.\n");

}

void reset_flags (int inet) {
/* This routine resets the flags of all the channel segments contained *
 * in the traceback of net inet to 0.  This allows us to check the     *
 * next net for connectivity (and the default state of the flags       *
 * should always be zero after they have been used).                   */

 struct s_trace *tptr;

 tptr = trace_head[inet];

 while (tptr != NULL) {
    if (tptr->type == CHANX) 
       chan_x[tptr->i][tptr->j].flag = 0;
    if (tptr->type == CHANY)
       chan_y[tptr->i][tptr->j].flag = 0;
    tptr = tptr->next;
 }
}

int check_adjacent (struct s_trace *to_ptr, struct s_trace *from_ptr) {
/* This routine checks if the channel segment or pin pointed to by  *
 * to_ptr is reachable from from_ptr.  It returns 1 if is reachable *
 * and 0 if it is not.                                              */

 int reached, ifrom, jfrom, ito, jto;

 reached = 0;
 ifrom = from_ptr->i;
 jfrom = from_ptr->j;
 ito = to_ptr->i;
 jto = to_ptr->j;

 if (from_ptr->type == OPIN) {
    if (clb[ifrom][jfrom].type == CLB) {   /* from is an OPIN on a CLB */
       if (to_ptr->type == CHANX) {
          if (pinloc[TOP][from_ptr->pinnum]) 
             if (ifrom == ito && jfrom == jto) 
                reached++;
          if (pinloc[BOTTOM][from_ptr->pinnum])
             if (ifrom == ito && jfrom == jto+1) 
                reached++;
       }
       if (to_ptr->type == CHANY) {
          if (pinloc[RIGHT][from_ptr->pinnum]) 
             if (ifrom == ito && jfrom == jto) 
                reached++;
          if (pinloc[LEFT][from_ptr->pinnum]) 
             if (ifrom == ito+1 && jfrom == jto)
                reached++;
       }
    }

    else {                           /* from is an OPIN on an IO block. */
       if (jfrom == 0) 
          if (to_ptr->type == CHANX)
             if (ifrom == ito && jfrom == jto)
                reached++;
     
       if (jfrom == ny+1) 
          if (to_ptr->type == CHANX)
             if (ifrom == ito && jfrom == jto+1)
                reached++;

       if (ifrom == 0)
          if (to_ptr->type == CHANY)
             if (ifrom == ito && jfrom == jto)
                reached++;

       if (ifrom == nx+1)
          if (to_ptr->type == CHANY)
             if (ifrom == ito+1 && jfrom == jto)
                reached++;
    }
 }

 else if (from_ptr->type == CHANX) {
    if (to_ptr->type == CHANX) {
       if (jfrom == jto) 
          if (ifrom == ito+1 || ifrom == ito-1) 
             reached++;
    }

    else if (to_ptr->type == CHANY) {
       if (jfrom == jto || jfrom == jto-1)
          if (ifrom == ito || ifrom == ito+1)
             reached++;
    }
   
    else if (to_ptr->type == IPIN) {
       if (clb[ito][jto].type == CLB) {
          if (ifrom == ito) {
             if (jfrom == jto && pinloc[TOP][to_ptr->pinnum] == 1)
                reached++;
             if (jfrom == jto-1 && pinloc[BOTTOM][to_ptr->pinnum] == 1)
                reached++;
          }
       }

       else {   /* This IPIN is on an IO block (OUTPAD actually) */
          if (jto == 0) {
             if (ifrom == ito && jfrom == jto) 
                reached++;
          }
          else if (jto == ny+1) {
             if (ifrom == ito && jfrom == jto-1) 
                reached++;
          }
       }
    }   /* End to_ptr type is IPIN */
 }   /* End from_ptr type is CHANX */

 else if (from_ptr->type == CHANY) {
    if (to_ptr->type == CHANY) { 
       if (ifrom == ito) 
          if (jfrom == jto+1 || jfrom == jto-1)  
             reached++;
    }
 
    else if (to_ptr->type == CHANX) {
       if (jfrom == jto || jfrom == jto+1)
          if (ifrom == ito || ifrom == ito-1) 
             reached++; 
    } 
    
    else if (to_ptr->type == IPIN) { 
       if (clb[ito][jto].type == CLB) {
          if (jfrom == jto) { 
             if (ifrom == ito && pinloc[RIGHT][to_ptr->pinnum] == 1) 
                reached++;
             if (ifrom == ito-1 && pinloc[LEFT][to_ptr->pinnum] == 1) 
                reached++; 
          } 
       }

       else {   /* IPIN is on an IO block */
          if (ito == 0) {
             if (ifrom == ito && jfrom == jto) 
                   reached++;
          }
          else if (ito == nx+1) {
             if (ifrom == ito-1 && jfrom == jto) 
                reached++;
          }
       }
    }    /* End to_ptr is IPIN type. */
 }    /* End from_ptr is CHAN_Y type. */

 if (reached != 1) {
    printf("Error in check_adjacent.  Reached = %d (expected 0 or 1)\n",
        reached);
    exit(1);
 }

 return (reached);
}


void check_linked_in_path (struct s_trace *tptr, int inet) {

/* This routine checks that a channel segment which starts a new path  *
 * has a connection to the previously made paths.   inet is the net    *
 * to which this segment should be connected.                          */
 
 int connected, i, j, bnum, class;

 connected = 0;
 i = tptr->i;
 j = tptr->j;

 if (tptr->type == CHANX) {
    if (chan_x[i][j].flag == 1) 
       connected = 1;
 }
 
 else if (tptr->type == CHANY) {
    if (chan_y[i][j].flag == 1) 
       connected = 1;
 }
 
 else if (tptr->type == OPIN) {
    bnum = net[inet].pins[0];
    if (clb[i][j].type == CLB) {
       if (clb[i][j].u.block == bnum) {
          class = net_pin_class[inet][0];
          if (clb_pin_class[tptr->pinnum] == class)
             connected = 1;
       }
    } 
    else {   /* IO block */
       if (clb[i][j].u.io_blocks[tptr->pinnum] == bnum) 
          connected = 1;
    }
 }

 if (connected != 1) {
    bnum = net[inet].pins[0];
    printf("Error in check_linked_in_path.  Net driven by block %d has\n",bnum);
    printf("a disconnected path.\n");
    exit(1);
 }

}

void check_segment (struct s_trace *tptr) {

/* This routine checks that a channel segment or pin is inside the  *
 * grid and has a valid pin number (if applicable).                 */

 int i, j, class;

 i = tptr->i;
 j = tptr->j;

 switch (tptr->type) {
 case CHANX:
    if (i < 1 || i > nx || j < 0 || j > ny) {
       printf("Error in check_segment:  CHANX out of range.\n");
       printf("(i,j) = (%d,%d)\n",i,j);
       exit(1);
    }
    break;

 case CHANY:
    if (i < 0 || i > nx || j < 1 || j > ny) {
       printf("Error in check_segment:  CHANY out of range.\n");
       printf("(i,j) = (%d,%d)\n",i,j);
       exit(1);
    }
    break;

 case OPIN:  
    if (i < 0 || i > nx+1 || j < 0 || j > ny+1) {
       printf("Error in check_segment:  OPIN out of range.\n");
       printf("(i,j) = (%d,%d)\n",i,j);
       exit(1);
    } 
    if (clb[i][j].type == ILLEGAL) {
       printf("Error in check_segment:  OPIN in illegal position.\n");
       printf("(i,j) = (%d,%d)\n",i,j);
       exit(1);
    }
    if (clb[i][j].type == CLB) {
       class = clb_pin_class[tptr->pinnum];
       if (class_inf[class].type != DRIVER) {
          printf("Error in check_segment:  OPIN is not on a clb output pin.\n");
          printf("Pin number is %d\n",tptr->pinnum);
          exit(1);
       }
    }

    else {
       if (tptr->pinnum < 0 || tptr->pinnum >= clb[i][j].occ) {
          printf("Error in check_segment:  IO OPIN pin number out of range.\n");
          printf("Pin number is %d\n",tptr->pinnum);
          exit(1);
       }
    }

    break;

 case IPIN:
    if (i < 0 || i > nx+1 || j < 0 || j > ny+1) {
       printf("Error in check_segment:  IPIN out of range.\n");
       printf("(i,j) = (%d,%d)\n",i,j);    
       exit(1);
    } 

    if (clb[i][j].type == CLB) {
       class = clb_pin_class[tptr->pinnum];
       if (class_inf[class].type != RECEIVER) {
          printf("Error in check_segment:  IPIN not on clb input pin.\n"); 
          printf("Pin number is %d\n",tptr->pinnum);
          exit(1);
       } 
    }
   
    else if (clb[i][j].type == IO) {   
       if (tptr->pinnum < 0 || tptr->pinnum >= clb[i][j].occ) {
          printf("Error in check_segment:  IO IPIN pin number out of");
          printf(" range.\nPin number is %d\n",tptr->pinnum);
          exit(1);
       }
    }
  
    else {                       /* Not IO or CLB; must be ILLEGAL */
       printf("Error in check_segment:  IPIN in illegal position.\n");
       printf("(i,j) = (%d,%d)\n",i,j);
       exit(1); 
    }

    break;

 default:
    printf("Error in check_segment:  Unexpected segment type: %d\n",
       tptr->type);
    exit(1);
 }
}


void pathfinder_update_one_cost (int inet, int add_or_sub, float pres_fac,
      float acc_fac) {

/* This routine updates the occupancy and cost of the channel segments   *
 * and pins that are affected by the routing of net inet.  If add_or_sub *
 * is -1 the net is ripped up, if it is 1 the net is added to the        *
 * routing.  The size of pres_fac and acc_fac determine how severly      *
 * oversubscribed channels and pins are penalized.  The cost function    *
 * used is that of Pathfinder.                                           */

 struct s_trace *tptr;
 int i, j, pinnum;
 float pcost;

 tptr = trace_head[inet];

 while (tptr != NULL) {
    i = tptr->i;
    j = tptr->j;
    if (tptr->type == IPIN || tptr->type == OPIN) {

       pinnum = tptr->pinnum;
       if (clb[i][j].type == CLB) {     /* No competition at IO blocks. */
          pin_inf[i][j][pinnum].occ += add_or_sub;

/* pcost is Pn in the Pathfinder paper.  The acc_cost member stores the  *
 * total number of segments that have overused this resource in all past *
 * iterations.  I set my pcost according to the overuse that would       *
 * result from having ONE MORE net use this routing segment.             */

          pcost = 1. + pin_inf[i][j][pinnum].occ * pres_fac;
          pin_inf[i][j][pinnum].cost = (1. + pin_inf[i][j][pinnum].acc_cost *
                acc_fac) * pcost;
       }   

       if (tptr->type == IPIN) {
          tptr = tptr->next;             /* Skip next segment. */
          if (tptr == NULL)
             break;
       } 
    }

    else  {

       if (tptr->type == CHANX) {
          chan_x[i][j].occ += add_or_sub;

          if (chan_x[i][j].occ < chan_width_x[j]) {
             pcost = 1.;
          }
          else {
             pcost = 1. + (chan_x[i][j].occ + 1. - chan_width_x[j]) * 
                     pres_fac;
          }

          chan_x[i][j].cost = (1. + chan_x[i][j].acc_cost * acc_fac) *
                     pcost;
       }   

       else if (tptr->type == CHANY) {
          chan_y[i][j].occ += add_or_sub;

          if (chan_y[i][j].occ < chan_width_y[i]) {
             pcost = 1.;
          }
          else {
             pcost = 1. + (chan_y[i][j].occ + 1 - chan_width_y[i]) *
                     pres_fac;
          }
     
          chan_y[i][j].cost = (1. + chan_y[i][j].acc_cost * acc_fac) *
                     pcost;
       } 
 
    }
    tptr = tptr->next;
 
 }   /* End while loop -- did an entire traceback. */
}


void pathfinder_update_cost (float pres_fac, float acc_fac) {
/* This routine recomputes the cost of each routing resource for the   *
 * pathfinder algorithm after all nets have been routed.  It updates   *
 * the accumulated cost to by adding in the number of extra signals    *
 * sharing a resource right now (i.e. after each complete iteration.   *
 * THIS ROUTINE ASSUMES THE OCCUPANCY ARRAYS ARE UP                    *
 * TO DATE -- USE UPDATE_OCCUPANCY TO GET THE CURRENT OCCUPANCIES      *
 * BEFORE CALLING THIS ROUTINE.                                        */

 int i, j, ipin;
 float pcost;
 
 for (i=1;i<=nx;i++)
    for (j=1;j<=ny;j++)
       for (ipin=0;ipin<clb_size;ipin++) {

          if (pin_inf[i][j][ipin].occ > 1) {
             pin_inf[i][j][ipin].acc_cost += pin_inf[i][j][ipin].occ - 1.;
          }
          
          pcost = 1. + pin_inf[i][j][ipin].occ * pres_fac;
          pin_inf[i][j][ipin].cost = (1. +  pin_inf[i][j][ipin].acc_cost *
                acc_fac) * pcost;
       }
 
 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++) {
       if (chan_x[i][j].occ >= chan_width_x[j]) {
          chan_x[i][j].acc_cost += chan_x[i][j].occ - chan_width_x[j];
          pcost = 1. + (chan_x[i][j].occ + 1. - chan_width_x[j]) * pres_fac;
       }
       else {
          pcost = 1.;
       } 
       chan_x[i][j].cost = (1. + chan_x[i][j].acc_cost * acc_fac) * pcost;
    }   
 
 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) {
       if (chan_y[i][j].occ >= chan_width_y[i]) {
          chan_y[i][j].acc_cost += chan_y[i][j].occ - chan_width_y[i];
          pcost = 1. + (chan_y[i][j].occ + 1. - chan_width_y[i]) * pres_fac;
       }
       else {
          pcost = 1.;
       }
       chan_y[i][j].cost = (1. + chan_y[i][j].acc_cost * acc_fac) * pcost;
    }

}    


void update_one_cost (int inet, int add_or_sub, float pres_fac,
      int resources_to_update) {

/* This routine updates the occupancy and cost of the channel segments   *
 * and pins that are affected by the routing of net inet.  If add_or_sub *
 * is -1 the net is ripped up, if it is 1 the net is added to the        *
 * routing.  The size of pres_fac determines how severly oversubscribed  *
 * channels and pins are penalized.  With resources_to_update ==         *
 * PINS_ONLY, only the occupancies and costs of pins are updated, while  *
 * resources_to_update == ALL means the channel segments are updated as  *
 * well.                                                                 */

 struct s_trace *tptr;
 int i, j, pinnum;

 tptr = trace_head[inet];

 while (tptr != NULL) {
    i = tptr->i;
    j = tptr->j;
    if (tptr->type == IPIN || tptr->type == OPIN) { 
       pinnum = tptr->pinnum;
       if (clb[i][j].type == CLB) {
          pin_inf[i][j][pinnum].occ += add_or_sub; 
          if (pin_inf[i][j][pinnum].occ == 0) {
             pin_inf[i][j][pinnum].cost = 0.; 
          }
          else {
             pin_inf[i][j][pinnum].cost = pin_inf[i][j][pinnum].occ
                * pres_fac;
          }
       }
   
       if (tptr->type == IPIN) {
          tptr = tptr->next;             /* Skip next segment. */ 
          if (tptr == NULL) 
             break;
       }
    }  

 /* Update the channel segment costs, etc. only if this is a full *
  * immediate_update algorithm.                                   */

    else if (resources_to_update == ALL) { 
       
       if (tptr->type == CHANX) {
          chan_x[i][j].occ += add_or_sub; 
          if (chan_x[i][j].occ < chan_width_x[j]) {
             chan_x[i][j].cost = 1.;
          }
          else {
             chan_x[i][j].cost = 1. + (chan_x[i][j].occ + 1. -
                chan_width_x[j]) * pres_fac;
          }
       } 

       else if (tptr->type == CHANY) { 
          chan_y[i][j].occ += add_or_sub;
          if (chan_y[i][j].occ < chan_width_y[i]) {
             chan_y[i][j].cost = 1.;
          }   
          else {
             chan_y[i][j].cost = 1. + (chan_y[i][j].occ + 1 -
                 chan_width_y[i]) * pres_fac;
          }   
       }  

    }
    tptr = tptr->next; 

 }   /* End while loop -- did an entire traceback. */

}


void immediate_update_cost (float pres_fac) {
/* This routine initializes the cost to what is needed by the immediate *
 * update portion of the router.  This cost function is computed        *
 * somewhat differently than the block update cost.  First, there is no *
 * acc_fac (overuse history) factor in the cost.  Secondly, all costs   *
 * are computed to reflect what the cost of a segment would be if we    *
 * added one more segment to a channel or pin, since this is the        *
 * decision the router will be making.  The cost of sharing segments is *
 * set by pres_fac.  THIS ROUTINE ASSUMES THE OCCUPANCY ARRAYS ARE UP   *
 * TO DATE -- USE UPDATE_OCCUPANCY TO GET THE CURRENT OCCUPANCIES       *
 * BEFORE CALLING THIS ROUTINE.                                         */

 int i, j;
 void immediate_update_pin_cost (float pres_fac);

 immediate_update_pin_cost (pres_fac);
 
 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++) {
       if (chan_x[i][j].occ < chan_width_x[j]) {
          chan_x[i][j].cost = 1.;
       }
       else {
          chan_x[i][j].cost = 1. + (chan_x[i][j].occ + 1. -
             chan_width_x[j]) * pres_fac;
       }
    }
 
 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) {
       if (chan_y[i][j].occ < chan_width_y[i]) {
          chan_y[i][j].cost = 1.; 
       }
       else {    
          chan_y[i][j].cost = 1.  + (chan_y[i][j].occ + 1. -
             chan_width_y[i]) * pres_fac;  
       }
    }
}

void immediate_update_pin_cost (float pres_fac) {

/* This routine updates the cost of all the pins in the FPGA en masse.  *
 * It is always used by immediate_update_cost and block_update_cost     *
 * uses it when the MIXED block_update_type is selected.                */

 int i, j, ipin;
 
 for (i=1;i<=nx;i++)
    for (j=1;j<=ny;j++)
       for (ipin=0;ipin<clb_size;ipin++) {
          if (pin_inf[i][j][ipin].occ == 0) {
             pin_inf[i][j][ipin].cost = 0.;
          }
          else {
             pin_inf[i][j][ipin].cost = pin_inf[i][j][ipin].occ
                * pres_fac;
          }
       }   

}


void block_update_cost (float pres_fac, float acc_fac,
      int block_update_type) {

/* This routine finds the cost of each channel segment from scratch.   *
 * It should be called only after a lot of nets have been rerouted.    *
 * acc_fac is the factor by which the accumulated cost is multiplied   *
 * during the update, and pres_fac is the factor by which the current  *
 * overuse of a channel is multiplied.  block_update_type is           *
 * either BLOCK or MIXED.  If it is BLOCK, a true pathfinder-type      *
 * algorithm is used where all routing resources have their costs      *
 * updated only once every net is rerourted.  If it is MIXED,          *
 * the costs of pins are updated immediately after each net is routed; *
 * this option is useful because logically equivalent pins present a   *
 * pathological topology to a pathfinder-based algorithm.              *
 * NOTE:  THIS ROUTINE DOES NOT RECOMPUTE THE OCCUPANCY OF EACH        *
 * CHANNEL SEGMENT AND PIN.  YOU MUST BE SURE THAT THE OCCUPANCY       *
 * ARRAYS HAVE THE CORRECT VALUES BEFORE CALLING THIS ROUTINE (USE     *
 * UPDATE_OCCUPANCY TO UPDATE THE OCCUPANCY ARRAYS).                   */

 int i, j, ipin;
 void immediate_update_pin_cost (float pres_fac);

 if (block_update_type == BLOCK) {   /* pathfinder for pins */

    for (i=1;i<=nx;i++) {
       for (j=1;j<=ny;j++) {
          for (ipin=0;ipin<clb_size;ipin++) {
             if (pin_inf[i][j][ipin].occ <= 1) {
                pin_inf[i][j][ipin].cost = pin_inf[i][j][ipin].acc_cost *
                   acc_fac;
             }
             else {
                pin_inf[i][j][ipin].cost = pin_inf[i][j][ipin].acc_cost *
                   acc_fac + (pin_inf[i][j][ipin].occ - 1.) * pres_fac;
                pin_inf[i][j][ipin].acc_cost += pin_inf[i][j][ipin].occ
                   - 1.;
             }
          }
       }
    }
 }

 else {    /* Immediate update of costs for pins. */

    immediate_update_pin_cost (pres_fac);
 }
 
 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++) {
       if (chan_x[i][j].occ <= chan_width_x[j]) {
          chan_x[i][j].cost = 1. + chan_x[i][j].acc_cost * acc_fac;
       }
       else {
          chan_x[i][j].cost = 1. + chan_x[i][j].acc_cost * acc_fac +
             (chan_x[i][j].occ - chan_width_x[j]) * pres_fac;
          chan_x[i][j].acc_cost += (float) (chan_x[i][j].occ - 
             chan_width_x[j]);
       }
    }
 
 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) {
       if (chan_y[i][j].occ <= chan_width_y[i]) {
          chan_y[i][j].cost = 1. + chan_y[i][j].acc_cost * acc_fac; 
       }
       else {    
          chan_y[i][j].cost = 1. + chan_y[i][j].acc_cost * acc_fac + 
             (chan_y[i][j].occ - chan_width_y[i]) * pres_fac;  
          chan_y[i][j].acc_cost += (float) (chan_y[i][j].occ  -
             chan_width_y[i]); 
       }
    }

}


void update_occupancy (void) {
/* This routine updates the occ fields in the pin_inf, chan_x and chan_y *
 * arrays according to the resource usage of the current routing.        */

 int i, j, ipin, inet;
 struct s_trace *tptr;

/* First set the occupancy of everything to zero. */

 for (i=1;i<=nx;i++)
    for (j=1;j<=ny;j++)
       for (ipin=0;ipin<clb_size;ipin++)
          pin_inf[i][j][ipin].occ = 0;

 for (i=1;i<=nx;i++)
    for (j=0;j<=ny;j++)
       chan_x[i][j].occ = 0;

 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++)
       chan_y[i][j].occ = 0;
 
/* Now go through each net and count the tracks and pins used everywhere */
 
 for (inet=0;inet<num_nets;inet++) {
    
    if (is_global[inet])            /* Skip global nets. */
       continue;

    tptr = trace_head[inet];
    while (tptr != NULL) {
       if (tptr->type == IPIN) {
          if (clb[tptr->i][tptr->j].type == CLB)
             pin_inf[tptr->i][tptr->j][tptr->pinnum].occ++;
          tptr = tptr->next;                /* Skip next segment. */
          if (tptr == NULL)
             break;
       }
       else if (tptr->type == OPIN) {
          if (clb[tptr->i][tptr->j].type == CLB)
             pin_inf[tptr->i][tptr->j][tptr->pinnum].occ++;
       }
       else if (tptr->type == CHANX) {
          chan_x[tptr->i][tptr->j].occ++;
       }     
       else if (tptr->type == CHANY) {
          chan_y[tptr->i][tptr->j].occ++;
       } 
       tptr = tptr->next;
    }
 }
}


#define HUGE_NUM 1.e25

void init_route_structs (int initial_cost_type) {
/* This routine sets the occupancy of each channel and pin to zero. *
 * It sets the cost of a channel according to expected congestion   *
 * and the cost of each pin to zero.  Call this before you route    *
 * any nets.                                                        */

 int i, j, inet, ipin;
 float val, crossing;
 struct s_bb bb;
 void get_non_updateable_bb (int inet, struct s_bb *bbptr);
 void free_traceback (int inet);

/* Need to get rid of any old traceback in case we're skipping the initial *
 * route and going right to immediate update type routes.                  */

 for (i=0;i<num_nets;i++)
    free_traceback (i);

/* Modified list (of channel segments touched) is empty. */
 
 chan_modified_head = NULL;

/* reset occupancies, flags, acc_cost, etc. */

 for (i=1;i<=nx;i++) 
    for (j=1;j<=ny;j++) 
       for (ipin=0;ipin<clb_size;ipin++) {
          pin_inf[i][j][ipin].occ = 0;
          pin_inf[i][j][ipin].cost = 0.;
          pin_inf[i][j][ipin].acc_cost = 0.;
       }

 for (i=1;i<=nx;i++) 
    for (j=0;j<=ny;j++) {
       chan_x[i][j].occ = 0;
       chan_x[i][j].acc_cost = 0.;
       chan_x[i][j].flag = NONE_FLAG;
       chan_x[i][j].path_cost = HUGE_NUM;
    }

 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) {
       chan_y[i][j].occ = 0;
       chan_y[i][j].acc_cost = 0.;
       chan_y[i][j].flag = NONE_FLAG;
       chan_y[i][j].path_cost = HUGE_NUM;
    }
 
/* If we're not doing an initial routing based on expected occupancies *
 * we're done.                                                         */

 if (initial_cost_type == NONE) 
    return;

/* I'm going to deal with fractional occupancies here, so I store them  *
 * in the acc_cost element temporarily, rather than in the integer .occ *
 * element.  This makes things a bit confusing.                         */

 for (inet=0;inet<num_nets;inet++) {

    if (is_global[inet])   /* Skip global nets. */
       continue;

    get_non_updateable_bb (inet,&bb);

/* Get the expected "crossing count" of a net, based on its number *
 * of pins.  Extrapolate for very large nets.                      */

    if (net[inet].num_pins > 50) {
       crossing = 2.7933 + 0.02616 * (net[inet].num_pins - 50);
    }
    else {
       crossing = cross_count[net[inet].num_pins-1];
    }

/* Load the x-directed .acc_cost elements with their expected occupancies. */

    val = crossing / (bb.ymax - bb.ymin + 2.);

    for (i=bb.xmin;i<=bb.xmax;i++)
       for (j=bb.ymin-1;j<=bb.ymax;j++)
          chan_x[i][j].acc_cost += val;

/* Load the y-directed .acc_cost elements with their expected occupancies. */

    val = crossing / (bb.xmax - bb.xmin + 2.);
    for (i=bb.xmin-1;i<=bb.xmax;i++) 
       for (j=bb.ymin;j<=bb.ymax;j++) 
          chan_y[i][j].acc_cost += val;
 }   

/* Set the costs according to the probable occupancies in .acc_cost */

 for (i=1;i<=nx;i++) 
    for (j=0;j<=ny;j++) {
       if (chan_x[i][j].acc_cost < chan_width_x[j]) {
          chan_x[i][j].cost = 1.;               
       }

/* Penalize for overuse.  Two possible func. forms. */

       else { 
          if (initial_cost_type == DIV) 
             chan_x[i][j].cost = 1. + chan_x[i][j].acc_cost / chan_width_x[j];
          else
             chan_x[i][j].cost = 1. + chan_x[i][j].acc_cost - chan_width_x[j];
       }
    }
 
 for (i=0;i<=nx;i++) 
    for (j=1;j<=ny;j++) {
       if (chan_y[i][j].acc_cost < chan_width_y[i]) {
          chan_y[i][j].cost = 1.;
       }
       else {
          if (initial_cost_type == DIV)
             chan_y[i][j].cost = 1. + chan_y[i][j].acc_cost / chan_width_y[i];
          else
             chan_y[i][j].cost = 1. + chan_y[i][j].acc_cost - chan_width_y[i];
       }
    }

/* Now I've got to set the .acc_cost element back to zero, since I used *
 * them for temporary storage.                                          */

 for (i=1;i<=nx;i++) 
    for (j=0;j<=ny;j++) 
       chan_x[i][j].acc_cost = 0.;
 
 for (i=0;i<=nx;i++)
    for (j=1;j<=ny;j++) 
       chan_y[i][j].acc_cost = 0.;
}

void route_net (int inet, float bend_cost) {
/* Uses a maze routing (Dijkstra's) algorithm to route a net.  The *
 * net begins at the net output, and expands outward until it hits *
 * a target pin.  The algorithm is then restarted with the entire  *
 * first wire segment included as part of the source this time.    *
 * For an n-pin net, the maze router is invoked n-1 times to       *
 * complete all the connections.  Inet is the index of the net to  *
 * be routed, and bend_cost is the cost of a bend in the route.    *
 * Bends are penalized since they make detailed routing more       *
 * difficult with segmented routing architectures.                 */

 int i;
 float pcost;
 struct s_heap *current;

 void free_traceback (int inet);
 void mark_ends (int inet);
 void expand_neighbours (struct s_heap *hptr, int inet, float bend_cost);
 void update_markers (struct s_heap *hptr, int inet);
 void update_traceback (struct s_heap *hptr, int inet);
 void empty_heap (void);
 void free_heap_data (struct s_heap *hptr);
 void restart_path (int inet);
 struct s_heap *get_heap_head (void); 
 void add_to_mod_list (float *fptr);
 
 pin_done[0] = 1;                  /* Output is starting point */
 for (i=1;i<net[inet].num_pins;i++) 
    pin_done[i] = 0;               /* No other pins connected yet */

 free_traceback (inet);
 mark_ends (inet);
 
 for (i=1;i<net[inet].num_pins;i++) { /* Need n-1 wires to connect n pins */
    restart_path (inet);
    current = get_heap_head();
    while (current->type != IPIN) {
       switch (current->type) {

       case CHANX:
          pcost = chan_x[current->i][current->j].path_cost;
          if (pcost > current->cost) {
             chan_x[current->i][current->j].path_cost = current->cost;
             chan_x[current->i][current->j].prev_type = 
                current->prev_type; 
             chan_x[current->i][current->j].prev_i = current->prev_i;
             chan_x[current->i][current->j].prev_j = current->prev_j;
             add_to_mod_list (&chan_x[current->i][current->j].path_cost);
             expand_neighbours(current, inet, bend_cost);  
          }
          break;

       case CHANY:
          pcost = chan_y[current->i][current->j].path_cost;
          if (pcost > current->cost) { 
             chan_y[current->i][current->j].path_cost = current->cost;
             chan_y[current->i][current->j].prev_type =
                current->prev_type;
             chan_y[current->i][current->j].prev_i = current->prev_i;
             chan_y[current->i][current->j].prev_j = current->prev_j;
             add_to_mod_list (&chan_y[current->i][current->j].path_cost);
             expand_neighbours(current, inet, bend_cost);  
          }
          break;

       case OPIN:
          expand_neighbours(current, inet, bend_cost);  
          break;

       default:
             printf("Error:  Unexpected heap type in route_net.\n");
             printf("Type was %d.\n", current->type);
             exit(1);
       }

       free_heap_data (current);
       current = get_heap_head();
    }
    update_markers (current, inet); 
    empty_heap ();
    update_traceback (current, inet);
    free_heap_data (current);
 }

#ifdef DEBUG
   for (i=0;i<net[inet].num_pins;i++) {
      if (pin_done[i] != 1) {
         printf("Error in route_net.  Pin %d of net %d not marked as done.\n",
            i, inet);
         exit(1);
      }
   }
#endif
}


void update_markers (struct s_heap *hptr, int inet) {
/* Updates the flags on the chan_x and chan_y arrays to indicate       *
 * channels which no longer have target pins on their sides.           *
 * Extra care has to be taken to allow multiple pins on a clb or       *
 * multiple io blocks in the same [i][j] location to be on the same    *
 * net (although few, if any, netlists should have these features).    *
 * hptr is a pointer to the IPIN heap element on which the search      *
 * ended, while inet is the net being routed.  This routine also       *
 * updates the pin_done array to show that a new pin is connected.     */

 int i, j, bnum, ipin, ifound, class;
 
 i = hptr->i;
 j = hptr->j;
 
 ifound = 0; 
 for (ipin=0;ipin<net[inet].num_pins;ipin++) {
    if (pin_done[ipin] == 0) {
       bnum = net[inet].pins[ipin]; 
       if (block[bnum].x == i && block[bnum].y == j) { 
          ifound++;
          if (clb[i][j].type == CLB) {
             class = net_pin_class[inet][ipin];

/* Note: netlist is checked when it is read that it does not have any *
 * nets that connect more than once to the same pin class on 1 clb.   *
 * Hence code below is safe.                                          */

             if (class == clb_pin_class[hptr->u.pinnum]) {
                pin_done[ipin] = 1;
             }
          }
         
          else {       /* IO block */
             if (clb[i][j].u.io_blocks[hptr->u.pinnum] == bnum) 
                pin_done[ipin] = 1;
          }
       }
    }
 }

 if (ifound > 1) return;   /* Still more pins to connect at that block */

#ifdef DEBUG
   if (ifound <= 0) {
       printf("Error in update markers: Unexpected ifound of %d.\n",ifound);
       printf("Occurred during expansion of net #%d.\n",inet);
       exit(1);
   }
#endif

 
 if (clb[i][j].type == CLB) {
    chan_x[i][j].flag ^= BFLAG;   /* Clear the BFLAG bit by complementation*/
    chan_x[i][j-1].flag ^= TFLAG;
    chan_y[i][j].flag ^= LFLAG;
    chan_y[i-1][j].flag ^= RFLAG;
 }
 else {    /* IO block (should really be only OUTPADs) */
    if (j == 0) {
       chan_x[i][j].flag ^= BFLAG;
    }
    else if (j == ny+1) {
       chan_x[i][j-1].flag ^= TFLAG;
    }
    else if (i == 0) {
       chan_y[i][j].flag ^= LFLAG;
    }
    else if (i == nx+1) {
       chan_y[i-1][j].flag ^= RFLAG;
    }
    else {
       printf("Illegal IO location in update_markers. (x,y) = (%d,%d).\n",
           i, j);
       exit(1);
    } 
 }
}

void update_traceback (struct s_heap *hptr, int inet) {
/* This routine adds the most recently finished wire segment to  *
 * the traceback linked list.  The first connection starts with  *
 * an OPIN, and begins at the structure pointed to by            *
 * trace_head[inet]. Each connection ends with an IPIN.  After   *
 * the first IPIN, the second connection begins (if the net has  *
 * more than 2 pins).  The first element after the IPIN gives    *
 * a channel segment which is part of a previously made          *
 * connection or the net's OPIN.  This element connects the new  *
 * path into the existing routing.  In each traceback I start at *
 * the end of a path and trace back through its predecessors to  *
 * the beginning.                                                *
 * I have stored information on the predecesser of each node to  *
 * make traceback easy -- this sacrificies some memory for       *
 * easier code maintenance.                                      */ 

 int bnum, io_pad;
 struct s_trace *tptr, *prevptr, *temptail;
 struct s_trace *alloc_trace_data (void); 
 void free_trace_data (struct s_trace *tptr);
 void set_opin_number (struct s_trace *tptr, int inet);

#ifdef DEBUG
   if (hptr->type != IPIN) {
      printf("Error in update_traceback.  Expected type = IPIN (%d).\n",
         IPIN);
      printf("Got type = %d when tracing net %d.\n",hptr->type, inet);
      exit(1);
   }
#endif
 
 tptr = alloc_trace_data ();   /* IPIN on the end of the connection */ 
 tptr->type = hptr->type;
 tptr->i = hptr->i;
 tptr->j = hptr->j;
 tptr->pinnum = hptr->u.pinnum;
 tptr->next = NULL;
 temptail = tptr;              /* This will become the new tail at the end */
                               /* of the routine.                          */

 prevptr = alloc_trace_data();      /* Now do it's predecessor */
 prevptr->type = hptr->prev_type;
 prevptr->i = hptr->prev_i;
 prevptr->j = hptr->prev_j;
 prevptr->next = tptr;

 while (prevptr->type != LASTONE) {
    tptr = prevptr;
    prevptr = alloc_trace_data();

    switch (tptr->type) {
    case CHANX:
       prevptr->type = chan_x[tptr->i][tptr->j].prev_type;
       prevptr->i = chan_x[tptr->i][tptr->j].prev_i;
       prevptr->j = chan_x[tptr->i][tptr->j].prev_j;
       break;

    case CHANY:  
       prevptr->type = chan_y[tptr->i][tptr->j].prev_type;
       prevptr->i = chan_y[tptr->i][tptr->j].prev_i;
       prevptr->j = chan_y[tptr->i][tptr->j].prev_j;
       break;

    case OPIN:
       prevptr->type = LASTONE;

/* For IO blocks I store the subblock number in the pinnum member of *
 * the trace structure.                                              */

       if (clb[tptr->i][tptr->j].type == IO) {
          bnum = net[inet].pins[0];
          for (io_pad=0;io_pad<clb[tptr->i][tptr->j].occ;io_pad++) {
             if (clb[tptr->i][tptr->j].u.io_blocks[io_pad] == bnum) 
                break;
          }
          tptr->pinnum = io_pad;  /* Pad number in clb (i,j)  */
       }
       else {
          set_opin_number (tptr, inet);
       }
       break;
 
    default:
       printf("Error in update_traceback:  illegal element type %d.\n",
          tptr->type);
       printf("Net %d was being traced back.\n",inet);
       exit(1);
    }

    prevptr->next = tptr;
 }

 if (trace_tail[inet] != NULL) {
    trace_tail[inet]->next = tptr;     /* Traceback ends with tptr */
 }
 else {
    trace_head[inet] = tptr;
 }

 trace_tail[inet] = temptail;
 free_trace_data (prevptr);
}


void set_opin_number (struct s_trace *tptr, int inet) {
/* This routine sets the pinnum member of the tptr structure for the *
 * OPIN driving net inet.  The routine is only called when the OPIN  *
 * is on a clb.  Because of the general logical equivalence          *
 * permitted in the architecture file, I have to figure out which    *
 * of the logically equivalent pins on the clb driving this net      *
 * provided the output.                                              */

 int class, i, ipin, best_pin, adjacent;
 float min_cost;
 int check_adjacent (struct s_trace *tptr, struct s_trace *prev_ptr);

 class = net_pin_class[inet][0];     /* class of the driver for this net */

/* Usually there will be no logical equivalence for output pins.  *
 * Use special case code for this most common case for speed.     */

 if (class_inf[class].num_pins == 1) {
    tptr->pinnum = class_inf[class].pinlist[0];
    return;
 }

/* General (logically equivalent output pins) code below.       */

 min_cost = 1.e30;
 best_pin = -1000; 

 for (i=0;i<class_inf[class].num_pins;i++) {
    ipin = class_inf[class].pinlist[i];
    tptr->pinnum = ipin;
    adjacent = check_adjacent (tptr->next, tptr);
    if (adjacent == 1) {
       if (pin_inf[tptr->i][tptr->j][ipin].cost < min_cost) {
          best_pin = ipin;
          min_cost = pin_inf[tptr->i][tptr->j][ipin].cost;
       }
    }
 }
 
 tptr->pinnum = best_pin;

#ifdef DEBUG
   if (best_pin < 0) {
      printf("Error in set_opin_number on net %d.\n",inet);
      printf("Didn't find a pin connecting opin to channel.\n");
      printf("Best pin = %d.\n",best_pin);
      exit (1);
   }
#endif
    
}


void restart_path (int inet) {
/* This routine puts the source points on the heap.  It puts all     *
 * channel segments in the traceback for this net on the heap with a *
 * cost of zero, since they are already connected to this net.  The  *
 * output pin of this net is also put on the heap with a cost of 0.  *
 * The routine initializes the path_cost to HUGE_NUM for all channel *
 * segments.  Note that signals can not pass through electrically    *
 * equivalent input pin locations since buffers isolate the input    *
 * pins from the tracks.  Hence, I do not put channel segments       *
 * adjacent to the ipins on the heap here.                           */ 

 struct s_trace *tptr; 
 struct s_heap *hptr; 
 int i, bnum, class, pinnum, num_mod_ptrs;
 struct s_linked_f_pointer *mod_ptr;

 void chan_to_heap (enum htypes type, int i, int j, float cost, 
    enum htypes prev_type, int prev_i, int prev_j); 
 struct s_heap *alloc_heap_data (void);
 void add_to_heap (struct s_heap *hptr);
 
/* First reset the pathcost of all channel segments that were reached *
 * in the last routing step.                                          */

 if (chan_modified_head != NULL) {
    mod_ptr = chan_modified_head;

#ifdef DEBUG 
    num_mod_ptrs = 1;
#endif

    while (mod_ptr->next != NULL) {
       *(mod_ptr->fptr) = HUGE_NUM;
       mod_ptr = mod_ptr->next;
#ifdef DEBUG 
       num_mod_ptrs++;
#endif
    }
    *(mod_ptr->fptr) = HUGE_NUM;   /* Do last one. */
    
/* Reset the modified list and put all the elements back in the free   *
 * list.                                                               */

    mod_ptr->next = linked_f_pointer_free_head;
    linked_f_pointer_free_head = chan_modified_head; 
    chan_modified_head = NULL;

#ifdef DEBUG 
    num_linked_f_pointer_allocated -= num_mod_ptrs;
#endif
 }
 

/* Now add segments in the traceback to the net. */
 
 tptr = trace_head[inet];

 while (tptr != NULL) {         
    if (tptr->type == CHANX || tptr->type == CHANY) 
       chan_to_heap (tptr->type, tptr->i, tptr->j, 0., LASTONE,
          -1, -1);
    tptr = tptr->next;
 }

/* Add all pins of the proper class (the net output) to source */

 bnum = net[inet].pins[0];

 if (block[bnum].type == CLB) {
    class = net_pin_class[inet][0];

    for (i=0;i<class_inf[class].num_pins;i++) {
       pinnum = class_inf[class].pinlist[i];

       hptr = alloc_heap_data();
       hptr->type = OPIN;
       hptr->i = block[bnum].x;
       hptr->j = block[bnum].y;
       hptr->cost = pin_inf[hptr->i][hptr->j][pinnum].cost;
       hptr->u.pinnum = pinnum;
       hptr->prev_type = LASTONE;
       hptr->prev_i = -1;
       hptr->prev_j = -1;
       add_to_heap (hptr);
    }
 }

 else {     /* IO block. */
    hptr = alloc_heap_data();
    hptr->type = OPIN;
    hptr->i = block[bnum].x;
    hptr->j = block[bnum].y;
    hptr->cost = 0.;
    hptr->u.pinnum = -1;   /* Should never be used -- dummy value. */
    hptr->prev_type = LASTONE;
    hptr->prev_i = -1;
    hptr->prev_j = -1;
    add_to_heap (hptr);
 }
}


void expand_neighbours (struct s_heap *current_ptr, int inet, 
     float bend_cost) {
/* Puts all the channels and target pins adjacent to current_ptr on *
 * the heap.  Channel segments outside of the expanded bounding box *
 * specified in bb_coords are not added to the heap.  There is no   *
 * need to check if IPINs are in the bounding box -- by definition  *
 * they always are, since the smallest allowable net bounding box   *
 * includes all blocks connected to the net and the channels        *
 * adjacent to them.                                                */

 int i, j, ipin;
 float cost;
 void chan_to_heap (enum htypes type, int i, int j, float cost, 
    enum htypes prev_type, int prev_i, int prev_j); 
 void ipin_to_heap (int i, int j, float cost, int ipin, enum htypes
    prev_type, int prev_i, int prev_j); 
 void expand_output (struct s_heap *hptr);
 int which_io_block (int i, int j, int inet);
 void add_pins (struct s_heap *from_ptr, int i, int j, int side, int inet);

 i = current_ptr->i;
 j = current_ptr->j;

 switch (current_ptr->type) {

 case OPIN:
    expand_output (current_ptr);
    break;

 case CHANX:  
    if (i > bb_coords[inet].xmin) {   /* x-channel to the left */
       cost = current_ptr->cost + chan_x[i-1][j].cost;
       chan_to_heap (CHANX,i-1,j,cost,current_ptr->type,i,j);
    }

    if (i < bb_coords[inet].xmax) {   /* x-channel to the right */
       cost = current_ptr->cost + chan_x[i+1][j].cost;
       chan_to_heap (CHANX,i+1,j,cost,current_ptr->type,i,j);
    }
   
    if (j >= bb_coords[inet].ymin) {    /* two y-channels below */
       cost = current_ptr->cost + chan_y[i][j].cost + bend_cost;
       chan_to_heap (CHANY,i,j,cost,current_ptr->type,i,j);
       
       cost = current_ptr->cost + chan_y[i-1][j].cost + bend_cost;
       chan_to_heap (CHANY,i-1,j,cost,current_ptr->type,i,j);
    }

    if (j < bb_coords[inet].ymax) {   /* two y-channels above */
       cost = current_ptr->cost + chan_y[i][j+1].cost + bend_cost;
       chan_to_heap (CHANY,i,j+1,cost,current_ptr->type,i,j);

       cost = current_ptr->cost + chan_y[i-1][j+1].cost + bend_cost;
       chan_to_heap (CHANY,i-1,j+1,cost,current_ptr->type,i,j);
    }

  /* Is the clb above a target clb with pin(s) on the bottom? */
    if (chan_x[i][j].flag & TFLAG) {
       if (clb[i][j+1].type == CLB) {
          add_pins (current_ptr, i, j+1, BOTTOM, inet);
       }

       else {   /* IO blocks have 1 pin, so no competition for pins */

/* I use the pinnum of IO blocks to tell which subblock is connected. */

          ipin = which_io_block (i, j+1, inet);
          cost = current_ptr->cost; 
          ipin_to_heap (i,j+1,cost,ipin,current_ptr->type,i,j);
       }
    }

  /* Is the clb below a target clb with pin(s) on the top? */
    if (chan_x[i][j].flag & BFLAG) {
       if (clb[i][j].type == CLB) {
          add_pins (current_ptr, i, j, TOP, inet);
       }

       else {   /* IO blocks have 1 pin, so no competition for pins */

/* I use the pinnum of IO blocks to tell which subblock is connected. */

          ipin = which_io_block (i, j, inet);
          cost = current_ptr->cost; 
          ipin_to_heap (i,j,cost,ipin,current_ptr->type,i,j);
       }
    }

    break;

 case CHANY:
    if (j > bb_coords[inet].ymin) {    /* y-channel below */
       cost = current_ptr->cost + chan_y[i][j-1].cost;
       chan_to_heap (CHANY,i,j-1,cost,current_ptr->type,i,j);
    }

    if (j < bb_coords[inet].ymax) {   /* y-channel above */ 
       cost = current_ptr->cost + chan_y[i][j+1].cost;
       chan_to_heap (CHANY,i,j+1,cost,current_ptr->type,i,j);
    }
   
    if (i >= bb_coords[inet].xmin) {    /* Two x-channels to the left */
       cost = current_ptr->cost + chan_x[i][j-1].cost + bend_cost;
       chan_to_heap (CHANX,i,j-1,cost,current_ptr->type,i,j);
    
       cost = current_ptr->cost + chan_x[i][j].cost + bend_cost;
       chan_to_heap (CHANX,i,j,cost,current_ptr->type,i,j);
    }

    if (i < bb_coords[inet].xmax) {   /* Two x-channels to the right */
       cost = current_ptr->cost + chan_x[i+1][j-1].cost + bend_cost; 
       chan_to_heap (CHANX,i+1,j-1,cost,current_ptr->type,i,j);

       cost = current_ptr->cost + chan_x[i+1][j].cost + bend_cost;
       chan_to_heap (CHANX,i+1,j,cost,current_ptr->type,i,j);
    }

    if (chan_y[i][j].flag & LFLAG) {
       if (clb[i][j].type == CLB) {
          add_pins (current_ptr, i, j, RIGHT, inet);
       }

       else {   /* IO blocks have 1 pin, so no competition for pins */

/* I use the pinnum of IO blocks to tell which subblock is connected. */

          ipin = which_io_block (i, j, inet);
          cost = current_ptr->cost; 
          ipin_to_heap (i,j,cost,ipin,current_ptr->type,i,j);
       }
    }

    if (chan_y[i][j].flag & RFLAG) {
       if (clb[i+1][j].type == CLB) {
          add_pins (current_ptr, i+1, j, LEFT, inet);
       }

       else {   /* IO blocks have 1 pin, so no competition for pins */

/* I use the pinnum of IO blocks to tell which subblock is connected. */

          ipin = which_io_block (i+1, j, inet);
          cost = current_ptr->cost; 
          ipin_to_heap (i+1,j,cost,ipin,current_ptr->type,i,j);
       }
    }

    break;

 default:  
    printf("Error:  Unexpected path type in expand_neighbours.\n");
    printf("Illegal type was %d.\n",current_ptr->type);
    exit(1);
 }
}


void add_pins (struct s_heap *from_ptr, int i, int j, int side, int inet) {
/* This routine adds all the potential target pins at slot (i,j), which *
 * must contain a CLB, to the heap.  from_ptr gives the channel segment *
 * from which we're coming, (i,j) gives the CLB location, side is the   *
 * side of the CLB on which the pins must be and inet is the net being  *
 * routed.                                                              */

 int netpin, clbpin, class, k;
 float cost;
 void ipin_to_heap (int i, int j, float cost, int ipin, enum htypes
    prev_type, int prev_i, int prev_j); 

/* This routine never adds the output, so skip pin 0.         */

 for (netpin=1;netpin<net[inet].num_pins;netpin++) {
    if (clb[i][j].u.block == net[inet].pins[netpin]) {
       if (pin_done[netpin] == 0) {
          class = net_pin_class[inet][netpin];
          
          for (k=0;k<class_inf[class].num_pins;k++) {
             clbpin = class_inf[class].pinlist[k];
             if (pinloc[side][clbpin] == 1) {
                cost = from_ptr->cost + pin_inf[i][j][clbpin].cost;
                ipin_to_heap (i, j, cost, clbpin, from_ptr->type, 
                    from_ptr->i, from_ptr->j);
             }
          }
       }
    }
 }

}


int which_io_block (int i, int j, int inet) {

/* This subroutine returns the sublock number (from 0 to io_rat - 1)  *
 * to which we are connecting at location (i,j).  This allows the     *
 * storage of which of several IO pads at this location was connected *
 * to.  The routine is written in a somewhat counter-intuitive way    *
 * for speed.  The more intuitive way would be to check every block   *
 * at location (i,j) to see if it was connected to this net.  That's  *
 * probably slower for high-fanout nets.                              */

 int count, io_pad, bnum;

 for (count=0;count<net[inet].num_pins;count++) {
    if (pin_done[count] == 0) {         /* Not already connected */
       bnum = net[inet].pins[count];

/* First if below is not necessary, and I'm not sure of whether it's faster *
 * or not.  Maybe remove it and see.                                        */

       if (block[bnum].x == i && block[bnum].y == j) {
          for (io_pad=0;io_pad<clb[i][j].occ;io_pad++) {
             if (bnum == clb[i][j].u.io_blocks[io_pad]) 
                return(io_pad);
          }
       printf("Error in which_io_block:  Can't find block at (%d,%d)",i,j);
       printf("to connect to net %d.\n",inet);
       exit(1);
       }
    }
 }

 printf("Error in which_io_block:  Can't find any block belonging to\n");
 printf("net %d at (%d,%d).\n", inet, i, j);
 exit (1);
}


void mark_ends (int inet) {
/* Load the flag element of the chan_x and chan_y arrays so I can   *
 * rapidly determine if this channel segment borders a target clb.  *
 * Some code here is to handle the case where two or more pins of   *
 * a clb are on the same net, or two io blocks in the same clb      *
 * slot are on the same net.                                        */

 int ipin, i, j, bnum;

 for (ipin=1;ipin<net[inet].num_pins;ipin++) {
    bnum = net[inet].pins[ipin];
    i = block[bnum].x; 
    j = block[bnum].y;

    if (clb[i][j].type == CLB) {   
       chan_x[i][j].flag |= BFLAG;
       chan_x[i][j-1].flag |= TFLAG;
       chan_y[i][j].flag |= LFLAG;
       chan_y[i-1][j].flag |= RFLAG;
    }
    else {    /* IO block (should really be only OUTPADs) */
       if (j == 0) {
          chan_x[i][j].flag |= BFLAG;
       }
       else if (j == ny+1) {
          chan_x[i][j-1].flag |= TFLAG;
       }
       else if (i == 0) {
          chan_y[i][j].flag |= LFLAG;
       }
       else if (i == nx+1) {
          chan_y[i-1][j].flag |= RFLAG;
       }    
       else {
          printf("Illegal IO location in mark ends. (x,y) = (%d,%d).\n",
              i, j); 
          exit(1);
       }      /* End of edge selection */
    }     /* End of CLB or IO if */
 }     /* End of while loop */
}


void ipin_to_heap (int i, int j, float cost, int ipin, enum htypes
   prev_type, int prev_i, int prev_j) {
/* Puts an input pin on the heap.  I don't store or check it       *
 * against a path_cost, since I stop the maze router as soon as an *
 * ipin is pulled off the queue, and thus path_cost would never be *
 * updated.  Prev_type etc. are stored to make traceback easier.   */
 struct s_heap *hptr; 
 struct s_heap *alloc_heap_data (void); 
 void add_to_heap (struct s_heap *hptr); 

 hptr = alloc_heap_data();
 hptr->i = i;
 hptr->j = j;
 hptr->cost = cost;
 hptr->type = IPIN;
 hptr->u.pinnum = ipin;
 hptr->prev_type = prev_type;
 hptr->prev_i = prev_i;
 hptr->prev_j = prev_j;
 add_to_heap(hptr);
}

void chan_to_heap (enum htypes type, int i, int j, float cost, 
   enum htypes prev_type, int prev_i, int prev_j) {
/* Puts a channel type element on the heap, if the new cost given  *
 * is lower than the current path_cost to this channel segment.    *
 * The type and location of its predecessor is stored to make      *
 * traceback easy.                                                 */

 float path_cost;
 struct s_heap *hptr;
 struct s_heap *alloc_heap_data (void); 
 void add_to_heap (struct s_heap *hptr); 

 if (type == CHANX) {
    path_cost = chan_x[i][j].path_cost;
 }
 else {
    path_cost = chan_y[i][j].path_cost;
 }
 if (cost >= path_cost) return;

 hptr = alloc_heap_data();
 hptr->i = i; 
 hptr->j = j;
 hptr->cost = cost;
 hptr->type = type; 
 hptr->prev_type = prev_type;
 hptr->prev_i = prev_i;
 hptr->prev_j = prev_j;
 add_to_heap (hptr);
}

void free_traceback (int inet) {
 struct s_trace *tptr, *tempptr;
 void free_trace_data (struct s_trace *tptr);

 tptr = trace_head[inet];

 while (tptr != NULL) {
    tempptr = tptr->next; 
    free_trace_data (tptr);
    tptr = tempptr;
 }
 
 trace_head[inet] = NULL; 
 trace_tail[inet] = NULL;
}

void expand_output (struct s_heap *hptr) {
/* Put all channels with pins from the output on the heap with the   *
 * appropriate cost.  Note:  there is no need to make bounding box   *
 * checks here, since the smallest allowable net bounding box always *
 * includes all channels adjacent to the OPIN (and all IPINs) of the *
 * net.                                                              */

 int iout, jout, pinout, costout;

 iout = hptr->i;
 jout = hptr->j;
 pinout = hptr->u.pinnum;
 costout = hptr->cost;


 if (clb[iout][jout].type == CLB) {
    if (pinloc[TOP][pinout] == 1)     /* Put channel above on heap */
       chan_to_heap (CHANX, iout, jout, costout + chan_x[iout][jout].cost,
          OPIN, iout, jout);

    if (pinloc[BOTTOM][pinout] == 1)  /* Put channel on bottom on heap */
       chan_to_heap (CHANX, iout, jout-1, costout + chan_x[iout][jout-1].cost,
          OPIN, iout, jout);

    if (pinloc[RIGHT][pinout] == 1)   /* Put channel on right on heap */
       chan_to_heap (CHANY, iout, jout, costout + chan_y[iout][jout].cost,
          OPIN, iout, jout);

    if (pinloc[LEFT][pinout] == 1)    /* Put channel on right on heap */
       chan_to_heap (CHANY, iout-1, jout, costout + chan_y[iout-1][jout].cost,
          OPIN, iout, jout);
 }

 else {  /* Inpad block (Outpads have only an input) */
    if (jout == 0)               /* Put chan_x above on heap */
       chan_to_heap (CHANX, iout, jout, costout + chan_x[iout][jout].cost,
          OPIN, iout, jout);

    else if (jout == ny+1)            /* Put chan_x below on heap */
       chan_to_heap (CHANX, iout, jout-1, costout + chan_x[iout][jout-1].cost,
          OPIN, iout, jout);

    else if (iout == 0)               /* Put chan_y to right on heap */
       chan_to_heap (CHANY, iout, jout, costout + chan_y[iout][jout].cost,
          OPIN, iout, jout);

    else if (iout == nx+1)            /* Put chan_y to left on heap */
       chan_to_heap (CHANY, iout-1, jout, costout + chan_y[iout-1][jout].cost,
          OPIN, iout, jout);

    else {
       printf("Error in expand_output.  IO block in unexpected location.\n");
       printf("Block is at (%d,%d)\n",iout,jout);
       exit(1);
    }
 }
}


struct s_trace **alloc_route_structs (void) {
 int max_pins;   /* Maximum number of pins on any net. */
 int inet;
 struct s_trace **best_routing;

 best_routing = (struct s_trace **) my_calloc (num_nets, 
     sizeof (struct s_trace *));

 chan_x = (struct s_phys_chan **) alloc_matrix (1,nx,0,ny,
    sizeof(struct s_phys_chan));
 chan_y = (struct s_phys_chan **) alloc_matrix (0,nx,1,ny,
    sizeof(struct s_phys_chan));    
 pin_inf = (struct s_pin ***) alloc_matrix3 (1,nx,1,ny,0,clb_size-1,
    sizeof(struct s_pin));
 trace_head = (struct s_trace **) my_calloc (num_nets,  
    sizeof(struct s_trace *));
 trace_tail = (struct s_trace **) my_malloc (num_nets * 
    sizeof(struct s_trace *));

 heap_size = nx*ny;
 heap = (struct s_heap **) my_malloc (heap_size * 
    sizeof (struct s_heap *));
 heap--;   /* heap stores from [1..heap_size] */
 heap_tail = 1;
 
 max_pins = 0;
 for (inet=0;inet<num_nets;inet++) 
    max_pins = max(max_pins,net[inet].num_pins);
 pin_done = (int *) my_malloc (max_pins * sizeof(int));

 return (best_routing);
}


void free_route_structs (struct s_trace **best_routing) {
/* Frees the temporary storage needed only during the routing.  The  *
 * final routing result is not freed.                                */

 free_matrix ((void **) chan_x,1,nx,0,sizeof(struct s_phys_chan));
 free_matrix ((void **) chan_y,0,nx,1,sizeof(struct s_phys_chan));
 free_matrix3 ((void ***) pin_inf,1,nx,1,ny,0,
    sizeof(struct s_pin));

 free ((void *) (heap + 1));
 free ((void *) best_routing);
}


void load_route_bb (int bb_factor) {
/* This routine loads the bounding box arrays used to limit the space *
 * searched by the maze router when routing each net.  The search is  *
 * limited to channels contained with the net bounding box expanded   *
 * by bb_factor channels on each side.  For example, if bb_factor is  *
 * 0, the maze router must route each net within its bounding box.    *
 * If bb_factor = nx, the maze router will search every channel in    *
 * the FPGA if necessary.  Before this routine is called, the correct *
 * (unexpanded) bounding box of each net must be contained in the     *
 * bb_coords array.                                                   */

 int inet;

 for (inet=0;inet<num_nets;inet++) {

/* Expand the net bounding box by bb_factor, then clip to the physical *
 * chip area.                                                          */

    bb_coords[inet].xmin = max (bb_coords[inet].xmin - bb_factor, 1);
    bb_coords[inet].xmax = min (bb_coords[inet].xmax + bb_factor, nx);
    bb_coords[inet].ymin = max (bb_coords[inet].ymin - bb_factor, 1);
    bb_coords[inet].ymax = min (bb_coords[inet].ymax + bb_factor, ny);
 }
}


void add_to_mod_list (float *fptr) {

/* This routine adds the floating point pointer (fptr) into a  *
 * linked list that indicates all the pathcosts that have been *
 * modified thus far.                                          */

 struct s_linked_f_pointer *mod_ptr;
 struct s_linked_f_pointer *alloc_linked_f_pointer (void);

 mod_ptr = alloc_linked_f_pointer ();

/* Add this element to the start of the modified list. */

 mod_ptr->next = chan_modified_head;
 mod_ptr->fptr = fptr;
 chan_modified_head = mod_ptr;
}


void add_to_heap (struct s_heap *hptr) {
 int ito, ifrom;
 struct s_heap *temp_ptr;
   
 if (heap_tail > heap_size ) {          /* Heap is full */
    heap_size *= 2;
    heap = my_realloc ((void *)(heap + 1), heap_size * 
       sizeof (struct s_heap *));
    heap--;     /* heap goes from [1..heap_size] */
 }

 heap[heap_tail] = hptr;
 ifrom = heap_tail;
 ito = ifrom/2;
 heap_tail++;

 while ((ito >= 1) && (heap[ifrom]->cost < heap[ito]->cost)) {
    temp_ptr = heap[ito];
    heap[ito] = heap[ifrom];
    heap[ifrom] = temp_ptr;
    ifrom = ito;
    ito = ifrom/2;
 } 
}


struct s_heap *get_heap_head (void) {
/* Returns the smallest element on the heap. */
 int ito, ifrom;
 struct s_heap *heap_head, *temp_ptr;

#ifdef DEBUG
 if (heap_tail == 1) {    /* Empty heap. */
    printf("Error:  Empty heap occurred in get_heap_head.\n");
    exit(1);
 }
#endif

 heap_head = heap[1];                /* Smallest element. */
 
 /* Now fix up the heap */

 heap_tail--;
 heap[1] = heap[heap_tail];
 ifrom = 1;
 ito = 2*ifrom;
   
 while (ito < heap_tail) {
    if (heap[ito+1]->cost < heap[ito]->cost)
       ito++;
    if (heap[ito]->cost > heap[ifrom]->cost)
       break;
    temp_ptr = heap[ito];
    heap[ito] = heap[ifrom];
    heap[ifrom] = temp_ptr;
    ifrom = ito;
    ito = 2*ifrom;
 } 

 return(heap_head);
}


void empty_heap (void) {
 void free_heap_data (struct s_heap *hptr); 

 while (heap_tail > 1) {
    heap_tail--;
    free_heap_data(heap[heap_tail]);
 }
}

#define NCHUNK 100  /* # of various structs malloced at a time. */


struct s_heap *alloc_heap_data (void) {
 int i;
 struct s_heap *temp_ptr;

 if (heap_free_head == NULL) {   /* No elements on the free list */
    heap_free_head = (struct s_heap *) my_malloc (NCHUNK *
       sizeof(struct s_heap));

/* If I want to free this memory, I have to store the original pointer *
 * somewhere.  Not worthwhile right now -- if you need more memory     *
 * for post-routing stages, look into it.                              */

    for (i=0;i<NCHUNK-1;i++) 
       (heap_free_head + i)->u.next = heap_free_head + i + 1;
    (heap_free_head + NCHUNK - 1)->u.next = NULL;
 }
 temp_ptr = heap_free_head;
 heap_free_head = heap_free_head->u.next;
#ifdef DEBUG
    num_heap_allocated++;
#endif
 return (temp_ptr);
}


void free_heap_data (struct s_heap *hptr) {

 hptr->u.next = heap_free_head; 
 heap_free_head = hptr;
#ifdef DEBUG
    num_heap_allocated--;
#endif
}


struct s_trace *alloc_trace_data (void) {
 int i;
 struct s_trace *temp_ptr;
 
 if (trace_free_head == NULL) {   /* No elements on the free list */
    trace_free_head = (struct s_trace *) my_malloc (NCHUNK *
       sizeof(struct s_trace));
 
/* If I want to free this memory, I have to store the original pointer *
 * somewhere.  Not worthwhile right now -- if you need more memory     *
 * for post-routing stages, look into it.                              */
 
    for (i=0;i<NCHUNK-1;i++)
       (trace_free_head + i)->next = trace_free_head + i + 1;
    (trace_free_head + NCHUNK-1)->next = NULL;
 }
 temp_ptr = trace_free_head;
 trace_free_head = trace_free_head->next;
#ifdef DEBUG
    num_trace_allocated++;
#endif
 return (temp_ptr);
}


void free_trace_data (struct s_trace *tptr) {
 
 tptr->next = trace_free_head;
 trace_free_head = tptr;
#ifdef DEBUG
    num_trace_allocated--;
#endif
}


struct s_linked_f_pointer *alloc_linked_f_pointer (void) {
 
/* This routine returns a linked list element with a float pointer as *
 * the node data.                                                     */
 
 int i;  
 struct s_linked_f_pointer *temp_ptr;
 
 if (linked_f_pointer_free_head == NULL) {
    /* No elements on the free list */
    linked_f_pointer_free_head = (struct s_linked_f_pointer *)
        my_malloc (NCHUNK * sizeof(struct s_linked_f_pointer));
 
/* If I want to free this memory, I have to store the original pointer *
 * somewhere.  Not worthwhile right now -- if you need more memory     * 
 * for post-routing stages, look into it.                              */
 
    for (i=0;i<NCHUNK-1;i++) {
       (linked_f_pointer_free_head + i)->next = linked_f_pointer_free_head
                          + i + 1;
    }
    (linked_f_pointer_free_head + NCHUNK-1)->next = NULL;
 }
 
 temp_ptr = linked_f_pointer_free_head;
 linked_f_pointer_free_head = linked_f_pointer_free_head->next;
 
#ifdef DEBUG     
    num_linked_f_pointer_allocated++;
#endif

 return (temp_ptr);
}


void print_route (char *route_file) {
 int inet, ipin, bnum;
 struct s_trace *tptr;
 char *name_type[4] = {"CHANX","CHANY","IPIN","OPIN"};
 FILE *fp;

 fp = my_open (route_file, "w", 0);

 fprintf(fp,"\nRouting:");
 for (inet=0;inet<num_nets;inet++) {
    if (is_global[inet] == 0) {
       fprintf(fp, "\n\nNet %d (%s)\n\n",inet, net[inet].name);
       tptr = trace_head[inet]; 

       while (tptr != NULL) {
          fprintf(fp, "%5s (%d,%d)", name_type[tptr->type],tptr->i,tptr->j);

          if (tptr->type == IPIN || tptr->type == OPIN) 
             fprintf(fp, "  Pin number: %d",tptr->pinnum);

          fprintf(fp, "\n");

          tptr = tptr->next;
       }
    }

    else {    /* Global net.  Never routed. */
       fprintf(fp, "\n\nNet %d (%s): global net connecting:\n\n",
          inet, net[inet].name);
       for (ipin=0;ipin<net[inet].num_pins;ipin++) {
          bnum = net[inet].pins[ipin];
     
          fprintf(fp, "Block %s (#%d) at (%d, %d), pinclass %d.\n",
             block[bnum].name, bnum, block[bnum].x, block[bnum].y, 
             net_pin_class[inet][ipin]);
       }
    }
 }

#ifdef DEBUG
   fprintf(fp, "\nNum_heap_allocated: %d   Num_trace_allocated: %d\n",
      num_heap_allocated, num_trace_allocated);
   fprintf(fp, "Num_linked_f_pointer_allocated: %d\n", 
      num_linked_f_pointer_allocated);
#endif

 fclose (fp);
}


void routing_stats (boolean full_stats) {

/* Prints out various statistics about the final (best) routing. */

 int i, j, max_occ, total_x, total_y, bends, total_bends, max_bends;
 int length, total_length, max_length;
 float av_occ, av_bends, av_length;
 void get_num_bends_and_length (int inet, int *bends, int *length);
 void print_wirelen_prob_dist (void);

/* Assess how many bends there are in each net's routing, and how long *
 * the routing of each net is.                                         */

 max_bends = 0;
 total_bends = 0;
 max_length = 0;
 total_length = 0;

 for (i=0;i<num_nets;i++) {
    if (is_global[i] == 0) {       /* Globals don't count. */
       get_num_bends_and_length (i, &bends, &length);

       total_bends += bends;
       max_bends = max (bends, max_bends);

       total_length += length;
       max_length = max (length, max_length);
    }
 }

 av_bends = (float) total_bends / (float) (num_nets - num_globals);
 printf ("\nAverage number of bends per net: %f  Maximum # of bends: %d\n\n",
    av_bends, max_bends);

 av_length = (float) total_length / (float) (num_nets - num_globals);
 printf ("Wirelength results (all in units of 1 clb segments):\n");
 printf ("\tTotal wirelength: %d   Average net length: %f\n", total_length,
       av_length);
 printf ("\tMaximum net length: %d\n\n", max_length);

/* Look at track utilization. */

 printf("\nX - Directed channels:\n\n");
 printf("j\tmax occ\tav_occ\t\tcapacity\n");
 
 total_x = 0;

 for (j=0;j<=ny;j++) {
    total_x += chan_width_x[j];
    av_occ = 0.;
    max_occ = -1;

    for (i=1;i<=nx;i++) {
       max_occ = max (chan_x[i][j].occ, max_occ);
       av_occ += chan_x[i][j].occ;
    } 
    av_occ /= nx;
    printf("%d\t%d\t%f\t%d\n", j, max_occ, av_occ, chan_width_x[j]);
 }

 printf("\nY - Directed channels:\n\n");
 printf("i\tmax occ\tav_occ\t\tcapacity\n");
 

 total_y = 0;

 for (i=0;i<=nx;i++) {
    total_y += chan_width_y[i];
    av_occ = 0.;
    max_occ = -1;

    for (j=1;j<=ny;j++) {
       max_occ = max (chan_y[i][j].occ, max_occ);
       av_occ += chan_y[i][j].occ;
    } 
    av_occ /= ny;
    printf("%d\t%d\t%f\t%d\n", i, max_occ, av_occ, chan_width_y[i]);
 }

 printf("\nTotal Tracks in X-direction: %d  in Y-direction: %d\n\n",
    total_x, total_y);

 if (full_stats == TRUE) 
    print_wirelen_prob_dist ();
}


void get_num_bends_and_length (int inet, int *bends, int *length) {

/* Counts and returns the number of bends in net i's routing. */

 struct s_trace *tptr, *prevptr;

 *bends = 0;
 *length = 0;
 prevptr = trace_head[inet];
 tptr = prevptr->next;

 while (tptr != NULL) {

    if (tptr->type == IPIN) {   /* Starting a new segment */
       tptr = tptr->next;       /* Skip link to existing path. */
       if (tptr == NULL) 
          break;
    }
    
    else if (tptr->type == CHANX || tptr->type == CHANY) {
       (*length)++;
       
       if ((tptr->type != prevptr->type) && (prevptr->type == CHANX || 
            prevptr->type == CHANY))
          (*bends)++;
    }

    prevptr = tptr;
    tptr = tptr->next;
 }
}

void dump_occ_and_cost (char *file_name) {
/* A utility routine to dump the contents of the channel and pin *
 * occupancy and cost into a file.  Used only for debugging.     */

 int i, j, ipin;
 FILE *fp;

 fp = my_open (file_name, "w", 0);

 fprintf(fp,"(i,j,ipin)\tpin_inf.occ\tpin_inf.cost\tpin_inf.acc_cost\n\n");
 for (i=1;i<=nx;i++) 
    for (j=1;j<=ny;j++)
       for (ipin=0;ipin<clb_size-1;ipin++) {
          fprintf(fp,"(%d,%d,%d)\t%d\t%f\t%f\n", i, j, ipin, 
              pin_inf[i][j][ipin].occ, pin_inf[i][j][ipin].cost,
              pin_inf[i][j][ipin].acc_cost);
       }

 fprintf(fp,"\n\n(i,j)\tchanx.occ\tchanx.cost\tchanx.acc_cost\n\n");
 for (i=1;i<=nx;i++) 
    for (j=0;j<=ny;j++)
       fprintf(fp,"(%d,%d)\t%d\t%f\t%f\n", i, j, chan_x[i][j].occ,
          chan_x[i][j].cost, chan_x[i][j].acc_cost);

 fprintf(fp,"\n\n(i,j)\tchany.occ\tchany.cost\tchany.acc_cost\n\n");
 fprintf(fp,"\n\n");
 for (i=0;i<=nx;i++) 
    for (j=1;j<=ny;j++)
       fprintf(fp,"(%d,%d)\t%d\t%f\t%f\n", i, j, chan_y[i][j].occ,
          chan_y[i][j].cost, chan_y[i][j].acc_cost);
          
 fclose (fp);
}
