#pragma once
/**
 * @file draw_debug.h
 * 
 *  This file contains all functions regarding the graphics related to the setting of place and route breakpoints.
 * Manages creation of new Gtk Windows with debug options on use of the "Debug" button.
 */

#ifndef NO_GRAPHICS

#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <QWidget>
#include <QCheckBox>
#include <string>
#include "breakpoint_state_globals.h"

/** debugger functions **/
void draw_debug_window();
void refresh_bpList();
void add_to_bpList(std::string bpDescription);
void set_moves_button_callback(QWidget* /*widget*/, QWidget* grid);
void set_temp_button_callback(QWidget* /*widgets*/, QWidget* grid);
void set_block_button_callback(QWidget* /*widget*/, QWidget* grid);
void set_router_iter_button_callback(QWidget* /*widget*/, QWidget* grid);
void set_net_id_button_callback(QWidget* /*widget*/, QWidget* grid);
void checkbox_callback(QCheckBox* widget);
void delete_bp_callback(QWidget* widget);
void advanced_button_callback();
void set_expression_button_callback(QWidget* /*widget*/, QWidget* grid);
void close_debug_window();
void close_advanced_window();
void ok_close_window(QWidget* /*widget*/, QWidget* window);
void invalid_breakpoint_entry_window(std::string error);
bool valid_expression(std::string exp);
void breakpoint_info_window(std::string bpDescription, BreakpointState draw_breakpoint_state, bool in_placer);

#endif /*NO_GRAPHICS*/
