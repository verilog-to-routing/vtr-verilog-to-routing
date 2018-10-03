#ifndef RR_GRAPH_INDEXED_DATA_H
#define RR_GRAPH_INDEXED_DATA_H
#include "physical_types.h"

void alloc_and_load_rr_indexed_data(const t_segment_inf * segment_inf,
		int num_segment, const t_rr_node_indices& L_rr_node_indices, int nodes_per_chan,
		int wire_to_ipin_switch, enum e_base_cost_type base_cost_type);

void alloc_and_load_rr_indexed_data_for_segments(
        const t_segment_inf * segment_inf,
        const int start_index,
        const int start_seg_index,
        const int num_x_segments,
        const int num_y_segments,
        const t_rr_node_indices& L_rr_node_indices,
        const int nodes_per_chan,
        enum e_base_cost_type base_cost_type);

void load_rr_index_segments(const int num_segment);

#endif
