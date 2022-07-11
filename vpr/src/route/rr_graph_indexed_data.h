#ifndef RR_GRAPH_INDEXED_DATA_H
#define RR_GRAPH_INDEXED_DATA_H
#include "physical_types.h"

void alloc_and_load_rr_indexed_data(const std::vector<t_segment_inf>& segment_inf,
                                    const std::vector<t_segment_inf>& segment_inf_x,
                                    const std::vector<t_segment_inf>& segment_inf_y,
                                    int wire_to_ipin_switch,
                                    enum e_base_cost_type base_cost_type);

void load_rr_index_segments(const int num_segment);

std::vector<int> find_ortho_cost_index(const std::vector<t_segment_inf> segment_inf_x,
                                       const std::vector<t_segment_inf> segment_inf_y,
                                       e_parallel_axis parallel_axis);

#endif
