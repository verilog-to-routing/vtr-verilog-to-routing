/*draw_rr.cpp contains all functions that relate to drawing routing resources.*/
#ifndef NO_GRAPHICS

#include <cstdio>
#include <cmath>
#include <algorithm>

#include "rr_graph_fwd.h"
#include "vtr_assert.h"
#include "vtr_color_map.h"

#include "vpr_utils.h"
#include "vpr_error.h"

#include "globals.h"
#include "draw_color.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_rr_edges.h"
#include "draw_basic.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "draw_global.h"
#include "search_bar.h"

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

/****************************** Define Macros *******************************/
#define DEFAULT_RR_NODE_COLOR ezgl::BLACK

//The arrow head position for turning/straight-thru connections in a switch box
constexpr float SB_EDGE_TURN_ARROW_POSITION = 0.2;
constexpr float SB_EDGE_STRAIGHT_ARROW_POSITION = 0.95;

/* Draws the routing resources that exist in the FPGA, if the user wants
 * them drawn.
 */
void draw_rr(ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    if (draw_state->draw_rr_toggle == DRAW_NO_RR) {
        g->set_line_width(3);
        drawroute(HIGHLIGHTED, g);
        g->set_line_width(0);
        return;
    }

    g->set_line_dash(ezgl::line_dash::none);

    for (const RRNodeId inode : device_ctx.rr_graph.nodes()) {
        int layer_num = rr_graph.node_layer(inode);
        int transparency_factor = get_rr_node_transparency(inode);
        if (!draw_state->draw_rr_node[inode].node_highlighted) {
            /* If not highlighted node, assign color based on type. */
            switch (rr_graph.node_type(inode)) {
                case e_rr_type::CHANX:
                case e_rr_type::CHANY:
                    draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
                    break;
                case e_rr_type::OPIN:
                    draw_state->draw_rr_node[inode].color = ezgl::PINK;
                    break;
                case e_rr_type::IPIN:
                    draw_state->draw_rr_node[inode].color = blk_LIGHTSKYBLUE;
                    break;
                case e_rr_type::SOURCE:
                    draw_state->draw_rr_node[inode].color = ezgl::PLUM;
                    break;
                case e_rr_type::SINK:
                    draw_state->draw_rr_node[inode].color = ezgl::DARK_SLATE_BLUE;
                    break;
                default:
                    break;
            }
        }

        draw_state->draw_rr_node[inode].color.alpha = transparency_factor;

        if (!draw_state->draw_layer_display[layer_num].visible)
            continue; // skip drawing if layer is not visible

        /* Now call drawing routines to draw the node. */
        switch (rr_graph.node_type(inode)) {
            case e_rr_type::SINK:
                draw_rr_src_sink(inode, draw_state->draw_rr_node[inode].color, g);
                break;
            case e_rr_type::SOURCE:
                draw_rr_edges(inode, g);
                draw_rr_src_sink(inode, draw_state->draw_rr_node[inode].color, g);
                break;

            case e_rr_type::CHANX:
                draw_rr_chan(inode, draw_state->draw_rr_node[inode].color, g);
                draw_rr_edges(inode, g);
                break;

            case e_rr_type::CHANY:
                draw_rr_chan(inode, draw_state->draw_rr_node[inode].color, g);
                draw_rr_edges(inode, g);
                break;

            case e_rr_type::IPIN:
                draw_cluster_pin(inode, draw_state->draw_rr_node[inode].color, g);
                draw_rr_edges(inode, g);
                break;

            case e_rr_type::OPIN:
                draw_cluster_pin(inode, draw_state->draw_rr_node[inode].color, g);
                draw_rr_edges(inode, g);
                break;

            default:
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                          "in draw_rr: Unexpected rr_node type: %d.\n", rr_graph.node_type(inode));
        }
    }

    drawroute(HIGHLIGHTED, g);
}

