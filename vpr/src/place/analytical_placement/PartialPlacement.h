
#pragma once

#include "ap_netlist.h"
#include "ap_netlist_fwd.h"
#include "vtr_vector.h"

class DeviceContext;

// The goal of this class is to contain positional information about the blocks
// that gets passed around during analytical placement.
struct PartialPlacement {
    vtr::vector<APBlockId, double> block_x_locs;
    vtr::vector<APBlockId, double> block_y_locs;
    vtr::vector<APBlockId, int> block_layer_nums;
    vtr::vector<APBlockId, int> block_sub_tiles;

    PartialPlacement() = delete;
    // FIXME: Move this to the definition file perhaps.
    PartialPlacement(const APNetlist& netlist) : block_x_locs(netlist.blocks().size(), -1.f),
                                                 block_y_locs(netlist.blocks().size(), -1.f),
                                                 block_layer_nums(netlist.blocks().size(), 0),
                                                 block_sub_tiles(netlist.blocks().size(), 0) {
        // Load the fixed block locations
        for (APBlockId blk_id : netlist.blocks()) {
            if (netlist.block_type(blk_id) != APBlockType::FIXED)
                continue;
            const APFixedBlockLoc &loc = netlist.block_loc(blk_id);
            if (loc.x != -1)
                block_x_locs[blk_id] = loc.x;
            if (loc.y != -1)
                block_y_locs[blk_id] = loc.y;
            if (loc.layer_num != -1)
                block_layer_nums[blk_id] = loc.layer_num;
            if (loc.sub_tile != -1)
                block_sub_tiles[blk_id] = loc.sub_tile;
        }
    }

    // Verify the block_x_locs and block_y_locs vectors. Currently ensures:
    //  - All blocks have locs
    //  - All blocks are on the device
    //  - All fixed locs are correct
    bool verify_locs(const APNetlist& netlist, const DeviceContext& device_ctx);

    // Verify the block_layer_nums vector. Currently ensures:
    //  - All blocks have layer_nums
    //  - All blocks are on the device
    //  - All fixed layers are correct
    bool verify_layer_nums(const APNetlist& netlist, const DeviceContext& device_ctx);

    // Verify the sub_tiles. Currently ensures:
    //  - All sub_tiles are non-negative
    bool verify_sub_tiles(const APNetlist& netlist, const DeviceContext& device_ctx);

    // Runs all of the verification checks above.
    bool verify(const APNetlist& netlist, const DeviceContext& device_ctx);
};

