#ifndef GET_PARALLEL_SEGS_H
#define GET_PARALLEL_SEGS_H

#include "rr_graph_type.h"
#include "physical_types.h"

std::vector<t_segment_inf> get_parallel_segs(const std::vector<t_segment_inf>& segment_inf,
                                             t_unified_to_parallel_seg_index& seg_index_map,
                                             enum e_parallel_axis parallel_axis);

#endif