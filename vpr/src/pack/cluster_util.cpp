#include "cluster_util.h"

#include "cluster_router.h"
#include "cluster_placement.h"
#include "output_clustering.h"

/* TODO: May want to check that all atom blocks are actually reached */
static void check_cluster_atom_blocks(t_pb* pb, std::unordered_set<AtomBlockId>& blocks_checked) {
    int i, j;
    const t_pb_type* pb_type;
    bool has_child = false;
    auto& atom_ctx = g_vpr_ctx.atom();

    pb_type = pb->pb_graph_node->pb_type;
    if (pb_type->num_modes == 0) {
        /* primitive */
        auto blk_id = atom_ctx.lookup.pb_atom(pb);
        if (blk_id) {
            if (blocks_checked.count(blk_id)) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "pb %s contains atom block %s but atom block is already contained in another pb.\n",
                                pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
            }
            blocks_checked.insert(blk_id);
            if (pb != atom_ctx.lookup.atom_pb(blk_id)) {
                VPR_FATAL_ERROR(VPR_ERROR_PACK,
                                "pb %s contains atom block %s but atom block does not link to pb.\n",
                                pb->name, atom_ctx.nlist.block_name(blk_id).c_str());
            }
        }
    } else {
        /* this is a container pb, all container pbs must contain children */
        for (i = 0; i < pb_type->modes[pb->mode].num_pb_type_children; i++) {
            for (j = 0; j < pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
                if (pb->child_pbs[i] != nullptr) {
                    if (pb->child_pbs[i][j].name != nullptr) {
                        has_child = true;
                        check_cluster_atom_blocks(&pb->child_pbs[i][j], blocks_checked);
                    }
                }
            }
        }
        VTR_ASSERT(has_child);
    }
}

/*Print the contents of each cluster to an echo file*/
static void echo_clusters(char* filename) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Clusters\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    std::map<ClusterBlockId, std::vector<AtomBlockId>> cluster_atoms;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        cluster_atoms.insert({blk_id, std::vector<AtomBlockId>()});
    }

    for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);

        cluster_atoms[clb_index].push_back(atom_blk_id);
    }

    for (auto i = cluster_atoms.begin(); i != cluster_atoms.end(); i++) {
        std::string cluster_name;
        cluster_name = cluster_ctx.clb_nlist.block_name(i->first);
        fprintf(fp, "Cluster %s Id: %zu \n", cluster_name.c_str(), size_t(i->first));
        fprintf(fp, "\tAtoms in cluster: \n");

        int num_atoms = i->second.size();

        for (auto j = 0; j < num_atoms; j++) {
            AtomBlockId atom_id = i->second[j];
            fprintf(fp, "\t %s \n", atom_ctx.nlist.block_name(atom_id).c_str());
        }
    }

    fprintf(fp, "\nCluster Floorplanning Constraints:\n");
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    for (ClusterBlockId clb_id : cluster_ctx.clb_nlist.blocks()) {
        std::vector<Region> reg = floorplanning_ctx.cluster_constraints[clb_id].get_partition_region();
        if (reg.size() != 0) {
            fprintf(fp, "\nRegions in Cluster %zu:\n", size_t(clb_id));
            for (unsigned int i = 0; i < reg.size(); i++) {
                print_region(fp, reg[i]);
            }
        }
    }

    fclose(fp);
}

/* TODO: Add more error checking! */
void check_clustering() {
    std::unordered_set<AtomBlockId> atoms_checked;
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    if (cluster_ctx.clb_nlist.blocks().size() == 0) {
        VTR_LOG_WARN("Packing produced no clustered blocks");
    }

    /*
     * Check that each atom block connects to one physical primitive and that the primitive links up to the parent clb
     */
    for (auto blk_id : atom_ctx.nlist.blocks()) {
        //Each atom should be part of a pb
        const t_pb* atom_pb = atom_ctx.lookup.atom_pb(blk_id);
        if (!atom_pb) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom block %s is not mapped to a pb\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        //Check the reverse mapping is consistent
        if (atom_ctx.lookup.pb_atom(atom_pb) != blk_id) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "pb %s does not contain atom block %s but atom block %s maps to pb.\n",
                            atom_pb->name,
                            atom_ctx.nlist.block_name(blk_id).c_str(),
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        VTR_ASSERT(atom_ctx.nlist.block_name(blk_id) == atom_pb->name);

        const t_pb* cur_pb = atom_pb;
        while (cur_pb->parent_pb) {
            cur_pb = cur_pb->parent_pb;
            VTR_ASSERT(cur_pb->name);
        }

        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(blk_id);
        if (clb_index == ClusterBlockId::INVALID()) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom %s is not mapped to a CLB\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }

        if (cur_pb != cluster_ctx.clb_nlist.block_pb(clb_index)) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "CLB %s does not match CLB contained by pb %s.\n",
                            cur_pb->name, atom_pb->name);
        }
    }

    /* Check that I do not have spurious links in children pbs */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        check_cluster_atom_blocks(cluster_ctx.clb_nlist.block_pb(blk_id), atoms_checked);
    }

    for (auto blk_id : atom_ctx.nlist.blocks()) {
        if (!atoms_checked.count(blk_id)) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Atom block %s not found in any cluster.\n",
                            atom_ctx.nlist.block_name(blk_id).c_str());
        }
    }
}