void draw_rr_chan(RRNodeId inode, const ezgl::color color, ezgl::renderer* g) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int transparency_factor = get_rr_node_transparency(inode);

    e_rr_type type = rr_graph.node_type(inode);

    VTR_ASSERT(type == e_rr_type::CHANX || type == e_rr_type::CHANY);

    ezgl::rectangle bound_box = draw_get_rr_chan_bbox(inode);
    Direction dir = rr_graph.node_direction(inode);

    //We assume increasing direction, and swap if needed
    ezgl::point2d start = bound_box.bottom_left();
    ezgl::point2d end = bound_box.top_right();
    if (dir == Direction::DEC) {
        std::swap(start, end);
    }

    g->set_color(color, transparency_factor);
    if (color != DEFAULT_RR_NODE_COLOR) {
        // If wire is highlighted, then draw with thicker linewidth.
        g->set_line_width(3);
    }

    g->draw_line(start, end);

    if (color != DEFAULT_RR_NODE_COLOR) {
        // Revert width change
        g->set_line_width(0);
    }

    e_side mux_dir = TOP;
    int coord_min = -1;
    int coord_max = -1;
    if (type == e_rr_type::CHANX) {
        coord_min = rr_graph.node_xlow(inode);
        coord_max = rr_graph.node_xhigh(inode);
        if (dir == Direction::INC) {
            mux_dir = RIGHT;
        } else {
            mux_dir = LEFT;
        }
    } else {
        VTR_ASSERT(type == e_rr_type::CHANY);
        coord_min = rr_graph.node_ylow(inode);
        coord_max = rr_graph.node_yhigh(inode);
        if (dir == Direction::INC) {
            mux_dir = TOP;
        } else {
            mux_dir = BOTTOM;
        }
    }

    //Draw direction indicators at the boundary of each switch block, and label them
    //with the corresponding switch point (see build_switchblocks.c for a description of switch points)
    t_draw_coords* draw_coords = get_draw_coords_vars();
    float arrow_offset = DEFAULT_ARROW_SIZE / 2;
    ezgl::color arrow_color = blk_LIGHTGREY;
    ezgl::color text_color = ezgl::BLACK;
    for (int k = coord_min; k <= coord_max; ++k) {
        int switchpoint_min = -1;
        int switchpoint_max = -1;
        if (dir == Direction::INC) {
            switchpoint_min = k - coord_min;
            switchpoint_max = switchpoint_min + 1;
        } else {
            switchpoint_min = (coord_max + 1) - k;
            switchpoint_max = switchpoint_min - 1;
        }

        ezgl::point2d arrow_loc_min(0, 0);
        ezgl::point2d arrow_loc_max(0, 0);
        if (type == e_rr_type::CHANX) {
            float sb_xmin = draw_coords->tile_x[k];
            arrow_loc_min = {sb_xmin + arrow_offset, start.y};

            float sb_xmax = draw_coords->tile_x[k] + draw_coords->get_tile_width();
            arrow_loc_max = {sb_xmax - arrow_offset, start.y};

        } else {
            float sb_ymin = draw_coords->tile_y[k];
            arrow_loc_min = {start.x, sb_ymin + arrow_offset};

            float sb_ymax = draw_coords->tile_y[k] + draw_coords->get_tile_height();
            arrow_loc_max = {start.x, sb_ymax - arrow_offset};
        }

        if (switchpoint_min == 0) {
            if (dir != Direction::BIDIR) {
                //Draw a mux at the start of each wire, labelled with it's size (#inputs)
                draw_mux_with_size(start, mux_dir, WIRE_DRAWING_WIDTH, rr_graph.node_fan_in(inode), transparency_factor, g);
            }
        } else {
            //Draw arrows and label with switch point
            if (k == coord_min) {
                std::swap(arrow_color, text_color);
            }

            g->set_color(arrow_color, transparency_factor);
            draw_triangle_along_line(g, arrow_loc_min, start, end);

            g->set_color(text_color, transparency_factor);
            ezgl::rectangle bbox(ezgl::point2d(arrow_loc_min.x - DEFAULT_ARROW_SIZE / 2, arrow_loc_min.y - DEFAULT_ARROW_SIZE / 4),
                                 ezgl::point2d(arrow_loc_min.x + DEFAULT_ARROW_SIZE / 2, arrow_loc_min.y + DEFAULT_ARROW_SIZE / 4));
            ezgl::point2d center = bbox.center();
            g->draw_text(center, std::to_string(switchpoint_min), bbox.width(), bbox.height());

            if (k == coord_min) {
                //Revert
                std::swap(arrow_color, text_color);
            }
        }

        if (switchpoint_max == 0) {
            if (dir != Direction::BIDIR) {
                //Draw a mux at the start of each wire, labelled with it's size (#inputs)
                draw_mux_with_size(start, mux_dir, WIRE_DRAWING_WIDTH, rr_graph.node_fan_in(inode), transparency_factor, g);
            }
        } else {
            //Draw arrows and label with switch point
            if (k == coord_max) {
                std::swap(arrow_color, text_color);
            }

            g->set_color(arrow_color, transparency_factor);
            draw_triangle_along_line(g, arrow_loc_max, start, end);

            g->set_color(text_color, transparency_factor);
            ezgl::rectangle bbox(ezgl::point2d(arrow_loc_max.x - DEFAULT_ARROW_SIZE / 2, arrow_loc_max.y - DEFAULT_ARROW_SIZE / 4),
                                 ezgl::point2d(arrow_loc_max.x + DEFAULT_ARROW_SIZE / 2, arrow_loc_max.y + DEFAULT_ARROW_SIZE / 4));
            ezgl::point2d center = bbox.center();
            g->draw_text(center, std::to_string(switchpoint_max), bbox.width(), bbox.height());

            if (k == coord_max) {
                //Revert
                std::swap(arrow_color, text_color);
            }
        }
    }
    g->set_color(color, transparency_factor); //Ensure color is still set correctly if we drew any arrows/text
}

