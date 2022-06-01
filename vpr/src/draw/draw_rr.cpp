
#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <array>
#include <iostream>

#include "vtr_assert.h"
#include "vtr_ndoffsetmatrix.h"
#include "vtr_memory.h"
#include "vtr_log.h"
#include "vtr_color_map.h"
#include "vtr_path.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "intra_logic_block.h"
#include "atom_netlist.h"
#include "tatum/report/TimingPathCollector.hpp"
#include "hsl.h"
#include "route_export.h"
#include "search_bar.h"
#include "save_graphics.h"
#include "timing_info.h"
#include "physical_types.h"
#include "route_common.h"
#include "breakpoint.h"
#include "manual_moves.h"

#include "move_utils.h"

#ifdef VTR_ENABLE_DEBUG_LOGGING
#    include "move_utils.h"
#endif

#ifdef WIN32 /* For runtime tracking in WIN32. The clock() function defined in time.h will *
              * track CPU runtime.														   */
#    include <time.h>
#else /* For X11. The clock() function in time.h will not output correct time difference   *
       * for X11, because the graphics is processed by the Xserver rather than local CPU,  *
       * which means tracking CPU time will not be the same as the actual wall clock time. *
       * Thus, so use gettimeofday() in sys/time.h to track actual calendar time.          */
#    include <sys/time.h>
#endif

#ifndef NO_GRAPHICS

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#    if defined(X11) && !defined(__MINGW32__)
#        include <X11/keysym.h>
#    endif

#    include "rr_graph.h"
#    include "route_util.h"
#    include "place_macro.h"
#    include "buttons.h"

/****************************** Define Macros *******************************/

#    define DEFAULT_RR_NODE_COLOR ezgl::BLACK
#    define OLD_BLK_LOC_COLOR blk_GOLD
#    define NEW_BLK_LOC_COLOR blk_GREEN
//#define TIME_DRAWSCREEN /* Enable if want to track runtime for drawscreen() */


