#ifndef DRAW_BASIC_H
#define DRAW_BASIC_H

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

void drawplace(ezgl::renderer* g);
void drawnets(ezgl::renderer* g);
void draw_congestion(ezgl::renderer* g);
void draw_routing_costs(ezgl::renderer* g);
void draw_routing_bb(ezgl::renderer* g);
void draw_routing_util(ezgl::renderer* g);
void draw_crit_path(ezgl::renderer* g);
void draw_placement_macros(ezgl::renderer* g);

void draw_x(float x, float y, float size, ezgl::renderer* g);
void drawroute(enum e_draw_net_type draw_net_type, ezgl::renderer* g);

void draw_get_rr_pin_coords(int inode, float* xcen, float* ycen, const e_side& pin_side);
void draw_get_rr_pin_coords(const t_rr_node& node, float* xcen, float* ycen, const e_side& pin_side);

void draw_routed_net(ClusterNetId net, ezgl::renderer* g);
void draw_partial_route(const std::vector<int>& rr_nodes_to_draw,
                        ezgl::renderer* g);

void draw_flyline_timing_edge(ezgl::point2d start, ezgl::point2d end, float incr_delay, ezgl::renderer* g);

void draw_routed_timing_edge(tatum::NodeId start_tnode,
                             tatum::NodeId end_tnode,
                             float incr_delay,
                             ezgl::color color,
                             ezgl::renderer* g);
void draw_routed_timing_edge_connection(tatum::NodeId src_tnode,
                                        tatum::NodeId sink_tnode,
                                        ezgl::color color,
                                        ezgl::renderer* g);

#endif /* NO_GRAPHICS */
#endif /* DRAW_BASIC_H */
