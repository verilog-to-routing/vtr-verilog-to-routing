/**
 * @file
 * @author  Alex Singer
 * @date    September 2026
 * @brief   Implementation of methods for estimating the quality of a global
 *          placement solution.
 */

#include "gp_solution_quality_estimation.h"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <unordered_set>
#include "ap_netlist.h"
#include "device_grid.h"
#include "net_cost_handler.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "vtr_assert.h"
#include "vtr_geometry.h"

/**
 * @brief Check if the given net is fully absorbed in a single tile in the device grid.
 *
 * We assume that a net will be fully absorbed if the flat locations of all pins are contained
 * within the same tile.
 *
 * NOTE: This ignores the effect of routing between different sub-tiles of the same tile (for
 *       example, two IO blocks placed in the same IO tile). Such nets would still need to be
 *       routed through the global routing network; however, it is not possible to know which
 *       sub-tiles blocks will be placed into at this point in the flow.
 */
static bool net_is_fully_absorbed_in_tile(APNetId net_id,
                                          const PartialPlacement& p_placement,
                                          const APNetlist& netlist,
                                          const DeviceGrid& device_grid) {
    auto net_pins = netlist.net_pins(net_id);
    VTR_ASSERT_SAFE(!net_pins.empty());

    // Get the tile bounding box of the tile that contains the first pin of the net.
    // NOTE: The first pin is used instead of the driver since a net is not
    //       guaranteed to have a driver.
    auto pin_it = net_pins.begin();
    APBlockId first_blk_id = netlist.pin_block(*pin_it);
    t_physical_tile_loc first_tile_loc = p_placement.get_containing_tile_loc(first_blk_id);
    VTR_ASSERT_SAFE(device_grid.is_valid_tile_loc(first_tile_loc));
    vtr::Rect<int> first_tile_bb = device_grid.get_tile_bb(first_tile_loc);

    // Check if any of the remaining pins are not within the bounds of the first
    // pin's tile; if so, then the net cannot be fully absorbed into the tile.
    // The first pin is skipped since it is trivially within its own tile.
    for (++pin_it; pin_it != net_pins.end(); ++pin_it) {
        APBlockId blk_id = netlist.pin_block(*pin_it);
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);

        // If the pin is not on the same layer as the first pin, then it cannot be
        // absorbed.
        // It is currently assumed that tiles do not cross die-layers.
        if (tile_loc.layer_num != first_tile_loc.layer_num)
            return false;

        // If the pin is not fully contained within the same tile as the first pin,
        // then it will not be absorbed.
        // NOTE: We use contains_inclusive since the bb is defined to exclude the outer edge.
        //       For example, a 1x1 tile at (2,3) would have the bb rect:
        //              {[xmin,xmax], [ymin,ymax]} = {[2,2], [3,3]}
        if (!first_tile_bb.contains_inclusive({tile_loc.x, tile_loc.y}))
            return false;
    }

    // If the above checks passed, then the net can be fully absorbed.
    return true;
}

/**
 * @brief Get the number of distinct tiles that the pins of the given net are placed within.
 *
 * Blocks placed at different locations within the same large tile (for example, different
 * rows of a 1x4 DSP tile) are counted as being within the same tile.
 */
static size_t get_num_distinct_tiles_in_net(APNetId net_id,
                                            const PartialPlacement& p_placement,
                                            const APNetlist& netlist,
                                            const DeviceGrid& device_grid) {
    // Collect the root location of the tile that contains each pin. The root
    // location uniquely identifies a tile, even for tiles larger than 1x1.
    auto net_pins = netlist.net_pins(net_id);
    std::unordered_set<t_physical_tile_loc> net_tile_locs;
    net_tile_locs.reserve(net_pins.size());
    for (APPinId pin_id : net_pins) {
        APBlockId blk_id = netlist.pin_block(pin_id);
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);
        VTR_ASSERT_SAFE(device_grid.is_valid_tile_loc(tile_loc));
        net_tile_locs.insert(device_grid.get_root_location(tile_loc));
    }

    return net_tile_locs.size();
}

