#include <stdio.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "route_export.h"
#include "check_route.h"
#include "check_rr_graph.h"


/******************** Subroutines local to this module **********************/

static void check_node_and_range (int inode, enum e_route_type route_type);
static void check_source (int inode, int inet);
static void check_sink (int inode, int inet, boolean *pin_done); 
static void check_switch (struct s_trace *tptr, int num_switch); 
static boolean check_adjacent (int from_node, int to_node);
static int pin_and_chan_adjacent (int pin_node, int chan_node);
static int chanx_chany_adjacent (int chanx_node, int chany_node);
static void reset_flags (int inet, boolean *connected_to_route);
static void recompute_occupancy_from_scratch (t_ivec **clb_opins_used_locally);
static void check_locally_used_clb_opins (t_ivec **clb_opins_used_locally,
         enum e_route_type route_type);


/************************ Subroutine definitions ****************************/


void check_route (enum e_route_type route_type, int num_switch,
        t_ivec **clb_opins_used_locally) {
 
/* This routine checks that a routing:  (1) Describes a properly         *
 * connected path for each net, (2) this path connects all the           *
 * pins spanned by that net, and (3) that no routing resources are       *
 * oversubscribed (the occupancy of everything is recomputed from        *
 * scratch).                                                             */
 
 int inet, ipin, max_pins, inode, prev_node;
 boolean valid, connects;
 boolean *connected_to_route;    /* [0 .. num_rr_nodes-1] */
 struct s_trace *tptr;
 boolean *pin_done;
 
 printf ("\nChecking to ensure routing is legal ...\n");
 
/* Recompute the occupancy from scratch and check for overuse of routing *
 * resources.  This was already checked in order to determine that this  *
 * is a successful routing, but I want to double check it here.          */
 
 recompute_occupancy_from_scratch (clb_opins_used_locally);
 valid = feasible_routing ();
 if (valid == FALSE) {
    printf("Error in check_route -- routing resources are overused.\n");
    exit(1);
 }

 check_locally_used_clb_opins (clb_opins_used_locally, route_type);
 
 connected_to_route = (boolean *) my_calloc (num_rr_nodes, sizeof (boolean));
 
 max_pins = 0;
 for (inet=0;inet<num_nets;inet++)
    max_pins = max (max_pins, net[inet].num_pins);

 pin_done = (boolean *) my_malloc (max_pins * sizeof(boolean));

/* Now check that all nets are indeed connected. */

 for (inet=0;inet<num_nets;inet++) {

    if (is_global[inet])               /* Skip global nets. */
       continue;

    for (ipin=0;ipin<net[inet].num_pins;ipin++)
       pin_done[ipin] = FALSE;

/* Check the SOURCE of the net. */

    tptr = trace_head[inet];
    if (tptr == NULL) {
       printf ("Error in check_route:  net %d has no routing.\n", inet);
       exit (1);
    }

    inode = tptr->index;
    check_node_and_range (inode, route_type);
    check_switch (tptr, num_switch);
    connected_to_route[inode] = TRUE;   /* Mark as in path. */

    check_source (inode, inet);
    pin_done[0] = TRUE;
 
    prev_node = inode;
    tptr = tptr->next;
 
/* Check the rest of the net */
 
    while (tptr != NULL) {
       inode = tptr->index;
       check_node_and_range (inode, route_type);
       check_switch (tptr, num_switch);
 
       if (rr_node[prev_node].type == SINK) {
          if (connected_to_route[inode] == FALSE) {
             printf ("Error in check_route.  Node %d does not link "
                     "into the existing routing for net %d.\n", inode, inet);
             exit(1);
          }
       } 
 
       else {
          connects = check_adjacent (prev_node, inode);
          if (!connects) {
             printf("Error in check_route while checking net %d.\n",
                inet);
             printf("Non-adjacent segments in traceback.\n");
             exit (1);
          }
 
          if (connected_to_route[inode] && rr_node[inode].type != SINK) {

 /* Note:  Can get multiple connections to the same logically-equivalent     *
  * SINK in some logic blocks.                                               */

             printf ("Error in check_route:  net %d routing is not a tree.\n",
                      inet);
             exit (1);
          }

          connected_to_route[inode] = TRUE;  /* Mark as in path. */
 
          if (rr_node[inode].type == SINK)
             check_sink (inode, inet, pin_done);
 
       }    /* End of prev_node type != SINK */
       prev_node = inode;
       tptr = tptr->next;
    }  /* End while */
 
    if (rr_node[prev_node].type != SINK) {
       printf("Error in check_route.  Net %d does not end\n", inet);
       printf("with a SINK.\n");
       exit(1);
    }
 
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       if (pin_done[ipin] == FALSE) {
          printf("Error in check_route.  Net %d does not \n",inet);
          printf("connect to pin %d.\n",ipin);
          exit(1);
       } 
    }
  
    reset_flags (inet, connected_to_route);
 
 }  /* End for each net */
 
 free (pin_done);
 free (connected_to_route);
 printf("Completed routing consistency check successfully.\n\n");
 
}
 
 
static void check_sink (int inode, int inet, boolean *pin_done) {
 
/* Checks that this SINK node is one of the terminals of inet, and marks   *
 * the appropriate pin as being reached.                                   */
 
 int i, j, ipin, ifound, ptc_num, bnum, iclass, blk_pin;
 
 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 ptc_num = rr_node[inode].ptc_num;
 ifound = 0;
 
 if (clb[i][j].type == CLB) {
    bnum = clb[i][j].u.block;
    for (ipin=1;ipin<net[inet].num_pins;ipin++) {  /* All net SINKs */

       if (net[inet].blocks[ipin] == bnum) {
          blk_pin = net[inet].blk_pin[ipin];
          iclass = clb_pin_class[blk_pin];

          if (iclass == ptc_num) {

/* Could connect to same pin class on the same clb more than once.  Only   *
 * update pin_done for a pin that hasn't been reached yet.                 */

             if (pin_done[ipin] == FALSE) {  
                ifound++;
                pin_done[ipin] = TRUE;
                break;
             }
          }
       } 
    }
 }
 else {  /* IO pad */
    bnum = clb[i][j].u.io_blocks[ptc_num];
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       if (net[inet].blocks[ipin] == bnum) {   /* Pad:  no pin class */
          ifound++;
          pin_done[ipin] = TRUE;
       } 
    }
 }
 
 if (ifound > 1) {
    printf ("Error in check_sink:  found %d terminals of net %d of class/pad"
            "\n %d at location (%d, %d).\n", ifound, inet, ptc_num, i, j);
    exit (1);
 }
 
 if (ifound < 1) {
    printf ("Error in check_sink:  node %d does not connect to any terminal "
            "\n of net %d.\n", inode, inet);
    exit (1);
 }
}


