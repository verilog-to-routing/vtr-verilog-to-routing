#ifndef CHECK_RR_GRAPH_H
#define CHECK_RR_GRAPH_H

void check_rr_graph(INP t_graph_type graph_type,
		INP t_type_ptr types,
		INP int L_nx,
		INP int L_ny,
		INP int nodes_per_chan,
		INP int Fs,
		INP int num_seg_types,
		INP int num_switches,
		INP t_segment_inf * segment_inf,
		INP int global_route_switch,
		INP int delayless_switch,
		INP int wire_to_ipin_switch,
		t_seg_details * seg_details,
		int ** Fc_in,
		int ** Fc_out,
		int *****opin_to_track_map,
		int *****ipin_to_track_map,
		t_ivec **** track_to_ipin_lookup,
		t_ivec *** switch_block_conn,
		boolean * perturb_ipins);

void check_node(int inode, enum e_route_type route_type);

#endif