/* Draws all the edges that the user wants shown between inode and what it
 * connects to.  inode is assumed to be a CHANX, CHANY, or IPIN.           */
void draw_rr_edges(RRNodeId inode, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto rr_node = inode;

    int transparency_factor = get_rr_node_transparency(rr_node);

    e_rr_type from_type, to_type;
    short switch_type;

    from_type = rr_graph.node_type(rr_node);

    if ((draw_state->draw_rr_toggle == DRAW_NODES_RR)
        || (draw_state->draw_rr_toggle == DRAW_NODES_SBOX_RR && (from_type == e_rr_type::OPIN || from_type == e_rr_type::SOURCE || from_type == e_rr_type::IPIN))
        || (draw_state->draw_rr_toggle == DRAW_NODES_SBOX_CBOX_RR && (from_type == e_rr_type::SOURCE || from_type == e_rr_type::IPIN))) {
        return; /* Nothing to draw. */
    }

    for (t_edge_size iedge = 0, l = rr_graph.num_edges(inode); iedge < l; iedge++) {
        RRNodeId to_node = rr_graph.edge_sink_node(rr_node, iedge);
        to_type = rr_graph.node_type(to_node);
        bool edge_configurable = rr_graph.edge_is_configurable(inode, iedge);

        if (!is_edge_valid_to_draw(RRNodeId(to_node), rr_node))
            continue; // skip drawing if edge is not valid to draw

        switch (from_type) {
            case e_rr_type::OPIN:
                switch (to_type) {
                    case e_rr_type::CHANX:
                    case e_rr_type::CHANY:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            // If OPIN was clicked on, set color to fan-out
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            // If CHANX or CHANY got clicked, set color to fan-in
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(ezgl::PINK, transparency_factor);
                        }
                        draw_pin_to_chan_edge(inode, to_node, g);
                        break;
                    case e_rr_type::IPIN:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(ezgl::MEDIUM_PURPLE, transparency_factor);
                        }
                        draw_pin_to_pin(inode, to_node, g);
                        break;
                    default:
                        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                                  "in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
                                  inode, from_type, to_node, to_type);
                        break;
                }
                break;

            case e_rr_type::CHANX: /* from_type */
                switch (to_type) {
                    case e_rr_type::IPIN:
                        if (draw_state->draw_rr_toggle == DRAW_NODES_SBOX_RR) {
                            break;
                        }

                        if (draw_state->draw_rr_node[to_node].node_highlighted && draw_state->draw_rr_node[inode].color == DEFAULT_RR_NODE_COLOR) {
                            // If the IPIN is clicked on, draw connection to all the CHANX
                            // wire segments fanning into the pin. If a CHANX wire is clicked
                            // on, draw only the connection between that wire and the IPIN, with
                            // the pin fanning out from the wire.
                            break;
                        }

                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(blk_LIGHTSKYBLUE, transparency_factor);
                        }
                        draw_pin_to_chan_edge(to_node, inode, g);
                        break;

                    case e_rr_type::CHANX:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else if (!edge_configurable) {
                            ezgl::color color = blk_DARKGREY;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(blk_DARKGREEN, transparency_factor);
                        }
                        switch_type = rr_graph.edge_switch(rr_node, iedge);
                        draw_chanx_to_chanx_edge(rr_node, RRNodeId(to_node),
                                                 switch_type, g);
                        break;

                    case e_rr_type::CHANY:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else if (!edge_configurable) {
                            g->set_color(blk_DARKGREY, transparency_factor);
                        } else {
                            g->set_color(blk_DARKGREEN, transparency_factor);
                        }
                        switch_type = rr_graph.edge_switch(rr_node, iedge);
                        draw_chanx_to_chany_edge(inode, to_node,
                                                 FROM_X_TO_Y, switch_type, g);
                        break;

                    default:
                        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                                  "in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
                                  inode, from_type, to_node, to_type);
                        break;
                }
                break;

            case e_rr_type::CHANY: /* from_type */
                switch (to_type) {
                    case e_rr_type::IPIN:
                        if (draw_state->draw_rr_toggle == DRAW_NODES_SBOX_RR) {
                            break;
                        }

                        if (draw_state->draw_rr_node[to_node].node_highlighted && draw_state->draw_rr_node[inode].color == DEFAULT_RR_NODE_COLOR) {
                            // If the IPIN is clicked on, draw connection to all the CHANY
                            // wire segments fanning into the pin. If a CHANY wire is clicked
                            // on, draw only the connection between that wire and the IPIN, with
                            // the pin fanning out from the wire.
                            break;
                        }

                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(blk_LIGHTSKYBLUE, transparency_factor);
                        }
                        draw_pin_to_chan_edge(to_node, inode, g);
                        break;

                    case e_rr_type::CHANX:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else if (!edge_configurable) {
                            ezgl::color color = blk_DARKGREY;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(blk_DARKGREEN, transparency_factor);
                        }
                        switch_type = rr_graph.edge_switch(rr_node, iedge);
                        draw_chanx_to_chany_edge(to_node, inode,
                                                 FROM_Y_TO_X, switch_type, g);
                        break;

                    case e_rr_type::CHANY:
                        if (rgb_is_same(draw_state->draw_rr_node[inode].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[to_node].color;
                            g->set_color(color, transparency_factor);
                        } else if (rgb_is_same(draw_state->draw_rr_node[to_node].color, ezgl::MAGENTA)) {
                            ezgl::color color = draw_state->draw_rr_node[inode].color;
                            g->set_color(color, transparency_factor);
                        } else if (!edge_configurable) {
                            ezgl::color color = blk_DARKGREY;
                            g->set_color(color, transparency_factor);
                        } else {
                            g->set_color(blk_DARKGREEN, transparency_factor);
                        }
                        switch_type = rr_graph.edge_switch(rr_node, iedge);
                        draw_chany_to_chany_edge(rr_node, RRNodeId(to_node),
                                                 switch_type, g);
                        break;

                    default:
                        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                                  "in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
                                  inode, from_type, to_node, to_type);
                        break;
                }
                break;
            case e_rr_type::IPIN: // from_type
                switch (to_type) {
                    case e_rr_type::SINK:
                        g->set_color(ezgl::DARK_SLATE_BLUE, transparency_factor);
                        draw_pin_to_sink(inode, to_node, g);
                        break;

                    default:
                        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                                  "in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
                                  inode, from_type, to_node, to_type);
                        break;
                }
                break;
            case e_rr_type::SOURCE: // from_type
                switch (to_type) {
                    case e_rr_type::OPIN:
                        g->set_color(ezgl::PLUM, transparency_factor);
                        draw_source_to_pin(inode, to_node, g);
                        break;

                    default:
                        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                                  "in draw_rr_edges: node %d (type: %d) connects to node %d (type: %d).\n",
                                  inode, from_type, to_node, to_type);
                        break;
                }
                break;
            default: /* from_type */
                vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                          "draw_rr_edges called with node %d of type %d.\n",
                          inode, from_type);
                break;
        }
    } /* End of for each edge loop */
}