//calculate the initial timing at the start of packing stage
void calc_init_packing_timing(const t_packer_opts& packer_opts,
                              const t_analysis_opts& analysis_opts,
                              const std::unordered_map<AtomBlockId, t_pb_graph_node*>& expected_lowest_cost_pb_gnode,
                              std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc,
                              std::shared_ptr<SetupTimingInfo>& timing_info,
                              vtr::vector<AtomBlockId, float>& atom_criticality) {
    auto& atom_ctx = g_vpr_ctx.atom();

    /*
     * Initialize the timing analyzer
     */
    clustering_delay_calc = std::make_shared<PreClusterDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, packer_opts.inter_cluster_net_delay, expected_lowest_cost_pb_gnode);
    timing_info = make_setup_timing_info(clustering_delay_calc, packer_opts.timing_update_type);

    //Calculate the initial timing
    timing_info->update();

    if (isEchoFileEnabled(E_ECHO_PRE_PACKING_TIMING_GRAPH)) {
        auto& timing_ctx = g_vpr_ctx.timing();
        tatum::write_echo(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH),
                          *timing_ctx.graph, *timing_ctx.constraints, *clustering_delay_calc, timing_info->analyzer());

        tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
        write_setup_timing_graph_dot(getEchoFileName(E_ECHO_PRE_PACKING_TIMING_GRAPH) + std::string(".dot"),
                                     *timing_info, debug_tnode);
    }

    {
        auto& timing_ctx = g_vpr_ctx.timing();
        PreClusterTimingGraphResolver resolver(atom_ctx.nlist,
                                               atom_ctx.lookup, *timing_ctx.graph, *clustering_delay_calc);
        resolver.set_detail_level(analysis_opts.timing_report_detail);

        tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                              *timing_ctx.constraints);

        timing_reporter.report_timing_setup(
            "pre_pack.report_timing.setup.rpt",
            *timing_info->setup_analyzer(),
            analysis_opts.timing_report_npaths);
    }

    //Calculate true criticalities of each block
    for (AtomBlockId blk : atom_ctx.nlist.blocks()) {
        for (AtomPinId in_pin : atom_ctx.nlist.block_input_pins(blk)) {
            //Max criticality over incoming nets
            float crit = timing_info->setup_pin_criticality(in_pin);
            atom_criticality[blk] = std::max(atom_criticality[blk], crit);
        }
    }
}

//Free the clustering data structures
void free_clustering_data(const t_packer_opts& packer_opts,
                          vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                          int* hill_climbing_inputs_avail,
                          t_cluster_placement_stats* cluster_placement_stats,
                          t_molecule_link* unclustered_list_head,
                          t_molecule_link* memory_pool,
                          t_pb_graph_node** primitives_list) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        free_intra_lb_nets(intra_lb_routing[blk_id]);

    intra_lb_routing.clear();

    if (packer_opts.hill_climbing_flag)
        free(hill_climbing_inputs_avail);

    free_cluster_placement_stats(cluster_placement_stats);

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        cluster_ctx.clb_nlist.remove_block(blk_id);

    cluster_ctx.clb_nlist = ClusteredNetlist();

    free(unclustered_list_head);
    free(memory_pool);

    free(primitives_list);
}

//check the clustering and output it
void check_and_output_clustering(const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch,
                                 const int& num_clb,
                                 const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                                 bool& floorplan_regions_overfull) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    VTR_ASSERT(num_clb == (int)cluster_ctx.clb_nlist.blocks().size());
    check_clustering();

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERS)) {
        echo_clusters(getEchoFileName(E_ECHO_CLUSTERS));
    }

    output_clustering(intra_lb_routing, packer_opts.global_clocks, is_clock, arch->architecture_id, packer_opts.output_file.c_str(), false);

    //check_floorplan_regions(floorplan_regions_overfull);
    floorplan_regions_overfull = floorplan_constraints_regions_overfull();

    VTR_ASSERT(cluster_ctx.clb_nlist.blocks().size() == intra_lb_routing.size());
}

void get_max_cluster_size_and_pb_depth(int& max_cluster_size,
                                       int& max_pb_depth) {
    auto& device_ctx = g_vpr_ctx.mutable_device();
    int cur_cluster_size, cur_pb_depth;

    for (const auto& type : device_ctx.logical_block_types) {
        if (is_empty_type(&type))
            continue;

        cur_cluster_size = get_max_primitives_in_pb_type(type.pb_type);
        cur_pb_depth = get_max_depth_of_pb_type(type.pb_type);
        if (cur_cluster_size > max_cluster_size) {
            max_cluster_size = cur_cluster_size;
        }
        if (cur_pb_depth > max_pb_depth) {
            max_pb_depth = cur_pb_depth;
        }
    }
}

bool check_cluster_legality(const int& verbosity,
                            const int& detailed_routing_stage,
                            t_lb_router_data* router_data) {
    bool is_cluster_legal;

    if (detailed_routing_stage == (int)E_DETAILED_ROUTE_AT_END_ONLY) {
        /* is_mode_conflict does not affect this stage. It is needed when trying to route the packed clusters.
         *
         * It holds a flag that is used to verify whether try_intra_lb_route ended in a mode conflict issue.
         * If the value is TRUE the cluster has to be repacked, and its internal pb_graph_nodes will have more restrict choices
         * for what regards the mode that has to be selected
         */
        t_mode_selection_status mode_status;
        is_cluster_legal = try_intra_lb_route(router_data, verbosity, &mode_status);
        if (is_cluster_legal) {
            VTR_LOGV(verbosity > 2, "\tPassed route at end.\n");
        } else {
            VTR_LOGV(verbosity > 0, "Failed route at end, repack cluster trying detailed routing at each stage.\n");
        }
    } else {
        is_cluster_legal = true;
    }
    return is_cluster_legal;
}
