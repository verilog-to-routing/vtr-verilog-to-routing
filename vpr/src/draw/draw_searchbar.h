#ifndef DRAW_SEARCHBAR_H
#define DRAW_SEARCHBAR_H

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

#include "move_utils.h"

#ifndef NO_GRAPHICS

#    include "draw_global.h"

#    include "ezgl/point.hpp"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "draw_color.h"
#    include "search_bar.h"
#    include "draw_debug.h"
#    include "manual_moves.h"

#    include "rr_graph.h"
#    include "route_util.h"
#    include "place_macro.h"
#    include "buttons.h"

/* This function computes and returns the boundary coordinates of a channel
 * wire segment. This can be used for drawing a wire or determining if a
 * wire has been clicked on by the user.*/
ezgl::rectangle draw_get_rr_chan_bbox(int inode);

/* Highlights a block and its fanout/fanin. */
void draw_highlight_blocks_color(t_logical_block_type_ptr type, ClusterBlockId blk_id);

/* If an rr_node has been clicked on, it will be highlighted in MAGENTA.
 * If so, and toggle nets is selected, highlight the whole net in that colour.*/
void highlight_nets(char* message, int hit_node);

/* If an rr_node has been clicked on, it will be either highlighted in MAGENTA,
 * or de-highlighted in WHITE. If highlighted, and toggle_rr is selected, highlight
 * fan_in into the node in blue and fan_out from the node in red. If de-highlighted,
 * de-highlight its fan_in and fan_out. */
void draw_highlight_fan_in_fan_out(const std::set<int>& nodes);

/* Calls draw_expand_non_configurable_rr_nodes_recurr with hit_node as from_node
 * and an empty set of expanded_nodes. */
std::set<int> draw_expand_non_configurable_rr_nodes(int hit_node);

/* Sets the color of all clbs, nets and rr_nodes to the default.
 * as well as clearing the highlighed sub-block */
void deselect_all();

#endif /* NO_GRAPHICS */
#endif /* DRAW_SEARCHBAR_H */
