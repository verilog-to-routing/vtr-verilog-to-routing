#ifndef ALLOC_AND_LOAD_RR_INDEXED_DATA_H
#define ALLOC_AND_LOAD_RR_INDEXED_DATA_H

#include "rr_graph_view.h"
#include "rr_node.h"
#include "rr_graph_cost.h"
#include "device_grid.h"

void alloc_and_load_rr_indexed_data(const RRGraphView& rr_graph,
                                    const DeviceGrid& grid,
                                    const std::vector<t_segment_inf>& segment_inf,
                                    const std::vector<t_segment_inf>& segment_inf_x,
                                    const std::vector<t_segment_inf>& segment_inf_y,
                                    vtr::vector<RRIndexedDataId, t_rr_indexed_data>& rr_indexed_data,
                                    int wire_to_ipin_switch,
                                    enum e_base_cost_type base_cost_type,
                                    const bool echo_enabled,
                                    const char* echo_file_name);

std::vector<int> find_ortho_cost_index(const RRGraphView& rr_graph,
                                       const std::vector<t_segment_inf> segment_inf_x,
                                       const std::vector<t_segment_inf> segment_inf_y,
                                       e_parallel_axis parallel_axis); 

#endif