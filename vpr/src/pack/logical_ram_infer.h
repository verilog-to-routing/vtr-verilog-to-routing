#pragma once

#include "atom_netlist_fwd.h"
#include "PreClusterTimingManager.h"

// // Each group: representative atom + pb_type (from its expected primitive)
struct LogicalRamGroup {
    AtomBlockId rep_blk;
    const t_pb_type* rep_pb_type; // primitive type used for the feasibility check
    std::vector<AtomBlockId> atoms;
    std::unordered_map<t_logical_block_type_ptr, int> candidate_capacity;
    float candidate_capacity_ratio;
    float candidate_area_ratio;
    std::vector<t_logical_block_type_ptr> candidate_types;
    int total_memory_slices = 0;
    int remaining_memory_slices = 0;
    t_logical_block_type_ptr pre_assigned_type = nullptr;
    t_logical_block_type_ptr last_selected_type = nullptr;
    bool is_output_registered = false;

    float max_atom_criticality = 0.0;


    // "Power-efficient RAM Mapping Algorithms for FPGA Embedded Memory Blocks" inspired elements
    t_logical_block_type_ptr type_init = nullptr;
    int area_init = 0;
    std::unordered_map<t_logical_block_type_ptr, int> area_type;
    int area_mem = 0;

};

// This is implemented only for 2 physical RAM type for now to try the usage of
// this idea. It can be extended to a general case if it works.
// Currently it is always defined to be M9K_cap / M144K_cap.
struct LogicalRamStats {
    float max_capacity_ratio = 0;
    float min_capacity_ratio = std::numeric_limits<float>::max();
};


class RamMapper {
  public:
    RamMapper(const AtomNetlist& atom_nlist,
              const Prepacker& prepacker,
              const LogicalModels& models,
              const std::vector<t_logical_block_type>& logical_block_types,
              const PreClusterTimingManager pre_cluster_timing_manager);
    
    RamMapper() = default;
    RamMapper(RamMapper&&) = default;
    RamMapper& operator=(RamMapper&&) = default;

    inline size_t group_id_of(AtomBlockId blk) const {
        if (!blk) return SIZE_MAX;
        if (size_t(blk) >= atom_to_group_.size()) return SIZE_MAX;
        return atom_to_group_[blk];
    }

    inline LogicalRamGroup& group_by_id_mut(size_t gid) const {
        VTR_ASSERT(gid < logical_ram_groups_.size());
        return logical_ram_groups_[gid];
    }
    inline const LogicalRamGroup& group_by_id(size_t gid) const {
        VTR_ASSERT(gid < logical_ram_groups_.size());
        return logical_ram_groups_[gid];
    }

    inline LogicalRamStats get_overall_logical_ram_stats () const {
        return logical_ram_stats_;
    }

    inline std::vector<LogicalRamGroup>& get_mutable_logical_rams () const {
        return logical_ram_groups_;
    }

    inline const std::unordered_map<t_logical_block_type_ptr, int> get_final_candidate_usages () const {
        return candidate_usages_final_;
    }

  private:
    mutable std::vector<LogicalRamGroup> logical_ram_groups_;
    vtr::vector<AtomBlockId, size_t> atom_to_group_;
    LogicalRamStats logical_ram_stats_;
    std::unordered_map<t_logical_block_type_ptr, int> candidate_usages_final_;
};
