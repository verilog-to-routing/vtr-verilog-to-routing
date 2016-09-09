#include <cstdio>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "route_export.h"
#include "check_route.h"
#include "rr_graph.h"
#include "check_rr_graph.h"
#include "read_xml_arch_file.h"

/******************** Subroutines local to this module **********************/
static void check_node_and_range(int inode, enum e_route_type route_type);
static void check_source(int inode, int inet);
static void check_sink(int inode, int inet, bool * pin_done);
static void check_switch(struct s_trace *tptr, int num_switch);
static bool check_adjacent(int from_node, int to_node);
static int pin_and_chan_adjacent(int pin_node, int chan_node);
static int chanx_chany_adjacent(int chanx_node, int chany_node);
static void reset_flags(int inet, bool * connected_to_route);
static void recompute_occupancy_from_scratch(vtr::t_ivec ** clb_opins_used_locally);
static void check_locally_used_clb_opins(vtr::t_ivec ** clb_opins_used_locally,
		enum e_route_type route_type);

/************************ Subroutine definitions ****************************/

void check_route(enum e_route_type route_type, int num_switches,
		vtr::t_ivec ** clb_opins_used_locally) {

	/* This routine checks that a routing:  (1) Describes a properly         *
	 * connected path for each net, (2) this path connects all the           *
	 * pins spanned by that net, and (3) that no routing resources are       *
	 * oversubscribed (the occupancy of everything is recomputed from        *
	 * scratch).                                                             */

	int max_pins, inode, prev_node;
	unsigned int inet, ipin;
	bool valid, connects;
	bool * connected_to_route; /* [0 .. num_rr_nodes-1] */
	struct s_trace *tptr;
	bool * pin_done;

	vtr::printf_info("\n");
	vtr::printf_info("Checking to ensure routing is legal...\n");

	/* Recompute the occupancy from scratch and check for overuse of routing *
	 * resources.  This was already checked in order to determine that this  *
	 * is a successful routing, but I want to double check it here.          */

	recompute_occupancy_from_scratch(clb_opins_used_locally);
	valid = feasible_routing();
	if (valid == false) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 		
			"Error in check_route -- routing resources are overused.\n");
	}

	check_locally_used_clb_opins(clb_opins_used_locally, route_type);

	connected_to_route = (bool *) vtr::calloc(num_rr_nodes, sizeof(bool));

	max_pins = 0;
	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++)
		max_pins = max(max_pins, (int) g_clbs_nlist.net[inet].pins.size());

	pin_done = (bool *) vtr::malloc(max_pins * sizeof(bool));

	/* Now check that all nets are indeed connected. */

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		if (g_clbs_nlist.net[inet].is_global || g_clbs_nlist.net[inet].num_sinks() == 0) /* Skip global nets. */
			continue;

		for (ipin = 0; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++)
			pin_done[ipin] = false;

		/* Check the SOURCE of the net. */

		tptr = trace_head[inet];
		if (tptr == NULL) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_route: net %d has no routing.\n", inet);
		}

		inode = tptr->index;
		check_node_and_range(inode, route_type);
		check_switch(tptr, num_switches);
		connected_to_route[inode] = true; /* Mark as in path. */

		check_source(inode, inet);
		pin_done[0] = true;

		prev_node = inode;
		tptr = tptr->next;

		/* Check the rest of the net */

		while (tptr != NULL) {
			inode = tptr->index;
			check_node_and_range(inode, route_type);
			check_switch(tptr, num_switches);

			if (rr_node[prev_node].type == SINK) {
				if (connected_to_route[inode] == false) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 					
						"in check_route: node %d does not link into existing routing for net %d.\n", inode, inet);
				}
			}

			else {
				connects = check_adjacent(prev_node, inode);
				if (!connects) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 					
						"in check_route: found non-adjacent segments in traceback while checking net %d.\n", inet);
				}

				if (connected_to_route[inode] && rr_node[inode].type != SINK) {

					/* Note:  Can get multiple connections to the same logically-equivalent     *
					 * SINK in some logic blocks.                                               */

					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 					
						"in check_route: net %d routing is not a tree.\n", inet);
				}

				connected_to_route[inode] = true; /* Mark as in path. */

				if (rr_node[inode].type == SINK)
					check_sink(inode, inet, pin_done);

			} /* End of prev_node type != SINK */
			prev_node = inode;
			tptr = tptr->next;
		} /* End while */

		if (rr_node[prev_node].type != SINK) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_route: net %d does not end with a SINK.\n", inet);
		}

		for (ipin = 0; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) {
			if (pin_done[ipin] == false) {
				vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 				
					"in check_route: net %d does not connect to pin %d.\n", inet, ipin);
			}
		}

		reset_flags(inet, connected_to_route);

	} /* End for each net */

	free(pin_done);
	free(connected_to_route);
	vtr::printf_info("Completed routing consistency check successfully.\n");
	vtr::printf_info("\n");
}

