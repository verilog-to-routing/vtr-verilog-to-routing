#include "constraints_report.h"

bool floorplan_constraints_regions_overfull() {
    GridTileLookup grid_tiles;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    const std::vector<t_logical_block_type>& block_types = device_ctx.logical_block_types;

    // keep record of how many blocks of each type are assigned to each PartitionRegion
    std::unordered_map<PartitionRegion, std::vector<int>> pr_count_info;

    for (const ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (!is_cluster_constrained(blk_id)) {
            continue;
        }
        t_logical_block_type_ptr bt = cluster_ctx.clb_nlist.block_type(blk_id);

        const PartitionRegion& pr = floorplanning_ctx.cluster_constraints[blk_id];
        auto got = pr_count_info.find(pr);

        if (got == pr_count_info.end()) {
            std::vector<int> block_type_counts(block_types.size(), 0);
            block_type_counts[bt->index]++;
            pr_count_info.insert({pr, block_type_counts});
        } else {
            got->second[bt->index]++;
        }
    }

    bool floorplan_regions_overfull = false;

    for (const auto& [pr, block_type_counts] : pr_count_info) {
        const std::vector<Region>& regions = pr.get_regions();

        for (const t_logical_block_type& block_type : block_types) {
            int num_assigned_blocks = block_type_counts[block_type.index];
            int num_tiles = std::accumulate(regions.begin(), regions.end(), 0, [&grid_tiles, &block_type](int acc, const Region& reg) -> int {
                return acc + grid_tiles.region_tile_count(reg, &block_type);
            });

            if (num_assigned_blocks > num_tiles) {
                floorplan_regions_overfull = true;
                floorplanning_ctx.overfull_partition_regions.push_back(pr);
                VTR_LOG("\n\nA partition including the following regions has been assigned %d blocks of type %s, "
                        "but only has %d tiles of that type\n",
                        num_assigned_blocks, block_type.name, num_tiles);
                for (const Region& reg : regions) {
                    const vtr::Rect<int>& rect = reg.get_rect();
                    const auto [layer_low, layer_high] = reg.get_layer_range();
                    VTR_LOG("\tRegion (%d, %d, %d) to (%d, %d, %d) st %d \n",
                            rect.xmin(), rect.ymin(), layer_low,
                            rect.xmax(), rect.ymax(), layer_high,
                            reg.get_sub_tile());
                }

            }
        }
    }

    return floorplan_regions_overfull;
}
