/*draw_rr_edges.cpp contains all functions that draw lines between RR nodes.*/
#ifndef NO_GRAPHICS

#include <algorithm>

#include "physical_types_util.h"
#include "vtr_assert.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_global.h"
#include "draw_basic.h"

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

void draw_chany_to_chany_edge(RRNodeId from_node, RRNodeId to_node, RRSwitchId rr_switch_id, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    // Get the coordinates of the channel wires.
    ezgl::rectangle from_chan = draw_get_rr_chan_bbox(from_node);
    ezgl::rectangle to_chan = draw_get_rr_chan_bbox(to_node);

    int from_ylow = rr_graph.node_ylow(from_node);
    int from_yhigh = rr_graph.node_yhigh(from_node);
    int to_ylow = rr_graph.node_ylow(to_node);
    int to_yhigh = rr_graph.node_yhigh(to_node);

    // (x1, y1) point on from_node, (x2, y2) point on to_node.
    float x1 = from_chan.left();
    float x2 = to_chan.left();

    float y1, y2;
    if (to_yhigh < from_ylow) { // From upper to lower
        y1 = from_chan.bottom();
        y2 = to_chan.top();
    } else if (to_ylow > from_yhigh) { // From lower to upper
        y1 = from_chan.top();
        y2 = to_chan.bottom();
    }
    // Segments overlap in the channel. Figure out the best way to draw. Have to
    // make sure the drawing is symmetric in the from rr and to rr so the edges
    // will be drawn on top of each other for bidirectional connections.
    else {
        if (rr_graph.node_direction(to_node) != Direction::BIDIR) {
            if (rr_graph.node_direction(to_node) == Direction::INC) { // INC wire starts at bottom edge

                y2 = to_chan.bottom();
                // since no U-turns from_tracks must be INC as well
                y1 = draw_coords->tile_y[to_ylow - 1]
                     + draw_coords->get_tile_width();
            } else { // DEC wire starts at top edge

                y2 = to_chan.top();
                y1 = draw_coords->tile_y[to_yhigh + 1];
            }
        } else {
            if (to_ylow < from_ylow) { // Draw from bottom edge of one to other.
                y1 = from_chan.bottom();
                y2 = draw_coords->tile_y[from_ylow - 1]
                     + draw_coords->get_tile_width();
            } else if (from_ylow < to_ylow) {
                y1 = draw_coords->tile_y[to_ylow - 1]
                     + draw_coords->get_tile_width();
                y2 = to_chan.bottom();
            } else if (to_yhigh > from_yhigh) { // Draw from top edge of one to other.
                y1 = from_chan.top();
                y2 = draw_coords->tile_y[from_yhigh + 1];
            } else if (from_yhigh > to_yhigh) {
                y1 = draw_coords->tile_y[to_yhigh + 1];
                y2 = to_chan.top();
            } else { // Complete overlap: start and end both align. Draw outside the sbox
                y1 = from_chan.bottom();
                y2 = from_chan.bottom() + draw_coords->get_tile_width();
            }
        }
    }

    g->draw_line({x1, y1}, {x2, y2});

    draw_rr_switch(x1, y1, x2, y2,
                   rr_graph.rr_switch_inf(rr_switch_id).buffered(),
                   rr_graph.rr_switch_inf(rr_switch_id).configurable(), g);
}

