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

ezgl::rectangle draw_get_rr_chan_bbox(int inode);
void draw_highlight_blocks_color(t_logical_block_type_ptr type, ClusterBlockId blk_id);
void highlight_nets(char* message, int hit_node);
void draw_highlight_fan_in_fan_out(const std::set<int>& nodes);
std::set<int> draw_expand_non_configurable_rr_nodes(int hit_node);
void deselect_all();

#endif /* NO_GRAPHICS */
#endif /* DRAW_SEARCHBAR_H */
