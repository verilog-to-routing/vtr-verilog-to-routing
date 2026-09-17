#pragma once

#include <limits>
#include "librrgraph_types.h"

/* Data that is pointed to by the cost_index of an RR node.  It's            *
 * purpose is to store the base_cost so that it can be quickly changed       *
 * and to store fields that have only a few different values (like           *
 * seg_index) or whose values should be an average over all rr_nodes of a    *
 * certain type (like T_linear etc., which are used to predict remaining     *
 * delay in the timing_driven router).                                       *
 *                                                                           *
 * base_cost:  The basic cost of using an rr_node.                           *
 * ortho_cost_index:  The index of the type of rr_node that generally        *
 *                    connects to this type of rr_node, but runs in the      *
 *                    orthogonal direction (e.g. vertical if the direction   *
 *                    of this member is horizontal).                         *
 * seg_index:  Index into segment_inf of this segment type if this type of   *
 *             rr_node is an CHANX or CHANY; OPEN (-1) otherwise.            *
 * inv_length:  1/length of this type of segment.                            *
 * T_linear:  Delay through N segments of this type is N * T_linear + N^2 *  *
 *            T_quadratic.  For buffered segments all delay is T_linear.     *
 * T_quadratic:  Dominant delay for unbuffered segments, 0 for buffered      *
 *               segments.                                                   *
 * C_load:  Load capacitance seen by the driver for each segment added to    *
 *          the chain driven by the driver.  0 for buffered segments.        */

struct t_rr_indexed_data {
    float base_cost = std::numeric_limits<float>::quiet_NaN();
    float saved_base_cost = std::numeric_limits<float>::quiet_NaN();
    int ortho_cost_index = LIBRRGRAPH_UNDEFINED_VAL;
    int seg_index = LIBRRGRAPH_UNDEFINED_VAL;
    float inv_length = std::numeric_limits<float>::quiet_NaN();
    float T_linear = std::numeric_limits<float>::quiet_NaN();
    float T_quadratic = std::numeric_limits<float>::quiet_NaN();
    float C_load = std::numeric_limits<float>::quiet_NaN();
};
