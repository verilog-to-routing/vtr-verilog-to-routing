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

// ============================================================================
// Free functions
// ============================================================================

vtr::vector<LogicalRamGroupId, LogicalRamGroup>
group_ram_atoms(const AtomNetlist& atom_nlist, const Prepacker& prepacker) {
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> groups;

    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    for (AtomBlockId atom_blk_id : atom_nlist.blocks()) {
        if (!atom_blk_id.is_valid()) continue;

        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(atom_blk_id);
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive()) continue;
        if (prim->pb_type->class_type != MEMORY_CLASS) continue;

        auto root_model_id = atom_nlist.block_model(atom_blk_id);
        std::vector<t_logical_block_type_ptr> candidate_types =
            primitive_candidate_block_types[root_model_id];

        // Try to add this atom to an existing sibling-feasible group.
        bool placed = false;
        for (LogicalRamGroupId gid : groups.keys()) {
            LogicalRamGroup& g = groups[gid];
            if (g.rep_pb_type->model_id != prim->pb_type->model_id) continue;

            // Check sibling-feasibility in both directions for safety.
            bool ok_ab = primitive_memory_sibling_feasible(atom_blk_id, g.rep_pb_type, g.rep_blk);
            bool ok_ba = primitive_memory_sibling_feasible(g.rep_blk, prim->pb_type, atom_blk_id);
            if (ok_ab && ok_ba && candidate_types == g.candidate_types) {
                g.atoms.push_back(atom_blk_id);
                placed = true;
                break;
            }
        }

        if (!placed) {
            LogicalRamGroup ng;
            ng.rep_blk = atom_blk_id;
            ng.rep_pb_type = prim->pb_type;
            ng.atoms.push_back(atom_blk_id);
            ng.candidate_types = candidate_types;
            if (prim->pb_type->blif_model) {
                std::string blif_model(prim->pb_type->blif_model);
                ng.is_output_registered =
                    blif_model.find("output_type{reg}") != std::string::npos;
            }
            groups.push_back(ng);
        }
    }

    // Initialize total slice counts and per-type capacities.
    for (LogicalRamGroupId gid : groups.keys()) {
        LogicalRamGroup& g = groups[gid];
        g.total_memory_slices = (int)g.atoms.size();
        for (t_logical_block_type_ptr cand : g.candidate_types) {
            std::vector<t_logical_block_type> candidate_logical_type = {*cand};
            const t_pb_graph_node* candidate_prim =
                prepacker.get_expected_lowest_cost_primitive_for_atom_block(
                    g.rep_blk, candidate_logical_type);
            g.candidate_capacity[cand] = candidate_prim->total_primitive_count;
        }
    }

    VTR_LOG("Inferred %zu logical RAM group(s).\n", groups.size());
    return groups;
}

void assign_ram_groups_by_min_area(vtr::vector<LogicalRamGroupId, LogicalRamGroup>& groups) {
    // Step 1: Compute area cost for each group × candidate type, then sort
    //         candidate_types by area so that candidate_types[0] is always
    //         the minimum-area choice.
    for (LogicalRamGroupId gid : groups.keys()) {
        LogicalRamGroup& g = groups[gid];
        for (t_logical_block_type_ptr cand : g.candidate_types) {
            int n = (int)std::ceil(vtr::safe_ratio<float>(
                g.total_memory_slices, g.candidate_capacity.at(cand)));
            int tile_area = cand->equivalent_tiles[0]->height
                          * cand->equivalent_tiles[0]->width;
            g.candidate_area_cost[cand] = n * tile_area;
        }
        std::sort(g.candidate_types.begin(), g.candidate_types.end(),
                  [&](t_logical_block_type_ptr a, t_logical_block_type_ptr b) {
                      int area_a = g.candidate_area_cost.at(a);
                      int area_b = g.candidate_area_cost.at(b);
                      if (area_a != area_b) return area_a < area_b;
                      // Tie-break: prefer higher capacity, then by name.
                      int cap_a = g.candidate_capacity.at(a);
                      int cap_b = g.candidate_capacity.at(b);
                      if (cap_a != cap_b) return cap_a > cap_b;
                      return std::string(a->name) < std::string(b->name);
                  });
    }

    // Step 2: Determine processing order without reordering the groups vector.
    //         Most constrained first (fewest candidate types), then largest
    //         groups (most atoms) first.
    std::vector<LogicalRamGroupId> order(groups.keys().begin(), groups.keys().end());
    std::stable_sort(order.begin(), order.end(),
                     [&](LogicalRamGroupId a, LogicalRamGroupId b) {
                         const auto& ga = groups[a];
                         const auto& gb = groups[b];
                         if (ga.candidate_types.size() != gb.candidate_types.size())
                             return ga.candidate_types.size() < gb.candidate_types.size();
                         return ga.total_memory_slices > gb.total_memory_slices;
                     });

    // Step 3: Assign each group to its minimum-area type (candidate_types[0]
    //         after the sort above).
    for (LogicalRamGroupId gid : order) {
        LogicalRamGroup& g = groups[gid];
        VTR_ASSERT(!g.candidate_types.empty());
        g.pre_assigned_type = g.candidate_types[0];
    }
}

