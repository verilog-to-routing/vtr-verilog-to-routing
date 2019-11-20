void do_clustering (int cluster_size, int inputs_per_cluster,
       int clocks_per_cluster, int lut_size, boolean global_clocks,
       boolean muxes_to_cluster_output_pins,
       boolean *is_clock, boolean hill_climbing_flag, 
       char *out_fname, boolean timing_driven, 
       enum e_cluster_seed cluster_seed_type, float alpha, 
       int recompute_timing_after, float block_delay, 
       float intra_cluster_net_delay, float inter_cluster_net_delay,
       boolean allow_unrelated_clustering,
       boolean allow_early_exit, boolean connection_driven);
int num_input_pins (int iblk); 
int get_cluster_of_block(int blkidx);

