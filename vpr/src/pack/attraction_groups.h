#pragma once
/*
 * attraction_groups.h
 *
 *  Created on: Jun. 13, 2021
 *      Author: khalid88
 */

#include <map>
#include <utility>

#include "user_relative_macros.h"
#include "vtr_strong_id.h"
#include "vtr_vector.h"
#include "atom_netlist_fwd.h"

// Forward declarations
class PartitionRegion;

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

    /*
     * When true, this group's molecules are proposed as candidates during the
     * initial candidate search of a cluster with this attraction group, instead
     * of only when the connectivity-based search runs dry (by which time the
     * cluster may already be full). Set for relative placement groups, whose
     * atoms must end up in one cluster but may not be connected to each other.
     */
    bool pull_in_initial_search = false;
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
    void create_att_groups_for_overfull_regions(const std::vector<PartitionRegion>& overfull_partition_regions);

    /*
     * Create attraction groups for all partitions.
     */
    void create_att_groups_for_all_regions();

    /*
     * Create one attraction group for each user-defined relative placement
     * group (see UserRelativeMacros), pulling the atoms of a group into the
     * same cluster.
     *
     * Must be called after construction AND after each create_att_groups_for_*
     * call above: those methods clear all attraction groups and rebuild them
     * from partitions only, which would otherwise silently drop the
     * relative-group pull on re-pack iterations.
     *
     * Relative groups are added last, so an atom that belongs to both a
     * partition and a relative placement group is assigned to the relative
     * group's attraction group.
     */
    void create_att_groups_for_relative_groups();

    /*
     * Increase the attraction gain of a relative placement group by the given
     * multiplier. Used to pull a group's atoms together more strongly when a
     * previous packing iteration left the group split across clusters.
     *
     * The boosted gain persists across the attraction group rebuilds above
     * (it is applied by create_att_groups_for_relative_groups).
     */
    void boost_relative_group_gain(UserRelativeMacroId macro_id, int group_idx, float multiplier);

    void assign_atom_attraction_ids();

    //Setters and getters for the class
    AttractGroupId get_atom_attraction_group(const AtomBlockId atom_id);

    AttractionGroup& get_attraction_group_info(const AttractGroupId group_id);

    void set_atom_attraction_group(const AtomBlockId atom_id, const AttractGroupId group_id);

    void set_attraction_group_info(AttractGroupId group_id, const AttractionGroup& group_info);

    float get_attraction_group_gain(const AttractGroupId group_id);

    void set_attraction_group_gain(const AttractGroupId group_id, const float new_gain);

    void add_attraction_group(const AttractionGroup& group_info);

    int num_attraction_groups() const;

    int get_att_group_pulls() const;

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

    /* The default attraction gain of relative placement groups. Stronger than the
     * default partition gain (0.08) since group atoms must end up in one cluster.
     */
    static constexpr float DEFAULT_REL_GROUP_GAIN = 0.5;

    /* Per relative placement group attraction gains, keyed by (macro id, group index).
     * Kept outside the AttractionGroup objects because those are destroyed on each
     * attraction group rebuild; boosted gains must survive rebuilds. Groups without
     * an entry use DEFAULT_REL_GROUP_GAIN.
     */
    std::map<std::pair<UserRelativeMacroId, int>, float> rel_group_gains_;
};

inline int AttractionInfo::get_att_group_pulls() const {
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

inline int AttractionInfo::num_attraction_groups() const {
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
