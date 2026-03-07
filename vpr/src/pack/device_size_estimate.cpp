#include "device_size_estimate.h"

#include "vpr_utils.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "globals.h"   // for g_vpr_ctx
#include "physical_types.h"
#include "setup_grid.h"
#include "cluster_util.h"
#include "vtr_math.h"
#include "cluster_placement.h"

std::map<t_logical_block_type_ptr, size_t> DeviceSizeEstimator::estimate_resource_requirement(const Prepacker& prepacker,
                                                                                              const t_packer_opts& packer_opts,
                                                                                              const LogicalModels& models) const {
    vtr::ScopedStartFinishTimer timer("Estimate Resource Requirement");

    VTR_LOG("num_type_instances is empty; faking demand of 10 instances for each logical block type.\n");

    auto& device_ctx = g_vpr_ctx.mutable_device();

    std::map<t_logical_block_type_ptr, size_t> num_type_instances_capacity;

    const auto& atom_ctx = g_vpr_ctx.atom();

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types = identify_primitive_candidate_block_types();

    //VTR_LOG("Iterating over all atoms once.\n");
    // My naive first implementation idea is to use the first candidate block
    // type always. Another naive part will be assuming that clusters will
    // contain only the same lowest cost primitive. Then summing those numbers
    // at the end. This is of course different not how the packer works but I
    // assume this can give us a good estimation number as a starting point.
    std::map<const t_pb_graph_node*, size_t> primitive_usage_count;
    std::map<const t_pb_graph_node*, t_logical_block_type_ptr> primitive_to_logical_type;
    for (AtomBlockId atom_blk_id: atom_ctx.netlist().blocks()) {
        //VTR_LOG("\tCurrrent atom id is %zu\n", atom_blk_id);
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);
        
        // Skip rams since we will handle them at the very end.
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type == MEMORY_CLASS) continue;

        auto root_model_id = atom_ctx.netlist().block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        
        // TODO: Currrently, I will try to map the first type always. However,
        //       we have multiple candidates in some architectures. So this
        //       should be extended to multiple case and the logic of this loop
        //       should be adapted accordingly.
        // for (t_logical_block_type_ptr candidate_type: candidate_types)
        {
            t_logical_block_type_ptr candidate_type = candidate_types[0];
            std::vector<t_logical_block_type> candidate_logical_type = {*candidate_type};
            const t_pb_graph_node* candidate_prim = prepacker.get_expected_lowest_cost_primitive_for_atom_block(atom_blk_id, candidate_logical_type);
            
            primitive_usage_count[candidate_prim]++;
            primitive_to_logical_type[candidate_prim] = candidate_type;

            int capacity = candidate_prim->total_primitive_count;
            //VTR_LOG("\t\tCandidate type %s can get max of %d of that atom.\n", candidate_type->name.c_str(), capacity);
        }
    }

    // TODO: Here we may want to check the clustering options like pin
    //       utilization or in-cluster utilization. It can be directly
    //       reflected here as an inverse multiplier.
    VTR_LOG("Primitive usage summary by capacity:\n");
    for (auto& [prim, count] : primitive_usage_count) {
        t_logical_block_type_ptr logical_type = primitive_to_logical_type[prim];
        size_t inferred_number = std::ceil(vtr::safe_ratio<float>(count, prim->total_primitive_count));
        VTR_LOG("  Primitive %s[%d] used by %zu atoms. One cluster (%s) can hold %d of it. Inferring %zu clusters.\n",
                prim->pb_type->name,
                prim->placement_index,
                count,
                logical_type->name.c_str(),
                prim->total_primitive_count,
                inferred_number);
        num_type_instances_capacity[logical_type] += inferred_number;
    }

    // Above is for the cluster capacity case. Now, i will try only with the external input pin counts.
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_input_pin;
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_output_pin;
    std::map<t_logical_block_type_ptr, std::vector<PackMoleculeId>> logical_type_molecules;
    for (PackMoleculeId mol_id: prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_atom = mol.atom_block_ids[mol.root];
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);
        
        // Skip rams since we will handle them at the very end.
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type == MEMORY_CLASS) continue;

        auto root_model_id = atom_ctx.netlist().block_model(root_atom);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        // TODO: Currrently, I will try to map the first type always. However,
        //       we have multiple candidates in some architectures. So this
        //       should be extended to multiple case and the logic of this loop
        //       should be adapted accordingly.
        // for (t_logical_block_type_ptr candidate_type: candidate_types)
        {
            t_logical_block_type_ptr candidate_type = candidate_types[0];
            logical_type_molecules[candidate_type].push_back(mol_id);
        }
    }

    // Now doing a pass to infer the logical memories. I will start with the simplest case.
    std::vector<LogicalRamGroup> logical_ram_groups;
    for (AtomBlockId atom_blk_id: atom_ctx.netlist().blocks()) {
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type != MEMORY_CLASS) continue;
        
        auto root_model_id = atom_ctx.netlist().block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        
        // Try to place this atom into an existing group by sibling-feasible equivalence
        bool placed = false;
        for (auto& g : logical_ram_groups) {
            // Quick filter: require same model_id, otherwise the function will definitely fail
            if (g.rep_pb_type->model_id != prim->pb_type->model_id) continue;

            // Key check (as used inside the packer): non-data ports must match bit-for-bit
            // Call in both directions to be extra safe (should be redundant if model_id matches)
            bool ok_ab = primitive_memory_sibling_feasible(atom_blk_id, g.rep_pb_type, g.rep_blk);
            bool ok_ba = primitive_memory_sibling_feasible(g.rep_blk, prim->pb_type, atom_blk_id);

            if (ok_ab && ok_ba && (candidate_types == g.candidate_types)) {
                g.atoms.push_back(atom_blk_id);
                placed = true;
                // float atom_criticality = pre_cluster_timing_manager.calc_atom_setup_criticality(atom_blk_id, atom_nlist);
                // if (atom_criticality > g.max_atom_criticality) {
                //     g.max_atom_criticality = atom_criticality;
                // }
                break;
            }
        }

        if (!placed) {
            // Start a new group with this atom as representative
            LogicalRamGroup ng;
            ng.rep_blk = atom_blk_id;
            ng.rep_pb_type = prim->pb_type;
            ng.atoms.push_back(atom_blk_id);
            ng.candidate_types = candidate_types;
            // float atom_criticality = pre_cluster_timing_manager.calc_atom_setup_criticality(atom_blk_id, atom_nlist);
            // if (atom_criticality > ng.max_atom_criticality) {
            //     ng.max_atom_criticality = atom_criticality;
            // }
            logical_ram_groups.push_back(std::move(ng));
        }
    }
    // Dump logical rams
    for (size_t gid = 0; gid < logical_ram_groups.size(); ++gid) {
        LogicalRamGroup& g = logical_ram_groups[gid];
        g.total_memory_slices = g.atoms.size();
        g.remaining_memory_slices = g.total_memory_slices;
        for (t_logical_block_type_ptr& cand : g.candidate_types) {
            /* Search through all primitives and return the lowest cost primitive that fits this atom block */
            std::vector<t_logical_block_type> candidate_logical_type = {*cand};
            const t_pb_graph_node* candidate_prim = prepacker.get_expected_lowest_cost_primitive_for_atom_block(g.rep_blk, candidate_logical_type);
            g.candidate_capacity[cand] = candidate_prim->total_primitive_count;
        }
    }

    // Inferring by both in&out counts.
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_constrained_pin;
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        int input_pin_number = logical_type->pb_type->num_input_pins;
        int output_pin_number = logical_type->pb_type->num_output_pins;
        int used_input = 0;
        int used_output = 0;
        for (PackMoleculeId mol_id: mol_ids) {
            t_molecule_stats mol_stats = prepacker.calc_molecule_stats(mol_id, atom_ctx.netlist(), models);
            int mol_used_input_pins = mol_stats.num_used_ext_inputs;
            int mol_used_output_pins = mol_stats.num_used_ext_outputs;
            used_input += mol_used_input_pins;
            used_output += mol_used_output_pins;
            if (used_input >= input_pin_number || used_output >= output_pin_number) {
                num_type_instances_by_constrained_pin[logical_type]++;
                used_input = mol_used_input_pins;
                used_output = mol_used_output_pins;
            }
        }
    }
    VTR_LOG("Inferred cluster counts for input and output pins:\n");
    for (auto& [type_ptr, count] : num_type_instances_by_constrained_pin) {
        VTR_LOG("  %s: %zu\n", type_ptr->name.c_str(), count);
    }

    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_unique_constrained_pin;
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        int input_pin_capacity  = logical_type->pb_type->num_input_pins;
        int output_pin_capacity = logical_type->pb_type->num_output_pins;

        // Current cluster’s unique external nets
        std::unordered_set<AtomNetId> cur_input_nets;
        std::unordered_set<AtomNetId> cur_output_nets;

        // Current cluster’s primitive usage (how many times each primitive template is used)
        std::map<const t_pb_graph_node*, int> cur_primitive_usage;

        // Helper: primitive demand of a molecule in this logical_type
        auto compute_molecule_primitive_usage =
            [&](PackMoleculeId mol_id) {
                std::map<const t_pb_graph_node*, int> usage;

                const t_pack_molecule& mol = prepacker.get_molecule(mol_id);

                // We always try mapping this molecule into 'logical_type'
                std::vector<t_logical_block_type> candidate_logical_type = {*logical_type};

                for (AtomBlockId blk : mol.atom_block_ids) {
                    if (!blk) continue;

                    const t_pb_graph_node* prim =
                        prepacker.get_expected_lowest_cost_primitive_for_atom_block(
                            blk, candidate_logical_type);

                    if (!prim) continue;

                    usage[prim]++; // this molecule wants one instance of this primitive
                }

                return usage;
            };

        for (PackMoleculeId mol_id : mol_ids) {
            auto ext = prepacker.calc_molecule_external_nets(
                        mol_id, atom_ctx.netlist());
            // We ignore ext.ext_clock_nets for now

            // Primitive demand of this molecule in this logical_type
            auto mol_prim_usage = compute_molecule_primitive_usage(mol_id);

            // ----------------- Unique-net pin capacity check -----------------
            int new_input_nets  = 0;
            int new_output_nets = 0;

            for (AtomNetId net : ext.ext_input_nets) {
                if (!cur_input_nets.count(net)) {
                    ++new_input_nets;
                }
            }

            for (AtomNetId net : ext.ext_output_nets) {
                if (!cur_output_nets.count(net)) {
                    ++new_output_nets;
                }
            }

            bool overflow_input  = (input_pin_capacity  > 0) &&
                                (static_cast<int>(cur_input_nets.size())  + new_input_nets  > input_pin_capacity);
            bool overflow_output = (output_pin_capacity > 0) &&
                                (static_cast<int>(cur_output_nets.size()) + new_output_nets > output_pin_capacity);

            // ----------------- Primitive capacity check -----------------
            bool capacity_overflow = false;
            for (const auto& [prim, need] : mol_prim_usage) {
                int already_used = cur_primitive_usage[prim];
                int capacity     = prim->total_primitive_count; // per-cluster capacity for this primitive template

                if (already_used + need > capacity) {
                    capacity_overflow = true;
                    break;
                }
            }

            // ----------------- If cannot fit, start a new cluster -----------------
            if (overflow_input || overflow_output || capacity_overflow) {
                if (!cur_input_nets.empty() || !cur_output_nets.empty() ||
                    !cur_primitive_usage.empty()) {
                    num_type_instances_by_unique_constrained_pin[logical_type]++;
                }

                // Reset cluster state
                cur_input_nets.clear();
                cur_output_nets.clear();
                cur_primitive_usage.clear();
            }

            // ----------------- Add this molecule to the current cluster -----------------
            cur_input_nets.insert(ext.ext_input_nets.begin(),  ext.ext_input_nets.end());
            cur_output_nets.insert(ext.ext_output_nets.begin(), ext.ext_output_nets.end());

            for (const auto& [prim, need] : mol_prim_usage) {
                cur_primitive_usage[prim] += need;
            }
        }

        // Count the final open cluster, if any
        if (!cur_input_nets.empty() || !cur_output_nets.empty() || !cur_primitive_usage.empty()) {
            num_type_instances_by_unique_constrained_pin[logical_type]++;
        }
    }

    VTR_LOG("Inferred cluster counts for unique input/output nets with primitive capacity:\n");
    for (auto& [type_ptr, count] : num_type_instances_by_unique_constrained_pin) {
        VTR_LOG("  %s: %zu\n", type_ptr->name.c_str(), count);
    }

    // ---------------------------------------------------------------------------
    // Realistic capacity estimation using mini-packer-style intra-cluster placement
    // ---------------------------------------------------------------------------
    // std::map<t_logical_block_type_ptr, size_t> num_type_instances_realistic_capacity;

    // VTR_LOG("Estimating realistic cluster counts using intra-cluster placement stats (capacity only):\n");

    // for (auto& [logical_type, mol_ids] : logical_type_molecules) {
    //     if (mol_ids.empty()) continue;

    //     VTR_LOG("  Logical type %s:\n", logical_type->name.c_str());

    //     // Pick a cluster mode. Often 0 is the "default" mode at the root.
    //     // If your arch uses a specific root mode, set it here.
    //     int cluster_mode = 0;

    //     t_intra_cluster_placement_stats* cluster_stats = nullptr;
    //     size_t cluster_count = 0;

    //     auto open_new_cluster = [&]() {
    //         if (cluster_stats) {
    //             free_cluster_placement_stats(cluster_stats);
    //         }
    //         cluster_stats = alloc_and_load_cluster_placement_stats(logical_type, cluster_mode);
    //         ++cluster_count;
    //         // VTR_LOG("    Opened new virtual cluster #%zu for type %s\n",
    //         //         cluster_count, logical_type->name.c_str());
    //     };

    //     for (PackMoleculeId mol_id : mol_ids) {
    //         const t_pack_molecule& mol = prepacker.get_molecule(mol_id);

    //         // Lazily open first cluster when we see the first molecule
    //         if (!cluster_stats) {
    //             open_new_cluster();
    //         }

    //         // Prepare primitives list sized to the molecule’s atoms
    //         std::vector<t_pb_graph_node*> primitives_list(mol.atom_block_ids.size(), nullptr);

    //         // Try to pack this molecule into the current virtual cluster
    //         bool placed = get_next_primitive_list(
    //             cluster_stats,
    //             mol_id,
    //             primitives_list,
    //             prepacker,
    //             /*force_site=*/-1);

    //         if (!placed) {
    //             // Could not fit in current cluster: open a new one and retry once
    //             // VTR_LOG("    Molecule %zu did not fit in current cluster, "
    //             //         "opening a new cluster and retrying.\n",
    //             //         size_t(mol_id));

    //             open_new_cluster();

    //             std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
    //             placed = get_next_primitive_list(
    //                 cluster_stats,
    //                 mol_id,
    //                 primitives_list,
    //                 prepacker,
    //                 /*force_site=*/-1);

    //             if (!placed) {
    //                 // Even a fresh cluster cannot host this molecule – log and skip
    //                 // VTR_LOG_WARN(
    //                 //     "    WARNING: mini-packer could not place molecule %zu "
    //                 //     "for logical type %s even in a fresh cluster; skipping.\n",
    //                 //     size_t(mol_id), logical_type->name.c_str());
    //                 continue;
    //             }
    //         }

    //         // We got a valid primitive list – commit them to this virtual cluster
    //         for (t_pb_graph_node* prim : primitives_list) {
    //             if (!prim) continue;
    //             commit_primitive(cluster_stats, prim);
    //         }
    //     }

    //     if (cluster_stats) {
    //         free_cluster_placement_stats(cluster_stats);
    //         cluster_stats = nullptr;
    //     }

    //     num_type_instances_realistic_capacity[logical_type] = cluster_count;
    //     VTR_LOG("    => Estimated realistic capacity: %zu clusters of type %s\n",
    //             cluster_count, logical_type->name.c_str());
    // }
    // VTR_LOG("Inferred cluster counts for realistic capacity clustering:\n");
    // for (auto& [type_ptr, count] : num_type_instances_realistic_capacity) {
    //     VTR_LOG("  %s: %zu\n", type_ptr->name.c_str(), count);
    // }

    // ---------------------------------------------------------------------------
    // Combined unique-net pin + realistic capacity estimation
    // ---------------------------------------------------------------------------
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_unique_constrained_pin_and_realistic_capacity;

    VTR_LOG("Estimating cluster counts using unique external nets + intra-cluster capacity:\n");

    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        if (mol_ids.empty()) continue;

        VTR_LOG("  Logical type %s:\n", logical_type->name.c_str());

        int input_pin_capacity  = logical_type->pb_type->num_input_pins;
        int output_pin_capacity = logical_type->pb_type->num_output_pins;

        // Current cluster’s unique external nets
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

            // VTR_LOG("    Opened new virtual cluster #%zu for type %s\n",
            //         cluster_count, logical_type->name.c_str());
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

        num_type_instances_by_unique_constrained_pin_and_realistic_capacity[logical_type] = cluster_count;
    }

    VTR_LOG("Inferred cluster counts for unique nets + realistic capacity:\n");
    for (auto& [type_ptr, count] : num_type_instances_by_unique_constrained_pin_and_realistic_capacity) {
        VTR_LOG("  %s: %zu\n", type_ptr->name.c_str(), count);
    }


    VTR_LOG("Inferring cluster counts by used external input pin numbers:\n");
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        int input_pin_number_of_type = logical_type->pb_type->num_input_pins;
        int used_inputs = 0;
        for (PackMoleculeId mol_id: mol_ids) {
            int num_ext_inputs = prepacker.calc_molecule_stats(mol_id, atom_ctx.netlist(), models).num_used_ext_inputs;
            used_inputs += num_ext_inputs;
            if (used_inputs == input_pin_number_of_type) {
                num_type_instances_by_input_pin[logical_type] ++;
                used_inputs = 0;
            } else if (used_inputs > input_pin_number_of_type) {
                num_type_instances_by_input_pin[logical_type] ++;
                used_inputs = num_ext_inputs;
            }
        }
        if (used_inputs > 0) {
            num_type_instances_by_input_pin[logical_type] ++;
        }
        int output_pin_number_of_type = logical_type->pb_type->num_output_pins;
        int used_outputs = 0;
        for (PackMoleculeId mol_id: mol_ids) {
            int num_ext_outputs = prepacker.calc_molecule_stats(mol_id, atom_ctx.netlist(), models).num_used_ext_outputs;
            used_outputs += num_ext_outputs;
            if (used_outputs == output_pin_number_of_type) {
                num_type_instances_by_output_pin[logical_type] ++;
                used_outputs = 0;
            } else if (used_outputs > output_pin_number_of_type) {
                num_type_instances_by_output_pin[logical_type] ++;
                used_outputs = num_ext_outputs;
            }
        }
        if (used_outputs > 0) {
            num_type_instances_by_output_pin[logical_type] ++;
        }
    }

    std::map<t_logical_block_type_ptr, size_t> num_type_instances_by_avg_pin;
    VTR_LOG("Logic type usage summary by external pins:\n");
    for (auto& [logical_type, inferred_num]: num_type_instances_by_input_pin) {
        VTR_LOG("  %s (input):     %zu\n", logical_type->name.c_str(), inferred_num);
        VTR_LOG("  %s (output):    %zu\n", logical_type->name.c_str(), num_type_instances_by_output_pin[logical_type]);
        size_t avg_number = std::ceil(vtr::safe_ratio<float>(num_type_instances_by_input_pin[logical_type] + num_type_instances_by_output_pin[logical_type], 2));
        num_type_instances_by_avg_pin[logical_type] = avg_number;
        VTR_LOG("    %s (average): %zu\n", logical_type->name.c_str(), avg_number);
    }

    // For the io constrained devices, i found the number pin number constraint is too loose.
    // So i will be maxing the average pin number and capacity above.
    VTR_LOG("Taking max of capacity and pin number:\n");
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_pin_and_capacity;
    for (auto& [logical_type, inferred_num]: num_type_instances_by_avg_pin) {
        VTR_LOG("  %s (pin):          %zu\n", logical_type->name.c_str(), inferred_num);
        VTR_LOG("  %s (capacity):     %zu\n", logical_type->name.c_str(), num_type_instances_capacity[logical_type]);
        size_t max_num = std::max(num_type_instances_by_avg_pin[logical_type], num_type_instances_capacity[logical_type]);
        num_type_instances_pin_and_capacity[logical_type] = max_num;
        VTR_LOG("    %s (max):        %zu\n", logical_type->name.c_str(), max_num);
    }

    // Note: check the resutls of neuron. Above formulation works perfectly for VTR benchmarks but struggle in Titan.
    //       we can do a new thing like: use the capacity only for RAMs and IOs. Then average of pin and capacity for others. Check that or other combinations.
    //
    // Note: For the io and memories (especially for the io), the capacity works very well.
    //       For LABs, the average pin seems to be the best but averaging or takin max of avg. pin and capacity can also be not bad.
    //         - most probably max of avg. pin and capacity would be better.
    //       For DSPs, they are the problem here. Sometimes so close to the capacity and sometimes to the pin number. If we figure it out, we can get a better estimate for StratixIV arch as well.
    //       For RAMs (memories), we can fast infer the logical RAMs and map them to the first available candidate always [for more accurate estimation, consistency and not to constrain device too much].
    VTR_LOG("Using capacity for io and memory blocks, pin count for other:\n");
    std::map<t_logical_block_type_ptr, size_t> num_type_instances_pin_or_capacity;
    for (auto& [logical_type, inferred_num]: num_type_instances_by_avg_pin) {
        size_t assigned_num = 0;
        // if (logical_type->is_io() || logical_type->pb_type->class_type == e_pb_type_class::MEMORY_CLASS) {
        if (logical_type->is_io()) {
            assigned_num = num_type_instances_by_unique_constrained_pin_and_realistic_capacity[logical_type];
        } else {
            // assigned_num = std::ceil(vtr::safe_ratio<float>(num_type_instances_by_avg_pin[logical_type] + num_type_instances_capacity[logical_type], 2));
            // assigned_num = num_type_instances_by_avg_pin[logical_type];
            // assigned_num = num_type_instances_capacity[logical_type];
            // assigned_num = std::max(num_type_instances_by_avg_pin[logical_type], num_type_instances_capacity[logical_type]);
            // assigned_num = num_type_instances_by_constrained_pin[logical_type];
            assigned_num = num_type_instances_by_unique_constrained_pin_and_realistic_capacity[logical_type];
            // assigned_num = std::max(num_type_instances_by_unique_constrained_pin[logical_type], num_type_instances_capacity[logical_type]);
        }
        num_type_instances_pin_or_capacity[logical_type] = assigned_num;
        VTR_LOG("  %s (assigned):   %zu\n", logical_type->name.c_str(), assigned_num);
    }

    // Count inferred numbers for the logical rams.
    // Note: Currently assigning to the first available type. This is the simplest
    //       solution to try and get an initial result.
    for (auto& g : logical_ram_groups) {
        t_logical_block_type_ptr candidate_type = g.candidate_types[0];
        size_t inferred_number = std::ceil(vtr::safe_ratio<float>(g.total_memory_slices, g.candidate_capacity[candidate_type]));
        num_type_instances_pin_or_capacity[candidate_type] += inferred_number;
    }
    
    
    return num_type_instances_pin_or_capacity;
}

