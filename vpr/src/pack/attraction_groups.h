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

/**
 * @file
 * @brief This file defines the AttractionInfo class, which is used to store atoms in attraction groups, which are
 *        used during the clustering process.
 *
 * Overview
 * ========
 * Attraction groups are used during the clustering process. Atoms in the same attraction groups will be highly desirable to
 * be packed together. If an atom is in the same attraction group as an atoms already in the cluster, its gain will be increased
 * to reflect the increased desire to pack atoms of the same attraction group together. Currently, the attraction groups are created
 * based on which atoms are in the same Partition, from floorplanning constraints. In the future, attraction groups can be used to
 * pack atoms together based on other concepts.
 */

/// @brief Type tag for AttractGroupId
struct attraction_id_tag;

/// @brief A unique identifier for an attraction group.
typedef vtr::StrongId<attraction_id_tag> AttractGroupId;

struct AttractionGroup {
    //stores all atoms in the attraction group
    std::vector<AtomBlockId> group_atoms;

    /*
     * Atoms belonging to this attraction group will receive this gain if they
     * are potential candidates to be put in a cluster with the same attraction group.
     */
    float gain = 0.08;
};

class AttractionInfo {
  public:
    //Constructor that fills in the attraction groups based on vpr's floorplan constraints.
    //If no constraints were specified, then no attraction groups will be created.
    AttractionInfo(bool attraction_groups_on);

    /*
     * Create attraction groups for the partitions that contain overfull regions (i.e.
     * The region has more blocks of a certain type assigned to than are actually available).
     */
    void create_att_groups_for_overfull_regions();

    /*
     * Create attraction groups for all partitions.
     */
    void create_att_groups_for_all_regions();

    void assign_atom_attraction_ids();

    //Setters and getters for the class
    AttractGroupId get_atom_attraction_group(const AtomBlockId atom_id);

    AttractionGroup& get_attraction_group_info(const AttractGroupId group_id);

    void set_atom_attraction_group(const AtomBlockId atom_id, const AttractGroupId group_id);

    void set_attraction_group_info(AttractGroupId group_id, const AttractionGroup& group_info);

    float get_attraction_group_gain(const AttractGroupId group_id);

    void set_attraction_group_gain(const AttractGroupId group_id, const float new_gain);

    void add_attraction_group(const AttractionGroup& group_info);

    int num_attraction_groups();

    int get_att_group_pulls();

    void set_att_group_pulls(int num_pulls);

  private:
    //Store each atom's attraction group assuming each atom is in at most one attraction group
    //Atoms with no attraction group will have AttractGroupId::INVALID
    vtr::vector<AtomBlockId, AttractGroupId> atom_attraction_group;

    //Store atoms and gain value that belong to each attraction group
    vtr::vector<AttractGroupId, AttractionGroup> attraction_groups;

    /* When packing a cluster with molecules, we have various ways of seeking candidates molecule
     * candidates for the cluster. The att_group_pulls value is a way of keeping count of how many
     * times a cluster has searched for candidate molecules from its attraction group. We can increase
     * this value if we want to pack the cluster more densely (i.e. fill it with more molecules from
     * its attraction group).
     */
    int att_group_pulls = 1;
};

inline int AttractionInfo::get_att_group_pulls() {
    return att_group_pulls;
}

inline void AttractionInfo::set_att_group_pulls(int num_pulls) {
    att_group_pulls = num_pulls;
}

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

inline AttractionGroup& AttractionInfo::get_attraction_group_info(const AttractGroupId group_id) {
    return attraction_groups[group_id];
}

#endif /* VPR_SRC_PACK_ATTRACTION_GROUPS_H_ */
