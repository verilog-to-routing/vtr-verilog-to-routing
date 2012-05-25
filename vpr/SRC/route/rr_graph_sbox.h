#ifndef RR_GRAPH_SBOX_H
#define RR_GRAPH_SBOX_H

struct s_ivec get_switch_box_tracks(INP int from_i,
		INP int from_j,
		INP int from_track,
		INP t_rr_type from_type,
		INP int to_i,
		INP int to_j,
		INP t_rr_type to_type,
		INP struct s_ivec ***switch_block_conn);

void free_switch_block_conn(struct s_ivec ***switch_block_conn,
		int nodes_per_chan);

struct s_ivec ***alloc_and_load_switch_block_conn(int nodes_per_chan,
		enum e_switch_block_type switch_block_type, int Fs);

int get_simple_switch_block_track(enum e_side from_side, enum e_side to_side,
		int from_track, enum e_switch_block_type switch_block_type,
		int nodes_per_chan);

#endif

