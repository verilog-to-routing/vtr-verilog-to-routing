/**
 * @file
 * @author  Haydar Cakan
 * @date    March 2026
 * @brief   Implementation of logical RAM inference and mapping.
 *
 * RAM atoms are grouped into sibling-feasible equivalence classes by
 * group_ram_atoms(). assign_ram_groups_by_min_area() then assigns each group to the
 * physical RAM type that minimizes total tile area, processing the most
 * constrained groups first. RamMapper wraps these two steps and adds a
 * timing pass that remaps timing-critical groups to the smallest (fastest)
 * candidate type, subject to device utilization constraints.
 */

#include "logical_ram_infer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "atom_netlist.h"
#include "cluster_util.h"
#include "vpr_error.h"
#include "globals.h"
#include "prepack.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_time.h"

vtr::vector<LogicalRamGroupId, LogicalRamGroup>
group_ram_atoms(const AtomNetlist& atom_nlist, const Prepacker& prepacker) {
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> groups;

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    for (AtomBlockId atom_blk_id : atom_nlist.blocks()) {
        if (!atom_blk_id.is_valid())
            continue;

        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive())
            continue;
        if (prim->pb_type->class_type != MEMORY_CLASS)
            continue;

        LogicalModelId root_model_id = atom_nlist.block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types =
            primitive_candidate_block_types[root_model_id];

        // Try to add this atom to an existing sibling-feasible group.
        bool placed = false;
        for (LogicalRamGroupId group_id : groups.keys()) {
            LogicalRamGroup& ram_group = groups[group_id];
            if (ram_group.rep_pb_type->model_id != prim->pb_type->model_id)
                continue;

            // Check sibling-feasibility in both directions for safety.
            bool atom_fits_in_group = primitive_memory_sibling_feasible(atom_blk_id, ram_group.rep_pb_type, ram_group.rep_blk);
            bool group_rep_fits_with_atom = primitive_memory_sibling_feasible(ram_group.rep_blk, prim->pb_type, atom_blk_id);
            if (atom_fits_in_group && group_rep_fits_with_atom && candidate_types == ram_group.candidate_types) {
                ram_group.atoms.push_back(atom_blk_id);
                placed = true;
                break;
            }
        }

        if (!placed) {
            LogicalRamGroup new_group;
            new_group.rep_blk = atom_blk_id;
            new_group.rep_pb_type = prim->pb_type;
            new_group.atoms.push_back(atom_blk_id);
            new_group.candidate_types = candidate_types;
            if (prim->pb_type->blif_model) {
                std::string blif_model(prim->pb_type->blif_model);
                new_group.is_output_registered =
                    blif_model.find("output_type{reg}") != std::string::npos;
            }
            groups.push_back(new_group);
        }
    }

    // Initialize total slice counts and per-type capacities.
    for (LogicalRamGroupId group_id : groups.keys()) {
        LogicalRamGroup& ram_group = groups[group_id];
        ram_group.total_memory_slices = (int)ram_group.atoms.size();
        for (t_logical_block_type_ptr cand : ram_group.candidate_types) {
            std::vector<t_logical_block_type> candidate_logical_type = {*cand};
            const t_pb_graph_node* candidate_prim =
                prepacker.get_expected_lowest_cost_primitive_for_atom_block(
                    ram_group.rep_blk, candidate_logical_type);
            ram_group.candidate_capacity[cand] = candidate_prim->total_primitive_count;
        }
    }

    VTR_LOG("Inferred %zu logical RAM group(s).\n", groups.size());
    return groups;
}

