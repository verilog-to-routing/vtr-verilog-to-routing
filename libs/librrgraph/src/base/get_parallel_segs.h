#pragma once

#include "rr_graph_type.h"
#include "physical_types.h"

/**
 * @brief Filters segments aligned with a given axis and records index mappings.
 *
 * Iterates through the unified segment list (`segment_inf`) and selects segments
 * whose `parallel_axis` matches the requested `parallel_axis`. If the requested
 * axis is not `Z_AXIS`, segments marked as `BOTH_AXIS` are also included.
 *
 * @param segment_inf    Unified list of all segments.
 * @param seg_index_map  Map from unified to axis-specific segment indices.
 * @param parallel_axis  Axis to filter segments by.
 * @param keep_original_index  Whether to keep the original index of the segment. Currently,
 *                             it is only set to true when building the tileable rr_graph.
 * @return Filtered list of segments for the given axis.
 */
std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             e_parallel_axis parallel_axis,
                                             bool keep_original_index = false);
