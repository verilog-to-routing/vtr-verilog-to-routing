

#include "logical_ram_infer.h"

#include "vtr_time.h"
#include "cluster_util.h"
#include "globals.h"
#include "vtr_math.h"

RamMapper::RamMapper(const AtomNetlist& atom_nlist,
    Prepacker& prepacker,
    const LogicalModels& models,
    const std::vector<t_logical_block_type>& logical_block_types,
    const PreClusterTimingManager pre_cluster_timing_manager) {

    
    vtr::ScopedStartFinishTimer prepacker_timer("Ram Mapper");


    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    VTR_LOG("Traversing molecules for RAM slices (sibling-feasible grouping):\n");

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>> primitive_candidate_block_types = identify_primitive_candidate_block_types();

    auto& device_ctx = g_vpr_ctx.device();

    std::unordered_map<t_logical_block_type_ptr, int> candidate_usages;

    for (AtomBlockId atom_blk_id: atom_nlist.blocks()) {
        // const t_pack_molecule& mol = get_molecule(mol_id);
        // AtomBlockId atom_blk_id = mol.atom_block_ids[mol.root];
        if (!atom_blk_id.is_valid()) continue;

        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type != MEMORY_CLASS) continue;

        auto root_model_id = atom_nlist.block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];

        // Try to place this atom into an existing group by sibling-feasible equivalence
        bool placed = false;
        for (auto& g : logical_ram_groups_) {
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
            logical_ram_groups_.push_back(std::move(ng));

            // Add current types if they not exist in the mapping.
            for (t_logical_block_type_ptr candidate_type: candidate_types) {
                if (candidate_usages.find(candidate_type) == candidate_usages.end()) {
                    candidate_usages[candidate_type] = 0;
                }
            }
        }
    }
    
    // Dump groups/
    VTR_LOG("\nInferred logical RAM groups (sibling-feasible):\n");
    for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
        LogicalRamGroup& g = logical_ram_groups_[gid];
        g.total_memory_slices = g.atoms.size();
        g.remaining_memory_slices = g.total_memory_slices;

        VTR_LOG("  Group %zu: %zu slice(s)\n", gid, g.atoms.size());

        // Hierarchical name (from representative)
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(g.rep_blk);
        if (prim) {
            VTR_LOG("    Representative atom id: %zu\n", g.rep_blk);
            VTR_LOG("    Hierarchical name: %s\n", prim->hierarchical_type_name().c_str());
            VTR_LOG("    Blif model: %s\n", prim->pb_type->blif_model ? prim->pb_type->blif_model : "");
            std::string blif_model_name = prim->pb_type->blif_model;
            bool output_registered = false;
            if (blif_model_name.find("output_type{reg}") != std::string::npos) {
                output_registered = true;
            }
            g.is_output_registered = output_registered;
            VTR_LOG("    Output registered: %s\n", output_registered ? "Yes" : "No");
            
        }

        auto root_model_id = atom_nlist.block_model(g.rep_blk);
        // std::vector<t_logical_block_type_ptr> candidate_types = primitive_candidate_block_types[root_model_id];
        VTR_LOG("    Candidate types & per-instance capacity for this slice:\n");
        for (t_logical_block_type_ptr& cand : g.candidate_types) {
            /* Search through all primitives and return the lowest cost primitive that fits this atom block */
            std::vector<t_logical_block_type> candidate_logical_type = {*cand};
            const t_pb_graph_node* candidate_prim = prepacker.get_expected_lowest_cost_primitive_for_atom_block(g.rep_blk, candidate_logical_type);
            g.candidate_capacity[cand] = candidate_prim->total_primitive_count;
            // g.candidate_types.push_back(cand);
            int equivalent_num_instances = 0;
            for (auto type : cand->equivalent_tiles) {
                equivalent_num_instances += device_ctx.grid.num_instances(type, -1);
            }
            VTR_LOG("        %s: %d (Total Instances: %d)\n", cand->name.c_str(), candidate_prim->total_primitive_count, equivalent_num_instances);
        }

        // Assign the first candidate in the arch as a greedy initial assignment. 
        // This will give an initial assignment using the first type in the architecture as packer
        // chooses types for other blocks.
        t_logical_block_type_ptr first_candidate = g.candidate_types[0];
        g.pre_assigned_type = first_candidate;
        g.type_init = first_candidate;
        
        // Assign the initial cost (only area for now)
        int inferred_instance_num = std::ceil(vtr::safe_ratio<float>(g.total_memory_slices, g.candidate_capacity[first_candidate]));
        int instance_area = first_candidate->equivalent_tiles[0]->height * first_candidate->equivalent_tiles[0]->width;
        g.area_init = inferred_instance_num * instance_area;
        VTR_LOG("    Initial assignment: (Type: %s) (Inferred instances: %d) (Initial area: %d)\n", first_candidate->name.c_str(), inferred_instance_num, g.area_init);
        candidate_usages[first_candidate] += inferred_instance_num;
    }

    // Crosscheck and validate the atoms are assigned to correct groups.
    // This should be at the end of logical ram assignemts after any sorting since
    // helper methods use the order in this data structure afterwards.
    atom_to_group_.resize(atom_nlist.blocks().size());
    for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
        auto& g = logical_ram_groups_[gid];
        for (AtomBlockId atom_id: g.atoms) {
            atom_to_group_[atom_id] = gid;
        }
    }

    // Verify the initial mappings are feasible (utilization check).
    VTR_LOG("Initial mappings device utilizations:\n");
    for (auto usage: candidate_usages) {
        t_logical_block_type_ptr type = usage.first;
        int used_ins_number = usage.second;
        int total_ins_number = 0;
        for (auto physical_type : type->equivalent_tiles)
            total_ins_number += device_ctx.grid.num_instances(physical_type, -1);
        float utilization = vtr::safe_ratio<float>(used_ins_number, total_ins_number);
        VTR_LOG("  %s: %f\n", type->name.c_str(), utilization);

        VTR_ASSERT_MSG(utilization <= 1.0, "At least one RAM type is over utilized after initial placement. Ideally, this assertion should be removed and reassigning should be tried");
    }


    // Timing pass to lock the timing critical ones to faster RAMs.
    VTR_LOG("Starting timing pass for Logical RAMs\n");
    // auto& logical_rams = logical_ram_groups_;
    AtomBlockId max_crit_logical_ram_rep_atom;
    float overall_max_atom_criticality = 0.0;
    for (LogicalRamGroup& logical_ram: logical_ram_groups_) {
        float max_atom_criticality = 0.0;
        for (AtomBlockId atom_blk_id: logical_ram.atoms) {
            float current_atom_criticality = pre_cluster_timing_manager.calc_atom_setup_criticality(atom_blk_id, g_vpr_ctx.atom().netlist());
            if (current_atom_criticality > max_atom_criticality) {
                max_atom_criticality = current_atom_criticality;
            }
        }
        logical_ram.max_atom_criticality = max_atom_criticality;
        VTR_LOG("\tMax Atom Crit. of logical ram with representative atom id %zu is %f\n", logical_ram.rep_blk, max_atom_criticality);
        if (max_atom_criticality > overall_max_atom_criticality) {
            overall_max_atom_criticality = max_atom_criticality;
            max_crit_logical_ram_rep_atom = logical_ram.rep_blk;
        }
    }
    std::vector<AtomBlockId> max_crit_logical_ram_rep_atoms;
    const float EPS = 1e-6f;
    for (LogicalRamGroup& logical_ram: logical_ram_groups_) {
        if (std::fabs(overall_max_atom_criticality - logical_ram.max_atom_criticality) < EPS) {
            max_crit_logical_ram_rep_atoms.push_back(logical_ram.rep_blk);
        }
    }
    VTR_LOG("The max atom criticality is %f with representative atom id %zu from logical ram group id %zu. There are total of %zu groups with same max criticality.\n", 
            overall_max_atom_criticality, max_crit_logical_ram_rep_atom, group_id_of(max_crit_logical_ram_rep_atom), max_crit_logical_ram_rep_atoms.size());

    VTR_LOG("Trying to remap these atoms to the smaller RAMs (believed to be faster)\n");
    // std::unordered_map<t_logical_block_type_ptr, int> candidate_usages = get_final_candidate_usages();

    std::unordered_set<AtomBlockId> timinig_locked_logical_ram_representative_atoms;

    for (AtomBlockId atom_blk_id: max_crit_logical_ram_rep_atoms) {
        size_t group_id =  group_id_of(atom_blk_id);
        LogicalRamGroup& logical_ram = logical_ram_groups_[group_id];
        VTR_LOG("\tPre-assigned type of timing critical group %zu is %s\n", group_id, logical_ram.pre_assigned_type->name.c_str());
        
        // TODO: Skip this mappings if the output is registered.
        if (logical_ram.is_output_registered) {
            VTR_LOG("\t\tSkipped since output is registered.\n");
            continue;
        }
        
        t_logical_block_type_ptr assigned_type = logical_ram.pre_assigned_type;
        int min_capacity = INT_MAX;
        t_logical_block_type_ptr min_capacity_type = nullptr;
        for (const auto& [cand_type, capacity]: logical_ram.candidate_capacity) {
            if (capacity < min_capacity) {
                min_capacity = capacity;
                min_capacity_type = cand_type;
            }
        }
        if (min_capacity_type == assigned_type) {
            VTR_LOG("\t\tAssigned type and min capacity type is same. No need to move to min capacity type.\n");
            timinig_locked_logical_ram_representative_atoms.insert(logical_ram.rep_blk);
            continue;
        }

        int min_capacity_inferred_number = std::ceil(vtr::safe_ratio<float>(logical_ram.total_memory_slices, logical_ram.candidate_capacity[min_capacity_type]));
        int min_capacity_equivalent_num_instances = 0;
        for (auto type : min_capacity_type->equivalent_tiles) {
            min_capacity_equivalent_num_instances += g_vpr_ctx.device().grid.num_instances(type, -1);
        }
        int min_capacity_type_new_usage = min_capacity_inferred_number + candidate_usages[min_capacity_type];
        VTR_LOG("\t\tAfter mapping, min capacity type utilization used and total: (%d/%d)\n", min_capacity_type_new_usage, min_capacity_equivalent_num_instances);
        if (min_capacity_type_new_usage > min_capacity_equivalent_num_instances) {
            VTR_LOG("\t\tRemapping rejected due to utilization going above 1.0.\n");
            continue;
        }

        // Remapping accepted at this point.
        timinig_locked_logical_ram_representative_atoms.insert(logical_ram.rep_blk);

        candidate_usages[min_capacity_type] = min_capacity_type_new_usage;
        int pre_assigned_capacity_inferred_number = std::ceil(vtr::safe_ratio<float>(logical_ram.total_memory_slices, logical_ram.candidate_capacity[assigned_type]));
        int pre_assigned_capacity_equivalent_num_instances = 0;
        for (auto type : min_capacity_type->equivalent_tiles) {
            pre_assigned_capacity_equivalent_num_instances += g_vpr_ctx.device().grid.num_instances(type, -1);
        }
        int pre_assigned_capacity_type_new_usage = candidate_usages[assigned_type] - pre_assigned_capacity_inferred_number;
        candidate_usages[assigned_type] = pre_assigned_capacity_type_new_usage;
        // Update the logical ram as well.
        logical_ram.pre_assigned_type = min_capacity_type;
    }

    // Verify the initial mappings are feasible (utilization check).
    VTR_LOG("After timing mappings device utilizations:\n");
    for (auto usage: candidate_usages) {
        t_logical_block_type_ptr type = usage.first;
        int used_ins_number = usage.second;
        int total_ins_number = 0;
        for (auto physical_type : type->equivalent_tiles)
            total_ins_number += device_ctx.grid.num_instances(physical_type, -1);
        float utilization = vtr::safe_ratio<float>(used_ins_number, total_ins_number);
        VTR_LOG("  %s: %f\n", type->name.c_str(), utilization);

        VTR_ASSERT_MSG(utilization <= 1.0, "At least one RAM type is over utilized after final placement. Ideally, this assertion should be removed and reassigning should be tried");
    }
    candidate_usages_final_ = candidate_usages;

    // Calculate cost of eahc possible implementation for each logical ram and store for later evaluation.
    VTR_LOG("Calculating each possible implementation costs for each logical RAM.");
    for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
        LogicalRamGroup& g = logical_ram_groups_[gid];
        VTR_LOG("  Group %zu: %zu slice(s)\n", gid, g.atoms.size());
        for (t_logical_block_type_ptr candidate_type: g.candidate_types) {
            int inferred_instance_num = std::ceil(vtr::safe_ratio<float>(g.total_memory_slices, g.candidate_capacity[candidate_type]));
            int instance_area = candidate_type->equivalent_tiles[0]->height * candidate_type->equivalent_tiles[0]->width;
            int type_area = inferred_instance_num * instance_area;
            g.area_type[candidate_type] = type_area;
            VTR_LOG("    %s Cost(Area): %d\n", candidate_type->name.c_str(), type_area);
        }

        // Sort the candidate types by their cost (area)
        std::sort(g.candidate_types.begin(),
                g.candidate_types.end(),
                [&](const t_logical_block_type_ptr& a,
                    const t_logical_block_type_ptr& b) {
                    // area must have been filled above
                    const int area_a = g.area_type.at(a);
                    const int area_b = g.area_type.at(b);
                    if (area_a != area_b) return area_a < area_b;

                    // tie-break 1: prefer the bigger type (if areas are equal like 8 M9K and 1 M144K, using M144K might be a perfect fit there)
                    // Also experimentally this inclusion lead to better results in a different setup.
                    const int cap_a = g.candidate_capacity.count(a) ? g.candidate_capacity.at(a) : 0;
                    const int cap_b = g.candidate_capacity.count(b) ? g.candidate_capacity.at(b) : 0;
                    if (cap_a != cap_b) return cap_a > cap_b; // higher capacity first.

                    // tie-break 2: deterministic by name
                    return std::string(a->name) < std::string(b->name);
                });

        // Set the Cost of memory to the minimum Cost type.
        t_logical_block_type_ptr min_cost_type = g.candidate_types[0];
        g.area_mem = g.area_type[min_cost_type];
        VTR_LOG("    Min Cost Type %s with Cost (Area) of %d\n", min_cost_type->name.c_str(), g.area_mem);
    }

    // Sort the logical RAM blocks by Cost_init - Cost_mem.
    VTR_LOG("Sorting the logical RAM blocks by Cost_init - Cost_mem.\n");
    // Stable sort to preserve original order for equal differences
    std::stable_sort(logical_ram_groups_.begin(),
                    logical_ram_groups_.end(),
                    [](const LogicalRamGroup& a, const LogicalRamGroup& b) {
                        int diff_a = a.area_init - a.area_mem;
                        int diff_b = b.area_init - b.area_mem;
                        // Larger improvement (higher diff) comes first
                        return diff_a > diff_b;
                    });

    // Log the sorted order
    // for (size_t i = 0; i < logical_ram_groups_.size(); ++i) {
    //     const LogicalRamGroup& g = logical_ram_groups_[i];
    //     int diff = g.area_init - g.area_mem;
    //     VTR_LOG("  Rank %zu: ΔCost = %d  (Init = %d, Mem = %d, Slices = %d)\n",
    //             i, diff, g.area_init, g.area_mem, g.total_memory_slices);
    // }

    // After sorting based on cost gain, select the memory type that has lowest Cost_type
    // and feasible.
    VTR_LOG("Selecting the lowest cost & feasible type for eahc logical RAM.\n");
    for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
        LogicalRamGroup& g = logical_ram_groups_[gid];
        
        // Skip this logical ram if it is locked to a type for timing.
        if (timinig_locked_logical_ram_representative_atoms.find(g.rep_blk) != timinig_locked_logical_ram_representative_atoms.end()) {
            VTR_LOG("\tSkipping the current logical ram with representative atom id %zu since its locked by timing.\n", g.rep_blk);
            continue;
        }
        
        for (t_logical_block_type_ptr candidate_type: g.candidate_types) {
            int inferred_instance_num = std::ceil(vtr::safe_ratio<float>(g.total_memory_slices, g.candidate_capacity[candidate_type]));
            int total_ins_number = 0;
            for (auto physical_type : candidate_type->equivalent_tiles)
                total_ins_number += device_ctx.grid.num_instances(physical_type, -1);
            float new_utilization = vtr::safe_ratio<float>(candidate_usages[candidate_type] + inferred_instance_num, total_ins_number);

            if (new_utilization <= 1.0) {
                // Accept the change and update stats, logical RAM preference.
                candidate_usages[candidate_type] += inferred_instance_num;

                // Decrease the old type numbers from its usage.
                int old_type_instance_num = std::ceil(vtr::safe_ratio<float>(g.total_memory_slices, g.candidate_capacity[g.pre_assigned_type]));
                candidate_usages[g.pre_assigned_type] -= old_type_instance_num;

                // Assign new type.
                g.pre_assigned_type = candidate_type;
                break;
            }
        }
    }

    // // Timing part.
    // VTR_LOG("Max timing criticalities:\n");
    // for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
    //     LogicalRamGroup& g = logical_ram_groups_[gid];
    //     VTR_LOG("\tGroup %zu has %zu atoms:\n", gid, g.total_memory_slices);
    //     VTR_LOG("\t\tMax criticality: %f\n", g.max_atom_criticality);
    //     VTR_LOG("\t\tPre-assigned type: %s\n", g.pre_assigned_type->name.c_str())l
    // }

    // Verify the initial mappings are feasible (utilization check).
    VTR_LOG("After area mappings device utilizations:\n");
    for (auto usage: candidate_usages) {
        t_logical_block_type_ptr type = usage.first;
        int used_ins_number = usage.second;
        int total_ins_number = 0;
        for (auto physical_type : type->equivalent_tiles)
            total_ins_number += device_ctx.grid.num_instances(physical_type, -1);
        float utilization = vtr::safe_ratio<float>(used_ins_number, total_ins_number);
        VTR_LOG("  %s: %f\n", type->name.c_str(), utilization);

        VTR_ASSERT_MSG(utilization <= 1.0, "At least one RAM type is over utilized after final placement. Ideally, this assertion should be removed and reassigning should be tried");
    }
    candidate_usages_final_ = candidate_usages;


    // Crosscheck and validate the atoms are assigned to correct groups.
    // This should be at the end of logical ram assignemts after any sorting since
    // helper methods use the order in this data structure afterwards.
    atom_to_group_.resize(atom_nlist.blocks().size());
    for (size_t gid = 0; gid < logical_ram_groups_.size(); ++gid) {
        auto& g = logical_ram_groups_[gid];
        for (AtomBlockId atom_id: g.atoms) {
            atom_to_group_[atom_id] = gid;
        }
    }
}