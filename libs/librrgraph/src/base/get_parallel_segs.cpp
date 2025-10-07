#include "get_parallel_segs.h"

std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             e_parallel_axis parallel_axis,
                                             bool keep_original_index /* = false */) {
    std::vector<t_segment_inf> result;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        e_parallel_axis seg_axis = segment_inf[i].parallel_axis;

        // Keep this segment if:
        //   1. Its axis exactly matches the requested parallel_axis, OR
        //   2. The requested axis is X or Y (not Z), and the segment is marked BOTH_AXIS.
        if (seg_axis == parallel_axis
            || (parallel_axis != e_parallel_axis::Z_AXIS && seg_axis == e_parallel_axis::BOTH_AXIS)) {
            result.push_back(segment_inf[i]);
            if (!keep_original_index) {
                result.back().seg_index = i;
            }
            seg_index_map.insert(std::make_pair(i, std::make_pair(result.size() - 1, parallel_axis)));
        }
    }
    return result;
}