static void check_sink(int inode, int inet, bool * pin_done) {

	/* Checks that this SINK node is one of the terminals of inet, and marks   *
	 * the appropriate pin as being reached.                                   */

	int i, j, ifound, ptc_num, bnum, iclass, node_block_pin, iblk;
	unsigned int ipin;
	t_type_ptr type;

	VTR_ASSERT(rr_node[inode].type == SINK);
	i = rr_node[inode].get_xlow();
	j = rr_node[inode].get_ylow();
	type = grid[i][j].type;
	/* For sinks, ptc_num is the class */
	ptc_num = rr_node[inode].get_ptc_num(); 
	ifound = 0;

	for (iblk = 0; iblk < type->capacity; iblk++) {
		bnum = grid[i][j].blocks[iblk]; /* Hardcoded to one block */
		for (ipin = 1; ipin < g_clbs_nlist.net[inet].pins.size(); ipin++) { /* All net SINKs */
			if (g_clbs_nlist.net[inet].pins[ipin].block == bnum) {
				node_block_pin = g_clbs_nlist.net[inet].pins[ipin].block_pin;
				iclass = type->pin_class[node_block_pin];
				if (iclass == ptc_num) {
					/* Could connect to same pin class on the same clb more than once.  Only   *
					 * update pin_done for a pin that hasn't been reached yet.                 */

					if (pin_done[ipin] == false) {
						ifound++;
						pin_done[ipin] = true;
					}
				}
			}
		}
	}

	if (ifound > 1 && type == IO_TYPE) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 		
			"in check_sink: found %d terminals of net %d of pad %d at location (%d, %d).\n", ifound, inet, ptc_num, i, j);
	}

	if (ifound < 1) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 		
				 "in check_sink: node %d does not connect to any terminal of net %s #%d.\n"
				 "This error is usually caused by incorrectly specified logical equivalence in your architecture file.\n"
				 "You should try to respecify what pins are equivalent or turn logical equivalence off.\n", inode, g_clbs_nlist.net[inet].name, inet);
	}
}

static void check_source(int inode, int inet) {

	/* Checks that the node passed in is a valid source for this net.        */

	t_rr_type rr_type;
	t_type_ptr type;
	int i, j, ptc_num, bnum, node_block_pin, iclass;

	rr_type = rr_node[inode].type;
	if (rr_type != SOURCE) {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 		
			"in check_source: net %d begins with a node of type %d.\n", inet, rr_type);
	}

	i = rr_node[inode].get_xlow();
	j = rr_node[inode].get_ylow();
	/* for sinks and sources, ptc_num is class */
	ptc_num = rr_node[inode].get_ptc_num(); 
	/* First node_block for net is the source */
	bnum = g_clbs_nlist.net[inet].pins[0].block; 
	type = grid[i][j].type;

	if (block[bnum].x != i || block[bnum].y != j) {		
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_source: net SOURCE is in wrong location (%d,%d).\n", i, j);
	}

	node_block_pin = g_clbs_nlist.net[inet].pins[0].block_pin;
	iclass = type->pin_class[node_block_pin];

	if (ptc_num != iclass) {		
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_source: net SOURCE is of wrong class (%d).\n", ptc_num);
	}
}

