#ifndef COST_INDICES_H
#define COST_INDICES_H

///@brief Index of the SOURCE, SINK, OPIN, IPIN, etc. member of device_ctx.rr_indexed_data.
enum e_cost_indices {
    SOURCE_COST_INDEX = 0,
    SINK_COST_INDEX,
    OPIN_COST_INDEX,
    IPIN_COST_INDEX,
    CHANX_COST_INDEX_START
};

#endif