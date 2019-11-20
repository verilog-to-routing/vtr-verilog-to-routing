void do_clustering(const t_arch *arch, t_pack_molecule *molecule_head,
		int num_models, boolean global_clocks, boolean *is_clock,
		boolean hill_climbing_flag, char *out_fname, boolean timing_driven,
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
		int recompute_timing_after, float block_delay,
		float intra_cluster_net_delay, float inter_cluster_net_delay,
		float aspect, boolean allow_unrelated_clustering,
		boolean allow_early_exit, boolean connection_driven,
		enum e_packer_algorithm packer_algorithm, t_timing_inf timing_inf);
int get_cluster_of_block(int blkidx);