void draw_rr_intra_cluster_pin(RRNodeId inode, const ezgl::color& color, ezgl::renderer* g) {
    t_draw_state* draw_state = get_draw_state_vars();
    t_draw_coords* draw_coords = get_draw_coords_vars();

    if (!draw_state->is_flat) {
        return;
    }

    auto [blk_id, pin_id] = get_rr_node_cluster_blk_id_pb_graph_pin(inode);

    ezgl::point2d p = draw_coords->get_absolute_pin_location(blk_id, pin_id);

    int transparency_factor = get_rr_node_transparency(inode);

    g->set_color(color, transparency_factor);
    g->fill_rectangle(
        {p.x - draw_coords->pin_size, p.y - draw_coords->pin_size},
        {p.x + draw_coords->pin_size, p.y + draw_coords->pin_size});
}

/* Draws an IPIN or OPIN rr_node.  Note that the pin can appear on more    *
 * than one side of a clb.  Also note that this routine can change the     *
 * current color to BLACK.                                                 */
void draw_cluster_pin(RRNodeId inode, const ezgl::color& color, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    float xcen, ycen;
    char str[vtr::bufsize];
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    if (!is_inter_cluster_node(rr_graph, inode)) {
        return;
    }

    int ipin = rr_graph.node_pin_num(RRNodeId(inode));

    int transparency_factor = get_rr_node_transparency(inode);

    g->set_color(color, transparency_factor);

    /* TODO: This is where we can hide fringe physical pins and also identify globals (hide, color, show) */
    /* As nodes may appear on more than one side, walk through the possible nodes
     * - draw the pin on each side that it appears
     */
    for (const e_side& pin_side : TOTAL_2D_SIDES) {
        if (!rr_graph.is_node_on_specific_side(RRNodeId(inode), pin_side)) {
            continue;
        }
        draw_get_rr_pin_coords(inode, &xcen, &ycen, pin_side);
        g->fill_rectangle(
            {xcen - draw_coords->pin_size, ycen - draw_coords->pin_size},
            {xcen + draw_coords->pin_size, ycen + draw_coords->pin_size});
        sprintf(str, "%d", ipin);
        g->set_color(ezgl::BLACK, transparency_factor);
        g->draw_text({xcen, ycen}, str, 2 * draw_coords->pin_size,
                     2 * draw_coords->pin_size);
        g->set_color(color, transparency_factor);
    }
}

