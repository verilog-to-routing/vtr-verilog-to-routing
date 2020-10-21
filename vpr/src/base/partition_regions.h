#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

#include "region.h"
#include "atom_netlist_fwd.h"

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
    PartitionRegions get_intersection(PartitionRegions part_region);

    //method to add to the partition_regions vector
    void add_to_part_regions(Region region);

    //method to get the partition_regions vector
    std::vector<Region> get_partition_regions();

  private:
    std::vector<Region> partition_regions; //vector of regions that a partition can be placed in
};

#endif /* PARTITION_REGIONS_H */