double estimate_post_routing_wire_usage(const PartialPlacement& p_placement,
                                        const APNetlist& netlist,
                                        const DeviceGrid& device_grid) {
    // Go through each net in the AP netlist and estimate the amount of post-routing
    // wire usage each net will contribute.
    double wire_usage = 0.0;
    for (APNetId net_id : netlist.nets()) {
        // Skip nets which are marked as global. These nets are assumed to not be
        // routed, which is only true for ideal clock modeling. With other clock
        // modeling options (e.g. route or dedicated_network) these nets are
        // routed, so their wire usage will not be included in this estimate.
        // NOTE: The AP netlist speculatively marks any net connected to a clock
        //       port or a non-clock global port as global.
        if (netlist.net_is_global(net_id))
            continue;

        // If the net is fully absorbed into the tile, it does not contribute to the wire
        // usage since only the inter-tile wire usage is counted. Since this is operating
        // on a flat placement, this accounts for nets which will be absorbed by clustering.
        // NOTE: Absorbed nets due to molecules are already handled by the construction of the
        //       AP Netlist. These nets trivially do not contribute to the wire usage.
        if (net_is_fully_absorbed_in_tile(net_id, p_placement, netlist, device_grid))
            continue;

        // Compute the tile bounding box of the net. This is the bounding box over the tiles
        // that contain each pin in the flat net. For example, if pins are located at
        // x = {0.1, 0.5, 1.7}, they are contained within the tiles at x = {0, 0, 1}, so the
        // tile bounding box in the x-dimension would be [0, 1].
        // TODO: For tiles larger than 1x1, this uses the location within the tile that each
        //       block is placed at. The placer instead uses the tile root plus a per-pin offset
        //       (the averaged physical pin locations in the architecture). Since the pin is not
        //       known here, investigate using the mean pin offset of the tile type instead.
        int min_x = std::numeric_limits<int>::max();
        int max_x = std::numeric_limits<int>::lowest();
        int min_y = std::numeric_limits<int>::max();
        int max_y = std::numeric_limits<int>::lowest();
        int min_z = std::numeric_limits<int>::max();
        int max_z = std::numeric_limits<int>::lowest();
        for (APPinId pin_id : netlist.net_pins(net_id)) {
            APBlockId blk_id = netlist.pin_block(pin_id);
            t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);
            min_x = std::min(min_x, tile_loc.x);
            max_x = std::max(max_x, tile_loc.x);
            min_y = std::min(min_y, tile_loc.y);
            max_y = std::max(max_y, tile_loc.y);
            min_z = std::min(min_z, tile_loc.layer_num);
            max_z = std::max(max_z, tile_loc.layer_num);
        }
        VTR_ASSERT_SAFE(max_x >= min_x && max_y >= min_y && max_z >= min_z);

        // Similar to the placer, the x and y spans include the channel adjacent to the
        // tile bounding box (hence the +1). The z span only counts actual layer crossings.
        // TODO: Do we just add dz here? Should a wire in the third dimension
        //       be worth more?
        int tile_bb_dx = max_x - min_x + 1;
        int tile_bb_dy = max_y - min_y + 1;
        int tile_bb_dz = max_z - min_z;
        int tile_hpwl = tile_bb_dx + tile_bb_dy + tile_bb_dz;

        // Similar to the placer, weight the wirelength of this net as a function
        // of its fanout.
        // We reuse the wirelength crossing count from the placer. Since the placer
        // operates on the clustered netlist, we estimate the number of pins this net
        // will have after clustering as the number of distinct tiles it connects.
        // TODO: AP should have its own crossing count function. This will just make changing
        //       this in the future easier.
        size_t num_distinct_tiles = get_num_distinct_tiles_in_net(net_id,
                                                                  p_placement,
                                                                  netlist,
                                                                  device_grid);
        // Since the net is not absorbed into a single tile, it must connect at least two tiles.
        VTR_ASSERT_SAFE(num_distinct_tiles >= 2);
        double crossing = wirelength_crossing_count(num_distinct_tiles);

        // Estimate the wire usage based on the tile-HPWL and the crossing factor.
        wire_usage += static_cast<double>(tile_hpwl) * crossing;
    }

    return wire_usage;
}