DeviceSizeEstimator::DeviceSizeEstimator(const std::string& device_layout,
                                         const t_packer_opts& packer_opts,
                                         const std::vector<t_grid_def>& grid_layouts,
                                         const LogicalModels& models,
                                         const Prepacker& prepacker) {
    vtr::ScopedStartFinishTimer timer("Estime Device Size");
    auto& device_ctx = g_vpr_ctx.mutable_device();
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed (%s), no need to estimate.\n", device_layout.c_str());
        std::map<t_logical_block_type_ptr, size_t> num_type_instances;
        device_ctx.grid = create_device_grid(device_layout, grid_layouts, num_type_instances, packer_opts.target_device_utilization);
        return;
    }

    VTR_LOG("Device is not fixed, need to estimate the resource requirement.\n");
    std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker, packer_opts, models);
    
    VTR_LOG("Estimated resoure requirement:\n");
    for (auto& [type_ptr, count] : num_type_instances) {
        VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);
    }

    // Not sure about do we always want to cast this to global.
    VTR_LOG("Cast the update to global device\n");
    device_ctx.grid = create_device_grid(device_layout, grid_layouts, num_type_instances, packer_opts.target_device_utilization);

    // Report the updated device.
    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n", device_ctx.grid.width(), device_ctx.grid.height(), num_grid_tiles, device_ctx.grid.name().c_str());
}