#include "rr_graph_util.h"

/************** Global variables shared only by the rr_* modules. ************/

extern boolean *rr_edge_done;   /* [0..num_rr_nodes-1].  Used to keep track  *
                                 * of whether or not a node has been put in  *
                                 * an edge list yet.                         */


extern t_linked_edge *free_edge_list_head; /* Start of linked list of free   *
                                            * edge_list elements (for speed) */


/******************* Subroutines exported by rr_graph2.c *********************/

int **alloc_and_load_rr_node_indices (int nodes_per_clb,
        int nodes_per_pad, int nodes_per_chan, t_seg_details *seg_details_x,
        t_seg_details *seg_details_y); 

void free_rr_node_indices (int **rr_node_indices);

int get_rr_node_index (int i_in, int j_in, t_rr_type rr_type, int ioff,
        int nodes_per_chan, int **rr_node_indices);

void free_seg_details (t_seg_details *seg_details, int nodes_per_chan);

t_seg_details *alloc_and_load_seg_details (int nodes_per_chan, t_segment_inf 
        *segment_inf, int num_seg_types, int max_len);

void dump_seg_details (t_seg_details *seg_details, int nodes_per_chan, char 
        *fname);

int get_closest_seg_start (t_seg_details *seg_details, int itrack, int seg_num,
        int chan_num); 

int get_seg_end (t_seg_details *seg_details, int itrack, int seg_start, int
       chan_num, int max_end);

boolean is_cbox (int seg_num, int chan_num, int itrack, t_seg_details
        *seg_details); 

boolean is_sbox (int seg_num, int chan_num, int itrack, t_seg_details
        *seg_details, boolean above_or_right); 


int get_clb_opin_connections (int ***clb_opin_to_tracks, int ipin, int i, int
        j, int Fc_output, t_seg_details *seg_details_x, t_seg_details
        *seg_details_y, t_linked_edge **edge_list_ptr, int nodes_per_chan, int
        **rr_node_indices); 

int get_pad_opin_connections (int **pads_to_tracks, int ipad, int i, int j,
        int Fc_pad, t_seg_details *seg_details_x, t_seg_details *seg_details_y,
        t_linked_edge **edge_list_ptr, int nodes_per_chan, int
        **rr_node_indices); 

int get_xtrack_to_clb_ipin_edges (int tr_istart, int tr_iend, int tr_j,
       int itrack, int iside, t_linked_edge **edge_list_ptr, struct s_ivec
       **tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices,
       t_seg_details *seg_details_x, int wire_to_ipin_switch); 

int get_ytrack_to_clb_ipin_edges (int tr_jstart, int tr_jend, int tr_i,
       int itrack, int iside, t_linked_edge **edge_list_ptr, struct s_ivec
       **tracks_to_clb_ipin, int nodes_per_chan, int **rr_node_indices,
       t_seg_details *seg_details_y, int wire_to_ipin_switch); 

int get_xtrack_to_pad_edges (int tr_istart, int tr_iend, int tr_j, int pad_j,
        int itrack, t_linked_edge **edge_list_ptr, struct s_ivec
        *tracks_to_pads, int nodes_per_chan, int **rr_node_indices,
        t_seg_details *seg_details_x, int wire_to_ipin_switch); 

int get_ytrack_to_pad_edges (int tr_jstart, int tr_jend, int tr_i, int pad_i,
        int itrack, t_linked_edge **edge_list_ptr, struct s_ivec
        *tracks_to_pads, int nodes_per_chan, int **rr_node_indices,
        t_seg_details *seg_details_y, int wire_to_ipin_switch); 

int get_xtrack_to_ytracks (int from_istart, int from_iend, int from_j, int
       from_track, int to_j, t_linked_edge **edge_list_ptr, int nodes_per_chan,
       int **rr_node_indices, t_seg_details *seg_details_x, t_seg_details
       *seg_details_y, enum e_switch_block_type switch_block_type); 

int get_ytrack_to_xtracks (int from_jstart, int from_jend, int from_i, int
       from_track, int to_i, t_linked_edge **edge_list_ptr, int nodes_per_chan,
       int **rr_node_indices, t_seg_details *seg_details_x, t_seg_details
       *seg_details_y, enum e_switch_block_type switch_block_type); 

int get_xtrack_to_xtrack (int from_i, int j, int from_track, int to_i,
       t_linked_edge **edge_list_ptr, int nodes_per_chan, int **rr_node_indices,
       t_seg_details *seg_details_x, enum e_switch_block_type 
       switch_block_type); 

int get_ytrack_to_ytrack (int i, int from_j, int from_track, int to_j,
       t_linked_edge **edge_list_ptr, int nodes_per_chan, int **rr_node_indices,
       t_seg_details *seg_details_y, enum e_switch_block_type 
       switch_block_type); 
