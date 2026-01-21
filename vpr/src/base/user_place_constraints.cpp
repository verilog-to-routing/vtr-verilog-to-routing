#include "user_place_constraints.h"
#include <unordered_set>
#include "physical_types.h"
#include "vtr_assert.h"

void UserPlaceConstraints::add_constrained_atom(AtomBlockId blk_id, PartitionId part_id) {
    auto got = constrained_atoms.find(blk_id);

    /**
     * Each atom can only be in one partition. If the atom is not found in constrained_atoms, it
     * will be added with its partition id.
     * If the atom is already in constrained_atoms, the partition id will be updated.
     */
    if (got == constrained_atoms.end()) {
        constrained_atoms.insert({blk_id, part_id});
    } else {
        got->second = part_id;
    }
}

PartitionId UserPlaceConstraints::get_atom_partition(AtomBlockId blk_id) const {
    auto got = constrained_atoms.find(blk_id);

    if (got == constrained_atoms.end()) {
        return PartitionId::INVALID(); ///< atom is not in a partition, i.e. unconstrained
    } else {
        return got->second;
    }
}

void UserPlaceConstraints::add_partition(const Partition& part) {
    partitions.push_back(part);
}

const Partition& UserPlaceConstraints::get_partition(PartitionId part_id) const {
    return partitions[part_id];
}

Partition& UserPlaceConstraints::get_mutable_partition(PartitionId part_id) {
    return partitions[part_id];
}

std::vector<AtomBlockId> UserPlaceConstraints::get_part_atoms(PartitionId part_id) const {
    std::vector<AtomBlockId> part_atoms;

    for (const auto& it : constrained_atoms) {
        if (it.second == part_id) {
            part_atoms.push_back(it.first);
        }
    }

    return part_atoms;
}

int UserPlaceConstraints::get_num_partitions() const {
    return partitions.size();
}

const PartitionRegion& UserPlaceConstraints::get_partition_pr(PartitionId part_id) const {
    return partitions[part_id].get_part_region();
}

PartitionRegion& UserPlaceConstraints::get_mutable_partition_pr(PartitionId part_id) {
    return partitions[part_id].get_mutable_part_region();
}

void UserPlaceConstraints::constrain_part_lb_type(PartitionId part_id, t_logical_block_type_ptr lb_type) {
    VTR_ASSERT_SAFE(part_id.is_valid());
    VTR_ASSERT_SAFE(lb_type != nullptr);

    auto it = constrained_part_lb_types_.find(part_id);
    if (it == constrained_part_lb_types_.end()) {
        // If this partition is not already constrained, create a new entry.
        constrained_part_lb_types_.insert({part_id, {lb_type}});
    } else {
        // If this partition is already constrained to a block type, add it
        // to the set.
        it->second.insert(lb_type);
    }
}

bool UserPlaceConstraints::is_part_constrained_to_lb_types(PartitionId part_id) const {
    return constrained_part_lb_types_.contains(part_id);
}

const std::unordered_set<t_logical_block_type_ptr>& UserPlaceConstraints::get_part_lb_type_constraints(PartitionId part_id) const {
    auto it = constrained_part_lb_types_.find(part_id);

    VTR_ASSERT_SAFE(it != constrained_part_lb_types_.end());

    return it->second;
}

void print_placement_constraints(FILE* fp, const UserPlaceConstraints& constraints) {
    std::vector<AtomBlockId> atoms;

    int num_parts = constraints.get_num_partitions();

    fprintf(fp, "\n Number of partitions is %d \n", num_parts);

    for (int i = 0; i < num_parts; i++) {
        PartitionId part_id(i);

        const Partition& temp_part = constraints.get_partition(part_id);

        fprintf(fp, "\npartition_id: %zu\n", size_t(part_id));
        print_partition(fp, temp_part);

        atoms = constraints.get_part_atoms(part_id);

        int atoms_size = atoms.size();

        fprintf(fp, "\tAtom vector size is %d\n", atoms_size);
        fprintf(fp, "\tIds of atoms in partition: \n");
        for (auto atom_id : atoms) {
            fprintf(fp, "\t#%zu\n", size_t(atom_id));
        }
    }
}
