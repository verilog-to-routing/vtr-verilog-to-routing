/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   Definitions of the independent placement verifier.
 *
 * Most of this code originally came from place.cpp and place_constraints.cpp
 *
 * By design, these methods should not use any global variables and anything
 * that it does not assume should be re-computed from scratch. This ensures that
 * the placement is actually valid.
 */

#include "verify_placement.h"
#include <vector>
#include "blk_loc_registry.h"
#include "clustered_netlist.h"
#include "device_grid.h"
#include "partition_region.h"
#include "physical_types.h"
#include "physical_types_util.h"
#include "place_macro.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vtr_log.h"
#include "vtr_vector.h"

/**
 * @brief Check the the block placement and grid blocks are consistent with each
 *        other and are both valid.
 *
 * This method checks:
 *  - The grid blocks matches the block locations.
 *  - Blocks are not in invalid grid locations.
 *  - All clusters are placed.
 *  - Clusters are only placed at the root tile location of large tiles.
 *
 *  @param blk_loc_registry     A registry containing the block locations and
 *                              the blocks at each grid location.
 *  @param clb_nlist            The clustered netlist being verified.
 *  @param device_grid          The device grid being verified over.
 *
 *  @return The number of errors in the block placement.
 */
static unsigned check_block_placement_consistency(const BlkLocRegistry& blk_loc_registry,
                                                  const ClusteredNetlist& clb_nlist,
                                                  const DeviceGrid& device_grid) {
    const auto& block_locs = blk_loc_registry.block_locs();
    const auto& grid_blocks = blk_loc_registry.grid_blocks();

    unsigned num_errors = 0;

    vtr::vector<ClusterBlockId, int> bdone(clb_nlist.blocks().size(), 0);

    /* Step through device grid and placement. Check it against blocks */
    for (int layer_num = 0; layer_num < (int)device_grid.get_num_layers(); layer_num++) {
        for (int i = 0; i < (int)device_grid.width(); i++) {
            for (int j = 0; j < (int)device_grid.height(); j++) {
                const t_physical_tile_loc tile_loc(i, j, layer_num);
                const auto& type = device_grid.get_physical_type(tile_loc);

                // If this is not a root tile block, ensure that its usage is 0
                // and that it has no valid clusters placed at this location.
                // TODO: Eventually it should be made impossible to place blocks
                //       at these locations.
                if (device_grid.get_width_offset(tile_loc) != 0 ||
                    device_grid.get_height_offset(tile_loc) != 0) {
                    // Usage must be 0
                    if (grid_blocks.get_usage(tile_loc) != 0) {
                        VTR_LOG_ERROR(
                            "%d blocks were placed at non-root tile location "
                            "(%d, %d, %d), but no blocks should be placed here.\n",
                            grid_blocks.get_usage(tile_loc), i, j, layer_num);
                        num_errors++;
                    }
                    // Check that all clusters at this tile location are invalid.
                    for (int k = 0; k < type->capacity; k++) {
                        ClusterBlockId bnum = grid_blocks.block_at_location({i, j, k, layer_num});
                        if (bnum.is_valid()) {
                            VTR_LOG_ERROR(
                                "Block %zu was placed at non-root tile location "
                                "(%d, %d, %d), but no blocks should be placed "
                                "here.\n",
                                size_t(bnum), i, j, layer_num);
                            num_errors++;
                        }
                    }
                }

                if (grid_blocks.get_usage(tile_loc) > type->capacity) {
                    VTR_LOG_ERROR(
                        "%d blocks were placed at grid location (%d,%d,%d), but location capacity is %d.\n",
                        grid_blocks.get_usage(tile_loc), i, j, layer_num, type->capacity);
                    num_errors++;
                }

                int usage_check = 0;
                for (int k = 0; k < type->capacity; k++) {
                    ClusterBlockId bnum = grid_blocks.block_at_location({i, j, k, layer_num});
                    if (bnum == ClusterBlockId::INVALID()) {
                        continue;
                    }

                    auto logical_block = clb_nlist.block_type(bnum);
                    auto physical_tile = type;
                    t_pl_loc block_loc = block_locs[bnum].loc;

                    if (physical_tile_type(block_loc) != physical_tile) {
                        VTR_LOG_ERROR(
                            "Block %zu type (%s) does not match grid location (%zu,%zu, %d) type (%s).\n",
                            size_t(bnum), logical_block->name.c_str(), i, j, layer_num, physical_tile->name.c_str());
                        num_errors++;
                    }

                    auto& loc = block_locs[bnum].loc;
                    if (loc.x != i || loc.y != j || loc.layer != layer_num
                        || !is_sub_tile_compatible(physical_tile, logical_block,
                                                   loc.sub_tile)) {
                        VTR_LOG_ERROR(
                            "Block %zu's location is (%d,%d,%d,%d) but found in grid at (%d,%d,%d,%d).\n",
                            size_t(bnum),
                            loc.x,
                            loc.y,
                            loc.sub_tile,
                            loc.layer,
                            i,
                            j,
                            k,
                            layer_num);
                        num_errors++;
                    }
                    ++usage_check;
                    bdone[bnum]++;
                }
                if (usage_check != grid_blocks.get_usage(tile_loc)) {
                    VTR_LOG_ERROR(
                        "%d block(s) were placed at location (%d,%d,%d), but location contains %d block(s).\n",
                        grid_blocks.get_usage(tile_loc),
                        tile_loc.x,
                        tile_loc.y,
                        tile_loc.layer_num,
                        usage_check);
                    num_errors++;
                }
            }
        }
    }

    /* Check that every block exists in the device_ctx.grid and cluster_ctx.blocks arrays somewhere. */
    for (ClusterBlockId blk_id : clb_nlist.blocks())
        if (bdone[blk_id] != 1) {
            VTR_LOG_ERROR("Block %zu listed %d times in device context grid.\n",
                          size_t(blk_id), bdone[blk_id]);
            num_errors++;
        }

    return num_errors;
}

