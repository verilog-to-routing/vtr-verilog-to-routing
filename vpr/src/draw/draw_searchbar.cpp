/*draw_searchbar.cpp contains all functions related to searchbar actions.*/
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
#include "draw_basic.h"
#include "draw_toggle_functions.h"
#include "draw_triangle.h"
#include "draw_searchbar.h"
#include "draw_mux.h"
#include "read_xml_arch_file.h"
#include "draw_global.h"
#include "intra_logic_block.h"
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

/****************************** Define Macros *******************************/

#    define DEFAULT_RR_NODE_COLOR ezgl::BLACK

/* This function computes and returns the boundary coordinates of a channel
 * wire segment. This can be used for drawing a wire or determining if a
 * wire has been clicked on by the user.
 * TODO: Fix this for global routing, currently for detailed only.
 */
ezgl::rectangle draw_get_rr_chan_bbox(int inode) {
    double left = 0, right = 0, top = 0, bottom = 0;
    t_draw_coords* draw_coords = get_draw_coords_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    auto rr_node = RRNodeId(inode);

    switch (rr_graph.node_type(rr_node)) {
        case CHANX:
            left = draw_coords->tile_x[rr_graph.node_xlow(rr_node)];
            right = draw_coords->tile_x[rr_graph.node_xhigh(rr_node)]
                    + draw_coords->get_tile_width();
            bottom = draw_coords->tile_y[rr_graph.node_ylow(rr_node)]
                     + draw_coords->get_tile_width()
                     + (1. + rr_graph.node_track_num(rr_node));
            top = draw_coords->tile_y[rr_graph.node_ylow(rr_node)]
                  + draw_coords->get_tile_width()
                  + (1. + rr_graph.node_track_num(rr_node));
            break;
        case CHANY:
            left = draw_coords->tile_x[rr_graph.node_xlow(rr_node)]
                   + draw_coords->get_tile_width()
                   + (1. + rr_graph.node_track_num(rr_node));
            right = draw_coords->tile_x[rr_graph.node_xlow(rr_node)]
                    + draw_coords->get_tile_width()
                    + (1. + rr_graph.node_track_num(rr_node));
            bottom = draw_coords->tile_y[rr_graph.node_ylow(rr_node)];
            top = draw_coords->tile_y[rr_graph.node_yhigh(rr_node)]
                  + draw_coords->get_tile_width();
            break;
        default:
            // a problem. leave at default value (ie. zeros)
            break;
    }
    ezgl::rectangle bound_box({left, bottom}, {right, top});

    return bound_box;
}

void draw_highlight_blocks_color(t_logical_block_type_ptr type,
                                 ClusterBlockId blk_id) {
    int k, iclass;
    ClusterBlockId fanblk;

    t_draw_state* draw_state = get_draw_state_vars();
    auto& cluster_ctx = g_vpr_ctx.clustering();

    for (k = 0; k < type->pb_type->num_pins; k++) { /* Each pin on a CLB */
        ClusterNetId net_id = cluster_ctx.clb_nlist.block_net(blk_id, k);

        if (net_id == ClusterNetId::INVALID())
            continue;

        auto physical_tile = physical_tile_type(blk_id);
        int physical_pin = get_physical_pin(physical_tile, type, k);

        iclass = physical_tile->pin_class[physical_pin];

        if (physical_tile->class_inf[iclass].type == DRIVER) { /* Fanout */
            if (draw_state->block_color(blk_id) == SELECTED_COLOR) {
                /* If block already highlighted, de-highlight the fanout. (the deselect case)*/
                draw_state->net_color[net_id] = ezgl::BLACK;
                for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                    fanblk = cluster_ctx.clb_nlist.pin_block(pin_id);
                    draw_reset_blk_color(fanblk);
                }
            } else {
                /* Highlight the fanout */
                draw_state->net_color[net_id] = DRIVES_IT_COLOR;
                for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                    fanblk = cluster_ctx.clb_nlist.pin_block(pin_id);
                    draw_state->set_block_color(fanblk, DRIVES_IT_COLOR);
                }
            }
        } else { /* This net is fanin to the block. */
            if (draw_state->block_color(blk_id) == SELECTED_COLOR) {
                /* If block already highlighted, de-highlight the fanin. (the deselect case)*/
                draw_state->net_color[net_id] = ezgl::BLACK;
                fanblk = cluster_ctx.clb_nlist.net_driver_block(net_id); /* DRIVER to net */
                draw_reset_blk_color(fanblk);
            } else {
                /* Highlight the fanin */
                draw_state->net_color[net_id] = DRIVEN_BY_IT_COLOR;
                fanblk = cluster_ctx.clb_nlist.net_driver_block(net_id); /* DRIVER to net */
                draw_state->set_block_color(fanblk, DRIVEN_BY_IT_COLOR);
            }
        }
    }

    if (draw_state->block_color(blk_id) == SELECTED_COLOR) {
        /* If block already highlighted, de-highlight the selected block. */
        draw_reset_blk_color(blk_id);
    } else {
        /* Highlight the selected block. */
        draw_state->set_block_color(blk_id, SELECTED_COLOR);
    }
}

