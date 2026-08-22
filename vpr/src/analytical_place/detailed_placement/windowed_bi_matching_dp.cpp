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
#include "place_and_route.h"
#include "physical_types_util.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "verify_placement.h"
#include "place_constraints.h"
#include "compressed_grid.h"
#include "place_macro.h"

/**
 * @brief Creates a placement location from grid coordinates.
 */
static t_pl_loc make_pl_loc(int x, int y, int sub_tile, int layer);

/**
 * @brief Returns true if the physical tile at the given grid location is not empty.
 */
static bool is_non_empty_physical_tile(const DeviceGrid& grid,
                                       const t_physical_tile_loc& loc) {
    t_physical_tile_type_ptr physical_type = grid.get_physical_type(loc);
    if (!physical_type) {
        return false;
    }
    if (physical_type->is_empty()) {
        return false;
    }
    return true;
}

/**
 * @brief Collects legal placement locations near a block in compressed-grid space.
 */
static std::vector<t_pl_loc> collect_compressed_window_locations(
    const t_compressed_block_grid& compressed_grid,
    const DeviceGrid& grid,
    t_pl_loc center_loc,
    int compressed_window_radius) {
    std::vector<t_pl_loc> candidate_locs;

    t_physical_tile_loc center_physical_loc{
        center_loc.x,
        center_loc.y,
        center_loc.layer};

    t_physical_tile_loc center_compressed_loc =
        compressed_grid.grid_loc_to_compressed_loc_approx(center_physical_loc);

    if (center_compressed_loc.x == UNDEFINED || center_compressed_loc.y == UNDEFINED) {
        return candidate_locs;
    }

    const int layer_num = center_compressed_loc.layer_num;

    const int num_columns = static_cast<int>(compressed_grid.get_num_columns(layer_num));
    const int num_rows = static_cast<int>(compressed_grid.get_num_rows(layer_num));

    if (num_columns == 0 || num_rows == 0) {
        return candidate_locs;
    }

    const int min_cx = std::max(0, center_compressed_loc.x - compressed_window_radius);
    const int max_cx = std::min(num_columns - 1, center_compressed_loc.x + compressed_window_radius);
    const int min_cy = std::max(0, center_compressed_loc.y - compressed_window_radius);
    const int max_cy = std::min(num_rows - 1, center_compressed_loc.y + compressed_window_radius);

    for (int cx = min_cx; cx <= max_cx; cx++) {
        const auto& column_block_map = compressed_grid.get_column_block_map(cx, layer_num);

        for (const auto& [cy, physical_loc] : column_block_map) {
            if (cy < min_cy || cy > max_cy) {
                continue;
            }

            t_physical_tile_type_ptr physical_type = grid.get_physical_type(physical_loc);
            if (!physical_type || physical_type->is_empty()) {
                continue;
            }

            auto compatible_sub_tile_iter =
                compressed_grid.compatible_sub_tiles_for_tile.find(physical_type->index);

            if (compatible_sub_tile_iter == compressed_grid.compatible_sub_tiles_for_tile.end()) {
                continue;
            }

            for (int sub_tile : compatible_sub_tile_iter->second) {
                candidate_locs.push_back(
                    make_pl_loc(physical_loc.x,
                                physical_loc.y,
                                sub_tile,
                                physical_loc.layer_num));
            }
        }
    }

    return candidate_locs;
}

/**
 * @brief Returns true if the physical tile location is the root of a tile.
 */
static bool is_root_physical_tile(const DeviceGrid& grid,
                                  const t_physical_tile_loc& loc) {
    return grid.get_width_offset(loc) == 0 && grid.get_height_offset(loc) == 0;
}

/**
 * @brief Returns true if target placement location can legally hold the block.
 */
static bool location_can_hold_block(const DeviceGrid& grid,
                                    const ClusteredNetlist& clb_nlist,
                                    ClusterBlockId block_id,
                                    t_pl_loc loc) {
    t_physical_tile_loc physical_loc{loc.x, loc.y, loc.layer};
    t_physical_tile_type_ptr physical_type =
        grid.get_physical_type({loc.x, loc.y, loc.layer});

    if (!physical_type) {
        return false;
    }

    if (physical_type->is_empty()) {
        return false;
    }

    if (!is_root_physical_tile(grid, physical_loc)) {
        return false;
    }

    // Currently: only handle 1x1 physical tiles
    if (physical_type->width != 1 || physical_type->height != 1) {
        return false;
    }

    if (loc.sub_tile < 0 || loc.sub_tile >= physical_type->capacity) {
        return false;
    }

    t_logical_block_type_ptr logical_type = clb_nlist.block_type(block_id);

    return is_sub_tile_compatible(physical_type, logical_type, loc.sub_tile);
}

