#ifndef VPR_CONSTRAINTS_H
#define VPR_CONSTRAINTS_H

#include "vtr_vector.h"
#include "vpr_utils.h"
#include "partition.h"
#include "partition_regions.h"

/**
 * @file
 * @brief This file defines the VprConstraints class used to store and read out data related to user-specified
 * 		  block and region constraints for the packing and placement stages.
 *
 * Overview
 * ========
 * This class contains functions that read in and store information from a constraints XML file.
 * The XML file provides a partition list, with the names/ids of the partitions, and each atom in the partition.
 * It also provides placement constraints, where it is specified which regions the partitions should go in.
 * In the constraints file, a placement constraint can also be created for a specific primitive. A partition would be automatically
 * created for the primitive when a placement constraint was specified for it.
 *
 */

class VprConstraints {
  public:
    /**
     * Returns the intersection of two PartitionRegions vectors that are passed to it.
     * Can be used by atom_cluster_intersection.
     */
    PartitionRegions do_regions_intersect(PartitionRegions part_regions_old, PartitionRegions part_regions_new);

    /**
     * Takes Atom Id and PartitonRegions vector and returns the intersection of the atom's partition regions
     * and the net PartitionRegions of the cluster (if any exists)
     * Used by clusterer during packing.
     */
    PartitionRegions atom_cluster_intersection(const AtomBlockId id, PartitionRegions part_regions);

  private:
    /**
     * Store constraints information of each  constrained atom.
     * Store ID of the partition the atom belongs to/invalid entry if it doesn't belong to any partition.
     */
    std::unordered_map<AtomBlockId, PartitionId> constrained_atoms;

    /**
     * Store partitions in a vector
     */
    std::vector<Partition> partitions;
};

#endif /* VPR_CONSTRAINTS_H */
