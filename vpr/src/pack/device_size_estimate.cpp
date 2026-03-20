/**
 * @file
 * @author  Haydar Cakan
 * @date    March 2026
 * @brief   Implementation of device size estimation before packing.
 *
 * Non-RAM molecules are grouped by logical block type and packed into virtual
 * clusters using a lightweight pin-capacity and cluster placement feasibility to
 * estimate the number of physical instances required. RAM atoms are grouped
 * into sibling-feasible equivalence classes and assigned to the minimum-area
 * physical RAM type. The resulting RAM groups are stored in ram_groups_ and
 * exposed via ram_groups() for reuse by RamMapper, avoiding redundant work.
 */

#include "device_size_estimate.h"

#include <unordered_set>

#include "cluster_placement.h"
#include "cluster_util.h"
#include "globals.h"
#include "prepack.h"
#include "setup_grid.h"
#include "vpr_api.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_time.h"

/**
 * @brief Attempts to place a molecule into the current virtual cluster.
 *
 * Tries each candidate primitive site from the queue until one succeeds.
 *
 * @return True if the molecule was placed successfully, false otherwise.
 */
static bool can_place_molecule(t_intra_cluster_placement_stats* cluster_stats,
                               PackMoleculeId mol_id,
                               std::vector<t_pb_graph_node*>& primitives_list,
                               const Prepacker& prepacker) {
    LazyPopUniquePriorityQueue<t_pb_graph_node*, std::tuple<float, int, int>>
        candidate_queue = build_primitive_candidate_queue(cluster_stats, mol_id, primitives_list, prepacker);

    while (!candidate_queue.empty()) {
        t_pb_graph_node* root = candidate_queue.pop().first;
        std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
        if (try_start_root_placement(cluster_stats, mol_id, root, primitives_list, prepacker)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Opens a fresh virtual cluster and tries to place the given molecule into it,
 *        iterating over all modes of the logical type (mirroring greedy clusterer).
 *
 * @return Cluster placement stats for the first mode that accepts the molecule,
 *         or nullptr if no mode works. The caller takes ownership and must call
 *         free_cluster_placement_stats() when done.
 */
static t_intra_cluster_placement_stats* open_cluster_for_molecule(t_logical_block_type_ptr logical_type,
                                                                  PackMoleculeId mol_id,
                                                                  std::vector<t_pb_graph_node*>& primitives_list,
                                                                  const Prepacker& prepacker) {
    int num_modes = logical_type->pb_graph_head->pb_type->num_modes;

    // Types with no explicit modes are treated as having a single mode (mode 0).
    int modes_to_try = std::max(1, num_modes);
    for (int mode = 0; mode < modes_to_try; mode++) {
        t_intra_cluster_placement_stats* stats = alloc_and_load_cluster_placement_stats(logical_type, mode);
        std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
        if (can_place_molecule(stats, mol_id, primitives_list, prepacker))
            return stats;
        free_cluster_placement_stats(stats);
    }
    return nullptr;
}

/**
 * @brief Estimates the number of virtual clusters needed for a single logical
 *        block type using a combined pin-capacity and mini-packer simulation.
 *
 * Molecules are packed greedily into virtual clusters. A new cluster is opened
 * when either the pin capacity would be exceeded or the mini-packer cannot fit
 * the molecule into the current cluster. When opening a new cluster, all modes
 * are tried in order (matching greedy clusterer behavior).
 *
 * @param logical_type  The logical block type to simulate packing for; its pb_type
 *                      determines the pin capacities and available primitive sites.
 * @param mol_ids       Molecules to pack, pre-filtered to belong to this logical type.
 * @param prepacker     Used to query molecule structure, primitive placement feasibility,
 *                      and external net counts.
 * @param atom_nlist    Used to compute the external nets each molecule contributes,
 *                      needed for the pin-capacity check.
 * @return              Number of virtual clusters needed to fit all molecules.
 */
static size_t count_clusters_for_type(t_logical_block_type_ptr logical_type,
                                      const std::vector<PackMoleculeId>& mol_ids,
                                      const Prepacker& prepacker,
                                      const AtomNetlist& atom_nlist) {
    const int input_pin_capacity = logical_type->pb_type->num_input_pins;
    const int output_pin_capacity = logical_type->pb_type->num_output_pins;

    std::unordered_set<AtomNetId> cur_input_nets;
    std::unordered_set<AtomNetId> cur_output_nets;
    t_intra_cluster_placement_stats* cluster_stats = nullptr;
    size_t cluster_count = 0;

    for (PackMoleculeId mol_id : mol_ids) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        std::vector<t_pb_graph_node*> primitives_list(mol.atom_block_ids.size(), nullptr);

        // Count new unique external nets this molecule would add.
        // Clock nets are excluded from pin-capacity checks.
        t_molecule_external_nets external_nets = prepacker.calc_molecule_external_nets(mol_id, atom_nlist);
        int new_input_nets = 0, new_output_nets = 0;
        for (AtomNetId net : external_nets.ext_input_nets) {
            if (!cur_input_nets.count(net))
                ++new_input_nets;
        }
        for (AtomNetId net : external_nets.ext_output_nets) {
            if (!cur_output_nets.count(net))
                ++new_output_nets;
        }

        bool pin_overflow = (input_pin_capacity > 0 && static_cast<int>(cur_input_nets.size()) + new_input_nets > input_pin_capacity) || (output_pin_capacity > 0 && static_cast<int>(cur_output_nets.size()) + new_output_nets > output_pin_capacity);

        // Pin capacity exceeded or no cluster open yet, open a new one.
        if (!cluster_stats || (pin_overflow && (!cur_input_nets.empty() || !cur_output_nets.empty()))) {
            if (cluster_stats)
                free_cluster_placement_stats(cluster_stats);
            cluster_stats = open_cluster_for_molecule(logical_type, mol_id, primitives_list, prepacker);
            ++cluster_count;
            cur_input_nets.clear();
            cur_output_nets.clear();
            if (!cluster_stats) {
                // Molecule cannot fit in any mode of a fresh cluster; skip it and count as one cluster.
                continue;
            }
        } else {
            // Try to place in the current cluster; if that fails, open a new one.
            bool placed = can_place_molecule(cluster_stats, mol_id, primitives_list, prepacker);
            if (!placed) {
                free_cluster_placement_stats(cluster_stats);
                cluster_stats = open_cluster_for_molecule(logical_type, mol_id, primitives_list, prepacker);
                ++cluster_count;
                cur_input_nets.clear();
                cur_output_nets.clear();
                if (!cluster_stats) {
                    // Molecule cannot fit in any mode of a fresh cluster; skip it and count as one cluster.
                    continue;
                }
            }
        }

        // Commit the placement and record the nets consumed by this cluster.
        for (t_pb_graph_node* prim : primitives_list) {
            if (prim)
                commit_primitive(cluster_stats, prim);
        }

        cur_input_nets.insert(external_nets.ext_input_nets.begin(), external_nets.ext_input_nets.end());
        cur_output_nets.insert(external_nets.ext_output_nets.begin(), external_nets.ext_output_nets.end());
    }

    if (cluster_stats)
        free_cluster_placement_stats(cluster_stats);

    return cluster_count;
}

std::map<t_logical_block_type_ptr, size_t> DeviceSizeEstimator::estimate_resource_requirement(const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estimate Resource Requirement");
    const auto& atom_ctx = g_vpr_ctx.atom();

    // Group RAM atoms and assign to minimum-area types.
    // Results are stored in ram_groups_ for later use by RamMapper.
    ram_groups_ = group_ram_atoms(atom_ctx.netlist(), prepacker);
    assign_ram_groups_by_min_area(ram_groups_, false /*is_fixed_device*/);

    // Group non-RAM molecules by their logical block type.
    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    std::unordered_map<t_logical_block_type_ptr, std::vector<PackMoleculeId>> logical_type_molecules;
    for (PackMoleculeId mol_id : prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_atom = mol.atom_block_ids[mol.root];
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);

        // Skip RAMs, they are handled separately via logical ram groups.
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive())
            continue;
        if (prim->pb_type->class_type == MEMORY_CLASS)
            continue;

        LogicalModelId root_model_id = atom_ctx.netlist().block_model(root_atom);
        t_logical_block_type_ptr candidate_type = primitive_candidate_block_types[root_model_id][0];
        logical_type_molecules[candidate_type].push_back(mol_id);
    }

    // Estimate instance counts for non-RAM types using pin capacity and cluster placement.
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        num_type_instances[logical_type] = count_clusters_for_type(logical_type, mol_ids, prepacker, atom_ctx.netlist());
    }

    // Estimate instance counts for RAM types using logical RAM groups.
    for (LogicalRamGroupId ram_group_id : ram_groups_.keys()) {
        const LogicalRamGroup& ram_group = ram_groups_[ram_group_id];
        t_logical_block_type_ptr logical_type = ram_group.pre_assigned_type ? ram_group.pre_assigned_type : ram_group.candidate_types[0];
        size_t inferred_number_of_instances = std::ceil(vtr::safe_ratio<float>(ram_group.total_memory_slices, ram_group.candidate_capacity.at(logical_type)));
        num_type_instances[logical_type] += inferred_number_of_instances;
    }

    return num_type_instances;
}

