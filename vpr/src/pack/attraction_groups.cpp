#include "attraction_groups.h"

AttractionInfo::AttractionInfo(bool attraction_groups_on) {
    const auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& atom_ctx = g_vpr_ctx.atom();
    int num_parts = floorplanning_ctx.constraints.get_num_partitions();

    //Initialize every atom to have no attraction group id
    int num_atoms = atom_ctx.nlist.blocks().size();

    atom_attraction_group.resize(num_atoms);
    fill(atom_attraction_group.begin(), atom_attraction_group.end(), AttractGroupId::INVALID());

    /*
     * Create an attraction group for each partition in the floorplanning constraints
     * if the packer option for attraction groups is turned on.
     */
    if (attraction_groups_on) {
        for (int ipart = 0; ipart < num_parts; ipart++) {
            PartitionId partid(ipart);

            AttractionGroup group_info;
            group_info.group_atoms = floorplanning_ctx.constraints.get_part_atoms(partid);

            attraction_groups.push_back(group_info);
        }

        //Then, fill in the group id for the atoms that do have an attraction group
        assign_atom_attraction_ids();

        att_group_pulls = 1;
    }
}

void AttractionInfo::create_att_groups_for_overfull_regions() {
    const auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& atom_ctx = g_vpr_ctx.atom();
    int num_parts = floorplanning_ctx.constraints.get_num_partitions();

    //clear the data structures before continuing
    atom_attraction_group.clear();
    attraction_groups.clear();

    //Initialize every atom to have no attraction group id
    int num_atoms = atom_ctx.nlist.blocks().size();

    atom_attraction_group.resize(num_atoms);
    fill(atom_attraction_group.begin(), atom_attraction_group.end(), AttractGroupId::INVALID());

    const std::vector<PartitionRegion>& overfull_prs = floorplanning_ctx.overfull_partition_regions;

    /*
     * Create an attraction group for each partition that overlaps with at least one overfull partition
     */
    for (int ipart = 0; ipart < num_parts; ipart++) {
        PartitionId partid(ipart);

        const Partition& part = floorplanning_ctx.constraints.get_partition(partid);

        for (const PartitionRegion& overfull_pr : overfull_prs) {
            PartitionRegion intersect_pr = intersection(part.get_part_region(), overfull_pr);
            if (!intersect_pr.empty()) {
                AttractionGroup group_info;
                group_info.group_atoms = floorplanning_ctx.constraints.get_part_atoms(partid);
                attraction_groups.push_back(group_info);
                break;
            }
        }
    }

    //Then, fill in the group id for the atoms that do have an attraction group
    assign_atom_attraction_ids();

    att_group_pulls = 1;

    VTR_LOG("%d clustering attraction groups created. \n", attraction_groups.size());
}

void AttractionInfo::create_att_groups_for_all_regions() {
    const auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& atom_ctx = g_vpr_ctx.atom();
    int num_parts = floorplanning_ctx.constraints.get_num_partitions();

    //clear the data structures before continuing
    atom_attraction_group.clear();
    attraction_groups.clear();

    //Initialize every atom to have no attraction group id
    int num_atoms = atom_ctx.nlist.blocks().size();

    atom_attraction_group.resize(num_atoms);
    fill(atom_attraction_group.begin(), atom_attraction_group.end(), AttractGroupId::INVALID());

    /*
     * Create a PartitionRegion that contains all the overfull regions so that you can
     * make an attraction group for any partition that intersects with any of these regions
     */

    /*
     * Create an attraction group for each partition with an overfull region.
     */

    for (int ipart = 0; ipart < num_parts; ipart++) {
        PartitionId partid(ipart);

        AttractionGroup group_info;
        group_info.group_atoms = floorplanning_ctx.constraints.get_part_atoms(partid);

        attraction_groups.push_back(group_info);
    }

    //Then, fill in the group id for the atoms that do have an attraction group
    assign_atom_attraction_ids();

    att_group_pulls = 1;

    VTR_LOG("%d clustering attraction groups created. \n", attraction_groups.size());
}

void AttractionInfo::assign_atom_attraction_ids() {
    //Fill in the group id for the atoms that do have an attraction group
    int num_att_grps = attraction_groups.size();

    for (int igroup = 0; igroup < num_att_grps; igroup++) {
        AttractGroupId group_id(igroup);

        AttractionGroup att_group = attraction_groups[group_id];

        for (auto group_atom : att_group.group_atoms) {
            atom_attraction_group[group_atom] = group_id;
        }
    }
}

void AttractionInfo::set_attraction_group_info(AttractGroupId group_id, const AttractionGroup& group_info) {
    attraction_groups[group_id] = group_info;
}

void AttractionInfo::add_attraction_group(const AttractionGroup& group_info) {
    attraction_groups.push_back(group_info);
}
