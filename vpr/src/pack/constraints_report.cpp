#include "constraints_report.h"

//To-do: alter this routine to check whether whole PartitionRegions are full instead of individual Regions
bool floorplan_constraints_regions_overfull() {
    GridTileLookup grid_tiles;

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    auto& block_types = device_ctx.logical_block_types;

    std::unordered_map<Region, std::vector<int>> regions_count_info;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (!is_cluster_constrained(blk_id)) {
            continue;
        }
        t_logical_block_type_ptr bt = cluster_ctx.clb_nlist.block_type(blk_id);

        PartitionRegion pr = floorplanning_ctx.cluster_constraints[blk_id];
        std::vector<Region> regions = pr.get_partition_region();

        for (unsigned int i_reg = 0; i_reg < regions.size(); i_reg++) {
            Region current_reg = regions[i_reg];

            auto got = regions_count_info.find(current_reg);

            if (got == regions_count_info.end()) {
                std::vector<int> block_type_counts(block_types.size(), 0);

                block_type_counts[bt->index]++;

                regions_count_info.insert({current_reg, block_type_counts});

            } else {
                got->second[bt->index]++;
            }
        }
    }

    bool floorplan_regions_overfull = false;

    for (auto& region_info : regions_count_info) {
        vtr::Rect<int> rect = region_info.first.get_region_rect();
        for (unsigned int j = 0; j < block_types.size(); j++) {
            int num_assigned_blocks = region_info.second[j];
            int num_tiles = 0;
            num_tiles = grid_tiles.region_tile_count(region_info.first, &block_types[j]);
            if (num_assigned_blocks > num_tiles) {
                floorplan_regions_overfull = true;
                floorplanning_ctx.overfull_regions.push_back(region_info.first);
                VTR_LOG("\n \nRegion (%d, %d) to (%d, %d) st %d \n", rect.xmin(), rect.ymin(), rect.xmax(), rect.ymax(), region_info.first.get_sub_tile());
                VTR_LOG("Assigned %d blocks of type %s, but only has %d tiles of that type\n", num_assigned_blocks, block_types[j].name, num_tiles);
            }
        }
    }

    return floorplan_regions_overfull;
}