static void check_source (int inode, int inet) {
 
/* Checks that the node passed in is a valid source for this net.        */
 
 t_rr_type rr_type;
 int i, j, ptc_num, bnum, blk_pin, iclass;
 
 rr_type = rr_node[inode].type;
 if (rr_type != SOURCE) {
    printf ("Error in check_source:  net %d begins with a node of type %d.\n",
            inet, rr_type);
    exit (1);
 }
 
 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 ptc_num = rr_node[inode].ptc_num;
 bnum = net[inet].blocks[0];
 
 if (block[bnum].x != i || block[bnum].y != j) {
    printf ("Error in check_source:  net SOURCE is in wrong location (%d,%d)."
            "\n", i, j);
    exit (1);
 }
 
 if (block[bnum].type == CLB) {
    blk_pin = net[inet].blk_pin[0];
    iclass = clb_pin_class[blk_pin];

    if (ptc_num != iclass) {
       printf ("Error in check_source:  net SOURCE is of wrong class (%d).\n",
              ptc_num);
       exit (1);
    }
 }
 else {    /* IO Pad.  NB:  check_node ensured ptc_num < occ of this pad.  */
    if (clb[i][j].u.io_blocks[ptc_num] != bnum) {
       printf ("Error in check_source:  net SOURCE is at wrong pad (pad #%d)."
               "\n", ptc_num);
       exit (1);
    }
 }
}


