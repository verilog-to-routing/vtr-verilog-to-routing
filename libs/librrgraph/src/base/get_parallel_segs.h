#ifndef GET_PARALLEL_SEGS_H
#define GET_PARALLEL_SEGS_H

#include "rr_graph_type.h"
#include "physical_types.h"

/**
 * @brief Returns segments aligned with a given axis, including BOTH_AXIS segments.
 *
 * Filters the unified segment list (`segment_inf`) to include only segments matching
 * the specified `parallel_axis` or marked as `BOTH_AXIS`. Also populates `seg_index_map`
 * to map unified indices to axis-specific ones.
 *
 * @param segment_inf    Unified list of all segments.
 * @param seg_index_map  Map from unified to axis-specific segment indices.
 * @param parallel_axis  Axis to filter segments by.
 * @return Filtered list of segments for the given axis.
 */
std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             enum e_parallel_axis parallel_axis);

#endif