/**
 * @brief Returns true if block belongs to a placement macro.
 */
static bool block_is_in_placement_macro(ClusterBlockId block_id) {
    const auto& place_macros_ptr = g_vpr_ctx.placement().place_macros;

    if (!place_macros_ptr) {
        return false;
    }

    const PlaceMacros& place_macros = *place_macros_ptr;
    for (const t_pl_macro& pl_macro : place_macros.macros()) {
        for (const t_pl_macro_member& member : pl_macro.members) {
            if (member.blk_index == block_id) {
                return true;
            }
        }
    }

    return false;
}

WindowedBiMatchingDetailedPlacer::WindowedBiMatchingDetailedPlacer(
    const BlkLocRegistry& curr_clustered_placement,
    const t_placer_opts& placer_opts)
    : placer_state_(false)
    , net_cost_handler_(placer_state_,
                        g_vpr_ctx.placement().cube_bb,
                        e_place_algorithm::BOUNDING_BOX_PLACE,
                        placer_opts.congestion_chan_util_threshold) {
    BlkLocRegistry& blk_loc_registry = placer_state_.mutable_blk_loc_registry();
    blk_loc_registry = curr_clustered_placement;

    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    for (ClusterBlockId block_id : clb_nlist.blocks()) {
        blk_loc_registry.place_sync_external_block_connections(block_id);
    }
    // PlacerState(false) for now because timing cost updates are not needed
    (void)net_cost_handler_.comp_bb_cong_cost(e_cost_methods::NORMAL);
}

static t_pl_loc make_pl_loc(int x, int y, int sub_tile, int layer) {
    t_pl_loc loc;
    loc.x = x;
    loc.y = y;
    loc.sub_tile = sub_tile;
    loc.layer = layer;
    return loc;
}

bool WindowedBiMatchingDetailedPlacer::try_swap_locations(BlkLocRegistry& blk_loc_registry,
                                                          const DeviceGrid& grid,
                                                          t_pl_loc loc_a,
                                                          t_pl_loc loc_b) {
    // Only consider real placeable physical tiles. still allows empty
    // placement locations, but skips architecture EMPTY tiles.
    if (!is_non_empty_physical_tile(grid, {loc_a.x, loc_a.y, loc_a.layer})
        || !is_non_empty_physical_tile(grid, {loc_b.x, loc_b.y, loc_b.layer})) {
        return false;
    }

    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();
    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;

    ClusterBlockId block_a = grid_blocks.block_at_location(loc_a);
    ClusterBlockId block_b = grid_blocks.block_at_location(loc_b);

    if (block_a == ClusterBlockId::INVALID() && block_b == ClusterBlockId::INVALID()) {
        return false;
    }

    // Skipping placement macro members for now.
    if (block_a != ClusterBlockId::INVALID() && block_is_in_placement_macro(block_a)) {
        return false;
    }

    if (block_b != ClusterBlockId::INVALID() && block_is_in_placement_macro(block_b)) {
        return false;
    }

    // CUrrently: only swap two occupied locations if the blocks have the same logical type.
    if (block_a != ClusterBlockId::INVALID()
        && block_b != ClusterBlockId::INVALID()
        && clb_nlist.block_type(block_a) != clb_nlist.block_type(block_b)) {
        return false;
    }

    if (block_a != ClusterBlockId::INVALID()
        && !location_can_hold_block(grid, clb_nlist, block_a, loc_b)) {
        return false;
    }

    if (block_b != ClusterBlockId::INVALID()
        && !location_can_hold_block(grid, clb_nlist, block_b, loc_a)) {
        return false;
    }

    t_pl_blocks_to_be_moved blocks_affected(2);

    // Move block A to location B if location A contains a block.
    if (block_a != ClusterBlockId::INVALID()) {
        e_block_move_result result_a =
            blocks_affected.record_block_move(block_a, loc_b, blk_loc_registry);

        if (result_a != e_block_move_result::VALID) {
            blocks_affected.clear_move_blocks();
            return false;
        }
    }

    // Move block B to location A if location B contains a block.
    if (block_b != ClusterBlockId::INVALID()) {
        e_block_move_result result_b =
            blocks_affected.record_block_move(block_b, loc_a, blk_loc_registry);

        if (result_b != e_block_move_result::VALID) {
            blocks_affected.clear_move_blocks();
            return false;
        }
    }

    // Reject moves that violate floorplan constraints.
    if (!floorplan_legal(blocks_affected)) {
        blocks_affected.clear_move_blocks();
        return false;
    }

    // Temporarily apply  move so bounding-box cost delta can be evaluated.
    blk_loc_registry.apply_move_blocks(blocks_affected);

    t_net_cost_terms cost_terms_delta;
    double timing_delta_cost = 0.;

    net_cost_handler_.find_affected_nets_and_update_costs(
        nullptr,
        nullptr,
        blocks_affected,
        cost_terms_delta,
        timing_delta_cost);

    // Commit only moves/swaps that reduce cost.
    if (cost_terms_delta.bb_cost < 0.) {
        net_cost_handler_.update_move_nets();
        blk_loc_registry.commit_move_blocks(blocks_affected);
        blocks_affected.clear_move_blocks();
        return true;
    }

    // Otherwise, undo cost update and temporary placement move.
    net_cost_handler_.reset_move_nets();
    blk_loc_registry.revert_move_blocks(blocks_affected);
    blocks_affected.clear_move_blocks();
    return false;
}

