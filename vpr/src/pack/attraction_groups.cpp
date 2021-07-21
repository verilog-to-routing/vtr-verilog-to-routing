#include "attraction_groups.h"

AttractionInfo::AttractionInfo() {
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& atom_ctx = g_vpr_ctx.atom();
    int num_parts = floorplanning_ctx.constraints.get_num_partitions();

    //Create an attraction group for each partition in the floorplanning constraints
    for (int ipart = 0; ipart < num_parts; ipart++) {
        PartitionId partid(ipart);
        Partition part = floorplanning_ctx.constraints.get_partition(partid);

        AttractionGroup group_info;
        group_info.group_atoms = floorplanning_ctx.constraints.get_part_atoms(partid);

        attraction_groups.push_back(group_info);
    }

    //Initialize every atom to have no attraction group id
    int num_atoms = atom_ctx.nlist.blocks().size();

    atom_attraction_group.resize(num_atoms);
    fill(atom_attraction_group.begin(), atom_attraction_group.end(), NO_ATTRACTION_GROUP);

    //Then, fill in the group id for the atoms that do have an attraction group
    int num_att_grps = attraction_groups.size();

    for (int igroup; igroup < num_att_grps; igroup++) {
        AttractGroupId group_id(igroup);

        AttractionGroup att_group = attraction_groups[group_id];

        for (unsigned int iatom = 0; iatom < att_group.group_atoms.size(); iatom++) {
            atom_attraction_group[att_group.group_atoms[iatom]] = group_id;
        }
    }
}

AttractGroupId AttractionInfo::get_atom_attraction_group(AtomBlockId atom_id) {
    return atom_attraction_group[atom_id];
}

AttractionGroup AttractionInfo::get_attraction_group_info(AttractGroupId group_id) {
    return attraction_groups[group_id];
}

void AttractionInfo::set_atom_attraction_group(AtomBlockId atom_id, AttractGroupId group_id) {
    atom_attraction_group[atom_id] = group_id;
}

void AttractionInfo::set_attraction_group_info(AttractGroupId group_id, AttractionGroup group_info) {
    attraction_groups[group_id] = group_info;
}

void AttractionInfo::add_attraction_group(AttractionGroup group_info) {
    attraction_groups.push_back(group_info);
}

int AttractionInfo::num_attraction_groups() {
    return attraction_groups.size();
}

float AttractionInfo::get_attraction_group_gain(AttractGroupId group_id) {
    return attraction_groups[group_id].gain;
}

void AttractionInfo::set_attraction_group_gain(AttractGroupId group_id, float new_gain) {
    attraction_groups[group_id].gain = new_gain;
}

void AttractionInfo::initialize_atom_attraction_groups(int num_atoms) {
    atom_attraction_group.resize(num_atoms);
    fill(atom_attraction_group.begin(), atom_attraction_group.end(), NO_ATTRACTION_GROUP);
}
