#include "cluster_util.h"

#include "cluster_router.h"
#include "cluster_placement.h"
#include "output_clustering.h"

#include "vtr_math.h"
#include "SetupGrid.h"

/**********************************/
/* Global variables in clustering */
/**********************************/

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
                          t_clustering_data& clustering_data) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        free_intra_lb_nets(clustering_data.intra_lb_routing[blk_id]);

    clustering_data.intra_lb_routing.clear();

    if (packer_opts.hill_climbing_flag)
        free(clustering_data.hill_climbing_inputs_avail);

    for (auto blk_id : cluster_ctx.clb_nlist.blocks())
        cluster_ctx.clb_nlist.remove_block(blk_id);

    cluster_ctx.clb_nlist = ClusteredNetlist();

    free(clustering_data.unclustered_list_head);
    free(clustering_data.memory_pool);
}

//check the clustering and output it
void check_and_output_clustering(const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch,
                                 const int& num_clb,
                                 const vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    VTR_ASSERT(num_clb == (int)cluster_ctx.clb_nlist.blocks().size());
    check_clustering();

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERS)) {
        echo_clusters(getEchoFileName(E_ECHO_CLUSTERS));
    }

    output_clustering(intra_lb_routing, packer_opts.global_clocks, is_clock, arch->architecture_id, packer_opts.output_file.c_str(), false);

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

/*print the header for the clustering progress table*/
void print_pack_status_header() {
    VTR_LOG("Starting Clustering - Clustering Progress: \n");
    VTR_LOG("-------------------   --------------------------   ---------\n");
    VTR_LOG("Molecules processed   Number of clusters created   FPGA size\n");
    VTR_LOG("-------------------   --------------------------   ---------\n");
}

/*incrementally print progress updates during clustering*/
void print_pack_status(int num_clb,
                       int tot_num_molecules,
                       int num_molecules_processed,
                       int& mols_since_last_print,
                       int device_width,
                       int device_height,
                       AttractionInfo& attraction_groups) {
    //Print a packing update each time another 4% of molecules have been packed.
    const float print_frequency = 0.04;

    double percentage = (num_molecules_processed / (double)tot_num_molecules) * 100;

    int int_percentage = int(percentage);

    int int_molecule_increment = (int)(print_frequency * tot_num_molecules);

    if (mols_since_last_print == int_molecule_increment) {
        VTR_LOG(
            "%6d/%-6d  %3d%%   "
            "%26d   "
            "%3d x %-3d   ",
            num_molecules_processed,
            tot_num_molecules,
            int_percentage,
            num_clb,
            device_width,
            device_height);

        VTR_LOG("\n");
        fflush(stdout);
        mols_since_last_print = 0;
        if (attraction_groups.num_attraction_groups() > 0) {
            rebuild_attraction_groups(attraction_groups);
        }
    }
}

/*
 * Periodically rebuild the attraction groups to reflect which atoms in them
 * are still available for new clusters (i.e. remove the atoms that have already
 * been packed from the attraction group).
 */
void rebuild_attraction_groups(AttractionInfo& attraction_groups) {
    auto& atom_ctx = g_vpr_ctx.atom();

    for (int igroup = 0; igroup < attraction_groups.num_attraction_groups(); igroup++) {
        AttractGroupId group_id(igroup);
        AttractionGroup& group = attraction_groups.get_attraction_group_info(group_id);
        AttractionGroup new_att_group_info;

        for (AtomBlockId atom : group.group_atoms) {
            //If the ClusterBlockId is anything other than invalid, the atom has been packed already
            if (atom_ctx.lookup.atom_clb(atom) == ClusterBlockId::INVALID()) {
                new_att_group_info.group_atoms.push_back(atom);
            }
        }

        attraction_groups.set_attraction_group_info(group_id, new_att_group_info);
    }
}

/* Determine if atom block is in pb */
bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb* pb) {
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
    while (cur_pb) {
        if (cur_pb == pb) {
            return true;
        }
        cur_pb = cur_pb->parent_pb;
    }
    return false;
}

/* Remove blk from list of feasible blocks sorted according to gain
 * Useful for removing blocks that are repeatedly failing. If a block
 * has been found to be illegal, we don't repeatedly consider it.*/
void remove_molecule_from_pb_stats_candidates(t_pack_molecule* molecule,
                                              t_pb* pb) {
    int molecule_index;
    bool found_molecule = false;

    //find the molecule index
    for (int i = 0; i < pb->pb_stats->num_feasible_blocks; i++) {
        if (pb->pb_stats->feasible_blocks[i] == molecule) {
            found_molecule = true;
            molecule_index = i;
        }
    }

    //if it is not in the array, return
    if (found_molecule == false) {
        return;
    }

    //Otherwise, shift the molecules while removing the specified molecule
    for (int j = molecule_index; j < pb->pb_stats->num_feasible_blocks - 1; j++) {
        pb->pb_stats->feasible_blocks[j] = pb->pb_stats->feasible_blocks[j + 1];
    }
    pb->pb_stats->num_feasible_blocks--;
}

/* Add blk to list of feasible blocks sorted according to gain */
void add_molecule_to_pb_stats_candidates(t_pack_molecule* molecule,
                                         std::map<AtomBlockId, float>& gain,
                                         t_pb* pb,
                                         int max_queue_size,
                                         AttractionInfo& attraction_groups) {
    int i, j;
    int num_molecule_failures = 0;

    AttractGroupId cluster_att_grp = pb->pb_stats->attraction_grp_id;

    /* When the clusterer packs with attraction groups the goal is to
     * pack more densely. Removing failed molecules to make room for the exploration of
     * more molecules helps to achieve this purpose.
     */
    if (attraction_groups.num_attraction_groups() > 0) {
        auto got = pb->pb_stats->atom_failures.find(molecule->atom_block_ids[0]);
        if (got == pb->pb_stats->atom_failures.end()) {
            num_molecule_failures = 0;
        } else {
            num_molecule_failures = got->second;
        }

        if (num_molecule_failures > 0) {
            remove_molecule_from_pb_stats_candidates(molecule, pb);
            return;
        }
    }

    for (i = 0; i < pb->pb_stats->num_feasible_blocks; i++) {
        if (pb->pb_stats->feasible_blocks[i] == molecule) {
            return; // already in queue, do nothing
        }
    }

    if (pb->pb_stats->num_feasible_blocks >= max_queue_size - 1) {
        /* maximum size for array, remove smallest gain element and sort */
        if (get_molecule_gain(molecule, gain, cluster_att_grp, attraction_groups, num_molecule_failures) > get_molecule_gain(pb->pb_stats->feasible_blocks[0], gain, cluster_att_grp, attraction_groups, num_molecule_failures)) {
            /* single loop insertion sort */
            for (j = 0; j < pb->pb_stats->num_feasible_blocks - 1; j++) {
                if (get_molecule_gain(molecule, gain, cluster_att_grp, attraction_groups, num_molecule_failures) <= get_molecule_gain(pb->pb_stats->feasible_blocks[j + 1], gain, cluster_att_grp, attraction_groups, num_molecule_failures)) {
                    pb->pb_stats->feasible_blocks[j] = molecule;
                    break;
                } else {
                    pb->pb_stats->feasible_blocks[j] = pb->pb_stats->feasible_blocks[j + 1];
                }
            }
            if (j == pb->pb_stats->num_feasible_blocks - 1) {
                pb->pb_stats->feasible_blocks[j] = molecule;
            }
        }
    } else {
        /* Expand array and single loop insertion sort */
        for (j = pb->pb_stats->num_feasible_blocks - 1; j >= 0; j--) {
            if (get_molecule_gain(pb->pb_stats->feasible_blocks[j], gain, cluster_att_grp, attraction_groups, num_molecule_failures) > get_molecule_gain(molecule, gain, cluster_att_grp, attraction_groups, num_molecule_failures)) {
                pb->pb_stats->feasible_blocks[j + 1] = pb->pb_stats->feasible_blocks[j];
            } else {
                pb->pb_stats->feasible_blocks[j + 1] = molecule;
                break;
            }
        }
        if (j < 0) {
            pb->pb_stats->feasible_blocks[0] = molecule;
        }
        pb->pb_stats->num_feasible_blocks++;
    }
}

/*****************************************/
void alloc_and_init_clustering(const t_molecule_stats& max_molecule_stats,
                               t_cluster_placement_stats** cluster_placement_stats,
                               t_pb_graph_node*** primitives_list,
                               t_pack_molecule* molecules_head,
                               t_clustering_data& clustering_data,
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                               int& unclustered_list_head_size,
                               int num_molecules) {
    /* Allocates the main data structures used for clustering and properly *
     * initializes them.                                                   */

    t_molecule_link* next_ptr;
    t_pack_molecule* cur_molecule;
    t_pack_molecule** molecule_array;
    int max_molecule_size;

    /* alloc and load list of molecules to pack */
    clustering_data.unclustered_list_head = (t_molecule_link*)vtr::calloc(max_molecule_stats.num_used_ext_inputs + 1, sizeof(t_molecule_link));
    unclustered_list_head_size = max_molecule_stats.num_used_ext_inputs + 1;

    for (int i = 0; i <= max_molecule_stats.num_used_ext_inputs; i++) {
        clustering_data.unclustered_list_head[i].next = nullptr;
    }

    molecule_array = (t_pack_molecule**)vtr::malloc(num_molecules * sizeof(t_pack_molecule*));
    cur_molecule = molecules_head;
    for (int i = 0; i < num_molecules; i++) {
        VTR_ASSERT(cur_molecule != nullptr);
        molecule_array[i] = cur_molecule;
        cur_molecule = cur_molecule->next;
    }
    VTR_ASSERT(cur_molecule == nullptr);
    qsort((void*)molecule_array, num_molecules, sizeof(t_pack_molecule*),
          compare_molecule_gain);

    clustering_data.memory_pool = (t_molecule_link*)vtr::malloc(num_molecules * sizeof(t_molecule_link));
    next_ptr = clustering_data.memory_pool;

    for (int i = 0; i < num_molecules; i++) {
        //Figure out how many external inputs are used by this molecule
        t_molecule_stats molecule_stats = calc_molecule_stats(molecule_array[i]);
        int ext_inps = molecule_stats.num_used_ext_inputs;

        //Insert the molecule into the unclustered lists by number of external inputs
        next_ptr->moleculeptr = molecule_array[i];
        next_ptr->next = clustering_data.unclustered_list_head[ext_inps].next;
        clustering_data.unclustered_list_head[ext_inps].next = next_ptr;

        next_ptr++;
    }
    free(molecule_array);

    /* load net info */
    auto& atom_ctx = g_vpr_ctx.atom();
    for (AtomNetId net : atom_ctx.nlist.nets()) {
        AtomPinId driver_pin = atom_ctx.nlist.net_driver(net);
        AtomBlockId driver_block = atom_ctx.nlist.pin_block(driver_pin);

        for (AtomPinId sink_pin : atom_ctx.nlist.net_sinks(net)) {
            AtomBlockId sink_block = atom_ctx.nlist.pin_block(sink_pin);

            if (driver_block == sink_block) {
                net_output_feeds_driving_block_input[net]++;
            }
        }
    }

    /* alloc and load cluster placement info */
    *cluster_placement_stats = alloc_and_load_cluster_placement_stats();

    /* alloc array that will store primitives that a molecule gets placed to,
     * primitive_list is referenced by index, for example a atom block in index 2 of a molecule matches to a primitive in index 2 in primitive_list
     * this array must be the size of the biggest molecule
     */
    max_molecule_size = 1;
    cur_molecule = molecules_head;
    while (cur_molecule != nullptr) {
        if (cur_molecule->num_blocks > max_molecule_size) {
            max_molecule_size = cur_molecule->num_blocks;
        }
        cur_molecule = cur_molecule->next;
    }
    *primitives_list = (t_pb_graph_node**)vtr::calloc(max_molecule_size, sizeof(t_pb_graph_node*));
}

/*****************************************/
void free_pb_stats_recursive(t_pb* pb) {
    int i, j;
    /* Releases all the memory used by clustering data structures.   */
    if (pb) {
        if (pb->pb_graph_node != nullptr) {
            if (!pb->pb_graph_node->is_primitive()) {
                for (i = 0; i < pb->pb_graph_node->pb_type->modes[pb->mode].num_pb_type_children; i++) {
                    for (j = 0; j < pb->pb_graph_node->pb_type->modes[pb->mode].pb_type_children[i].num_pb; j++) {
                        if (pb->child_pbs && pb->child_pbs[i]) {
                            free_pb_stats_recursive(&pb->child_pbs[i][j]);
                        }
                    }
                }
            }
        }
        free_pb_stats(pb);
    }
}

bool primitive_feasible(const AtomBlockId blk_id, t_pb* cur_pb) {
    const t_pb_type* cur_pb_type = cur_pb->pb_graph_node->pb_type;

    VTR_ASSERT(cur_pb_type->num_modes == 0); /* primitive */

    auto& atom_ctx = g_vpr_ctx.atom();
    AtomBlockId cur_pb_blk_id = atom_ctx.lookup.pb_atom(cur_pb);
    if (cur_pb_blk_id && cur_pb_blk_id != blk_id) {
        /* This pb already has a different logical block */
        return false;
    }

    if (cur_pb_type->class_type == MEMORY_CLASS) {
        /* Memory class has additional feasibility requirements:
         *   - all siblings must share all nets, including open nets, with the exception of data nets */

        /* find sibling if one exists */
        AtomBlockId sibling_memory_blk_id = find_memory_sibling(cur_pb);

        if (sibling_memory_blk_id) {
            //There is a sibling, see if the current block is feasible with it
            bool sibling_feasible = primitive_memory_sibling_feasible(blk_id, cur_pb_type, sibling_memory_blk_id);
            if (!sibling_feasible) {
                return false;
            }
        }
    }

    //Generic feasibility check
    return primitive_type_feasible(blk_id, cur_pb_type);
}

bool primitive_memory_sibling_feasible(const AtomBlockId blk_id, const t_pb_type* cur_pb_type, const AtomBlockId sibling_blk_id) {
    /* Check that the two atom blocks blk_id and sibling_blk_id (which should both be memory slices)
     * are feasible, in the sence that they have precicely the same net connections (with the
     * exception of nets in data port classes).
     *
     * Note that this routine does not check pin feasibility against the cur_pb_type; so
     * primitive_type_feasible() should also be called on blk_id before concluding it is feasible.
     */
    auto& atom_ctx = g_vpr_ctx.atom();
    VTR_ASSERT(cur_pb_type->class_type == MEMORY_CLASS);

    //First, identify the 'data' ports by looking at the cur_pb_type
    std::unordered_set<t_model_ports*> data_ports;
    for (int iport = 0; iport < cur_pb_type->num_ports; ++iport) {
        const char* port_class = cur_pb_type->ports[iport].port_class;
        if (port_class && strstr(port_class, "data") == port_class) {
            //The port_class starts with "data", so it is a data port

            //Record the port
            data_ports.insert(cur_pb_type->ports[iport].model_port);
        }
    }

    //Now verify that all nets (except those connected to data ports) are equivalent
    //between blk_id and sibling_blk_id

    //Since the atom netlist stores only in-use ports, we iterate over the model to ensure
    //all ports are compared
    const t_model* model = cur_pb_type->model;
    for (t_model_ports* port : {model->inputs, model->outputs}) {
        for (; port; port = port->next) {
            if (data_ports.count(port)) {
                //Don't check data ports
                continue;
            }

            //Note: VPR doesn't support multi-driven nets, so all outputs
            //should be data ports, otherwise the siblings will both be
            //driving the output net

            //Get the ports from each primitive
            auto blk_port_id = atom_ctx.nlist.find_atom_port(blk_id, port);
            auto sib_port_id = atom_ctx.nlist.find_atom_port(sibling_blk_id, port);

            //Check that all nets (including unconnected nets) match
            for (int ipin = 0; ipin < port->size; ++ipin) {
                //The nets are initialized as invalid (i.e. disconnected)
                AtomNetId blk_net_id;
                AtomNetId sib_net_id;

                //We can get the actual net provided the port exists
                //
                //Note that if the port did not exist, the net is left
                //as invalid/disconneced
                if (blk_port_id) {
                    blk_net_id = atom_ctx.nlist.port_net(blk_port_id, ipin);
                }
                if (sib_port_id) {
                    sib_net_id = atom_ctx.nlist.port_net(sib_port_id, ipin);
                }

                //The sibling and block must have the same (possibly disconnected)
                //net on this pin
                if (blk_net_id != sib_net_id) {
                    //Nets do not match, not feasible
                    return false;
                }
            }
        }
    }

    return true;
}

