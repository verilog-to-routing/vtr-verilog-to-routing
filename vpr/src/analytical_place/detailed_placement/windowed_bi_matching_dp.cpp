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
#include <algorithm>
#include <random>

// Variable to alter (square) window size
static constexpr int window_size = 2;

/**
 * @brief Creates a placement location from grid coordinates.
 */
static t_pl_loc make_pl_loc(int x, int y, int sub_tile, int layer);

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

    // TODO: Relax temporary 1x1 restriction after adding support for
    // multi-tile physical types
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
    return place_macros_ptr->get_imacro_from_iblk(block_id) != UNDEFINED;
}

static void collect_window_blocks(const t_compressed_block_grid& compressed_grid,
                                  const DeviceGrid& grid,
                                  const GridBlock& grid_blocks,
                                  const ClusteredNetlist& clb_nlist,
                                  t_logical_block_type_ptr logical_type,
                                  int start_cx,
                                  int start_cy,
                                  int layer_num,
                                  std::vector<ClusterBlockId>& blocks,
                                  std::vector<t_pl_loc>& locs) {
    blocks.clear();
    locs.clear();

    // each of the 3 compressed grid columns
    for (int cx = start_cx; cx < start_cx + window_size; cx++) {
        const auto& column_block_map =
            compressed_grid.get_column_block_map(cx, layer_num);

        // now iterating over every row in the column
        for (const auto& [cy, physical_loc] : column_block_map) {
            if (cy < start_cy || cy >= start_cy + window_size) {
                continue;
            }

            // inspect FPGA tile once we are in the correct location

            t_physical_tile_type_ptr physical_type =
                grid.get_physical_type(physical_loc);

            if (!physical_type || physical_type->is_empty()) {
                continue;
            }

            if (physical_type->is_io()) {
                continue;
            }

            // find compatible subtiles
            auto compatible_sub_tile_iter =
                compressed_grid.compatible_sub_tiles_for_tile.find(physical_type->index);
            if (compatible_sub_tile_iter == compressed_grid.compatible_sub_tiles_for_tile.end()) {
                continue;
            }

            // create full placement location for each compatible subtile
            for (int sub_tile : compatible_sub_tile_iter->second) {
                t_pl_loc loc = make_pl_loc(physical_loc.x,
                                           physical_loc.y,
                                           sub_tile,
                                           physical_loc.layer_num);

                ClusterBlockId block_id = grid_blocks.block_at_location(loc);

                if (block_id == ClusterBlockId::INVALID()) {
                    continue;
                }
                if (clb_nlist.block_type(block_id) != logical_type) {
                    continue;
                }
                if (block_is_in_placement_macro(block_id)) {
                    continue;
                }
                if (!location_can_hold_block(grid, clb_nlist, block_id, loc)) {
                    continue;
                }

                blocks.push_back(block_id);
                locs.push_back(loc);
            }
        }
    }
}

