#ifndef CLUSTER_H
#define CLUSTER_H
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>

#include "physical_types.h"
#include "vpr_types.h"
#include "atom_netlist_fwd.h"

//#define USE_HMETIS 1

std::map<t_type_ptr,size_t> do_clustering(const t_arch *arch, t_pack_molecule *molecule_head,
		int num_models, bool global_clocks,
        const std::unordered_set<AtomNetId>& is_clock,
        std::multimap<AtomBlockId,t_pack_molecule*>& atom_molecules,
        const std::unordered_map<AtomBlockId,t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
		bool hill_climbing_flag, const char *out_fname, bool timing_driven,
		enum e_cluster_seed cluster_seed_type, float alpha, float beta,
        float inter_cluster_net_delay,
        float target_device_utilization,
		bool allow_unrelated_clustering,
		bool connection_driven,
		enum e_packer_algorithm packer_algorithm,
		std::vector<t_lb_type_rr_node> *lb_type_rr_graphs,
        std::string device_layout_name,
        int verbosity,
        bool enable_pin_feasibility_filter,
        const t_ext_pin_util_targets& ext_pin_util_targets
#ifdef USE_HMETIS
		, vtr::vector_map<AtomBlockId, int>& partitions
#endif
#ifdef ENABLE_CLASSIC_VPR_STA
        , t_timing_inf timing_inf
#endif
        );
int get_cluster_of_block(int blkidx);
#endif
