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
    std::vector<Region>& get_mutable_regions();

    /**
     * @brief Return the union of regions
     */
    const std::vector<Region>& get_regions() const;

    /**
     * @brief Set the union of regions
     */
    void set_partition_region(std::vector<Region> pr);

    /**
     * @brief Check if the PartitionRegion is empty (meaning there is no constraint on the object the PartitionRegion belongs to)
     */
    bool empty() const;

    /**
     * @brief Check if the given location is within the legal bounds of the  PartitionRegion.
     * The location provided is assumed to be valid.
     *
     *   @param loc       The location to be checked
     */
    bool is_loc_in_part_reg(const t_pl_loc& loc) const;

    /**
     * @brief Compares whether this PartitionRegion includes the same regions
     * and another PartitionRegion.
     * @param pr The other PartitionRegion to be compared with this object.
     * @return True if both PartitionRegions contain the same regions.
     */
    bool operator==(const PartitionRegion& pr) const {
        return (regions == pr.get_regions());
    }

  private:
    std::vector<Region> regions; ///< union of rectangular regions that a partition can be placed in
};

///@brief used to print data from a PartitionRegion
void print_partition_region(FILE* fp, const PartitionRegion& pr);

/**
 * @brief Global function that returns the intersection of two PartitionRegions
 *
 *   @param cluster_pr     One of the PartitionRegions to be intersected
 *   @param new_pr         One of the PartitionRegions to be intersected
 */
PartitionRegion intersection(const PartitionRegion& cluster_pr, const PartitionRegion& new_pr);

/**
 * @brief Global function that updates the PartitionRegion of a cluster with the intersection
 *        of the cluster PartitionRegion and a new PartitionRegion
 *
 *   @param cluster_pr     The cluster PartitionRegion that is to be updated
 *   @param new_pr         The new PartitionRegion that the cluster PartitionRegion will be intersected with
 */
void update_cluster_part_reg(PartitionRegion& cluster_pr, const PartitionRegion& new_pr);

/**
 * @brief Get a PartitionRegion with a single Region that covers the entire device.
 *
 * @return A PartitionRegion that covers the whole device grid.
 */
const PartitionRegion& get_device_partition_region();

namespace std {
template<>
struct hash<PartitionRegion> {
    std::size_t operator()(const PartitionRegion& pr) const noexcept {
        const std::vector<Region>& regions = pr.get_regions();

        std::size_t seed = std::hash<size_t>{}(regions.size());

        for (const Region& region : regions) {
            vtr::hash_combine(seed, region);
        }

        return seed;
    }
};
} // namespace std


#endif /* PARTITION_REGIONS_H */
