/*
 * constraints_report.cpp
 *
 *  Created on: Dec. 7, 2021
 *      Author: khalid88
 */

#include "constraints_report.h"

struct info {
    Region reg;
    std::vector<int> block_type_counts;
};

void check_constraints_filling() {
    //GridTileLookup class provides info needed for calculating number of tiles covered by a region
    //TO-DO: Find better way of accessing grid tiles so you don't have to initialize it in two different locations (initial placement and here)
    GridTileLookup grid_tiles;
    grid_tiles.initialize_grid_tile_matrices();

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& floorplanning_ctx = g_vpr_ctx.floorplanning();
    auto& device_ctx = g_vpr_ctx.device();

    auto& block_types = device_ctx.logical_block_types;

    std::vector<info> regions_count_info;

    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (!is_cluster_constrained(blk_id)) {
            continue;
        }
        t_logical_block_type_ptr bt = cluster_ctx.clb_nlist.block_type(blk_id);

        bool found_region = false;

        //store logical block type and region of block
        PartitionRegion pr = floorplanning_ctx.cluster_constraints[blk_id];
        std::vector<Region> regions = pr.get_partition_region();
        Region current_reg = regions[0];

        for (unsigned int k = 0; k < regions_count_info.size(); k++) {
            if (regions_equal(current_reg, regions_count_info[k].reg)) {
                found_region = true;
                regions_count_info[k].block_type_counts[bt->index]++;
            }
        }

        if (!found_region) {
            info new_region_info;
            new_region_info.reg = current_reg;

            for (unsigned int i = 0; i < block_types.size(); i++) {
                new_region_info.block_type_counts.push_back(0);
            }

            new_region_info.block_type_counts[bt->index]++;

            regions_count_info.push_back(new_region_info);
        }
    }

    VTR_LOG("Number of regions found is %d \n", regions_count_info.size());

    for (auto region_info : regions_count_info) {
        vtr::Rect<int> rect = region_info.reg.get_region_rect();
        //VTR_LOG("\n \n Region is (%d, %d) to (%d, %d) st %d \n", rect.xmin(), rect.ymin(), rect.xmax(), rect.ymax(), region_info.reg.get_sub_tile());
        for (unsigned int j = 0; j < block_types.size(); j++) {
            //VTR_LOG("Assigned %d cluster blocks of type %s \n", region_info.block_type_counts[j], block_types[j].name);
            int num_assigned_blocks = region_info.block_type_counts[j];
            int num_tiles = 0;
            num_tiles = grid_tiles.region_tile_count(region_info.reg, &block_types[j]);
            //VTR_LOG("Has %d cluster blocks of type %s \n", num_tiles, block_types[j].name);
            if (num_assigned_blocks > num_tiles) {
                VTR_LOG("\n \nRegion (%d, %d) to (%d, %d) st %d \n", rect.xmin(), rect.ymin(), rect.xmax(), rect.ymax(), region_info.reg.get_sub_tile());
                VTR_LOG("Assigned %d blocks of type %s, but only has %d tiles of that type\n", num_assigned_blocks, block_types[j].name, num_tiles);
            }
        }
    }
}
