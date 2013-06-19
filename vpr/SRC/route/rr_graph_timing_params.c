#include <cstdio>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_timing_params.h"

/****************** Subroutine definitions *********************************/

void add_rr_graph_C_from_switches(float C_ipin_cblock) {

	/* This routine finishes loading the C elements of the rr_graph. It assumes *
	 * that when you call it the CHANX and CHANY nodes have had their C set to  *
	 * their metal capacitance, and everything else has C set to 0.  The graph  *
	 * connectivity (edges, switch types etc.) must all be loaded too.  This    *
	 * routine will add in the capacitance on the CHANX and CHANY nodes due to: *
	 *                                                                          *
	 * 1) The output capacitance of the switches coming from OPINs;             *
	 * 2) The input and output capacitance of the switches between the various  *
	 *    wiring (CHANX and CHANY) segments; and                                *
	 * 3) The input capacitance of the buffers separating routing tracks from   *
	 *    the connection block inputs.                                          */

	int inode, iedge, switch_index, to_node, maxlen;
	int icblock, isblock, iseg_low, iseg_high;
	float Cin, Cout;
	t_rr_type from_rr_type, to_rr_type;
	boolean * cblock_counted; /* [0..max(nx,ny)] -- 0th element unused. */
	float *buffer_Cin; /* [0..max(nx,ny)] */
	boolean buffered;
	float *Couts_to_add; /* UDSD */

	maxlen = max(nx, ny) + 1;
	cblock_counted = (boolean *) my_calloc(maxlen, sizeof(boolean));
	buffer_Cin = (float *) my_calloc(maxlen, sizeof(float));

	for (inode = 0; inode < num_rr_nodes; inode++) {

		from_rr_type = rr_node[inode].type;

		if (from_rr_type == CHANX || from_rr_type == CHANY) {

			for (iedge = 0; iedge < rr_node[inode].num_edges; iedge++) {

				to_node = rr_node[inode].edges[iedge];
				to_rr_type = rr_node[to_node].type;

				if (to_rr_type == CHANX || to_rr_type == CHANY) {

					switch_index = rr_node[inode].switches[iedge];
					Cin = switch_inf[switch_index].Cin;
					Cout = switch_inf[switch_index].Cout;
					buffered = switch_inf[switch_index].buffered;

					/* If both the switch from inode to to_node and the switch from *
					 * to_node back to inode use bidirectional switches (i.e. pass  *
					 * transistors), there will only be one physical switch for     *
					 * both edges.  Hence, I only want to count the capacitance of  *
					 * that switch for one of the two edges.  (Note:  if there is   *
					 * a pass transistor edge from x to y, I always build the graph *
					 * so that there is a corresponding edge using the same switch  *
					 * type from y to x.) So, I arbitrarily choose to add in the    *
					 * capacitance in that case of a pass transistor only when      *
					 * processing the the lower inode number.                       *
					 * If an edge uses a buffer I always have to add in the output  *
					 * capacitance.  I assume that buffers are shared at the same   *
					 * (i,j) location, so only one input capacitance needs to be    *
					 * added for all the buffered switches at that location.  If    *
					 * the buffers at that location have different sizes, I use the *
					 * input capacitance of the largest one.                        */

					if (!buffered && inode < to_node) { /* Pass transistor. */
						rr_node[inode].C += Cin;
						rr_node[to_node].C += Cout;
					}

					else if (buffered) {
						/* Prevent double counting of capacitance for UDSD */
						if (rr_node[to_node].drivers != SINGLE) {
							/* For multiple-driver architectures the output capacitance can
							 * be added now since each edge is actually a driver */
							rr_node[to_node].C += Cout;
						}
						isblock = seg_index_of_sblock(inode, to_node);
						buffer_Cin[isblock] = max(buffer_Cin[isblock], Cin);
					}

				}
				/* End edge to CHANX or CHANY node. */
				else if (to_rr_type == IPIN) {

					/* Code below implements sharing of the track to connection     *
					 * box buffer.  I assume there is one such buffer at every      *
					 * segment of the wire at which at least one logic block input  *
					 * connects.                                                    */

					icblock = seg_index_of_cblock(from_rr_type, to_node);
					if (cblock_counted[icblock] == FALSE) {
						rr_node[inode].C += C_ipin_cblock;
						cblock_counted[icblock] = TRUE;
					}
				}
			} /* End loop over all edges of a node. */

			/* Reset the cblock_counted and buffer_Cin arrays, and add buf Cin. */

			/* Method below would be faster for very unpopulated segments, but I  *
			 * think it would be slower overall for most FPGAs, so commented out. */

			/*   for (iedge=0;iedge<rr_node[inode].num_edges;iedge++) {
			 * to_node = rr_node[inode].edges[iedge];
			 * if (rr_node[to_node].type == IPIN) {
			 * icblock = seg_index_of_cblock (from_rr_type, to_node);
			 * cblock_counted[icblock] = FALSE;
			 * }
			 * }     */

			if (from_rr_type == CHANX) {
				iseg_low = rr_node[inode].xlow;
				iseg_high = rr_node[inode].xhigh;
			} else { /* CHANY */
				iseg_low = rr_node[inode].ylow;
				iseg_high = rr_node[inode].yhigh;
			}

			for (icblock = iseg_low; icblock <= iseg_high; icblock++) {
				cblock_counted[icblock] = FALSE;
			}

			for (isblock = iseg_low - 1; isblock <= iseg_high; isblock++) {
				rr_node[inode].C += buffer_Cin[isblock]; /* Biggest buf Cin at loc */
				buffer_Cin[isblock] = 0.;
			}

		}
		/* End node is CHANX or CHANY */
		else if (from_rr_type == OPIN) {

			for (iedge = 0; iedge < rr_node[inode].num_edges; iedge++) {
				switch_index = rr_node[inode].switches[iedge];
				/* UDSD by ICK Start */
				to_node = rr_node[inode].edges[iedge];
				to_rr_type = rr_node[to_node].type;

				if (to_rr_type != CHANX && to_rr_type != CHANY)
					continue;

				if (rr_node[to_node].drivers != SINGLE) {
					Cout = switch_inf[switch_index].Cout;
					to_node = rr_node[inode].edges[iedge]; /* Will be CHANX or CHANY or IPIN */
					rr_node[to_node].C += Cout;
				}
			}
		}
		/* End node is OPIN. */
	} /* End for all nodes. */

	/* Now we need to add any cout loads for nets that we previously didn't process
	 * Current structures only keep switch information from a node to the next node and
	 * not the reverse.  Therefore I need to go through all the possible edges to figure 
	 * out what the Cout's should be */
	Couts_to_add = (float *) my_calloc(num_rr_nodes, sizeof(float));
	for (inode = 0; inode < num_rr_nodes; inode++) {
		for (iedge = 0; iedge < rr_node[inode].num_edges; iedge++) {
			switch_index = rr_node[inode].switches[iedge];
			to_node = rr_node[inode].edges[iedge];
			to_rr_type = rr_node[to_node].type;
			if (to_rr_type == CHANX || to_rr_type == CHANY) {
				if (rr_node[to_node].drivers == SINGLE) {
					/* Cout was not added in these cases */
					if (Couts_to_add[to_node] != 0) {
						/* We've already found a Cout to add to this node
						 * We could take the max of all possibilities but
						 * instead I will fail if there are conflicting Couts */
						if (Couts_to_add[to_node]
								!= switch_inf[switch_index].Cout) {
							vpr_throw(VPR_ERROR_ROUTE, __FILE__,__LINE__,
								"A single driver resource (%i) is driven by different Cout's (%e!=%e)\n",
									to_node, Couts_to_add[to_node],
									switch_inf[switch_index].Cout);
						}
					}
					Couts_to_add[to_node] = switch_inf[switch_index].Cout;

				}
			}
		}
	}
	for (inode = 0; inode < num_rr_nodes; inode++) {
		rr_node[inode].C += Couts_to_add[inode];
	}
	free(Couts_to_add);
	free(cblock_counted);
	free(buffer_Cin);
}
