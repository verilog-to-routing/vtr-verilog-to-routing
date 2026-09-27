#pragma once

#include <limits>
#include "librrgraph_types.h"

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

/**
 * @brief Cost data shared by all RR nodes with the same cost_index.
 *
 * Stores the base_cost and fields that take only a few distinct values
 * (like seg_index) or that are averaged over all RR nodes of a type like T_linear.
 */
struct t_rr_indexed_data {
    /// The basic cost of using an RR node.
    float base_cost = std::numeric_limits<float>::quiet_NaN();
    /// Copy of base_cost kept while it is temporarily overridden.
    float saved_base_cost = std::numeric_limits<float>::quiet_NaN();
    /// Cost index of the RR node type that generally connects to this one but runs in the orthogonal direction.
    int ortho_cost_index = LIBRRGRAPH_UNDEFINED_VAL;
    /// Index into segment_inf if this type is a CHANX or CHANY. LIBRRGRAPH_UNDEFINED_VAL (-1) otherwise.
    int seg_index = LIBRRGRAPH_UNDEFINED_VAL;
    /// 1 / length of this segment type.
    float inv_length = std::numeric_limits<float>::quiet_NaN();
    /// Delay through N segments is N * T_linear + N^2 * T_quadratic. Buffered segments have only T_linear.
    float T_linear = std::numeric_limits<float>::quiet_NaN();
    /// Dominant delay for unbuffered segments. 0 for buffered segments.
    float T_quadratic = std::numeric_limits<float>::quiet_NaN();
    /// Load capacitance the driver sees for each segment added to the chain. 0 for buffered segments.
    float C_load = std::numeric_limits<float>::quiet_NaN();
};
