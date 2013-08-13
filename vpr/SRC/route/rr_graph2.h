#ifndef RR_GRAPH2_H
#define RR_GRAPH2_H

/************** Global variables shared only by the rr_* modules. ************/

extern boolean *rr_edge_done; /* [0..num_rr_nodes-1].  Used to keep track  *
 * of whether or not a node has been put in  *
 * an edge list yet. TRUE if a node is already listed in the edges array *
 * that's being constructed. Ensure that there are no duplicate edges.   */

/******************* Types shared by rr_graph2 functions *********************/

enum e_seg_details_type {
	SEG_DETAILS_X, SEG_DETAILS_Y
};

/******************* Subroutines exported by rr_graph2.c *********************/

struct s_ivec ***alloc_and_load_rr_node_indices(
		INP int max_chan_width,
		INP int L_nx, INP int L_ny, 
		INOUTP int *index, 
		INP t_chan_details *chan_details_x,
		INP t_chan_details *chan_details_y);

void free_rr_node_indices(
		INP t_ivec ***L_rr_node_indices);

int get_rr_node_index(
		int x, int y, 
		t_rr_type rr_type, int ptc,
		t_ivec ***L_rr_node_indices);

int find_average_rr_node_index(
		int L_nx, int L_ny,
		t_rr_type rr_type, int ptc,
		t_ivec ***L_rr_node_indices);

t_seg_details *alloc_and_load_seg_details(
		INOUTP int *max_chan_width,
		INP int max_len,
		INP int num_seg_types,
		INP t_segment_inf *segment_inf,
		INP boolean use_full_seg_groups,
		INP boolean is_global_graph,
		INP enum e_directionality directionality,
		OUTP int *num_seg_details = 0);

void alloc_and_load_chan_details( 
		INP int L_nx, INP int L_ny,
		INP t_chan_width *nodes_per_chan,
		INP boolean trim_empty_channels,
		INP boolean trim_obs_channels,
		INP int num_seg_details,
		INP const t_seg_details *seg_details,
		OUTP t_chan_details **chan_details_x,
		OUTP t_chan_details **chan_details_y);
t_chan_details *init_chan_details( 
		INP int L_nx, INP int L_ny,
		INP t_chan_width *nodes_per_chan,
		INP int num_seg_details,
		INP const t_seg_details *seg_details,
		INP enum e_seg_details_type seg_details_type);
void obstruct_chan_details( 
		INP int L_nx, INP int L_ny,
		INP t_chan_width *nodes_per_chan,
		INP boolean trim_empty_channels,
		INP boolean trim_obs_channels,
		INOUTP t_chan_details *chan_details_x,
		INOUTP t_chan_details *chan_details_y);
void adjust_chan_details(
		INP int L_nx, INP int L_ny,
		INP t_chan_width *nodes_per_chan,
		INOUTP t_chan_details *chan_details_x,
		INOUTP t_chan_details *chan_details_y);
void adjust_seg_details(
		INP int x, INP int y,
		INP int L_nx, INP int L_ny,
		INP t_chan_width *nodes_per_chan,
		INOUTP t_chan_details *chan_details,
		INP enum e_seg_details_type seg_details_type);

void free_seg_details(
		t_seg_details *seg_details,
		INP int max_chan_width); 
void free_chan_details(
		t_chan_details *chan_details_x,
		t_chan_details *chan_details_y,
		INP int max_chan_width,
		INP int L_nx, INP int L_ny);

int get_seg_start(
		INP t_seg_details *seg_details,
		INP int itrack,
		INP int chan_num,
		INP int seg_num);
int get_seg_end(INP t_seg_details *seg_details,
		INP int itrack,
		INP int istart,
		INP int chan_num,
		INP int seg_max);

boolean is_cblock(
		INP int chan,
		INP int seg,
		INP int track,
		INP t_seg_details *seg_details,
		INP enum e_directionality directionality);

boolean is_sblock(
		INP int chan,
		INP int wire_seg,
		INP int sb_seg,
		INP int track,
		INP t_seg_details *seg_details,
		INP enum e_directionality directionality);