void draw_rr_src_sink(RRNodeId inode, ezgl::color color, ezgl::renderer* g) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    if (!is_inter_cluster_node(rr_graph, inode)) {
        return;
    }

    int transparency_factor = get_rr_node_transparency(inode);

    float xcen, ycen;
    draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[size_t(inode)], &xcen, &ycen);

    g->set_color(color, transparency_factor);

    g->fill_rectangle(
        {xcen - draw_coords->pin_size, ycen - draw_coords->pin_size},
        {xcen + draw_coords->pin_size, ycen + draw_coords->pin_size});

    std::string str = vtr::string_fmt("%d",
                                      rr_graph.node_class_num(inode));
    g->set_color(ezgl::BLACK, transparency_factor);
    g->draw_text({xcen, ycen}, str.c_str(), 2 * draw_coords->pin_size,
                 2 * draw_coords->pin_size);
    g->set_color(color, transparency_factor);
}

void draw_get_rr_src_sink_coords(const t_rr_node& node, float* xcen, float* ycen) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    RRNodeId rr_node = node.id();
    t_physical_tile_type_ptr tile_type = device_ctx.grid.get_physical_type({rr_graph.node_xlow(rr_node),
                                                                            rr_graph.node_ylow(rr_node),
                                                                            rr_graph.node_layer(rr_node)});

    //Number of classes (i.e. src/sinks) we need to draw
    float num_class = tile_type->class_inf.size();

    int height = tile_type->height; //Height in blocks

    //How many classes to draw per unit block height
    int class_per_height = num_class;
    if (height > 1) {
        class_per_height = num_class / (height - 1);
    }

    int class_height_offset = rr_graph.node_class_num(rr_node) / class_per_height; //Offset wrt block height
    int class_height_shift = rr_graph.node_class_num(rr_node) % class_per_height;  //Offset within unit block

    float xc = draw_coords->tile_x[rr_graph.node_xlow(rr_node)];
    float yc = draw_coords->tile_y[rr_graph.node_ylow(rr_node) + class_height_offset];

    *xcen = xc + 0.5 * draw_coords->get_tile_width();

    float class_section_height = class_per_height + 1;

    float ypos = (class_height_shift + 1) / class_section_height;
    *ycen = yc + ypos * draw_coords->get_tile_height();
}

