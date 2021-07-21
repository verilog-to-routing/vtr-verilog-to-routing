/*
 * attraction_groups.h
 *
 *  Created on: Jun. 13, 2021
 *      Author: khalid88
 */

#ifndef VPR_SRC_PACK_ATTRACTION_GROUPS_H_
#define VPR_SRC_PACK_ATTRACTION_GROUPS_H_

#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "atom_netlist.h"
#include "globals.h"

/// @brief Type tag for AttractGroupId
struct attraction_id_tag;

/// @brief A unique identifier for a partition
typedef vtr::StrongId<attraction_id_tag> AttractGroupId;

struct AttractionGroup {
    std::vector<AtomBlockId> group_atoms;
    float gain = 5; //give every attraction group an initial gain of 5
    bool region_size_one = false;
};

/// @brief sentinel value for indicating that an attraction group has not been specified
constexpr AttractGroupId NO_ATTRACTION_GROUP(-1);

class AttractionInfo {
  public:
    AttractionInfo();

    AttractGroupId get_atom_attraction_group(const AtomBlockId atom_id);

    AttractionGroup get_attraction_group_info(const AttractGroupId group_id);

    void set_atom_attraction_group(const AtomBlockId atom_id, const AttractGroupId group_id);

    void set_attraction_group_info(AttractGroupId group_id, const AttractionGroup& group_info);

    void add_attraction_group(const AttractionGroup& group_info);

    int num_attraction_groups();

    float get_attraction_group_gain(const AttractGroupId group_id);

    void set_attraction_group_gain(const AttractGroupId group_id, const float new_gain);

  private:
    //Store each atom's attraction group assuming each atom is in at most one attraction group
    vtr::vector<AtomBlockId, AttractGroupId> atom_attraction_group;

    //Store atoms and gain value that belong to each attraction group
    vtr::vector<AttractGroupId, AttractionGroup> attraction_groups;
};

inline AttractGroupId AttractionInfo::get_atom_attraction_group(const AtomBlockId atom_id) {
    return atom_attraction_group[atom_id];
}

inline void AttractionInfo::set_atom_attraction_group(const AtomBlockId atom_id, const AttractGroupId group_id) {
    atom_attraction_group[atom_id] = group_id;
    attraction_groups[group_id].group_atoms.push_back(atom_id);
}

inline int AttractionInfo::num_attraction_groups() {
    return attraction_groups.size();
}

inline float AttractionInfo::get_attraction_group_gain(const AttractGroupId group_id) {
    return attraction_groups[group_id].gain;
}

inline void AttractionInfo::set_attraction_group_gain(const AttractGroupId group_id, const float new_gain) {
    attraction_groups[group_id].gain = new_gain;
}







#endif /* VPR_SRC_PACK_ATTRACTION_GROUPS_H_ */
