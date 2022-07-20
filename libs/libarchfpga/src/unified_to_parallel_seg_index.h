#ifndef UNIFIED_TO_PARALLEL_SEG_INDEX_H
#define UNIFIED_TO_PARALLEL_SEG_INDEX_H

#include "physical_types.h"

/* This map is used to get indices w.r.t segment_inf_x or segment_inf_y based on parallel_axis of a segment, 
 * from indices w.r.t the **unified** segment vector, segment_inf in devices context which stores all segments 
 * regardless of their axis. (see get_parallel_segs for more details)*/
typedef std::unordered_multimap<size_t, std::pair<size_t, e_parallel_axis>> t_unified_to_parallel_seg_index;

#endif