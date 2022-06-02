#ifndef DRAW_MUX_H
#define DRAW_MUX_H

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

void draw_mux_with_size(ezgl::point2d origin, e_side orientation, float height, int size, ezgl::renderer* g);
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, ezgl::renderer* g);
ezgl::rectangle draw_mux(ezgl::point2d origin, e_side orientation, float height, float width, float height_scale, ezgl::renderer* g);

#endif /* NO_GRAPHICS */
#endif /* DRAW_MUX_H */
