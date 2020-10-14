#include "partition_regions.h"

PartitionRegions PartitionRegions::get_intersection(PartitionRegions region) {
    //region is a vector of regions, part_regions is another vector of regions
    //have to intersect the regions and return their intersection
    PartitionRegions pr;
    Region intersect_region;
    bool regions_intersect;
    for (int i = 0; i < partition_regions.size(); i++) {
        for (int j = 0; j < region.partition_regions.size(); j++) {
            regions_intersect = partition_regions[i].do_regions_intersect(region.partition_regions[j]);
            if (regions_intersect) {
                intersect_region = partition_regions[i].regions_intersection(region.partition_regions[j]);
                pr.partition_regions.push_back(intersect_region);
            }
        }
    }

    return pr;
}

bool PartitionRegions::intersects(PartitionRegions region) {
    //fill in check for intersection
    bool test;
    return test;
}

PartitionRegions PartitionRegions::get_intersect(const AtomBlockId atom_id) {
    //fill in to get intersection
    PartitionRegions pr;
    return pr;
}

void PartitionRegions::add_to_part_regions(Region region) {
    partition_regions.push_back(region);
}

std::vector<Region> PartitionRegions::get_partition_regions() {
    return partition_regions;
}
