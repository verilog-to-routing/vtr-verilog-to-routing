void do_clustering(const t_arch *arch, t_pack_molecule *molecule_head,
		int num_models, bool global_clocks, bool *is_clock,
		bool hill_climbing_flag, char *out_fname, bool timing_driven,
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
		int recompute_timing_after, float block_delay,
		float intra_cluster_net_delay, float inter_cluster_net_delay,
		float aspect, bool allow_unrelated_clustering,
		bool allow_early_exit, bool connection_driven,
		enum e_packer_algorithm packer_algorithm, t_timing_inf timing_inf,
		vector<t_lb_type_rr_node> *lb_type_rr_graphs);
int get_cluster_of_block(int blkidx);
