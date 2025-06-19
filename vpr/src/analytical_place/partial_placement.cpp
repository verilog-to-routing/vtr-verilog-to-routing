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

double PartialPlacement::get_hpwl(const APNetlist& netlist) const {
    double hpwl = 0.0;
    for (APNetId net_id : netlist.nets()) {
        if (netlist.net_is_ignored(net_id))
            continue;
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            min_x = std::min(min_x, block_x_locs[blk_id]);
            max_x = std::max(max_x, block_x_locs[blk_id]);
            min_y = std::min(min_y, block_y_locs[blk_id]);
            max_y = std::max(max_y, block_y_locs[blk_id]);
        }
        VTR_ASSERT_SAFE(max_x >= min_x && max_y >= min_y);
        hpwl += max_x - min_x + max_y - min_y;
    }
    return hpwl;
}

double PartialPlacement::estimate_post_placement_wirelength(const APNetlist& netlist) const {
    // Go through each net and calculate the half-perimeter wirelength. Since
    // we want to estimate the post-placement wirelength, we do not want the
    // flat placement positions of the blocks. Instead we compute the HPWL over
    // the tiles that the flat placement is placing the blocks over.
    unsigned total_hpwl = 0;
    for (APNetId net_id : netlist.nets()) {
        // Note: Other wirelength estimates in VTR ignore global nets; however
        //       it is not known if a net is global or not until packing is
        //       complete. For now, we just approximate post-placement wirelength
        //       using the HPWL (in tile space).
        // TODO: The reason we do not know what nets are ignored / global is
        //       because the pin on the tile that the net connects to is what
        //       decides if a net is global / ignored for place and route. Since
        //       we have not packed anything yet, we do not know what pin each
        //       net will go to; however, we can probably get a good idea based
        //       on some properties of the net and the tile its going to / from.
        //       Should investigate this to get a better estimate of wirelength.
        double min_x = std::numeric_limits<unsigned>::max();
        double max_x = std::numeric_limits<unsigned>::lowest();
        double min_y = std::numeric_limits<unsigned>::max();
        double max_y = std::numeric_limits<unsigned>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            min_x = std::min(min_x, block_x_locs[blk_id]);
            max_x = std::max(max_x, block_x_locs[blk_id]);
            min_y = std::min(min_y, block_y_locs[blk_id]);
            max_y = std::max(max_y, block_y_locs[blk_id]);
        }
        VTR_ASSERT_SAFE(max_x >= min_x && max_y >= min_y);

        // Floor the positions to get the x and y coordinates of the tiles each
        // block belongs to.
        unsigned tile_dx = std::floor(max_x) - std::floor(min_x);
        unsigned tile_dy = std::floor(max_y) - std::floor(min_y);

        total_hpwl += tile_dx + tile_dy;
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
        if (layer_num < 0.0 || layer_num >= static_cast<double>(grid_num_layers))
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
