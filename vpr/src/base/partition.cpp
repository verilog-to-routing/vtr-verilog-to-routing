#include "partition.h"
#include <algorithm>
#include <vector>

PartitionId Partition::get_partition_id() {
    return id;
}

void Partition::set_partition_id(PartitionId _part_id) {
    id = _part_id;
}

std::string Partition::get_name() {
    return name;
}

void Partition::set_name(std::string _part_name) {
    name = _part_name;
}

void Partition::add_to_atoms(AtomBlockId atom_id) {
    atom_blocks.push_back(atom_id);
}

bool Partition::contains_atom(AtomBlockId atom_id) {
    bool contains_atom = false;
    if (std::find(atom_blocks.begin(), atom_blocks.end(), atom_id) != atom_blocks.end()) {
        contains_atom = true;
    }
    return contains_atom;
}

std::vector<AtomBlockId> Partition::get_atoms() {
    return atom_blocks;
}

PartitionRegions Partition::get_part_regions() {
    return part_regions;
}

void Partition::set_part_regions(PartitionRegions pr) {
    part_regions = pr;
}
