#include <cstdio>
using namespace std;

#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "segment_stats.h"

/*************** Variables and defines local to this module ****************/

#define LONGLINE 0

/******************* Subroutine definitions ********************************/

void get_segment_usage_stats(int num_segment, t_segment_inf * segment_inf) {

	/* Computes statistics on the fractional utilization of segments by type    *
	 * (index) and by length.  This routine needs a valid rr_graph, and a       *
	 * completed routing.  Note that segments cut off by the end of the array   *
	 * are counted as full-length segments (e.g. length 4 even if the last 2    *
	 * units of wire were chopped off by the chip edge).                        */

	int inode, length, seg_type, max_segment_length, cost_index;
	int *seg_occ_by_length, *seg_cap_by_length; /* [0..max_segment_length] */
	int *seg_occ_by_type, *seg_cap_by_type; /* [0..num_segment-1]      */
	float utilization;

	max_segment_length = 0;
	for (seg_type = 0; seg_type < num_segment; seg_type++) {
		if (segment_inf[seg_type].longline == FALSE)
			max_segment_length = max(max_segment_length,
					segment_inf[seg_type].length);
	}

	seg_occ_by_length = (int *) my_calloc((max_segment_length + 1),
			sizeof(int));
	seg_cap_by_length = (int *) my_calloc((max_segment_length + 1),
			sizeof(int));

	seg_occ_by_type = (int *) my_calloc(num_segment, sizeof(int));
	seg_cap_by_type = (int *) my_calloc(num_segment, sizeof(int));

	for (inode = 0; inode < num_rr_nodes; inode++) {
		if (rr_node[inode].type == CHANX || rr_node[inode].type == CHANY) {
			cost_index = rr_node[inode].cost_index;
			seg_type = rr_indexed_data[cost_index].seg_index;

			if (!segment_inf[seg_type].longline)
				length = segment_inf[seg_type].length;
			else
				length = LONGLINE;

			seg_occ_by_length[length] += rr_node[inode].occ;
			seg_cap_by_length[length] += rr_node[inode].capacity;
			seg_occ_by_type[seg_type] += rr_node[inode].occ;
			seg_cap_by_type[seg_type] += rr_node[inode].capacity;

		}
	}

	vpr_printf_info("\n");
	vpr_printf_info("Segment usage by type (index): type utilization\n");
	vpr_printf_info("                               ---- -----------\n");

	for (seg_type = 0; seg_type < num_segment; seg_type++) {
		if (seg_cap_by_type[seg_type] != 0) {
			utilization = (float) seg_occ_by_type[seg_type] / (float) seg_cap_by_type[seg_type];
			vpr_printf_info("                               %4d %11.3g\n", seg_type, utilization);
		}
	}

	vpr_printf_info("\n");
	vpr_printf_info("Segment usage by length: length utilization\n");
	vpr_printf_info("                         ------ -----------\n");

	for (length = 1; length <= max_segment_length; length++) {
		if (seg_cap_by_length[length] != 0) {
			utilization = (float) seg_occ_by_length[length] / (float) seg_cap_by_length[length];
			vpr_printf_info("                         %6d %11.3g\n", length, utilization);
		}
	}
	vpr_printf_info("\n");

	if (seg_cap_by_length[LONGLINE] != 0) {
		utilization = (float) seg_occ_by_length[LONGLINE] / (float) seg_cap_by_length[LONGLINE];
		vpr_printf_info("   longline                 %5.3g\n", utilization);
	}

	free(seg_occ_by_length);
	free(seg_cap_by_length);
	free(seg_occ_by_type);
	free(seg_cap_by_type);
}
