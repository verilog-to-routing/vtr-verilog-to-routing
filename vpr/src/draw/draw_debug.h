/*This file contains all function declarations reagrding the graphics of setting and handling breakpoints*/

#include "breakpoint.h"
#include "draw_global.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"

#include <cstdio>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <array>
#include <iostream>

//debugger functions
void ok_close_window(GtkWidget* /*widget*/, GtkWidget* window);
void invalid_breakpoint_entry_window(std::string error);