DeviceSizeEstimator::DeviceSizeEstimator(t_vpr_setup& vpr_setup,
                                         const t_arch& arch,
                                         const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estimate Device Size");
    const std::string& device_layout = vpr_setup.PackerOpts.device_layout;
    const t_packer_opts& packer_opts = vpr_setup.PackerOpts;
    bool is_ap = (vpr_setup.APOpts.doAP == e_stage_action::DO);
    auto& device_ctx = g_vpr_ctx.mutable_device();

    // If device size is fixed, create device grid without estimation.
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed to %s.\n", device_layout.c_str());
        std::map<t_logical_block_type_ptr, size_t> num_type_instances;
        device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts,
                                             num_type_instances,
                                             packer_opts.target_device_utilization);
    } else {
        VTR_ASSERT(device_layout == "auto");
        VTR_LOG("Device layout '%s' selected. Need to estimate device size.\n", device_layout.c_str());

        std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker);
        VTR_LOG("Estimated resource requirements:\n");
        for (auto& [type_ptr, count] : num_type_instances)
            VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);

        device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts,
                                             num_type_instances,
                                             packer_opts.target_device_utilization);

        size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
        VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n",
                device_ctx.grid.width(), device_ctx.grid.height(),
                num_grid_tiles, device_ctx.grid.name().c_str());
    }

    // Build the RR graph for AP flow to run global and detailed placement.
    if (is_ap && vpr_setup.PlacerOpts.place_chan_width != NO_FIXED_CHANNEL_WIDTH) {
        vpr_create_rr_graph(vpr_setup, arch, vpr_setup.PlacerOpts.place_chan_width, /*is_flat=*/false);
    }
}