void draw_rr_switch(float from_x, float from_y, float to_x, float to_y, bool buffered, bool configurable, ezgl::renderer* g) {
    /* Draws a buffer (triangle) or pass transistor (circle) on the edge        *
     * connecting from to to, depending on the status of buffered.  The drawing *
     * is closest to the from_node, since it reflects the switch type of from.  */

    if (!buffered) {
        if (configurable) { /* Draw a circle for a pass transistor */
            float xcen = from_x + (to_x - from_x) / 10.;
            float ycen = from_y + (to_y - from_y) / 10.;
            const float switch_rad = 0.15;
            g->draw_arc({xcen, ycen}, switch_rad, 0., 360.);
        } else {
            //Pass, nothing to draw
        }
    } else { /* Buffer */
        if (from_x == to_x || from_y == to_y) {
            //Straight connection
            draw_triangle_along_line(g, {from_x, from_y}, {to_x, to_y},
                                     SB_EDGE_STRAIGHT_ARROW_POSITION);
        } else {
            //Turn connection
            draw_triangle_along_line(g, {from_x, from_y}, {to_x, to_y},
                                     SB_EDGE_TURN_ARROW_POSITION);
        }
    }
}

void draw_expand_non_configurable_rr_nodes_recurr(RRNodeId from_node,
                                                  std::set<RRNodeId>& expanded_nodes) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    expanded_nodes.insert(from_node);

    for (t_edge_size iedge = 0; iedge < rr_graph.num_edges(from_node); ++iedge) {
        bool edge_configurable = rr_graph.edge_is_configurable(from_node, iedge);
        RRNodeId to_node = rr_graph.edge_sink_node(from_node, iedge);

        if (!edge_configurable && !expanded_nodes.count(to_node)) {
            draw_expand_non_configurable_rr_nodes_recurr(to_node,
                                                         expanded_nodes);
        }
    }
}

/* This is a helper function for highlight_rr_nodes(). It determines whether
 * a routing resource has been clicked on by computing a bounding box for that
 *  and checking if the mouse click hit inside its bounding box. The function does not check 
 * routing resources that are on currently invisible layers (layer view is toggled off) to ensure that 
 * only resources on visible layers are set to be highlighted. There is no priority based on FPGA layer 
 * for highlighting routing resources (Does not iterate through nodes by order of layer a node is located on).
 *
 *  It returns the hit RR node's ID (or OPEN if no hit)
 */
