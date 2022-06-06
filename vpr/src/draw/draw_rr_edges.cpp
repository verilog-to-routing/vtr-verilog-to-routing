/*draw_rr_edges.cpp contains all functions that draw lines between RR nodes.*/
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
#include "draw_rr_edges.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "draw_basic.h"
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

void draw_chany_to_chany_edge(RRNodeId from_node, RRNodeId to_node, int to_track, short switch_type, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    /* Draws a connection between two y-channel segments.  Passing in the track *
     * numbers allows this routine to be used for both rr_graph and routing     *
     * drawing->                                                                 */

    float x1, x2, y1, y2;
    ezgl::rectangle from_chan;
    ezgl::rectangle to_chan;
    int from_ylow, to_ylow, from_yhigh, to_yhigh; //, from_x, to_x;

    // Get the coordinates of the channel wires.
    from_chan = draw_get_rr_chan_bbox(size_t(from_node));
    to_chan = draw_get_rr_chan_bbox(size_t(to_node));

    // from_x = rr_graph.node_xlow(RRNodeId(from_node));
    // to_x = rr_graph.node_xlow(RRNodeId(to_node));
    from_ylow = rr_graph.node_ylow(from_node);
    from_yhigh = rr_graph.node_yhigh(from_node);
    to_ylow = rr_graph.node_ylow(to_node);
    to_yhigh = rr_graph.node_yhigh(to_node);

    /* (x1, y1) point on from_node, (x2, y2) point on to_node. */

    x1 = from_chan.left();
    x2 = to_chan.left();

    if (to_yhigh < from_ylow) { /* From upper to lower */
        y1 = from_chan.bottom();
        y2 = to_chan.top();
    } else if (to_ylow > from_yhigh) { /* From lower to upper */
        y1 = from_chan.top();
        y2 = to_chan.bottom();
    }

    /* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
     * make sure the drawing is symmetric in the from rr and to rr so the edges *
     * will be drawn on top of each other for bidirectional connections.        */

    /* UDSD Modification by WMF Begin */
    else {
        if (rr_graph.node_direction(to_node) != Direction::BIDIR) {
            if (to_track % 2 == 0) { /* INC wire starts at bottom edge */

                y2 = to_chan.bottom();
                /* since no U-turns from_track must be INC as well */
                y1 = draw_coords->tile_y[to_ylow - 1]
                     + draw_coords->get_tile_width();
            } else { /* DEC wire starts at top edge */

                y2 = to_chan.top();
                y1 = draw_coords->tile_y[to_yhigh + 1];
            }
        } else {
            if (to_ylow < from_ylow) { /* Draw from bottom edge of one to other. */
                y1 = from_chan.bottom();
                y2 = draw_coords->tile_y[from_ylow - 1]
                     + draw_coords->get_tile_width();
            } else if (from_ylow < to_ylow) {
                y1 = draw_coords->tile_y[to_ylow - 1]
                     + draw_coords->get_tile_width();
                y2 = to_chan.bottom();
            } else if (to_yhigh > from_yhigh) { /* Draw from top edge of one to other. */
                y1 = from_chan.top();
                y2 = draw_coords->tile_y[from_yhigh + 1];
            } else if (from_yhigh > to_yhigh) {
                y1 = draw_coords->tile_y[to_yhigh + 1];
                y2 = to_chan.top();
            } else { /* Complete overlap: start and end both align. Draw outside the sbox */
                y1 = from_chan.bottom();
                y2 = from_chan.bottom() + draw_coords->get_tile_width();
            }
        }
    }

    /* UDSD Modification by WMF End */
    g->draw_line({x1, y1}, {x2, y2});

    if (draw_state->draw_rr_toggle == DRAW_ALL_RR
        || draw_state->draw_rr_node[size_t(from_node)].node_highlighted) {
        draw_rr_switch(x1, y1, x2, y2,
                       rr_graph.rr_switch_inf(RRSwitchId(switch_type)).buffered(),
                       rr_graph.rr_switch_inf(RRSwitchId(switch_type)).configurable(), g);
    }
}

