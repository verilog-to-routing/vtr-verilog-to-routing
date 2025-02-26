#include "cluster_util.h"
#include <algorithm>
#include <unordered_set>

#include "PreClusterTimingGraphResolver.h"
#include "PreClusterDelayCalculator.h"
#include "atom_netlist.h"
#include "attraction_groups.h"
#include "cluster_legalizer.h"
#include "clustered_netlist.h"
#include "concrete_timing_info.h"
#include "output_clustering.h"
#include "prepack.h"
#include "tatum/TimingReporter.hpp"
#include "tatum/echo_writer.hpp"
#include "vpr_context.h"

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

    for (auto atom_blk_id : atom_ctx.netlist().blocks()) {
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
            fprintf(fp, "\t %s \n", atom_ctx.netlist().block_name(atom_id).c_str());
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
    clustering_delay_calc = std::make_shared<PreClusterDelayCalculator>(atom_ctx.netlist(), atom_ctx.lookup(), packer_opts.inter_cluster_net_delay, prepacker);
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
        PreClusterTimingGraphResolver resolver(atom_ctx.netlist(),
                                               atom_ctx.lookup(), *timing_ctx.graph, *clustering_delay_calc);
        resolver.set_detail_level(analysis_opts.timing_report_detail);

        tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                              *timing_ctx.constraints);

        timing_reporter.report_timing_setup(
            "pre_pack.report_timing.setup.rpt",
            *timing_info->setup_analyzer(),
            analysis_opts.timing_report_npaths);
    }

    //Calculate true criticalities of each block
    for (AtomBlockId blk : atom_ctx.netlist().blocks()) {
        for (AtomPinId in_pin : atom_ctx.netlist().block_input_pins(blk)) {
            //Max criticality over incoming nets
            float crit = timing_info->setup_pin_criticality(in_pin);
            atom_criticality[blk] = std::max(atom_criticality[blk], crit);
        }
    }
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

/*****************************************/

std::map<const t_model*, std::vector<t_logical_block_type_ptr>> identify_primitive_candidate_block_types() {
    std::map<const t_model*, std::vector<t_logical_block_type_ptr>> model_candidates;
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().netlist();
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

void init_clb_atoms_lookup(vtr::vector<ClusterBlockId, std::unordered_set<AtomBlockId>>& atoms_lookup,
                           const AtomContext& atom_ctx,
                           const ClusteredNetlist& clb_nlist) {
    // Resize the atoms lookup to the number of clusters.
    atoms_lookup.resize(clb_nlist.blocks().size());
    for (AtomBlockId atom_blk_id : atom_ctx.netlist().blocks()) {
        // Get the CLB that this atom is packed into.
        ClusterBlockId clb_index = atom_ctx.lookup().atom_clb(atom_blk_id);
        // Every atom block should be in a cluster.
        VTR_ASSERT_SAFE(clb_index.is_valid());
        // Insert this clb into the lookup's set.
        atoms_lookup[clb_index].insert(atom_blk_id);
    }
}