void draw_chanx_to_chanx_edge(RRNodeId from_node, RRNodeId to_node, RRSwitchId rr_switch_id, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    // Get the coordinates of the channel wires.
    ezgl::rectangle from_chan = draw_get_rr_chan_bbox(from_node);
    ezgl::rectangle to_chan = draw_get_rr_chan_bbox(to_node);

    // (x1, y1) point on from_node, (x2, y2) point on to_node.
    float x1, x2;
    float y1 = from_chan.bottom();
    float y2 = to_chan.bottom();

    int from_xlow = rr_graph.node_xlow(from_node);
    int from_xhigh = rr_graph.node_xhigh(from_node);
    int to_xlow = rr_graph.node_xlow(to_node);
    int to_xhigh = rr_graph.node_xhigh(to_node);
    if (to_xhigh < from_xlow) { /* From right to left */
        // Could never happen for INC wires, unless U-turn. For DEC wires this handles well
        x1 = from_chan.left();
        x2 = to_chan.right();
    } else if (to_xlow > from_xhigh) { /* From left to right */
        // Could never happen for DEC wires, unless U-turn. For INC wires this handles well
        x1 = from_chan.right();
        x2 = to_chan.left();
    }
    /* Segments overlap in the channel.  Figure out best way to draw.  Have to  *
     * make sure the drawing is symmetric in the from rr and to rr so the edges *
     * will be drawn on top of each other for bidirectional connections.        */
    else {
        if (rr_graph.node_direction(to_node) != Direction::BIDIR) {
            // must connect to to_node's wire beginning at x2
            if (rr_graph.node_direction(to_node) == Direction::INC) { // INC wire starts at leftmost edge
                VTR_ASSERT(from_xlow < to_xlow);
                x2 = to_chan.left();
                // since no U-turns from_tracks must be INC as well
                x1 = draw_coords->tile_x[to_xlow - 1]
                     + draw_coords->get_tile_width();
            } else { // DEC wire starts at rightmost edge
                VTR_ASSERT(from_xhigh > to_xhigh);
                x2 = to_chan.right();
                x1 = draw_coords->tile_x[to_xhigh + 1];
            }
        } else {
            if (to_xlow < from_xlow) { // Draw from left edge of one to other
                x1 = from_chan.left();
                x2 = draw_coords->tile_x[from_xlow - 1]
                     + draw_coords->get_tile_width();
            } else if (from_xlow < to_xlow) {
                x1 = draw_coords->tile_x[to_xlow - 1]
                     + draw_coords->get_tile_width();
                x2 = to_chan.left();

            } // The following then is executed when from_xlow == to_xlow
            else if (to_xhigh > from_xhigh) { // Draw from right edge of one to other
                x1 = from_chan.right();
                x2 = draw_coords->tile_x[from_xhigh + 1];
            } else if (from_xhigh > to_xhigh) {
                x1 = draw_coords->tile_x[to_xhigh + 1];
                x2 = to_chan.right();
            } else { // Complete overlap: start and end both align. Draw outside the sbox
                x1 = from_chan.left();
                x2 = from_chan.left() + draw_coords->get_tile_width();
            }
        }
    }

    g->draw_line({x1, y1}, {x2, y2});

    draw_rr_switch(x1, y1, x2, y2,
                   rr_graph.rr_switch_inf(rr_switch_id).buffered(),
                   rr_graph.rr_switch_inf(rr_switch_id).configurable(), g);
}

