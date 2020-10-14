#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

#include "region.h"

/**
 * @file
 * @brief This file defines the PartitionRegions class. The PartitionRegions class is used to store the union
 * of regions that a partition can be placed in.
 */

class PartitionRegions {
  public:
    /**
     * Returns the intersection of two PartitionRegions vectors that are passed to it.
     */
    PartitionRegions get_intersection(PartitionRegions region);

    /**
     * A test for intersection of the PartitionRegions of two blocks
     */
    bool intersects(PartitionRegions region);

    /**
     * Takes Atom Id returns intersection of atom's PartitionRegions with the PartitionRegions of the object
     */
    PartitionRegions get_intersect(const AtomBlockId atom_id);

    //method to add to the partition_regions vector
    void add_to_part_regions(Region region);

    //method to get the partition_regions vector
    std::vector<Region> get_partition_regions();

  private:
    std::vector<Region> partition_regions; //vector of regions that a partition can be placed in
};

#endif /* PARTITION_REGIONS_H */
