#ifndef CLUSTER_H
#define CLUSTER_H
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "physical_types.h"
#include "vpr_types.h"

void do_clustering(const t_arch *arch, t_pack_molecule *molecule_head,
		int num_models, bool global_clocks, 
        const std::unordered_set<int>& is_clock,
        const std::unordered_multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
		bool hill_climbing_flag, const char *out_fname, bool timing_driven,
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
        float inter_cluster_net_delay,
		float aspect, bool allow_unrelated_clustering,
		bool connection_driven,
		enum e_packer_algorithm packer_algorithm, t_timing_inf timing_inf,
		std::vector<t_lb_type_rr_node> *lb_type_rr_graphs);
int get_cluster_of_block(int blkidx);
#endif