void WindowedBiMatchingDetailedPlacer::optimize_placement() {
    vtr::ScopedStartFinishTimer timer("Windowed Bipartite Matching Detailed Placer");
    VTR_LOG("Running Windowed Bipartite Matching detailed placer.\n");

    g_vpr_ctx.mutable_placement().lock_loc_vars();

    BlkLocRegistry& blk_loc_registry =
        placer_state_.mutable_blk_loc_registry();

    const DeviceGrid& grid = g_vpr_ctx.device().grid;

    const double initial_bb_wirelength_estimate =
        net_cost_handler_.get_total_wirelength_estimate();

    int num_committed_moves = 0;

    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    const auto& compressed_block_grids = g_vpr_ctx.placement().compressed_block_grids;

    for (ClusterBlockId block_id : clb_nlist.blocks()) {
        if (block_is_in_placement_macro(block_id)) {
            continue;
        }

        t_pl_loc current_loc = blk_loc_registry.block_locs()[block_id].loc;
        t_logical_block_type_ptr logical_type = clb_nlist.block_type(block_id);

        const t_compressed_block_grid& compressed_grid =
            compressed_block_grids[logical_type->index];

        std::vector<t_pl_loc> candidate_locs =
            collect_compressed_window_locations(compressed_grid,
                                                grid,
                                                current_loc,
                                                compressed_window_radius_);

        for (t_pl_loc target_loc : candidate_locs) {
            if (target_loc.x == current_loc.x
                && target_loc.y == current_loc.y
                && target_loc.sub_tile == current_loc.sub_tile
                && target_loc.layer == current_loc.layer) {
                continue;
            }

            if (!try_swap_locations(blk_loc_registry, grid, current_loc, target_loc)) {
                continue;
            }

            num_committed_moves++;

            // block may have moved, so update its current location.
            current_loc = blk_loc_registry.block_locs()[block_id].loc;
        }
    }

    const double final_bb_wirelength_estimate =
        net_cost_handler_.get_total_wirelength_estimate();

    auto& placement_ctx = g_vpr_ctx.mutable_placement();
    placement_ctx.unlock_loc_vars();
    placement_ctx.mutable_blk_loc_registry() = placer_state_.blk_loc_registry();
    if (num_committed_moves > 0) {
        post_place_sync();
    }
    // Verify that the placement remains valid after the detailed-placement swaps.
    unsigned num_placement_errors = verify_placement(g_vpr_ctx);
    if (num_placement_errors == 0) {
        VTR_LOG("Completed placement consistency check successfully.\n");
    } else {
        VPR_ERROR(VPR_ERROR_PLACE,
                  "Completed placement consistency check, %u errors found.\n"
                  "Aborting program.\n",
                  num_placement_errors);
    }
    VTR_LOG("Windowed BiMatching DP stats");
    VTR_LOG("--Initial wirelength estimate: %g\n",
            initial_bb_wirelength_estimate);
    VTR_LOG("--Final wirelength estimate: %g\n",
            final_bb_wirelength_estimate);
    VTR_LOG("--BB wirelength estimate delta: %g\n",
            final_bb_wirelength_estimate - initial_bb_wirelength_estimate);
    VTR_LOG("--Location moves/swaps committed: %d\n", num_committed_moves);
}