void draw_chanx_to_chanx_edge(RRNodeId from_node, RRNodeId to_node, int to_track, short switch_type, ezgl::renderer* g) {
    /* Draws a connection between two x-channel segments.  Passing in the track *
     * numbers allows this routine to be used for both rr_graph and routing     *
     * drawing->                                                                 */

    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    float x1, x2, y1, y2;
    ezgl::rectangle from_chan;
    ezgl::rectangle to_chan;
    int from_xlow, to_xlow, from_xhigh, to_xhigh;

    // Get the coordinates of the channel wires.
    from_chan = draw_get_rr_chan_bbox(size_t(from_node));
    to_chan = draw_get_rr_chan_bbox(size_t(to_node));

    /* (x1, y1) point on from_node, (x2, y2) point on to_node. */

    y1 = from_chan.bottom();
    y2 = to_chan.bottom();

    from_xlow = rr_graph.node_xlow(from_node);
    from_xhigh = rr_graph.node_xhigh(from_node);
    to_xlow = rr_graph.node_xlow(to_node);
    to_xhigh = rr_graph.node_xhigh(to_node);
    if (to_xhigh < from_xlow) { /* From right to left */
        /* UDSD Note by WMF: could never happen for INC wires, unless U-turn. For DEC
         * wires this handles well */
        x1 = from_chan.left();
        x2 = to_chan.right();
    } else if (to_xlow > from_xhigh) { /* From left to right */
        /* UDSD Note by WMF: could never happen for DEC wires, unless U-turn. For INC
         * wires this handles well */
        x1 = from_chan.right();
        x2 = to_chan.left();
    }

    /* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
     * make sure the drawing is symmetric in the from rr and to rr so the edges *
     * will be drawn on top of each other for bidirectional connections.        */

    else {
        if (rr_graph.node_direction(to_node) != Direction::BIDIR) {
            /* must connect to to_node's wire beginning at x2 */
            if (to_track % 2 == 0) { /* INC wire starts at leftmost edge */
                VTR_ASSERT(from_xlow < to_xlow);
                x2 = to_chan.left();
                /* since no U-turns from_track must be INC as well */
                x1 = draw_coords->tile_x[to_xlow - 1]
                     + draw_coords->get_tile_width();
            } else { /* DEC wire starts at rightmost edge */
                VTR_ASSERT(from_xhigh > to_xhigh);
                x2 = to_chan.right();
                x1 = draw_coords->tile_x[to_xhigh + 1];
            }
        } else {
            if (to_xlow < from_xlow) { /* Draw from left edge of one to other */
                x1 = from_chan.left();
                x2 = draw_coords->tile_x[from_xlow - 1]
                     + draw_coords->get_tile_width();
            } else if (from_xlow < to_xlow) {
                x1 = draw_coords->tile_x[to_xlow - 1]
                     + draw_coords->get_tile_width();
                x2 = to_chan.left();

            }                                 /* The following then is executed when from_xlow == to_xlow */
            else if (to_xhigh > from_xhigh) { /* Draw from right edge of one to other */
                x1 = from_chan.right();
                x2 = draw_coords->tile_x[from_xhigh + 1];
            } else if (from_xhigh > to_xhigh) {
                x1 = draw_coords->tile_x[to_xhigh + 1];
                x2 = to_chan.right();
            } else { /* Complete overlap: start and end both align. Draw outside the sbox */
                x1 = from_chan.left();
                x2 = from_chan.left() + draw_coords->get_tile_width();
            }
        }
    }

    g->draw_line({x1, y1}, {x2, y2});

    if (draw_state->draw_rr_toggle == DRAW_ALL_RR
        || draw_state->draw_rr_node[size_t(from_node)].node_highlighted) {
        draw_rr_switch(x1, y1, x2, y2,
                       rr_graph.rr_switch_inf(RRSwitchId(switch_type)).buffered(),
                       rr_graph.rr_switch_inf(RRSwitchId(switch_type)).configurable(), g);
    }
}

void draw_chanx_to_chany_edge(int chanx_node, int chanx_track, int chany_node, int chany_track, enum e_edge_dir edge_dir, short switch_type, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    /* Draws an edge (SBOX connection) between an x-directed channel and a    *
     * y-directed channel.                                                    */

    float x1, y1, x2, y2;
    ezgl::rectangle chanx_bbox;
    ezgl::rectangle chany_bbox;
    int chanx_xlow, chany_x, chany_ylow, chanx_y;

    /* Get the coordinates of the CHANX and CHANY segments. */
    chanx_bbox = draw_get_rr_chan_bbox(chanx_node);
    chany_bbox = draw_get_rr_chan_bbox(chany_node);

    /* (x1,y1): point on CHANX segment, (x2,y2): point on CHANY segment. */

    y1 = chanx_bbox.bottom();
    x2 = chany_bbox.left();

    chanx_xlow = rr_graph.node_xlow(RRNodeId(chanx_node));
    chanx_y = rr_graph.node_ylow(RRNodeId(chanx_node));
    chany_x = rr_graph.node_xlow(RRNodeId(chany_node));
    chany_ylow = rr_graph.node_ylow(RRNodeId(chany_node));

    if (chanx_xlow <= chany_x) { /* Can draw connection going right */
        /* Connection not at end of the CHANX segment. */
        x1 = draw_coords->tile_x[chany_x] + draw_coords->get_tile_width();

        if (rr_graph.node_direction(RRNodeId(chanx_node)) != Direction::BIDIR) {
            if (edge_dir == FROM_X_TO_Y) {
                if ((chanx_track % 2) == 1) { /* If dec wire, then going left */
                    x1 = draw_coords->tile_x[chany_x + 1];
                }
            }
        }

    } else { /* Must draw connection going left. */
        x1 = chanx_bbox.left();
    }

    if (chany_ylow <= chanx_y) { /* Can draw connection going up. */
        /* Connection not at end of the CHANY segment. */
        y2 = draw_coords->tile_y[chanx_y] + draw_coords->get_tile_width();

        if (rr_graph.node_direction(RRNodeId(chany_node)) != Direction::BIDIR) {
            if (edge_dir == FROM_Y_TO_X) {
                if ((chany_track % 2) == 1) { /* If dec wire, then going down */
                    y2 = draw_coords->tile_y[chanx_y + 1];
                }
            }
        }

    } else { /* Must draw connection going down. */
        y2 = chany_bbox.bottom();
    }

    g->draw_line({x1, y1}, {x2, y2});

    if (draw_state->draw_rr_toggle == DRAW_ALL_RR
        || draw_state->draw_rr_node[chanx_node].node_highlighted) {
        if (edge_dir == FROM_X_TO_Y) {
            draw_rr_switch(x1, y1, x2, y2,
                           rr_graph.rr_switch_inf(RRSwitchId(switch_type)).buffered(),
                           rr_graph.rr_switch_inf(RRSwitchId(switch_type)).configurable(), g);
        } else {
            draw_rr_switch(x2, y2, x1, y1,
                           rr_graph.rr_switch_inf(RRSwitchId(switch_type)).buffered(),
                           rr_graph.rr_switch_inf(RRSwitchId(switch_type)).configurable(), g);
        }
    }
}

