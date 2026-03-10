#include "device_size_estimate.h"

#include "vpr_utils.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "globals.h"
#include "physical_types.h"
#include "setup_grid.h"
#include "cluster_util.h"
#include "vtr_math.h"
#include "cluster_placement.h"

std::map<t_logical_block_type_ptr, size_t> DeviceSizeEstimator::estimate_resource_requirement(
    const Prepacker& prepacker) const {
    vtr::ScopedStartFinishTimer timer("Estimate Resource Requirement");

    const auto& atom_ctx = g_vpr_ctx.atom();

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types = identify_primitive_candidate_block_types();

    std::map<t_logical_block_type_ptr, std::vector<PackMoleculeId>> logical_type_molecules;
    for (PackMoleculeId mol_id: prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_atom = mol.atom_block_ids[mol.root];
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);

        // Skip RAMs — they are handled separately via ram_groups_.
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type == MEMORY_CLASS) continue;

        auto root_model_id = atom_ctx.netlist().block_model(root_atom);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        t_logical_block_type_ptr candidate_type = candidate_types[0];
        logical_type_molecules[candidate_type].push_back(mol_id);
    }

    // ---------------------------------------------------------------------------
    // Combined unique-net pin + realistic capacity estimation for non-RAM types
    // ---------------------------------------------------------------------------
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;

    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        if (mol_ids.empty()) continue;

        int input_pin_capacity  = logical_type->pb_type->num_input_pins;
        int output_pin_capacity = logical_type->pb_type->num_output_pins;

        // Current cluster's unique external nets
        std::unordered_set<AtomNetId> cur_input_nets;
        std::unordered_set<AtomNetId> cur_output_nets;

        // Mini-packer state for this logical type
        int cluster_mode = 0; // root mode; adjust if your arch needs another
        t_intra_cluster_placement_stats* cluster_stats = nullptr;
        size_t cluster_count = 0;

        auto open_new_cluster = [&]() {
            if (cluster_stats) {
                free_cluster_placement_stats(cluster_stats);
            }
            cluster_stats = alloc_and_load_cluster_placement_stats(logical_type, cluster_mode);
            ++cluster_count;
            cur_input_nets.clear();
            cur_output_nets.clear();
        };

        for (PackMoleculeId mol_id : mol_ids) {
            const t_pack_molecule& mol = prepacker.get_molecule(mol_id);

            // Lazily open first cluster
            if (!cluster_stats) {
                open_new_cluster();
            }

            // ----------------- Unique-net pin capacity part -----------------
            auto ext = prepacker.calc_molecule_external_nets(
                mol_id, atom_ctx.netlist());
            // ignoring ext.ext_clock_nets for now

            int new_input_nets  = 0;
            int new_output_nets = 0;

            for (AtomNetId net : ext.ext_input_nets) {
                if (!cur_input_nets.count(net)) ++new_input_nets;
            }
            for (AtomNetId net : ext.ext_output_nets) {
                if (!cur_output_nets.count(net)) ++new_output_nets;
            }

            bool overflow_input  = (input_pin_capacity  > 0)
                                && (static_cast<int>(cur_input_nets.size())  + new_input_nets  > input_pin_capacity);
            bool overflow_output = (output_pin_capacity > 0)
                                && (static_cast<int>(cur_output_nets.size()) + new_output_nets > output_pin_capacity);

            bool pin_overflow = overflow_input || overflow_output;

            // If we would overflow pins and this cluster is already used, move to a new one
            if (pin_overflow &&
                (!cur_input_nets.empty() || !cur_output_nets.empty())) {
                open_new_cluster();
            }

            // ----------------- Capacity part via mini-packer -----------------
            std::vector<t_pb_graph_node*> primitives_list(mol.atom_block_ids.size(), nullptr);

            // First try: place in current cluster
            bool placed = false;
            {
                auto candidate_queue = build_primitive_candidate_queue(
                    cluster_stats,
                    mol_id,
                    primitives_list,
                    prepacker,
                    /*force_site=*/-1);

                while (!candidate_queue.empty()) {
                    t_pb_graph_node* root = candidate_queue.pop().first;
                    std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
                    if (try_start_root_placement(cluster_stats,
                                                mol_id,
                                                root,
                                                primitives_list,
                                                prepacker)) {
                        placed = true;
                        break;
                    }
                }
            }

            if (!placed) {
                // Could not fit in current cluster. Open a new one and retry once.
                open_new_cluster();
                std::fill(primitives_list.begin(), primitives_list.end(), nullptr);

                auto candidate_queue = build_primitive_candidate_queue(
                    cluster_stats,
                    mol_id,
                    primitives_list,
                    prepacker,
                    /*force_site=*/-1);

                while (!candidate_queue.empty()) {
                    t_pb_graph_node* root = candidate_queue.pop().first;
                    std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
                    if (try_start_root_placement(cluster_stats,
                                                mol_id,
                                                root,
                                                primitives_list,
                                                prepacker)) {
                        placed = true;
                        break;
                    }
                }

                if (!placed) {
                    continue;
                }
            }

            // Commit the primitives for this molecule into the current virtual cluster
            for (t_pb_graph_node* prim : primitives_list) {
                if (!prim) continue;
                commit_primitive(cluster_stats, prim);
            }

            // ----------------- Update unique nets for this cluster -----------------
            cur_input_nets.insert(ext.ext_input_nets.begin(),  ext.ext_input_nets.end());
            cur_output_nets.insert(ext.ext_output_nets.begin(), ext.ext_output_nets.end());
        }

        if (cluster_stats) {
            free_cluster_placement_stats(cluster_stats);
            cluster_stats = nullptr;
        }

        num_type_instances[logical_type] = cluster_count;
    }

    // Add instance counts inferred from the minimum-area RAM assignment.
    // Note: Currently assigning to the first available type. This is the simplest
    //       solution to try and get an initial result.
    for (LogicalRamGroupId gid : ram_groups_.keys()) {
        const LogicalRamGroup& g = ram_groups_[gid];
        t_logical_block_type_ptr candidate_type = g.pre_assigned_type
                                                  ? g.pre_assigned_type
                                                  : g.candidate_types[0];
        size_t inferred_number = std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices, g.candidate_capacity.at(candidate_type)));
        num_type_instances[candidate_type] += inferred_number;
    }

    return num_type_instances;
}

