#include "partition_region.h"
#include "region.h"

void PartitionRegion::add_to_part_region(Region region) {
    partition_region.push_back(region);
}

std::vector<Region> PartitionRegion::get_partition_region() {
    return partition_region;
}

bool PartitionRegion::empty() {
    return partition_region.size() == 0;
}

bool PartitionRegion::is_loc_in_part_reg(t_pl_loc loc) {
    bool is_in_pr = false;

    for (unsigned int i = 0; i < partition_region.size(); i++) {
        is_in_pr = partition_region[i].is_loc_in_reg(loc);
        if (is_in_pr == true) {
            break;
        }
    }

    return is_in_pr;
}

PartitionRegion intersection(PartitionRegion& pr1, PartitionRegion& pr2) {
    /**for N regions in part_region and M in the calling object you can get anywhere from
     * 0 to M*N regions in the resulting vector. Only intersection regions with non-zero area rectangles and
     * equivalent subtiles are put in the resulting vector
     * Rectangles are not merged even if it would be possible
     */
    PartitionRegion pr;
    Region intersect_region;
    bool regions_intersect;
    for (unsigned int i = 0; i < pr1.partition_region.size(); i++) {
        for (unsigned int j = 0; j < pr2.partition_region.size(); j++) {
            regions_intersect = do_regions_intersect(pr1.partition_region[i], pr2.partition_region[j]);
            if (regions_intersect) {
                intersect_region = intersection(pr1.partition_region[i], pr2.partition_region[j]);
                pr.partition_region.push_back(intersect_region);
            }
        }
    }

    return pr;
}

void print_partition_region(FILE* fp, PartitionRegion pr) {
    std::vector<Region> part_region = pr.get_partition_region();

    int pr_size = part_region.size();

    fprintf(fp, "\tNumber of regions in partition is: %d\n", pr_size);

    for (unsigned int i = 0; i < part_region.size(); i++) {
        print_region(fp, part_region[i]);
    }
}
