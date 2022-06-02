#ifndef DRAW_X_TO_Y_H
#define DRAW_X_TO_Y_H

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

void draw_chany_to_chany_edge(RRNodeId from_node, RRNodeId to_node, int to_track, short switch_type, ezgl::renderer* g);
void draw_chanx_to_chanx_edge(RRNodeId from_node, RRNodeId to_node, int to_track, short switch_type, ezgl::renderer* g);
void draw_chanx_to_chany_edge(int chanx_node, int chanx_track, int chany_node, int chany_track, enum e_edge_dir edge_dir, short switch_type, ezgl::renderer* g);
void draw_pin_to_pin(int opin, int ipin, ezgl::renderer* g);
void draw_pin_to_sink(int ipin_node, int sink_node, ezgl::renderer* g);
void draw_source_to_pin(int source_node, int opin_node, ezgl::renderer* g);
void draw_pin_to_chan_edge(int pin_node, int chan_node, ezgl::renderer* g);

#endif /* NO_GRAPHICS */
#endif /* DRAW_X_TO_Y_H */
