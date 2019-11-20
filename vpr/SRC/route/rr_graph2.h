#ifndef RR_GRAPH2_H
#define RR_GRAPH2_H

/************** Global variables shared only by the rr_* modules. ************/

extern boolean *rr_edge_done; /* [0..num_rr_nodes-1].  Used to keep track  *
 * of whether or not a node has been put in  *
 * an edge list yet. TRUE if a node is already listed in the edges array *
 * that's being constructed. Ensure that there are no duplicate edges.   */

/******************* Subroutines exported by rr_graph2.c *********************/

struct s_ivec ***alloc_and_load_rr_node_indices(INP int nodes_per_chan, 
		INP int L_nx,
		INP int L_ny, 
		INOUTP int *index, 
		INP t_seg_details * seg_details) ;

void free_rr_node_indices(INP t_ivec *** L_rr_node_indices);

int get_rr_node_index(int x, int y, t_rr_type rr_type, int ptc,
		t_ivec *** L_rr_node_indices);

void free_seg_details(t_seg_details * seg_details, int nodes_per_chan);

t_seg_details *alloc_and_load_seg_details(INOUTP int *nodes_per_chan,
		INP int max_len,
		INP int num_seg_types,
		INP t_segment_inf * segment_inf,
		INP boolean use_full_seg_groups,
		INP boolean is_global_graph,
		INP enum e_directionality
		directionality);

void dump_seg_details(t_seg_details * seg_details, int nodes_per_chan,
		const char *fname);

int get_seg_start(INP t_seg_details * seg_details,
		INP int itrack,
		INP int chan_num,
		INP int seg_num);

int get_seg_end(INP t_seg_details * seg_details,
		INP int itrack,
		INP int istart,
		INP int chan_num,
		INP int seg_max);

boolean is_cbox(INP int chan,
		INP int seg,
		INP int track,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality);

boolean is_sbox(INP int chan,
		INP int wire_seg,
		INP int sb_seg,
		INP int track,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality);

int get_bidir_opin_connections(INP int i,
		INP int j,
		INP int ipin,
		INP struct s_linked_edge **edge_list,
		INP int *****opin_to_track_map,
		INP int Fc,
		INP boolean * L_rr_edge_done,
		INP t_ivec *** L_rr_node_indices,
		INP t_seg_details * seg_details);

int get_unidir_opin_connections(INP int chan,
		INP int seg,
		INP int Fc,
		INP t_rr_type chan_type,
		INP t_seg_details * seg_details,
		INOUTP t_linked_edge ** edge_list_ptr,
		INOUTP int **Fc_ofs,
		INOUTP boolean * L_rr_edge_done,
		INP int max_len,
		INP int nodes_per_chan,
		INP t_ivec *** L_rr_node_indices,
		OUTP boolean * Fc_clipped);

int get_track_to_ipins(int seg, int chan, int track,
		t_linked_edge ** edge_list_ptr, t_ivec *** L_rr_node_indices,
		struct s_ivec ****track_to_ipin_lookup, t_seg_details * seg_details,
		enum e_rr_type chan_type, int chan_length, int wire_to_ipin_switch,
		enum e_directionality directionality);

int get_track_to_tracks(INP int from_chan,
		INP int from_seg,
		INP int from_track,
		INP t_rr_type from_type,
		INP int to_seg,
		INP t_rr_type to_type,
		INP int chan_len,
		INP int nodes_per_chan,
		INP int *opin_mux_size,
		INP int Fs_per_side,
		INP short *****sblock_pattern,
		INOUTP struct s_linked_edge **edge_list,
		INP t_seg_details * seg_details,
		INP enum e_directionality directionality,
		INP t_ivec *** L_rr_node_indices,
		INOUTP boolean * L_rr_edge_done,
		INP struct s_ivec ***switch_block_conn);

short *****alloc_sblock_pattern_lookup(INP int L_nx,
		INP int L_ny,
		INP int nodes_per_chan);
void free_sblock_pattern_lookup(INOUTP short *****sblock_pattern);
void load_sblock_pattern_lookup(INP int i,
		INP int j,
		INP int nodes_per_chan,
		INP t_seg_details * seg_details,
		INP int Fs,
		INP enum e_switch_block_type switch_block_type,
		INOUTP short *****sblock_pattern);

#endif