/*****************************************/
t_pack_molecule* get_molecule_by_num_ext_inputs(const int ext_inps,
                                                const enum e_removal_policy remove_flag,
                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                t_molecule_link* unclustered_list_head) {
    /* This routine returns an atom block which has not been clustered, has  *
     * no connection to the current cluster, satisfies the cluster     *
     * clock constraints, is a valid subblock inside the cluster, does not exceed the cluster subblock units available,
     * and has ext_inps external inputs.  If        *
     * there is no such atom block it returns ClusterBlockId::INVALID().  Remove_flag      *
     * controls whether or not blocks that have already been clustered *
     * are removed from the unclustered_list data structures.  NB:     *
     * to get a atom block regardless of clock constraints just set clocks_ *
     * avail > 0.                                                      */

    t_molecule_link *ptr, *prev_ptr;
    int i;
    bool success;

    prev_ptr = &unclustered_list_head[ext_inps];
    ptr = unclustered_list_head[ext_inps].next;
    while (ptr != nullptr) {
        /* TODO: Get better candidate atom block in future, eg. return most timing critical or some other smarter metric */
        if (ptr->moleculeptr->valid) {
            success = true;
            for (i = 0; i < get_array_size_of_molecule(ptr->moleculeptr); i++) {
                if (ptr->moleculeptr->atom_block_ids[i]) {
                    auto blk_id = ptr->moleculeptr->atom_block_ids[i];
                    if (!exists_free_primitive_for_atom_block(cluster_placement_stats_ptr, blk_id)) {
                        /* TODO: I should be using a better filtering check especially when I'm
                         * dealing with multiple clock/multiple global reset signals where the clock/reset
                         * packed in matters, need to do later when I have the circuits to check my work */
                        success = false;
                        break;
                    }
                }
            }
            if (success == true) {
                return ptr->moleculeptr;
            }
            prev_ptr = ptr;
        }

        else if (remove_flag == REMOVE_CLUSTERED) {
            VTR_ASSERT(0); /* this doesn't work right now with 2 the pass packing for each complex block */
            prev_ptr->next = ptr->next;
        }

        ptr = ptr->next;
    }

    return nullptr;
}

/*****************************************/
t_pack_molecule* get_free_molecule_with_most_ext_inputs_for_cluster(t_pb* cur_pb,
                                                                    t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                    t_molecule_link* unclustered_list_head,
                                                                    const int& unclustered_list_head_size) {
    /* This routine is used to find new blocks for clustering when there are no feasible       *
     * blocks with any attraction to the current cluster (i.e. it finds       *
     * blocks which are unconnected from the current cluster).  It returns    *
     * the atom block with the largest number of used inputs that satisfies the    *
     * clocking and number of inputs constraints.  If no suitable atom block is    *
     * found, the routine returns ClusterBlockId::INVALID().
     * TODO: Analyze if this function is useful in more detail, also, should probably not include clock in input count
     */

    int inputs_avail = 0;

    for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
        inputs_avail += cur_pb->pb_stats->input_pins_used[i].size();
    }

    t_pack_molecule* molecule = nullptr;

    if (inputs_avail >= unclustered_list_head_size) {
        inputs_avail = unclustered_list_head_size - 1;
    }

    for (int ext_inps = inputs_avail; ext_inps >= 0; ext_inps--) {
        molecule = get_molecule_by_num_ext_inputs(ext_inps, LEAVE_CLUSTERED, cluster_placement_stats_ptr, unclustered_list_head);
        if (molecule != nullptr) {
            break;
        }
    }
    return molecule;
}

/*****************************************/
void alloc_and_load_pb_stats(t_pb* pb, const int feasible_block_array_size) {
    /* Call this routine when starting to fill up a new cluster.  It resets *
     * the gain vector, etc.                                                */

    pb->pb_stats = new t_pb_stats;

    /* If statement below is for speed.  If nets are reasonably low-fanout,  *
     * only a relatively small number of blocks will be marked, and updating *
     * only those atom block structures will be fastest.  If almost all blocks    *
     * have been touched it should be faster to just run through them all    *
     * in order (less addressing and better cache locality).                 */
    pb->pb_stats->input_pins_used = std::vector<std::unordered_map<size_t, AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
    pb->pb_stats->output_pins_used = std::vector<std::unordered_map<size_t, AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
    pb->pb_stats->lookahead_input_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_input_pin_class);
    pb->pb_stats->lookahead_output_pins_used = std::vector<std::vector<AtomNetId>>(pb->pb_graph_node->num_output_pin_class);
    pb->pb_stats->num_feasible_blocks = NOT_VALID;
    pb->pb_stats->feasible_blocks = (t_pack_molecule**)vtr::calloc(feasible_block_array_size, sizeof(t_pack_molecule*));

    pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID();

    pb->pb_stats->pulled_from_atom_groups = 0;
    pb->pb_stats->num_att_group_atoms_used = 0;

    pb->pb_stats->gain.clear();
    pb->pb_stats->timinggain.clear();
    pb->pb_stats->connectiongain.clear();
    pb->pb_stats->sharinggain.clear();
    pb->pb_stats->hillgain.clear();
    pb->pb_stats->transitive_fanout_candidates.clear();

    pb->pb_stats->num_pins_of_net_in_pb.clear();

    pb->pb_stats->num_child_blocks_in_pb = 0;

    pb->pb_stats->explore_transitive_fanout = true;
}
/*****************************************/

/**
 * Cleans up a pb after unsuccessful molecule packing
 *
 * Recursively frees pbs from a t_pb tree. The given root pb itself is not
 * deleted.
 *
 * If a pb object has its children allocated then before freeing them the
 * function checks if there is no atom that corresponds to any of them. The
 * check is performed only for leaf (primitive) pbs. The function recurses for
 * non-primitive pbs.
 *
 * The cleaning itself includes deleting all child pbs, resetting mode of the
 * pb and also freeing its name. This prepares the pb for another round of
 * molecule packing tryout. 
 */
bool cleanup_pb(t_pb* pb) {
    bool can_free = true;

    /* Recursively check if there are any children with already assigned atoms */
    if (pb->child_pbs != nullptr) {
        const t_mode* mode = &pb->pb_graph_node->pb_type->modes[pb->mode];
        VTR_ASSERT(mode != nullptr);

        /* Check each mode */
        for (int i = 0; i < mode->num_pb_type_children; ++i) {
            /* Check each child */
            if (pb->child_pbs[i] != nullptr) {
                for (int j = 0; j < mode->pb_type_children[i].num_pb; ++j) {
                    t_pb* pb_child = &pb->child_pbs[i][j];
                    t_pb_type* pb_type = pb_child->pb_graph_node->pb_type;

                    /* Primitive, check occupancy */
                    if (pb_type->num_modes == 0) {
                        if (pb_child->name != nullptr) {
                            can_free = false;
                        }
                    }

                    /* Non-primitive, recurse */
                    else {
                        if (!cleanup_pb(pb_child)) {
                            can_free = false;
                        }
                    }
                }
            }
        }

        /* Free if can */
        if (can_free) {
            for (int i = 0; i < mode->num_pb_type_children; ++i) {
                if (pb->child_pbs[i] != nullptr) {
                    delete[] pb->child_pbs[i];
                }
            }

            delete[] pb->child_pbs;
            pb->child_pbs = nullptr;
            pb->mode = 0;

            if (pb->name) {
                free(pb->name);
                pb->name = nullptr;
            }
        }
    }

    return can_free;
}

/**
 * Performs legality checks to see whether the selected molecule can be
 * packed into the current cluster. The legality checks are related to
 * floorplanning, pin feasibility, and routing (if detailed route
 * checking is enabled). The routine returns BLK_PASSED if the molecule
 * can be packed in the cluster. If the block passes, the routine commits
 * it to the current cluster and updates the appropriate data structures.
 * Otherwise, it returns the appropriate failed pack status based on which
 * legality check the molecule failed.
 */
enum e_block_pack_status try_pack_molecule(t_cluster_placement_stats* cluster_placement_stats_ptr,
                                           t_pack_molecule* molecule,
                                           t_pb_graph_node** primitives_list,
                                           t_pb* pb,
                                           const int max_models,
                                           const int max_cluster_size,
                                           const ClusterBlockId clb_index,
                                           const int detailed_routing_stage,
                                           t_lb_router_data* router_data,
                                           int verbosity,
                                           bool enable_pin_feasibility_filter,
                                           const int feasible_block_array_size,
                                           t_ext_pin_util max_external_pin_util,
                                           PartitionRegion& temp_cluster_pr) {
    int molecule_size, failed_location;
    int i;
    enum e_block_pack_status block_pack_status;
    t_pb* parent;
    t_pb* cur_pb;

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    parent = nullptr;

    block_pack_status = BLK_STATUS_UNDEFINED;

    molecule_size = get_array_size_of_molecule(molecule);
    failed_location = 0;

    if (verbosity > 3) {
        AtomBlockId root_atom = molecule->atom_block_ids[molecule->root];
        VTR_LOG("\t\tTry pack molecule: '%s' (%s)",
                atom_ctx.nlist.block_name(root_atom).c_str(),
                atom_ctx.nlist.block_model(root_atom)->name);
        VTR_LOGV(molecule->pack_pattern,
                 " molecule_type %s molecule_size %zu",
                 molecule->pack_pattern->name,
                 molecule->atom_block_ids.size());
        VTR_LOG("\n");
    }

    // if this cluster has a molecule placed in it that is part of a long chain
    // (a chain that consists of more than one molecule), don't allow more long chain
    // molecules to be placed in this cluster. To avoid possibly creating cluster level
    // blocks that have incompatible placement constraints or form very long placement
    // macros that limit placement flexibility.
    if (cluster_placement_stats_ptr->has_long_chain && molecule->is_chain() && molecule->chain_info->is_long_chain) {
        VTR_LOGV(verbosity > 4, "\t\t\tFAILED Placement Feasibility Filter: Only one long chain per cluster is allowed\n");
        //Record the failure of this molecule in the current pb stats
        record_molecule_failure(molecule, pb);
        return BLK_FAILED_FEASIBLE;
    }

    bool cluster_pr_needs_update = false;
    bool cluster_pr_update_check = false;

    //check if every atom in the molecule is legal in the cluster from a floorplanning perspective
    for (int i_mol = 0; i_mol < molecule_size; i_mol++) {
        //try to intersect with atom PartitionRegion if atom exists
        if (molecule->atom_block_ids[i_mol]) {
            block_pack_status = atom_cluster_floorplanning_check(molecule->atom_block_ids[i_mol],
                                                                 clb_index, verbosity,
                                                                 temp_cluster_pr,
                                                                 cluster_pr_needs_update);
            if (block_pack_status == BLK_FAILED_FLOORPLANNING) {
                //Record the failure of this molecule in the current pb stats
                record_molecule_failure(molecule, pb);
                return block_pack_status;
            }
            if (cluster_pr_needs_update == true) {
                cluster_pr_update_check = true;
            }
        }
    }

    //change  status back to undefined before the while loop in case in was changed to BLK_PASSED in the above for loop
    block_pack_status = BLK_STATUS_UNDEFINED;

    while (block_pack_status != BLK_PASSED) {
        if (get_next_primitive_list(cluster_placement_stats_ptr, molecule,
                                    primitives_list)) {
            block_pack_status = BLK_PASSED;

            for (i = 0; i < molecule_size && block_pack_status == BLK_PASSED; i++) {
                VTR_ASSERT((primitives_list[i] == nullptr) == (!molecule->atom_block_ids[i]));
                failed_location = i + 1;
                // try place atom block if it exists
                if (molecule->atom_block_ids[i]) {
                    block_pack_status = try_place_atom_block_rec(primitives_list[i],
                                                                 molecule->atom_block_ids[i], pb, &parent,
                                                                 max_models, max_cluster_size, clb_index,
                                                                 cluster_placement_stats_ptr, molecule, router_data,
                                                                 verbosity, feasible_block_array_size);
                }
            }

            if (enable_pin_feasibility_filter && block_pack_status == BLK_PASSED) {
                /* Check if pin usage is feasible for the current packing assignment */
                reset_lookahead_pins_used(pb);
                try_update_lookahead_pins_used(pb);
                if (!check_lookahead_pins_used(pb, max_external_pin_util)) {
                    VTR_LOGV(verbosity > 4, "\t\t\tFAILED Pin Feasibility Filter\n");
                    block_pack_status = BLK_FAILED_FEASIBLE;
                }
            }
            if (block_pack_status == BLK_PASSED) {
                /*
                 * during the clustering step of `do_clustering`, `detailed_routing_stage` is incremented at each iteration until it a cluster
                 * is correctly generated or `detailed_routing_stage` assumes an invalid value (E_DETAILED_ROUTE_INVALID).
                 * depending on its value we have different behaviors:
                 *  - E_DETAILED_ROUTE_AT_END_ONLY: Skip routing if heuristic is to route at the end of packing complex block.
                 *  - E_DETAILED_ROUTE_FOR_EACH_ATOM: Try to route if heuristic is to route for every atom. If the clusterer arrives at this stage,
                 *                                    it means that more checks have to be performed as the previous stage failed to generate a new cluster.
                 *
                 * mode_status is a data structure containing the status of the mode selection. Its members are:
                 *  - bool is_mode_conflict
                 *  - bool try_expand_all_modes
                 *  - bool expand_all_modes
                 *
                 * is_mode_conflict affects this stage. Its value determines whether the cluster failed to pack after a mode conflict issue.
                 * It holds a flag that is used to verify whether try_intra_lb_route ended in a mode conflict issue.
                 *
                 * Until is_mode_conflict is set to FALSE by try_intra_lb_route, the loop re-iterates. If all the available modes are exhausted
                 * an error will be thrown during mode conflicts checks (this to prevent infinite loops).
                 *
                 * If the value is TRUE the cluster has to be re-routed, and its internal pb_graph_nodes will have more restrict choices
                 * for what regards the mode that has to be selected.
                 *
                 * is_mode_conflict is initially set to TRUE, and, unless a mode conflict is found, it is set to false in `try_intra_lb_route`.
                 *
                 * try_expand_all_modes is set if the node expansion failed to find a valid routing path. The clusterer tries to find another route
                 * by using all the modes during node expansion.
                 *
                 * expand_all_modes is used to enable the expansion of all the nodes using all the possible modes.
                 */
                t_mode_selection_status mode_status;
                bool is_routed = false;
                bool do_detailed_routing_stage = detailed_routing_stage == (int)E_DETAILED_ROUTE_FOR_EACH_ATOM;
                if (do_detailed_routing_stage) {
                    do {
                        reset_intra_lb_route(router_data);
                        is_routed = try_intra_lb_route(router_data, verbosity, &mode_status);
                    } while (do_detailed_routing_stage && mode_status.is_mode_issue());
                }

                if (do_detailed_routing_stage && is_routed == false) {
                    /* Cannot pack */
                    VTR_LOGV(verbosity > 4, "\t\t\tFAILED Detailed Routing Legality\n");
                    block_pack_status = BLK_FAILED_ROUTE;
                } else {
                    /* Pack successful, commit
                     * TODO: SW Engineering note - may want to update cluster stats here too instead of doing it outside
                     */
                    VTR_ASSERT(block_pack_status == BLK_PASSED);
                    if (molecule->is_chain()) {
                        /* Chained molecules often take up lots of area and are important,
                         * if a chain is packed in, want to rename logic block to match chain name */
                        AtomBlockId chain_root_blk_id = molecule->atom_block_ids[molecule->pack_pattern->root_block->block_id];
                        cur_pb = atom_ctx.lookup.atom_pb(chain_root_blk_id)->parent_pb;
                        while (cur_pb != nullptr) {
                            free(cur_pb->name);
                            cur_pb->name = vtr::strdup(atom_ctx.nlist.block_name(chain_root_blk_id).c_str());
                            cur_pb = cur_pb->parent_pb;
                        }
                        // if this molecule is part of a chain, mark the cluster as having a long chain
                        // molecule. Also check if it's the first molecule in the chain to be packed.
                        // If so, update the chain id for this chain of molecules to make sure all
                        // molecules will be packed to the same chain id and can reach each other using
                        // the chain direct links between clusters
                        if (molecule->chain_info->is_long_chain) {
                            cluster_placement_stats_ptr->has_long_chain = true;
                            if (molecule->chain_info->chain_id == -1) {
                                update_molecule_chain_info(molecule, primitives_list[molecule->root]);
                            }
                        }
                    }

                    //update cluster PartitionRegion if atom with floorplanning constraints was added
                    if (cluster_pr_update_check) {
                        floorplanning_ctx.cluster_constraints[clb_index] = temp_cluster_pr;
                        if (verbosity > 2) {
                            VTR_LOG("\nUpdated PartitionRegion of cluster %d\n", clb_index);
                        }
                    }

                    for (i = 0; i < molecule_size; i++) {
                        if (molecule->atom_block_ids[i]) {
                            /* invalidate all molecules that share atom block with current molecule */

                            auto rng = atom_ctx.atom_molecules.equal_range(molecule->atom_block_ids[i]);
                            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                                t_pack_molecule* cur_molecule = kv.second;
                                cur_molecule->valid = false;
                            }

                            commit_primitive(cluster_placement_stats_ptr, primitives_list[i]);
                        }
                    }
                }
            }

            if (block_pack_status != BLK_PASSED) {
                for (i = 0; i < failed_location; i++) {
                    if (molecule->atom_block_ids[i]) {
                        remove_atom_from_target(router_data, molecule->atom_block_ids[i]);
                    }
                }
                for (i = 0; i < failed_location; i++) {
                    if (molecule->atom_block_ids[i]) {
                        revert_place_atom_block(molecule->atom_block_ids[i], router_data);
                    }
                }

                //Record the failure of this molecule in the current pb stats
                record_molecule_failure(molecule, pb);

                /* Packing failed, but a part of the pb tree is still allocated and pbs have their modes set.
                 * Before trying to pack next molecule the unused pbs need to be freed and, the most important,
                 * their modes reset. This task is performed by the cleanup_pb() function below. */
                cleanup_pb(pb);

            } else {
                VTR_LOGV(verbosity > 3, "\t\tPASSED pack molecule\n");
            }
        } else {
            VTR_LOGV(verbosity > 3, "\t\tFAILED No candidate primitives available\n");
            block_pack_status = BLK_FAILED_FEASIBLE;
            break; /* no more candidate primitives available, this molecule will not pack, return fail */
        }
    }
    return block_pack_status;
}

