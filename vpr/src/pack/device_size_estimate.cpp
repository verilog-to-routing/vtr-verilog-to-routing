/**
 * @file
 * @author  Haydar Cakan
 * @date    March 12, 2026
 * @brief   Implementation of device size estimation before packing.
 *
 * Non-RAM molecules are grouped by logical block type and packed into virtual
 * clusters using a lightweight pin-capacity and mini-packer simulation to
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
 * @brief Estimates the number of virtual clusters needed for a single logical
 *        block type using a combined pin-capacity and mini-packer simulation.
 *
 * Molecules are packed greedily into virtual clusters. A new cluster is opened
 * when either the pin capacity would be exceeded or the mini-packer cannot fit
 * the molecule into the current cluster.
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
    const int input_pin_capacity  = logical_type->pb_type->num_input_pins;
    const int output_pin_capacity = logical_type->pb_type->num_output_pins;
    const int cluster_mode = 0;

    std::unordered_set<AtomNetId> cur_input_nets;
    std::unordered_set<AtomNetId> cur_output_nets;
    t_intra_cluster_placement_stats* cluster_stats = nullptr;
    size_t cluster_count = 0;

    for (PackMoleculeId mol_id : mol_ids) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);

        // Open the first cluster on the initial iteration.
        if (!cluster_stats) {
            cluster_stats = alloc_and_load_cluster_placement_stats(logical_type, cluster_mode);
            ++cluster_count;
        }

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

        bool pin_overflow = (input_pin_capacity > 0 && static_cast<int>(cur_input_nets.size()) + new_input_nets > input_pin_capacity) ||
                            (output_pin_capacity > 0 && static_cast<int>(cur_output_nets.size()) + new_output_nets > output_pin_capacity);

        // Pin capacity exceeded, close the current cluster and start a fresh one.
        if (pin_overflow && (!cur_input_nets.empty() || !cur_output_nets.empty())) {
            free_cluster_placement_stats(cluster_stats);
            cluster_stats = alloc_and_load_cluster_placement_stats(logical_type, cluster_mode);
            ++cluster_count;
            cur_input_nets.clear();
            cur_output_nets.clear();
        }

        // Try to place in the current cluster; if that fails, open a new one and retry.
        std::vector<t_pb_graph_node*> primitives_list(mol.atom_block_ids.size(), nullptr);
        bool placed = can_place_molecule(cluster_stats, mol_id, primitives_list, prepacker);
        if (!placed) {
            // Cluster placement rejected the molecule, start a new cluster and try once more.
            free_cluster_placement_stats(cluster_stats);
            cluster_stats = alloc_and_load_cluster_placement_stats(logical_type, cluster_mode);
            ++cluster_count;
            cur_input_nets.clear();
            cur_output_nets.clear();
            std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
            placed = can_place_molecule(cluster_stats, mol_id, primitives_list, prepacker);
            if (!placed) {
                // Molecule cannot fit any fresh cluster; skip it in the estimate.
                continue;
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
    assign_minimum_area(ram_groups_);

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

DeviceSizeEstimator::DeviceSizeEstimator(const std::string& device_layout,
                                         const t_packer_opts& packer_opts,
                                         const std::vector<t_grid_def>& grid_layouts,
                                         const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estimate Device Size");
    auto& device_ctx = g_vpr_ctx.mutable_device();

    // If device size is fixed, create device grid without estimation.
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed to %s.\n", device_layout.c_str());
        std::map<t_logical_block_type_ptr, size_t> num_type_instances;
        device_ctx.grid = create_device_grid(device_layout, grid_layouts,
                                             num_type_instances,
                                             packer_opts.target_device_utilization);
        return;
    }

    VTR_ASSERT(device_layout == "auto");
    VTR_LOG("Device layout '%s' selected. Need to estimate device size.\n");

    std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker);
    VTR_LOG("Estimated resource requirements:\n");
    for (auto& [type_ptr, count] : num_type_instances)
        VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);

    device_ctx.grid = create_device_grid(device_layout, grid_layouts,
                                         num_type_instances,
                                         packer_opts.target_device_utilization);

    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n",
            device_ctx.grid.width(), device_ctx.grid.height(),
            num_grid_tiles, device_ctx.grid.name().c_str());
}
