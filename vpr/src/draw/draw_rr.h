#ifndef DRAW_RR_H
#define DRAW_RR_H

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

void draw_rr(ezgl::renderer* g);
void draw_rr_edges(int from_node, ezgl::renderer* g);
void draw_rr_chan(int inode, const ezgl::color color, ezgl::renderer* g);
void draw_rr_pin(int inode, const ezgl::color& color, ezgl::renderer* g);
void draw_rr_src_sink(int inode, ezgl::color color, ezgl::renderer* g);
void draw_get_rr_src_sink_coords(const t_rr_node& node, float* xcen, float* ycen);


#endif /* NO_GRAPHICS */
#endif /* DRAW_RR_H */