/* Record the failure of the molecule in this cluster in the current pb stats.
 * If a molecule fails repeatedly, it's gain will be penalized if packing with
 * attraction groups on. */
void record_molecule_failure(t_pack_molecule* molecule, t_pb* pb) {
    //Only have to record the failure for the first atom in the molecule.
    //The convention when checking if a molecule has failed to pack in the cluster
    //is to check whether the first atoms has been recorded as having failed

    auto got = pb->pb_stats->atom_failures.find(molecule->atom_block_ids[0]);
    if (got == pb->pb_stats->atom_failures.end()) {
        pb->pb_stats->atom_failures.insert({molecule->atom_block_ids[0], 1});
    } else {
        got->second++;
    }
}

/**
 * Try place atom block into current primitive location
 */

enum e_block_pack_status try_place_atom_block_rec(const t_pb_graph_node* pb_graph_node,
                                                  const AtomBlockId blk_id,
                                                  t_pb* cb,
                                                  t_pb** parent,
                                                  const int max_models,
                                                  const int max_cluster_size,
                                                  const ClusterBlockId clb_index,
                                                  const t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                  const t_pack_molecule* molecule,
                                                  t_lb_router_data* router_data,
                                                  int verbosity,
                                                  const int feasible_block_array_size) {
    int i, j;
    bool is_primitive;
    enum e_block_pack_status block_pack_status;

    t_pb* my_parent;
    t_pb *pb, *parent_pb;
    const t_pb_type* pb_type;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    my_parent = nullptr;

    block_pack_status = BLK_PASSED;

    /* Discover parent */
    if (pb_graph_node->parent_pb_graph_node != cb->pb_graph_node) {
        block_pack_status = try_place_atom_block_rec(pb_graph_node->parent_pb_graph_node, blk_id, cb,
                                                     &my_parent, max_models, max_cluster_size, clb_index,
                                                     cluster_placement_stats_ptr, molecule, router_data,
                                                     verbosity, feasible_block_array_size);
        parent_pb = my_parent;
    } else {
        parent_pb = cb;
    }

    /* Create siblings if siblings are not allocated */
    if (parent_pb->child_pbs == nullptr) {
        atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), parent_pb);

        VTR_ASSERT(parent_pb->name == nullptr);
        parent_pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());
        parent_pb->mode = pb_graph_node->pb_type->parent_mode->index;
        set_reset_pb_modes(router_data, parent_pb, true);
        const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
        parent_pb->child_pbs = new t_pb*[mode->num_pb_type_children];

        for (i = 0; i < mode->num_pb_type_children; i++) {
            parent_pb->child_pbs[i] = new t_pb[mode->pb_type_children[i].num_pb];

            for (j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                parent_pb->child_pbs[i][j].parent_pb = parent_pb;

                atom_ctx.lookup.set_atom_pb(AtomBlockId::INVALID(), &parent_pb->child_pbs[i][j]);

                parent_pb->child_pbs[i][j].pb_graph_node = &(parent_pb->pb_graph_node->child_pb_graph_nodes[parent_pb->mode][i][j]);
            }
        }
    } else {
        VTR_ASSERT(parent_pb->mode == pb_graph_node->pb_type->parent_mode->index);
    }

    const t_mode* mode = &parent_pb->pb_graph_node->pb_type->modes[parent_pb->mode];
    for (i = 0; i < mode->num_pb_type_children; i++) {
        if (pb_graph_node->pb_type == &mode->pb_type_children[i]) {
            break;
        }
    }
    VTR_ASSERT(i < mode->num_pb_type_children);
    pb = &parent_pb->child_pbs[i][pb_graph_node->placement_index];
    *parent = pb; /* this pb is parent of it's child that called this function */
    VTR_ASSERT(pb->pb_graph_node == pb_graph_node);
    if (pb->pb_stats == nullptr) {
        alloc_and_load_pb_stats(pb, feasible_block_array_size);
    }
    pb_type = pb_graph_node->pb_type;

    /* Any pb_type under an mode, which is disabled for packing, should not be considerd for mapping 
     * Early exit to flag failure
     */
    if (true == pb_type->parent_mode->disable_packing) {
        return BLK_FAILED_FEASIBLE;
    }

    is_primitive = (pb_type->num_modes == 0);

    if (is_primitive) {
        VTR_ASSERT(!atom_ctx.lookup.pb_atom(pb)
                   && atom_ctx.lookup.atom_pb(blk_id) == nullptr
                   && atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID());
        /* try pack to location */
        VTR_ASSERT(pb->name == nullptr);
        pb->name = vtr::strdup(atom_ctx.nlist.block_name(blk_id).c_str());

        //Update the atom netlist mappings
        atom_ctx.lookup.set_atom_clb(blk_id, clb_index);
        atom_ctx.lookup.set_atom_pb(blk_id, pb);

        add_atom_as_target(router_data, blk_id);
        if (!primitive_feasible(blk_id, pb)) {
            /* failed location feasibility check, revert pack */
            block_pack_status = BLK_FAILED_FEASIBLE;
        }

        // if this block passed and is part of a chained molecule
        if (block_pack_status == BLK_PASSED && molecule->is_chain()) {
            auto molecule_root_block = molecule->atom_block_ids[molecule->root];
            // if this is the root block of the chain molecule check its placmeent feasibility
            if (blk_id == molecule_root_block) {
                block_pack_status = check_chain_root_placement_feasibility(pb_graph_node, molecule, blk_id);
            }
        }

        VTR_LOGV(verbosity > 4 && block_pack_status == BLK_PASSED,
                 "\t\t\tPlaced atom '%s' (%s) at %s\n",
                 atom_ctx.nlist.block_name(blk_id).c_str(),
                 atom_ctx.nlist.block_model(blk_id)->name,
                 pb->hierarchical_type_name().c_str());
    }

    if (block_pack_status != BLK_PASSED) {
        free(pb->name);
        pb->name = nullptr;
    }
    return block_pack_status;
}

/*
 * Checks if the atom and cluster have compatible floorplanning constraints
 * If the atom and cluster both have non-empty PartitionRegions, and the intersection
 * of the PartitionRegions is empty, the atom cannot be packed in the cluster.
 */
enum e_block_pack_status atom_cluster_floorplanning_check(const AtomBlockId blk_id,
                                                          const ClusterBlockId clb_index,
                                                          const int verbosity,
                                                          PartitionRegion& temp_cluster_pr,
                                                          bool& cluster_pr_needs_update) {
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    /*check if the atom can go in the cluster by checking if the atom and cluster have intersecting PartitionRegions*/

    //get partition that atom belongs to
    PartitionId partid;
    partid = floorplanning_ctx.constraints.get_atom_partition(blk_id);

    PartitionRegion atom_pr;
    PartitionRegion cluster_pr;

    //if the atom does not belong to a partition, it can be put in the cluster
    //regardless of what the cluster's PartitionRegion is because it has no constraints
    if (partid == PartitionId::INVALID()) {
        if (verbosity > 3) {
            VTR_LOG("\t\t\t Intersect: Atom block %d has no floorplanning constraints, passed for cluster %d \n", blk_id, clb_index);
        }
        cluster_pr_needs_update = false;
        return BLK_PASSED;
    } else {
        //get pr of that partition
        atom_pr = floorplanning_ctx.constraints.get_partition_pr(partid);

        //intersect it with the pr of the current cluster
        cluster_pr = floorplanning_ctx.cluster_constraints[clb_index];

        if (cluster_pr.empty() == true) {
            temp_cluster_pr = atom_pr;
            cluster_pr_needs_update = true;
            if (verbosity > 3) {
                VTR_LOG("\t\t\t Intersect: Atom block %d has floorplanning constraints, passed cluster %d which has empty PR\n", blk_id, clb_index);
            }
            return BLK_PASSED;
        } else {
            //update cluster_pr with the intersection of the cluster's PartitionRegion
            //and the atom's PartitionRegion
            update_cluster_part_reg(cluster_pr, atom_pr);
        }

        if (cluster_pr.empty() == true) {
            if (verbosity > 3) {
                VTR_LOG("\t\t\t Intersect: Atom block %d failed floorplanning check for cluster %d \n", blk_id, clb_index);
            }
            cluster_pr_needs_update = false;
            return BLK_FAILED_FLOORPLANNING;
        } else {
            //update the cluster's PartitionRegion with the intersecting PartitionRegion
            temp_cluster_pr = cluster_pr;
            cluster_pr_needs_update = true;
            if (verbosity > 3) {
                VTR_LOG("\t\t\t Intersect: Atom block %d passed cluster %d, cluster PR was updated with intersection result \n", blk_id, clb_index);
            }
            return BLK_PASSED;
        }
    }
}

/* Revert trial atom block iblock and free up memory space accordingly
 */
void revert_place_atom_block(const AtomBlockId blk_id, t_lb_router_data* router_data) {
    auto& atom_ctx = g_vpr_ctx.mutable_atom();

    //We cast away const here since we may free the pb, and it is
    //being removed from the active mapping.
    //
    //In general most code works fine accessing cosnt t_pb*,
    //which is why we store them as such in atom_ctx.lookup
    t_pb* pb = const_cast<t_pb*>(atom_ctx.lookup.atom_pb(blk_id));

    if (pb != nullptr) {
        /* When freeing molecules, the current block might already have been freed by a prior revert
         * When this happens, no need to do anything beyond basic book keeping at the atom block
         */

        t_pb* next = pb->parent_pb;
        revalid_molecules(pb);
        free_pb(pb);
        pb = next;

        while (pb != nullptr) {
            /* If this is pb is created only for the purposes of holding new molecule, remove it
             * Must check if cluster is already freed (which can be the case)
             */
            next = pb->parent_pb;

            if (pb->child_pbs != nullptr && pb->pb_stats != nullptr
                && pb->pb_stats->num_child_blocks_in_pb == 0) {
                set_reset_pb_modes(router_data, pb, false);
                if (next != nullptr) {
                    /* If the code gets here, then that means that placing the initial seed molecule
                     * failed, don't free the actual complex block itself as the seed needs to find
                     * another placement */
                    revalid_molecules(pb);
                    free_pb(pb);
                }
            }
            pb = next;
        }
    }

    //Update the atom netlist mapping
    atom_ctx.lookup.set_atom_clb(blk_id, ClusterBlockId::INVALID());
    atom_ctx.lookup.set_atom_pb(blk_id, nullptr);
}

void update_connection_gain_values(const AtomNetId net_id, const AtomBlockId clustered_blk_id, t_pb* cur_pb, enum e_net_relation_to_clustered_block net_relation_to_clustered_block) {
    /*This function is called when the connectiongain values on the net net_id*
     *require updating.   */

    int num_internal_connections, num_open_connections, num_stuck_connections;

    num_internal_connections = num_open_connections = num_stuck_connections = 0;

    auto& atom_ctx = g_vpr_ctx.atom();
    ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(clustered_blk_id);

    /* may wish to speed things up by ignoring clock nets since they are high fanout */

    for (auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
        auto blk_id = atom_ctx.nlist.pin_block(pin_id);
        if (atom_ctx.lookup.atom_clb(blk_id) == clb_index
            && is_atom_blk_in_pb(blk_id, atom_ctx.lookup.atom_pb(clustered_blk_id))) {
            num_internal_connections++;
        } else if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
            num_open_connections++;
        } else {
            num_stuck_connections++;
        }
    }

    if (net_relation_to_clustered_block == OUTPUT) {
        for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
            VTR_ASSERT(blk_id);

            if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
                /* TODO: Gain function accurate only if net has one connection to block,
                 * TODO: Should we handle case where net has multi-connection to block?
                 *       Gain computation is only off by a bit in this case */
                if (cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
                    cur_pb->pb_stats->connectiongain[blk_id] = 0;
                }

                if (num_internal_connections > 1) {
                    cur_pb->pb_stats->connectiongain[blk_id] -= 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 1 + 0.1);
                }
                cur_pb->pb_stats->connectiongain[blk_id] += 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1);
            }
        }
    }

    if (net_relation_to_clustered_block == INPUT) {
        /*Calculate the connectiongain for the atom block which is driving *
         *the atom net that is an input to an atom block in the cluster */

        auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
        auto blk_id = atom_ctx.nlist.pin_block(driver_pin_id);

        if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
            if (cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
                cur_pb->pb_stats->connectiongain[blk_id] = 0;
            }
            if (num_internal_connections > 1) {
                cur_pb->pb_stats->connectiongain[blk_id] -= 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1 + 1);
            }
            cur_pb->pb_stats->connectiongain[blk_id] += 1 / (float)(num_open_connections + 1.5 * num_stuck_connections + 0.1);
        }
    }
}

void try_fill_cluster(const t_packer_opts& packer_opts,
                      t_cluster_placement_stats* cur_cluster_placement_stats_ptr,
                      t_pack_molecule*& prev_molecule,
                      t_pack_molecule*& next_molecule,
                      int& num_same_molecules,
                      t_pb_graph_node** primitives_list,
                      t_cluster_progress_stats& cluster_stats,
                      int num_clb,
                      const int num_models,
                      const int max_cluster_size,
                      const ClusterBlockId clb_index,
                      const int detailed_routing_stage,
                      AttractionInfo& attraction_groups,
                      vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                      bool allow_unrelated_clustering,
                      const int& high_fanout_threshold,
                      const std::unordered_set<AtomNetId>& is_clock,
                      const std::shared_ptr<SetupTimingInfo>& timing_info,
                      t_lb_router_data* router_data,
                      t_ext_pin_util target_ext_pin_util,
                      PartitionRegion& temp_cluster_pr,
                      e_block_pack_status& block_pack_status,
                      t_molecule_link* unclustered_list_head,
                      const int& unclustered_list_head_size,
                      std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input,
                      std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    block_pack_status = try_pack_molecule(cur_cluster_placement_stats_ptr,
                                          next_molecule,
                                          primitives_list,
                                          cluster_ctx.clb_nlist.block_pb(clb_index),
                                          num_models,
                                          max_cluster_size,
                                          clb_index,
                                          detailed_routing_stage,
                                          router_data,
                                          packer_opts.pack_verbosity,
                                          packer_opts.enable_pin_feasibility_filter,
                                          packer_opts.feasible_block_array_size,
                                          target_ext_pin_util,
                                          temp_cluster_pr);

    auto blk_id = next_molecule->atom_block_ids[next_molecule->root];
    VTR_ASSERT(blk_id);

    std::string blk_name = atom_ctx.nlist.block_name(blk_id);
    const t_model* blk_model = atom_ctx.nlist.block_model(blk_id);

    if (block_pack_status != BLK_PASSED) {
        if (packer_opts.pack_verbosity > 2) {
            if (block_pack_status == BLK_FAILED_ROUTE) {
                VTR_LOG("\tNO_ROUTE: '%s' (%s)", blk_name.c_str(), blk_model->name);
                VTR_LOGV(next_molecule->pack_pattern, " molecule %s molecule_size %zu",
                         next_molecule->pack_pattern->name, next_molecule->atom_block_ids.size());
                VTR_LOG("\n");
                fflush(stdout);
            } else if (block_pack_status == BLK_FAILED_FLOORPLANNING) {
                VTR_LOG("\tFAILED_FLOORPLANNING_CONSTRAINTS_CHECK: '%s' (%s)", blk_name.c_str(), blk_model->name);
                VTR_LOG("\n");
            } else {
                VTR_LOG("\tFAILED_FEASIBILITY_CHECK: '%s' (%s)", blk_name.c_str(), blk_model->name, block_pack_status);
                VTR_LOGV(next_molecule->pack_pattern, " molecule %s molecule_size %zu",
                         next_molecule->pack_pattern->name, next_molecule->atom_block_ids.size());
                VTR_LOG("\n");
                fflush(stdout);
            }
        }

        next_molecule = get_molecule_for_cluster(cluster_ctx.clb_nlist.block_pb(clb_index),
                                                 attraction_groups,
                                                 allow_unrelated_clustering,
                                                 packer_opts.prioritize_transitive_connectivity,
                                                 packer_opts.transitive_fanout_threshold,
                                                 packer_opts.feasible_block_array_size,
                                                 &cluster_stats.num_unrelated_clustering_attempts,
                                                 cur_cluster_placement_stats_ptr,
                                                 clb_inter_blk_nets,
                                                 clb_index, packer_opts.pack_verbosity,
                                                 unclustered_list_head,
                                                 unclustered_list_head_size,
                                                 primitive_candidate_block_types);
        if (prev_molecule == next_molecule) {
            num_same_molecules++;
        }
        return;
    }

    /* Continue packing by filling smallest cluster */
    if (packer_opts.pack_verbosity > 2) {
        VTR_LOG("\tPASSED: '%s' (%s)", blk_name.c_str(), blk_model->name);
        VTR_LOGV(next_molecule->pack_pattern, " molecule %s molecule_size %zu",
                 next_molecule->pack_pattern->name, next_molecule->atom_block_ids.size());
        VTR_LOG("\n");
    }

    fflush(stdout);

    //Since molecule passed, update num_molecules_processed
    cluster_stats.num_molecules_processed++;
    cluster_stats.mols_since_last_print++;
    print_pack_status(num_clb, cluster_stats.num_molecules,
                      cluster_stats.num_molecules_processed,
                      cluster_stats.mols_since_last_print,
                      device_ctx.grid.width(),
                      device_ctx.grid.height(),
                      attraction_groups);

    update_cluster_stats(next_molecule, clb_index,
                         is_clock, //Set of all clocks
                         is_clock, //Set of all global signals (currently clocks)
                         packer_opts.global_clocks, packer_opts.alpha, packer_opts.beta, packer_opts.timing_driven,
                         packer_opts.connection_driven,
                         high_fanout_threshold,
                         *timing_info,
                         attraction_groups,
                         net_output_feeds_driving_block_input);
    cluster_stats.num_unrelated_clustering_attempts = 0;

    if (packer_opts.timing_driven) {
        cluster_stats.blocks_since_last_analysis++; /* historically, timing slacks were recomputed after X number of blocks were packed, but this doesn't significantly alter results so I (jluu) did not port the code */
    }
    next_molecule = get_molecule_for_cluster(cluster_ctx.clb_nlist.block_pb(clb_index),
                                             attraction_groups,
                                             allow_unrelated_clustering,
                                             packer_opts.prioritize_transitive_connectivity,
                                             packer_opts.transitive_fanout_threshold,
                                             packer_opts.feasible_block_array_size,
                                             &cluster_stats.num_unrelated_clustering_attempts,
                                             cur_cluster_placement_stats_ptr,
                                             clb_inter_blk_nets,
                                             clb_index,
                                             packer_opts.pack_verbosity,
                                             unclustered_list_head,
                                             unclustered_list_head_size,
                                             primitive_candidate_block_types);

    if (prev_molecule == next_molecule) {
        num_same_molecules++;
    }
}

