#include "partition_region.h"

PartitionRegion PartitionRegion::get_intersection(PartitionRegion part_region) {
    /**for N regions in part_region and M in the calling object you can get anywhere from
     * 0 to M*N regions in the resulting vector. Only intersection regions with non-zero area rectangles and
     * equivalent subtiles are put in the resulting vector
     * Rectangles are not merged even if it would be possible
     */
    PartitionRegion pr;
    Region intersect_region;
    bool regions_intersect;
    for (unsigned int i = 0; i < partition_region.size(); i++) {
        for (unsigned int j = 0; j < part_region.partition_region.size(); j++) {
            regions_intersect = partition_region[i].do_regions_intersect(part_region.partition_region[j]);
            if (regions_intersect) {
                intersect_region = partition_region[i].regions_intersection(part_region.partition_region[j]);
                pr.partition_region.push_back(intersect_region);
            }
        }
    }

    return pr;
}

void PartitionRegion::add_to_part_region(Region region) {
    partition_region.push_back(region);
}

std::vector<Region> PartitionRegion::get_partition_region() {
    return partition_region;
}