/**
 * @brief Check that the macro placement is consistent
 *
 * This checks that the pl_macro placement is legal by making sure that blocks
 * are in the proper relative positions.
 *
 * It is assumed in this method that the macros were initialized properly.
 * FIXME: This needs to be verified somewhere in the flow.
 *
 *  @param blk_loc_registry     A registry containing the block locations and
 *                              the blocks at each grid location.
 *
 *  @return The number of errors in the macro placement.
 */
static unsigned check_macro_placement_consistency(const BlkLocRegistry& blk_loc_registry,
                                                  const PlaceMacros& pl_macros) {
    const auto& block_locs = blk_loc_registry.block_locs();
    const auto& grid_blocks = blk_loc_registry.grid_blocks();

    unsigned num_errors = 0;

    /* Check the pl_macro placement are legal - blocks are in the proper relative position. */
    for (size_t imacro = 0; imacro < pl_macros.macros().size(); imacro++) {
        auto head_iblk = pl_macros[imacro].members[0].blk_index;

        for (size_t imember = 0; imember < pl_macros[imacro].members.size(); imember++) {
            auto member_iblk = pl_macros[imacro].members[imember].blk_index;

            // Compute the supposed member's x,y,z location
            t_pl_loc member_pos = block_locs[head_iblk].loc + pl_macros[imacro].members[imember].offset;

            // Check the blk_loc_registry.block_locs data structure first
            if (block_locs[member_iblk].loc != member_pos) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                num_errors++;
            }

            // Then check the blk_loc_registry.grid data structure
            if (grid_blocks.block_at_location(member_pos) != member_iblk) {
                VTR_LOG_ERROR(
                    "Block %zu in pl_macro #%zu is not placed in the proper orientation.\n",
                    size_t(member_iblk), imacro);
                num_errors++;
            }
        } // Finish going through all the members
    } // Finish going through all the macros

    return num_errors;
}

/**
 * @brief Check that the placement is following the floorplanning constraints.
 *
 * This checks that all constrained blocks are placed within their floorplanning
 * constraints.
 *
 *  @param blk_loc_registry     A registry containing the block locations and
 *                              the blocks at each grid location.
 *  @param cluster_constraints  The constraints on the placement of each
 *                              cluster.
 *  @param clb_nlist            The clustered netlist being verified.
 *
 *  @return The number of errors in the placement regarding floorplanning.
 */
static unsigned check_placement_floorplanning(const BlkLocRegistry& blk_loc_registry,
                                              const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints,
                                              const ClusteredNetlist& clb_nlist) {
    const auto& block_locs = blk_loc_registry.block_locs();

    unsigned num_errors = 0;
    for (ClusterBlockId blk_id : clb_nlist.blocks()) {
        const PartitionRegion& blk_pr = cluster_constraints[blk_id];
        // If the cluster is not constrained, no need to check.
        if (blk_pr.empty())
            continue;
        // Check if the block is placed in its constrained partition region.
        const t_pl_loc& blk_loc = block_locs[blk_id].loc;
        if (!blk_pr.is_loc_in_part_reg(blk_loc)) {
            VTR_LOG_ERROR(
                "Block %zu is not in correct floorplanning region.\n",
                size_t(blk_id));
            num_errors++;
        }
    }
    return num_errors;
}

unsigned verify_placement(const BlkLocRegistry& blk_loc_registry,
                          const PlaceMacros& place_macros,
                          const ClusteredNetlist& clb_nlist,
                          const DeviceGrid& device_grid,
                          const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints) {
    // TODO: Verify the assumptions with an assert_debug.
    //  - Once a verify_clustering method is written, it would make sense to
    //    call it here in debug mode.
    unsigned num_errors = 0;

    // Check that the block placement is valid.
    num_errors += check_block_placement_consistency(blk_loc_registry,
                                                    clb_nlist,
                                                    device_grid);

    // Check that the pl_macros are observed.
    // FIXME: Should we be checking the macro consistency at all? Does the
    //        router use the pl_macros? If not this should be removed from this
    //        method and only used when the macro placement is actually used.
    num_errors += check_macro_placement_consistency(blk_loc_registry, place_macros);

    // Check that the floorplanning is observed.
    num_errors += check_placement_floorplanning(blk_loc_registry,
                                                cluster_constraints,
                                                clb_nlist);

    return num_errors;
}

unsigned verify_placement(const VprContext& ctx) {
    // Verify the placement within the given context.
    return verify_placement(ctx.placement().blk_loc_registry(),
                            *ctx.clustering().place_macros,
                            ctx.clustering().clb_nlist,
                            ctx.device().grid,
                            ctx.floorplanning().cluster_constraints);
}