RRNodeId draw_check_rr_node_hit(float click_x, float click_y) {
    RRNodeId hit_node = RRNodeId::INVALID();
    ezgl::rectangle bound_box;

    t_draw_coords* draw_coords = get_draw_coords_vars();
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    for (const RRNodeId& inode : device_ctx.rr_graph.nodes()) {
        int layer_num = rr_graph.node_layer(inode);
        if (!draw_state->draw_layer_display[layer_num].visible) {
            continue; /* Don't check RR nodes on currently invisible layers*/
        }

        // Skip Source and Sink Nodes
        if (rr_graph.node_type(inode) == e_rr_type::SOURCE
            || rr_graph.node_type(inode) == e_rr_type::SINK) {
            continue;
        }

        // Check for intra cluster nodes
        if (!is_inter_cluster_node(rr_graph, inode)) {

            if (!draw_state->is_flat) {
                continue;
            }

            auto [blk_id, pin_id] = get_rr_node_cluster_blk_id_pb_graph_pin(inode);
            ezgl::point2d p = draw_coords->get_absolute_pin_location(blk_id, pin_id);

            if (click_x >= p.x - draw_coords->pin_size && click_x <= p.x + draw_coords->pin_size && click_y >= p.y - draw_coords->pin_size && click_y <= p.y + draw_coords->pin_size) {
                hit_node = inode;
                return hit_node;
            }

            continue;
        }

        // Check for inter cluster nodes
        switch (rr_graph.node_type(inode)) {
            case e_rr_type::IPIN:
            case e_rr_type::OPIN: {
                int i = rr_graph.node_xlow(inode);
                int j = rr_graph.node_ylow(inode);
                t_physical_tile_type_ptr type = device_ctx.grid.get_physical_type({i, j, layer_num});
                int width_offset = device_ctx.grid.get_width_offset({i, j, layer_num});
                int height_offset = device_ctx.grid.get_height_offset({i, j, layer_num});
                int ipin = rr_graph.node_pin_num(inode);
                float xcen, ycen;
                for (const e_side& iside : TOTAL_2D_SIDES) {
                    // If pin exists on this side of the block, then get pin coordinates
                    if (type->pinloc[width_offset][height_offset][size_t(iside)][ipin]) {
                        draw_get_rr_pin_coords(inode, &xcen, &ycen, iside);

                        // Now check if we clicked on this pin
                        if (click_x >= xcen - draw_coords->pin_size && click_x <= xcen + draw_coords->pin_size && click_y >= ycen - draw_coords->pin_size && click_y <= ycen + draw_coords->pin_size) {
                            hit_node = inode;
                            return hit_node;
                        }
                    }
                }
                break;
            }
            case e_rr_type::SOURCE:
            case e_rr_type::SINK: {
                float xcen, ycen;
                draw_get_rr_src_sink_coords(rr_graph.rr_nodes()[size_t(inode)], &xcen, &ycen);

                // Now check if we clicked on this pin
                if (click_x >= xcen - draw_coords->pin_size && click_x <= xcen + draw_coords->pin_size && click_y >= ycen - draw_coords->pin_size && click_y <= ycen + draw_coords->pin_size) {
                    hit_node = inode;
                    return hit_node;
                }
                break;
            }
            case e_rr_type::CHANX:
            case e_rr_type::CHANY: {
                bound_box = draw_get_rr_chan_bbox(inode);

                // Check if we clicked on this wire, with 30%
                // tolerance outside its boundary
                const float tolerance = 0.3;
                if (click_x >= bound_box.left() - tolerance && click_x <= bound_box.right() + tolerance && click_y >= bound_box.bottom() - tolerance && click_y <= bound_box.top() + tolerance) {
                    hit_node = inode;
                    return hit_node;
                }
                break;
            }
            default:
                break;
        }
    }
    return hit_node;
}

/* This routine is called when the routing resource graph is shown, and someone
 * clicks outside a block. That click might represent a click on a wire -- we call
 * this routine to determine which wire (if any) was clicked on.  If a wire was
 * clicked upon, we highlight it in Magenta, and its fanout in red.
 */
bool highlight_rr_nodes(float x, float y) {
    t_draw_state* draw_state = get_draw_state_vars();

    if (draw_state->draw_rr_toggle == DRAW_NO_RR && !draw_state->show_nets) {
        application.update_message(draw_state->default_message);
        application.refresh_drawing();
        return false; //No rr shown
    }

    // Check which rr_node (if any) was clicked on.
    RRNodeId hit_node = draw_check_rr_node_hit(x, y);

    return highlight_rr_nodes(hit_node);
}