t_pack_molecule* save_cluster_routing_and_pick_new_seed(const t_packer_opts& packer_opts,
                                                        const int& num_clb,
                                                        const std::vector<AtomBlockId>& seed_atoms,
                                                        const int& num_blocks_hill_added,
                                                        vtr::vector<ClusterBlockId, std::vector<t_intra_lb_net>*>& intra_lb_routing,
                                                        int& seedindex,
                                                        t_cluster_progress_stats& cluster_stats,
                                                        t_lb_router_data* router_data) {
    t_pack_molecule* next_seed = nullptr;

    intra_lb_routing.push_back(router_data->saved_lb_nets);
    VTR_ASSERT((int)intra_lb_routing.size() == num_clb);
    router_data->saved_lb_nets = nullptr;

    //Pick a new seed
    next_seed = get_highest_gain_seed_molecule(&seedindex, seed_atoms);

    if (packer_opts.timing_driven) {
        if (num_blocks_hill_added > 0) {
            cluster_stats.blocks_since_last_analysis += num_blocks_hill_added;
        }
    }
    return next_seed;
}

void store_cluster_info_and_free(const t_packer_opts& packer_opts,
                                 const ClusterBlockId& clb_index,
                                 const t_logical_block_type_ptr logic_block_type,
                                 const t_pb_type* le_pb_type,
                                 std::vector<int>& le_count,
                                 vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();
    auto& atom_ctx = g_vpr_ctx.atom();

    /* store info that will be used later in packing from pb_stats and free the rest */
    t_pb_stats* pb_stats = cluster_ctx.clb_nlist.block_pb(clb_index)->pb_stats;
    for (const AtomNetId mnet_id : pb_stats->marked_nets) {
        int external_terminals = atom_ctx.nlist.net_pins(mnet_id).size() - pb_stats->num_pins_of_net_in_pb[mnet_id];
        /* Check if external terminals of net is within the fanout limit and that there exists external terminals */
        if (external_terminals < packer_opts.transitive_fanout_threshold && external_terminals > 0) {
            clb_inter_blk_nets[clb_index].push_back(mnet_id);
        }
    }
    auto cur_pb = cluster_ctx.clb_nlist.block_pb(clb_index);

    // update the data structure holding the LE counts
    update_le_count(cur_pb, logic_block_type, le_pb_type, le_count);

    //print clustering progress incrementally
    //print_pack_status(num_clb, num_molecules, num_molecules_processed, mols_since_last_print, device_ctx.grid.width(), device_ctx.grid.height());

    free_pb_stats_recursive(cur_pb);
}

/* Free up data structures and requeue used molecules */
void free_data_and_requeue_used_mols_if_illegal(const ClusterBlockId& clb_index,
                                                const int& savedseedindex,
                                                std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                                                int& num_clb,
                                                int& seedindex) {
    auto& cluster_ctx = g_vpr_ctx.mutable_clustering();

    num_used_type_instances[cluster_ctx.clb_nlist.block_type(clb_index)]--;
    revalid_molecules(cluster_ctx.clb_nlist.block_pb(clb_index));
    cluster_ctx.clb_nlist.remove_block(clb_index);
    cluster_ctx.clb_nlist.compress();
    num_clb--;
    seedindex = savedseedindex;
}

/*****************************************/
void update_timing_gain_values(const AtomNetId net_id,
                               t_pb* cur_pb,
                               enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                               const SetupTimingInfo& timing_info,
                               const std::unordered_set<AtomNetId>& is_global,
                               std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input) {
    /*This function is called when the timing_gain values on the atom net*
     *net_id requires updating.   */
    float timinggain;

    auto& atom_ctx = g_vpr_ctx.atom();

    /* Check if this atom net lists its driving atom block twice.  If so, avoid  *
     * double counting this atom block by skipping the first (driving) pin. */
    auto pins = atom_ctx.nlist.net_pins(net_id);
    if (net_output_feeds_driving_block_input[net_id] != 0)
        pins = atom_ctx.nlist.net_sinks(net_id);

    if (net_relation_to_clustered_block == OUTPUT
        && !is_global.count(net_id)) {
        for (auto pin_id : pins) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
            if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
                timinggain = timing_info.setup_pin_criticality(pin_id);

                if (cur_pb->pb_stats->timinggain.count(blk_id) == 0) {
                    cur_pb->pb_stats->timinggain[blk_id] = 0;
                }
                if (timinggain > cur_pb->pb_stats->timinggain[blk_id])
                    cur_pb->pb_stats->timinggain[blk_id] = timinggain;
            }
        }
    }

    if (net_relation_to_clustered_block == INPUT
        && !is_global.count(net_id)) {
        /*Calculate the timing gain for the atom block which is driving *
         *the atom net that is an input to a atom block in the cluster */
        auto driver_pin = atom_ctx.nlist.net_driver(net_id);
        auto new_blk_id = atom_ctx.nlist.pin_block(driver_pin);

        if (atom_ctx.lookup.atom_clb(new_blk_id) == ClusterBlockId::INVALID()) {
            for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                timinggain = timing_info.setup_pin_criticality(pin_id);

                if (cur_pb->pb_stats->timinggain.count(new_blk_id) == 0) {
                    cur_pb->pb_stats->timinggain[new_blk_id] = 0;
                }
                if (timinggain > cur_pb->pb_stats->timinggain[new_blk_id])
                    cur_pb->pb_stats->timinggain[new_blk_id] = timinggain;
            }
        }
    }
}

/*****************************************/
void mark_and_update_partial_gain(const AtomNetId net_id,
                                  enum e_gain_update gain_flag,
                                  const AtomBlockId clustered_blk_id,
                                  bool timing_driven,
                                  bool connection_driven,
                                  enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                                  const SetupTimingInfo& timing_info,
                                  const std::unordered_set<AtomNetId>& is_global,
                                  const int high_fanout_net_threshold,
                                  std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input) {
    /* Updates the marked data structures, and if gain_flag is GAIN,  *
     * the gain when an atom block is added to a cluster.  The        *
     * sharinggain is the number of inputs that a atom block shares with   *
     * blocks that are already in the cluster. Hillgain is the        *
     * reduction in number of pins-required by adding a atom block to the  *
     * cluster. The timinggain is the criticality of the most critical*
     * atom net between this atom block and an atom block in the cluster.             */

    auto& atom_ctx = g_vpr_ctx.atom();
    t_pb* cur_pb = atom_ctx.lookup.atom_pb(clustered_blk_id)->parent_pb;
    cur_pb = get_top_level_pb(cur_pb);

    if (int(atom_ctx.nlist.net_sinks(net_id).size()) > high_fanout_net_threshold) {
        /* Optimization: It can be too runtime costly for marking all sinks for
         * a high fanout-net that probably has no hope of ever getting packed,
         * thus ignore those high fanout nets */
        if (!is_global.count(net_id)) {
            /* If no low/medium fanout nets, we may need to consider
             * high fan-out nets for packing, so select one and store it */
            AtomNetId stored_net = cur_pb->pb_stats->tie_break_high_fanout_net;
            if (!stored_net || atom_ctx.nlist.net_sinks(net_id).size() < atom_ctx.nlist.net_sinks(stored_net).size()) {
                cur_pb->pb_stats->tie_break_high_fanout_net = net_id;
            }
        }
        return;
    }

    /* Mark atom net as being visited, if necessary. */

    if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
        cur_pb->pb_stats->marked_nets.push_back(net_id);
    }

    /* Update gains of affected blocks. */

    if (gain_flag == GAIN) {
        /* Check if this net is connected to it's driver block multiple times (i.e. as both an output and input)
         * If so, avoid double counting by skipping the first (driving) pin. */

        auto pins = atom_ctx.nlist.net_pins(net_id);
        if (net_output_feeds_driving_block_input[net_id] != 0)
            //We implicitly assume here that net_output_feeds_driver_block_input[net_id] is 2
            //(i.e. the net loops back to the block only once)
            pins = atom_ctx.nlist.net_sinks(net_id);

        if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
            for (auto pin_id : pins) {
                auto blk_id = atom_ctx.nlist.pin_block(pin_id);
                if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
                    if (cur_pb->pb_stats->sharinggain.count(blk_id) == 0) {
                        cur_pb->pb_stats->marked_blocks.push_back(blk_id);
                        cur_pb->pb_stats->sharinggain[blk_id] = 1;
                        cur_pb->pb_stats->hillgain[blk_id] = 1 - num_ext_inputs_atom_block(blk_id);
                    } else {
                        cur_pb->pb_stats->sharinggain[blk_id]++;
                        cur_pb->pb_stats->hillgain[blk_id]++;
                    }
                }
            }
        }

        if (connection_driven) {
            update_connection_gain_values(net_id, clustered_blk_id, cur_pb,
                                          net_relation_to_clustered_block);
        }

        if (timing_driven) {
            update_timing_gain_values(net_id, cur_pb,
                                      net_relation_to_clustered_block,
                                      timing_info,
                                      is_global,
                                      net_output_feeds_driving_block_input);
        }
    }
    if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
        cur_pb->pb_stats->num_pins_of_net_in_pb[net_id] = 0;
    }
    cur_pb->pb_stats->num_pins_of_net_in_pb[net_id]++;
}

/*****************************************/
void update_total_gain(float alpha, float beta, bool timing_driven, bool connection_driven, t_pb* pb, AttractionInfo& attraction_groups) {
    /*Updates the total  gain array to reflect the desired tradeoff between*
     *input sharing (sharinggain) and path_length minimization (timinggain)
     *input each time a new molecule is added to the cluster.*/
    auto& atom_ctx = g_vpr_ctx.atom();
    t_pb* cur_pb = pb;

    cur_pb = get_top_level_pb(cur_pb);
    AttractGroupId cluster_att_grp_id;

    cluster_att_grp_id = cur_pb->pb_stats->attraction_grp_id;

    for (AtomBlockId blk_id : cur_pb->pb_stats->marked_blocks) {
        //Initialize connectiongain and sharinggain if
        //they have not previously been updated for the block
        if (cur_pb->pb_stats->connectiongain.count(blk_id) == 0) {
            cur_pb->pb_stats->connectiongain[blk_id] = 0;
        }
        if (cur_pb->pb_stats->sharinggain.count(blk_id) == 0) {
            cur_pb->pb_stats->sharinggain[blk_id] = 0;
        }

        AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);
        if (atom_grp_id != AttractGroupId::INVALID() && atom_grp_id == cluster_att_grp_id) {
            //increase gain of atom based on attraction group gain
            float att_grp_gain = attraction_groups.get_attraction_group_gain(atom_grp_id);
            cur_pb->pb_stats->gain[blk_id] += att_grp_gain;
        }

        /* Todo: This was used to explore different normalization options, can
         * be made more efficient once we decide on which one to use*/
        int num_used_input_pins = atom_ctx.nlist.block_input_pins(blk_id).size();
        int num_used_output_pins = atom_ctx.nlist.block_output_pins(blk_id).size();
        /* end todo */

        /* Calculate area-only cost function */
        int num_used_pins = num_used_input_pins + num_used_output_pins;
        VTR_ASSERT(num_used_pins > 0);
        if (connection_driven) {
            /*try to absorb as many connections as possible*/
            cur_pb->pb_stats->gain[blk_id] = ((1 - beta)
                                                  * (float)cur_pb->pb_stats->sharinggain[blk_id]
                                              + beta * (float)cur_pb->pb_stats->connectiongain[blk_id])
                                             / (num_used_pins);
        } else {
            cur_pb->pb_stats->gain[blk_id] = ((float)cur_pb->pb_stats->sharinggain[blk_id])
                                             / (num_used_pins);
        }

        /* Add in timing driven cost into cost function */
        if (timing_driven) {
            cur_pb->pb_stats->gain[blk_id] = alpha
                                                 * cur_pb->pb_stats->timinggain[blk_id]
                                             + (1.0 - alpha) * (float)cur_pb->pb_stats->gain[blk_id];
        }
    }
}

