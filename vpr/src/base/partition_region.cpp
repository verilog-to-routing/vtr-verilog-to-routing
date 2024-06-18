#include "partition_region.h"
#include "region.h"

#include <utility>

void PartitionRegion::add_to_part_region(Region region) {
    regions.push_back(region);
}

const std::vector<Region>& PartitionRegion::get_regions() const {
    return regions;
}

std::vector<Region>& PartitionRegion::get_mutable_regions() {
    return regions;
}

void PartitionRegion::set_partition_region(std::vector<Region> pr) {
    regions = std::move(pr);
}

bool PartitionRegion::empty() const {
    return regions.empty();
}

bool PartitionRegion::is_loc_in_part_reg(const t_pl_loc& loc) const {
    bool is_in_pr = false;

    for (const auto& region : regions) {
        is_in_pr = region.is_loc_in_reg(loc);
        if (is_in_pr) {
            break;
        }
    }

    return is_in_pr;
}

PartitionRegion intersection(const PartitionRegion& cluster_pr, const PartitionRegion& new_pr) {
    /**for N regions in part_region and M in the calling object you can get anywhere from
     * 0 to M*N regions in the resulting vector. Only intersection regions with non-zero area rectangles and
     * equivalent subtiles are put in the resulting vector
     * Rectangles are not merged even if it would be possible
     */
    PartitionRegion pr;
    auto& pr_regions = pr.get_mutable_regions();

    for (const auto& cluster_region : cluster_pr.get_regions()) {
        for (const auto& new_region : new_pr.get_regions()) {
            Region intersect_region = intersection(cluster_region, new_region);
            if (!intersect_region.empty()) {
                pr_regions.push_back(intersect_region);
            }
        }
    }

    return pr;
}

void update_cluster_part_reg(PartitionRegion& cluster_pr, const PartitionRegion& new_pr) {
    std::vector<Region> int_regions;

    // now that we know PartitionRegions are compatible, look for overlapping regions
    for (const auto& cluster_region : cluster_pr.get_regions()) {
        for (const auto& new_region : new_pr.get_regions()) {
            Region intersect_region = intersection(cluster_region, new_region);
            if (!intersect_region.empty()) {
                int_regions.push_back(intersect_region);
            }
        }
    }

    cluster_pr.set_partition_region(int_regions);
}

void print_partition_region(FILE* fp, const PartitionRegion& pr) {
    const std::vector<Region>& regions = pr.get_regions();

    int pr_size = regions.size();

    fprintf(fp, "\tNumber of regions in partition is: %d\n", pr_size);

    for (const auto& region : regions) {
        print_region(fp, region);
    }
}
