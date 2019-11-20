void do_clustering (int cluster_size, int inputs_per_cluster,
        int clocks_per_cluster, int lut_size, boolean global_clocks,
        boolean *is_clock, boolean hill_climbing_flag,
        enum e_cluster_seed cluster_seed_type, boolean
        muxes_to_cluster_output_pins, char *out_fname);
int num_input_pins (int iblk); 
