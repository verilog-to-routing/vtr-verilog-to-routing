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
    AttractGroupId get_atom_attraction_group(AtomBlockId atom_id);

    AttractionGroup get_attraction_group_info(AttractGroupId group_id);

    void set_atom_attraction_group(AtomBlockId atom_id, AttractGroupId group_id);

    void set_attraction_group_info(AttractGroupId group_id, AttractionGroup group_info);

    void add_attraction_group(AttractionGroup group_info);

    int num_attraction_groups();

    float get_attraction_group_gain(AttractGroupId group_id);

    void set_attraction_group_gain(AttractGroupId group_id, float new_gain);

    void initialize_atom_attraction_groups(int num_atoms);

  private:
    //Store each atom's attraction group assuming each atom is in at most one attraction group
    vtr::vector<AtomBlockId, AttractGroupId> atom_attraction_group;

    //Store atoms and gain value that belong to each attraction group
    vtr::vector<AttractGroupId, AttractionGroup> attraction_groups;
};

#endif /* VPR_SRC_PACK_ATTRACTION_GROUPS_H_ */