int get_bidir_opin_connections(
		INP int i, INP int j,
		INP int ipin,
		INP struct s_linked_edge **edge_list,
		INP int ******opin_to_track_map,
		INP int Fc,
		INP boolean *L_rr_edge_done,
		INP t_ivec ***L_rr_node_indices,
		INP t_seg_details *seg_details);

int get_unidir_opin_connections(
		INP int chan,
		INP int seg,
		INP int Fc,
		INP t_rr_type chan_type,
		INP t_seg_details *seg_details,
		INOUTP t_linked_edge **edge_list_ptr,
		INOUTP int **Fc_ofs,
		INOUTP boolean *L_rr_edge_done,
		INP int max_len,
		INP int max_chan_width,
		INP t_ivec ***L_rr_node_indices,
		OUTP boolean *Fc_clipped);

int get_track_to_pins(
		int seg, int chan, int track, int tracks_per_chan,
		t_linked_edge **edge_list_ptr, t_ivec ***L_rr_node_indices,
		struct s_ivec *****track_to_pin_lookup, t_seg_details *seg_details,
		enum e_rr_type chan_type, int chan_length, int wire_to_ipin_switch,
		enum e_directionality directionality);

int get_track_to_tracks(
		INP int from_chan,
		INP int from_seg,
		INP int from_track,
		INP t_rr_type from_type,
		INP int to_seg,
		INP t_rr_type to_type,
		INP int chan_len,
		INP int max_chan_width,
		INP int *opin_mux_size,
		INP int Fs_per_side,
		INP short ******sblock_pattern,
		INOUTP struct s_linked_edge **edge_list,
		INP t_seg_details *from_seg_details,
		INP t_seg_details *to_seg_details,
		INP t_chan_details *to_chan_details,
		INP enum e_directionality directionality,
		INP t_ivec ***L_rr_node_indices,
		INOUTP boolean *L_rr_edge_done,
		INP struct s_ivec ***switch_block_conn);

short ******alloc_sblock_pattern_lookup(
		INP int L_nx, INP int L_ny,
		INP int max_chan_width);
void free_sblock_pattern_lookup(
		INOUTP short ******sblock_pattern);
void load_sblock_pattern_lookup(
		INP int i, INP int j,
		INP t_chan_width *nodes_per_chan,
		INP t_chan_details *chan_details_x,
		INP t_chan_details *chan_details_y,
		INP int Fs,
		INP enum e_switch_block_type switch_block_type,
		INOUTP short ******sblock_pattern);

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
		INP int L_nx, int INP L_ny,
		const char *fname);
void dump_sblock_pattern(
		INP short ******sblock_pattern,
		int max_chan_width,
		INP int L_nx, int INP L_ny,
		const char *fname);

void print_rr_node_indices(int L_nx, int L_ny, t_ivec ***L_rr_node_indices);
void print_rr_node_indices(t_rr_type rr_type, int L_nx, int L_ny, t_ivec ***L_rr_node_indices);

//===========================================================================//
#include "TFH_FabricSwitchBoxHandler.h"

void override_sblock_pattern_lookup(
		INP int x, INP int y,
		INP int max_chan_width,
		INOUTP short ******sblock_pattern);
void override_sblock_pattern_lookup_side(
		INP int x, INP int y,
		INP const TC_MapTable_c& mapTable,
		INP TC_SideMode_t sideMode,
		INP int max_chan_width,
		INOUTP short ******sblock_pattern);
void override_sblock_pattern_set_side_track(
		INP int x, INP int y,
		INP int from_side, INP int from_track,
		INP const TC_SideList_t& sideList,
		INP int max_chan_width,
		INOUTP short ******sblock_pattern);
void override_sblock_pattern_reset_side_track(
		INP int x, INP int y,
		INP int from_side, INP int from_track,
		INOUTP short ******sblock_pattern);
int override_sblock_pattern_map_side_mode(
		INP TC_SideMode_t sideMode);
//===========================================================================//

//===========================================================================//
#include "TFH_FabricConnectionBlockHandler.h"

void override_cblock_edge_lists(
		int num_rr_nodes, 
		t_rr_node *rr_node );
//===========================================================================//

#endif
