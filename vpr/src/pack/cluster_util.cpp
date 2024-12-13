#include "cluster_util.h"
#include <algorithm>
#include <unordered_set>

#include "PreClusterTimingGraphResolver.h"
#include "PreClusterDelayCalculator.h"
#include "atom_netlist.h"
#include "cluster_legalizer.h"
#include "cluster_placement.h"
#include "clustered_netlist.h"
#include "concrete_timing_info.h"
#include "output_clustering.h"
#include "prepack.h"
#include "tatum/TimingReporter.hpp"
#include "tatum/echo_writer.hpp"
#include "vpr_context.h"

/**********************************/
/* Global variables in clustering */
/**********************************/

/*Print the contents of each cluster to an echo file*/
static void echo_clusters(char* filename, const ClusterLegalizer& cluster_legalizer) {
    FILE* fp;
    fp = vtr::fopen(filename, "w");

    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "Clusters\n");
    fprintf(fp, "--------------------------------------------------------------\n");
    fprintf(fp, "\n");

    auto& atom_ctx = g_vpr_ctx.atom();

    std::map<LegalizationClusterId, std::vector<AtomBlockId>> cluster_atoms;

    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        cluster_atoms.insert({cluster_id, std::vector<AtomBlockId>()});
    }

    for (auto atom_blk_id : atom_ctx.nlist.blocks()) {
        LegalizationClusterId cluster_id = cluster_legalizer.get_atom_cluster(atom_blk_id);

        cluster_atoms[cluster_id].push_back(atom_blk_id);
    }

    for (auto& cluster_atom : cluster_atoms) {
        const std::string& cluster_name = cluster_legalizer.get_cluster_pb(cluster_atom.first)->name;
        fprintf(fp, "Cluster %s Id: %zu \n", cluster_name.c_str(), size_t(cluster_atom.first));
        fprintf(fp, "\tAtoms in cluster: \n");

        int num_atoms = cluster_atom.second.size();

        for (auto j = 0; j < num_atoms; j++) {
            AtomBlockId atom_id = cluster_atom.second[j];
            fprintf(fp, "\t %s \n", atom_ctx.nlist.block_name(atom_id).c_str());
        }
    }

    fprintf(fp, "\nCluster Floorplanning Constraints:\n");

    for (LegalizationClusterId cluster_id : cluster_legalizer.clusters()) {
        const std::vector<Region>& regions = cluster_legalizer.get_cluster_pr(cluster_id).get_regions();
        if (!regions.empty()) {
            fprintf(fp, "\nRegions in Cluster %zu:\n", size_t(cluster_id));
            for (const auto& region : regions) {
                print_region(fp, region);
            }
        }
    }

    fclose(fp);
}

void calc_init_packing_timing(const t_packer_opts& packer_opts,
                              const t_analysis_opts& analysis_opts,
                              const Prepacker& prepacker,
                              std::shared_ptr<PreClusterDelayCalculator>& clustering_delay_calc,
                              std::shared_ptr<SetupTimingInfo>& timing_info,
                              vtr::vector<AtomBlockId, float>& atom_criticality) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    /*
     * Initialize the timing analyzer
     */
    clustering_delay_calc = std::make_shared<PreClusterDelayCalculator>(atom_ctx.nlist, atom_ctx.lookup, packer_opts.inter_cluster_net_delay, prepacker);
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

void free_clustering_data(t_clustering_data& clustering_data) {
    delete[] clustering_data.unclustered_list_head;
    delete[] clustering_data.memory_pool;
}

void check_and_output_clustering(ClusterLegalizer& cluster_legalizer,
                                 const t_packer_opts& packer_opts,
                                 const std::unordered_set<AtomNetId>& is_clock,
                                 const t_arch* arch) {
    cluster_legalizer.verify();

    if (getEchoEnabled() && isEchoFileEnabled(E_ECHO_CLUSTERS)) {
        echo_clusters(getEchoFileName(E_ECHO_CLUSTERS), cluster_legalizer);
    }

    output_clustering(&cluster_legalizer,
                      packer_opts.global_clocks,
                      is_clock,
                      arch->architecture_id,
                      packer_opts.output_file.c_str(),
                      false, /*skip_clustering*/
                      true /*from_legalizer*/);
}

void print_pack_status_header() {
    VTR_LOG("Starting Clustering - Clustering Progress: \n");
    VTR_LOG("-------------------   --------------------------   ---------\n");
    VTR_LOG("Molecules processed   Number of clusters created   FPGA size\n");
    VTR_LOG("-------------------   --------------------------   ---------\n");
}

