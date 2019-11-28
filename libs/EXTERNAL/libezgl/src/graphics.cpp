/*
 * Copyright 2019 University of Toronto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Mario Badr, Sameh Attia and Tanner Young-Schultz
 */

#include "ezgl/graphics.hpp"

#include <cassert>
#include <glib.h>

namespace ezgl {

renderer::renderer(cairo_t *cairo,
    transform_fn transform,
    camera *p_camera,
    cairo_surface_t *m_surface)
    : m_cairo(cairo), m_transform(std::move(transform)), m_camera(p_camera), rotation_angle(0)
{
#ifdef EZGL_USE_X11
  // Check if the created cairo surface is an XLIB surface
  if (cairo_surface_get_type(m_surface) == CAIRO_SURFACE_TYPE_XLIB) {
    // get the underlying x11 drawable used by cairo surface
    x11_drawable = cairo_xlib_surface_get_drawable(m_surface);

    // get the x11 display
    x11_display = cairo_xlib_surface_get_display(m_surface);

    // create the x11 context from the drawable of the cairo surface
    if (x11_display != nullptr) {
      x11_context = XCreateGC(x11_display, x11_drawable, 0, 0);
    }
  }
#endif
}

renderer::~renderer()
{
#ifdef EZGL_USE_X11
  // free the x11 context
  if (x11_display != nullptr) {
    XFreeGC(x11_display, x11_context);
  }
#endif
}

void renderer::update_renderer(cairo_t *cairo, cairo_surface_t *m_surface)
{
  // Update Cairo Context
  m_cairo = cairo;

  // Update X11 Context
#ifdef EZGL_USE_X11
  // Check if the created cairo surface is an XLIB surface
  if (cairo_surface_get_type(m_surface) == CAIRO_SURFACE_TYPE_XLIB) {
    // get the underlying x11 drawable used by cairo surface
    x11_drawable = cairo_xlib_surface_get_drawable(m_surface);

    // get the x11 display
    x11_display = cairo_xlib_surface_get_display(m_surface);

    // create the x11 context from the drawable of the cairo surface
    if (x11_display != nullptr) {
      XFreeGC(x11_display, x11_context);
      x11_context = XCreateGC(x11_display, x11_drawable, 0, 0);
    }
  }
#endif

  // Restore graphics attributes
  set_color(current_color);
  set_line_width(current_line_width);
  set_line_cap(current_line_cap);
  set_line_dash(current_line_dash);
}

void renderer::set_coordinate_system(t_coordinate_system new_coordinate_system)
{
  current_coordinate_system = new_coordinate_system;
}

void renderer::set_visible_world(rectangle new_world)
{
  // Change the aspect ratio of the new_world to align with the aspect ratio of the initial world
  // Get the width and height of the new_world
  point2d n_center = new_world.center();
  double n_width = new_world.width();
  double n_height = new_world.height();

  // Get the aspect ratio of the initial world
  double i_width = m_camera->get_initial_world().width();
  double i_height = m_camera->get_initial_world().height();
  double i_aspect_ratio = i_width / i_height;

  // Make sure the required area is entirely visible
  if (n_width/i_aspect_ratio >= n_height) {
    // Change the height
    double new_height = n_width/i_aspect_ratio;
    new_world ={{n_center.x-n_width/2, n_center.y-new_height/2}, n_width, new_height};
  }
  else {
    // Change the width
    double new_width = n_height/i_aspect_ratio;
    new_world ={{n_center.x-new_width/2, n_center.y-n_height/2}, new_width, n_height};
  }

  // set the visible bounds of the world
  m_camera->set_world(new_world);
}

rectangle renderer::get_visible_world()
{
  // m_camera->get_world() is not good representative of the visible world since it doesn't
  // account for the drawable margins.
  // TODO: precalculate the visible world in camera class to speedup the clipping

  // Get the world and screen dimensions
  rectangle world = m_camera->get_world();
  rectangle screen = m_camera->get_screen();

  // Calculate the margins by converting the screen origin to world coordinates
  point2d margin = screen.bottom_left() * m_camera->get_world_scale_factor();

  // The actual visible world
  return {(world.bottom_left() - margin), (world.top_right() + margin)};
}

rectangle renderer::get_visible_screen()
{
  // Get the widget dimensions
  return m_camera->get_widget();
}

rectangle renderer::world_to_screen(const rectangle& box)
{
  point2d origin = m_transform(box.bottom_left());
  point2d top_right = m_transform(box.top_right());

  return rectangle(origin, top_right);
}

bool renderer::rectangle_off_screen(rectangle rect)
{
  if(current_coordinate_system == SCREEN)
    return false;

  rectangle visible = get_visible_world();

  if(rect.right() < visible.left())
    return true;

  if(rect.left() > visible.right())
    return true;

  if(rect.top() < visible.bottom())
    return true;

  if(rect.bottom() > visible.top())
    return true;

  return false;
}

void renderer::set_color(color c)
{
  set_color(c.red, c.green, c.blue, c.alpha);
}

void renderer::set_color(color c, uint_fast8_t alpha)
{
  set_color(c.red, c.green, c.blue, alpha);
}

void renderer::set_color(uint_fast8_t red,
    uint_fast8_t green,
    uint_fast8_t blue,
    uint_fast8_t alpha)
{
  // set color for cairo
  cairo_set_source_rgba(m_cairo, red / 255.0, green / 255.0, blue / 255.0, alpha / 255.0);

  // set current_color
  current_color = {red, green, blue, alpha};

#ifdef EZGL_USE_X11
  // check transparency
  if(alpha != 255)
    transparency_flag = true;
  else
    transparency_flag = false;

  // set color for x11 (no transparency)
  if (x11_display != nullptr) {
    unsigned long xcolor = 0;
    xcolor |= (red << 2 * 8 | red << 8 | red) & 0xFF0000;
    xcolor |= (green << 2 * 8 | green << 8 | green) & 0xFF00;
    xcolor |= (blue << 2 * 8 | blue << 8 | blue) & 0xFF;
    xcolor |= 0xFF000000;
    XSetForeground(x11_display, x11_context, xcolor);
  }
#endif
}

void renderer::set_line_cap(line_cap cap)
{
  auto cairo_cap = static_cast<cairo_line_cap_t>(cap);
  cairo_set_line_cap(m_cairo, cairo_cap);

  current_line_cap = cap;

#ifdef EZGL_USE_X11
  if (x11_display != nullptr) {
    XSetLineAttributes(x11_display, x11_context, current_line_width,
        current_line_dash == line_dash::none ? LineSolid : LineOnOffDash,
        current_line_cap == line_cap::butt ? CapButt : CapRound, JoinMiter);
  }
#endif
}

void renderer::set_line_dash(line_dash dash)
{
  if(dash == line_dash::none) {
    int num_dashes = 0; // disables dashing

    cairo_set_dash(m_cairo, nullptr, num_dashes, 0);
  } else if(dash == line_dash::asymmetric_5_3) {
    static double dashes[] = {5.0, 3.0};
    int num_dashes = 2; // asymmetric dashing

    cairo_set_dash(m_cairo, dashes, num_dashes, 0);
  }

  current_line_dash = dash;

#ifdef EZGL_USE_X11
  if (x11_display != nullptr) {
    XSetLineAttributes(x11_display, x11_context, current_line_width,
        current_line_dash == line_dash::none ? LineSolid : LineOnOffDash,
        current_line_cap == line_cap::butt ? CapButt : CapRound, JoinMiter);
  }
#endif
}

void renderer::set_line_width(int width)
{
  cairo_set_line_width(m_cairo, width == 0 ? 1 : width);

  current_line_width = width;

#ifdef EZGL_USE_X11
  if (x11_display != nullptr) {
    XSetLineAttributes(x11_display, x11_context, current_line_width,
        current_line_dash == line_dash::none ? LineSolid : LineOnOffDash,
        current_line_cap == line_cap::butt ? CapButt : CapRound, JoinMiter);
  }
#endif
}

void renderer::set_font_size(double new_size)
{
  cairo_set_font_size(m_cairo, new_size);
}

void renderer::format_font(std::string const &family, font_slant slant, font_weight weight)
{
  cairo_select_font_face(m_cairo, family.c_str(), static_cast<cairo_font_slant_t>(slant),
      static_cast<cairo_font_weight_t>(weight));
}

void renderer::format_font(std::string const &family,
    font_slant slant,
    font_weight weight,
    double new_size)
{
  set_font_size(new_size);
  format_font(family, slant, weight);
}

void renderer::set_text_rotation(double degrees)
{
  // convert the given angle to rad
  rotation_angle = -degrees * M_PI / 180;
}

void renderer::set_horiz_text_just(text_just horiz_just)
{
  if (horiz_just != text_just::top && horiz_just != text_just::bottom)
    horiz_text_just = horiz_just;
}

void renderer::set_vert_text_just(text_just vert_just)
{
  if (vert_just != text_just::right && vert_just != text_just::left)
    vert_text_just = vert_just;
}

void renderer::draw_line(point2d start, point2d end)
{
  if(rectangle_off_screen({start, end}))
    return;

  if(current_coordinate_system == WORLD) {
    start = m_transform(start);
    end = m_transform(end);
  }

#ifdef EZGL_USE_X11
  if(!transparency_flag && x11_display != nullptr) {
    XDrawLine(x11_display, x11_drawable, x11_context, start.x, start.y, end.x, end.y);
    return;
  }
#endif

  cairo_move_to(m_cairo, start.x, start.y);
  cairo_line_to(m_cairo, end.x, end.y);

  cairo_stroke(m_cairo);
}

void renderer::draw_rectangle(point2d start, point2d end)
{
  if(rectangle_off_screen({start, end}))
    return;

  draw_rectangle_path(start, end, false);
}

void renderer::draw_rectangle(point2d start, double width, double height)
{
  if(rectangle_off_screen({start, {start.x + width, start.y + height}}))
    return;

  draw_rectangle_path(start, {start.x + width, start.y + height}, false);
}

void renderer::draw_rectangle(rectangle r)
{
  if(rectangle_off_screen({{r.left(), r.bottom()}, {r.right(), r.top()}}))
    return;

  draw_rectangle_path({r.left(), r.bottom()}, {r.right(), r.top()}, false);
}

void renderer::fill_rectangle(point2d start, point2d end)
{
  if(rectangle_off_screen({start, end}))
    return;

  draw_rectangle_path(start, end, true);
}

void renderer::fill_rectangle(point2d start, double width, double height)
{
  if(rectangle_off_screen({start, {start.x + width, start.y + height}}))
    return;

  draw_rectangle_path(start, {start.x + width, start.y + height}, true);
}

void renderer::fill_rectangle(rectangle r)
{
  if(rectangle_off_screen({{r.left(), r.bottom()}, {r.right(), r.top()}}))
    return;

  draw_rectangle_path({r.left(), r.bottom()}, {r.right(), r.top()}, true);
}

// For speed, use a fixed size polygon point buffer when possible
// Dynamically allocate an arbitrary size buffer only when necessary.
#define X11_MAX_FIXED_POLY_PTS 100

void renderer::fill_poly(std::vector<point2d> const &points)
{
  assert(points.size() > 1);

  // Conservative but fast clip test -- check containing rectangle of polygon
  double x_min = points[0].x;
  double x_max = points[0].x;
  double y_min = points[0].y;
  double y_max = points[0].y;

  for(std::size_t i = 1; i < points.size(); ++i) {
    x_min = std::min(x_min, points[i].x);
    x_max = std::max(x_max, points[i].x);
    y_min = std::min(y_min, points[i].y);
    y_max = std::max(y_max, points[i].y);
  }

  if(rectangle_off_screen({{x_min, y_min}, {x_max, y_max}}))
    return;

  point2d next_point = points[0];

#ifdef EZGL_USE_X11
  if(!transparency_flag && x11_display != nullptr) {
    XPoint fixed_trans_points[X11_MAX_FIXED_POLY_PTS];
    XPoint *trans_points = fixed_trans_points;

    if(points.size() > X11_MAX_FIXED_POLY_PTS) {
      trans_points = new XPoint[points.size()];
    }

    for(size_t i = 0; i < points.size(); i++) {
      if(current_coordinate_system == WORLD)
        next_point = m_transform(points[i]);
      else
        next_point = points[i];
      trans_points[i].x = static_cast<long>(next_point.x);
      trans_points[i].y = static_cast<long>(next_point.y);
    }

    XFillPolygon(x11_display, x11_drawable, x11_context, trans_points, points.size(), Complex,
        CoordModeOrigin);

    if(points.size() > X11_MAX_FIXED_POLY_PTS)
      delete[] trans_points;
    return;
  }
#endif

  if(current_coordinate_system == WORLD)
    next_point = m_transform(points[0]);

  cairo_move_to(m_cairo, next_point.x, next_point.y);

  for(std::size_t i = 1; i < points.size(); ++i) {
    if(current_coordinate_system == WORLD)
      next_point = m_transform(points[i]);
    else
      next_point = points[i];
    cairo_line_to(m_cairo, next_point.x, next_point.y);
  }

  cairo_close_path(m_cairo);
  cairo_fill(m_cairo);
}

void renderer::draw_elliptic_arc(point2d center,
    double radius_x,
    double radius_y,
    double start_angle,
    double extent_angle)
{
  if(rectangle_off_screen(
         {{center.x - radius_x, center.y - radius_y}, {center.x + radius_x, center.y + radius_y}}))
    return;

  // define the stretch factor (i.e. An ellipse is a stretched circle)
  double stretch_factor = radius_y / radius_x;

  draw_arc_path(center, radius_x, start_angle, extent_angle, stretch_factor, false);
}

void renderer::draw_arc(point2d center, double radius, double start_angle, double extent_angle)
{
  if(rectangle_off_screen(
         {{center.x - radius, center.y - radius}, {center.x + radius, center.y + radius}}))
    return;

  draw_arc_path(center, radius, start_angle, extent_angle, 1, false);
}

void renderer::fill_elliptic_arc(point2d center,
    double radius_x,
    double radius_y,
    double start_angle,
    double extent_angle)
{
  if(rectangle_off_screen(
         {{center.x - radius_x, center.y - radius_y}, {center.x + radius_x, center.y + radius_y}}))
    return;

  // define the stretch factor (i.e. An ellipse is a stretched circle)
  double stretch_factor = radius_y / radius_x;

  draw_arc_path(center, radius_x, start_angle, extent_angle, stretch_factor, true);
}

void renderer::fill_arc(point2d center, double radius, double start_angle, double extent_angle)
{
  if(rectangle_off_screen(
         {{center.x - radius, center.y - radius}, {center.x + radius, center.y + radius}}))
    return;

  draw_arc_path(center, radius, start_angle, extent_angle, 1, true);
}

void renderer::draw_text(point2d point, std::string const &text)
{
  // call the draw_text function with no bounds
  draw_text(point, text, DBL_MAX, DBL_MAX);
}

void renderer::draw_text(point2d point, std::string const &text, double bound_x, double bound_y)
{
  // the center point of the text
  point2d center = point;

  // roughly calculate the center point for pre-clipping
  if (horiz_text_just == text_just::left)
    center.x += bound_x/2;
  else if (horiz_text_just == text_just::right)
    center.x -= bound_x/2;
  if (vert_text_just == text_just::top)
    center.y -= bound_y/2;
  else if (vert_text_just == text_just::bottom)
    center.y += bound_y/2;

  if(rectangle_off_screen({{center.x - bound_x / 2, center.y - bound_y / 2}, bound_x, bound_y}))
    return;

  // get the width and height of the drawn text
  cairo_text_extents_t text_extents{0,0,0,0,0,0};
  cairo_text_extents(m_cairo, text.c_str(), &text_extents);

  // get more information about the font used
  cairo_font_extents_t font_extents{0,0,0,0,0};
  cairo_font_extents(m_cairo, &font_extents);

  // get text width and height in world coordinates (text width and height are constant in widget coordinates)
  double scaled_width = text_extents.width * m_camera->get_world_scale_factor().x;
  double scaled_height = text_extents.height * m_camera->get_world_scale_factor().y;

  // if text width or height is greater than the given bounds, don't draw the text.
  // NOTE: text rotation is NOT taken into account in bounding check (i.e. text width is compared to bound_x)
  if(scaled_width > bound_x || scaled_height > bound_y) {
    return;
  }

  // save the current state to undo the rotation needed for drawing rotated text
  cairo_save(m_cairo);

  // transform the given point
  if(current_coordinate_system == WORLD)
    center = m_transform(point);
  else
    center = point;

  // calculating the reference point to center the text around "center" taking into account the rotation_angle
  // for more info about reference point location: see https://www.cairographics.org/tutorial/#L1understandingtext
  point2d ref_point = {0, 0};

  ref_point.x = center.x -
                (text_extents.x_bearing + (text_extents.width / 2)) * cos(rotation_angle) -
                (-font_extents.descent + (text_extents.height / 2)) * sin(rotation_angle);

  ref_point.y = center.y -
                (text_extents.y_bearing + (text_extents.height / 2)) * cos(rotation_angle) -
                (text_extents.x_bearing + (text_extents.width / 2)) * sin(rotation_angle);

  // adjust the reference point according to the required justification
  if (horiz_text_just == text_just::left) {
    ref_point.x += (text_extents.width / 2) * cos(rotation_angle);
    ref_point.y += (text_extents.width / 2) * sin(rotation_angle);
  }
  else if (horiz_text_just == text_just::right) {
    ref_point.x -= (text_extents.width / 2) * cos(rotation_angle);
    ref_point.y -= (text_extents.width / 2) * sin(rotation_angle);
  }
  if (vert_text_just == text_just::top) {
    ref_point.x -= (text_extents.height / 2) * sin(rotation_angle);
    ref_point.y += (text_extents.height / 2) * cos(rotation_angle);
  }
  else if (vert_text_just == text_just::bottom) {
    ref_point.x += (text_extents.height / 2) * sin(rotation_angle);
    ref_point.y -= (text_extents.height / 2) * cos(rotation_angle);
  }

  // move to the reference point, perform the rotation, and draw the text
  cairo_move_to(m_cairo, ref_point.x, ref_point.y);
  cairo_rotate(m_cairo, rotation_angle);
  cairo_show_text(m_cairo, text.c_str());

  // restore the old state to undo the performed rotation
  cairo_restore(m_cairo);
}

void renderer::draw_rectangle_path(point2d start, point2d end, bool fill_flag)
{
  if(current_coordinate_system == WORLD) {
    start = m_transform(start);
    end = m_transform(end);
  }

#ifdef EZGL_USE_X11
  if(!transparency_flag && x11_display != nullptr) {
    // Add 0.5 for extra half-pixel accuracy
    int start_x = static_cast<int>(start.x + 0.5);
    int start_y = static_cast<int>(start.y + 0.5);
    int end_x = static_cast<int>(end.x + 0.5);
    int end_y = static_cast<int>(end.y + 0.5);

    if(fill_flag)
      XFillRectangle(x11_display, x11_drawable, x11_context, std::min(start_x, end_x),
          std::min(start_y, end_y), std::abs(end_x - start_x), std::abs(end_y - start_y));
    else
      XDrawRectangle(x11_display, x11_drawable, x11_context, std::min(start_x, end_x),
          std::min(start_y, end_y), std::abs(end_x - start_x), std::abs(end_y - start_y));
    return;
  }
#endif

  cairo_move_to(m_cairo, start.x, start.y);
  cairo_line_to(m_cairo, start.x, end.y);
  cairo_line_to(m_cairo, end.x, end.y);
  cairo_line_to(m_cairo, end.x, start.y);

  cairo_close_path(m_cairo);

  // actual drawing
  if(fill_flag)
    cairo_fill(m_cairo);
  else
    cairo_stroke(m_cairo);
}

void renderer::draw_arc_path(point2d center,
    double radius,
    double start_angle,
    double extent_angle,
    double stretch_factor,
    bool fill_flag)
{
  // point_x is a point on the arc outline
  point2d point_x = {center.x + radius, center.y};

  // transform the center point of the arc, and the other point
  if(current_coordinate_system == WORLD) {
    center = m_transform(center);
    point_x = m_transform(point_x);
  }

  // calculate the new radius after transforming to the new coordinates
  radius = point_x.x - center.x;

#ifdef EZGL_USE_X11
  if(!transparency_flag && x11_display != nullptr) {
    if(fill_flag)
      XFillArc(x11_display, x11_drawable, x11_context, center.x - radius,
          center.y - radius * stretch_factor, 2 * radius, 2 * radius * stretch_factor,
          start_angle * 64, extent_angle * 64);
    else
      XDrawArc(x11_display, x11_drawable, x11_context, center.x - radius,
          center.y - radius * stretch_factor, 2 * radius, 2 * radius * stretch_factor,
          start_angle * 64, extent_angle * 64);
    return;
  }
#endif

  // save the current state to undo the scaling needed for drawing ellipse
  cairo_save(m_cairo);

  // scale the drawing by the stretch factor to draw elliptic circles
  cairo_scale(m_cairo, 1 / stretch_factor, 1);
  center.x = center.x * stretch_factor;
  radius = radius * stretch_factor;

  // start a new path (forget the current point). Alternative for cairo_move_to() for drawing non-filled arc
  cairo_new_path(m_cairo);

  // if the arc will be filled in, start drawing from the center of the arc
  if(fill_flag)
    cairo_move_to(m_cairo, center.x, center.y);

  // calculating the ending angle
  double end_angle = start_angle + extent_angle;

  // draw the arc in counter clock-wise direction if the extent angle is positive
  if(extent_angle >= 0) {
    cairo_arc_negative(
        m_cairo, center.x, center.y, radius, -start_angle * M_PI / 180, -end_angle * M_PI / 180);
  }
  // draw the arc in clock-wise direction if the extent angle is negative
  else {
    cairo_arc(
        m_cairo, center.x, center.y, radius, -start_angle * M_PI / 180, -end_angle * M_PI / 180);
  }

  // if the arc will be filled in, return back to the center of the arc
  if(fill_flag)
    cairo_close_path(m_cairo);

  // restore the old state to undo the scaling needed for drawing ellipse
  cairo_restore(m_cairo);

  // actual drawing
  if(fill_flag)
    cairo_fill(m_cairo);
  else
    cairo_stroke(m_cairo);
}

void renderer::draw_surface(surface *p_surface, point2d top_left)
{
  // Check if the surface is properly created
  if(cairo_surface_status(p_surface) != CAIRO_STATUS_SUCCESS)
    return;

  // pre-clipping
  double s_width = (double)cairo_image_surface_get_width(p_surface);
  double s_height = (double)cairo_image_surface_get_height(p_surface);

  if(rectangle_off_screen({{top_left.x, top_left.y - s_height}, s_width, s_height}))
    return;

  // transform the given top_left point
  if(current_coordinate_system == WORLD)
    top_left = m_transform(top_left);

  // Create a source for painting from the surface
  cairo_set_source_surface(m_cairo, p_surface, top_left.x, top_left.y);

  // Actual drawing
  cairo_paint(m_cairo);
}

surface *renderer::load_png(const char *file_path)
{
  // Create an image surface from a PNG image
  cairo_surface_t *png_surface = cairo_image_surface_create_from_png(file_path);

  return png_surface;
}

void renderer::free_surface(surface *p_surface)
{
  // Check if the surface is properly created
  if (cairo_surface_status(p_surface) == CAIRO_STATUS_SUCCESS)
    cairo_surface_destroy(p_surface);
}
}
