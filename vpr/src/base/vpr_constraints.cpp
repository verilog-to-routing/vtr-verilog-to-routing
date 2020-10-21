#include "vpr_constraints.h"

void VprConstraints::add_constrained_atom(const AtomBlockId blk_id, const PartitionId partition) {
    constrained_atoms.insert({blk_id, partition});
}

PartitionId VprConstraints::get_atom_partition(AtomBlockId blk_id) {
    PartitionId part_id;

    std::unordered_map<AtomBlockId, PartitionId>::const_iterator got = constrained_atoms.find(blk_id);

    if (got == constrained_atoms.end()) {
        return part_id = PartitionId::INVALID();
    } else {
        return got->second;
    }
}

void VprConstraints::add_partition(Partition part) {
    partitions.push_back(part);
}

vtr::vector<PartitionId, Partition> VprConstraints::get_partitions() {
    return partitions;
}