static void check_switch(struct s_trace *tptr, int num_switch) {

	/* Checks that the switch leading from this traceback element to the next *
	 * one is a legal switch type.                                            */

	int inode;
	short switch_type;

	inode = tptr->index;
	switch_type = tptr->iswitch;

	if (rr_node[inode].type != SINK) {
		if (switch_type < 0 || switch_type >= num_switch) {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_switch: rr_node %d left via switch type %d.\n"
				"\tSwitch type is out of range.\n", inode, switch_type);
		}
	}

	else { /* Is a SINK */

		/* Without feedthroughs, there should be no switch.  If feedthroughs are    *
		 * allowed, change to treat a SINK like any other node (as above).          */

		if (switch_type != OPEN) {			
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_switch: rr_node %d is a SINK, but attempts to use a switch of type %d.\n", inode, switch_type);
		}
	}
}

static void reset_flags(int inet, bool * connected_to_route) {

	/* This routine resets the flags of all the channel segments contained *
	 * in the traceback of net inet to 0.  This allows us to check the     * 
	 * next net for connectivity (and the default state of the flags       * 
	 * should always be zero after they have been used).                   */

	struct s_trace *tptr;
	int inode;

	tptr = trace_head[inet];

	while (tptr != NULL) {
		inode = tptr->index;
		connected_to_route[inode] = false; /* Not in routed path now. */
		tptr = tptr->next;
	}
}

