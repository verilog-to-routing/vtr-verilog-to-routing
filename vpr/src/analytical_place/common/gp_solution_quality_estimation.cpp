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
#include <fstream>
#include <limits>
#include <string>
#include <unordered_set>
#include "ap_netlist.h"
#include "device_grid.h"
#include "echo_files.h"
#include "net_cost_handler.h"
#include "partial_placement.h"
#include "physical_types.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vpr_types.h"

/**
 * @brief Empirical correction factor applied to the estimated wire usage of
 *        each net.
 *
 * The tile-HPWL estimate assumes that a net uses exactly as much wire as its
 * bounding box spans. Many FPGA architectures use longer wire segments (e.g.
 * L4) rather than L1 wires; a net whose bounding box is shorter than a wire
 * segment still uses the full segment. Since many nets are short, this causes
 * the estimate to consistently underestimate the routed wirelength.
 *
 * This factor was tuned by comparing the estimated and routed wirelength over
 * the koios, mcnc, titan_quick, and vtr_largest benchmarks (79 circuits over 4
 * architectures). The mean ratio of routed to estimated wirelength was ~1.40,
 * and scaling by this factor reduced the mean absolute percent error of the
 * estimate from ~28% to ~9%.
 *
 * TODO: The ideal factor varies by architecture (~1.29 to ~1.53 over the
 *       tested architectures). Investigate deriving it from the wire segment
 *       lengths in the architecture instead of using a single constant.
 */
static constexpr double WIRE_USAGE_CORRECTION_FACTOR = 1.4;

namespace {

/**
 * @brief The reason a net does or does not contribute to the wire usage estimate.
 */
enum class e_net_wire_usage_status {
    GLOBAL,   ///< The net is global and assumed to not be routed.
    CONSTANT, ///< The net is a constant net which will not be routed.
    ABSORBED, ///< The net is expected to be fully absorbed into a single tile.
    ESTIMATED ///< The net is expected to be routed and its wire usage was estimated.
};

/**
 * @brief The estimated post-routing wire usage of a single net, along with the
 *        intermediate values used to compute it.
 *
 * The bounding box and crossing fields are only valid when the status is
 * ESTIMATED.
 */
struct t_net_wire_usage_estimate {
    e_net_wire_usage_status status = e_net_wire_usage_status::ESTIMATED; ///< Why the net does or does not contribute.
    size_t num_distinct_tiles = 0;                                       ///< The number of distinct tiles the net connects.
    int tile_bb_dx = 0;                                                  ///< The x span of the tile bounding box (including the full extent of the edge tiles).
    int tile_bb_dy = 0;                                                  ///< The y span of the tile bounding box (including the full extent of the edge tiles).
    int tile_bb_dz = 0;                                                  ///< The number of layers crossed by the tile bounding box.
    double crossing = 0.0;                                               ///< The crossing count used to weight the tile HPWL.
    double wire_usage = 0.0;                                             ///< The estimated wire usage of the net.
};

/**
 * @brief The tile bounding box of a net and the number of distinct tiles that
 *        the pins of the net are placed within.
 */
struct t_net_tile_info {
    t_bb tile_bb;              ///< The bounding box over the tiles that contain each pin of the net.
    size_t num_distinct_tiles; ///< The number of distinct tiles that the pins of the net are placed within.
};

} // namespace

/**
 * @brief Get the tile bounding box of the given net and the number of distinct
 *        tiles that the pins of the net are placed within.
 *
 * The tile bounding box is the bounding box over the tiles that contain each pin
 * in the flat net. For example, if pins are located at x = {0.1, 0.5, 1.7}, they
 * are contained within the tiles at x = {0, 0, 1}, so the tile bounding box in
 * the x-dimension would be [0, 1].
 *
 * Blocks placed at different locations within the same large tile (for example,
 * different rows of a 1x4 DSP tile) are counted as being within the same tile.
 *
 * These are computed together since they both require a pass over the pins of
 * the net.
 *
 * TODO: For tiles larger than 1x1, the bounding box uses the location within the
 *       tile that each block is placed at. The placer instead uses the tile root
 *       plus a per-pin offset (the averaged physical pin locations in the
 *       architecture). Since the pin is not known here, investigate using the
 *       mean pin offset of the tile type instead.
 */