void draw_chanx_to_chany_edge(RRNodeId chanx_node,
                              RRNodeId chany_node,
                              e_chan_edge_dir edge_dir,
                              RRSwitchId rr_switch_id,
                              ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    const t_rr_switch_inf& rr_switch_inf = rr_graph.rr_switch_inf(rr_switch_id);

    // Get the coordinates of the CHANX and CHANY segments.
    ezgl::rectangle chanx_bbox = draw_get_rr_chan_bbox(chanx_node);
    ezgl::rectangle chany_bbox = draw_get_rr_chan_bbox(chany_node);

    // (x1,y1): point on CHANX segment, (x2,y2): point on CHANY segment.
    float y1 = chanx_bbox.bottom();
    float x2 = chany_bbox.left();
    float x1, y2;

    // these values xhigh/low yhigh/low mark the coordinates for the beginning and ends of the wire.
    int chanx_xlow = rr_graph.node_xlow(chanx_node);
    int chanx_y = rr_graph.node_ylow(chanx_node);
    int chany_x = rr_graph.node_xlow(chany_node);
    int chany_ylow = rr_graph.node_ylow(chany_node);

    if (chanx_xlow <= chany_x) { // Can draw connection going right
        // Connection not at end of the CHANX segment.
        x1 = draw_coords->tile_x[chany_x] + draw_coords->get_tile_width();
        if (rr_graph.node_direction(chanx_node) != Direction::BIDIR && rr_switch_inf.type() != e_switch_type::SHORT) {
            if (edge_dir == e_chan_edge_dir::FROM_X_TO_Y) {
                if (rr_graph.node_direction(chanx_node) == Direction::DEC) { // If dec wire, then going left
                    x1 = draw_coords->tile_x[chany_x + 1];
                }
            }
        }
    } else { // Must draw connection going left.
        x1 = chanx_bbox.left();
    }
    if (chany_ylow <= chanx_y) { // Can draw connection going up.
        // Connection not at end of the CHANY segment.
        y2 = draw_coords->tile_y[chanx_y] + draw_coords->get_tile_width();

        if (rr_graph.node_direction(chany_node) != Direction::BIDIR && rr_switch_inf.type() != e_switch_type::SHORT) {
            if (edge_dir == e_chan_edge_dir::FROM_Y_TO_X) {
                if (rr_graph.node_direction(chany_node) == Direction::DEC) { // If dec wire, then going down
                    y2 = draw_coords->tile_y[chanx_y + 1];
                }
            }
        }

    } else { // Must draw connection going down.
        y2 = chany_bbox.bottom();
    }

    g->draw_line({x1, y1}, {x2, y2});

    if (edge_dir == e_chan_edge_dir::FROM_X_TO_Y) {
        draw_rr_switch(x1, y1, x2, y2,
                       rr_switch_inf.buffered(),
                       rr_switch_inf.configurable(), g);
    } else {
        draw_rr_switch(x2, y2, x1, y1,
                       rr_switch_inf.buffered(),
                       rr_switch_inf.configurable(), g);
    }
}

void draw_intra_cluster_edge(RRNodeId inode, RRNodeId prev_node, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    auto [blk_id, pin_id] = get_rr_node_cluster_blk_id_pb_graph_pin(inode);
    auto [prev_blk_id, prev_pin_id] = get_rr_node_cluster_blk_id_pb_graph_pin(prev_node);

    ezgl::point2d icoord = draw_coords->get_absolute_pin_location(blk_id, pin_id);
    ezgl::point2d prev_coord = draw_coords->get_absolute_pin_location(prev_blk_id, prev_pin_id);

    g->draw_line(prev_coord, icoord);

    float triangle_coord_x = icoord.x + (prev_coord.x - icoord.x) / 10.;
    float triangle_coord_y = icoord.y + (prev_coord.y - icoord.y) / 10.;
    draw_triangle_along_line(g, triangle_coord_x, triangle_coord_y, prev_coord.x, icoord.x, prev_coord.y, icoord.y);
}

void draw_intra_cluster_pin_to_pin(RRNodeId intra_cluster_node, RRNodeId inter_cluster_node, e_pin_edge_dir pin_edge_dir, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    for (const e_side pin_side : TOTAL_2D_SIDES) {
        // Draw connections to each side of the inter-cluster node
        if (!rr_graph.is_node_on_specific_side(inter_cluster_node, pin_side)) {
            continue;
        }

        // determine the location of the pins
        float inter_cluster_x, inter_cluster_y;
        ezgl::point2d intra_cluster_coord;

        draw_get_rr_pin_coords(inter_cluster_node, &inter_cluster_x, &inter_cluster_y, pin_side);

        auto [blk_id, pin_id] = get_rr_node_cluster_blk_id_pb_graph_pin(intra_cluster_node);
        intra_cluster_coord = draw_coords->get_absolute_pin_location(blk_id, pin_id);

        // determine which coord is first based on the pin edge direction
        ezgl::point2d prev_coord, icoord;
        if (pin_edge_dir == FROM_INTRA_CLUSTER_TO_INTER_CLUSTER) {
            prev_coord = intra_cluster_coord;
            icoord = {inter_cluster_x, inter_cluster_y};
        } else if (pin_edge_dir == FROM_INTER_CLUSTER_TO_INTRA_CLUSTER) {
            prev_coord = {inter_cluster_x, inter_cluster_y};
            icoord = intra_cluster_coord;
        } else {
            VPR_ERROR(VPR_ERROR_DRAW, "Invalid pin edge direction: %d", pin_edge_dir);
        }

        g->draw_line(prev_coord, icoord);
        float triangle_coord_x = icoord.x + (prev_coord.x - icoord.x) / 10.;
        float triangle_coord_y = icoord.y + (prev_coord.y - icoord.y) / 10.;
        draw_triangle_along_line(g, triangle_coord_x, triangle_coord_y, prev_coord.x, icoord.x, prev_coord.y, icoord.y);
    }
}