// ============================================================================
// RamMapper
// ============================================================================

RamMapper::RamMapper(const AtomNetlist& atom_nlist,
                     const Prepacker& prepacker,
                     const PreClusterTimingManager& timing_manager,
                     vtr::vector<LogicalRamGroupId, LogicalRamGroup> precomputed_groups) {
    vtr::ScopedStartFinishTimer timer("Ram Mapper");

    if (precomputed_groups.empty()) {
        VTR_LOG("No pre-computed RAM groups, running grouping and area assignment.\n");
        logical_ram_groups_ = group_ram_atoms(atom_nlist, prepacker);
        assign_ram_groups_by_min_area(logical_ram_groups_);
        check_assigned_rams_fit_on_device();
    } else {
        VTR_LOG("Using pre-computed RAM groups from device size estimator.\n");
        logical_ram_groups_ = std::move(precomputed_groups);
    }

    log_utilizations("Device ram utilizations after area driven assignment:");
    timing_pass(atom_nlist, timing_manager);
    log_utilizations("Device ram utilizations after timing pass:");
    build_atom_to_group_map(atom_nlist);
}

LogicalRamGroupId RamMapper::group_id_of(AtomBlockId blk) const {
    if (!blk.is_valid()) return LogicalRamGroupId::INVALID();
    if (size_t(blk) >= atom_to_group_.size()) return LogicalRamGroupId::INVALID();
    return atom_to_group_[blk];
}

const LogicalRamGroup& RamMapper::group(LogicalRamGroupId id) const {
    VTR_ASSERT(id.is_valid() && size_t(id) < logical_ram_groups_.size());
    return logical_ram_groups_[id];
}

void RamMapper::build_atom_to_group_map(const AtomNetlist& atom_nlist) {
    atom_to_group_.assign(atom_nlist.blocks().size(), LogicalRamGroupId::INVALID());
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        for (AtomBlockId atom_id : logical_ram_groups_[gid].atoms) {
            atom_to_group_[atom_id] = gid;
        }
    }
}

void RamMapper::log_utilizations(const std::string& label) const {
    const auto& device_ctx = g_vpr_ctx.device();

    // Compute current type usages from pre_assigned_type of each group.
    std::unordered_map<t_logical_block_type_ptr, int> usages;
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        const LogicalRamGroup& g = logical_ram_groups_[gid];
        if (!g.pre_assigned_type) continue;
        int n = (int)std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices,
            g.candidate_capacity.at(g.pre_assigned_type)));
        usages[g.pre_assigned_type] += n;
    }

    VTR_LOG("%s\n", label.c_str());
    for (const auto& [type, used] : usages) {
        int total = 0;
        for (auto phys : type->equivalent_tiles)
            total += device_ctx.grid.num_instances(phys, -1);
        VTR_LOG("  %s: %.4f (%d/%d)\n",
                type->name.c_str(),
                vtr::safe_ratio<float>(used, total),
                used, total);
    }
}

