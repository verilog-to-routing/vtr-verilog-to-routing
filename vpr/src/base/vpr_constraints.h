#ifndef VPR_CONSTRAINTS_H
#define VPR_CONSTRAINTS_H

#include "vtr_vector.h"
#include "vpr_utils.h"
#include "partition.h"
#include "partition_region.h"

/**
 * @file
 * @brief This file defines the VprConstraints class used to store and read out data related to user-specified
 * 		  block and region constraints for the packing and placement stages.
 *
 * Overview
 * ========
 * This class contains functions that read in and store information from a constraints XML file.
 * The XML file provides a partition list, with the names/ids of the partitions, and each atom in the partition.
 * It also specifies which regions the partitions should be placed in.
 *
 */

class VprConstraints {
  public:
    //function to add an atom to constrained_atoms
    void add_constrained_atom(const AtomBlockId blk_id, const PartitionId part_id);

    //function to find which partition an atom belongs to
    PartitionId get_atom_partition(AtomBlockId blk_id);

    //function to add a partition to partitions
    void add_partition(Partition part);

    //function to return a partition given a PartitionId
    Partition get_partition(PartitionId part_id);

    //function to return the atoms belonging to a partition
    std::vector<AtomBlockId> get_part_atoms(PartitionId part_id);

  private:
    /**
     * Store constraints information of each  constrained atom.
     * Store ID of the partition the atom belongs to.
     */
    std::unordered_map<AtomBlockId, PartitionId> constrained_atoms;

    /**
     * Store partitions in a vector
     */
    vtr::vector<PartitionId, Partition> partitions;
};

#endif /* VPR_CONSTRAINTS_H */
