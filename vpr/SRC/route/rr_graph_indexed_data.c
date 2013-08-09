#include <cmath>		/* Needed only for sqrt call (remove if sqrt removed) */
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_indexed_data.h"
#include "read_xml_arch_file.h"

/******************* Subroutines local to this module ************************/

static void load_rr_indexed_data_base_costs(int nodes_per_chan,
		t_ivec *** L_rr_node_indices, enum e_base_cost_type base_cost_type,
		int wire_to_ipin_switch);

static float get_delay_normalization_fac(int nodes_per_chan,
		t_ivec *** L_rr_node_indices);

static float get_average_opin_delay(t_ivec *** L_rr_node_indices,
		int nodes_per_chan);

static void load_rr_indexed_data_T_values(int index_start,
		int num_indices_to_load, t_rr_type rr_type, int nodes_per_chan,
		t_ivec *** L_rr_node_indices, t_segment_inf * segment_inf);

/******************** Subroutine definitions *********************************/

/* Allocates the rr_indexed_data array and loads it with appropriate values. *
 * It currently stores the segment type (or OPEN if the index doesn't        *
 * correspond to an CHANX or CHANY type), the base cost of nodes of that     *
 * type, and some info to allow rapid estimates of time to get to a target   *
 * to be computed by the router.                                             *
 *
 * Right now all SOURCES have the same base cost; and similarly there's only *
 * one base cost for each of SINKs, OPINs, and IPINs (four total).  This can *
 * be changed just by allocating more space in the array below and changing  *
 * the cost_index values for these rr_nodes, if you want to make some pins   *
 * etc. more expensive than others.  I give each segment type in an          *
 * x-channel its own cost_index, and each segment type in a y-channel its    *
 * own cost_index.                                                           */
void alloc_and_load_rr_indexed_data(INP t_segment_inf * segment_inf,
		INP int num_segment, INP t_ivec *** L_rr_node_indices,
		INP int nodes_per_chan, int wire_to_ipin_switch,
		enum e_base_cost_type base_cost_type) {

	int iseg, length, i, index;

	num_rr_indexed_data = CHANX_COST_INDEX_START + (2 * num_segment);
	rr_indexed_data = (t_rr_indexed_data *) my_malloc(
			num_rr_indexed_data * sizeof(t_rr_indexed_data));

	/* For rr_types that aren't CHANX or CHANY, base_cost is valid, but most     *
	 * * other fields are invalid.  For IPINs, the T_linear field is also valid;   *
	 * * all other fields are invalid.  For SOURCES, SINKs and OPINs, all fields   *
	 * * other than base_cost are invalid. Mark invalid fields as OPEN for safety. */

	for (i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
		rr_indexed_data[i].ortho_cost_index = OPEN;
		rr_indexed_data[i].seg_index = OPEN;
		rr_indexed_data[i].inv_length = OPEN;
		rr_indexed_data[i].T_linear = OPEN;
		rr_indexed_data[i].T_quadratic = OPEN;
		rr_indexed_data[i].C_load = OPEN;
	}

	rr_indexed_data[IPIN_COST_INDEX].T_linear =
			switch_inf[wire_to_ipin_switch].Tdel;

	/* X-directed segments. */

	for (iseg = 0; iseg < num_segment; iseg++) {
		index = CHANX_COST_INDEX_START + iseg;

		rr_indexed_data[index].ortho_cost_index = index + num_segment;

		if (segment_inf[iseg].longline)
			length = nx;
		else
			length = min(segment_inf[iseg].length, nx);

		rr_indexed_data[index].inv_length = 1. / length;
		rr_indexed_data[index].seg_index = iseg;
	}

	load_rr_indexed_data_T_values(CHANX_COST_INDEX_START, num_segment, CHANX,
			nodes_per_chan, L_rr_node_indices, segment_inf);

	/* Y-directed segments. */

	for (iseg = 0; iseg < num_segment; iseg++) {
		index = CHANX_COST_INDEX_START + num_segment + iseg;

		rr_indexed_data[index].ortho_cost_index = index - num_segment;

		if (segment_inf[iseg].longline)
			length = ny;
		else
			length = min(segment_inf[iseg].length, ny);

		rr_indexed_data[index].inv_length = 1. / length;
		rr_indexed_data[index].seg_index = iseg;
	}

	load_rr_indexed_data_T_values((CHANX_COST_INDEX_START + num_segment),
			num_segment, CHANY, nodes_per_chan, L_rr_node_indices, segment_inf);

	load_rr_indexed_data_base_costs(nodes_per_chan, L_rr_node_indices,
			base_cost_type, wire_to_ipin_switch);

}