static void check_switch (struct s_trace *tptr, int num_switch) {

/* Checks that the switch leading from this traceback element to the next *
 * one is a legal switch type.                                            */

 int inode;
 short switch_type;

 inode = tptr->index;
 switch_type = tptr->iswitch;

 if (rr_node[inode].type != SINK) {
    if (switch_type < 0 || switch_type >= num_switch) {
       printf ("Error in check_switch: rr_node %d left via switch type %d.\n",
               inode, switch_type);
       printf ("Switch type is out of range.\n");
       exit (1);
    }
 }

 else {  /* Is a SINK */ 

/* Without feedthroughs, there should be no switch.  If feedthroughs are    *
 * allowed, change to treat a SINK like any other node (as above).          */

    if (switch_type != OPEN) {
       printf ("Error in check_switch:  rr_node %d is a SINK, but attempts \n"
               "to use a switch of type %d.\n", inode, switch_type);
       exit (1);
    }
 }
}
 
 
static void reset_flags (int inet, boolean *connected_to_route) {
 
/* This routine resets the flags of all the channel segments contained *
 * in the traceback of net inet to 0.  This allows us to check the     * 
 * next net for connectivity (and the default state of the flags       * 
 * should always be zero after they have been used).                   */
 
 struct s_trace *tptr;
 int inode;
 
 tptr = trace_head[inet];
 
 while (tptr != NULL) {
    inode = tptr->index;
    connected_to_route[inode] = FALSE;   /* Not in routed path now. */
    tptr = tptr->next;
 }
}
 
 
static boolean check_adjacent (int from_node, int to_node) {
 
/* This routine checks if the rr_node to_node is reachable from from_node.   *
 * It returns TRUE if is reachable and FALSE if it is not.  Check_node has   *
 * already been used to verify that both nodes are valid rr_nodes, so only   *
 * adjacency is checked here.                                                */
 
 int from_xlow, from_ylow, to_xlow, to_ylow, from_ptc, to_ptc, iclass;
 int num_adj, to_xhigh, to_yhigh, from_xhigh, from_yhigh, iconn;
 boolean reached;
 t_rr_type from_type, to_type;
 
 reached = FALSE;
 
 for (iconn=0;iconn<rr_node[from_node].num_edges;iconn++) {
    if (rr_node[from_node].edges[iconn] == to_node) {
       reached = TRUE;
       break;
    }
 }
 
 if (!reached)
    return (FALSE);
 
/* Now we know the rr graph says these two nodes are adjacent.  Double  *
 * check that this makes sense, to verify the rr graph.                 */
 
 num_adj = 0;
 
 from_type = rr_node[from_node].type;
 from_xlow = rr_node[from_node].xlow;
 from_ylow = rr_node[from_node].ylow;
 from_ptc = rr_node[from_node].ptc_num;
 to_type = rr_node[to_node].type;
 to_xlow = rr_node[to_node].xlow;
 to_ylow = rr_node[to_node].ylow;
 to_ptc = rr_node[to_node].ptc_num;
 
 switch (from_type) {
 
 case SOURCE:
    if (to_type == OPIN) {
       if (from_xlow == to_xlow && from_ylow == to_ylow) {
          if (clb[to_xlow][to_ylow].type == CLB) {
             iclass = clb_pin_class[to_ptc];
             if (iclass == from_ptc)
                num_adj++;
          }
          else {   /* IO block */
             if (to_ptc == from_ptc)
                num_adj++;
          }
       } 
    }
    break;
 
 case SINK:
    if (to_type == SOURCE) {   /* Feedthrough.  Not in code as yet. */
       if (from_xlow == to_xlow && from_ylow == to_ylow &&
               clb[to_xlow][to_ylow].type == CLB)
          num_adj++;
    }
    break;
 
 case OPIN:
    if (to_type == CHANX || to_type == CHANY)
       num_adj += pin_and_chan_adjacent (from_node, to_node);
 
    break;
 
 case IPIN:
    if (to_type == SINK && from_xlow == to_xlow && from_ylow == to_ylow) {
       if (clb[from_xlow][from_ylow].type == CLB) {
          iclass = clb_pin_class[from_ptc];
          if (iclass == to_ptc)
             num_adj++;
       } 
       else {    /* OUTPAD */
          if (from_ptc == to_ptc)
             num_adj++;
       } 
    }
    break;
 
 case CHANX:
    if (to_type == IPIN) {
       num_adj += pin_and_chan_adjacent (to_node, from_node);
    }
    else if (to_type == CHANX) {
       from_xhigh = rr_node[from_node].xhigh;
       to_xhigh = rr_node[to_node].xhigh;
       if (from_ylow == to_ylow) {
          if (to_xhigh == from_xlow-1 || from_xhigh == to_xlow-1)
             num_adj++;
       } 
    }
    else if (to_type == CHANY) {
       num_adj += chanx_chany_adjacent (from_node, to_node);
    }
    break;
 
 case CHANY:
    if (to_type == IPIN) {
       num_adj += pin_and_chan_adjacent (to_node, from_node);
    }
    else if (to_type == CHANY) {
       from_yhigh = rr_node[from_node].yhigh;
       to_yhigh = rr_node[to_node].yhigh;
       if (from_xlow == to_xlow) {
          if (to_yhigh == from_ylow-1 || from_yhigh == to_ylow-1)
             num_adj++;
       } 
    }
    else if (to_type == CHANX) {
       num_adj += chanx_chany_adjacent (to_node, from_node);
    }
    break;
 
 default:
    break;
 
 }
 
 if (num_adj == 1)
    return (TRUE);
 else if (num_adj == 0)
    return (FALSE);
 
 printf ("Error in check_adjacent: num_adj = %d. Expected 0 or 1.\n", num_adj);
 exit (1);
}
 
 
static int chanx_chany_adjacent (int chanx_node, int chany_node) {
 
/* Returns 1 if the specified CHANX and CHANY nodes are adjacent, 0         *
 * otherwise.                                                               */
 
 int chanx_y, chanx_xlow, chanx_xhigh;
 int chany_x, chany_ylow, chany_yhigh;
 
 chanx_y = rr_node[chanx_node].ylow;
 chanx_xlow = rr_node[chanx_node].xlow;
 chanx_xhigh = rr_node[chanx_node].xhigh;

 chany_x = rr_node[chany_node].xlow; 
 chany_ylow = rr_node[chany_node].ylow; 
 chany_yhigh = rr_node[chany_node].yhigh; 

 if (chany_ylow > chanx_y+1 || chany_yhigh < chanx_y)
    return (0);
 
 if (chanx_xlow > chany_x+1 || chanx_xhigh < chany_x)
    return (0);
 
 return (1);
}
 
 
static int pin_and_chan_adjacent (int pin_node, int chan_node) {
 
/* Checks if pin_node is adjacent to chan_node.  It returns 1 if the two   *
 * nodes are adjacent and 0 if they are not (any other value means there's *
 * a bug in this routine).                                                 */
 
 int num_adj, pin_x, pin_y, chan_xlow, chan_ylow, chan_xhigh, chan_yhigh;
 int pin_ptc;
 t_rr_type chan_type;
 
 num_adj = 0;
 pin_x = rr_node[pin_node].xlow;
 pin_y = rr_node[pin_node].ylow;
 pin_ptc = rr_node[pin_node].ptc_num;
 chan_type = rr_node[chan_node].type;
 chan_xlow = rr_node[chan_node].xlow;
 chan_ylow = rr_node[chan_node].ylow;
 chan_xhigh = rr_node[chan_node].xhigh;
 chan_yhigh = rr_node[chan_node].yhigh;
 
 if (clb[pin_x][pin_y].type == CLB) {
    if (chan_type == CHANX) {
       if (chan_ylow == pin_y) {   /* CHANX above CLB */
          if (pinloc[TOP][pin_ptc] == 1 && pin_x <= chan_xhigh &&
                pin_x >= chan_xlow)
             num_adj++;
       } 
       else if (chan_ylow == pin_y-1) {   /* CHANX below CLB */
          if (pinloc[BOTTOM][pin_ptc] == 1 && pin_x <= chan_xhigh &&
                pin_x >= chan_xlow)
             num_adj++;
       } 
    }
    else if (chan_type == CHANY) {
       if (chan_xlow == pin_x) {   /* CHANY to right of CLB */
          if (pinloc[RIGHT][pin_ptc] == 1 && pin_y <= chan_yhigh &&
                pin_y >= chan_ylow)
             num_adj++;
       } 
       else if (chan_xlow == pin_x-1) {   /* CHANY to left of CLB */
          if (pinloc[LEFT][pin_ptc] == 1 && pin_y <= chan_yhigh &&
                pin_y >= chan_ylow)
             num_adj++;
       } 
    }
 }
 
 else {            /* IO pad */
    if (pin_y == 0) {    /* Bottom row of pads. */
       if (chan_type == CHANX && chan_ylow == 0 && pin_x <= chan_xhigh &&
              pin_x >= chan_xlow)
          num_adj++;
    }
    else if (pin_y == ny+1) {   /* Top row of pads. */
       if (chan_type == CHANX && chan_ylow == ny && pin_x <= chan_xhigh &&
              pin_x >= chan_xlow)
          num_adj++;
    }
    else if (pin_x == 0) {     /* Left column of pads */
       if (chan_type == CHANY && chan_xlow == 0 && pin_y <= chan_yhigh &&
              pin_y >= chan_ylow)
          num_adj++;
    }
    else if (pin_x == nx+1) {  /* Right row of pads */
       if (chan_type == CHANY && chan_xlow == nx && pin_y <= chan_yhigh &&
              pin_y >= chan_ylow)
          num_adj++;
    }
 }
 
 return (num_adj);
}