void draw_pin_to_pin(int opin_node, int ipin_node, ezgl::renderer* g) {
    /* This routine draws an edge from the opin rr node to the ipin rr node */
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    VTR_ASSERT(rr_graph.node_type(RRNodeId(opin_node)) == OPIN);
    VTR_ASSERT(rr_graph.node_type(RRNodeId(ipin_node)) == IPIN);

    /* FIXME: May use a smarter strategy
     * Currently, we use the last side found for both OPIN and IPIN
     * when draw the direct connection between the two nodes
     * Note: tried first side but see missing connections
     */
    float x1 = 0, y1 = 0;
    std::vector<e_side> opin_candidate_sides;
    for (const e_side& opin_candidate_side : SIDES) {
        if (rr_graph.is_node_on_specific_side(RRNodeId(opin_node), opin_candidate_side)) {
            opin_candidate_sides.push_back(opin_candidate_side);
        }
    }
    VTR_ASSERT(1 <= opin_candidate_sides.size());
    draw_get_rr_pin_coords(opin_node, &x1, &y1, opin_candidate_sides.back());

    float x2 = 0, y2 = 0;
    std::vector<e_side> ipin_candidate_sides;
    for (const e_side& ipin_candidate_side : SIDES) {
        if (rr_graph.is_node_on_specific_side(RRNodeId(ipin_node), ipin_candidate_side)) {
            ipin_candidate_sides.push_back(ipin_candidate_side);
        }
    }
    VTR_ASSERT(1 <= ipin_candidate_sides.size());
    draw_get_rr_pin_coords(ipin_node, &x2, &y2, ipin_candidate_sides.back());

    g->draw_line({x1, y1}, {x2, y2});

    float xend = x2 + (x1 - x2) / 10.;
    float yend = y2 + (y1 - y2) / 10.;
    draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
}

void draw_pin_to_sink(int ipin_node, int sink_node, ezgl::renderer* g) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    float x1 = 0, y1 = 0;
    /* Draw the line for each ipin on different sides */
    for (const e_side& pin_side : SIDES) {
        if (!rr_graph.is_node_on_specific_side(RRNodeId(ipin_node), pin_side)) {
            continue;
        }

        draw_get_rr_pin_coords(ipin_node, &x1, &y1, pin_side);

        float x2 = 0, y2 = 0;
        draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[sink_node], &x2, &y2);

        g->draw_line({x1, y1}, {x2, y2});

        float xend = x2 + (x1 - x2) / 10.;
        float yend = y2 + (y1 - y2) / 10.;
        draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
    }
}

void draw_source_to_pin(int source_node, int opin_node, ezgl::renderer* g) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    float x1 = 0, y1 = 0;
    draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[source_node], &x1, &y1);

    /* Draw the line for each ipin on different sides */
    for (const e_side& pin_side : SIDES) {
        if (!rr_graph.is_node_on_specific_side(RRNodeId(opin_node), pin_side)) {
            continue;
        }

        float x2 = 0, y2 = 0;
        draw_get_rr_pin_coords(opin_node, &x2, &y2, pin_side);

        g->draw_line({x1, y1}, {x2, y2});

        float xend = x2 + (x1 - x2) / 10.;
        float yend = y2 + (y1 - y2) / 10.;
        draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
    }
}

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