void draw_pin_to_chan_edge(int pin_node, int chan_node, ezgl::renderer* g) {
    /* This routine draws an edge from the pin_node to the chan_node (CHANX or   *
     * CHANY).  The connection is made to the nearest end of the track instead   *
     * of perpendicular to the track to symbolize a single-drive connection.     */

    /* TODO: Fix this for global routing, currently for detailed only */

    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    //const t_rr_node& pin_rr = device_ctx.rr_nodes[pin_node];
    auto pin_rr = RRNodeId(pin_node);
    auto chan_rr = RRNodeId(chan_node);

    const t_grid_tile& grid_tile = device_ctx.grid[rr_graph.node_xlow(pin_rr)][rr_graph.node_ylow(pin_rr)];
    t_physical_tile_type_ptr grid_type = grid_tile.type;

    float x1 = 0, y1 = 0;
    /* If there is only one side, no need for the following inference!!!
     * When a node may have multiple sides,
     * we lack direct information about which side of the node drives the channel node
     * However, we can infer which side is actually used by the driver based on the
     * coordinates of the channel node.
     * In principle, in a regular rr_graph that can pass check_rr_graph() function,
     * the coordinates should follow the illustration:
     *
     *                +----------+
     *                |  CHANX   |
     *                |  [x][y]  |
     *                +----------+
     *   +----------+ +----------+ +--------+
     *   |          | |          | |        |
     *   |  CHANY   | |  Grid    | | CHANY  |
     *   | [x-1][y] | | [x][y]   | | [x][y] |
     *   |          | |          | |        |
     *   +----------+ +----------+ +--------+
     *                +----------+
     *                |  CHANX   |
     *                | [x][y-1] |
     *                +----------+
     *
     *
     * Therefore, when there are multiple side:
     * - a TOP side node is considered when the ylow of CHANX >= ylow of the node
     * - a BOTTOM side node is considered when the ylow of CHANX <= ylow - 1 of the node
     * - a RIGHT side node is considered when the xlow of CHANY >= xlow of the node
     * - a LEFT side node is considered when the xlow of CHANY <= xlow - 1 of the node
     *
     * Note: ylow == yhigh for CHANX and xlow == xhigh for CHANY
     *
     * Note: Similar rules are applied for grid that has width > 1 and height > 1
     *       This is because (xlow, ylow) or (xhigh, yhigh) of the node follows
     *       the actual offset of the pin in the context of grid width and height
     */
    std::vector<e_side> pin_candidate_sides;
    for (const e_side& pin_candidate_side : SIDES) {
        if ((rr_graph.is_node_on_specific_side(pin_rr, pin_candidate_side))
            && (grid_type->pinloc[grid_tile.width_offset][grid_tile.height_offset][pin_candidate_side][rr_graph.node_pin_num(pin_rr)])) {
            pin_candidate_sides.push_back(pin_candidate_side);
        }
    }
    /* Only 1 side will be picked in the end
     * Any rr_node of a grid should have at least 1 side!!!
     */
    e_side pin_side = NUM_SIDES;
    const t_rr_type channel_type = rr_graph.node_type(RRNodeId(chan_node));
    if (1 == pin_candidate_sides.size()) {
        pin_side = pin_candidate_sides[0];
    } else {
        VTR_ASSERT(1 < pin_candidate_sides.size());
        if (CHANX == channel_type && rr_graph.node_ylow(pin_rr) <= rr_graph.node_ylow(chan_rr)) {
            pin_side = TOP;
        } else if (CHANX == channel_type && rr_graph.node_ylow(pin_rr) - 1 >= rr_graph.node_ylow(chan_rr)) {
            pin_side = BOTTOM;
        } else if (CHANY == channel_type && rr_graph.node_xlow(pin_rr) <= rr_graph.node_xlow(chan_rr)) {
            pin_side = RIGHT;
        } else if (CHANY == channel_type && rr_graph.node_xlow(pin_rr) - 1 >= rr_graph.node_xlow(chan_rr)) {
            pin_side = LEFT;
        }
        /* The inferred side must be in the list of sides of the pin rr_node!!! */
        VTR_ASSERT(pin_candidate_sides.end() != std::find(pin_candidate_sides.begin(), pin_candidate_sides.end(), pin_side));
    }
    /* Sanity check */
    VTR_ASSERT(NUM_SIDES != pin_side);

    /* Now we determine which side to be used, calculate the offset for the pin to be drawn
     * - For the pin locates above/right to the grid (at the top/right side),
     *   a positive offset (+ve) is required
     * - For the pin locates below/left to the grid (at the bottom/left side),
     *   a negative offset (-ve) is required
     *
     *   y
     *   ^                           +-----+ ---
     *   |                           | PIN |  ^
     *   |                           |     |  offset
     *   |                           |     |  v
     *   |               +-----------+-----+----------+
     *   |               |                            |<- offset ->|
     *   |    |<-offset->|                            +------------+
     *   |    +----------+        Grid                |   PIN      |
     *   |    | PIN      |                            +------------+
     *   |    +----------+                            |
     *   |               |                            |
     *   |               +---+-----+------------------+
     *   |               ^   |     |
     *   |            offset | PIN |
     *   |               v   |     |
     *   |               ----+-----+
     *   +------------------------------------------------------------>x
     */
    float draw_pin_offset;
    if (TOP == pin_side || RIGHT == pin_side) {
        draw_pin_offset = draw_coords->pin_size;
    } else {
        VTR_ASSERT(BOTTOM == pin_side || LEFT == pin_side);
        draw_pin_offset = -draw_coords->pin_size;
    }

    draw_get_rr_pin_coords(pin_node, &x1, &y1, pin_side);

    ezgl::rectangle chan_bbox = draw_get_rr_chan_bbox(chan_node);

    float x2 = 0, y2 = 0;
    const Direction chan_rr_direction = rr_graph.node_direction(RRNodeId(chan_node));
    switch (channel_type) {
        case CHANX: {
            y1 += draw_pin_offset;
            y2 = chan_bbox.bottom();
            x2 = x1;
            if (is_opin(rr_graph.node_pin_num(pin_rr), grid_type)) {
                if (chan_rr_direction == Direction::INC) {
                    x2 = chan_bbox.left();
                } else if (chan_rr_direction == Direction::DEC) {
                    x2 = chan_bbox.right();
                }
            }
            break;
        }
        case CHANY: {
            x1 += draw_pin_offset;
            x2 = chan_bbox.left();
            y2 = y1;
            if (is_opin(rr_graph.node_pin_num(pin_rr), grid_type)) {
                if (chan_rr_direction == Direction::INC) {
                    y2 = chan_bbox.bottom();
                } else if (chan_rr_direction == Direction::DEC) {
                    y2 = chan_bbox.top();
                }
            }
            break;
        }
        default: {
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "in draw_pin_to_chan_edge: Invalid channel node %d.\n", chan_node);
        }
    }
    g->draw_line({x1, y1}, {x2, y2});

    //don't draw the ex, or triangle unless zoomed in really far
    if (chan_rr_direction == Direction::BIDIR || !is_opin(rr_graph.node_pin_num(pin_rr), grid_type)) {
        draw_x(x2, y2, 0.7 * draw_coords->pin_size, g);
    } else {
        float xend = x2 + (x1 - x2) / 10.;
        float yend = y2 + (y1 - y2) / 10.;
        draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
    }
}

#endif