static bool check_adjacent(int from_node, int to_node) {

	/* This routine checks if the rr_node to_node is reachable from from_node.   *
	 * It returns true if is reachable and false if it is not.  Check_node has   *
	 * already been used to verify that both nodes are valid rr_nodes, so only   *
	 * adjacency is checked here.                                                
	 * Special case: direct OPIN to IPIN connections need not be adjacent.  These
	 * represent specially-crafted connections such as carry-chains or more advanced
	 * blocks where adjacency is overridden by the architect */

	 
	int from_xlow, from_ylow, to_xlow, to_ylow, from_ptc, to_ptc, iclass;
	int num_adj, to_xhigh, to_yhigh, from_xhigh, from_yhigh, iconn;
	bool reached;
	t_rr_type from_type, to_type;
	t_type_ptr from_grid_type, to_grid_type;

	reached = false;

	for (iconn = 0; iconn < rr_node[from_node].get_num_edges(); iconn++) {
		if (rr_node[from_node].edges[iconn] == to_node) {
			reached = true;
			break;
		}
	}

	VTR_ASSERT(reached==1);
	if (!reached)
		return (false);

	/* Now we know the rr graph says these two nodes are adjacent.  Double  *
	 * check that this makes sense, to verify the rr graph.                 */

	num_adj = 0;

	from_type = rr_node[from_node].type;
	from_xlow = rr_node[from_node].get_xlow();
	from_ylow = rr_node[from_node].get_ylow();
	from_xhigh = rr_node[from_node].get_xhigh();
	from_yhigh = rr_node[from_node].get_yhigh();
	from_ptc = rr_node[from_node].get_ptc_num();
	to_type = rr_node[to_node].type;
	to_xlow = rr_node[to_node].get_xlow();
	to_ylow = rr_node[to_node].get_ylow();
	to_xhigh = rr_node[to_node].get_xhigh();
	to_yhigh = rr_node[to_node].get_yhigh();
	to_ptc = rr_node[to_node].get_ptc_num();

	switch (from_type) {

	case SOURCE:
		VTR_ASSERT(to_type == OPIN);
		if (from_xlow == to_xlow && from_ylow == to_ylow
				&& from_xhigh == to_xhigh && from_yhigh == to_yhigh) {

			from_grid_type = grid[from_xlow][from_ylow].type;
			to_grid_type = grid[to_xlow][to_ylow].type;
			VTR_ASSERT(from_grid_type == to_grid_type);

			iclass = to_grid_type->pin_class[to_ptc];
			if (iclass == from_ptc)
				num_adj++;

			
		}
		break;

	case SINK:
		/* SINKS are adjacent to not connected */
		break;

	case OPIN:
		if(to_type == CHANX || to_type == CHANY) {
			num_adj += pin_and_chan_adjacent(from_node, to_node);
		} else {
			VTR_ASSERT(to_type == IPIN); /* direct OPIN to IPIN connections not necessarily adjacent */
			return true; /* Special case, direct OPIN to IPIN connections need not be adjacent */
		}

		break;

	case IPIN:
		VTR_ASSERT(to_type == SINK);
		if (from_xlow == to_xlow && from_ylow == to_ylow
				&& from_xhigh == to_xhigh && from_yhigh == to_yhigh) {

			from_grid_type = grid[from_xlow][from_ylow].type;
			to_grid_type = grid[to_xlow][to_ylow].type;
			VTR_ASSERT(from_grid_type == to_grid_type);

			iclass = from_grid_type->pin_class[from_ptc];
			if (iclass == to_ptc)
				num_adj++;
		}
		break;

	case CHANX:
		if (to_type == IPIN) {
			num_adj += pin_and_chan_adjacent(to_node, from_node);
		} else if (to_type == CHANX) {
			from_xhigh = rr_node[from_node].get_xhigh();
			to_xhigh = rr_node[to_node].get_xhigh();
			if (from_ylow == to_ylow) {
				/* UDSD Modification by WMF Begin */
				/*For Fs > 3, can connect to overlapping wire segment */
				if (to_xhigh == from_xlow - 1 || from_xhigh == to_xlow - 1) {
					num_adj++;
				}
				/* Overlapping */
				else {
					int i;

					for (i = from_xlow; i <= from_xhigh; i++) {
						if (i >= to_xlow && i <= to_xhigh) {
							num_adj++;
							break;
						}
					}
				}
				/* UDSD Modification by WMF End */
			}
		} else if (to_type == CHANY) {
			num_adj += chanx_chany_adjacent(from_node, to_node);
		} else {
			VTR_ASSERT(0);
		}
		break;

	case CHANY:
		if (to_type == IPIN) {
			num_adj += pin_and_chan_adjacent(to_node, from_node);
		} else if (to_type == CHANY) {
			from_yhigh = rr_node[from_node].get_yhigh();
			to_yhigh = rr_node[to_node].get_yhigh();
			if (from_xlow == to_xlow) {
				/* UDSD Modification by WMF Begin */
				if (to_yhigh == from_ylow - 1 || from_yhigh == to_ylow - 1) {
					num_adj++;
				}
				/* Overlapping */
				else {
					int j;

					for (j = from_ylow; j <= from_yhigh; j++) {
						if (j >= to_ylow && j <= to_yhigh) {
							num_adj++;
							break;
						}
					}
				}
				/* UDSD Modification by WMF End */
			}
		} else if (to_type == CHANX) {
			num_adj += chanx_chany_adjacent(to_node, from_node);
		} else {
			VTR_ASSERT(0);
		}
		break;

	default:
		break;

	}

	if (num_adj == 1)
		return (true);
	else if (num_adj == 0)
		return (false);
	
	vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
		"in check_adjacent: num_adj = %d. Expected 0 or 1.\n", num_adj);
	return false; //Should not reach here once thrown
}

static int chanx_chany_adjacent(int chanx_node, int chany_node) {

	/* Returns 1 if the specified CHANX and CHANY nodes are adjacent, 0         *
	 * otherwise.                                                               */

	int chanx_y, chanx_xlow, chanx_xhigh;
	int chany_x, chany_ylow, chany_yhigh;

	chanx_y = rr_node[chanx_node].get_ylow();
	chanx_xlow = rr_node[chanx_node].get_xlow();
	chanx_xhigh = rr_node[chanx_node].get_xhigh();

	chany_x = rr_node[chany_node].get_xlow();
	chany_ylow = rr_node[chany_node].get_ylow();
	chany_yhigh = rr_node[chany_node].get_yhigh();

	if (chany_ylow > chanx_y + 1 || chany_yhigh < chanx_y)
		return (0);

	if (chanx_xlow > chany_x + 1 || chanx_xhigh < chany_x)
		return (0);

	return (1);
}

static int pin_and_chan_adjacent(int pin_node, int chan_node) {

	/* Checks if pin_node is adjacent to chan_node.  It returns 1 if the two   *
	 * nodes are adjacent and 0 if they are not (any other value means there's *
	 * a bug in this routine).                                                 */

	 /*
	 This functions checks that channels are located at sides of the blocks.
	 After the change to pin allignment, this is no longer true.
	 Hence, I'm overriding this function
	 */
	 return true;
}