void assign_ram_groups_by_min_area(vtr::vector<LogicalRamGroupId, LogicalRamGroup>& groups,
                                   bool is_fixed_device) {
    // Compute the area cost of each candidate type for every group and sort
    // candidate_types by area so that candidate_types[0] is always the minimum-area choice.
    for (LogicalRamGroupId group_id : groups.keys()) {
        LogicalRamGroup& ram_group = groups[group_id];
        for (t_logical_block_type_ptr cand : ram_group.candidate_types) {
            int num_tiles = (int)std::ceil(vtr::safe_ratio<float>(
                ram_group.total_memory_slices, ram_group.candidate_capacity.at(cand)));
            int tile_area = cand->equivalent_tiles[0]->height
                            * cand->equivalent_tiles[0]->width;
            ram_group.candidate_area_cost[cand] = num_tiles * tile_area;
        }
        std::sort(ram_group.candidate_types.begin(), ram_group.candidate_types.end(),
                  [&](t_logical_block_type_ptr type_a, t_logical_block_type_ptr type_b) {
                      int area_a = ram_group.candidate_area_cost.at(type_a);
                      int area_b = ram_group.candidate_area_cost.at(type_b);
                      if (area_a != area_b) return area_a < area_b;
                      // Tie-break: prefer higher capacity, then by name.
                      int cap_a = ram_group.candidate_capacity.at(type_a);
                      int cap_b = ram_group.candidate_capacity.at(type_b);
                      if (cap_a != cap_b) return cap_a > cap_b;
                      return std::string(type_a->name) < std::string(type_b->name);
                  });
    }

    // Process groups most constrained first (fewest candidate types), then
    // largest (most atoms) first.
    std::vector<LogicalRamGroupId> order(groups.keys().begin(), groups.keys().end());
    std::stable_sort(order.begin(), order.end(),
                     [&](LogicalRamGroupId id_a, LogicalRamGroupId id_b) {
                         const LogicalRamGroup& group_a = groups[id_a];
                         const LogicalRamGroup& group_b = groups[id_b];
                         if (group_a.candidate_types.size() != group_b.candidate_types.size())
                             return group_a.candidate_types.size() < group_b.candidate_types.size();
                         return group_a.total_memory_slices > group_b.total_memory_slices;
                     });

    // For fixed devices, pre-compute available capacity per type so that
    // assignment can respect hard capacity limits.
    std::unordered_map<t_logical_block_type_ptr, int> available_resources;
    std::unordered_map<t_logical_block_type_ptr, int> resource_usages;
    if (is_fixed_device) {
        const DeviceContext& device_ctx = g_vpr_ctx.device();
        for (LogicalRamGroupId group_id : groups.keys()) {
            for (t_logical_block_type_ptr candidate_type : groups[group_id].candidate_types) {
                if (available_resources.count(candidate_type) == 0) {
                    int capacity = 0;
                    for (t_physical_tile_type_ptr physical_tile_type : candidate_type->equivalent_tiles)
                        capacity += device_ctx.grid.num_instances(physical_tile_type, -1);
                    available_resources[candidate_type] = capacity;
                }
            }
        }
    }

    // Assign each group to a physical type.
    for (LogicalRamGroupId group_id : order) {
        LogicalRamGroup& ram_group = groups[group_id];
        VTR_ASSERT(!ram_group.candidate_types.empty());

        if (!is_fixed_device) {
            // Auto-device: always pick the minimum-area type, no need to check
            // device utilization.
            ram_group.pre_assigned_type = ram_group.candidate_types[0];
        } else {
            // Fixed device: pick the first candidate (in area order) that
            // still has capacity on this device.
            t_logical_block_type_ptr chosen_type = nullptr;
            for (t_logical_block_type_ptr candidate_type : ram_group.candidate_types) {
                int needed = (int)std::ceil(vtr::safe_ratio<float>(
                    ram_group.total_memory_slices, ram_group.candidate_capacity.at(candidate_type)));
                if (resource_usages[candidate_type] + needed <= available_resources[candidate_type]) {
                    chosen_type = candidate_type;
                    break;
                }
            }

            if (!chosen_type) {
                // No type has remaining capacity; fall back to min-area and
                // let the packer handle the overflow.
                chosen_type = ram_group.candidate_types[0];
                VTR_LOG_WARN("RAM group %zu cannot fit on any available type on the fixed "
                             "device; assigning to '%s' as fallback.\n",
                             size_t(group_id), chosen_type->name.c_str());
            }

            int needed = (int)std::ceil(vtr::safe_ratio<float>(
                ram_group.total_memory_slices, ram_group.candidate_capacity.at(chosen_type)));
            resource_usages[chosen_type] += needed;
            ram_group.pre_assigned_type = chosen_type;
        }
    }
}

