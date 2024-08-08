
#include "PartialPlacement.h"
#include <cmath>
#include <cstddef>
#include "ap_netlist.h"
#include "vpr_context.h"

bool PartialPlacement::verify_locs(const APNetlist& netlist, const DeviceContext& device_ctx) {
    // Make sure all of the loc values are there.
    if (block_x_locs.size() != netlist.blocks().size())
        return false;
    if (block_y_locs.size() != netlist.blocks().size())
        return false;
    // Make sure all locs are on the device and fixed locs are correct
    size_t grid_width = device_ctx.grid.width();
    size_t grid_height = device_ctx.grid.height();
    for (APBlockId blk_id : netlist.blocks()) {
        double x_pos = block_x_locs[blk_id];
        double y_pos = block_y_locs[blk_id];
        if (std::isnan(x_pos) ||
            x_pos < 0.0 ||
            x_pos >= grid_width)
            return false;
        if (std::isnan(y_pos) ||
            y_pos < 0.0 ||
            y_pos >= grid_height)
            return false;
        if (netlist.block_type(blk_id) == APBlockType::FIXED) {
            APFixedBlockLoc fixed_loc = netlist.block_loc(blk_id);
            int fixed_x = fixed_loc.x;
            int fixed_y = fixed_loc.y;
            if (fixed_x != -1 && x_pos != fixed_x)
                return false;
            if (fixed_y != -1 && y_pos != fixed_y)
                return false;
        }
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify_layer_nums(const APNetlist& netlist, const DeviceContext& device_ctx) {
    // Make sure all of the layer nums are there
    if (block_layer_nums.size() != netlist.blocks().size())
        return false;
    // Make sure all layer_nums are on the device and fixed locs are correct.
    int num_layers = device_ctx.grid.get_num_layers();
    for (APBlockId blk_id : netlist.blocks()) {
        int layer_num = block_layer_nums[blk_id];
        if (layer_num < 0 || layer_num >= num_layers)
            return false;
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify_sub_tiles(const APNetlist& netlist, const DeviceContext& /*device_ctx*/) {
    // Make sure all of the sub tiles are there
    if (block_sub_tiles.size() != netlist.blocks().size())
        return false;
    // For now, we do not really do much with the sub_tile information. Ideally
    // we should check that all blocks have a sub_tile that actually exists
    // (a block actually has a sub_tile of that idx); however, this may be
    // challenging to enforce. For now, just ensure that the number is
    // non-negative (implying no choice was made).
    for (APBlockId blk_id : netlist.blocks()) {
        if (block_sub_tiles[blk_id] < 0)
            return false;
    }
    // If all previous checks passed, return true
    return true;
}

bool PartialPlacement::verify(const APNetlist& netlist, const DeviceContext& device_ctx) {
    if (!verify_locs(netlist, device_ctx))
        return false;
    if (!verify_layer_nums(netlist, device_ctx))
        return false;
    if (!verify_sub_tiles(netlist, device_ctx))
        return false;
    return true;
}

