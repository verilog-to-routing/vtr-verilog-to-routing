#include "attraction_groups.h"

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
