#include <cstdio>
using namespace std;

#include "vtr_log.h"
#include "vtr_memory.h"

#include "vpr_types.h"
#include "globals.h"
#include "segment_stats.h"

/*************** Variables and defines local to this module ****************/

#define LONGLINE 0

/******************* Subroutine definitions ********************************/

void get_segment_usage_stats(std::vector<t_segment_inf>& segment_inf) {
    /* Computes statistics on the fractional utilization of segments by type    *
     * (index) and by length.  This routine needs a valid rr_graph, and a       *
     * completed routing.  Note that segments cut off by the end of the array   *
     * are counted as full-length segments (e.g. length 4 even if the last 2    *
     * units of wire were chopped off by the chip edge).                        */

    int length, max_segment_length, cost_index;
    int *seg_occ_by_length, *seg_cap_by_length; /* [0..max_segment_length] */
    int *seg_occ_by_type, *seg_cap_by_type;     /* [0..num_segment-1]      */
    float utilization;

    auto& device_ctx = g_vpr_ctx.device();
    auto& route_ctx = g_vpr_ctx.routing();

    max_segment_length = 0;
    for (size_t seg_type = 0; seg_type < segment_inf.size(); seg_type++) {
        if (segment_inf[seg_type].longline == false)
            max_segment_length = max(max_segment_length,
                                     segment_inf[seg_type].length);
    }

    seg_occ_by_length = (int*)vtr::calloc((max_segment_length + 1),
                                          sizeof(int));
    seg_cap_by_length = (int*)vtr::calloc((max_segment_length + 1),
                                          sizeof(int));

    seg_occ_by_type = (int*)vtr::calloc(segment_inf.size(), sizeof(int));
    seg_cap_by_type = (int*)vtr::calloc(segment_inf.size(), sizeof(int));

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        if (device_ctx.rr_nodes[inode].type() == CHANX || device_ctx.rr_nodes[inode].type() == CHANY) {
            cost_index = device_ctx.rr_nodes[inode].cost_index();
            size_t seg_type = device_ctx.rr_indexed_data[cost_index].seg_index;

            if (!segment_inf[seg_type].longline)
                length = segment_inf[seg_type].length;
            else
                length = LONGLINE;

            seg_occ_by_length[length] += route_ctx.rr_node_route_inf[inode].occ();
            seg_cap_by_length[length] += device_ctx.rr_nodes[inode].capacity();
            seg_occ_by_type[seg_type] += route_ctx.rr_node_route_inf[inode].occ();
            seg_cap_by_type[seg_type] += device_ctx.rr_nodes[inode].capacity();
        }
    }

    VTR_LOG("\n");
    VTR_LOG("Segment usage by type (index): type utilization\n");
    VTR_LOG("                               ---- -----------\n");

    for (size_t seg_type = 0; seg_type < segment_inf.size(); seg_type++) {
        if (seg_cap_by_type[seg_type] != 0) {
            utilization = (float)seg_occ_by_type[seg_type] / (float)seg_cap_by_type[seg_type];
            VTR_LOG("                               %4d %11.3g\n", seg_type, utilization);
        }
    }

    VTR_LOG("\n");
    VTR_LOG("Segment usage by length: length utilization\n");
    VTR_LOG("                         ------ -----------\n");

    for (length = 1; length <= max_segment_length; length++) {
        if (seg_cap_by_length[length] != 0) {
            utilization = (float)seg_occ_by_length[length] / (float)seg_cap_by_length[length];
            VTR_LOG("                         %6d %11.3g\n", length, utilization);
        }
    }
    VTR_LOG("\n");

    if (seg_cap_by_length[LONGLINE] != 0) {
        utilization = (float)seg_occ_by_length[LONGLINE] / (float)seg_cap_by_length[LONGLINE];
        VTR_LOG("   longline                 %5.3g\n", utilization);
    }

    free(seg_occ_by_length);
    free(seg_cap_by_length);
    free(seg_occ_by_type);
    free(seg_cap_by_type);
}
