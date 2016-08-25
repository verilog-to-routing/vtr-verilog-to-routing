#ifndef RR_GRAPH2_H
#define RR_GRAPH2_H

#include "build_switchblocks.h"

/************** Global variables shared only by the rr_* modules. ************/

extern bool *rr_edge_done; /* [0..num_rr_nodes-1].  Used to keep track  *
 * of whether or not a node has been put in  *
 * an edge list yet. true if a node is already listed in the edges array *
 * that's being constructed. Ensure that there are no duplicate edges.   */

/******************* Types shared by rr_graph2 functions *********************/

enum e_seg_details_type {
	SEG_DETAILS_X, SEG_DETAILS_Y
};

/******************* Subroutines exported by rr_graph2.c *********************/

vtr::t_ivec ***alloc_and_load_rr_node_indices(
		const int max_chan_width,
		const int L_nx, const int L_ny, 
		int *index, 
		const t_chan_details *chan_details_x,
		const t_chan_details *chan_details_y);

void free_rr_node_indices(vtr::t_ivec ***L_rr_node_indices);

int get_rr_node_index(
		int x, int y, 
		t_rr_type rr_type, int ptc,
		vtr::t_ivec ***L_rr_node_indices);

int find_average_rr_node_index(
		int L_nx, int L_ny,
		t_rr_type rr_type, int ptc,
		vtr::t_ivec ***L_rr_node_indices);

t_seg_details *alloc_and_load_seg_details(
		int *max_chan_width,
		const int max_len,
		const int num_seg_types,
		const t_segment_inf *segment_inf,
		const bool use_full_seg_groups,
		const bool is_global_graph,
		const enum e_directionality directionality,
		int *num_seg_details = 0);

void alloc_and_load_chan_details( 
		const int L_nx, const int L_ny,
		const t_chan_width *nodes_per_chan,
		const bool trim_empty_channels,
		const bool trim_obs_channels,
		const int num_seg_details,
		const t_seg_details *seg_details,
		t_chan_details **chan_details_x,
		t_chan_details **chan_details_y);
t_chan_details *init_chan_details( 
		const int L_nx, const int L_ny,
		const t_chan_width *nodes_per_chan,
		const int num_seg_details,
		const t_seg_details *seg_details,
		const enum e_seg_details_type seg_details_type);
void obstruct_chan_details( 
		const int L_nx, const int L_ny,
		const t_chan_width *nodes_per_chan,
		const bool trim_empty_channels,
		const bool trim_obs_channels,
		t_chan_details *chan_details_x,
		t_chan_details *chan_details_y);
void adjust_chan_details(
		const int L_nx, const int L_ny,
		const t_chan_width *nodes_per_chan,
		t_chan_details *chan_details_x,
		t_chan_details *chan_details_y);
void adjust_seg_details(
		const int x, const int y,
		const int L_nx, const int L_ny,
		const t_chan_width *nodes_per_chan,
		t_chan_details *chan_details,
		const enum e_seg_details_type seg_details_type);

void free_seg_details(
		t_seg_details *seg_details,
		const int max_chan_width); 
void free_chan_details(
		t_chan_details *chan_details_x,
		t_chan_details *chan_details_y,
		const int max_chan_width,
		const int L_nx, const int L_ny);

int get_seg_start(
		const t_seg_details *seg_details,
		const int itrack,
		const int chan_num,
		const int seg_num);
int get_seg_end(const t_seg_details *seg_details,
		const int itrack,
		const int istart,
		const int chan_num,
		const int seg_max);

bool is_cblock(
		const int chan,
		const int seg,
		const int track,
		const t_seg_details *seg_details,
		const enum e_directionality directionality);

bool is_sblock(
		const int chan,
		int wire_seg,
		const int sb_seg,
		const int track,
		const t_seg_details *seg_details,
		const enum e_directionality directionality);

int get_bidir_opin_connections(
		const int i, const int j,
		const int ipin,
		struct s_linked_edge **edge_list,
		int ******opin_to_track_map,
		const int Fc,
		bool *L_rr_edge_done,
		vtr::t_ivec ***L_rr_node_indices,
		const t_seg_details *seg_details);

int get_unidir_opin_connections(
		const t_type_ptr type,
		const int chan,
		const int seg,
		int Fc,
		const int seg_type_index,
		const t_rr_type chan_type,
		const t_seg_details *seg_details,
		t_linked_edge **edge_list_ptr,
		int ***Fc_ofs,
		bool *L_rr_edge_done,
		const int max_len,
		const int max_chan_width,
		vtr::t_ivec ***L_rr_node_indices,
		bool *Fc_clipped);

int get_track_to_pins(
		int seg, int chan, int track, int tracks_per_chan,
		t_linked_edge **edge_list_ptr, vtr::t_ivec ***L_rr_node_indices,
		vtr::t_ivec *****track_to_pin_lookup, t_seg_details *seg_details,
		enum e_rr_type chan_type, int chan_length, int wire_to_ipin_switch,
		enum e_directionality directionality);

int get_track_to_tracks(
		const int from_chan,
		const int from_seg,
		const int from_track,
		const t_rr_type from_type,
		const int to_seg,
		const t_rr_type to_type,
		const int chan_len,
		const int max_chan_width,
		const int *opin_mux_size,
		const int Fs_per_side,
		short ******sblock_pattern,
		struct s_linked_edge **edge_list,
		const t_seg_details *from_seg_details,
		const t_seg_details *to_seg_details,
		const t_chan_details *to_chan_details,
		const enum e_directionality directionality,
		vtr::t_ivec ***L_rr_node_indices,
		bool *L_rr_edge_done,
		vtr::t_ivec ***switch_block_conn,
		t_sb_connection_map *sb_conn_map);

short ******alloc_sblock_pattern_lookup(
		const int L_nx, const int L_ny,
		const int max_chan_width);
void free_sblock_pattern_lookup(
		short ******sblock_pattern);
void load_sblock_pattern_lookup(
		const int i, const int j,
		const t_chan_width *nodes_per_chan,
		const t_chan_details *chan_details_x,
		const t_chan_details *chan_details_y,
		const int Fs,
		const enum e_switch_block_type switch_block_type,
		short ******sblock_pattern);

int *get_seg_track_counts(
		const int num_sets, const int num_seg_types,
		const t_segment_inf * segment_inf, const bool use_full_seg_groups);

void dump_seg_details(
		const t_seg_details *seg_details,
		int max_chan_width,
		const char *fname);
void dump_seg_details(
		const t_seg_details *seg_details,
		int max_chan_width,
		FILE *fp);
void dump_chan_details(
		const t_chan_details *chan_details_x,
		const t_chan_details *chan_details_y,
		int max_chan_width,
		const int L_nx, int const L_ny,
		const char *fname);
void dump_sblock_pattern(
		short ******sblock_pattern,
		int max_chan_width,
		const int L_nx, int const L_ny,
		const char *fname);

void print_rr_node_indices(int L_nx, int L_ny, vtr::t_ivec ***L_rr_node_indices);
void print_rr_node_indices(t_rr_type rr_type, int L_nx, int L_ny, vtr::t_ivec ***L_rr_node_indices);
#endif
