#include "globals.h"
#include "rr_graph_indexed_data.h"

void load_rr_index_segments(const int num_segment) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    int iseg, i, index;

    for (i = SOURCE_COST_INDEX; i <= IPIN_COST_INDEX; i++) {
        device_ctx.rr_indexed_data[RRIndexedDataId(i)].seg_index = OPEN;
    }

    /* X-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + iseg;
        device_ctx.rr_indexed_data[RRIndexedDataId(index)].seg_index = iseg;
    }
    /* Y-directed segments. */
    for (iseg = 0; iseg < num_segment; iseg++) {
        index = CHANX_COST_INDEX_START + num_segment + iseg;
        device_ctx.rr_indexed_data[RRIndexedDataId(index)].seg_index = iseg;
    }
}