RamMapper::RamMapper(const AtomNetlist& atom_nlist,
                     const Prepacker& prepacker,
                     const PreClusterTimingManager& timing_manager,
                     const vtr::vector<LogicalRamGroupId, LogicalRamGroup>& precomputed_groups,
                     int verbosity,
                     bool is_fixed_device)
    : log_verbosity_(verbosity) {
    vtr::ScopedStartFinishTimer timer("Ram Mapper");

    if (precomputed_groups.empty()) {
        VTR_LOG("No pre-computed RAM groups, running grouping and area assignment.\n");
        logical_ram_groups_ = group_ram_atoms(atom_nlist, prepacker);
        assign_ram_groups_by_min_area(logical_ram_groups_, is_fixed_device);
    } else {
        VTR_LOG("Using pre-computed RAM groups from device size estimator.\n");
        logical_ram_groups_ = precomputed_groups;
    }

    log_utilizations("Device ram utilizations after area driven assignment:");
    check_assigned_rams_fit_on_device();
    if (timing_manager.is_valid()) {
        timing_pass(atom_nlist, timing_manager);
        log_utilizations("Device ram utilizations after timing driven remapping:");
        check_assigned_rams_fit_on_device();
    }
    build_atom_to_group_map(atom_nlist);
}

LogicalRamGroupId RamMapper::group_id_of(AtomBlockId blk) const {
    if (!blk.is_valid())
        return LogicalRamGroupId::INVALID();
    if (size_t(blk) >= atom_to_group_.size())
        return LogicalRamGroupId::INVALID();
    return atom_to_group_[blk];
}

const LogicalRamGroup& RamMapper::group(LogicalRamGroupId id) const {
    VTR_ASSERT(id.is_valid() && size_t(id) < logical_ram_groups_.size());
    return logical_ram_groups_[id];
}

void RamMapper::build_atom_to_group_map(const AtomNetlist& atom_nlist) {
    atom_to_group_.assign(atom_nlist.blocks().size(), LogicalRamGroupId::INVALID());
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        for (AtomBlockId atom_id : logical_ram_groups_[group_id].atoms) {
            atom_to_group_[atom_id] = group_id;
        }
    }
}

void RamMapper::log_utilizations(const std::string& label) const {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Compute current type usages from pre_assigned_type of each group.
    std::unordered_map<t_logical_block_type_ptr, int> usages;
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        const LogicalRamGroup& ram_group = logical_ram_groups_[group_id];
        if (!ram_group.pre_assigned_type)
            continue;
        int num_tiles = (int)std::ceil(vtr::safe_ratio<float>(
            ram_group.total_memory_slices,
            ram_group.candidate_capacity.at(ram_group.pre_assigned_type)));
        usages[ram_group.pre_assigned_type] += num_tiles;
    }

    VTR_LOG("%s\n", label.c_str());
    for (const auto& [type, used] : usages) {
        int total = 0;
        for (t_physical_tile_type_ptr phys : type->equivalent_tiles)
            total += device_ctx.grid.num_instances(phys, -1);
        VTR_LOG("  %s: %.4f (%d/%d)\n",
                type->name.c_str(),
                vtr::safe_ratio<float>(used, total),
                used, total);
    }
}

void RamMapper::check_assigned_rams_fit_on_device() const {
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Sum required instances per type from the current assignment and
    // verify that every group was assigned a type and has at least one candidate.
    std::unordered_map<t_logical_block_type_ptr, int> required;
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        const LogicalRamGroup& ram_group = logical_ram_groups_[group_id];

        if (ram_group.candidate_types.empty()) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Logical RAM group %zu has no compatible physical RAM type "
                            "in the architecture.\n",
                            size_t(group_id));
        }

        if (!ram_group.pre_assigned_type) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Logical RAM group %zu was not assigned a physical type.\n",
                            size_t(group_id));
        }

        int num_tiles = (int)std::ceil(vtr::safe_ratio<float>(
            ram_group.total_memory_slices, ram_group.candidate_capacity.at(ram_group.pre_assigned_type)));
        required[ram_group.pre_assigned_type] += num_tiles;
    }

    // Check each type against the available device capacity.
    for (const auto& [type, needed] : required) {
        int available = 0;
        for (t_physical_tile_type_ptr phys : type->equivalent_tiles)
            available += device_ctx.grid.num_instances(phys, -1);

        // pre_assigned_type is a hint to the packer, not a hard constraint.
        // The packer may still place the group on an alternative candidate
        // type, so a warning is issued rather than a fatal error.
        // TODO: Currently each logical RAM group is assigned to a single
        //       physical type. Allowing atoms within a group to be split
        //       across different physical types could yield a lower total
        //       area, at the cost of potentially worse timing. This finer-
        //       grained assignment could be invoked as a fallback when the
        //       current assignment exceeds the available device capacity.
        if (needed > available) {
            VTR_LOG_WARN("RAM type '%s' requires %d instance(s) but the device "
                         "only provides %d. The packer can attempt to use "
                         "alternative candidate types.\n",
                         type->name.c_str(), needed, available);
        }
    }
}

