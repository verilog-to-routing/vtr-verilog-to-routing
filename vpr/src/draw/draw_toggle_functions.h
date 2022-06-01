#ifndef DRAW_TOGGLE_FUNCTIONS_H
#define DRAW_TOGGLE_FUNCTIONS_H

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

void toggle_nets(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_rr(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_congestion(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_congestion_cost(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_bounding_box(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_routing_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_crit_path(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_block_pin_util(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_router_expansion_costs(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void toggle_placement_macros(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void net_max_fanout(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
void set_net_alpha_value(GtkWidget* /*widget*/, gint /*response_id*/, gpointer /*data*/);
float get_net_alpha();

ezgl::color get_block_type_color(t_physical_tile_type_ptr type);
ezgl::color lighten_color(ezgl::color color, float amount);

#endif /* NO_GRAPHICS */
#endif /* DRAW_TOGGLE_FUNCTIONS_H */
