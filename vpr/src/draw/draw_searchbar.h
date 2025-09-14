#pragma once
/**
 * @file draw_searchbar.h
 * 
 * draw_searchbar contains functions that draw/highlight search results,
 * and manages the selection/highlighting of currently selected options.
 */

#ifndef NO_GRAPHICS

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>

#include "clustered_netlist_fwd.h"
#include "ezgl/rectangle.hpp"
#include "physical_types.h"
#include "rr_graph_fwd.h"

/* This function computes and returns the boundary coordinates of a channel
 * wire segment. This can be used for drawing a wire or determining if a
 * wire has been clicked on by the user.*/
ezgl::rectangle draw_get_rr_chan_bbox(RRNodeId inode);

/* Highlights a block and its fanout/fanin. */
void draw_highlight_blocks_color(t_logical_block_type_ptr type, ClusterBlockId blk_id);

/**
 * @brief Highlights the net associated with the specified RRNodeId in Magenta. De-highlights all other nets. Also appends relevant net information to the provided message char pointer.
 *
 * @param message   A message which we want to update by appending additional net information.
 * @param hit_node  The RRNodeId of the routing resource node whose net should be highlighted.
 */
void highlight_nets(char* message, RRNodeId hit_node);

/**
 * @brief Returns the name of the net associated with a ParentNetId.
 */
std::string draw_get_net_name(ParentNetId parent_id);

/**
 * @brief Determines if a given RRNode is part of the specified net and highlights the net if so. Additionally, appends relevant net information to the provided message char pointer.
 * @param message Pointer to a character array where additional net information will be appended.
 * @param parent_net_id The ParentNetId representing the net to check. This may refer to either atom or cluster nets.
 * @param hit_node The RRNodeId of the node that was selected.
 */
void check_node_highlight_net(char* message, ParentNetId parent_net_id, RRNodeId hit_node);

/* If an rr_node has been clicked on, it will be either highlighted in MAGENTA,
 * or de-highlighted in WHITE. If highlighted, and toggle_rr is selected, highlight
 * fan_in into the node in blue and fan_out from the node in red. If de-highlighted,
 * de-highlight its fan_in and fan_out. */
void draw_highlight_fan_in_fan_out(const std::set<RRNodeId>& nodes);

/* Calls draw_expand_non_configurable_rr_nodes_recurr with hit_node as from_node
 * and an empty set of expanded_nodes. */
std::set<RRNodeId> draw_expand_non_configurable_rr_nodes(RRNodeId hit_node);

/* Sets the color of all clbs, nets and rr_nodes to the default.
 * as well as clearing the highlighed sub-block */
void deselect_all();

#endif /* NO_GRAPHICS */
