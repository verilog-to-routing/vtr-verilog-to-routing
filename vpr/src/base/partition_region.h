#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

#include "region.h"
#include "atom_netlist_fwd.h"

/**
 * @file
 * @brief This file defines the PartitionRegions class. The PartitionRegions class is used to store the union
 * of regions that a partition can be placed in.
 */

class PartitionRegion {
  public:
    /**
     * @brief Return the intersection of two PartitionRegions
     *
     *   @param part_region     The PartitionRegion that the calling object will be intersected with
     */
    PartitionRegion get_intersection(PartitionRegion part_region);

    /**
     * @brief Add a region to the union of regions belonging to the partition
     *
     *   @param region     The region to be added to the calling object
     */
    void add_to_part_region(Region region);

    /**
     * @brief Return the union of regions
     */
    std::vector<Region> get_partition_region();

  private:
    std::vector<Region> partition_region; ///< union of rectangular regions that a partition can be placed in
};

#endif /* PARTITION_REGIONS_H */
