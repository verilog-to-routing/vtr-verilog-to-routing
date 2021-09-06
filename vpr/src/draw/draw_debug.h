/** This file contains all functions reagrding the graphics related to the setting of place and route breakpoints **/
#ifndef DRAW_DEBUG_H
#define DRAW_DEBUG_H

#ifndef NO_GRAPHICS

#    include "breakpoint.h"
#    include "draw_global.h"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

#    include <cstdio>
#    include <cfloat>
#    include <cstring>
#    include <cmath>
#    include <algorithm>
#    include <sstream>
#    include <array>
#    include <iostream>

/** debugger functions **/
void draw_debug_window();
void refresh_bpList();
void add_to_bpList(std::string bpDescription);
void set_moves_button_callback(GtkWidget* /*widget*/, GtkWidget* grid);
void set_temp_button_callback(GtkWidget* /*widgets*/, GtkWidget* grid);
void set_block_button_callback(GtkWidget* /*widget*/, GtkWidget* grid);
void set_router_iter_button_callback(GtkWidget* /*widget*/, GtkWidget* grid);
void set_net_id_button_callback(GtkWidget* /*widget*/, GtkWidget* grid);
void checkbox_callback(GtkWidget* widget);
void delete_bp_callback(GtkWidget* widget);
void advanced_button_callback();
void set_expression_button_callback(GtkWidget* /*widget*/, GtkWidget* grid);
void close_debug_window();
void close_advanced_window();
void ok_close_window(GtkWidget* /*widget*/, GtkWidget* window);
void invalid_breakpoint_entry_window(std::string error);
bool valid_expression(std::string exp);
void breakpoint_info_window(std::string bpDescription, BreakpointState draw_breakpoint_state, bool in_placer);

#endif /*NO_GRAPHICS*/

#endif /*DRAW_DEBUG_H*/
