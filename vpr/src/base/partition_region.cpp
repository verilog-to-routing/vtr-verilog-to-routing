#include "partition_region.h"
#include "region.h"

void PartitionRegion::add_to_part_region(Region region) {
    partition_region.push_back(region);
}

std::vector<Region> PartitionRegion::get_partition_region() {
    return partition_region;
}

void PartitionRegion::set_partition_region(std::vector<Region> pr) {
    partition_region = pr;
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

PartitionRegion intersection(const PartitionRegion& cluster_pr, const PartitionRegion& new_pr) {
    /**for N regions in part_region and M in the calling object you can get anywhere from
     * 0 to M*N regions in the resulting vector. Only intersection regions with non-zero area rectangles and
     * equivalent subtiles are put in the resulting vector
     * Rectangles are not merged even if it would be possible
     */
    PartitionRegion pr;
    Region intersect_region;
    for (unsigned int i = 0; i < cluster_pr.partition_region.size(); i++) {
        for (unsigned int j = 0; j < new_pr.partition_region.size(); j++) {
            intersect_region = intersection(cluster_pr.partition_region[i], new_pr.partition_region[j]);
            if (!intersect_region.empty()) {
                pr.partition_region.push_back(intersect_region);
            }
        }
    }

    return pr;
}

void update_cluster_part_reg(PartitionRegion& cluster_pr, const PartitionRegion& new_pr) {
    Region intersect_region;
    std::vector<Region> int_regions;
    for (unsigned int i = 0; i < cluster_pr.partition_region.size(); i++) {
        for (unsigned int j = 0; j < new_pr.partition_region.size(); j++) {
            intersect_region = intersection(cluster_pr.partition_region[i], new_pr.partition_region[j]);
            if (!intersect_region.empty()) {
                int_regions.push_back(intersect_region);
            }
        }
    }
    cluster_pr.set_partition_region(int_regions);
}

void print_partition_region(FILE* fp, PartitionRegion pr) {
    std::vector<Region> part_region = pr.get_partition_region();

    int pr_size = part_region.size();

    fprintf(fp, "\tNumber of regions in partition is: %d\n", pr_size);

    for (unsigned int i = 0; i < part_region.size(); i++) {
        print_region(fp, part_region[i]);
    }
}