void print_pack_status(int tot_num_molecules,
                       int num_molecules_processed,
                       int& mols_since_last_print,
                       int device_width,
                       int device_height,
                       AttractionInfo& attraction_groups,
                       const ClusterLegalizer& cluster_legalizer) {
    //Print a packing update each time another 4% of molecules have been packed.
    const float print_frequency = 0.04;

    double percentage = (num_molecules_processed / (double)tot_num_molecules) * 100;

    int int_percentage = int(percentage);

    int int_molecule_increment = (int)(print_frequency * tot_num_molecules);

    int num_clusters_created = cluster_legalizer.clusters().size();

    if (mols_since_last_print >= int_molecule_increment ||
        num_molecules_processed == tot_num_molecules) {
        VTR_LOG(
            "%6d/%-6d  %3d%%   "
            "%26d   "
            "%3d x %-3d   ",
            num_molecules_processed,
            tot_num_molecules,
            int_percentage,
            num_clusters_created,
            device_width,
            device_height);

        VTR_LOG("\n");
        fflush(stdout);
        mols_since_last_print = 0;
        // FIXME: This really should not be here. This has nothing to do with
        //        printing the pack status! Abstract this into the candidate
        //        selector class.
        if (attraction_groups.num_attraction_groups() > 0) {
            rebuild_attraction_groups(attraction_groups, cluster_legalizer);
        }
    }
}

void rebuild_attraction_groups(AttractionInfo& attraction_groups,
                               const ClusterLegalizer& cluster_legalizer) {

    for (int igroup = 0; igroup < attraction_groups.num_attraction_groups(); igroup++) {
        AttractGroupId group_id(igroup);
        AttractionGroup& group = attraction_groups.get_attraction_group_info(group_id);
        AttractionGroup new_att_group_info;

        for (AtomBlockId atom : group.group_atoms) {
            if (!cluster_legalizer.is_atom_clustered(atom)) {
                new_att_group_info.group_atoms.push_back(atom);
            }
        }

        attraction_groups.set_attraction_group_info(group_id, new_att_group_info);
    }
}

bool is_atom_blk_in_pb(const AtomBlockId blk_id, const t_pb* pb) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    const t_pb* cur_pb = atom_ctx.lookup.atom_pb(blk_id);
    while (cur_pb) {
        if (cur_pb == pb) {
            return true;
        }
        cur_pb = cur_pb->parent_pb;
    }
    return false;
}

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
                               const Prepacker& prepacker,
                               t_clustering_data& clustering_data,
                               int num_molecules) {
    /* Allocates the main data structures used for clustering and properly *
     * initializes them.                                                   */
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    /* alloc and load list of molecules to pack */
    clustering_data.unclustered_list_head = new t_molecule_link[max_molecule_stats.num_used_ext_inputs + 1];
    clustering_data.unclustered_list_head_size = max_molecule_stats.num_used_ext_inputs + 1;

    for (int i = 0; i <= max_molecule_stats.num_used_ext_inputs; i++) {
        clustering_data.unclustered_list_head[i] = t_molecule_link();
        clustering_data.unclustered_list_head[i].next = nullptr;
    }

    // Create a sorted list of molecules, sorted on increasing molecule base gain.
    std::vector<t_pack_molecule*> molecules_vector = prepacker.get_molecules_vector();
    VTR_ASSERT(molecules_vector.size() == (size_t)num_molecules);
    std::stable_sort(molecules_vector.begin(),
                     molecules_vector.end(),
                     [](t_pack_molecule* a, t_pack_molecule* b) {
                        return a->base_gain < b->base_gain;
                     });

    clustering_data.memory_pool = new t_molecule_link[num_molecules];
    t_molecule_link* next_ptr = clustering_data.memory_pool;

    for (t_pack_molecule* mol : molecules_vector) {
        //Figure out how many external inputs are used by this molecule
        t_molecule_stats molecule_stats = calc_molecule_stats(mol, atom_ctx.nlist);
        int ext_inps = molecule_stats.num_used_ext_inputs;

        //Insert the molecule into the unclustered lists by number of external inputs
        next_ptr->moleculeptr = mol;
        next_ptr->next = clustering_data.unclustered_list_head[ext_inps].next;
        clustering_data.unclustered_list_head[ext_inps].next = next_ptr;

        next_ptr++;
    }
}

/*****************************************/