static t_net_tile_info get_net_tile_info(APNetId net_id,
                                         const PartialPlacement& p_placement,
                                         const APNetlist& netlist,
                                         const DeviceGrid& device_grid) {
    auto net_pins = netlist.net_pins(net_id);
    VTR_ASSERT_SAFE(!net_pins.empty());

    int min_x = std::numeric_limits<int>::max();
    int max_x = std::numeric_limits<int>::lowest();
    int min_y = std::numeric_limits<int>::max();
    int max_y = std::numeric_limits<int>::lowest();
    int min_z = std::numeric_limits<int>::max();
    int max_z = std::numeric_limits<int>::lowest();

    // Collect the root location of the tile that contains each pin. The root
    // location uniquely identifies a tile, even for tiles larger than 1x1.
    std::unordered_set<t_physical_tile_loc> net_tile_locs;
    net_tile_locs.reserve(net_pins.size());

    for (APPinId pin_id : net_pins) {
        APBlockId blk_id = netlist.pin_block(pin_id);
        t_physical_tile_loc tile_loc = p_placement.get_containing_tile_loc(blk_id);
        VTR_ASSERT_SAFE(device_grid.is_valid_tile_loc(tile_loc));

        min_x = std::min(min_x, tile_loc.x);
        max_x = std::max(max_x, tile_loc.x);
        min_y = std::min(min_y, tile_loc.y);
        max_y = std::max(max_y, tile_loc.y);
        min_z = std::min(min_z, tile_loc.layer_num);
        max_z = std::max(max_z, tile_loc.layer_num);

        net_tile_locs.insert(device_grid.get_root_location(tile_loc));
    }

    return {t_bb(min_x, max_x, min_y, max_y, min_z, max_z), net_tile_locs.size()};
}

/**
 * @brief Get the name of the given net wire usage status, as written to the
 *        wire usage estimate echo file.
 */
static const char* net_wire_usage_status_name(e_net_wire_usage_status status) {
    switch (status) {
        case e_net_wire_usage_status::GLOBAL:
            return "global";
        case e_net_wire_usage_status::CONSTANT:
            return "constant";
        case e_net_wire_usage_status::ABSORBED:
            return "absorbed";
        case e_net_wire_usage_status::ESTIMATED:
            return "estimated";
        default:
            VTR_ASSERT_MSG(false, "Unknown net wire usage status");
            return "";
    }
}

/**
 * @brief Estimate the post-routing wire usage of a single net in the given
 *        flat placement.
 *
 * See estimate_post_routing_wire_usage for a description of the estimate.
 */
static t_net_wire_usage_estimate estimate_net_wire_usage(APNetId net_id,
                                                         const PartialPlacement& p_placement,
                                                         const APNetlist& netlist,
                                                         const DeviceGrid& device_grid) {
    t_net_wire_usage_estimate net_estimate;

    // Skip nets which are marked as global. These nets are assumed to not be
    // routed, which is only true for ideal clock modeling. With other clock
    // modeling options (e.g. route or dedicated_network) these nets are
    // routed, so their wire usage will not be included in this estimate.
    // NOTE: The AP netlist speculatively marks any net connected to a clock
    //       port or a non-clock global port as global.
    if (netlist.net_is_global(net_id)) {
        net_estimate.status = e_net_wire_usage_status::GLOBAL;
        return net_estimate;
    }

    // Skip constant nets (e.g. gnd / vcc) which will not be routed. The AP netlist
    // marks these nets as ignored when they will not be routed (see
    // --constant_net_method).
    // NOTE: Constant nets may also be ignored for other reasons (e.g. high fanout).
    //       If constant nets are routed, these nets will be skipped even though
    //       they are routed.
    if (netlist.net_is_constant(net_id) && netlist.net_is_ignored(net_id)) {
        net_estimate.status = e_net_wire_usage_status::CONSTANT;
        return net_estimate;
    }

    t_net_tile_info net_tile_info = get_net_tile_info(net_id, p_placement, netlist, device_grid);
    net_estimate.num_distinct_tiles = net_tile_info.num_distinct_tiles;

    // If the net is fully absorbed into the tile, it does not contribute to the wire
    // usage since only the inter-tile wire usage is counted. Since this is operating
    // on a flat placement, this accounts for nets which will be absorbed by clustering.
    // We assume that a net will be fully absorbed if the flat locations of all pins are
    // contained within the same tile.
    // NOTE: Absorbed nets due to molecules are already handled by the construction of the
    //       AP Netlist. These nets trivially do not contribute to the wire usage.
    // NOTE: This ignores the effect of routing between different sub-tiles of the same tile
    //       (for example, two IO blocks placed in the same IO tile). Such nets would still
    //       need to be routed through the global routing network; however, it is not
    //       possible to know which sub-tiles blocks will be placed into at this point in
    //       the flow.
    if (net_tile_info.num_distinct_tiles == 1) {
        net_estimate.status = e_net_wire_usage_status::ABSORBED;
        return net_estimate;
    }

    // Compute the spans of the tile bounding box.
    // The tile bounding box is computed from the root (lower-left) locations of the
    // tiles containing each pin. A pin located anywhere within a tile (e.g. at x = 4.8)
    // is floored to that tile's location (x = 4), so max - min alone would stop short
    // of the far side of the max tile. Routing to a pin in that tile still requires a
    // full channel segment spanning the tile, so +1 is added to the x and y spans to
    // include the full extent of the tiles on both edges of the bounding box. This
    // mirrors the placer's bounding box cost, which also adds +1 to the x and y spans.
    // The z span only counts actual layer crossings.
    // TODO: Do we just add dz here? Should a wire in the third dimension
    //       be worth more?
    const t_bb& tile_bb = net_tile_info.tile_bb;
    net_estimate.tile_bb_dx = tile_bb.xmax - tile_bb.xmin + 1;
    net_estimate.tile_bb_dy = tile_bb.ymax - tile_bb.ymin + 1;
    net_estimate.tile_bb_dz = tile_bb.layer_max - tile_bb.layer_min;
    int tile_hpwl = net_estimate.tile_bb_dx + net_estimate.tile_bb_dy + net_estimate.tile_bb_dz;

    // Similar to the placer, weight the wirelength of this net as a function
    // of its fanout.
    // We reuse the wirelength crossing count from the placer. Since the placer
    // operates on the clustered netlist, we estimate the number of pins this net
    // will have after clustering as the number of distinct tiles it connects.
    // NOTE: The number of distinct tiles includes the driver's tile; this matches
    //       the placer, which passes the total number of pins (driver included)
    //       to the crossing count lookup.
    // TODO: AP should have its own crossing count function. This will just make changing
    //       this in the future easier.
    net_estimate.crossing = wirelength_crossing_count(net_estimate.num_distinct_tiles);

    // Estimate the wire usage based on the tile-HPWL and the crossing factor,
    // corrected for the systematic underestimate of short nets (see
    // WIRE_USAGE_CORRECTION_FACTOR).
    net_estimate.status = e_net_wire_usage_status::ESTIMATED;
    net_estimate.wire_usage = static_cast<double>(tile_hpwl) * net_estimate.crossing * WIRE_USAGE_CORRECTION_FACTOR;

    return net_estimate;
}