/*****************************************/
void update_cluster_stats(const t_pack_molecule* molecule,
                          const ClusterBlockId clb_index,
                          const std::unordered_set<AtomNetId>& is_clock,
                          const std::unordered_set<AtomNetId>& is_global,
                          const bool global_clocks,
                          const float alpha,
                          const float beta,
                          const bool timing_driven,
                          const bool connection_driven,
                          const int high_fanout_net_threshold,
                          const SetupTimingInfo& timing_info,
                          AttractionInfo& attraction_groups,
                          std::unordered_map<AtomNetId, int>& net_output_feeds_driving_block_input) {
    /* Routine that is called each time a new molecule is added to the cluster.
     * Makes calls to update cluster stats such as the gain map for atoms, used pins, and clock structures,
     * in order to reflect the new content of the cluster.
     * Also keeps track of which attraction group the cluster belongs to. */

    int molecule_size;
    int iblock;
    t_pb *cur_pb, *cb;

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    molecule_size = get_array_size_of_molecule(molecule);
    cb = nullptr;

    for (iblock = 0; iblock < molecule_size; iblock++) {
        auto blk_id = molecule->atom_block_ids[iblock];
        if (!blk_id) {
            continue;
        }

        //Update atom netlist mapping
        atom_ctx.lookup.set_atom_clb(blk_id, clb_index);

        const t_pb* atom_pb = atom_ctx.lookup.atom_pb(blk_id);
        VTR_ASSERT(atom_pb);

        cur_pb = atom_pb->parent_pb;

        //Update attraction group
        AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);

        while (cur_pb) {
            /* reset list of feasible blocks */
            if (cur_pb->is_root()) {
                cb = cur_pb;
            }
            cur_pb->pb_stats->num_feasible_blocks = NOT_VALID;
            cur_pb->pb_stats->num_child_blocks_in_pb++;

            if (atom_grp_id != AttractGroupId::INVALID()) {
                /* TODO: Allow clusters to have more than one attraction group. */
                cur_pb->pb_stats->attraction_grp_id = atom_grp_id;
            }

            cur_pb = cur_pb->parent_pb;
        }

        /* Outputs first */
        for (auto pin_id : atom_ctx.nlist.block_output_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (!is_clock.count(net_id) || !global_clocks) {
                mark_and_update_partial_gain(net_id, GAIN, blk_id,
                                             timing_driven,
                                             connection_driven, OUTPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            } else {
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id,
                                             timing_driven,
                                             connection_driven, OUTPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            }
        }

        /* Next Inputs */
        for (auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            mark_and_update_partial_gain(net_id, GAIN, blk_id,
                                         timing_driven, connection_driven,
                                         INPUT,
                                         timing_info,
                                         is_global,
                                         high_fanout_net_threshold,
                                         net_output_feeds_driving_block_input);
        }

        /* Finally Clocks */
        for (auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (global_clocks) {
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id,
                                             timing_driven, connection_driven, INPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            } else {
                mark_and_update_partial_gain(net_id, GAIN, blk_id,
                                             timing_driven, connection_driven, INPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            }
        }

        update_total_gain(alpha, beta, timing_driven, connection_driven,
                          atom_pb->parent_pb, attraction_groups);

        commit_lookahead_pins_used(cb);
    }

    // if this molecule came from the transitive fanout candidates remove it
    if (cb) {
        cb->pb_stats->transitive_fanout_candidates.erase(molecule->atom_block_ids[molecule->root]);
        cb->pb_stats->explore_transitive_fanout = true;
    }
}

void start_new_cluster(t_cluster_placement_stats* cluster_placement_stats,
                       t_pb_graph_node** primitives_list,
                       ClusterBlockId clb_index,
                       t_pack_molecule* molecule,
                       std::map<t_logical_block_type_ptr, size_t>& num_used_type_instances,
                       const float target_device_utilization,
                       const int num_models,
                       const int max_cluster_size,
                       const t_arch* arch,
                       std::string device_layout_name,
                       std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
                       t_lb_router_data** router_data,
                       const int detailed_routing_stage,
                       ClusteredNetlist* clb_nlist,
                       const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types,
                       int verbosity,
                       bool enable_pin_feasibility_filter,
                       bool balance_block_type_utilization,
                       const int feasible_block_array_size,
                       PartitionRegion& temp_cluster_pr) {
    /* Given a starting seed block, start_new_cluster determines the next cluster type to use
     * It expands the FPGA if it cannot find a legal cluster for the atom block
     */

    auto& atom_ctx = g_vpr_ctx.atom();
    auto& device_ctx = g_vpr_ctx.mutable_device();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();

    /*Cluster's PartitionRegion is empty initially, meaning it has no floorplanning constraints*/
    PartitionRegion empty_pr;
    floorplanning_ctx.cluster_constraints.push_back(empty_pr);

    /* Allocate a dummy initial cluster and load a atom block as a seed and check if it is legal */
    AtomBlockId root_atom = molecule->atom_block_ids[molecule->root];
    const std::string& root_atom_name = atom_ctx.nlist.block_name(root_atom);
    const t_model* root_model = atom_ctx.nlist.block_model(root_atom);

    auto itr = primitive_candidate_block_types.find(root_model);
    VTR_ASSERT(itr != primitive_candidate_block_types.end());
    std::vector<t_logical_block_type_ptr> candidate_types = itr->second;

    if (balance_block_type_utilization) {
        //We sort the candidate types in ascending order by their current utilization.
        //This means that the packer will prefer to use types with lower utilization.
        //This is a naive approach to try balancing utilization when multiple types can
        //support the same primitive(s).
        std::stable_sort(candidate_types.begin(), candidate_types.end(),
                         [&](t_logical_block_type_ptr lhs, t_logical_block_type_ptr rhs) {
                             int lhs_num_instances = 0;
                             int rhs_num_instances = 0;
                             // Count number of instances for each type
                             for (auto type : lhs->equivalent_tiles)
                                 lhs_num_instances += device_ctx.grid.num_instances(type);
                             for (auto type : rhs->equivalent_tiles)
                                 rhs_num_instances += device_ctx.grid.num_instances(type);

                             float lhs_util = vtr::safe_ratio<float>(num_used_type_instances[lhs], lhs_num_instances);
                             float rhs_util = vtr::safe_ratio<float>(num_used_type_instances[rhs], rhs_num_instances);
                             //Lower util first
                             return lhs_util < rhs_util;
                         });
    }

    if (verbosity > 2) {
        VTR_LOG("\tSeed: '%s' (%s)", root_atom_name.c_str(), root_model->name);
        VTR_LOGV(molecule->pack_pattern, " molecule_type %s molecule_size %zu",
                 molecule->pack_pattern->name, molecule->atom_block_ids.size());
        VTR_LOG("\n");
    }

    //Try packing into each candidate type
    bool success = false;
    for (size_t i = 0; i < candidate_types.size(); i++) {
        auto type = candidate_types[i];

        t_pb* pb = new t_pb;
        pb->pb_graph_node = type->pb_graph_head;
        alloc_and_load_pb_stats(pb, feasible_block_array_size);
        pb->parent_pb = nullptr;

        *router_data = alloc_and_load_router_data(&lb_type_rr_graphs[type->index], type);

        //Try packing into each mode
        e_block_pack_status pack_result = BLK_STATUS_UNDEFINED;
        for (int j = 0; j < type->pb_graph_head->pb_type->num_modes && !success; j++) {
            pb->mode = j;

            reset_cluster_placement_stats(&cluster_placement_stats[type->index]);
            set_mode_cluster_placement_stats(pb->pb_graph_node, j);

            //Note that since we are starting a new cluster, we use FULL_EXTERNAL_PIN_UTIL,
            //which allows all cluster pins to be used. This ensures that if we have a large
            //molecule which would otherwise exceed the external pin utilization targets it
            //can use the full set of cluster pins when selected as the seed block -- ensuring
            //it is still implementable.
            pack_result = try_pack_molecule(&cluster_placement_stats[type->index],
                                            molecule, primitives_list, pb,
                                            num_models, max_cluster_size, clb_index,
                                            detailed_routing_stage, *router_data,
                                            verbosity,
                                            enable_pin_feasibility_filter,
                                            feasible_block_array_size,
                                            FULL_EXTERNAL_PIN_UTIL,
                                            temp_cluster_pr);

            success = (pack_result == BLK_PASSED);
        }

        if (success) {
            VTR_LOGV(verbosity > 2, "\tPASSED_SEED: Block Type %s\n", type->name);
            //Once clustering succeeds, add it to the clb netlist
            if (pb->name != nullptr) {
                free(pb->name);
            }
            pb->name = vtr::strdup(root_atom_name.c_str());
            clb_index = clb_nlist->create_block(root_atom_name.c_str(), pb, type);
            break;
        } else {
            VTR_LOGV(verbosity > 2, "\tFAILED_SEED: Block Type %s\n", type->name);
            //Free failed clustering and try again
            free_router_data(*router_data);
            free_pb(pb);
            delete pb;
            *router_data = nullptr;
        }
    }

    if (!success) {
        //Explored all candidates
        if (molecule->type == MOLECULE_FORCED_PACK) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tPattern %s %s\n",
                            molecule->pack_pattern->name,
                            root_atom_name.c_str());
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Can not find any logic block that can implement molecule.\n"
                            "\tAtom %s (%s)\n",
                            root_atom_name.c_str(), root_model->name);
        }
    }

    VTR_ASSERT(success);

    //Successfully create cluster
    auto block_type = clb_nlist->block_type(clb_index);
    num_used_type_instances[block_type]++;

    /* Expand FPGA size if needed */
    // Check used type instances against the possible equivalent physical locations
    unsigned int num_instances = 0;
    for (auto equivalent_tile : block_type->equivalent_tiles) {
        num_instances += device_ctx.grid.num_instances(equivalent_tile);
    }

    if (num_used_type_instances[block_type] > num_instances) {
        device_ctx.grid = create_device_grid(device_layout_name, arch->grid_layouts, num_used_type_instances, target_device_utilization);
    }
}

/*
 * Get candidate molecule to pack into currently open cluster
 * Molecule selection priority:
 * 1. Find unpacked molecules based on criticality and strong connectedness (connected by low fanout nets) with current cluster
 * 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
 * 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
 * 4. Find unpacked molecules based on attraction group of the current cluster (if the cluster has an attraction group)
 */
t_pack_molecule* get_highest_gain_molecule(t_pb* cur_pb,
                                           AttractionInfo& attraction_groups,
                                           const enum e_gain_type gain_mode,
                                           t_cluster_placement_stats* cluster_placement_stats_ptr,
                                           vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                           const ClusterBlockId cluster_index,
                                           bool prioritize_transitive_connectivity,
                                           int transitive_fanout_threshold,
                                           const int feasible_block_array_size,
                                           std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    /*
     * This routine populates a list of feasible blocks outside the cluster, then returns the best candidate for the cluster.
     * If there are no feasible blocks it returns a nullptr.
     */

    if (gain_mode == HILL_CLIMBING) {
        VPR_FATAL_ERROR(VPR_ERROR_PACK,
                        "Hill climbing not supported yet, error out.\n");
    }

    // 1. Find unpacked molecules based on criticality and strong connectedness (connected by low fanout nets) with current cluster
    if (cur_pb->pb_stats->num_feasible_blocks == NOT_VALID) {
        add_cluster_molecule_candidates_by_connectivity_and_timing(cur_pb, cluster_placement_stats_ptr, feasible_block_array_size, attraction_groups);
    }

    if (prioritize_transitive_connectivity) {
        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cur_pb, cluster_placement_stats_ptr, clb_inter_blk_nets,
                                                                       cluster_index, transitive_fanout_threshold, feasible_block_array_size, attraction_groups);
        }

        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cur_pb, cluster_placement_stats_ptr, feasible_block_array_size, attraction_groups);
        }
    } else { //Reverse order
        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cur_pb, cluster_placement_stats_ptr, feasible_block_array_size, attraction_groups);
        }

        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cur_pb, cluster_placement_stats_ptr, clb_inter_blk_nets,
                                                                       cluster_index, transitive_fanout_threshold, feasible_block_array_size, attraction_groups);
        }
    }

    // 4. Find unpacked molecules based on attraction group of the current cluster (if the cluster has an attraction group)
    add_cluster_molecule_candidates_by_attraction_group(cur_pb, cluster_placement_stats_ptr, attraction_groups,
                                                        feasible_block_array_size, cluster_index, primitive_candidate_block_types);

    /* Grab highest gain molecule */
    t_pack_molecule* molecule = nullptr;
    if (cur_pb->pb_stats->num_feasible_blocks > 0) {
        cur_pb->pb_stats->num_feasible_blocks--;
        int index = cur_pb->pb_stats->num_feasible_blocks;
        molecule = cur_pb->pb_stats->feasible_blocks[index];
        VTR_ASSERT(molecule->valid == true);
        return molecule;
    }

    return molecule;
}

/* Add molecules with strong connectedness to the current cluster to the list of feasible blocks. */
void add_cluster_molecule_candidates_by_connectivity_and_timing(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    VTR_ASSERT(cur_pb->pb_stats->num_feasible_blocks == NOT_VALID);

    cur_pb->pb_stats->num_feasible_blocks = 0;
    cur_pb->pb_stats->explore_transitive_fanout = true; /* If no legal molecules found, enable exploration of molecules two hops away */

    auto& atom_ctx = g_vpr_ctx.atom();

    for (AtomBlockId blk_id : cur_pb->pb_stats->marked_blocks) {
        if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
            auto rng = atom_ctx.atom_molecules.equal_range(blk_id);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                t_pack_molecule* molecule = kv.second;
                if (molecule->valid) {
                    bool success = check_free_primitives_for_molecule_atoms(molecule, cluster_placement_stats_ptr);
                    if (success) {
                        add_molecule_to_pb_stats_candidates(molecule,
                                                            cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
                    }
                }
            }
        }
    }
}

/* Add molecules based on weak connectedness (connected by high fanout nets) with current cluster */
void add_cluster_molecule_candidates_by_highfanout_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    /* Because the packer ignores high fanout nets when marking what blocks
     * to consider, use one of the ignored high fanout net to fill up lightly
     * related blocks */
    reset_tried_but_unused_cluster_placements(cluster_placement_stats_ptr);

    AtomNetId net_id = cur_pb->pb_stats->tie_break_high_fanout_net;

    auto& atom_ctx = g_vpr_ctx.atom();

    int count = 0;
    for (auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
        if (count >= AAPACK_MAX_HIGH_FANOUT_EXPLORE) {
            break;
        }

        AtomBlockId blk_id = atom_ctx.nlist.pin_block(pin_id);

        if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
            auto rng = atom_ctx.atom_molecules.equal_range(blk_id);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                t_pack_molecule* molecule = kv.second;
                if (molecule->valid) {
                    bool success = check_free_primitives_for_molecule_atoms(molecule, cluster_placement_stats_ptr);
                    if (success) {
                        add_molecule_to_pb_stats_candidates(molecule,
                                                            cur_pb->pb_stats->gain, cur_pb, std::min(feasible_block_array_size, AAPACK_MAX_HIGH_FANOUT_EXPLORE), attraction_groups);
                        count++;
                    }
                }
            }
        }
    }
    cur_pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID(); /* Mark off that this high fanout net has been considered */
}

/*
 * If the current cluster being packed has an attraction group associated with it
 * (i.e. there are atoms in it that belong to an attraction group), this routine adds molecules
 * from the associated attraction group to the list of feasible blocks for the cluster.
 * Attraction groups can be very large, so we only add some randomly selected molecules for efficiency
 * if the number of atoms in the group is greater than 500. Therefore, the molecules added to the candidates
 * will vary each time you call this function.
 */
void add_cluster_molecule_candidates_by_attraction_group(t_pb* cur_pb,
                                                         t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                         AttractionInfo& attraction_groups,
                                                         const int feasible_block_array_size,
                                                         ClusterBlockId clb_index,
                                                         std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    auto cluster_type = cluster_ctx.clb_nlist.block_type(clb_index);

    /*
     * For each cluster, we want to explore the attraction group molecules as potential
     * candidates for the cluster a limited number of times. This limit is imposed because
     * if the cluster belongs to a very large attraction group, we could potentially search
     * through its attraction group molecules for a very long time.
     * Defining a number of times to search through the attraction groups (i.e. number of
     * attraction group pulls) determines how many times we search through the cluster's attraction
     * group molecules for candidate molecules.
     */
    int num_pulls = attraction_groups.get_att_group_pulls();
    if (cur_pb->pb_stats->pulled_from_atom_groups < num_pulls) {
        cur_pb->pb_stats->pulled_from_atom_groups++;
    } else {
        return;
    }

    AttractGroupId grp_id = cur_pb->pb_stats->attraction_grp_id;
    if (grp_id == AttractGroupId::INVALID()) {
        return;
    }

    AttractionGroup& group = attraction_groups.get_attraction_group_info(grp_id);
    std::vector<AtomBlockId> available_atoms;
    for (AtomBlockId atom_id : group.group_atoms) {
        const auto& atom_model = atom_ctx.nlist.block_model(atom_id);
        auto itr = primitive_candidate_block_types.find(atom_model);
        VTR_ASSERT(itr != primitive_candidate_block_types.end());
        std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

        //Only consider molecules that are unpacked and of the correct type
        if (atom_ctx.lookup.atom_clb(atom_id) == ClusterBlockId::INVALID()
            && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
            available_atoms.push_back(atom_id);
        }
    }

    //int num_available_atoms = group.group_atoms.size();
    int num_available_atoms = available_atoms.size();
    if (num_available_atoms == 0) {
        return;
    }

    if (num_available_atoms < 500) {
        //for (AtomBlockId atom_id : group.group_atoms) {
        for (AtomBlockId atom_id : available_atoms) {
            const auto& atom_model = atom_ctx.nlist.block_model(atom_id);
            auto itr = primitive_candidate_block_types.find(atom_model);
            VTR_ASSERT(itr != primitive_candidate_block_types.end());
            std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

            //Only consider molecules that are unpacked and of the correct type
            if (atom_ctx.lookup.atom_clb(atom_id) == ClusterBlockId::INVALID()
                && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
                auto rng = atom_ctx.atom_molecules.equal_range(atom_id);
                for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                    t_pack_molecule* molecule = kv.second;
                    if (molecule->valid) {
                        bool success = check_free_primitives_for_molecule_atoms(molecule, cluster_placement_stats_ptr);
                        if (success) {
                            add_molecule_to_pb_stats_candidates(molecule,
                                                                cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
                        }
                    }
                }
            }
        }
        return;
    }

    int min = 0;
    int max = num_available_atoms - 1;

    for (int j = 0; j < 500; j++) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(min, max);
        int selected_atom = distr(gen);

        //AtomBlockId blk_id = group.group_atoms[selected_atom];
        AtomBlockId blk_id = available_atoms[selected_atom];
        const auto& atom_model = atom_ctx.nlist.block_model(blk_id);
        auto itr = primitive_candidate_block_types.find(atom_model);
        VTR_ASSERT(itr != primitive_candidate_block_types.end());
        std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

        //Only consider molecules that are unpacked and of the correct type
        if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()
            && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
            auto rng = atom_ctx.atom_molecules.equal_range(blk_id);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                t_pack_molecule* molecule = kv.second;
                if (molecule->valid) {
                    bool success = check_free_primitives_for_molecule_atoms(molecule, cluster_placement_stats_ptr);
                    if (success) {
                        add_molecule_to_pb_stats_candidates(molecule,
                                                            cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
                    }
                }
            }
        }
    }
}