void draw_pin_to_pin(RRNodeId opin_node, RRNodeId ipin_node, ezgl::renderer* g) {
    /* This routine draws an edge from the opin rr node to the ipin rr node */
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    VTR_ASSERT(rr_graph.node_type(opin_node) == e_rr_type::OPIN);
    VTR_ASSERT(rr_graph.node_type(ipin_node) == e_rr_type::IPIN);

    /* FIXME: May use a smarter strategy
     * Currently, we use the last side found for both OPIN and IPIN
     * when draw the direct connection between the two nodes
     * Note: tried first side but see missing connections
     */
    float x1 = 0, y1 = 0;
    std::vector<e_side> opin_candidate_sides;
    for (const e_side opin_candidate_side : TOTAL_2D_SIDES) {
        if (rr_graph.is_node_on_specific_side(opin_node, opin_candidate_side)) {
            opin_candidate_sides.push_back(opin_candidate_side);
        }
    }
    VTR_ASSERT(1 <= opin_candidate_sides.size());
    draw_get_rr_pin_coords(opin_node, &x1, &y1, opin_candidate_sides.back());

    float x2 = 0, y2 = 0;
    std::vector<e_side> ipin_candidate_sides;
    for (const e_side ipin_candidate_side : TOTAL_2D_SIDES) {
        if (rr_graph.is_node_on_specific_side(ipin_node, ipin_candidate_side)) {
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

void draw_pin_to_sink(RRNodeId ipin_node, RRNodeId sink_node, ezgl::renderer* g) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    float x1 = 0, y1 = 0;
    /* Draw the line for each ipin on different sides */
    for (const e_side pin_side : TOTAL_2D_SIDES) {
        if (!rr_graph.is_node_on_specific_side(ipin_node, pin_side)) {
            continue;
        }

        draw_get_rr_pin_coords(ipin_node, &x1, &y1, pin_side);

        float x2 = 0, y2 = 0;
        draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[size_t(sink_node)], &x2, &y2);

        g->draw_line({x1, y1}, {x2, y2});

        float xend = x2 + (x1 - x2) / 10.;
        float yend = y2 + (y1 - y2) / 10.;
        draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
    }
}

void draw_source_to_pin(RRNodeId source_node, RRNodeId opin_node, ezgl::renderer* g) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    float x1 = 0, y1 = 0;
    draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[size_t(source_node)], &x1, &y1);

    /* Draw the line for each ipin on different sides */
    for (const e_side pin_side : TOTAL_2D_SIDES) {
        if (!rr_graph.is_node_on_specific_side(opin_node, pin_side)) {
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

e_side get_pin_side(RRNodeId pin_node, RRNodeId chan_node) {

    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    t_physical_tile_loc tile_loc = {
        rr_graph.node_xlow(pin_node),
        rr_graph.node_ylow(pin_node),
        rr_graph.node_layer_low(pin_node)};

    const auto& grid_type = device_ctx.grid.get_physical_type(tile_loc);
    int width_offset = device_ctx.grid.get_width_offset(tile_loc);
    int height_offset = device_ctx.grid.get_height_offset(tile_loc);

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
    for (const e_side pin_candidate_side : TOTAL_2D_SIDES) {
        if ((rr_graph.is_node_on_specific_side(pin_node, pin_candidate_side))
            && (grid_type->pinloc[width_offset][height_offset][pin_candidate_side][rr_graph.node_pin_num(pin_node)])) {
            pin_candidate_sides.push_back(pin_candidate_side);
        }
    }
    /* Only 1 side will be picked in the end
     * Any rr_node of a grid should have at least 1 side!!!
     */
    e_side pin_side = NUM_2D_SIDES;
    const e_rr_type channel_type = rr_graph.node_type(chan_node);
    if (1 == pin_candidate_sides.size()) {
        pin_side = pin_candidate_sides[0];
    } else {
        VTR_ASSERT(1 < pin_candidate_sides.size());
        if (e_rr_type::CHANX == channel_type && rr_graph.node_ylow(pin_node) <= rr_graph.node_ylow(chan_node)) {
            pin_side = TOP;
        } else if (e_rr_type::CHANX == channel_type && rr_graph.node_ylow(pin_node) - 1 >= rr_graph.node_ylow(chan_node)) {
            pin_side = BOTTOM;
        } else if (e_rr_type::CHANY == channel_type && rr_graph.node_xlow(pin_node) <= rr_graph.node_xlow(chan_node)) {
            pin_side = RIGHT;
        } else if (e_rr_type::CHANY == channel_type && rr_graph.node_xlow(pin_node) - 1 >= rr_graph.node_xlow(chan_node)) {
            pin_side = LEFT;
        }
        /* The inferred side must be in the list of sides of the pin rr_node!!! */
        VTR_ASSERT(pin_candidate_sides.end() != std::find(pin_candidate_sides.begin(), pin_candidate_sides.end(), pin_side));
    }
    /* Sanity check */
    VTR_ASSERT(NUM_2D_SIDES != pin_side);

    return pin_side;
}

void draw_pin_to_chan_edge(RRNodeId pin_node, RRNodeId chan_node, ezgl::renderer* g) {
    /* This routine draws an edge from the pin_node to the chan_node (CHANX or   *
     * CHANY).  The connection is made to the nearest end of the track instead   *
     * of perpendicular to the track to symbolize a single-drive connection.     */

    /* TODO: Fix this for global routing, currently for detailed only */

    t_draw_coords* draw_coords = get_draw_coords_vars();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;

    t_physical_tile_loc tile_loc = {
        rr_graph.node_xlow(pin_node),
        rr_graph.node_ylow(pin_node),
        rr_graph.node_layer_low(pin_node)};

    const auto& grid_type = device_ctx.grid.get_physical_type(tile_loc);
    const e_rr_type channel_type = rr_graph.node_type(chan_node);
    e_side pin_side = get_pin_side(pin_node, chan_node);

    float x1 = 0, y1 = 0;
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
    const Direction chan_rr_direction = rr_graph.node_direction(chan_node);
    switch (channel_type) {
        case e_rr_type::CHANX: {
            y1 += draw_pin_offset;
            y2 = chan_bbox.bottom();
            x2 = x1;
            if (is_opin(rr_graph.node_pin_num(pin_node), grid_type)) {
                if (chan_rr_direction == Direction::INC) {
                    x2 = chan_bbox.left();
                } else if (chan_rr_direction == Direction::DEC) {
                    x2 = chan_bbox.right();
                }
            }
            break;
        }
        case e_rr_type::CHANY: {
            x1 += draw_pin_offset;
            x2 = chan_bbox.left();
            y2 = y1;
            if (is_opin(rr_graph.node_pin_num(pin_node), grid_type)) {
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
    if (chan_rr_direction == Direction::BIDIR || !is_opin(rr_graph.node_pin_num(pin_node), grid_type)) {
        draw_x(x2, y2, 0.7 * draw_coords->pin_size, g);
    } else {
        float xend = x2 + (x1 - x2) / 10.;
        float yend = y2 + (y1 - y2) / 10.;
        draw_triangle_along_line(g, xend, yend, x1, x2, y1, y2);
    }
}

void draw_rr_edge(RRNodeId inode, RRNodeId prev_node, ezgl::color color, ezgl::renderer* g) {
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const RRGraphView& rr_graph = device_ctx.rr_graph;
    t_draw_state* draw_state = get_draw_state_vars();

    e_rr_type rr_type = rr_graph.node_type(inode);
    bool inode_inter_cluster = is_inter_cluster_node(rr_graph, inode);
    int current_node_layer = rr_graph.node_layer_low(inode);

    e_rr_type prev_type = rr_graph.node_type(prev_node);
    bool prev_node_inter_cluster = is_inter_cluster_node(rr_graph, prev_node);
    int prev_node_layer = rr_graph.node_layer_low(prev_node);

    t_draw_layer_display edge_visibility = get_element_visibility_and_transparency(prev_node_layer, current_node_layer);

    // For 3D architectures, draw only visible layers
    if (!draw_state->draw_layer_display[current_node_layer].visible || !edge_visibility.visible) {
        return;
    }

    // Skip drawing edges to or from sources and sinks
    if (rr_type == e_rr_type::SINK || rr_type == e_rr_type::SOURCE || prev_type == e_rr_type::SINK || prev_type == e_rr_type::SOURCE) {
        return;
    }

    g->set_color(color, edge_visibility.alpha);

    if (!inode_inter_cluster && !prev_node_inter_cluster) {
        draw_intra_cluster_edge(inode, prev_node, g);
        return;
    }

    if (!prev_node_inter_cluster && inode_inter_cluster) {
        draw_intra_cluster_pin_to_pin(prev_node, inode, FROM_INTRA_CLUSTER_TO_INTER_CLUSTER, g);
        return;
    }

    if (prev_node_inter_cluster && !inode_inter_cluster) {
        draw_intra_cluster_pin_to_pin(inode, prev_node, FROM_INTER_CLUSTER_TO_INTRA_CLUSTER, g);
        return;
    }

    draw_inter_cluster_rr_edge(inode, prev_node, rr_type, prev_type, g);
}

void draw_inter_cluster_rr_edge(RRNodeId inode, RRNodeId prev_node, e_rr_type rr_type, e_rr_type prev_type, ezgl::renderer* g) {
    const RRGraphView& rr_graph = g_vpr_ctx.device().rr_graph;
    t_edge_size iedge = find_edge(prev_node, inode);
    RRSwitchId rr_switch_id = (RRSwitchId)rr_graph.edge_switch(prev_node, iedge);

    switch (rr_type) {
        case e_rr_type::IPIN:
            if (prev_type == e_rr_type::OPIN) {
                draw_pin_to_pin(prev_node, inode, g);
            } else {
                draw_pin_to_chan_edge(inode, prev_node, g);
            }
            break;

        case e_rr_type::CHANX:
            switch (prev_type) {
                case e_rr_type::CHANX:
                    draw_chanx_to_chanx_edge(prev_node, inode, rr_switch_id, g);
                    break;

                case e_rr_type::CHANY:
                    draw_chanx_to_chany_edge(inode, prev_node, e_chan_edge_dir::FROM_Y_TO_X, rr_switch_id, g);
                    break;

                case e_rr_type::OPIN:
                    draw_pin_to_chan_edge(prev_node, inode, g);
                    break;

                default:
                    VPR_ERROR(VPR_ERROR_OTHER,
                              "Unexpected connection from an rr_node of type %d to one of type %d.\n",
                              prev_type, rr_type);
            }

            break;

        case e_rr_type::CHANY:
            switch (prev_type) {
                case e_rr_type::CHANX:
                    draw_chanx_to_chany_edge(prev_node, inode, e_chan_edge_dir::FROM_X_TO_Y, rr_switch_id, g);
                    break;

                case e_rr_type::CHANY:
                    draw_chany_to_chany_edge(prev_node, inode, rr_switch_id, g);
                    break;

                case e_rr_type::OPIN:
                    draw_pin_to_chan_edge(prev_node, inode, g);

                    break;

                default:
                    VPR_ERROR(VPR_ERROR_OTHER,
                              "Unexpected connection from an rr_node of type %d to one of type %d.\n",
                              prev_type, rr_type);
            }

            break;

        default:
            break;
    }
}

#endif