void RamMapper::check_assigned_rams_fit_on_device() const {
    const auto& device_ctx = g_vpr_ctx.device();

    // Sum required instances per type from the current assignment.
    // Also verify that every group was assigned a type and has at least one candidate.
    std::unordered_map<t_logical_block_type_ptr, int> required;
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        const LogicalRamGroup& g = logical_ram_groups_[gid];

        if (g.candidate_types.empty()) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Logical RAM group %zu has no compatible physical RAM type "
                            "in the architecture.\n",
                            size_t(gid));
        }

        if (!g.pre_assigned_type) {
            VPR_FATAL_ERROR(VPR_ERROR_PACK,
                            "Logical RAM group %zu was not assigned a physical type.\n",
                            size_t(gid));
        }

        int n = (int)std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices, g.candidate_capacity.at(g.pre_assigned_type)));
        required[g.pre_assigned_type] += n;
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
    VTR_LOG("Starting timing pass for logical RAMs.\n");

    // Compute max criticality per group.
    float overall_max_criticality = 0.0f;
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        LogicalRamGroup& g = logical_ram_groups_[gid];
        float max_crit = 0.0f;
        for (AtomBlockId atom_id : g.atoms) {
            float crit = timing_manager.calc_atom_setup_criticality(atom_id, atom_nlist);
            if (crit > max_crit) max_crit = crit;
        }
        g.max_atom_criticality = max_crit;
        if (max_crit > overall_max_criticality)
            overall_max_criticality = max_crit;
    }

    // Collect groups at the overall maximum criticality.
    const float EPS = 1e-6f;
    std::vector<LogicalRamGroupId> critical_groups;
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        if (std::fabs(overall_max_criticality
                      - logical_ram_groups_[gid].max_atom_criticality) < EPS) {
            critical_groups.push_back(gid);
        }
    }
    VTR_LOG("Max atom criticality: %.4f (%zu critical group(s)).\n",
            overall_max_criticality, critical_groups.size());

    // Compute current usages for feasibility checks below.
    std::unordered_map<t_logical_block_type_ptr, int> usages;
    for (LogicalRamGroupId gid : logical_ram_groups_.keys()) {
        const LogicalRamGroup& g = logical_ram_groups_[gid];
        if (!g.pre_assigned_type) continue;
        int n = (int)std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices, g.candidate_capacity.at(g.pre_assigned_type)));
        usages[g.pre_assigned_type] += n;
    }

    const auto& device_ctx = g_vpr_ctx.device();

    // Try to remap each critical group to the smallest (fastest) candidate type.
    for (LogicalRamGroupId gid : critical_groups) {
        LogicalRamGroup& g = logical_ram_groups_[gid];

        if (g.is_output_registered) {
            VTR_LOG("  Group %zu: skipped (output is registered).\n", size_t(gid));
            continue;
        }

        // Find the smallest-capacity candidate (assumed to be the fastest).
        t_logical_block_type_ptr min_cap_type = nullptr;
        int min_cap = std::numeric_limits<int>::max();
        for (const auto& [cand, cap] : g.candidate_capacity) {
            if (cap < min_cap) {
                min_cap = cap;
                min_cap_type = cand;
            }
        }

        if (min_cap_type == g.pre_assigned_type) {
            VTR_LOG("  Group %zu: already on smallest type (%s), no remap needed.\n",
                    size_t(gid), g.pre_assigned_type->name.c_str());
            continue;
        }

        // Check if remapping is feasible.
        int needed = (int)std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices, g.candidate_capacity.at(min_cap_type)));
        int total_avail = 0;
        for (auto phys : min_cap_type->equivalent_tiles)
            total_avail += device_ctx.grid.num_instances(phys, -1);

        if (usages[min_cap_type] + needed > total_avail) {
            VTR_LOG("  Group %zu: remap to %s rejected (utilization would exceed 1.0).\n",
                    size_t(gid), min_cap_type->name.c_str());
            continue;
        }

        // Accept the remap.
        int old_needed = (int)std::ceil(vtr::safe_ratio<float>(
            g.total_memory_slices, g.candidate_capacity.at(g.pre_assigned_type)));
        usages[g.pre_assigned_type] -= old_needed;
        usages[min_cap_type] += needed;
        g.pre_assigned_type = min_cap_type;
        VTR_LOG("  Group %zu: remapped to %s for timing.\n",
                size_t(gid), min_cap_type->name.c_str());
    }

    // Verify no over-utilization after timing remapping.
    for (const auto& [type, used] : usages) {
        int total = 0;
        for (auto phys : type->equivalent_tiles)
            total += device_ctx.grid.num_instances(phys, -1);
        VTR_ASSERT_MSG(used <= total,
                       "At least one RAM type is over-utilized after the timing pass.");
    }
}
