#include "get_parallel_segs.h"

/*Gets t_segment_inf for parallel segments as defined by the user. 
 *Segments that have BOTH_AXIS attribute value are always included in the returned vector.*/
std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             enum e_parallel_axis parallel_axis) {
    std::vector<t_segment_inf> result;
    for (size_t i = 0; i < segment_inf.size(); ++i) {
        if (segment_inf[i].parallel_axis == parallel_axis || segment_inf[i].parallel_axis == BOTH_AXIS) {
            result.push_back(segment_inf[i]);
            result[result.size() - 1].seg_index = i;
            seg_index_map.insert(std::make_pair(i, std::make_pair(result.size() - 1, parallel_axis)));
        }
    }
    return result;
}