/* Add molecules based on transitive connections (eg. 2 hops away) with current cluster*/
void add_cluster_molecule_candidates_by_transitive_connectivity(t_pb* cur_pb,
                                                                t_cluster_placement_stats* cluster_placement_stats_ptr,
                                                                vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                                                const ClusterBlockId cluster_index,
                                                                int transitive_fanout_threshold,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    //TODO: For now, only done by fan-out; should also consider fan-in
    cur_pb->pb_stats->explore_transitive_fanout = false;

    /* First time finding transitive fanout candidates therefore alloc and load them */
    load_transitive_fanout_candidates(cluster_index,
                                      cur_pb->pb_stats,
                                      clb_inter_blk_nets,
                                      transitive_fanout_threshold);
    /* Only consider candidates that pass a very simple legality check */
    for (const auto& transitive_candidate : cur_pb->pb_stats->transitive_fanout_candidates) {
        t_pack_molecule* molecule = transitive_candidate.second;
        if (molecule->valid) {
            bool success = check_free_primitives_for_molecule_atoms(molecule, cluster_placement_stats_ptr);
            if (success) {
                add_molecule_to_pb_stats_candidates(molecule,
                                                    cur_pb->pb_stats->gain, cur_pb, std::min(feasible_block_array_size, AAPACK_MAX_TRANSITIVE_EXPLORE), attraction_groups);
            }
        }
    }
}

/*Check whether a free primitive exists for each atom block in the molecule*/
bool check_free_primitives_for_molecule_atoms(t_pack_molecule* molecule, t_cluster_placement_stats* cluster_placement_stats_ptr) {
    auto& atom_ctx = g_vpr_ctx.atom();
    bool success = true;

    for (int i_atom = 0; i_atom < get_array_size_of_molecule(molecule); i_atom++) {
        if (molecule->atom_block_ids[i_atom]) {
            VTR_ASSERT(atom_ctx.lookup.atom_clb(molecule->atom_block_ids[i_atom]) == ClusterBlockId::INVALID());
            auto blk_id2 = molecule->atom_block_ids[i_atom];
            if (!exists_free_primitive_for_atom_block(cluster_placement_stats_ptr, blk_id2)) {
                /* TODO (Jason Luu): debating whether to check if placement exists for molecule
                 * (more robust) or individual atom blocks (faster)*/
                success = false;
                break;
            }
        }
    }

    return success;
}

/*****************************************/
t_pack_molecule* get_molecule_for_cluster(t_pb* cur_pb,
                                          AttractionInfo& attraction_groups,
                                          const bool allow_unrelated_clustering,
                                          const bool prioritize_transitive_connectivity,
                                          const int transitive_fanout_threshold,
                                          const int feasible_block_array_size,
                                          int* num_unrelated_clustering_attempts,
                                          t_cluster_placement_stats* cluster_placement_stats_ptr,
                                          vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                          ClusterBlockId cluster_index,
                                          int verbosity,
                                          t_molecule_link* unclustered_list_head,
                                          const int& unclustered_list_head_size,
                                          std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    /* Finds the block with the greatest gain that satisfies the
     * input, clock and capacity constraints of a cluster that are
     * passed in.  If no suitable block is found it returns ClusterBlockId::INVALID().
     */

    VTR_ASSERT(cur_pb->is_root());

    /* If cannot pack into primitive, try packing into cluster */

    auto best_molecule = get_highest_gain_molecule(cur_pb, attraction_groups,
                                                   NOT_HILL_CLIMBING, cluster_placement_stats_ptr, clb_inter_blk_nets,
                                                   cluster_index, prioritize_transitive_connectivity,
                                                   transitive_fanout_threshold, feasible_block_array_size, primitive_candidate_block_types);

    /* If no blocks have any gain to the current cluster, the code above      *
     * will not find anything.  However, another atom block with no inputs in *
     * common with the cluster may still be inserted into the cluster.        */

    if (allow_unrelated_clustering) {
        if (best_molecule == nullptr) {
            if (*num_unrelated_clustering_attempts == 0) {
                best_molecule = get_free_molecule_with_most_ext_inputs_for_cluster(cur_pb,
                                                                                   cluster_placement_stats_ptr,
                                                                                   unclustered_list_head,
                                                                                   unclustered_list_head_size);
                (*num_unrelated_clustering_attempts)++;
                VTR_LOGV(best_molecule && verbosity > 2, "\tFound unrelated molecule to cluster\n");
            }
        } else {
            *num_unrelated_clustering_attempts = 0;
        }
    } else {
        VTR_LOGV(!best_molecule && verbosity > 2, "\tNo related molecule found and unrelated clustering disabled\n");
    }

    return best_molecule;
}

void mark_all_molecules_valid(t_pack_molecule* molecule_head) {
    for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
        cur_molecule->valid = true;
    }
}

int count_molecules(t_pack_molecule* molecule_head) {
    int num_molecules = 0;
    for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
        ++num_molecules;
    }
    return num_molecules;
}

//Calculates molecule statistics for a single molecule
t_molecule_stats calc_molecule_stats(const t_pack_molecule* molecule) {
    t_molecule_stats molecule_stats;

    auto& atom_ctx = g_vpr_ctx.atom();

    //Calculate the number of available pins on primitives within the molecule
    for (auto blk : molecule->atom_block_ids) {
        if (!blk) continue;

        ++molecule_stats.num_blocks; //Record number of valid blocks in molecule

        const t_model* model = atom_ctx.nlist.block_model(blk);

        for (const t_model_ports* input_port = model->inputs; input_port != nullptr; input_port = input_port->next) {
            molecule_stats.num_input_pins += input_port->size;
        }

        for (const t_model_ports* output_port = model->outputs; output_port != nullptr; output_port = output_port->next) {
            molecule_stats.num_output_pins += output_port->size;
        }
    }
    molecule_stats.num_pins = molecule_stats.num_input_pins + molecule_stats.num_output_pins;

    //Calculate the number of externally used pins
    std::set<AtomBlockId> molecule_atoms(molecule->atom_block_ids.begin(), molecule->atom_block_ids.end());
    for (auto blk : molecule->atom_block_ids) {
        if (!blk) continue;

        for (auto pin : atom_ctx.nlist.block_pins(blk)) {
            auto net = atom_ctx.nlist.pin_net(pin);

            auto pin_type = atom_ctx.nlist.pin_type(pin);
            if (pin_type == PinType::SINK) {
                auto driver_blk = atom_ctx.nlist.net_driver_block(net);

                if (molecule_atoms.count(driver_blk)) {
                    //Pin driven by a block within the molecule
                    //Does not count as an external connection
                } else {
                    //Pin driven by a block outside the molecule
                    ++molecule_stats.num_used_ext_inputs;
                }

            } else {
                VTR_ASSERT(pin_type == PinType::DRIVER);

                bool net_leaves_molecule = false;
                for (auto sink_pin : atom_ctx.nlist.net_sinks(net)) {
                    auto sink_blk = atom_ctx.nlist.pin_block(sink_pin);

                    if (!molecule_atoms.count(sink_blk)) {
                        //There is at least one sink outside of the current molecule
                        net_leaves_molecule = true;
                        break;
                    }
                }

                //We assume that any fanout occurs outside of the molecule, hence we only
                //count one used output (even if there are multiple sinks outside the molecule)
                if (net_leaves_molecule) {
                    ++molecule_stats.num_used_ext_outputs;
                }
            }
        }
    }
    molecule_stats.num_used_ext_pins = molecule_stats.num_used_ext_inputs + molecule_stats.num_used_ext_outputs;

    return molecule_stats;
}

//Calculates maximum molecule statistics accross all molecules in linked list
t_molecule_stats calc_max_molecules_stats(const t_pack_molecule* molecule_head) {
    t_molecule_stats max_molecules_stats;

    for (auto cur_molecule = molecule_head; cur_molecule != nullptr; cur_molecule = cur_molecule->next) {
        //Calculate per-molecule statistics
        t_molecule_stats cur_molecule_stats = calc_molecule_stats(cur_molecule);

        //Record the maximums (member-wise) over all molecules
        max_molecules_stats.num_blocks = std::max(max_molecules_stats.num_blocks, cur_molecule_stats.num_blocks);

        max_molecules_stats.num_pins = std::max(max_molecules_stats.num_pins, cur_molecule_stats.num_pins);
        max_molecules_stats.num_input_pins = std::max(max_molecules_stats.num_input_pins, cur_molecule_stats.num_input_pins);
        max_molecules_stats.num_output_pins = std::max(max_molecules_stats.num_output_pins, cur_molecule_stats.num_output_pins);

        max_molecules_stats.num_used_ext_pins = std::max(max_molecules_stats.num_used_ext_pins, cur_molecule_stats.num_used_ext_pins);
        max_molecules_stats.num_used_ext_inputs = std::max(max_molecules_stats.num_used_ext_inputs, cur_molecule_stats.num_used_ext_inputs);
        max_molecules_stats.num_used_ext_outputs = std::max(max_molecules_stats.num_used_ext_outputs, cur_molecule_stats.num_used_ext_outputs);
    }

    return max_molecules_stats;
}

std::vector<AtomBlockId> initialize_seed_atoms(const e_cluster_seed seed_type,
                                               const t_molecule_stats& max_molecule_stats,
                                               const vtr::vector<AtomBlockId, float>& atom_criticality) {
    std::vector<AtomBlockId> seed_atoms;

    //Put all atoms in seed list
    auto& atom_ctx = g_vpr_ctx.atom();
    for (auto blk : atom_ctx.nlist.blocks()) {
        seed_atoms.emplace_back(blk);
    }

    //Initially all gains are zero
    vtr::vector<AtomBlockId, float> atom_gains(atom_ctx.nlist.blocks().size(), 0.);

    if (seed_type == e_cluster_seed::TIMING) {
        VTR_ASSERT(atom_gains.size() == atom_criticality.size());

        //By criticality
        atom_gains = atom_criticality;

    } else if (seed_type == e_cluster_seed::MAX_INPUTS) {
        //By number of used molecule input pins
        for (auto blk : atom_ctx.nlist.blocks()) {
            int max_molecule_inputs = 0;
            auto molecule_rng = atom_ctx.atom_molecules.equal_range(blk);
            for (const auto& kv : vtr::make_range(molecule_rng.first, molecule_rng.second)) {
                const t_pack_molecule* blk_mol = kv.second;

                const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol);

                //Keep the max over all molecules associated with the atom
                max_molecule_inputs = std::max(max_molecule_inputs, molecule_stats.num_used_ext_inputs);
            }

            atom_gains[blk] = max_molecule_inputs;
        }

    } else if (seed_type == e_cluster_seed::BLEND) {
        //By blended gain (criticality and inputs used)
        for (auto blk : atom_ctx.nlist.blocks()) {
            /* Score seed gain of each block as a weighted sum of timing criticality,
             * number of tightly coupled blocks connected to it, and number of external inputs */
            float seed_blend_fac = 0.5;
            float max_blend_gain = 0;

            auto molecule_rng = atom_ctx.atom_molecules.equal_range(blk);
            for (const auto& kv : vtr::make_range(molecule_rng.first, molecule_rng.second)) {
                const t_pack_molecule* blk_mol = kv.second;

                const t_molecule_stats molecule_stats = calc_molecule_stats(blk_mol);

                VTR_ASSERT(max_molecule_stats.num_used_ext_inputs > 0);

                float blend_gain = (seed_blend_fac * atom_criticality[blk]
                                    + (1 - seed_blend_fac) * (molecule_stats.num_used_ext_inputs / max_molecule_stats.num_used_ext_inputs));
                blend_gain *= (1 + 0.2 * (molecule_stats.num_blocks - 1));

                //Keep the max over all molecules associated with the atom
                max_blend_gain = std::max(max_blend_gain, blend_gain);
            }
            atom_gains[blk] = max_blend_gain;
        }

    } else if (seed_type == e_cluster_seed::MAX_PINS || seed_type == e_cluster_seed::MAX_INPUT_PINS) {
        //By pins per molecule (i.e. available pins on primitives, not pins in use)

        for (auto blk : atom_ctx.nlist.blocks()) {
            int max_molecule_pins = 0;
            auto molecule_rng = atom_ctx.atom_molecules.equal_range(blk);
            for (const auto& kv : vtr::make_range(molecule_rng.first, molecule_rng.second)) {
                const t_pack_molecule* mol = kv.second;

                const t_molecule_stats molecule_stats = calc_molecule_stats(mol);

                //Keep the max over all molecules associated with the atom
                int molecule_pins = 0;
                if (seed_type == e_cluster_seed::MAX_PINS) {
                    //All pins
                    molecule_pins = molecule_stats.num_pins;
                } else {
                    VTR_ASSERT(seed_type == e_cluster_seed::MAX_INPUT_PINS);
                    //Input pins only
                    molecule_pins = molecule_stats.num_input_pins;
                }

                //Keep the max over all molecules associated with the atom
                max_molecule_pins = std::max(max_molecule_pins, molecule_pins);
            }
            atom_gains[blk] = max_molecule_pins;
        }

    } else if (seed_type == e_cluster_seed::BLEND2) {
        for (auto blk : atom_ctx.nlist.blocks()) {
            float max_gain = 0;
            auto molecule_rng = atom_ctx.atom_molecules.equal_range(blk);
            for (const auto& kv : vtr::make_range(molecule_rng.first, molecule_rng.second)) {
                const t_pack_molecule* mol = kv.second;

                const t_molecule_stats molecule_stats = calc_molecule_stats(mol);

                float pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_pins, max_molecule_stats.num_pins);
                float input_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_input_pins, max_molecule_stats.num_input_pins);
                float output_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_output_pins, max_molecule_stats.num_output_pins);
                float used_ext_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_pins, max_molecule_stats.num_used_ext_pins);
                float used_ext_input_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_inputs, max_molecule_stats.num_used_ext_inputs);
                float used_ext_output_pin_ratio = vtr::safe_ratio<float>(molecule_stats.num_used_ext_outputs, max_molecule_stats.num_used_ext_outputs);
                float num_blocks_ratio = vtr::safe_ratio<float>(molecule_stats.num_blocks, max_molecule_stats.num_blocks);
                float criticality = atom_criticality[blk];

                constexpr float PIN_WEIGHT = 0.;
                constexpr float INPUT_PIN_WEIGHT = 0.5;
                constexpr float OUTPUT_PIN_WEIGHT = 0.;
                constexpr float USED_PIN_WEIGHT = 0.;
                constexpr float USED_INPUT_PIN_WEIGHT = 0.2;
                constexpr float USED_OUTPUT_PIN_WEIGHT = 0.;
                constexpr float BLOCKS_WEIGHT = 0.2;
                constexpr float CRITICALITY_WEIGHT = 0.1;

                float gain = PIN_WEIGHT * pin_ratio
                             + INPUT_PIN_WEIGHT * input_pin_ratio
                             + OUTPUT_PIN_WEIGHT * output_pin_ratio

                             + USED_PIN_WEIGHT * used_ext_pin_ratio
                             + USED_INPUT_PIN_WEIGHT * used_ext_input_pin_ratio
                             + USED_OUTPUT_PIN_WEIGHT * used_ext_output_pin_ratio

                             + BLOCKS_WEIGHT * num_blocks_ratio
                             + CRITICALITY_WEIGHT * criticality;

                max_gain = std::max(max_gain, gain);
            }

            atom_gains[blk] = max_gain;
        }

    } else {
        VPR_FATAL_ERROR(VPR_ERROR_PACK, "Unrecognized cluster seed type");
    }

    //Sort seeds in descending order of gain (i.e. highest gain first)
    //
    // Note that we use a *stable* sort here. It has been observed that different
    // standard library implementations (e.g. gcc-4.9 vs gcc-5) use sorting algorithms
    // which produce different orderings for seeds of equal gain (which is allowed with
    // std::sort which does not specify how equal values are handled). Using a stable
    // sort ensures that regardless of the underlying sorting algorithm the same seed
    // order is produced regardless of compiler.
    auto by_descending_gain = [&](const AtomBlockId lhs, const AtomBlockId rhs) {
        return atom_gains[lhs] > atom_gains[rhs];
    };
    std::stable_sort(seed_atoms.begin(), seed_atoms.end(), by_descending_gain);

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES)) {
        print_seed_gains(getEchoFileName(E_ECHO_CLUSTERING_BLOCK_CRITICALITIES), seed_atoms, atom_gains, atom_criticality);
    }

    return seed_atoms;
}

