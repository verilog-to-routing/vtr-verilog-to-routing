#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "atom_netlist_fwd.h"
#include "physical_types.h"
#include "PreClusterTimingManager.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"

// Forward declarations
class AtomNetlist;
class Prepacker;

/// @brief Unique identifier for a logical RAM group.
typedef vtr::StrongId<struct logical_ram_group_id_tag, size_t> LogicalRamGroupId;

/**
 * @brief Represents a group of RAM atom blocks that share the same non-data
 *        port configuration (sibling-feasible). All atoms in a group can be
 *        implemented by the same physical RAM block type.
 */
struct LogicalRamGroup {
    /// @brief Representative atom used for sibling-feasibility checks.
    AtomBlockId rep_blk;
    /// @brief Primitive pb_type of the representative atom.
    const t_pb_type* rep_pb_type = nullptr;
    /// @brief All atoms in this group.
    std::vector<AtomBlockId> atoms;
    /// @brief Legal physical block types for this group, sorted by area cost
    ///        (ascending) after assign_minimum_area() is called.
    std::vector<t_logical_block_type_ptr> candidate_types;
    /// @brief Number of primitives per physical instance for each candidate type.
    std::unordered_map<t_logical_block_type_ptr, int> candidate_capacity;
    /// @brief Area cost (instances × tile_area) for each candidate type.
    ///        Populated by assign_minimum_area().
    std::unordered_map<t_logical_block_type_ptr, int> area_type;
    /// @brief Total number of RAM slices (atoms) in the group.
    int total_memory_slices = 0;
    /// @brief Physical block type selected by area/timing assignment.
    t_logical_block_type_ptr pre_assigned_type = nullptr;
    /// @brief True if the RAM output is registered (relevant for timing remapping).
    bool is_output_registered = false;
    /// @brief Maximum setup-timing criticality among atoms in this group.
    float max_atom_criticality = 0.0f;
};

/**
 * @brief Groups RAM atoms from the netlist into sibling-feasible equivalence
 *        classes and initializes per-type capacities for each group.
 *
 * Two atoms are sibling-feasible if their non-data ports match bit-for-bit,
 * meaning they can be packed into the same physical RAM block.
 *
 * @param atom_nlist  The atom netlist.
 * @param prepacker   The prepacker (provides expected primitive mappings).
 * @return            A vector of groups, indexed by LogicalRamGroupId.
 */
vtr::vector<LogicalRamGroupId, LogicalRamGroup>
group_ram_atoms(const AtomNetlist& atom_nlist, const Prepacker& prepacker);

/**
 * @brief Assigns each group's pre_assigned_type to the minimum-area type.
 *
 * Groups are processed in priority order (most constrained first: fewest
 * candidate types, then most atoms) using a sorted index, without reordering
 * the groups vector. No device-grid feasibility check is performed.
 *
 * After this call, each group's candidate_types is sorted by area cost
 * (ascending) and pre_assigned_type is set to candidate_types[0].
 *
 * @param groups  Groups to assign (modified in place).
 */
void assign_minimum_area(vtr::vector<LogicalRamGroupId, LogicalRamGroup>& groups);

/**
 * @brief Infers logical RAM groups from the atom netlist and assigns each
 *        group to a physical RAM type, first minimizing area and then
 *        considering timing.
 *
 * In the auto-device flow, the area assignment is already performed by
 * DeviceSizeEstimator. The pre-computed groups can be passed directly to
 * skip the area assignment and only run the timing pass.
 */
class RamMapper {
  public:
    /**
     * @brief Constructs a RamMapper.
     *
     * If precomputed_groups is non-empty (typically from DeviceSizeEstimator
     * in the auto-device flow), the area assignment is skipped and only the
     * timing pass is performed. Otherwise, group_ram_atoms() followed by
     * assign_minimum_area() are called internally before the timing pass.
     *
     * @param atom_nlist          The atom netlist.
     * @param prepacker           The prepacker.
     * @param timing_manager      Pre-cluster timing information.
     * @param precomputed_groups  Previously computed groups (optional).
     */
    RamMapper(const AtomNetlist& atom_nlist,
              const Prepacker& prepacker,
              const PreClusterTimingManager& timing_manager,
              vtr::vector<LogicalRamGroupId, LogicalRamGroup> precomputed_groups = {});

    RamMapper() = default;
    RamMapper(RamMapper&&) = default;
    RamMapper& operator=(RamMapper&&) = default;

    /// @brief Returns the group ID for an atom, or INVALID if not a RAM atom.
    LogicalRamGroupId group_id_of(AtomBlockId blk) const;

    /// @brief Returns a const reference to the group for a given ID.
    const LogicalRamGroup& group(LogicalRamGroupId id) const;

    /// @brief Returns the number of logical RAM groups.
    size_t num_groups() const { return logical_ram_groups_.size(); }

  private:
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> logical_ram_groups_;
    vtr::vector<AtomBlockId, LogicalRamGroupId> atom_to_group_;

    /// @brief Builds the atom-to-group lookup map. Called once at the end of construction.
    void build_atom_to_group_map(const AtomNetlist& atom_nlist);

    /// @brief Remaps timing-critical groups to the fastest (smallest) RAM type.
    void timing_pass(const AtomNetlist& atom_nlist,
                     const PreClusterTimingManager& timing_manager);

    /// @brief Logs device utilization per RAM type with a given label.
    void log_utilizations(const std::string& label) const;
};