void draw_rr_costs(ezgl::renderer* g, const vtr::vector<RRNodeId, float>& rr_costs, bool lowest_cost_first) {
    t_draw_state* draw_state = get_draw_state_vars();

    /* Draws routing costs */

    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    g->set_line_width(0);

    bool with_edges = (draw_state->show_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_TOTAL_WITH_EDGES
                       || draw_state->show_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_KNOWN_WITH_EDGES
                       || draw_state->show_router_expansion_cost == DRAW_ROUTER_EXPANSION_COST_EXPECTED_WITH_EDGES);

    VTR_ASSERT(rr_costs.size() == rr_graph.num_nodes());

    float min_cost = std::numeric_limits<float>::infinity();
    float max_cost = -min_cost;
    for (RRNodeId inode : rr_graph.nodes()) {
        if (std::isnan(rr_costs[inode])) continue;

        min_cost = std::min(min_cost, rr_costs[inode]);
        max_cost = std::max(max_cost, rr_costs[inode]);
    }
    if (min_cost == std::numeric_limits<float>::infinity()) min_cost = 0;
    if (max_cost == -std::numeric_limits<float>::infinity()) max_cost = 0;
    std::unique_ptr<vtr::ColorMap> cmap = std::make_unique<vtr::PlasmaColorMap>(min_cost, max_cost);

    //Draw the nodes in ascending order of value, this ensures high valued nodes
    //are not overdrawn by lower value ones (e.g-> when zoomed-out far)
    std::vector<RRNodeId> nodes(device_ctx.rr_graph.nodes().begin(), device_ctx.rr_graph.nodes().end());
    auto cmp_ascending_cost = [&](RRNodeId lhs_node, RRNodeId rhs_node) {
        if (lowest_cost_first) {
            return rr_costs[lhs_node] > rr_costs[rhs_node];
        }
        return rr_costs[lhs_node] < rr_costs[rhs_node];
    };
    std::stable_sort(nodes.begin(), nodes.end(), cmp_ascending_cost);

    for (RRNodeId inode : nodes) {
        float cost = rr_costs[inode];
        if (std::isnan(cost)) continue;

        int transparency_factor = get_rr_node_transparency(inode);

        // continue if rr_node layer is not visible
        int layer_num = rr_graph.node_layer(inode);
        if (!draw_state->draw_layer_display[layer_num].visible)
            continue;

        ezgl::color color = to_ezgl_color(cmap->color(cost));
        color.alpha = transparency_factor;

        switch (rr_graph.node_type(inode)) {
            case e_rr_type::CHANX: //fallthrough
            case e_rr_type::CHANY:
                draw_rr_chan(inode, color, g);
                if (with_edges) draw_rr_edges(inode, g);
                break;

            case e_rr_type::IPIN: //fallthrough
                draw_cluster_pin(inode, color, g);
                if (with_edges) draw_rr_edges(inode, g);
                break;
            case e_rr_type::OPIN:
                draw_cluster_pin(inode, color, g);
                if (with_edges) draw_rr_edges(inode, g);
                break;
            case e_rr_type::SOURCE:
            case e_rr_type::SINK:
                color.alpha *= 0.8;
                draw_rr_src_sink(inode, color, g);
                if (with_edges) draw_rr_edges(inode, g);
                break;
            default:
                break;
        }
    }

    draw_state->color_map = std::move(cmap);
}

/* Returns the coordinates at which the center of this pin should be drawn. *
 * inode gives the node number, and iside gives the side of the clb or pad  *
 * the physical pin is on.                                                  */
void draw_get_rr_pin_coords(RRNodeId inode, float* xcen, float* ycen, const e_side& pin_side) {
    auto& device_ctx = g_vpr_ctx.device();
    draw_get_rr_pin_coords(device_ctx.rr_graph.rr_nodes()[size_t(inode)], xcen, ycen, pin_side);
}

void draw_get_rr_pin_coords(const t_rr_node& node, float* xcen, float* ycen, const e_side& pin_side) {
    t_draw_coords* draw_coords = get_draw_coords_vars();

    int i, j, k, ipin, pins_per_sub_tile;
    float offset, xc, yc, step;
    t_physical_tile_type_ptr type;
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto rr_node = node.id();

    i = rr_graph.node_xlow(rr_node);
    j = rr_graph.node_ylow(rr_node);
    int layer_num = rr_graph.node_layer(rr_node);

    xc = draw_coords->tile_x[i];
    yc = draw_coords->tile_y[j];

    ipin = rr_graph.node_pin_num(rr_node);
    type = device_ctx.grid.get_physical_type({i, j, layer_num});
    pins_per_sub_tile = type->num_pins / type->capacity;
    k = ipin / pins_per_sub_tile;

    /* Since pins numbers go across all sub_tiles in a block in order
     * we can treat as a block box for this step */

    /* For each sub_tile we need and extra padding space */
    step = (float)(draw_coords->get_tile_width())
           / (float)(type->num_pins + type->capacity);
    offset = (ipin + k + 1) * step;

    switch (pin_side) {
        case LEFT:
            yc += offset;
            break;

        case RIGHT:
            xc += draw_coords->get_tile_width();
            yc += offset;
            break;

        case BOTTOM:
            xc += offset;
            break;

        case TOP:
            xc += offset;
            yc += draw_coords->get_tile_width();
            break;

        default:
            vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                      "in draw_get_rr_pin_coords: Unexpected side.\n");
            break;
    }

    *xcen = xc;
    *ycen = yc;
}

int get_rr_node_transparency(RRNodeId rr_node) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    int layer_num = rr_graph.node_layer(rr_node);

    return draw_state->draw_layer_display[layer_num].alpha;
}
#endif