t_pack_molecule* get_highest_gain_seed_molecule(int* seedindex, const std::vector<AtomBlockId> seed_atoms) {
    auto& atom_ctx = g_vpr_ctx.atom();

    while (*seedindex < static_cast<int>(seed_atoms.size())) {
        AtomBlockId blk_id = seed_atoms[(*seedindex)++];

        if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
            t_pack_molecule* best = nullptr;

            auto rng = atom_ctx.atom_molecules.equal_range(blk_id);
            for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                t_pack_molecule* molecule = kv.second;
                if (molecule->valid) {
                    if (best == nullptr || (best->base_gain) < (molecule->base_gain)) {
                        best = molecule;
                    }
                }
            }
            VTR_ASSERT(best != nullptr);
            return best;
        }
    }

    /*if it makes it to here , there are no more blocks available*/
    return nullptr;
}

/* get gain of packing molecule into current cluster
 * gain is equal to:
 * total_block_gain
 * + molecule_base_gain*some_factor
 * - introduced_input_nets_of_unrelated_blocks_pulled_in_by_molecule*some_other_factor
 */
float get_molecule_gain(t_pack_molecule* molecule, std::map<AtomBlockId, float>& blk_gain, AttractGroupId cluster_attraction_group_id, AttractionInfo& attraction_groups, int num_molecule_failures) {
    float gain;
    int i;
    int num_introduced_inputs_of_indirectly_related_block;
    auto& atom_ctx = g_vpr_ctx.atom();

    gain = 0;
    float attraction_group_penalty = 0.1;

    num_introduced_inputs_of_indirectly_related_block = 0;
    for (i = 0; i < get_array_size_of_molecule(molecule); i++) {
        auto blk_id = molecule->atom_block_ids[i];
        if (blk_id) {
            if (blk_gain.count(blk_id) > 0) {
                gain += blk_gain[blk_id];
            } else {
                /* This block has no connection with current cluster, penalize molecule for having this block
                 */
                for (auto pin_id : atom_ctx.nlist.block_input_pins(blk_id)) {
                    auto net_id = atom_ctx.nlist.pin_net(pin_id);
                    VTR_ASSERT(net_id);

                    auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
                    VTR_ASSERT(driver_pin_id);

                    auto driver_blk_id = atom_ctx.nlist.pin_block(driver_pin_id);

                    num_introduced_inputs_of_indirectly_related_block++;
                    for (int iblk = 0; iblk < get_array_size_of_molecule(molecule); iblk++) {
                        if (molecule->atom_block_ids[iblk] && driver_blk_id == molecule->atom_block_ids[iblk]) {
                            //valid block which is driver (and hence not an input)
                            num_introduced_inputs_of_indirectly_related_block--;
                            break;
                        }
                    }
                }
            }
            AttractGroupId atom_grp_id = attraction_groups.get_atom_attraction_group(blk_id);
            if (atom_grp_id == cluster_attraction_group_id && cluster_attraction_group_id != AttractGroupId::INVALID()) {
                float att_grp_gain = attraction_groups.get_attraction_group_gain(atom_grp_id);
                gain += att_grp_gain;
            } else if (cluster_attraction_group_id != AttractGroupId::INVALID() && atom_grp_id != cluster_attraction_group_id) {
                gain -= attraction_group_penalty;
            }
        }
    }

    gain += molecule->base_gain * 0.0001; /* Use base gain as tie breaker TODO: need to sweep this value and perhaps normalize */
    gain -= num_introduced_inputs_of_indirectly_related_block * (0.001);

    if (num_molecule_failures > 0 && attraction_groups.num_attraction_groups() > 0) {
        gain -= 0.1 * num_molecule_failures;
    }

    return gain;
}

int compare_molecule_gain(const void* a, const void* b) {
    float base_gain_a, base_gain_b, diff;
    const t_pack_molecule *molecule_a, *molecule_b;
    molecule_a = (*(const t_pack_molecule* const*)a);
    molecule_b = (*(const t_pack_molecule* const*)b);

    base_gain_a = molecule_a->base_gain;
    base_gain_b = molecule_b->base_gain;
    diff = base_gain_a - base_gain_b;
    if (diff > 0) {
        return 1;
    }
    if (diff < 0) {
        return -1;
    }
    return 0;
}

/* Determine if speculatively packed cur_pb is pin feasible
 * Runtime is actually not that bad for this.  It's worst case O(k^2) where k is the
 * number of pb_graph pins.  Can use hash tables or make incremental if becomes an issue.
 */
void try_update_lookahead_pins_used(t_pb* cur_pb) {
    int i, j;
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    // run recursively till a leaf (primitive) pb block is reached
    if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
        if (cur_pb->child_pbs != nullptr) {
            for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i] != nullptr) {
                    for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        try_update_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
                    }
                }
            }
        }
    } else {
        // find if this child (primitive) pb block has an atom mapped to it,
        // if yes compute and mark lookahead pins used for that pb block
        auto& atom_ctx = g_vpr_ctx.atom();
        AtomBlockId blk_id = atom_ctx.lookup.pb_atom(cur_pb);
        if (pb_type->blif_model != nullptr && blk_id) {
            compute_and_mark_lookahead_pins_used(blk_id);
        }
    }
}

/* Resets nets used at different pin classes for determining pin feasibility */
void reset_lookahead_pins_used(t_pb* cur_pb) {
    int i, j;
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;
    if (cur_pb->pb_stats == nullptr) {
        return; /* No pins used, no need to continue */
    }

    if (pb_type->num_modes > 0 && cur_pb->name != nullptr) {
        for (i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            cur_pb->pb_stats->lookahead_input_pins_used[i].clear();
        }

        for (i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            cur_pb->pb_stats->lookahead_output_pins_used[i].clear();
        }

        if (cur_pb->child_pbs != nullptr) {
            for (i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i] != nullptr) {
                    for (j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        reset_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
                    }
                }
            }
        }
    }
}

/* Determine if pins of speculatively packed pb are legal */
void compute_and_mark_lookahead_pins_used(const AtomBlockId blk_id) {
    auto& atom_ctx = g_vpr_ctx.atom();

    const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
    VTR_ASSERT(cur_pb != nullptr);

    /* Walk through inputs, outputs, and clocks marking pins off of the same class */
    for (auto pin_id : atom_ctx.nlist.block_pins(blk_id)) {
        auto net_id = atom_ctx.nlist.pin_net(pin_id);

        const t_pb_graph_pin* pb_graph_pin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        compute_and_mark_lookahead_pins_used_for_pin(pb_graph_pin, cur_pb, net_id);
    }
}

/**
 * Given a pin and its assigned net, mark all pin classes that are affected.
 * Check if connecting this pin to it's driver pin or to all sink pins will
 * require leaving a pb_block starting from the parent pb_block of the
 * primitive till the root block (depth = 0). If leaving a pb_block is
 * required add this net to the pin class (to increment the number of used
 * pins from this class) that should be used to leave the pb_block.
 */
void compute_and_mark_lookahead_pins_used_for_pin(const t_pb_graph_pin* pb_graph_pin, const t_pb* primitive_pb, const AtomNetId net_id) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // starting from the parent pb of the input primitive go up in the hierarchy till the root block
    for (auto cur_pb = primitive_pb->parent_pb; cur_pb; cur_pb = cur_pb->parent_pb) {
        const auto depth = cur_pb->pb_graph_node->pb_type->depth;
        const auto pin_class = pb_graph_pin->parent_pin_class[depth];
        VTR_ASSERT(pin_class != OPEN);

        const auto driver_blk_id = atom_ctx.nlist.net_driver_block(net_id);

        // if this primitive pin is an input pin
        if (pb_graph_pin->port->type == IN_PORT) {
            /* find location of net driver if exist in clb, NULL otherwise */
            // find the driver of the input net connected to the pin being studied
            const auto driver_pin_id = atom_ctx.nlist.net_driver(net_id);
            // find the id of the atom occupying the input primitive_pb
            const auto prim_blk_id = atom_ctx.lookup.pb_atom(primitive_pb);
            // find the pb block occupied by the driving atom
            const auto driver_pb = atom_ctx.lookup.atom_pb(driver_blk_id);
            // pb_graph_pin driving net_id in the driver pb block
            t_pb_graph_pin* output_pb_graph_pin = nullptr;
            // if the driver block is in the same clb as the input primitive block
            if (atom_ctx.lookup.atom_clb(driver_blk_id) == atom_ctx.lookup.atom_clb(prim_blk_id)) {
                // get pb_graph_pin driving the given net
                output_pb_graph_pin = get_driver_pb_graph_pin(driver_pb, driver_pin_id);
            }

            bool is_reachable = false;

            // if the driver pin is within the cluster
            if (output_pb_graph_pin) {
                // find if the driver pin can reach the input pin of the primitive or not
                const t_pb* check_pb = driver_pb;
                while (check_pb && check_pb != cur_pb) {
                    check_pb = check_pb->parent_pb;
                }
                if (check_pb) {
                    for (int i = 0; i < output_pb_graph_pin->num_connectable_primitive_input_pins[depth]; i++) {
                        if (pb_graph_pin == output_pb_graph_pin->list_of_connectable_input_pin_ptrs[depth][i]) {
                            is_reachable = true;
                            break;
                        }
                    }
                }
            }

            // Must use an input pin to connect the driver to the input pin of the given primitive, either the
            // driver atom is not contained in the cluster or is contained but cannot reach the primitive pin
            if (!is_reachable) {
                // add net to lookahead_input_pins_used if not already added
                auto it = std::find(cur_pb->pb_stats->lookahead_input_pins_used[pin_class].begin(),
                                    cur_pb->pb_stats->lookahead_input_pins_used[pin_class].end(), net_id);
                if (it == cur_pb->pb_stats->lookahead_input_pins_used[pin_class].end()) {
                    cur_pb->pb_stats->lookahead_input_pins_used[pin_class].push_back(net_id);
                }
            }
        } else {
            VTR_ASSERT(pb_graph_pin->port->type == OUT_PORT);
            /*
             * Determine if this net (which is driven from within this cluster) leaves this cluster
             * (and hence uses an output pin).
             */

            bool net_exits_cluster = true;
            int num_net_sinks = static_cast<int>(atom_ctx.nlist.net_sinks(net_id).size());

            if (pb_graph_pin->num_connectable_primitive_input_pins[depth] >= num_net_sinks) {
                //It is possible the net is completely absorbed in the cluster,
                //since this pin could (potentially) drive all the net's sinks

                /* Important: This runtime penalty looks a lot scarier than it really is.
                 * For high fan-out nets, I at most look at the number of pins within the
                 * cluster which limits runtime.
                 *
                 * DO NOT REMOVE THIS INITIAL FILTER WITHOUT CAREFUL ANALYSIS ON RUNTIME!!!
                 *
                 * Key Observation:
                 * For LUT-based designs it is impossible for the average fanout to exceed
                 * the number of LUT inputs so it's usually around 4-5 (pigeon-hole argument,
                 * if the average fanout is greater than the number of LUT inputs, where do
                 * the extra connections go?  Therefore, average fanout must be capped to a
                 * small constant where the constant is equal to the number of LUT inputs).
                 * The real danger to runtime is when the number of sinks of a net gets doubled
                 */

                //Check if all the net sinks are, in fact, inside this cluster
                bool all_sinks_in_cur_cluster = true;
                ClusterBlockId driver_clb = atom_ctx.lookup.atom_clb(driver_blk_id);
                for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
                    auto sink_blk_id = atom_ctx.nlist.pin_block(pin_id);
                    if (atom_ctx.lookup.atom_clb(sink_blk_id) != driver_clb) {
                        all_sinks_in_cur_cluster = false;
                        break;
                    }
                }

                if (all_sinks_in_cur_cluster) {
                    //All the sinks are part of this cluster, so the net may be fully absorbed.
                    //
                    //Verify this, by counting the number of net sinks reachable from the driver pin.
                    //If the count equals the number of net sinks then the net is fully absorbed and
                    //the net does not exit the cluster
                    /* TODO: I should cache the absorbed outputs, once net is absorbed,
                     *       net is forever absorbed, no point in rechecking every time */
                    if (net_sinks_reachable_in_cluster(pb_graph_pin, depth, net_id)) {
                        //All the sinks are reachable inside the cluster
                        net_exits_cluster = false;
                    }
                }
            }

            if (net_exits_cluster) {
                /* This output must exit this cluster */
                cur_pb->pb_stats->lookahead_output_pins_used[pin_class].push_back(net_id);
            }
        }
    }
}

int net_sinks_reachable_in_cluster(const t_pb_graph_pin* driver_pb_gpin, const int depth, const AtomNetId net_id) {
    size_t num_reachable_sinks = 0;
    auto& atom_ctx = g_vpr_ctx.atom();

    //Record the sink pb graph pins we are looking for
    std::unordered_set<const t_pb_graph_pin*> sink_pb_gpins;
    for (const AtomPinId pin_id : atom_ctx.nlist.net_sinks(net_id)) {
        const t_pb_graph_pin* sink_pb_gpin = find_pb_graph_pin(atom_ctx.nlist, atom_ctx.lookup, pin_id);
        VTR_ASSERT(sink_pb_gpin);

        sink_pb_gpins.insert(sink_pb_gpin);
    }

    //Count how many sink pins are reachable
    for (int i_prim_pin = 0; i_prim_pin < driver_pb_gpin->num_connectable_primitive_input_pins[depth]; ++i_prim_pin) {
        const t_pb_graph_pin* reachable_pb_gpin = driver_pb_gpin->list_of_connectable_input_pin_ptrs[depth][i_prim_pin];

        if (sink_pb_gpins.count(reachable_pb_gpin)) {
            ++num_reachable_sinks;
            if (num_reachable_sinks == atom_ctx.nlist.net_sinks(net_id).size()) {
                return true;
            }
        }
    }

    return false;
}

/**
 * Returns the pb_graph_pin of the atom pin defined by the driver_pin_id in the driver_pb
 */
t_pb_graph_pin* get_driver_pb_graph_pin(const t_pb* driver_pb, const AtomPinId driver_pin_id) {
    auto& atom_ctx = g_vpr_ctx.atom();
    const auto driver_pb_type = driver_pb->pb_graph_node->pb_type;
    int output_port = 0;
    // find the port of the pin driving the net as well as the port model
    auto driver_port_id = atom_ctx.nlist.pin_port(driver_pin_id);
    auto driver_model_port = atom_ctx.nlist.port_model(driver_port_id);
    // find the port id of the port containing the driving pin in the driver_pb_type
    for (int i = 0; i < driver_pb_type->num_ports; i++) {
        auto& prim_port = driver_pb_type->ports[i];
        if (prim_port.type == OUT_PORT) {
            if (prim_port.model_port == driver_model_port) {
                // get the output pb_graph_pin driving this input net
                return &(driver_pb->pb_graph_node->output_pins[output_port][atom_ctx.nlist.pin_port_bit(driver_pin_id)]);
            }
            output_port++;
        }
    }
    // the pin should be found
    VTR_ASSERT(false);
    return nullptr;
}

/* Check if the number of available inputs/outputs for a pin class is sufficient for speculatively packed blocks */
bool check_lookahead_pins_used(t_pb* cur_pb, t_ext_pin_util max_external_pin_util) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (pb_type->num_modes > 0 && cur_pb->name) {
        for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            size_t class_size = cur_pb->pb_graph_node->input_pin_class_size[i];

            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.input_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster inputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, cur_pb->pb_stats->input_pins_used[i].size());
            }

            if (cur_pb->pb_stats->lookahead_input_pins_used[i].size() > class_size) {
                return false;
            }
        }

        for (int i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            size_t class_size = cur_pb->pb_graph_node->output_pin_class_size[i];
            if (cur_pb->is_root()) {
                // Scale the class size by the maximum external pin utilization factor
                // Use ceil to avoid classes of size 1 from being scaled to zero
                class_size = std::ceil(max_external_pin_util.output_pin_util * class_size);
                // if the number of pins already used is larger than class size, then the number of
                // cluster outputs already used should be our constraint. Why is this needed? This is
                // needed since when packing the seed block the maximum external pin utilization is
                // used as 1.0 allowing molecules that are using up to all the cluster inputs to be
                // packed legally. Therefore, if the seed block is already using more inputs than
                // the allowed maximum utilization, this should become the new maximum pin utilization.
                class_size = std::max<size_t>(class_size, cur_pb->pb_stats->output_pins_used[i].size());
            }

            if (cur_pb->pb_stats->lookahead_output_pins_used[i].size() > class_size) {
                return false;
            }
        }

        if (cur_pb->child_pbs) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i]) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        if (!check_lookahead_pins_used(&cur_pb->child_pbs[i][j], max_external_pin_util))
                            return false;
                    }
                }
            }
        }
    }

    return true;
}

