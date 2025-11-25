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

/**
 * @brief Retrieves the segment index in an axis-specific segment list.
 *
 * @param abs_index     Index of the segment in the unified segment list.
 * @param index_map     Mapping from unified indices to (axis-specific index, axis).
 *                      This map must have been populated by `get_parallel_segs()`.
 * @param parallel_axis Axis to resolve ambiguous mappings for segments usable on multiple axes.
 *
 * @return The index of the segment in the axis-specific list if found, otherwise -1.
 */

int get_parallel_seg_index(int abs_index,
                           const t_unified_to_parallel_seg_index& index_map,
                           e_parallel_axis parallel_axis);