WindowedBiMatchingDetailedPlacer::WindowedBiMatchingDetailedPlacer(
    const BlkLocRegistry& curr_clustered_placement,
    const t_placer_opts& placer_opts)
    : placer_state_(/*placement_is_timing_driven=*/false, /*placement_uses_setup_slacks=*/false)
    , net_cost_handler_(placer_state_,
                        e_place_algorithm::BOUNDING_BOX_PLACE,
                        placer_opts.congestion_chan_util_threshold) {
    BlkLocRegistry& blk_loc_registry = placer_state_.mutable_blk_loc_registry();
    blk_loc_registry = curr_clustered_placement;

    const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
    for (ClusterBlockId block_id : clb_nlist.blocks()) {
        blk_loc_registry.place_sync_external_block_connections(block_id);
    }
    // PlacerState is not timing driven for now because timing cost updates are not needed
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

bool WindowedBiMatchingDetailedPlacer::exhaustive_window(
    BlkLocRegistry& blk_loc_registry,
    const std::vector<ClusterBlockId>& blocks,
    const std::vector<t_pl_loc>& locs) {
    //nothing to arrange
    if (blocks.size() <= 1) {
        return false;
    }

    if (blocks.size() != locs.size()) {
        return false;
    }

    std::vector<ClusterBlockId> shuffled_blocks = blocks;
    // shuffle block ordering once per window
    static std::mt19937 rng(32768);
    std::shuffle(shuffled_blocks.begin(), shuffled_blocks.end(), rng);

    // list of all locations within window
    std::vector<int> perm;
    for (size_t i = 0; i < blocks.size(); i++) {
        perm.push_back(static_cast<int>(i));
    }

    // track best result
    double best_delta = 0;
    std::vector<int> best_perm = perm;

    // capping number of permutations explored
    size_t num_perms_explored = 0;
    // 2^19
    const size_t max_perms = 524288;

    bool more_perms = true;
    while (more_perms) {
        // checking if cap has been reached
        if (num_perms_explored >= max_perms) {
            VTR_LOG_WARN("Permutation cap reached: explored=%zu cap=%zu window_blocks=%zu\n",
                         num_perms_explored,
                         max_perms,
                         blocks.size());
            break;
        }
        num_perms_explored++;

        // exploring possible moves

        //TODO: with empty locations?
        t_pl_blocks_to_be_moved blocks_affected(blocks.size());
        bool valid_perm = true;

        for (size_t i = 0; i < blocks.size(); i++) {
            ClusterBlockId block_id = shuffled_blocks[i];
            t_pl_loc target_loc = locs[perm[i]];
            //record proposed moves
            e_block_move_result result =
                blocks_affected.record_block_move(block_id,
                                                  target_loc,
                                                  blk_loc_registry);

            if (result != e_block_move_result::VALID) {
                valid_perm = false;
                break;
            }
        }

        //i f invalid, going to next permutation
        if (!valid_perm) {
            blocks_affected.clear_move_blocks();
            more_perms = std::next_permutation(perm.begin(), perm.end());
            continue;
        }
        if (!floorplan_legal(blocks_affected)) {
            blocks_affected.clear_move_blocks();
            more_perms = std::next_permutation(perm.begin(), perm.end());
            continue;
        }

        // temporariliy apply move
        blk_loc_registry.apply_move_blocks(blocks_affected);

        t_net_cost_terms cost_terms_delta;
        double timing_delta_cost = 0.;

        net_cost_handler_.find_affected_nets_and_update_costs(
            nullptr,
            nullptr,
            blocks_affected,
            cost_terms_delta,
            timing_delta_cost);

        // updating if we found a new best permutation
        if (cost_terms_delta.bb_cost < best_delta) {
            best_delta = cost_terms_delta.bb_cost;
            best_perm = perm;
        }

        net_cost_handler_.reset_move_nets();
        blk_loc_registry.revert_move_blocks(blocks_affected);
        blocks_affected.clear_move_blocks();

        more_perms = std::next_permutation(perm.begin(), perm.end());
    }

    // if no wirelength decrease, can just return
    if (best_delta >= 0.) {
        return false;
    }

    // applying best permutation found in window
    t_pl_blocks_to_be_moved blocks_affected(blocks.size());
    for (size_t i = 0; i < blocks.size(); i++) {
        e_block_move_result result =
            blocks_affected.record_block_move(shuffled_blocks[i],
                                              locs[best_perm[i]],
                                              blk_loc_registry);

        if (result != e_block_move_result::VALID) {
            blocks_affected.clear_move_blocks();
            return false;
        }
    }

    if (!floorplan_legal(blocks_affected)) {
        blocks_affected.clear_move_blocks();
        return false;
    }
    blk_loc_registry.apply_move_blocks(blocks_affected);

    t_net_cost_terms cost_terms_delta;
    double timing_delta_cost = 0.;
    net_cost_handler_.find_affected_nets_and_update_costs(
        nullptr,
        nullptr,
        blocks_affected,
        cost_terms_delta,
        timing_delta_cost);

    //update cost
    net_cost_handler_.update_move_nets();
    blk_loc_registry.commit_move_blocks(blocks_affected);
    blocks_affected.clear_move_blocks();

    return true;
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

    const GridBlock& grid_blocks = blk_loc_registry.grid_blocks();

    // building a unique list of block types
    std::vector<t_logical_block_type_ptr> logical_types;
    std::vector<int> seen_type_indices;

    for (ClusterBlockId block_id : clb_nlist.blocks()) {
        t_logical_block_type_ptr logical_type = clb_nlist.block_type(block_id);

        if (std::find(seen_type_indices.begin(),
                      seen_type_indices.end(),
                      logical_type->index)
            != seen_type_indices.end()) {
            continue;
        }

        seen_type_indices.push_back(logical_type->index);
        logical_types.push_back(logical_type);
    }

    int num_windows_seen = 0;
    int num_windows_with_blocks = 0;
    size_t max_window_size = 0;

    // doing the actual window sweep
    for (t_logical_block_type_ptr logical_type : logical_types) {
        const t_compressed_block_grid& compressed_grid =
            compressed_block_grids[logical_type->index];

        const int layer_num = placement_layer_;

        const int num_columns =
            static_cast<int>(compressed_grid.get_num_columns(layer_num));
        const int num_rows =
            static_cast<int>(compressed_grid.get_num_rows(layer_num));

        if (num_columns < window_size || num_rows < window_size) {
            continue;
        }

        // sliding window
        for (int start_cx = 0; start_cx + window_size <= num_columns; start_cx++) {
            for (int start_cy = 0; start_cy + window_size <= num_rows; start_cy++) {
                std::vector<ClusterBlockId> blocks;
                std::vector<t_pl_loc> locs;

                num_windows_seen++;

                collect_window_blocks(compressed_grid,
                                      grid,
                                      grid_blocks,
                                      clb_nlist,
                                      logical_type,
                                      start_cx,
                                      start_cy,
                                      layer_num,
                                      blocks,
                                      locs);

                if (blocks.size() > max_window_size) {
                    max_window_size = blocks.size();

                    VTR_LOG("new max window size: %zu \n",
                            max_window_size);
                }

                if (blocks.size() <= 1) {
                    continue;
                }

                num_windows_with_blocks++;

                if (exhaustive_window(blk_loc_registry, blocks, locs)) {
                    num_committed_moves++;
                }
            }
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
    VTR_LOG("Windowed BiMatching DP stats\n");
    VTR_LOG("--Initial wirelength estimate: %g\n",
            initial_bb_wirelength_estimate);
    VTR_LOG("--Final wirelength estimate: %g\n",
            final_bb_wirelength_estimate);
    VTR_LOG("--BB wirelength estimate delta: %g\n",
            final_bb_wirelength_estimate - initial_bb_wirelength_estimate);
    VTR_LOG("--Location moves/swaps committed: %d\n", num_committed_moves);
    VTR_LOG("--max window size: %zu\n", max_window_size);
    VTR_LOG("--num committed: %d\n", num_committed_moves);
}