static void recompute_occupancy_from_scratch(vtr::t_ivec ** clb_opins_used_locally) {

	/* This routine updates the occ field in the rr_node structure according to *
	 * the resource usage of the current routing.  It does a brute force        *
	 * recompute from scratch that is useful for sanity checking.               */

	int inode, iblk, iclass, ipin, num_local_opins;
	unsigned inet;
	struct s_trace *tptr;

	/* First set the occupancy of everything to zero. */

	for (inode = 0; inode < num_rr_nodes; inode++)
		rr_node[inode].set_occ(0);

	/* Now go through each net and count the tracks and pins used everywhere */

	for (inet = 0; inet < g_clbs_nlist.net.size(); inet++) {

		if (g_clbs_nlist.net[inet].is_global) /* Skip global nets. */
			continue;

		tptr = trace_head[inet];
		if (tptr == NULL)
			continue;

		for (;;) {
			inode = tptr->index;
			rr_node[inode].set_occ(rr_node[inode].get_occ() + 1);

			if (rr_node[inode].type == SINK) {
				tptr = tptr->next; /* Skip next segment. */
				if (tptr == NULL)
					break;
			}

			tptr = tptr->next;
		}
	}

	/* Now update the occupancy of each of the "locally used" OPINs on each CLB *
	 * (CLB outputs used up by being directly wired to subblocks used only      *
	 * locally).                                                                */

	for (iblk = 0; iblk < num_blocks; iblk++) {
		for (iclass = 0; iclass < block[iblk].type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
			/* Will always be 0 for pads or SINK classes. */
			for (ipin = 0; ipin < num_local_opins; ipin++) {
				inode = clb_opins_used_locally[iblk][iclass].list[ipin];
				rr_node[inode].set_occ(rr_node[inode].get_occ() + 1);
			}
		}
	}
}

static void check_locally_used_clb_opins(vtr::t_ivec ** clb_opins_used_locally,
		enum e_route_type route_type) {

	/* Checks that enough OPINs on CLBs have been set aside (used up) to make a *
	 * legal routing if subblocks connect to OPINs directly.                    */

	int iclass, iblk, num_local_opins, inode, ipin;
	t_rr_type rr_type;

	for (iblk = 0; iblk < num_blocks; iblk++) {
		for (iclass = 0; iclass < block[iblk].type->num_class; iclass++) {
			num_local_opins = clb_opins_used_locally[iblk][iclass].nelem;
			/* Always 0 for pads and for SINK classes */

			for (ipin = 0; ipin < num_local_opins; ipin++) {
				inode = clb_opins_used_locally[iblk][iclass].list[ipin];
				check_node_and_range(inode, route_type); /* Node makes sense? */

				/* Now check that node is an OPIN of the right type. */

				rr_type = rr_node[inode].type;
				if (rr_type != OPIN) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 					
						"in check_locally_used_opins: block #%d (%s)\n"
						"\tClass %d local OPIN is wrong rr_type -- rr_node #%d of type %d.\n",
						iblk, block[iblk].name, iclass, inode, rr_type);
				}

				ipin = rr_node[inode].get_ptc_num();
				if (block[iblk].type->pin_class[ipin] != iclass) {
					vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 					
						"in check_locally_used_opins: block #%d (%s):\n"
						"\tExpected class %d local OPIN has class %d -- rr_node #: %d.\n",
						iblk, block[iblk].name, iclass,	block[iblk].type->pin_class[ipin], inode);
				}
			}
		}
	}
}

static void check_node_and_range(int inode, enum e_route_type route_type) {

	/* Checks that inode is within the legal range, then calls check_node to    *
	 * check that everything else about the node is OK.                         */

	if (inode < 0 || inode >= num_rr_nodes) { 		
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 			
				"in check_node_and_range: rr_node #%d is out of legal, range (0 to %d).\n", inode, num_rr_nodes - 1);
	}
	check_node(inode, route_type);
}
