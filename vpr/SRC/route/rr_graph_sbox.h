#ifndef RR_GRAPH_SBOX_H
#define RR_GRAPH_SBOX_H

#include "vtr_matrix.h"

vtr::t_ivec get_switch_box_tracks(const int from_i,
		const int from_j,
		const int from_track,
		const t_rr_type from_type,
		const int to_i,
		const int to_j,
		const t_rr_type to_type,
		const vtr::t_ivec ***switch_block_conn);

void free_switch_block_conn(vtr::t_ivec ***switch_block_conn,
		int nodes_per_chan);

vtr::t_ivec ***alloc_and_load_switch_block_conn(int nodes_per_chan,
		enum e_switch_block_type switch_block_type, int Fs);

int get_simple_switch_block_track(enum e_side from_side, enum e_side to_side,
		int from_track, enum e_switch_block_type switch_block_type,
		int nodes_per_chan);

#endif

