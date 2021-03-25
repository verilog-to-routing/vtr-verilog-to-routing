#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

#include "region.h"
#include "atom_netlist_fwd.h"
#include "vpr_types.h"

/**
 * @file
 * @brief This file defines the PartitionRegion class. The PartitionRegion class is used to store the union
 * of regions that a partition can be placed in.
 *
 * For more details on what a region is, see vpr/src/base/region.h
 */

class PartitionRegion {
  public:
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

    /**
     * @brief Check if the PartitionRegion is empty (meaning there is no constraint on the object the PartitionRegion belongs to)
     */
    bool empty();

    /**
     * @brief Check if the given location is within the legal bounds of the  PartitionRegion.
     * The location provided is assumed to be valid.
     *
     *   @param loc       The location to be checked
     */
    bool is_loc_in_part_reg(t_pl_loc loc);

    /**
     * @brief Global friend function that returns the intersection of two PartitionRegions
     *
     *   @param pr1     One of the PartitionRegions to be intersected
     *   @param pr2     One of the PartitionRegions to be intersected
     */
    friend PartitionRegion intersection(PartitionRegion& pr1, PartitionRegion& pr2);

  private:
    std::vector<Region> partition_region; ///< union of rectangular regions that a partition can be placed in
};

///@brief used to print data from a PartitionRegion
void print_partition_region(FILE* fp, PartitionRegion pr);

#endif /* PARTITION_REGIONS_H */