/* Speculation successful, commit input/output pins used */
void commit_lookahead_pins_used(t_pb* cur_pb) {
    const t_pb_type* pb_type = cur_pb->pb_graph_node->pb_type;

    if (pb_type->num_modes > 0 && cur_pb->name) {
        for (int i = 0; i < cur_pb->pb_graph_node->num_input_pin_class; i++) {
            VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->input_pin_class_size[i]);
            for (size_t j = 0; j < cur_pb->pb_stats->lookahead_input_pins_used[i].size(); j++) {
                VTR_ASSERT(cur_pb->pb_stats->lookahead_input_pins_used[i][j]);
                cur_pb->pb_stats->input_pins_used[i].insert({j, cur_pb->pb_stats->lookahead_input_pins_used[i][j]});
            }
        }

        for (int i = 0; i < cur_pb->pb_graph_node->num_output_pin_class; i++) {
            VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i].size() <= (unsigned int)cur_pb->pb_graph_node->output_pin_class_size[i]);
            for (size_t j = 0; j < cur_pb->pb_stats->lookahead_output_pins_used[i].size(); j++) {
                VTR_ASSERT(cur_pb->pb_stats->lookahead_output_pins_used[i][j]);
                cur_pb->pb_stats->output_pins_used[i].insert({j, cur_pb->pb_stats->lookahead_output_pins_used[i][j]});
            }
        }

        if (cur_pb->child_pbs) {
            for (int i = 0; i < pb_type->modes[cur_pb->mode].num_pb_type_children; i++) {
                if (cur_pb->child_pbs[i]) {
                    for (int j = 0; j < pb_type->modes[cur_pb->mode].pb_type_children[i].num_pb; j++) {
                        commit_lookahead_pins_used(&cur_pb->child_pbs[i][j]);
                    }
                }
            }
        }
    }
}

/**
 * Score unclustered atoms that are two hops away from current cluster
 * For example, consider a cluster that has a FF feeding an adder in another
 * cluster. Since this FF is feeding an adder that is packed in another cluster
 * this function should find other FFs that are feeding other inputs of this adder
 * since they are two hops away from the FF packed in this cluster
 */
void load_transitive_fanout_candidates(ClusterBlockId clb_index,
                                       t_pb_stats* pb_stats,
                                       vtr::vector<ClusterBlockId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       int transitive_fanout_threshold) {
    auto& atom_ctx = g_vpr_ctx.atom();

    // iterate over all the nets that have pins in this cluster
    for (const auto net_id : pb_stats->marked_nets) {
        // only consider small nets to constrain runtime
        if (int(atom_ctx.nlist.net_pins(net_id).size()) < transitive_fanout_threshold + 1) {
            // iterate over all the pins of the net
            for (const auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
                AtomBlockId atom_blk_id = atom_ctx.nlist.pin_block(pin_id);
                // get the transitive cluster
                ClusterBlockId tclb = atom_ctx.lookup.atom_clb(atom_blk_id);
                // if the block connected to this pin is packed in another cluster
                if (tclb != clb_index && tclb != ClusterBlockId::INVALID()) {
                    // explore transitive nets from already packed cluster
                    for (AtomNetId tnet : clb_inter_blk_nets[tclb]) {
                        // iterate over all the pins of the net
                        for (AtomPinId tpin : atom_ctx.nlist.net_pins(tnet)) {
                            auto blk_id = atom_ctx.nlist.pin_block(tpin);
                            // This transitive atom is not packed, score and add
                            if (atom_ctx.lookup.atom_clb(blk_id) == ClusterBlockId::INVALID()) {
                                auto& transitive_fanout_candidates = pb_stats->transitive_fanout_candidates;

                                if (pb_stats->gain.count(blk_id) == 0) {
                                    pb_stats->gain[blk_id] = 0.001;
                                } else {
                                    pb_stats->gain[blk_id] += 0.001;
                                }
                                auto rng = atom_ctx.atom_molecules.equal_range(blk_id);
                                for (const auto& kv : vtr::make_range(rng.first, rng.second)) {
                                    t_pack_molecule* molecule = kv.second;
                                    if (molecule->valid) {
                                        transitive_fanout_candidates.insert({molecule->atom_block_ids[molecule->root], molecule});
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types() {
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>> model_candidates;
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& atom_nlist = atom_ctx.nlist;
    auto& device_ctx = g_vpr_ctx.device();

    std::set<const t_model*> unique_models;
    for (auto blk : atom_nlist.blocks()) {
        auto model = atom_nlist.block_model(blk);
        unique_models.insert(model);
    }

    for (auto model : unique_models) {
        model_candidates[model] = {};

        for (auto const& type : device_ctx.logical_block_types) {
            if (block_type_contains_blif_model(&type, model->name)) {
                model_candidates[model].push_back(&type);
            }
        }
    }

    return model_candidates;
}

void print_seed_gains(const char* fname, const std::vector<AtomBlockId>& seed_atoms, const vtr::vector<AtomBlockId, float>& atom_gain, const vtr::vector<AtomBlockId, float>& atom_criticality) {
    FILE* fp = vtr::fopen(fname, "w");

    auto& atom_ctx = g_vpr_ctx.atom();

    //For prett formatting determine the maximum name length
    int max_name_len = strlen("atom_block_name");
    int max_type_len = strlen("atom_block_type");
    for (auto blk_id : atom_ctx.nlist.blocks()) {
        max_name_len = std::max(max_name_len, (int)atom_ctx.nlist.block_name(blk_id).size());

        const t_model* model = atom_ctx.nlist.block_model(blk_id);
        max_type_len = std::max(max_type_len, (int)strlen(model->name));
    }

    fprintf(fp, "%-*s %-*s %8s %8s\n", max_name_len, "atom_block_name", max_type_len, "atom_block_type", "gain", "criticality");
    fprintf(fp, "\n");
    for (auto blk_id : seed_atoms) {
        std::string name = atom_ctx.nlist.block_name(blk_id);
        fprintf(fp, "%-*s ", max_name_len, name.c_str());

        const t_model* model = atom_ctx.nlist.block_model(blk_id);
        fprintf(fp, "%-*s ", max_type_len, model->name);

        fprintf(fp, "%*f ", std::max((int)strlen("gain"), 8), atom_gain[blk_id]);
        fprintf(fp, "%*f ", std::max((int)strlen("criticality"), 8), atom_criticality[blk_id]);
        fprintf(fp, "\n");
    }

    fclose(fp);
}

/**
 * This function takes a chain molecule, and the pb_graph_node that is chosen
 * for packing the molecule's root block. Using the given root_primitive, this
 * function will identify which chain id this molecule is being mapped to and
 * will update the chain id value inside the chain info data structure of this
 * molecule
 */
void update_molecule_chain_info(t_pack_molecule* chain_molecule, const t_pb_graph_node* root_primitive) {
    VTR_ASSERT(chain_molecule->chain_info->chain_id == -1 && chain_molecule->chain_info->is_long_chain);

    auto chain_root_pins = chain_molecule->pack_pattern->chain_root_pins;

    // long chains should only be placed at the beginning of the chain
    // Since for long chains the molecule size is already equal to the
    // total number of adders in the cluster. Therefore, it should
    // always be placed at the very first adder in this cluster.
    for (size_t chainId = 0; chainId < chain_root_pins.size(); chainId++) {
        if (chain_root_pins[chainId][0]->parent_node == root_primitive) {
            chain_molecule->chain_info->chain_id = chainId;
            chain_molecule->chain_info->first_packed_molecule = chain_molecule;
            return;
        }
    }

    VTR_ASSERT(false);
}

/**
 * This function takes the root block of a chain molecule and a proposed
 * placement primitive for this block. The function then checks if this
 * chain root block has a placement constraint (such as being driven from
 * outside the cluster) and returns the status of the placement accordingly.
 */
enum e_block_pack_status check_chain_root_placement_feasibility(const t_pb_graph_node* pb_graph_node,
                                                                const t_pack_molecule* molecule,
                                                                const AtomBlockId blk_id) {
    enum e_block_pack_status block_pack_status = BLK_PASSED;
    auto& atom_ctx = g_vpr_ctx.atom();

    bool is_long_chain = molecule->chain_info->is_long_chain;

    const auto& chain_root_pins = molecule->pack_pattern->chain_root_pins;

    t_model_ports* root_port = chain_root_pins[0][0]->port->model_port;
    AtomNetId chain_net_id;
    auto port_id = atom_ctx.nlist.find_atom_port(blk_id, root_port);

    if (port_id) {
        chain_net_id = atom_ctx.nlist.port_net(port_id, chain_root_pins[0][0]->pin_number);
    }

    // if this block is part of a long chain or it is driven by a cluster
    // input pin we need to check the placement legality of this block
    // Depending on the logic synthesis even small chains that can fit within one
    // cluster might need to start at the top of the cluster as their input can be
    // driven by a global gnd or vdd. Therefore even if this is not a long chain
    // but its input pin is driven by a net, the placement legality is checked.
    if (is_long_chain || chain_net_id) {
        auto chain_id = molecule->chain_info->chain_id;
        // if this chain has a chain id assigned to it (implies is_long_chain too)
        if (chain_id != -1) {
            // the chosen primitive should be a valid starting point for the chain
            // long chains should only be placed at the top of the chain tieOff = 0
            if (pb_graph_node != chain_root_pins[chain_id][0]->parent_node) {
                block_pack_status = BLK_FAILED_FEASIBLE;
            }
            // the chain doesn't have an assigned chain_id yet
        } else {
            block_pack_status = BLK_FAILED_FEASIBLE;
            for (const auto& chain : chain_root_pins) {
                for (size_t tieOff = 0; tieOff < chain.size(); tieOff++) {
                    // check if this chosen primitive is one of the possible
                    // starting points for this chain.
                    if (pb_graph_node == chain[tieOff]->parent_node) {
                        // this location matches with the one of the dedicated chain
                        // input from outside logic block, therefore it is feasible
                        block_pack_status = BLK_PASSED;
                        break;
                    }
                    // long chains should only be placed at the top of the chain tieOff = 0
                    if (is_long_chain) break;
                }
            }
        }
    }

    return block_pack_status;
}

/**
 * This function update the pb_type_count data structure by incrementing
 * the number of used pb_types in the given packed cluster t_pb
 */
size_t update_pb_type_count(const t_pb* pb, std::map<t_pb_type*, int>& pb_type_count, size_t depth) {
    size_t max_depth = depth;

    t_pb_graph_node* pb_graph_node = pb->pb_graph_node;
    t_pb_type* pb_type = pb_graph_node->pb_type;
    t_mode* mode = &pb_type->modes[pb->mode];
    std::string pb_type_name(pb_type->name);

    pb_type_count[pb_type]++;

    if (pb_type->num_modes > 0) {
        for (int i = 0; i < mode->num_pb_type_children; i++) {
            for (int j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                if (pb->child_pbs[i] && pb->child_pbs[i][j].name) {
                    size_t child_depth = update_pb_type_count(&pb->child_pbs[i][j], pb_type_count, depth + 1);

                    max_depth = std::max(max_depth, child_depth);
                }
            }
        }
    }
    return max_depth;
}

void print_pb_type_count_recurr(t_pb_type* pb_type, size_t max_name_chars, size_t curr_depth, std::map<t_pb_type*, int>& pb_type_count) {
    std::string display_name(curr_depth, ' '); //Indent by depth
    display_name += pb_type->name;

    if (pb_type_count.count(pb_type)) {
        VTR_LOG("  %-*s : %d\n", max_name_chars, display_name.c_str(), pb_type_count[pb_type]);
    }

    //Recurse
    for (int imode = 0; imode < pb_type->num_modes; ++imode) {
        t_mode* mode = &pb_type->modes[imode];
        for (int ichild = 0; ichild < mode->num_pb_type_children; ++ichild) {
            t_pb_type* child_pb_type = &mode->pb_type_children[ichild];

            print_pb_type_count_recurr(child_pb_type, max_name_chars, curr_depth + 1, pb_type_count);
        }
    }
}

/**
 * This function identifies the logic block type which is
 * defined by the block type which has a lut primitive
 */
t_logical_block_type_ptr identify_logic_block_type(std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    std::string lut_name = ".names";

    for (auto& model : primitive_candidate_block_types) {
        std::string model_name(model.first->name);
        if (model_name == lut_name)
            return model.second[0];
    }

    return nullptr;
}

/**
 * This function returns the pb_type that is similar to Logic Element (LE) in an FPGA
 * The LE is defined as a physical block that contains a LUT primitive and
 * is found by searching a cluster type to find the first pb_type (from the top
 * of the hierarchy clb->LE) that has more than one instance within the cluster.
 */
t_pb_type* identify_le_block_type(t_logical_block_type_ptr logic_block_type) {
    // if there is no CLB-like cluster, then there is no LE pb_block
    if (!logic_block_type)
        return nullptr;

    // search down the hierarchy starting from the pb_graph_head
    auto pb_graph_node = logic_block_type->pb_graph_head;

    while (pb_graph_node->child_pb_graph_nodes) {
        // if this pb_graph_node has more than one mode or more than one pb_type in the default mode return
        // nullptr since the logic block of this architecture is not a CLB-like logic block
        if (pb_graph_node->pb_type->num_modes > 1 || pb_graph_node->pb_type->modes[0].num_pb_type_children > 1)
            return nullptr;
        // explore the only child of this pb_graph_node
        pb_graph_node = &pb_graph_node->child_pb_graph_nodes[0][0][0];
        // if the child node has more than one instance in the
        // cluster then this is the pb_type similar to a LE
        if (pb_graph_node->pb_type->num_pb > 1)
            return pb_graph_node->pb_type;
    }

    return nullptr;
}

/**
 * This function updates the le_count data structure from the given packed cluster
 */
void update_le_count(const t_pb* pb, const t_logical_block_type_ptr logic_block_type, const t_pb_type* le_pb_type, std::vector<int>& le_count) {
    // if this cluster doesn't contain LEs or there
    // are no les in this architecture, ignore it
    if (!logic_block_type || pb->pb_graph_node != logic_block_type->pb_graph_head || !le_pb_type)
        return;

    const std::string lut(".names");
    const std::string ff(".latch");
    const std::string adder("adder");

    auto parent_pb = pb;

    // go down the hierarchy till the parent physical block of the LE is found
    while (parent_pb->child_pbs[0][0].pb_graph_node->pb_type != le_pb_type) {
        parent_pb = &parent_pb->child_pbs[0][0];
    }

    // iterate over all the LEs and update the LE count accordingly
    for (int ile = 0; ile < parent_pb->get_num_children_of_type(0); ile++) {
        if (!parent_pb->child_pbs[0][ile].name)
            continue;

        auto has_used_lut = pb_used_for_blif_model(&parent_pb->child_pbs[0][ile], lut);
        auto has_used_adder = pb_used_for_blif_model(&parent_pb->child_pbs[0][ile], adder);
        auto has_used_ff = pb_used_for_blif_model(&parent_pb->child_pbs[0][ile], ff);

        // First type of LEs: used for logic and registers
        if ((has_used_lut || has_used_adder) && has_used_ff) {
            le_count[0]++;
            // Second type of LEs: used for logic only
        } else if (has_used_lut || has_used_adder) {
            le_count[1]++;
            // Third type of LEs: used for registers only
        } else if (has_used_ff) {
            le_count[2]++;
        }
    }
}

/**
 * This function returns true if the given physical block has
 * a primitive matching the given blif model and is used
 */
bool pb_used_for_blif_model(const t_pb* pb, std::string blif_model_name) {
    auto pb_graph_node = pb->pb_graph_node;
    auto pb_type = pb_graph_node->pb_type;
    auto mode = &pb_type->modes[pb->mode];

    // if this is a primitive check if it matches the given blif model name
    if (pb_type->blif_model) {
        if (blif_model_name == pb_type->blif_model || ".subckt " + blif_model_name == pb_type->blif_model) {
            return true;
        }
    }

    if (pb_type->num_modes > 0) {
        for (int i = 0; i < mode->num_pb_type_children; i++) {
            for (int j = 0; j < mode->pb_type_children[i].num_pb; j++) {
                if (pb->child_pbs[i] && pb->child_pbs[i][j].name) {
                    if (pb_used_for_blif_model(&pb->child_pbs[i][j], blif_model_name)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/**
 * Print the LE count data strurture
 */
void print_le_count(std::vector<int>& le_count, const t_pb_type* le_pb_type) {
    VTR_LOG("\nLogic Element (%s) detailed count:\n", le_pb_type->name);
    VTR_LOG("  Total number of Logic Elements used : %d\n", le_count[0] + le_count[1] + le_count[2]);
    VTR_LOG("  LEs used for logic and registers    : %d\n", le_count[0]);
    VTR_LOG("  LEs used for logic only             : %d\n", le_count[1]);
    VTR_LOG("  LEs used for registers only         : %d\n\n", le_count[2]);
}

/**
 * Given a pointer to a pb in a cluster, this routine returns
 * a pointer to the top-level pb of the given pb.
 * This is needed when updating the gain for a cluster.
 */
t_pb* get_top_level_pb(t_pb* pb) {
    t_pb* top_level_pb = pb;

    while (pb) {
        top_level_pb = pb;
        pb = pb->parent_pb;
    }

    VTR_ASSERT(top_level_pb != nullptr);

    return top_level_pb;
}
