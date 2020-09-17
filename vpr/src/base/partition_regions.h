#ifndef PARTITION_REGIONS_H
#define PARTITION_REGIONS_H

/**
 * Vectors of type PartitionRegions are used to return information on the intersection of regions
 * (ex. between a cluster and an atom).
 */

class PartitionRegions {
  public: //accessors
  private:
    std::vector<Region> partition_regions;
};

#endif /* PARTITION_REGIONS_H */
