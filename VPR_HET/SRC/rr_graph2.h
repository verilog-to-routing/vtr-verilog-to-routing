/************** Global variables shared only by the rr_* modules. ************/

extern boolean *rr_edge_done;	/* [0..num_rr_nodes-1].  Used to keep track  *
				 * of whether or not a node has been put in  *
				 * an edge list yet.                         */

/******************* Subroutines exported by rr_graph2.c *********************/

struct s_ivec ***alloc_and_load_rr_node_indices(IN int nodes_per_chan,
						IN int nx,
						IN int ny,
						INOUT int *index,
						IN t_seg_details *
						seg_details);

void free_rr_node_indices(IN t_ivec *** rr_node_indices);

int get_rr_node_index(int x,
		      int y,
		      t_rr_type rr_type,
		      int ptc,
		      t_ivec *** rr_node_indices);

void free_seg_details(t_seg_details * seg_details,
		      int nodes_per_chan);

t_seg_details *alloc_and_load_seg_details(INOUT int *nodes_per_chan,
					  IN int max_len,
					  IN int num_seg_types,
					  IN t_segment_inf * segment_inf,
					  IN boolean use_full_seg_groups,
					  IN boolean is_global_graph,
					  IN enum e_directionality
					  directionality);

void dump_seg_details(t_seg_details * seg_details,
		      int nodes_per_chan,
		      char *fname);

int get_seg_start(IN t_seg_details * seg_details,
		  IN int itrack,
		  IN int chan_num,
		  IN int seg_num);

int get_seg_end(IN t_seg_details * seg_details,
		IN int itrack,
		IN int istart,
		IN int chan_num,
		IN int seg_max);

boolean is_cbox(IN int chan,
		IN int seg,
		IN int track,
		IN t_seg_details * seg_details,
		IN enum e_directionality directionality);

boolean is_sbox(IN int chan,
		IN int wire_seg,
		IN int sb_seg,
		IN int track,
		IN t_seg_details * seg_details,
		IN enum e_directionality directionality);

int get_bidir_opin_connections(IN int i,
			       IN int j,
			       IN int ipin,
			       IN struct s_linked_edge **edge_list,
			       IN int *****opin_to_track_map,
			       IN int Fc,
			       IN boolean * rr_edge_done,
			       IN t_ivec *** rr_node_indices,
			       IN t_seg_details * seg_details);

int get_unidir_opin_connections(IN int chan,
				IN int seg,
				IN int Fc,
				IN t_rr_type chan_type,
				IN t_seg_details * seg_details,
				INOUT t_linked_edge ** edge_list_ptr,
				INOUT int **Fc_ofs,
				INOUT boolean * rr_edge_done,
				IN int max_len,
				IN int nodes_per_chan,
				IN t_ivec *** rr_node_indices,
				OUT boolean * Fc_clipped);

int get_track_to_ipins(int seg,
		       int chan,
		       int track,
		       t_linked_edge ** edge_list_ptr,
		       t_ivec *** rr_node_indices,
		       struct s_ivec ****track_to_ipin_lookup,
		       t_seg_details * seg_details,
		       enum e_rr_type chan_type,
		       int chan_length,
		       int wire_to_ipin_switch,
		       enum e_directionality directionality);

int get_track_to_tracks(IN int from_chan,
			IN int from_seg,
			IN int from_track,
			IN t_rr_type from_type,
			IN int to_seg,
			IN t_rr_type to_type,
			IN int chan_len,
			IN int nodes_per_chan,
			IN int *opin_mux_size,
			IN int Fs_per_side,
			IN short *****sblock_pattern,
			INOUT struct s_linked_edge **edge_list,
			IN t_seg_details * seg_details,
			IN enum e_directionality directionality,
			IN t_ivec *** rr_node_indices,
			INOUT boolean * rr_edge_done,
			IN struct s_ivec ***switch_block_conn);

short *****alloc_sblock_pattern_lookup(IN int nx,
				       IN int ny,
				       IN int nodes_per_chan);
void free_sblock_pattern_lookup(INOUT short *****sblock_pattern);
void load_sblock_pattern_lookup(IN int i,
				IN int j,
				IN int nodes_per_chan,
				IN t_seg_details * seg_details,
				IN int Fs,
				IN enum e_switch_block_type switch_block_type,
				INOUT short *****sblock_pattern);
