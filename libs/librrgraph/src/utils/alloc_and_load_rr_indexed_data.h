#ifndef ALLOC_AND_LOAD_RR_INDEXED_DATA_H
#define ALLOC_AND_LOAD_RR_INDEXED_DATA_H

#include "physical_types.h"

void alloc_and_load_rr_indexed_data(const std::vector<t_segment_inf>& segment_inf,
                                    const std::vector<t_segment_inf>& segment_inf_x,
                                    const std::vector<t_segment_inf>& segment_inf_y,
                                    int wire_to_ipin_switch,
                                    enum e_base_cost_type base_cost_type);

#endif