static void recompute_occupancy_from_scratch (t_ivec **clb_opins_used_locally)
           {

/* This routine updates the occ field in the rr_node structure according to *
 * the resource usage of the current routing.  It does a brute force        *
 * recompute from scratch that is useful for sanity checking.               */

 int inode, inet, iblk, iclass, ipin, num_local_opins;
 struct s_trace *tptr;

/* First set the occupancy of everything to zero. */

 for (inode=0;inode<num_rr_nodes;inode++)
    rr_node[inode].occ = 0;

/* Now go through each net and count the tracks and pins used everywhere */

 for (inet=0;inet<num_nets;inet++) {
   
    if (is_global[inet])            /* Skip global nets. */
       continue;

    tptr = trace_head[inet];
    if (tptr == NULL)
       continue;

    while (1) {
       inode = tptr->index;
       rr_node[inode].occ++;

       if (rr_node[inode].type == SINK) {
          tptr = tptr->next;                /* Skip next segment. */
          if (tptr == NULL)
             break;
       }
 
       tptr = tptr->next;
    }
 }

/* Now update the occupancy of each of the "locally used" OPINs on each CLB *
 * (CLB outputs used up by being directly wired to subblocks used only      *
 * locally).                                                                */

 for (iblk=0;iblk<num_blocks;iblk++) {
    for (iclass=0;iclass<num_class;iclass++) {
       num_local_opins = clb_opins_used_locally[iblk][iclass].nelem; 
           /* Will always be 0 for pads or SINK classes. */
       for (ipin=0;ipin<num_local_opins;ipin++) {
          inode = clb_opins_used_locally[iblk][iclass].list[ipin];
          rr_node[inode].occ++;
       }
    }
 }
}


