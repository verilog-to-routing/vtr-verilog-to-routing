#include "vpr_constraints.h"
#include "partition.h"

void VprConstraints::add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id) {
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

PartitionId VprConstraints::get_atom_partition(AtomBlockId blk_id) {
    PartitionId part_id;

    auto got = constrained_atoms.find(blk_id);

    if (got == constrained_atoms.end()) {
        return part_id = PartitionId::INVALID(); ///< atom is not in a partition, i.e. unconstrained
    } else {
        return got->second;
    }
}

void VprConstraints::add_partition(Partition part) {
    partitions.push_back(part);
}

Partition VprConstraints::get_partition(PartitionId part_id) {
    return partitions[part_id];
}

std::vector<AtomBlockId> VprConstraints::get_part_atoms(PartitionId part_id) {
    std::vector<AtomBlockId> part_atoms;

    for (auto& it : constrained_atoms) {
        if (it.second == part_id) {
            part_atoms.push_back(it.first);
        }
    }

    return part_atoms;
}

int VprConstraints::get_num_partitions() {
    return partitions.size();
}

PartitionRegion VprConstraints::get_partition_pr(PartitionId part_id) {
    PartitionRegion pr;
    pr = partitions[part_id].get_part_region();
    return pr;
}

void print_constraints(FILE* fp, VprConstraints constraints) {
    Partition temp_part;
    std::vector<AtomBlockId> atoms;

    int num_parts = constraints.get_num_partitions();

    fprintf(fp, "\n Number of partitions is %d \n", num_parts);

    for (int i = 0; i < num_parts; i++) {
        PartitionId part_id(i);

        temp_part = constraints.get_partition(part_id);

        fprintf(fp, "\npartition_id: %zu\n", size_t(part_id));
        print_partition(fp, temp_part);

        atoms = constraints.get_part_atoms(part_id);

        int atoms_size = atoms.size();

        fprintf(fp, "\tAtom vector size is %d\n", atoms_size);
        fprintf(fp, "\tIds of atoms in partition: \n");
        for (unsigned int j = 0; j < atoms.size(); j++) {
            AtomBlockId atom_id = atoms[j];
            fprintf(fp, "\t#%zu\n", size_t(atom_id));
        }
    }
}