static void load_rr_indexed_data_base_costs(int nodes_per_chan,
		t_ivec *** L_rr_node_indices, enum e_base_cost_type base_cost_type,
		int wire_to_ipin_switch) {

	/* Loads the base_cost member of rr_indexed_data according to the specified *
	 * base_cost_type.                                                          */

	float delay_normalization_fac;
	int index;

	if (base_cost_type == DELAY_NORMALIZED) {
		delay_normalization_fac = get_delay_normalization_fac(nodes_per_chan,
				L_rr_node_indices);
	} else {
		delay_normalization_fac = 1.;
	}

	if (base_cost_type == DEMAND_ONLY || base_cost_type == DELAY_NORMALIZED) {
		rr_indexed_data[SOURCE_COST_INDEX].base_cost = delay_normalization_fac;
		rr_indexed_data[SINK_COST_INDEX].base_cost = 0.;
		rr_indexed_data[OPIN_COST_INDEX].base_cost = delay_normalization_fac;

#ifndef SPEC
		rr_indexed_data[IPIN_COST_INDEX].base_cost = 0.95
				* delay_normalization_fac;
#else /* Avoid roundoff for SPEC */
		rr_indexed_data[IPIN_COST_INDEX].base_cost =
		delay_normalization_fac;
#endif
	}

	else if (base_cost_type == INTRINSIC_DELAY) {
		rr_indexed_data[SOURCE_COST_INDEX].base_cost = 0.;
		rr_indexed_data[SINK_COST_INDEX].base_cost = 0.;
		rr_indexed_data[OPIN_COST_INDEX].base_cost = get_average_opin_delay(
				L_rr_node_indices, nodes_per_chan);
		rr_indexed_data[IPIN_COST_INDEX].base_cost =
				switch_inf[wire_to_ipin_switch].Tdel;
	}

	/* Load base costs for CHANX and CHANY segments */

	for (index = CHANX_COST_INDEX_START; index < num_rr_indexed_data; index++) {
		if (base_cost_type == INTRINSIC_DELAY)
			rr_indexed_data[index].base_cost = rr_indexed_data[index].T_linear
					+ rr_indexed_data[index].T_quadratic;
		else
			/*       rr_indexed_data[index].base_cost = delay_normalization_fac /
			 rr_indexed_data[index].inv_length;  */

			rr_indexed_data[index].base_cost = delay_normalization_fac;
		/*       rr_indexed_data[index].base_cost = delay_normalization_fac *
		 sqrt (1. / rr_indexed_data[index].inv_length);  */
		/*       rr_indexed_data[index].base_cost = delay_normalization_fac *
		 (1. + 1. / rr_indexed_data[index].inv_length);  */
	}

	/* Save a copy of the base costs -- if dynamic costing is used by the     * 
	 * router, the base_cost values will get changed all the time and being   *
	 * able to restore them from a saved version is useful.                   */

	for (index = 0; index < num_rr_indexed_data; index++) {
		rr_indexed_data[index].saved_base_cost =
				rr_indexed_data[index].base_cost;
	}
}

static float get_delay_normalization_fac(int nodes_per_chan,
		t_ivec *** L_rr_node_indices) {

	/* Returns the average delay to go 1 CLB distance along a wire.  */

	const int clb_dist = 3; /* Number of CLBs I think the average conn. goes. */

	int inode, itrack, cost_index;
	float Tdel, Tdel_sum, frac_num_seg;

	Tdel_sum = 0.;

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		inode = find_average_rr_node_index(nx, ny, CHANX, itrack, 
				L_rr_node_indices);
		if (inode == -1)
			continue;
		cost_index = rr_node[inode].cost_index;
		frac_num_seg = clb_dist * rr_indexed_data[cost_index].inv_length;
		Tdel = frac_num_seg * rr_indexed_data[cost_index].T_linear
				+ frac_num_seg * frac_num_seg
						* rr_indexed_data[cost_index].T_quadratic;
		Tdel_sum += Tdel / (float) clb_dist;
	}

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		inode = find_average_rr_node_index(nx, ny, CHANY, itrack, 
				L_rr_node_indices);
		if (inode == -1)
			continue;
		cost_index = rr_node[inode].cost_index;
		frac_num_seg = clb_dist * rr_indexed_data[cost_index].inv_length;
		Tdel = frac_num_seg * rr_indexed_data[cost_index].T_linear
				+ frac_num_seg * frac_num_seg
						* rr_indexed_data[cost_index].T_quadratic;
		Tdel_sum += Tdel / (float) clb_dist;
	}

	return (Tdel_sum / (2. * nodes_per_chan));
}

