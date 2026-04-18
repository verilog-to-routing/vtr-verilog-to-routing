/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   The definitions of the PartialPlacement methods
 */

#include "partial_placement.h"
#include <cmath>
#include <cstddef>
#include <limits>
#include "ap_netlist.h"
#include "net_cost_handler.h"

double PartialPlacement::get_hpwl(const APNetlist& netlist) const {
    double hpwl = 0.0;
    for (APNetId net_id : netlist.nets()) {
        if (netlist.net_is_ignored(net_id))
            continue;
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        double min_z = std::numeric_limits<double>::max();
        double max_z = std::numeric_limits<double>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            min_x = std::min(min_x, block_x_locs[blk_id]);
            max_x = std::max(max_x, block_x_locs[blk_id]);
            min_y = std::min(min_y, block_y_locs[blk_id]);
            max_y = std::max(max_y, block_y_locs[blk_id]);
            min_z = std::min(min_z, block_layer_nums[blk_id]);
            max_z = std::max(max_z, block_layer_nums[blk_id]);
        }
        // TODO: In the placer, the x and y dimensions are multiplied by cost
        //       factors based on the channel width. Should somehow bring these
        //       in here.
        //       Vaughn thinks these may make sense in the objective HPWL, but
        //       not the in the estimated post-placement wirelength.
        VTR_ASSERT_SAFE(max_x >= min_x && max_y >= min_y && max_z >= min_z);
        // TODO: Should dz be added here? Or should we add a penalty?
        hpwl += max_x - min_x + max_y - min_y + max_z - min_z;
    }
    return hpwl;
}

double PartialPlacement::estimate_post_placement_wirelength(const APNetlist& netlist) const {
    // Go through each net and calculate the half-perimeter wirelength. Since
    // we want to estimate the post-placement wirelength, we do not want the
    // flat placement positions of the blocks. Instead we compute the HPWL over
    // the tiles that the flat placement is placing the blocks over.
    double total_hpwl = 0;
    for (APNetId net_id : netlist.nets()) {
        // To align with other wirelength estimators in VTR (for example in the
        // placer), we do not include global nets (clocks, etc.) in the wirelength
        // calculation.
        if (netlist.net_is_global(net_id))
            continue;

        // Similar to the placer, weight the wirelength of this net as a function
        // of its fanout. Since these fanouts are at the AP netlist (unclustered)
        // level, the correction factor may lead to a somewhat higher HPWL prediction
        // than after clustering.
        // TODO: Investigate the clustered vs unclustered factors further.
        // TODO: Should update the costs to 3D.
        double crossing = wirelength_crossing_count(netlist.net_pins(net_id).size());

        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        double min_z = std::numeric_limits<double>::max();
        double max_z = std::numeric_limits<double>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            min_x = std::min(min_x, block_x_locs[blk_id]);
            max_x = std::max(max_x, block_x_locs[blk_id]);
            min_y = std::min(min_y, block_y_locs[blk_id]);
            max_y = std::max(max_y, block_y_locs[blk_id]);
            min_z = std::min(min_z, block_layer_nums[blk_id]);
            max_z = std::max(max_z, block_layer_nums[blk_id]);
        }
        VTR_ASSERT_SAFE(max_x >= min_x && max_y >= min_y && max_z >= min_z);

        // Floor the positions to get the x and y coordinates of the tiles each
        // block belongs to.
        double tile_dx = std::floor(max_x) - std::floor(min_x);
        double tile_dy = std::floor(max_y) - std::floor(min_y);
        double tile_dz = std::floor(max_z) - std::floor(min_z);

        // TODO: Do we just add dz here? Should a wire in the third dimension
        //       be worth more?
        total_hpwl += (tile_dx + tile_dy + tile_dz) * crossing;
    }

    return total_hpwl;
}

bool PartialPlacement::verify_locs(const APNetlist& netlist,
                                   size_t grid_width,
                                   size_t grid_height) const {
    // Make sure all of the loc values are there.
    if (block_x_locs.size() != netlist.blocks().size())
        return false;
    if (block_y_locs.size() != netlist.blocks().size())
        return false;
    // Make sure all locs are on the device and fixed locs are correct
    for (APBlockId blk_id : netlist.blocks()) {
        double x_pos = block_x_locs[blk_id];
        double y_pos = block_y_locs[blk_id];
        if (std::isnan(x_pos) || x_pos < 0.0 || x_pos >= grid_width)
            return false;
        if (std::isnan(y_pos) || y_pos < 0.0 || y_pos >= grid_height)
            return false;
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED) {
            const APFixedBlockLoc& fixed_loc = netlist.block_loc(blk_id);
            if (g_vpr_ctx.atom().flat_placement_info().valid) {
                // Flat placement files use the anchor positions of blocks, so to match the
                // internal global placer of VTR we add 0.5 here to move the flat placement
                // locations to the center of (at least 1x1) tiles.
                // TODO: This should be handled more explicitly.
                x_pos += 0.5f;
                y_pos += 0.5f;
            }
            if (fixed_loc.x != -1 && x_pos != fixed_loc.x)
                return false;
            if (fixed_loc.y != -1 && y_pos != fixed_loc.y)
                return false;
        }
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify_layer_nums(const APNetlist& netlist,
                                         size_t grid_num_layers) const {
    // Make sure all of the layer nums are there
    if (block_layer_nums.size() != netlist.blocks().size())
        return false;
    // Make sure all layer_nums are on the device and fixed locs are correct.
    for (APBlockId blk_id : netlist.blocks()) {
        double layer_num = block_layer_nums[blk_id];
        if (std::isnan(layer_num) || layer_num < 0.0 || layer_num >= static_cast<double>(grid_num_layers))
            return false;
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED) {
            const APFixedBlockLoc& fixed_loc = netlist.block_loc(blk_id);
            if (fixed_loc.layer_num != -1 && layer_num != fixed_loc.layer_num)
                return false;
        }
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify_sub_tiles(const APNetlist& netlist) const {
    // Make sure all of the sub tiles are there
    if (block_sub_tiles.size() != netlist.blocks().size())
        return false;
    // For now, we do not really do much with the sub_tile information. Ideally
    // we should check that all blocks have a sub_tile that actually exists
    // (a tile actually has a sub_tile of that idx); however, this may be
    // challenging to enforce. For now, just ensure that the number is
    // non-negative (implying a choice was made).
    for (APBlockId blk_id : netlist.blocks()) {
        int sub_tile = block_sub_tiles[blk_id];
        if (sub_tile < 0)
            return false;
        if (netlist.block_mobility(blk_id) == APBlockMobility::FIXED) {
            const APFixedBlockLoc& fixed_loc = netlist.block_loc(blk_id);
            if (fixed_loc.sub_tile != -1 && sub_tile != fixed_loc.sub_tile)
                return false;
        }
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify(const APNetlist& netlist,
                              size_t grid_width,
                              size_t grid_height,
                              size_t grid_num_layers) const {
    // Check that all the other verify methods passed.
    if (!verify_locs(netlist, grid_width, grid_height))
        return false;
    if (!verify_layer_nums(netlist, grid_num_layers))
        return false;
    if (!verify_sub_tiles(netlist))
        return false;
    // If all other verify methods passed, then the placement is valid.
    return true;
}