t_pack_molecule* get_molecule_by_num_ext_inputs(const int ext_inps,
                                                const enum e_removal_policy remove_flag,
                                                t_molecule_link* unclustered_list_head,
                                                LegalizationClusterId legalization_cluster_id,
                                                const ClusterLegalizer& cluster_legalizer) {

    t_molecule_link* prev_ptr = &unclustered_list_head[ext_inps];
    t_molecule_link* ptr = unclustered_list_head[ext_inps].next;
    while (ptr != nullptr) {
        /* TODO: Get better candidate atom block in future, eg. return most timing critical or some other smarter metric */
        if (!cluster_legalizer.is_mol_clustered(ptr->moleculeptr)) {
            /* TODO: I should be using a better filtering check especially when I'm
             * dealing with multiple clock/multiple global reset signals where the clock/reset
             * packed in matters, need to do later when I have the circuits to check my work */
            if (cluster_legalizer.is_molecule_compatible(ptr->moleculeptr, legalization_cluster_id)) {
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
                                                                    t_molecule_link* unclustered_list_head,
                                                                    const int& unclustered_list_head_size,
                                                                    LegalizationClusterId legalization_cluster_id,
                                                                    const ClusterLegalizer& cluster_legalizer) {
    /*
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
        molecule = get_molecule_by_num_ext_inputs(ext_inps,
                                                  LEAVE_CLUSTERED,
                                                  unclustered_list_head,
                                                  legalization_cluster_id,
                                                  cluster_legalizer);
        if (molecule != nullptr) {
            break;
        }
    }
    return molecule;
}

/*****************************************/

void update_connection_gain_values(const AtomNetId net_id,
                                   const AtomBlockId clustered_blk_id,
                                   t_pb* cur_pb,
                                   const ClusterLegalizer& cluster_legalizer,
                                   enum e_net_relation_to_clustered_block net_relation_to_clustered_block) {
    /*This function is called when the connectiongain values on the net net_id*
     *require updating.   */
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    int num_internal_connections, num_open_connections, num_stuck_connections;

    num_internal_connections = num_open_connections = num_stuck_connections = 0;

    LegalizationClusterId legalization_cluster_id = cluster_legalizer.get_atom_cluster(clustered_blk_id);

    /* may wish to speed things up by ignoring clock nets since they are high fanout */

    for (auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
        auto blk_id = atom_ctx.nlist.pin_block(pin_id);
        if (cluster_legalizer.get_atom_cluster(blk_id) == legalization_cluster_id
            && is_atom_blk_in_pb(blk_id, atom_ctx.lookup.atom_pb(clustered_blk_id))) {
            num_internal_connections++;
        } else if (!cluster_legalizer.is_atom_clustered(blk_id)) {
            num_open_connections++;
        } else {
            num_stuck_connections++;
        }
    }

    if (net_relation_to_clustered_block == OUTPUT) {
        for (auto pin_id : atom_ctx.nlist.net_sinks(net_id)) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
            VTR_ASSERT(blk_id);

            if (!cluster_legalizer.is_atom_clustered(blk_id)) {
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

        if (!cluster_legalizer.is_atom_clustered(blk_id)) {
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

/*****************************************/
void update_timing_gain_values(const AtomNetId net_id,
                               t_pb* cur_pb,
                               const ClusterLegalizer& cluster_legalizer,
                               enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                               const SetupTimingInfo& timing_info,
                               const std::unordered_set<AtomNetId>& is_global,
                               const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input) {
    /*This function is called when the timing_gain values on the atom net*
     *net_id requires updating.   */
    float timinggain;

    const AtomContext& atom_ctx = g_vpr_ctx.atom();

    /* Check if this atom net lists its driving atom block twice.  If so, avoid  *
     * double counting this atom block by skipping the first (driving) pin. */
    auto pins = atom_ctx.nlist.net_pins(net_id);
    if (net_output_feeds_driving_block_input.count(net_id) != 0)
        pins = atom_ctx.nlist.net_sinks(net_id);

    if (net_relation_to_clustered_block == OUTPUT
        && !is_global.count(net_id)) {
        for (auto pin_id : pins) {
            auto blk_id = atom_ctx.nlist.pin_block(pin_id);
            if (!cluster_legalizer.is_atom_clustered(blk_id)) {
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

        if (!cluster_legalizer.is_atom_clustered(new_blk_id)) {
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
                                  const ClusterLegalizer& cluster_legalizer,
                                  bool timing_driven,
                                  bool connection_driven,
                                  enum e_net_relation_to_clustered_block net_relation_to_clustered_block,
                                  const SetupTimingInfo& timing_info,
                                  const std::unordered_set<AtomNetId>& is_global,
                                  const int high_fanout_net_threshold,
                                  const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input) {

    const AtomContext& atom_ctx = g_vpr_ctx.atom();
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
        if (net_output_feeds_driving_block_input.count(net_id) != 0)
            //We implicitly assume here that net_output_feeds_driver_block_input[net_id] is 2
            //(i.e. the net loops back to the block only once)
            pins = atom_ctx.nlist.net_sinks(net_id);

        if (cur_pb->pb_stats->num_pins_of_net_in_pb.count(net_id) == 0) {
            for (auto pin_id : pins) {
                auto blk_id = atom_ctx.nlist.pin_block(pin_id);
                if (!cluster_legalizer.is_atom_clustered(blk_id)) {
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
                                          cluster_legalizer,
                                          net_relation_to_clustered_block);
        }

        if (timing_driven) {
            update_timing_gain_values(net_id, cur_pb, cluster_legalizer,
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
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
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
         * be made more efficient once we decide on which one to use */
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
                          const ClusterLegalizer& cluster_legalizer,
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
                          const std::unordered_set<AtomNetId>& net_output_feeds_driving_block_input) {

    int molecule_size;
    int iblock;
    t_pb *cur_pb, *cb;

    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    molecule_size = get_array_size_of_molecule(molecule);
    cb = nullptr;

    for (iblock = 0; iblock < molecule_size; iblock++) {
        auto blk_id = molecule->atom_block_ids[iblock];
        if (!blk_id) {
            continue;
        }

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
                mark_and_update_partial_gain(net_id, GAIN, blk_id, cluster_legalizer,
                                             timing_driven,
                                             connection_driven, OUTPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            } else {
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id, cluster_legalizer,
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
            mark_and_update_partial_gain(net_id, GAIN, blk_id, cluster_legalizer,
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
                mark_and_update_partial_gain(net_id, NO_GAIN, blk_id, cluster_legalizer,
                                             timing_driven, connection_driven, INPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            } else {
                mark_and_update_partial_gain(net_id, GAIN, blk_id, cluster_legalizer,
                                             timing_driven, connection_driven, INPUT,
                                             timing_info,
                                             is_global,
                                             high_fanout_net_threshold,
                                             net_output_feeds_driving_block_input);
            }
        }

        update_total_gain(alpha, beta, timing_driven, connection_driven,
                          atom_pb->parent_pb, attraction_groups);
    }

    // if this molecule came from the transitive fanout candidates remove it
    if (cb) {
        cb->pb_stats->transitive_fanout_candidates.erase(molecule->atom_block_ids[molecule->root]);
        cb->pb_stats->explore_transitive_fanout = true;
    }
}

t_pack_molecule* get_highest_gain_molecule(t_pb* cur_pb,
                                           AttractionInfo& attraction_groups,
                                           const enum e_gain_type gain_mode,
                                           const Prepacker& prepacker,
                                           const ClusterLegalizer& cluster_legalizer,
                                           vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                           const LegalizationClusterId legalization_cluster_id,
                                           bool prioritize_transitive_connectivity,
                                           int transitive_fanout_threshold,
                                           const int feasible_block_array_size,
                                           const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
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
        add_cluster_molecule_candidates_by_connectivity_and_timing(cur_pb,
                                                                   legalization_cluster_id,
                                                                   prepacker,
                                                                   cluster_legalizer,
                                                                   feasible_block_array_size,
                                                                   attraction_groups);
    }

    if (prioritize_transitive_connectivity) {
        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cur_pb,
                                                                       prepacker,
                                                                       cluster_legalizer,
                                                                       clb_inter_blk_nets,
                                                                       legalization_cluster_id,
                                                                       transitive_fanout_threshold,
                                                                       feasible_block_array_size,
                                                                       attraction_groups);
        }

        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cur_pb,
                                                                       legalization_cluster_id,
                                                                       prepacker,
                                                                       cluster_legalizer,
                                                                       feasible_block_array_size,
                                                                       attraction_groups);
        }
    } else { //Reverse order
        // 3. Find unpacked molecules based on weak connectedness (connected by high fanout nets) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->tie_break_high_fanout_net) {
            add_cluster_molecule_candidates_by_highfanout_connectivity(cur_pb,
                                                                       legalization_cluster_id,
                                                                       prepacker,
                                                                       cluster_legalizer,
                                                                       feasible_block_array_size,
                                                                       attraction_groups);
        }

        // 2. Find unpacked molecules based on transitive connections (eg. 2 hops away) with current cluster
        if (cur_pb->pb_stats->num_feasible_blocks == 0 && cur_pb->pb_stats->explore_transitive_fanout) {
            add_cluster_molecule_candidates_by_transitive_connectivity(cur_pb,
                                                                       prepacker,
                                                                       cluster_legalizer,
                                                                       clb_inter_blk_nets,
                                                                       legalization_cluster_id,
                                                                       transitive_fanout_threshold,
                                                                       feasible_block_array_size,
                                                                       attraction_groups);
        }
    }

    // 4. Find unpacked molecules based on attraction group of the current cluster (if the cluster has an attraction group)
    if (cur_pb->pb_stats->num_feasible_blocks == 0) {
        add_cluster_molecule_candidates_by_attraction_group(cur_pb,
                                                            prepacker,
                                                            cluster_legalizer,
                                                            attraction_groups,
                                                            feasible_block_array_size,
                                                            legalization_cluster_id,
                                                            primitive_candidate_block_types);
    }
    /* Grab highest gain molecule */
    t_pack_molecule* molecule = nullptr;
    if (cur_pb->pb_stats->num_feasible_blocks > 0) {
        cur_pb->pb_stats->num_feasible_blocks--;
        int index = cur_pb->pb_stats->num_feasible_blocks;
        molecule = cur_pb->pb_stats->feasible_blocks[index];
        VTR_ASSERT(!cluster_legalizer.is_mol_clustered(molecule));
        return molecule;
    }

    return molecule;
}

void add_cluster_molecule_candidates_by_connectivity_and_timing(t_pb* cur_pb,
                                                                LegalizationClusterId legalization_cluster_id,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    VTR_ASSERT(cur_pb->pb_stats->num_feasible_blocks == NOT_VALID);

    cur_pb->pb_stats->num_feasible_blocks = 0;
    cur_pb->pb_stats->explore_transitive_fanout = true; /* If no legal molecules found, enable exploration of molecules two hops away */

    for (AtomBlockId blk_id : cur_pb->pb_stats->marked_blocks) {
        if (!cluster_legalizer.is_atom_clustered(blk_id)) {
            t_pack_molecule* molecule = prepacker.get_atom_molecule(blk_id);
            if (!cluster_legalizer.is_mol_clustered(molecule)) {
                if (cluster_legalizer.is_molecule_compatible(molecule, legalization_cluster_id)) {
                    add_molecule_to_pb_stats_candidates(molecule,
                                                        cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
                }
            }
        }
    }
}

void add_cluster_molecule_candidates_by_highfanout_connectivity(t_pb* cur_pb,
                                                                LegalizationClusterId legalization_cluster_id,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    /* Because the packer ignores high fanout nets when marking what blocks
     * to consider, use one of the ignored high fanout net to fill up lightly
     * related blocks */
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;

    AtomNetId net_id = cur_pb->pb_stats->tie_break_high_fanout_net;

    int count = 0;
    for (auto pin_id : atom_nlist.net_pins(net_id)) {
        if (count >= AAPACK_MAX_HIGH_FANOUT_EXPLORE) {
            break;
        }

        AtomBlockId blk_id = atom_nlist.pin_block(pin_id);

        if (!cluster_legalizer.is_atom_clustered(blk_id)) {
            t_pack_molecule* molecule = prepacker.get_atom_molecule(blk_id);
            if (!cluster_legalizer.is_mol_clustered(molecule)) {
                if (cluster_legalizer.is_molecule_compatible(molecule, legalization_cluster_id)) {
                    add_molecule_to_pb_stats_candidates(molecule,
                                                        cur_pb->pb_stats->gain, cur_pb, std::min(feasible_block_array_size, AAPACK_MAX_HIGH_FANOUT_EXPLORE), attraction_groups);
                    count++;
                }
            }
        }
    }
    cur_pb->pb_stats->tie_break_high_fanout_net = AtomNetId::INVALID(); /* Mark off that this high fanout net has been considered */
}

void add_cluster_molecule_candidates_by_attraction_group(t_pb* cur_pb,
                                                         const Prepacker& prepacker,
                                                         const ClusterLegalizer& cluster_legalizer,
                                                         AttractionInfo& attraction_groups,
                                                         const int feasible_block_array_size,
                                                         LegalizationClusterId legalization_cluster_id,
                                                         const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;

    auto cluster_type = cluster_legalizer.get_cluster_type(legalization_cluster_id);

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
        const auto& atom_model = atom_nlist.block_model(atom_id);
        auto itr = primitive_candidate_block_types.find(atom_model);
        VTR_ASSERT(itr != primitive_candidate_block_types.end());
        const std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

        //Only consider molecules that are unpacked and of the correct type
        if (!cluster_legalizer.is_atom_clustered(atom_id)
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
            const auto& atom_model = atom_nlist.block_model(atom_id);
            auto itr = primitive_candidate_block_types.find(atom_model);
            VTR_ASSERT(itr != primitive_candidate_block_types.end());
            const std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

            //Only consider molecules that are unpacked and of the correct type
            if (!cluster_legalizer.is_atom_clustered(atom_id)
                && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
                t_pack_molecule* molecule = prepacker.get_atom_molecule(atom_id);
                if (!cluster_legalizer.is_mol_clustered(molecule)) {
                    if (cluster_legalizer.is_molecule_compatible(molecule, legalization_cluster_id)) {
                        add_molecule_to_pb_stats_candidates(molecule,
                                                            cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
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
        const auto& atom_model = atom_nlist.block_model(blk_id);
        auto itr = primitive_candidate_block_types.find(atom_model);
        VTR_ASSERT(itr != primitive_candidate_block_types.end());
        const std::vector<t_logical_block_type_ptr>& candidate_types = itr->second;

        //Only consider molecules that are unpacked and of the correct type
        if (!cluster_legalizer.is_atom_clustered(blk_id)
            && std::find(candidate_types.begin(), candidate_types.end(), cluster_type) != candidate_types.end()) {
            t_pack_molecule* molecule = prepacker.get_atom_molecule(blk_id);
            if (!cluster_legalizer.is_mol_clustered(molecule)) {
                if (cluster_legalizer.is_molecule_compatible(molecule, legalization_cluster_id)) {
                    add_molecule_to_pb_stats_candidates(molecule,
                                                        cur_pb->pb_stats->gain, cur_pb, feasible_block_array_size, attraction_groups);
                }
            }
        }
    }
}

void add_cluster_molecule_candidates_by_transitive_connectivity(t_pb* cur_pb,
                                                                const Prepacker& prepacker,
                                                                const ClusterLegalizer& cluster_legalizer,
                                                                vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                                                const LegalizationClusterId legalization_cluster_id,
                                                                int transitive_fanout_threshold,
                                                                const int feasible_block_array_size,
                                                                AttractionInfo& attraction_groups) {
    //TODO: For now, only done by fan-out; should also consider fan-in
    cur_pb->pb_stats->explore_transitive_fanout = false;

    /* First time finding transitive fanout candidates therefore alloc and load them */
    load_transitive_fanout_candidates(legalization_cluster_id,
                                      cur_pb->pb_stats,
                                      prepacker,
                                      cluster_legalizer,
                                      clb_inter_blk_nets,
                                      transitive_fanout_threshold);
    /* Only consider candidates that pass a very simple legality check */
    for (const auto& transitive_candidate : cur_pb->pb_stats->transitive_fanout_candidates) {
        t_pack_molecule* molecule = transitive_candidate.second;
        if (!cluster_legalizer.is_mol_clustered(molecule)) {
            if (cluster_legalizer.is_molecule_compatible(molecule, legalization_cluster_id)) {
                add_molecule_to_pb_stats_candidates(molecule,
                                                    cur_pb->pb_stats->gain, cur_pb, std::min(feasible_block_array_size, AAPACK_MAX_TRANSITIVE_EXPLORE), attraction_groups);
            }
        }
    }
}

/*****************************************/
t_pack_molecule* get_molecule_for_cluster(t_pb* cur_pb,
                                          AttractionInfo& attraction_groups,
                                          const bool allow_unrelated_clustering,
                                          const bool prioritize_transitive_connectivity,
                                          const int transitive_fanout_threshold,
                                          const int feasible_block_array_size,
                                          int* num_unrelated_clustering_attempts,
                                          const Prepacker& prepacker,
                                          const ClusterLegalizer& cluster_legalizer,
                                          vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                          LegalizationClusterId legalization_cluster_id,
                                          int verbosity,
                                          t_molecule_link* unclustered_list_head,
                                          const int& unclustered_list_head_size,
                                          const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    /* Finds the block with the greatest gain that satisfies the
     * input, clock and capacity constraints of a cluster that are
     * passed in.  If no suitable block is found it returns nullptr.
     */

    VTR_ASSERT(cur_pb->is_root());

    /* If cannot pack into primitive, try packing into cluster */

    auto best_molecule = get_highest_gain_molecule(cur_pb, attraction_groups,
                                                   NOT_HILL_CLIMBING,
                                                   prepacker, cluster_legalizer, clb_inter_blk_nets,
                                                   legalization_cluster_id, prioritize_transitive_connectivity,
                                                   transitive_fanout_threshold, feasible_block_array_size, primitive_candidate_block_types);

    /* If no blocks have any gain to the current cluster, the code above      *
     * will not find anything.  However, another atom block with no inputs in *
     * common with the cluster may still be inserted into the cluster.        */

    if (allow_unrelated_clustering) {
        if (best_molecule == nullptr) {
            if (*num_unrelated_clustering_attempts == 0) {
                best_molecule = get_free_molecule_with_most_ext_inputs_for_cluster(cur_pb,
                                                                                   unclustered_list_head,
                                                                                   unclustered_list_head_size,
                                                                                   legalization_cluster_id,
                                                                                   cluster_legalizer);
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

t_molecule_stats calc_molecule_stats(const t_pack_molecule* molecule, const AtomNetlist& atom_nlist) {
    t_molecule_stats molecule_stats;

    //Calculate the number of available pins on primitives within the molecule
    for (auto blk : molecule->atom_block_ids) {
        if (!blk) continue;

        ++molecule_stats.num_blocks; //Record number of valid blocks in molecule

        const t_model* model = atom_nlist.block_model(blk);

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

        for (auto pin : atom_nlist.block_pins(blk)) {
            auto net = atom_nlist.pin_net(pin);

            auto pin_type = atom_nlist.pin_type(pin);
            if (pin_type == PinType::SINK) {
                auto driver_blk = atom_nlist.net_driver_block(net);

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
                for (auto sink_pin : atom_nlist.net_sinks(net)) {
                    auto sink_blk = atom_nlist.pin_block(sink_pin);

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

float get_molecule_gain(t_pack_molecule* molecule, std::map<AtomBlockId, float>& blk_gain, AttractGroupId cluster_attraction_group_id, AttractionInfo& attraction_groups, int num_molecule_failures) {
    float gain;
    int i;
    int num_introduced_inputs_of_indirectly_related_block;
    const AtomContext& atom_ctx = g_vpr_ctx.atom();

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

void load_transitive_fanout_candidates(LegalizationClusterId legalization_cluster_id,
                                       t_pb_stats* pb_stats,
                                       const Prepacker& prepacker,
                                       const ClusterLegalizer& cluster_legalizer,
                                       vtr::vector<LegalizationClusterId, std::vector<AtomNetId>>& clb_inter_blk_nets,
                                       int transitive_fanout_threshold) {
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;

    // iterate over all the nets that have pins in this cluster
    for (const auto net_id : pb_stats->marked_nets) {
        // only consider small nets to constrain runtime
        if (int(atom_nlist.net_pins(net_id).size()) < transitive_fanout_threshold + 1) {
            // iterate over all the pins of the net
            for (const auto pin_id : atom_nlist.net_pins(net_id)) {
                AtomBlockId atom_blk_id = atom_nlist.pin_block(pin_id);
                // get the transitive cluster
                LegalizationClusterId tclb = cluster_legalizer.get_atom_cluster(atom_blk_id);
                // if the block connected to this pin is packed in another cluster
                if (tclb != legalization_cluster_id && tclb != LegalizationClusterId::INVALID()) {
                    // explore transitive nets from already packed cluster
                    for (AtomNetId tnet : clb_inter_blk_nets[tclb]) {
                        // iterate over all the pins of the net
                        for (AtomPinId tpin : atom_nlist.net_pins(tnet)) {
                            auto blk_id = atom_nlist.pin_block(tpin);
                            // This transitive atom is not packed, score and add
                            if (!cluster_legalizer.is_atom_clustered(blk_id)) {
                                auto& transitive_fanout_candidates = pb_stats->transitive_fanout_candidates;

                                if (pb_stats->gain.count(blk_id) == 0) {
                                    pb_stats->gain[blk_id] = 0.001;
                                } else {
                                    pb_stats->gain[blk_id] += 0.001;
                                }
                                t_pack_molecule* molecule = prepacker.get_atom_molecule(blk_id);
                                if (!cluster_legalizer.is_mol_clustered(molecule)) {
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

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types() {
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>> model_candidates;
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().nlist;
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    std::set<const t_model*> unique_models;
    // Find all logic models used in the netlist
    for (auto blk : atom_nlist.blocks()) {
        auto model = atom_nlist.block_model(blk);
        unique_models.insert(model);
    }

    /* For each technology-mapped logic model, find logical block types
     * that can accommodate that logic model
     */
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

std::unordered_set<AtomNetId> identify_net_output_feeds_driving_block_input(const AtomNetlist& atom_netlist) {
    std::unordered_set<AtomNetId> net_output_feeds_driving_block_input;

    for (AtomNetId net : atom_netlist.nets()) {
        AtomPinId driver_pin = atom_netlist.net_driver(net);
        AtomBlockId driver_block = atom_netlist.pin_block(driver_pin);

        for (AtomPinId sink_pin : atom_netlist.net_sinks(net)) {
            AtomBlockId sink_block = atom_netlist.pin_block(sink_pin);

            if (driver_block == sink_block) {
                net_output_feeds_driving_block_input.insert(net);
                break;
            }
        }
    }

    return net_output_feeds_driving_block_input;
}

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

void print_pb_type_count(const ClusteredNetlist& clb_nlist) {
    auto& device_ctx = g_vpr_ctx.device();

    std::map<t_pb_type*, int> pb_type_count;

    size_t max_depth = 0;
    for (ClusterBlockId blk : clb_nlist.blocks()) {
        size_t pb_max_depth = update_pb_type_count(clb_nlist.block_pb(blk), pb_type_count, 0);

        max_depth = std::max(max_depth, pb_max_depth);
    }

    size_t max_pb_type_name_chars = 0;
    for (auto& pb_type : pb_type_count) {
        max_pb_type_name_chars = std::max(max_pb_type_name_chars, strlen(pb_type.first->name));
    }

    VTR_LOG("\nPb types usage...\n");
    for (const auto& logical_block_type : device_ctx.logical_block_types) {
        if (!logical_block_type.pb_type) continue;

        print_pb_type_count_recurr(logical_block_type.pb_type, max_pb_type_name_chars + max_depth, 0, pb_type_count);
    }
    VTR_LOG("\n");
}

t_logical_block_type_ptr identify_logic_block_type(const std::map<const t_model*, std::vector<t_logical_block_type_ptr>>& primitive_candidate_block_types) {
    std::string lut_name = ".names";

    for (auto& model : primitive_candidate_block_types) {
        std::string model_name(model.first->name);
        if (model_name == lut_name)
            return model.second[0];
    }

    return nullptr;
}

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

void update_le_count(const t_pb* pb,
                     const t_logical_block_type_ptr logic_block_type,
                     const t_pb_type* le_pb_type,
                     int& num_logic_le,
                     int& num_reg_le,
                     int& num_logic_and_reg_le) {
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

        if ((has_used_lut || has_used_adder) && has_used_ff) {
            // First type of LEs: used for logic and registers
            num_logic_and_reg_le++;
        } else if (has_used_lut || has_used_adder) {
            // Second type of LEs: used for logic only
            num_logic_le++;
        } else if (has_used_ff) {
            // Third type of LEs: used for registers only
            num_reg_le++;
        }
    }
}

bool pb_used_for_blif_model(const t_pb* pb, const std::string& blif_model_name) {
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

void print_le_count(int num_logic_le,
                    int num_reg_le,
                    int num_logic_and_reg_le,
                    const t_pb_type* le_pb_type) {
    VTR_ASSERT(le_pb_type != nullptr);

    int num_total_le = num_logic_and_reg_le + num_logic_le + num_reg_le;

    VTR_LOG("\nLogic Element (%s) detailed count:\n", le_pb_type->name);
    VTR_LOG("  Total number of Logic Elements used : %d\n", num_total_le);
    VTR_LOG("  LEs used for logic and registers    : %d\n", num_logic_and_reg_le);
    VTR_LOG("  LEs used for logic only             : %d\n", num_logic_le);
    VTR_LOG("  LEs used for registers only         : %d\n\n", num_reg_le);
}

t_pb* get_top_level_pb(t_pb* pb) {
    t_pb* top_level_pb = pb;

    while (pb) {
        top_level_pb = pb;
        pb = pb->parent_pb;
    }

    VTR_ASSERT(top_level_pb != nullptr);

    return top_level_pb;
}

void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup,
                           const AtomContext& atom_ctx,
                           const ClusteredNetlist& clb_nlist) {
    // Resize the atoms lookup to the number of clusters.
    atoms_lookup.resize(clb_nlist.blocks().size());
    for (AtomBlockId atom_blk_id : atom_ctx.nlist.blocks()) {
        // Get the CLB that this atom is packed into.
        ClusterBlockId clb_index = atom_ctx.lookup.atom_clb(atom_blk_id);
        // Every atom block should be in a cluster.
        VTR_ASSERT_SAFE(clb_index.is_valid());
        // Insert this clb into the lookup's set.
        atoms_lookup[clb_index].insert(atom_blk_id);
    }
}
