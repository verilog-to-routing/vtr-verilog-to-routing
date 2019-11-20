#include <stdio.h>
#include "util.h"
#include "pr.h"
#include "ext.h"
#include "route.h"
#include "check_route.h"


static void check_source (int inode, int inet);
static void check_sink (int inode, int inet, boolean *pin_done); 
static void check_node (int inode, enum e_route_type route_type);
static boolean check_adjacent (int from_node, int to_node);
static int pin_and_chan_adjacent (int pin_node, int chan_node);
static int chanx_chany_adjacent (int chanx_node, int chany_node);
static void reset_flags (int inet);
static void recompute_occupancy_from_scratch (void);



void check_route (enum e_route_type route_type) {
 
/* This routine checks that a routing:  (1) Describes a properly         *
 * connected path for each net, (2) this path connects all the           *
 * pins spanned by that net, and (3) that no routing resources are       *
 * oversubscribed (the occupancy of everything is recomputed from        *
 * scratch).  The .target_flag elements of the rr_nodes are used for     *
 * scratch space.                                                        */
 
 int inet, ipin, max_pins, inode, prev_node;
 boolean valid, connects;
 struct s_trace *tptr;
 boolean *pin_done;
 
 printf ("\nChecking to ensure routing is legal ...\n");
 
/* Recompute the occupancy from scratch and check for overuse of routing *
 * resources.  This was already checked in order to determine that this  *
 * is a successful routing, but I want to double check it here.          */
 
 recompute_occupancy_from_scratch ();
 valid = feasible_routing ();
 if (valid == FALSE) {
    printf("Error in check_route -- routing resources are overused.\n");
    exit(1);
 }
 
 for (inode=0;inode<num_rr_nodes;inode++)
    rr_node[inode].target_flag = FALSE;    /* Will be used later. */
 
 max_pins = 0;
 for (inet=0;inet<num_nets;inet++)
    max_pins = max(max_pins,net[inet].num_pins);
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
    check_node (inode, route_type);
    rr_node[inode].target_flag = TRUE;   /* Mark as in path. */

    check_source (inode, inet);
    pin_done[0] = TRUE;
 
    prev_node = inode;
    tptr = tptr->next;
 
/* Check the rest of the net */
 
    while (tptr != NULL) {
       inode = tptr->index;
       check_node (inode, route_type);
 
       if (rr_node_draw_inf[prev_node].type == SINK) {
          if (rr_node[inode].target_flag == FALSE) {
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
 
          rr_node[inode].target_flag = TRUE;  /* Mark as in path. */
 
          if (rr_node_draw_inf[inode].type == SINK)
             check_sink (inode, inet, pin_done);
 
       }    /* End of prev_node type != SINK */
       prev_node = inode;
       tptr = tptr->next;
    }  /* End while */
 
    if (rr_node_draw_inf[prev_node].type != SINK) {
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
  
    reset_flags (inet);
 
 }  /* End for each net */
 
 free (pin_done);
 printf("Completed routing consistency check successfully.\n\n");
 
}
 
 
static void check_sink (int inode, int inet, boolean *pin_done) {
 
/* Checks that this SINK node is one of the terminals of inet, and marks   *
 * the appropriate pin as being reached.                                   */
 
 int i, j, ipin, ifound, ptc_num, bnum;
 
 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 ptc_num = rr_node_draw_inf[inode].ptc_num;
 ifound = 0;
 
 if (clb[i][j].type == CLB) {
    bnum = clb[i][j].u.block;
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       if (net[inet].pins[ipin] == bnum && net_pin_class[inet][ipin] ==
             ptc_num) {
          ifound++;
          pin_done[ipin] = TRUE;
       } 
    }
 }
 else {  /* IO pad */
    bnum = clb[i][j].u.io_blocks[ptc_num];
    for (ipin=0;ipin<net[inet].num_pins;ipin++) {
       if (net[inet].pins[ipin] == bnum) {   /* Pad:  no pin class */
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
 int i, j, ptc_num, bnum;
 
 rr_type = rr_node_draw_inf[inode].type;
 if (rr_type != SOURCE) {
    printf ("Error in check_source:  net %d begins with a node of type %d.\n",
            inet, rr_type);
    exit (1);
 }
 
 i = rr_node[inode].xlow;
 j = rr_node[inode].ylow;
 ptc_num = rr_node_draw_inf[inode].ptc_num;
 bnum = net[inet].pins[0];
 
 if (block[bnum].x != i || block[bnum].y != j) {
    printf ("Error in check_source:  net SOURCE is in wrong location (%d,%d)."
            "\n", i, j);
    exit (1);
 }
 
 if (block[bnum].type == CLB) {
    if (ptc_num != net_pin_class[inet][0]) {
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
 
 
static void reset_flags (int inet) {
 
/* This routine resets the flags of all the channel segments contained *
 * in the traceback of net inet to 0.  This allows us to check the     * 
 * next net for connectivity (and the default state of the flags       * 
 * should always be zero after they have been used).                   */
 
 struct s_trace *tptr;
 int inode;
 
 tptr = trace_head[inet];
 
 while (tptr != NULL) {
    inode = tptr->index;
    rr_node[inode].target_flag = FALSE;   /* Not in routed path now. */
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
 
 from_type = rr_node_draw_inf[from_node].type;
 from_xlow = rr_node[from_node].xlow;
 from_ylow = rr_node[from_node].ylow;
 from_ptc = rr_node_draw_inf[from_node].ptc_num;
 to_type = rr_node_draw_inf[to_node].type;
 to_xlow = rr_node[to_node].xlow;
 to_ylow = rr_node[to_node].ylow;
 to_ptc = rr_node_draw_inf[to_node].ptc_num;
 
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
 pin_ptc = rr_node_draw_inf[pin_node].ptc_num;
 chan_type = rr_node_draw_inf[chan_node].type;
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


static void check_node (int inode, enum e_route_type route_type) {
 
/* This routine checks that the rr_node is inside the grid and has a valid  *
 * pin number, etc.                                                         */
 
 int xlow, ylow, xhigh, yhigh, ptc_num, bnum, capacity;
 t_rr_type rr_type;
 int nodes_per_chan, tracks_per_node;
 
 rr_type = rr_node_draw_inf[inode].type;
 xlow = rr_node[inode].xlow;
 xhigh = rr_node[inode].xhigh;
 ylow = rr_node[inode].ylow;
 yhigh = rr_node[inode].yhigh;
 ptc_num = rr_node_draw_inf[inode].ptc_num;
 capacity = rr_node_cost_inf[inode].capacity;
 
 if (xlow > xhigh || ylow > yhigh) {
    printf ("Error in check_node:  rr endpoints are (%d,%d) and (%d,%d).\n",
            xlow, ylow, xhigh, yhigh);
    exit (1);
 }
 
 if (xlow < 0 || xhigh > nx+1 || ylow < 0 || yhigh > ny+1) {
    printf ("Error in check_node:  rr endpoints, (%d,%d) and (%d,%d), \n"
            "are out of range.\n", xlow, ylow, xhigh, yhigh);
    exit (1);
 }
 
 if (ptc_num < 0) {
    printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
            "of %d.\n", inode, rr_type, ptc_num);
    exit (1);
 }
 
/* Check that the segment is within the array and such. */
  
 switch (rr_type) {
 
 case SOURCE: case SINK: case IPIN: case OPIN:
    if (xlow != xhigh || ylow != yhigh) {
       printf ("Error in check_node:  Node %d (type %d) has endpoints of\n"
            "(%d,%d) and (%d,%d)\n", inode, rr_type, xlow, ylow, xhigh, yhigh);
       exit (1);
    }
    if (clb[xlow][ylow].type != CLB && clb[xlow][ylow].type != IO) {
       printf ("Error in check_node:  Node %d (type %d) is at an illegal\n"
               " clb location (%d, %d).\n", inode, rr_type, xlow, ylow);
       exit (1);
    }
    break;
 
 case CHANX:
    if (xlow < 1 || xhigh > nx || yhigh > ny || yhigh != ylow) {
       printf("Error in check_node:  CHANX out of range.\n");
       printf("Endpoints: (%d,%d) and (%d,%d)\n", xlow, ylow, xhigh, yhigh);
       exit(1);
    }
    if (route_type == GLOBAL && xlow != xhigh) {
       printf ("Error in check_node:  node %d spans multiple channel segments\n"
               "which is not allowed with global routing.\n", inode);
       exit (1);
    }
    break;
 
 case CHANY:
    if (xhigh > nx || ylow < 1 || yhigh > ny || xlow != xhigh) {
       printf("Error in check_node:  CHANY out of range.\n");
       printf("Endpoints: (%d,%d) and (%d,%d)\n", xlow, ylow, xhigh, yhigh);
       exit(1);
    }
    if (route_type == GLOBAL && ylow != yhigh) {
       printf ("Error in check_node:  node %d spans multiple channel segments\n"
               "which is not allowed with global routing.\n", inode);
       exit (1);
    }
    break;
 
 default:
    printf("Error in check_node:  Unexpected segment type: %d\n", rr_type);
    exit(1);
 }
 
/* Check that it's capacities and such make sense. */
 
 switch (rr_type) {
 
 case SOURCE:
    if (clb[xlow][ylow].type == CLB) {
       if (ptc_num >= num_class || class_inf[ptc_num].type != DRIVER) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       if (class_inf[ptc_num].num_pins != capacity) {
          printf ("Error in check_node.  Inode %d (type %d) had a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
          exit (1);
       } 
    }
    else {   /* IO block */
       if (ptc_num >= clb[xlow][ylow].occ) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       bnum = clb[xlow][ylow].u.io_blocks[ptc_num];
       if (block[bnum].type != INPAD) {
          printf ("Error in check_node.  Inode %d (type %d) is associated "
                  "with a block of type %d.\n", inode, rr_type,
                   block[bnum].type);
          exit (1);
       } 
       if (capacity != 1) {
          printf ("Error in check_node:  Inode %d (type %d) had a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
          exit (1);
       } 
    }
    break;
 
 case SINK:
    if (clb[xlow][ylow].type == CLB) {
       if (ptc_num >= num_class || class_inf[ptc_num].type != RECEIVER) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       if (class_inf[ptc_num].num_pins != capacity) {
          printf ("Error in check_node.  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
          exit (1);
       } 
    }
    else {   /* IO block */
       if (ptc_num >= clb[xlow][ylow].occ) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
               "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       bnum = clb[xlow][ylow].u.io_blocks[ptc_num];
       if (block[bnum].type != OUTPAD) {
          printf ("Error in check_node.  Inode %d (type %d) is associated "
                  "with a block of type %d.\n", inode, rr_type,
                   block[bnum].type);
          exit (1);
       } 
       if (capacity != 1) {
          printf ("Error in check_node:  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
          exit (1);
       } 
    }
    break;
 
 case OPIN:
    if (clb[xlow][ylow].type == CLB) {
       if (ptc_num >= pins_per_clb || class_inf[clb_pin_class[ptc_num]].type
                != DRIVER) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
    }
    else {   /* IO block */
       if (ptc_num >= clb[xlow][ylow].occ) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       bnum = clb[xlow][ylow].u.io_blocks[ptc_num];
       if (block[bnum].type != INPAD) {
          printf ("Error in check_node.  Inode %d (type %d) is associated "
                  "with a block of type %d.\n", inode, rr_type,
                   block[bnum].type);
          exit (1);
       } 
    }
    if (capacity != 1) {
      printf ("Error in check_node:  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
       exit (1);
    }
    break;
 
 case IPIN:
    if (clb[xlow][ylow].type == CLB) {
       if (ptc_num >= pins_per_clb || class_inf[clb_pin_class[ptc_num]].type
                != RECEIVER) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
    }
    else {   /* IO block */
       if (ptc_num >= clb[xlow][ylow].occ) {
          printf ("Error in check_node.  Inode %d (type %d) had a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
          exit (1);
       } 
       bnum = clb[xlow][ylow].u.io_blocks[ptc_num];
       if (block[bnum].type != OUTPAD) {
          printf ("Error in check_node.  Inode %d (type %d) is associated "
                  "with a block of type %d.\n", inode, rr_type,
                   block[bnum].type);
          exit (1);
       } 
    }
    if (capacity != 1) {
      printf ("Error in check_node:  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
       exit (1);
    }
    break;
 
 case CHANX:
    if (route_type == DETAILED) {
       nodes_per_chan = chan_width_x[ylow];
       tracks_per_node = 1;
    }
    else {
       nodes_per_chan = 1;
       tracks_per_node = chan_width_x[ylow];
    }
 
    if (ptc_num >= nodes_per_chan) {
      printf ("Error in check_node:  Inode %d (type %d) has a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
       exit (1);
    }
 
    if (capacity != tracks_per_node) {
      printf ("Error in check_node:  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
       exit (1);
    }
    break;
 
 case CHANY:
    if (route_type == DETAILED) {
       nodes_per_chan = chan_width_y[xlow];
       tracks_per_node = 1;
    }
    else {
       nodes_per_chan = 1;
       tracks_per_node = chan_width_y[xlow];
    }
 
    if (ptc_num >= nodes_per_chan) {
      printf ("Error in check_node:  Inode %d (type %d) has a ptc_num\n"
                  "of %d.\n", inode, rr_type, ptc_num);
       exit (1);
    }
 
    if (capacity != tracks_per_node) {
      printf ("Error in check_node:  Inode %d (type %d) has a capacity\n"
                  "of %d.\n", inode, rr_type, capacity);
       exit (1);
    }
    break;

 default:
    printf("Error in check_node:  Unexpected segment type: %d\n", rr_type);
    exit(1);

 }
}


static void recompute_occupancy_from_scratch (void) {

/* This routine updates the occ field in the rr_node_cost_inf structure     *
 * according to the resource usage of the current routing.  It does a brute *
 * force recompute from scratch that is useful for sanity checking.         */

 int inode, inet;
 struct s_trace *tptr;

/* First set the occupancy of everything to zero. */

 for (inode=0;inode<num_rr_nodes;inode++)
    rr_node_cost_inf[inode].occ = 0;

/* Now go through each net and count the tracks and pins used everywhere */

 for (inet=0;inet<num_nets;inet++) {
   
    if (is_global[inet])            /* Skip global nets. */
       continue;

    tptr = trace_head[inet];
    if (tptr == NULL)
       continue;

    while (1) {
       inode = tptr->index;
       rr_node_cost_inf[inode].occ++;

       if (rr_node_draw_inf[inode].type == SINK) {
          tptr = tptr->next;                /* Skip next segment. */
          if (tptr == NULL)
             break;
       }
 
       tptr = tptr->next;
    }
 }
}