DeviceSizeEstimator::DeviceSizeEstimator(const std::string& device_layout,
                                         const t_packer_opts& packer_opts,
                                         const std::vector<t_grid_def>& grid_layouts,
                                         const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estimate Device Size");
    auto& device_ctx = g_vpr_ctx.mutable_device();
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed (%s), no need to estimate.\n", device_layout.c_str());
        std::map<t_logical_block_type_ptr, size_t> num_type_instances;
        device_ctx.grid = create_device_grid(device_layout, grid_layouts, num_type_instances, packer_opts.target_device_utilization);
        return;
    }

    VTR_LOG("Device is not fixed, need to estimate the resource requirement.\n");

    // Group RAM atoms and assign to minimum-area types. The resulting groups
    // are stored and later passed to RamMapper to avoid redundant computation.
    ram_groups_ = group_ram_atoms(g_vpr_ctx.atom().netlist(), prepacker);
    assign_minimum_area(ram_groups_);

    std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker);

    VTR_LOG("Estimated resource requirement:\n");
    for (auto& [type_ptr, count] : num_type_instances) {
        VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);
    }

    device_ctx.grid = create_device_grid(device_layout, grid_layouts, num_type_instances, packer_opts.target_device_utilization);

    // Report the updated device.
    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n", device_ctx.grid.width(), device_ctx.grid.height(), num_grid_tiles, device_ctx.grid.name().c_str());
}
