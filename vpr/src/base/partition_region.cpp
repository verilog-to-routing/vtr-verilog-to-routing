#include "partition_region.h"

#include "region.h"
#include "globals.h"

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

    fprintf(fp, "\tNumber of regions in partition is: %d\n", (int)regions.size());

    for (const auto& region : regions) {
        print_region(fp, region);
    }
}

const PartitionRegion& get_device_partition_region() {
    // the device partition region is initialized the first time this function is called
    static PartitionRegion device_pr;

    const auto& grid = g_vpr_ctx.device().grid;

    // this function is supposed to be called when the grid is constructed
    VTR_ASSERT_SAFE(grid.grid_size() != 0);

    const int n_layers = grid.get_num_layers();
    const int width = static_cast<int>(grid.width());
    const int height = static_cast<int>(grid.height());

    if (device_pr.empty()) {
        Region device_region(0, 0, width - 1, height - 1, 0, n_layers - 1);
        device_region.set_sub_tile(NO_SUBTILE);
        device_pr.add_to_part_region(device_region);
    }

    VTR_ASSERT_SAFE(device_pr.get_regions().size() == 1);
    const auto [xmin, ymin, xmax, ymax] = device_pr.get_regions()[0].get_rect().coordinates();
    VTR_ASSERT_SAFE(xmin == 0 && ymin == 0 && xmax == width -1 && ymax == height - 1);
    const auto [layer_low, layer_high] = device_pr.get_regions()[0].get_layer_range();
    VTR_ASSERT_SAFE(layer_low == 0 && layer_high == n_layers - 1);

    return device_pr;
}