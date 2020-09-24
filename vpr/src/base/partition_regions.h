#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

/**
 * @file
 * @brief This file defines the PartitionRegions class. The PartitionRegions class is used to store the union
 * of regions that a particular partition can be placed in.
 */

class PartitionRegions {
  public:
    /**
     * Returns the intersection of two PartitionRegions vectors that are passed to it.
     */
    PartitionRegions get_intersection(PartitionRegions region);

    /**
     * A test for intersection
     * For example, test if the PartitionRegions of an atom intersect with PartitionRegions of a cluster.
     */
    bool intersects(PartitionRegions region);

    /**
     * Takes Atom Id and PartitonRegions vector and returns the intersection of the atom's PartitionRegions
     * and the net PartitionRegions of the cluster (if any exists)
     */
    PartitionRegions get_intersection(const AtomBlockId& id, PartitionRegions part_regions);

    void add_to_part_regions(Region region);

    std::vector<Region> get_partition_regions();

  private:
    std::vector<Region> partition_regions; //vector of regions that a partition can be placed in
};

#endif /* PARTITION_REGIONS_H */
