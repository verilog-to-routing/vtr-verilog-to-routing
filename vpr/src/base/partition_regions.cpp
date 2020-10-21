#include "partition_regions.h"

PartitionRegions PartitionRegions::get_intersection(PartitionRegions part_region) {
    /**for N regions in part_region and M in the calling object you can get anywhere from
     * 0 to M*N regions in the resulting vector. Only intersection regions with non-zero area rectangles and
     * equivalent subtiles are put in the resulting vector
     * Rectangles are not merged even if it would be possible
     */
    PartitionRegions pr;
    Region intersect_region;
    bool regions_intersect;
    for (unsigned int i = 0; i < partition_regions.size(); i++) {
        for (unsigned int j = 0; j < part_region.partition_regions.size(); j++) {
            regions_intersect = partition_regions[i].do_regions_intersect(part_region.partition_regions[j]);
            if (regions_intersect) {
                intersect_region = partition_regions[i].regions_intersection(part_region.partition_regions[j]);
                pr.partition_regions.push_back(intersect_region);
            }
        }
    }

    return pr;
}

void PartitionRegions::add_to_part_regions(Region region) {
    partition_regions.push_back(region);
}

std::vector<Region> PartitionRegions::get_partition_regions() {
    return partition_regions;
}