static float get_average_opin_delay(t_ivec *** L_rr_node_indices,
		int nodes_per_chan) {

	/* Returns the average delay from an OPIN to a wire in an adjacent channel. */
	/* RESEARCH TODO: Got to think if this heuristic needs to change for hetero, right now, I'll calculate
	 * the average delay of non-IO blocks */
	int inode, ipin, iclass, iedge, itype, num_edges, to_switch, to_node,
			num_conn;
	float Cload, Tdel;

	Tdel = 0.;
	num_conn = 0;
	for (itype = 0; itype < num_types && &type_descriptors[itype] != IO_TYPE;
			itype++) {
		for (ipin = 0; ipin < type_descriptors[itype].num_pins; ipin++) {
			iclass = type_descriptors[itype].pin_class[ipin];
			if (type_descriptors[itype].class_inf[iclass].type == DRIVER) { /* OPIN */
				inode = find_average_rr_node_index(nx, ny, OPIN,
						ipin, L_rr_node_indices);
				if (inode == -1)
					continue;
				num_edges = rr_node[inode].num_edges;

				for (iedge = 0; iedge < num_edges; iedge++) {
					to_node = rr_node[inode].edges[iedge];
					to_switch = rr_node[inode].switches[iedge];
					Cload = rr_node[to_node].C;
					Tdel += Cload * switch_inf[to_switch].R
							+ switch_inf[to_switch].Tdel;
					num_conn++;
				}
			}
		}
	}

	Tdel /= (float) num_conn;
	return (Tdel);
}

static void load_rr_indexed_data_T_values(int index_start,
		int num_indices_to_load, t_rr_type rr_type, int nodes_per_chan,
		t_ivec *** L_rr_node_indices, t_segment_inf * segment_inf) {

	/* Loads the average propagation times through segments of each index type  *
	 * for either all CHANX segment types or all CHANY segment types.  It does  *
	 * this by looking at all the segments in one channel in the middle of the  *
	 * array and averaging the R and C values of all segments of the same type  *
	 * and using them to compute average delay values for this type of segment. */

	int itrack, iseg, inode, cost_index, iswitch;
	float *C_total, *R_total; /* [0..num_rr_indexed_data - 1] */
	int *num_nodes_of_index; /* [0..num_rr_indexed_data - 1] */
	float Rnode, Cnode, Rsw, Tsw;

	num_nodes_of_index = (int *) my_calloc(num_rr_indexed_data, sizeof(int));
	C_total = (float *) my_calloc(num_rr_indexed_data, sizeof(float));
	R_total = (float *) my_calloc(num_rr_indexed_data, sizeof(float));

	/* Get average C and R values for all the segments of this type in one      *
	 * channel segment, near the middle of the array.                           */

	for (itrack = 0; itrack < nodes_per_chan; itrack++) {
		inode = find_average_rr_node_index(nx, ny, rr_type, itrack,
				L_rr_node_indices);
		if (inode == -1)
			continue;
		cost_index = rr_node[inode].cost_index;
		num_nodes_of_index[cost_index]++;
		C_total[cost_index] += rr_node[inode].C;
		R_total[cost_index] += rr_node[inode].R;
	}

	for (cost_index = index_start;
			cost_index < index_start + num_indices_to_load; cost_index++) {

		if (num_nodes_of_index[cost_index] == 0) { /* Segments don't exist. */
			rr_indexed_data[cost_index].T_linear = OPEN;
			rr_indexed_data[cost_index].T_quadratic = OPEN;
			rr_indexed_data[cost_index].C_load = OPEN;
		} else {
			Rnode = R_total[cost_index] / num_nodes_of_index[cost_index];
			Cnode = C_total[cost_index] / num_nodes_of_index[cost_index];
			iseg = rr_indexed_data[cost_index].seg_index;
			iswitch = segment_inf[iseg].wire_switch;
			Rsw = switch_inf[iswitch].R;
			Tsw = switch_inf[iswitch].Tdel;

			if (switch_inf[iswitch].buffered) {
				rr_indexed_data[cost_index].T_linear = Tsw + Rsw * Cnode
						+ 0.5 * Rnode * Cnode;
				rr_indexed_data[cost_index].T_quadratic = 0.;
				rr_indexed_data[cost_index].C_load = 0.;
			} else { /* Pass transistor */
				rr_indexed_data[cost_index].C_load = Cnode;

				/* See Dec. 23, 1997 notes for deriviation of formulae. */

				rr_indexed_data[cost_index].T_linear = Tsw + 0.5 * Rsw * Cnode;
				rr_indexed_data[cost_index].T_quadratic = (Rsw + Rnode) * 0.5
						* Cnode;
			}
		}
	}

	free(num_nodes_of_index);
	free(C_total);
	free(R_total);
}
