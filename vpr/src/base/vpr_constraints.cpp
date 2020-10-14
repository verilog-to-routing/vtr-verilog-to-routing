#include "vpr_constraints.h"

void VprConstraints::add_constrained_atom(const AtomBlockId blk_id, const PartitionId partition) {
    constrained_atoms.insert({blk_id, partition});
}

PartitionId VprConstraints::get_atom_partition(AtomBlockId blk_id) {
    PartitionId part_id;
    part_id = constrained_atoms.at(blk_id);
    return part_id;
}

void VprConstraints::add_partition(Partition part) {
    partitions.push_back(part);
}

vtr::vector<PartitionId, Partition> VprConstraints::get_partitions() {
    return partitions;
}