/**
 * @brief Write the per-net wire usage estimates of the given flat placement to
 *        the given file.
 *
 * Every net in the AP netlist is written, including nets which do not
 * contribute to the estimate (with the reason in the status column). The net
 * name is the last column since it is the only column which may contain
 * unusual characters.
 *
 * NOTE: The per-net estimates are re-computed here, rather than stored while
 *       computing the total, to avoid storing them when the echo file is not
 *       enabled.
 */
static void write_wire_usage_estimate_echo(const std::string& filename,
                                           const PartialPlacement& p_placement,
                                           const APNetlist& netlist,
                                           const DeviceGrid& device_grid) {
    std::ofstream os(filename);
    if (!os) {
        VTR_LOG_WARN("Unable to open wire usage estimate echo file '%s' for writing.\n", filename.c_str());
        return;
    }

    os << "# Estimated post-routing wire usage of each net in the AP netlist.\n";
    os << "#   num_pins:  Number of pins on the net (atom-level).\n";
    os << "#   status:    global | constant | absorbed (do not contribute) or estimated.\n";
    os << "#   num_tiles: Number of distinct tiles connected by the net.\n";
    os << "#   bb_dx/dy:  Tile bounding box span, including the full extent of the edge tiles (+1).\n";
    os << "#   bb_dz:     Number of layers crossed by the tile bounding box.\n";
    os << "#   crossing:  Crossing count used to weight the tile HPWL.\n";
    os << "#   estimate:  (bb_dx + bb_dy + bb_dz) * crossing * " << WIRE_USAGE_CORRECTION_FACTOR << " (correction factor).\n";
    os << "num_pins status num_tiles bb_dx bb_dy bb_dz crossing estimate net_name\n";
    for (APNetId net_id : netlist.nets()) {
        t_net_wire_usage_estimate net_estimate = estimate_net_wire_usage(net_id,
                                                                         p_placement,
                                                                         netlist,
                                                                         device_grid);
        os << netlist.net_pins(net_id).size() << " "
           << net_wire_usage_status_name(net_estimate.status) << " "
           << net_estimate.num_distinct_tiles << " "
           << net_estimate.tile_bb_dx << " "
           << net_estimate.tile_bb_dy << " "
           << net_estimate.tile_bb_dz << " "
           << net_estimate.crossing << " "
           << net_estimate.wire_usage << " "
           << netlist.net_name(net_id) << "\n";
    }
}

double estimate_post_routing_wire_usage(const PartialPlacement& p_placement,
                                        const APNetlist& netlist,
                                        const DeviceGrid& device_grid) {
    // Go through each net in the AP netlist and estimate the amount of post-routing
    // wire usage each net will contribute.
    double wire_usage = 0.0;
    for (APNetId net_id : netlist.nets()) {
        wire_usage += estimate_net_wire_usage(net_id, p_placement, netlist, device_grid).wire_usage;
    }

    // Write the per-net estimates to an echo file if requested, so they can be
    // compared net-by-net against the routed wire usage of each net.
    if (isEchoFileEnabled(E_ECHO_AP_POST_ROUTING_WIRE_USAGE_ESTIMATE))
        write_wire_usage_estimate_echo(getEchoFileName(E_ECHO_AP_POST_ROUTING_WIRE_USAGE_ESTIMATE),
                                       p_placement,
                                       netlist,
                                       device_grid);

    return wire_usage;
}
