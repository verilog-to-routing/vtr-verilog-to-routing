#pragma once

enum e_base_cost_type {
    DELAY_NORMALIZED,
    DELAY_NORMALIZED_LENGTH,
    DELAY_NORMALIZED_FREQUENCY,
    DELAY_NORMALIZED_LENGTH_FREQUENCY,
    DELAY_NORMALIZED_LENGTH_BOUNDED,
    DEMAND_ONLY,
    DEMAND_ONLY_NORMALIZED_LENGTH
};

///@brief Index of the SOURCE, SINK, OPIN, IPIN, etc. member of device_ctx.rr_indexed_data.
enum e_cost_indices {
    SOURCE_COST_INDEX = 0,
    SINK_COST_INDEX,
    MUX_COST_INDEX,
    OPIN_COST_INDEX,
    IPIN_COST_INDEX,
    CHANX_COST_INDEX_START
};
