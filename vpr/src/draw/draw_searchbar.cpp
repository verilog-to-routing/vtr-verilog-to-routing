/*draw_searchbar.cpp contains all functions related to searchbar actions.*/
#ifndef NO_GRAPHICS

#include <cstdio>

#include "netlist_fwd.h"

#include "physical_types_util.h"
#include "vpr_utils.h"

#include "globals.h"
#include "draw.h"
#include "draw_rr.h"
#include "draw_basic.h"
#include "draw_searchbar.h"
#include "draw_global.h"
#include "intra_logic_block.h"

//To process key presses we need the X11 keysym definitions,
//which are unavailable when building with MINGW
#if defined(X11) && !defined(__MINGW32__)
#include <X11/keysym.h>
#endif

/****************************** Define Macros *******************************/

#define DEFAULT_RR_NODE_COLOR ezgl::BLACK

/* This function computes and returns the boundary coordinates of a channel
 * wire segment. This can be used for drawing a wire or determining if a
 * wire has been clicked on by the user.
 * TODO: Fix this for global routing, currently for detailed only.
 */
ezgl::rectangle draw_get_rr_chan_bbox(RRNodeId inode) {
    double left = 0, right = 0, top = 0, bottom = 0;
    t_draw_coords* draw_coords = get_draw_coords_vars();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    switch (rr_graph.node_type(inode)) {
        case e_rr_type::CHANX:
            left = draw_coords->tile_x[rr_graph.node_xlow(inode)];
            right = draw_coords->tile_x[rr_graph.node_xhigh(inode)]
                    + draw_coords->get_tile_width();
            bottom = draw_coords->tile_y[rr_graph.node_ylow(inode)]
                     + draw_coords->get_tile_width()
                     + (1. + rr_graph.node_track_num(inode));
            top = draw_coords->tile_y[rr_graph.node_ylow(inode)]
                  + draw_coords->get_tile_width()
                  + (1. + rr_graph.node_track_num(inode));
            break;
        case e_rr_type::CHANY:
            left = draw_coords->tile_x[rr_graph.node_xlow(inode)]
                   + draw_coords->get_tile_width()
                   + (1. + rr_graph.node_track_num(inode));
            right = draw_coords->tile_x[rr_graph.node_xlow(inode)]
                    + draw_coords->get_tile_width()
                    + (1. + rr_graph.node_track_num(inode));
            bottom = draw_coords->tile_y[rr_graph.node_ylow(inode)];
            top = draw_coords->tile_y[rr_graph.node_yhigh(inode)]
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
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& block_locs = draw_state->get_graphics_blk_loc_registry_ref().block_locs();

    for (int k = 0; k < type->pb_type->num_pins; k++) { /* Each pin on a CLB */
        ClusterNetId net_id = cluster_ctx.clb_nlist.block_net(blk_id, k);

        if (net_id == ClusterNetId::INVALID()) {
            continue;
        }

        t_pl_loc block_loc = block_locs[blk_id].loc;
        auto physical_tile = physical_tile_type(block_loc);
        int physical_pin = get_physical_pin(physical_tile, type, k);

        auto class_type = get_pin_type_from_pin_physical_num(physical_tile, physical_pin);

        if (class_type == DRIVER) { /* Fanout */
            if (draw_state->block_color(blk_id) == SELECTED_COLOR) {
                /* If block already highlighted, de-highlight the fanout. (the deselect case)*/
                draw_state->net_color[net_id] = ezgl::BLACK;
                for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                    ClusterBlockId fanblk = cluster_ctx.clb_nlist.pin_block(pin_id);
                    draw_reset_blk_color(fanblk);
                }
            } else {
                /* Highlight the fanout */
                draw_state->net_color[net_id] = DRIVES_IT_COLOR;
                for (auto pin_id : cluster_ctx.clb_nlist.net_sinks(net_id)) {
                    ClusterBlockId fanblk = cluster_ctx.clb_nlist.pin_block(pin_id);
                    draw_state->set_block_color(fanblk, DRIVES_IT_COLOR);
                }
            }
        } else { /* This net is fanin to the block. */
            if (draw_state->block_color(blk_id) == SELECTED_COLOR) {
                /* If block already highlighted, de-highlight the fanin. (the deselect case)*/
                draw_state->net_color[net_id] = ezgl::BLACK;
                ClusterBlockId fanblk = cluster_ctx.clb_nlist.net_driver_block(net_id); /* DRIVER to net */
                draw_reset_blk_color(fanblk);
            } else {
                /* Highlight the fanin */
                draw_state->net_color[net_id] = DRIVEN_BY_IT_COLOR;
                ClusterBlockId fanblk = cluster_ctx.clb_nlist.net_driver_block(net_id); /* DRIVER to net */
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
void highlight_nets(char* message, RRNodeId hit_node) {
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& atom_ctx = g_vpr_ctx.atom();
    auto& route_ctx = g_vpr_ctx.routing();

    // Don't crash if there's no routing
    if (route_ctx.route_trees.empty())
        return;

    if (route_ctx.is_flat) {
        for (AtomNetId net_id : atom_ctx.netlist().nets()) {
            check_node_highlight_net(message, net_id, hit_node);
        }

    } else {
        for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
            check_node_highlight_net(message, net_id, hit_node);
        }
    }

    application.update_message(message);
}

std::string draw_get_net_name(ParentNetId parent_id) {
    if (!g_vpr_ctx.routing().is_flat) {
        return g_vpr_ctx.clustering().clb_nlist.net_name(convert_to_cluster_net_id(parent_id));
    } else {
        return g_vpr_ctx.atom().netlist().net_name(convert_to_atom_net_id(parent_id));
    }
}

void check_node_highlight_net(char* message, ParentNetId parent_net_id, RRNodeId hit_node) {
    auto& route_ctx = g_vpr_ctx.routing();
    t_draw_state* draw_state = get_draw_state_vars();

    if (!route_ctx.route_trees[parent_net_id])
        return;

    for (const RouteTreeNode& rt_node : route_ctx.route_trees[parent_net_id].value().all_nodes()) {
        RRNodeId inode = rt_node.inode;
        if (draw_state->draw_rr_node[inode].color == ezgl::MAGENTA) {
            draw_state->net_color[parent_net_id] = draw_state->draw_rr_node[inode].color;
            if (inode == hit_node) {
                std::string orig_msg(message);
                sprintf(message, "%s  ||  Net: %zu (%s)", orig_msg.c_str(),
                        size_t(parent_net_id),
                        draw_get_net_name(parent_net_id).c_str());
            }
        } else if (draw_state->draw_rr_node[inode].color == ezgl::WHITE) {
            // If node is de-selected.
            draw_state->net_color[parent_net_id] = ezgl::BLACK;
            break;
        }
    }
}

/* If an rr_node has been clicked on, it will be either highlighted in MAGENTA,
 * or de-highlighted in WHITE. If highlighted, and toggle_rr is selected, highlight
 * fan_in into the node in blue and fan_out from the node in red. If de-highlighted,
 * de-highlight its fan_in and fan_out.
 */
void draw_highlight_fan_in_fan_out(const std::set<RRNodeId>& nodes) {
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    for (auto node : nodes) {
        /* Highlight the fanout nodes in red. */
        for (t_edge_size iedge = 0, l = rr_graph.num_edges(node);
             iedge < l; iedge++) {
            RRNodeId fanout_node = rr_graph.edge_sink_node(node, iedge);

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
        for (RRNodeId inode : rr_graph.nodes()) {
            for (t_edge_size iedge = 0, l = rr_graph.num_edges(inode); iedge < l; iedge++) {
                RRNodeId fanout_node = rr_graph.edge_sink_node(inode, iedge);
                if (fanout_node == node) {
                    if (draw_state->draw_rr_node[node].color == ezgl::MAGENTA
                        && draw_state->draw_rr_node[inode].color
                               != ezgl::MAGENTA) {
                        // If node is highlighted, highlight its fanin
                        draw_state->draw_rr_node[inode].color = ezgl::BLUE;
                        draw_state->draw_rr_node[inode].node_highlighted = true;
                    } else if (draw_state->draw_rr_node[node].color
                               == ezgl::WHITE) {
                        // If node is de-highlighted, de-highlight its fanin
                        draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
                        draw_state->draw_rr_node[inode].node_highlighted = false;
                    }
                }
            }
        }
    }
}

std::set<RRNodeId> draw_expand_non_configurable_rr_nodes(RRNodeId from_node) {
    std::set<RRNodeId> expanded_nodes;
    draw_expand_non_configurable_rr_nodes_recurr(from_node, expanded_nodes);
    return expanded_nodes;
}

void deselect_all() {
    // Sets the color of all clbs, nets and rr_nodes to the default.
    // as well as clearing the highlighted sub-block

    t_draw_state* draw_state = get_draw_state_vars();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const auto& device_ctx = g_vpr_ctx.device();

    /* Create some colour highlighting */
    for (auto blk_id : cluster_ctx.clb_nlist.blocks()) {
        if (blk_id != ClusterBlockId::INVALID())
            draw_reset_blk_color(blk_id);
    }

    if (draw_state->is_flat) {
        for (auto net_id : atom_ctx.netlist().nets())
            draw_state->net_color[net_id] = ezgl::BLACK;
    } else {
        for (auto net_id : cluster_ctx.clb_nlist.nets())
            draw_state->net_color[net_id] = ezgl::BLACK;
    }

    for (RRNodeId inode : device_ctx.rr_graph.nodes()) {
        draw_state->draw_rr_node[inode].color = DEFAULT_RR_NODE_COLOR;
        draw_state->draw_rr_node[inode].node_highlighted = false;
    }
    get_selected_sub_block_info().clear();
}

#endif
