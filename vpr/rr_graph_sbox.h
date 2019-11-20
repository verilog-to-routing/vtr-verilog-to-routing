struct s_ivec get_switch_box_tracks (int from_i, int from_j, int from_track,
             t_rr_type from_type, int to_i, int to_j, t_rr_type to_type, enum
             e_switch_block_type switch_block_type, int nodes_per_chan); 

void free_switch_block_conn (int nodes_per_chan); 

void alloc_and_load_switch_block_conn (int nodes_per_chan,
        enum e_switch_block_type switch_block_type);