static void check_locally_used_clb_opins (t_ivec **clb_opins_used_locally,
         enum e_route_type route_type) {

/* Checks that enough OPINs on CLBs have been set aside (used up) to make a *
 * legal routing if subblocks connect to OPINs directly.                    */

 int iclass, iblk, num_local_opins, inode, ipin;
 t_rr_type rr_type;
 
 for (iblk=0;iblk<num_blocks;iblk++) {
    for (iclass=0;iclass<num_class;iclass++) {
       num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
        /* Always 0 for pads and for SINK classes */

       for (ipin=0;ipin<num_local_opins;ipin++) {
          inode = clb_opins_used_locally[iblk][iclass].list[ipin];
          check_node_and_range (inode, route_type);  /* Node makes sense? */

         /* Now check that node is an OPIN of the right type. */

          rr_type = rr_node[inode].type;
          if (rr_type != OPIN) {
             printf ("Error in check_locally_used_opins:  Block #%d (%s)\n"
                    "\tclass %d locally used OPIN is of the wrong rr_type --\n"
                    "\tit is rr_node #%d of type %d.\n", iblk, 
                    block[iblk].name, iclass, inode, rr_type);
             exit (1);
          }

          ipin = rr_node[inode].ptc_num;
          if (clb_pin_class[ipin] != iclass) {
             printf ("Error in check_locally_used_opins:  Block #%d (%s):\n"
                    "\tExpected class %d locally used OPIN, got class %d."
                    "\trr_node #: %d.\n", iblk, block[iblk].name, iclass, 
                    clb_pin_class[ipin], inode);
             exit (1);
          }
       }
    }
 }
}


static void check_node_and_range (int inode, enum e_route_type route_type) {

/* Checks that inode is within the legal range, then calls check_node to    *
 * check that everything else about the node is OK.                         */

 if (inode < 0 || inode >= num_rr_nodes) {
    printf ("Error in check_node_and_range:  rr_node #%d is out of legal "
            "\trange (0 to %d).\n", inode, num_rr_nodes-1);
    exit (1);
 }
 check_node (inode, route_type);
}
