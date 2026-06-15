/**
 * @file
 * @author  Athavan Balakumar
 * @date    May 2026
 * @brief   Implementation of the Windowed Bipartite Matching Detailed Placer.
 */

#include "windowed_bi_matching_dp.h"
#include "blk_loc_registry.h"
#include "clustered_netlist.h"
#include "device_grid.h"
#include "globals.h"
#include "move_transactions.h"
#include "physical_types.h"
#include "place_and_route.h"
#include "vtr_log.h"
#include "vtr_time.h"

namespace {
int window_size = 2;
int placement_layer = 0;
int placement_sub_tile = 0;
/**
 * @brief Creates a placement location from grid coordinates.
 */
t_pl_loc make_pl_loc(int x, int y, int sub_tile, int layer) {
    t_pl_loc loc;
    loc.x = x;
    loc.y = y;
    loc.sub_tile = sub_tile;
    loc.layer = layer;
    return loc;
}
/**
 * @brief Returns true if  physical tile at given grid location is not empty.
 */
bool is_non_empty_physical_tile(const DeviceGrid& grid, int x, int y, int layer) {
    t_physical_tile_type_ptr physical_type = grid.get_physical_type({x, y, layer});
    if (!physical_type) {
        return false;
    }
    if (physical_type->is_empty()) {
        return false;
    }
    return true;
}

/**
 * @brief Returns true if every physical tile in the 2x2 window is not empty.
 */
bool window_has_no_empty_physical_tiles(const DeviceGrid& grid, int x, int y, int layer) {
    for (int dx = 0; dx < window_size; ++dx) {
        for (int dy = 0; dy < window_size; ++dy) {
            if (!is_non_empty_physical_tile(grid, x + dx, y + dy, layer)) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Returns true if every checked sub-tile location in the window contains a placed block.
 */
bool window_has_all_placed_blocks(const BlkLocRegistry& blk_loc_registry, int x, int y, int layer) {
    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();
    for (int dx = 0; dx < window_size; dx++) {
        for (int dy = 0; dy < window_size; dy++) {
            t_pl_loc loc = make_pl_loc(x + dx, y + dy, placement_sub_tile, layer);
            ClusterBlockId blk = grid_blocks.block_at_location(loc);
            if (blk == ClusterBlockId::INVALID()) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Returns true if two blocks can be swapped by this pass.
 */
bool blocks_are_swappable(const BlkLocRegistry& blk_loc_registry,
                          const ClusteredNetlist& clb_nlist,
                          ClusterBlockId block_a,
                          ClusterBlockId block_b) {
    if (block_a == ClusterBlockId::INVALID() || block_b == ClusterBlockId::INVALID()) {
        return false;
    }
    const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs = blk_loc_registry.block_locs();
    if (block_locs[block_a].is_fixed || block_locs[block_b].is_fixed) {
        return false;
    }
    if (clb_nlist.block_type(block_a) != clb_nlist.block_type(block_b)) {
        return false;
    }
    return true;
}

/**
 * @brief Attempts to commit a swap between two placed blocks.
 */
bool try_swap_blocks(BlkLocRegistry& blk_loc_registry,
                     ClusterBlockId block_a,
                     t_pl_loc loc_a,
                     ClusterBlockId block_b,
                     t_pl_loc loc_b) {
    t_pl_blocks_to_be_moved blocks_affected(2);

    e_block_move_result result_a =
        blocks_affected.record_block_move(block_a, loc_b, blk_loc_registry);
    if (result_a != e_block_move_result::VALID) {
        blocks_affected.clear_move_blocks();
        return false;
    }
    e_block_move_result result_b =
        blocks_affected.record_block_move(block_b, loc_a, blk_loc_registry);
    if (result_b != e_block_move_result::VALID) {
        blocks_affected.clear_move_blocks();
        return false;
    }
    blk_loc_registry.apply_move_blocks(blocks_affected);
    blk_loc_registry.commit_move_blocks(blocks_affected);
    blocks_affected.clear_move_blocks();
    return true;
}
} // namespace

void WindowedBiMatchingDetailedPlacer::optimize_placement() {
    vtr::ScopedStartFinishTimer timer("Windowed Bipartite Matching Detailed Placer");
    VTR_LOG("Running Windowed Bipartite Matching detailed placer.\n");

    BlkLocRegistry& blk_loc_registry =
        g_vpr_ctx.mutable_placement().mutable_blk_loc_registry();

    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();
    const DeviceGrid& grid = g_vpr_ctx.device().grid;
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const int grid_width = static_cast<int>(grid.width());
    const int grid_height = static_cast<int>(grid.height());

    int num_swaps = 0;

    for (int x = 0; x + window_size - 1 < grid_width; x += window_size) {
        for (int y = 0; y + window_size - 1 < grid_height; y += window_size) {
            if (!window_has_no_empty_physical_tiles(grid, x, y, placement_layer)) {
                continue;
            }
            if (!window_has_all_placed_blocks(blk_loc_registry, x, y, placement_layer)) {
                continue;
            }

            ///Temporary test: fixed swap between top left and top right block (no cost function)
            t_pl_loc left_loc = make_pl_loc(x, y, placement_sub_tile, placement_layer);
            t_pl_loc right_loc = make_pl_loc(x + 1, y, placement_sub_tile, placement_layer);

            ClusterBlockId left_blk = grid_blocks.block_at_location(left_loc);
            ClusterBlockId right_blk = grid_blocks.block_at_location(right_loc);

            if (!blocks_are_swappable(blk_loc_registry, clb_nlist, left_blk, right_blk)) {
                continue;
            }
            if (!try_swap_blocks(blk_loc_registry, left_blk, left_loc, right_blk, right_loc)) {
                continue;
            }

            num_swaps++;
        }
    }
    if (num_swaps > 0) {
        post_place_sync();
    }
    ///Verifying that swaps are actually happening
    VTR_LOG("Windowed BiMatching DP: committed %d neighbor swaps.\n", num_swaps);
}