void RamMapper::timing_pass(const AtomNetlist& atom_nlist,
                            const PreClusterTimingManager& timing_manager) {
    VTR_LOG("Timing pass for logical RAMs.\n");
    VTR_ASSERT_MSG(timing_manager.is_valid(), "Valid timing manager required for timing pass.");

    const DeviceContext& device_ctx = g_vpr_ctx.device();

    // Compute max criticality per group.
    float overall_max_criticality = 0.0f;
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        LogicalRamGroup& ram_group = logical_ram_groups_[group_id];
        float max_crit = 0.0f;
        for (AtomBlockId atom_id : ram_group.atoms) {
            float crit = timing_manager.calc_atom_setup_criticality(atom_id, atom_nlist);
            if (crit > max_crit)
                max_crit = crit;
        }
        ram_group.max_atom_criticality = max_crit;
        if (max_crit > overall_max_criticality)
            overall_max_criticality = max_crit;
    }

    // Collect groups at the overall maximum criticality.
    constexpr float kEps = 1e-6f;
    std::vector<LogicalRamGroupId> critical_groups;
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        if (std::fabs(overall_max_criticality
                      - logical_ram_groups_[group_id].max_atom_criticality)
            < kEps)
            critical_groups.push_back(group_id);
    }
    VTR_LOG("Max atom criticality: %.4f (%zu critical group(s)).\n",
            overall_max_criticality, critical_groups.size());

    // Compute current usage of each type for device feasibility check during remapping.
    std::unordered_map<t_logical_block_type_ptr, int> usages;
    for (LogicalRamGroupId group_id : logical_ram_groups_.keys()) {
        const LogicalRamGroup& ram_group = logical_ram_groups_[group_id];
        if (!ram_group.pre_assigned_type)
            continue;
        int num_tiles = (int)std::ceil(vtr::safe_ratio<float>(
            ram_group.total_memory_slices, ram_group.candidate_capacity.at(ram_group.pre_assigned_type)));
        usages[ram_group.pre_assigned_type] += num_tiles;
    }

    // Try to remap each critical group to the smallest (fastest) candidate type.
    for (LogicalRamGroupId group_id : critical_groups) {
        LogicalRamGroup& ram_group = logical_ram_groups_[group_id];

        if (ram_group.is_output_registered) {
            VTR_LOGV(log_verbosity_ > 2, "  Group %zu: skipped (output is registered).\n", size_t(group_id));
            continue;
        }

        // Find the smallest-capacity candidate (assumed to be the faster).
        t_logical_block_type_ptr min_cap_type = nullptr;
        int min_cap = std::numeric_limits<int>::max();
        for (t_logical_block_type_ptr cand : ram_group.candidate_types) {
            int cap = ram_group.candidate_capacity.at(cand);
            if (cap < min_cap) {
                min_cap = cap;
                min_cap_type = cand;
            }
        }

        if (min_cap_type == ram_group.pre_assigned_type) {
            VTR_LOGV(log_verbosity_ > 2, "  Group %zu: already on smallest type (%s), no remap needed.\n",
                     size_t(group_id), ram_group.pre_assigned_type->name.c_str());
            continue;
        }

        // Check if remapping is feasible.
        int num_tiles_new_type = (int)std::ceil(vtr::safe_ratio<float>(
            ram_group.total_memory_slices, ram_group.candidate_capacity.at(min_cap_type)));
        int total_available = 0;
        for (t_physical_tile_type_ptr phys : min_cap_type->equivalent_tiles)
            total_available += device_ctx.grid.num_instances(phys, -1);

        if (usages[min_cap_type] + num_tiles_new_type > total_available) {
            VTR_LOGV(log_verbosity_ > 2, "  Group %zu: remap to %s rejected (utilization would exceed 1.0).\n",
                     size_t(group_id), min_cap_type->name.c_str());
            continue;
        }

        // Accept the remap.
        int num_tiles_old_type = (int)std::ceil(vtr::safe_ratio<float>(
            ram_group.total_memory_slices, ram_group.candidate_capacity.at(ram_group.pre_assigned_type)));
        usages[ram_group.pre_assigned_type] -= num_tiles_old_type;
        usages[min_cap_type] += num_tiles_new_type;
        ram_group.pre_assigned_type = min_cap_type;
        VTR_LOGV(log_verbosity_ > 2, "  Group %zu: remapped to %s for timing.\n",
                 size_t(group_id), min_cap_type->name.c_str());
    }
}