/* If an rr_node has been clicked on, it will be highlighted in MAGENTA.
 * If so, and toggle nets is selected, highlight the whole net in that colour.
 */
void highlight_nets(char* message, int hit_node) {
    t_trace* tptr;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();

    t_draw_state* draw_state = get_draw_state_vars();

    for (auto net_id : cluster_ctx.clb_nlist.nets()) {
        for (tptr = route_ctx.trace[net_id].head; tptr != nullptr;
             tptr = tptr->next) {
            if (draw_state->draw_rr_node[tptr->index].color == ezgl::MAGENTA) {
                draw_state->net_color[net_id] = draw_state->draw_rr_node[tptr->index].color;
                if (tptr->index == hit_node) {
                    std::string orig_msg(message);
                    sprintf(message, "%s  ||  Net: %zu (%s)", orig_msg.c_str(),
                            size_t(net_id),
                            cluster_ctx.clb_nlist.net_name(net_id).c_str());
                }
            } else if (draw_state->draw_rr_node[tptr->index].color
                       == ezgl::WHITE) {
                // If node is de-selected.
                draw_state->net_color[net_id] = ezgl::BLACK;
                break;
            }
        }
    }
    application.update_message(message);
}

/* If an rr_node has been clicked on, it will be either highlighted in MAGENTA,
 * or de-highlighted in WHITE. If highlighted, and toggle_rr is selected, highlight
 * fan_in into the node in blue and fan_out from the node in red. If de-highlighted,
 * de-highlight its fan_in and fan_out.
 */
void draw_highlight_fan_in_fan_out(const std::set<int>& nodes) {
    t_draw_state* draw_state = get_draw_state_vars();
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;
    for (auto node : nodes) {
        /* Highlight the fanout nodes in red. */
        for (t_edge_size iedge = 0, l = rr_graph.num_edges(RRNodeId(node));
             iedge < l; iedge++) {
            int fanout_node = size_t(rr_graph.edge_sink_node(RRNodeId(node), iedge));

            if (draw_state->draw_rr_node[node].color == ezgl::MAGENTA
                && draw_state->draw_rr_node[fanout_node].color
                       != ezgl::MAGENTA) {
                // If node is highlighted, highlight its fanout
                draw_state->draw_rr_node[fanout_node].color = DRIVES_IT_COLOR;
                draw_state->draw_rr_node[fanout_node].node_highlighted = true;
            } else if (draw_state->draw_rr_node[node].color == ezgl::WHITE) {
                // If node is de-highlighted, de-highlight its fanout
                draw_state->draw_rr_node[fanout_node].color = DEFAULT_RR_NODE_COLOR;
                draw_state->draw_rr_node[fanout_node].node_highlighted = false;
            }
        }

        /* Highlight the nodes that can fanin to this node in blue. */
        for (const RRNodeId& inode : rr_graph.nodes()) {
            for (t_edge_size iedge = 0, l = rr_graph.num_edges(inode); iedge < l;
                 iedge++) {
                int fanout_node = size_t(rr_graph.edge_sink_node(inode, iedge));
                if (fanout_node == node) {
                    if (draw_state->draw_rr_node[node].color == ezgl::MAGENTA
                        && draw_state->draw_rr_node[size_t(inode)].color
                               != ezgl::MAGENTA) {
                        // If node is highlighted, highlight its fanin
                        draw_state->draw_rr_node[size_t(inode)].color = ezgl::BLUE;
                        draw_state->draw_rr_node[size_t(inode)].node_highlighted = true;
                    } else if (draw_state->draw_rr_node[node].color
                               == ezgl::WHITE) {
                        // If node is de-highlighted, de-highlight its fanin
                        draw_state->draw_rr_node[size_t(inode)].color = DEFAULT_RR_NODE_COLOR;
                        draw_state->draw_rr_node[size_t(inode)].node_highlighted = false;
                    }
                }
            }
        }
    }
}

std::set<int> draw_expand_non_configurable_rr_nodes(int from_node) {
    std::set<int> expanded_nodes;
    draw_expand_non_configurable_rr_nodes_recurr(from_node, expanded_nodes);
    return expanded_nodes;
}

void deselect_all() {
    // Sets the color of all clbs, nets and rr_nodes to the default.
    // as well as clearing the highlighed sub-block

    t_draw_state* draw_state = get_draw_state_vars();
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& device_ctx = g_vpr_ctx.device();

    /* Create some colour highlighting */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (blk_id != ClusterBlockId::INVALID())
            draw_reset_blk_color(blk_id);
    }

    for (auto net_id : cluster_ctx.clb_nlist.nets())
        draw_state->net_color[net_id] = ezgl::BLACK;

    for (const RRNodeId& rr_id : device_ctx.rr_graph.nodes()) {
        size_t i = (size_t)rr_id;
        draw_state->draw_rr_node[i].color = DEFAULT_RR_NODE_COLOR;
        draw_state->draw_rr_node[i].node_highlighted = false;
    }
    get_selected_sub_block_info().clear();
}

#endif
