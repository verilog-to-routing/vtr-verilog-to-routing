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

#ifndef EZGL_GRAPHICS_HPP
#define EZGL_GRAPHICS_HPP

#include "ezgl/color.hpp"
#include "ezgl/point.hpp"
#include "ezgl/rectangle.hpp"
#include "ezgl/camera.hpp"

#include <cairo.h>
#include <gdk/gdk.h>

#ifdef CAIRO_HAS_XLIB_SURFACE
#ifdef GDK_WINDOWING_X11
#include <cairo-xlib.h>

// Speed up draw calls by using X11 instead of cairo wherever possible.
#define EZGL_USE_X11
#endif
#endif

#include <functional>
#include <string>
#include <vector>
#include <cfloat>
#include <cmath>
#include <algorithm>

namespace ezgl {

/**
 * define ezgl::surface type used for drawing pngs
 */
typedef cairo_surface_t surface;

/**
 * Available coordinate systems
 */
enum t_coordinate_system {
  /**
   * Default coordinate system
   */
  WORLD,
  /**
   * Screen coordinate system. Screen Coordinates are not transformed so the drawn objects do not pan or zoom.
   */
  SCREEN
};

/**
 * Text justification options
 */
enum class text_just {
  /**
   * Center Justification: used for both vertical and horizontal justification
   */
  center,
  /**
   * Left justification: used for horizontal justification
   */
  left,
  /**
   * Right justification: used for horizontal justification
   */
  right,
  /**
   * Top justification: used for vertical justification
   */
  top,
  /**
   * Bottom justification: used for vertical justification
   */
  bottom
};

/**
 * The slant of the font.
 *
 * This enum is setup to match with the cairo graphics library and should not be changed.
 */
enum class font_slant : int {
  /**
   * No slant.
   */
  normal = CAIRO_FONT_SLANT_NORMAL,

  /**
   * Slant is more calligraphic. Make sure the font you're using has an italic design, otherwise it may look ugly.
   */
  italic = CAIRO_FONT_SLANT_ITALIC,

  /**
   * Slanted to the right.
   */
  oblique = CAIRO_FONT_SLANT_OBLIQUE
};

/**
 * The weight of the font.
 */
enum class font_weight : int {
  /**
   * No additional weight.
   */
  normal = CAIRO_FONT_WEIGHT_NORMAL,

  /**
   * Bold font weight.
   */
  bold = CAIRO_FONT_WEIGHT_BOLD
};

/**
 * The shape of a line's start and end point.
 */
enum class line_cap : int {
  /**
   * Start and stop the line exactly where it begins/ends.
   */
  butt = CAIRO_LINE_CAP_BUTT,

  /**
   * Each end of the line has circles.
   */
  round = CAIRO_LINE_CAP_ROUND
};

/**
 * The dash style of a line.
 */
enum class line_dash : int {
  /**
   * No dashes in the line (i.e., solid).
   */
  none,

  /**
   * Dash to whitespace ratio is 5:3.
   */
  asymmetric_5_3
};

/**
 * Provides functions to draw primitives (e.g., lines, shapes) to a rendering context.
 *
 * The renderer modifies a cairo_t context based on draw calls. The renderer uses an ezgl::camera object to convert
 * world coordinates into cairo's expected coordinate system.
 */
class renderer {
public:
  /**
   * Change the current coordinate system
   *
   * @param new_coordinate_system The drawing coordinate system SCREEN or WORLD
   */
  void set_coordinate_system(t_coordinate_system new_coordinate_system);

  /**
   * Set the visible bounds of the world
   *
   * The function preserves the aspect ratio of the initial world
   *
   * @param new_world The new visible bounds of the world
   */
  void set_visible_world(rectangle new_world);

  /**
   * Get the current visible bounds of the world
   */
  rectangle get_visible_world();

  /**
   * Get the current visible bounds of the screen
   */
  rectangle get_visible_screen();

  /**
   * Get the screen coordinates (i.e. pixel locations) of the world coordinate rectangle box
   *
   * @param box: a rectangle in world coordinates
   */
  rectangle world_to_screen(const rectangle& box);

  /**** Functions to set graphics attributes (for all subsequent drawing calls). ****/

  /**
   * Change the color for subsequent draw calls.
   *
   * @param new_color The new color to use.
   */
  void set_color(color new_color);

  /**
   * Change the color for subsequent draw calls.
   *
   * @param new_color The new color to use.
   * @param alpha Overwrite the alpha channel in the chosen color.
   */
  void set_color(color new_color, uint_fast8_t alpha);

  /**
   * Change the color for subsequent draw calls.
   *
   * @param red The amount of red to use, between 0 and 255.
   * @param green The amount of green to use, between 0 and 255.
   * @param blue The amount of blue to use, between 0 and 255.
   * @param alpha The transparency level (0 is fully transparent, 255 is opaque).
   */
  void set_color(uint_fast8_t red, uint_fast8_t green, uint_fast8_t blue, uint_fast8_t alpha = 255);

  /**
   * Change how line endpoints will be rendered in subsequent draw calls.
   */
  void set_line_cap(line_cap cap);

  /**
   * Change the dash style of the line.
   */
  void set_line_dash(line_dash dash);

  /**
   * Set the line width.
   *
   * @param width The width in pixels. 
   * A value of 0 is still one pixel wide but about 100x faster 
   * to draw than other line widths.
   */
  void set_line_width(int width);

  /**
   * Change the font size.
   *
   * @param new_size The new size text should be drawn at.
   */
  void set_font_size(double new_size);

  /**
   * Change the font.
   *
   * @param family The font family to use (e.g., serif)
   * @param slant The slant to use (e.g., italic)
   * @param weight The weight of the font (e.g., bold)
   */
  void format_font(std::string const &family, font_slant slant, font_weight weight);

  /**
   * Change the font.
   *
   * @param family The font family to use (e.g., serif)
   * @param slant The slant to use (e.g., italic)
   * @param weight The weight of the font (e.g., bold)
   * @param new_size The new size text should be drawn at.
   */
  void
  format_font(std::string const &family, font_slant slant, font_weight weight, double new_size);

  /**
   * set the rotation_angle variable that is used in rotating text.
   *
   * @param degrees The angle by which the text should rotate, in degrees.
   */
  void set_text_rotation(double degrees);

  /**
   * set horizontal text justification.
   *
   * @param horiz_just Options: center, left and right justification.
   */
  void set_horiz_text_just(text_just horiz_just);

  /**
   * set vertical text justification.
   *
   * @param vert_just Options: center, top and bottom justification.
   */
  void set_vert_text_just(text_just vert_just);

  /**** Functions to draw various graphics primitives ****/

  /**
   * Draw a line.
   *
   * @param start The start point of the line, in pixels.
   * @param end The end point of the line, in pixels.
   */
  void draw_line(point2d start, point2d end);

  /**
   * Draw the outline a rectangle.
   *
   * @param start The start point of the rectangle, in pixels.
   * @param end The end point of the rectangle, in pixels.
   */
  void draw_rectangle(point2d start, point2d end);

  /**
   * Draw the outline of a rectangle.
   *
   * @param start The start point of the rectangle, in pixels.
   * @param width How wide the rectangle is, in pixels.
   * @param height How high the rectangle is, in pixels.
   */
  void draw_rectangle(point2d start, double width, double height);

  /**
   * Draw the outline of a rectangle.
   */
  void draw_rectangle(rectangle r);

  /**
   * Draw a filled in rectangle.
   *
   * @param start The start point of the rectangle, in pixels.
   * @param end The end point of the rectangle, in pixels.
   */
  void fill_rectangle(point2d start, point2d end);

  /**
   * Draw a filled in rectangle.
   *
   * @param start The start point of the rectangle, in pixels.
   * @param width How wide the rectangle is, in pixels.
   * @param height How high the rectangle is, in pixels.
   */
  void fill_rectangle(point2d start, double width, double height);

  /**
   * Draw a filled in rectangle.
   */
  void fill_rectangle(rectangle r);

  /**
   * Draw a filled polygon.
   *
   * @param points The points to draw. The first and last points are connected to close the polygon.
   */
  void fill_poly(std::vector<point2d> const &points);

  /**
   * Draw the outline of an elliptic arc.
   *
   * @param center The center of the arc, in pixels.
   * @param radius_x The x radius of the elliptic arc, in pixels.
   * @param radius_y The y radius of the elliptic arc, in pixels.
   * @param start_angle The starting angle of the arc, in degrees.
   * @param extent_angle The extent angle of the arc, in degrees.
   */
  void draw_elliptic_arc(point2d center,
      double radius_x,
      double radius_y,
      double start_angle,
      double extent_angle);

  /**
   * Draw the outline of an arc.
   *
   * @param center The center of the arc, in pixels.
   * @param radius The radius of the arc, in pixels.
   * @param start_angle The starting angle of the arc, in degrees.
   * @param extent_angle The extent angle of the arc, in degrees.
   */
  void draw_arc(point2d center, double radius, double start_angle, double extent_angle);

  /**
   * Draw a filled in elliptic arc.
   *
   * @param center The center of the arc, in pixels.
   * @param radius_x The x radius of the elliptic arc, in pixels.
   * @param radius_y The y radius of the elliptic arc, in pixels.
   * @param start_angle The starting angle of the arc, in degrees.
   * @param extent_angle The extent angle of the arc, in degrees.
   */
  void fill_elliptic_arc(point2d center,
      double radius_x,
      double radius_y,
      double start_angle,
      double extent_angle);

  /**
   * Draw a filled in arc.
   *
   * @param center The center of the arc, in pixels.
   * @param radius The radius of the arc, in pixels.
   * @param start_angle The starting angle of the arc, in degrees.
   * @param extent_angle The extent angle of the arc, in degrees.
   */
  void fill_arc(point2d center, double radius, double start_angle, double extent_angle);

  /**
   * Draw text.
   *
   * @param point The point where the text is drawn, in pixels.
   * @param text The text to draw.
   */
  void draw_text(point2d point, std::string const &text);

  /**
   * Draw text with bounds.
   *
   * @param point The point where the text is drawn, in pixels.
   * @param text The text to draw.
   * @param bound_x The maximum allowed width of the text
   * @param bound_y The maximum allowed height of the text
   */
  void draw_text(point2d point, std::string const &text, double bound_x, double bound_y);

  /**
   * Draw a surface
   *
   * @param surface The surface to draw
   * @param top_left The corner point of the drawn surface.
   */
  void draw_surface(surface *surface, point2d top_left);

  /**
   * load a png image
   *
   * @param file_path The path to the png image.
   *
   * @return a pointer to the created surface. Should be freed using free_surface()
   */
  static surface *load_png(const char *file_path);

  /**
   * Free a surface
   *
   * @param surface The surface to destroy
   */
  static void free_surface(surface *surface);

  /**
   * Destructor.
   */
  ~renderer();

protected:
  // Only the canvas class can create a renderer.
  friend class canvas;

  /**
   * A callback for transforming points from one coordinate system to another.
   */
  using transform_fn = std::function<point2d(point2d)>;

  /**
   * Constructor.
   *
   * @param cairo The cairo graphics state.
   * @param transform The function to use to transform points to cairo's coordinate system.
   */
  renderer(cairo_t *cairo, transform_fn transform, camera *m_camera, cairo_surface_t *m_surface);

  /**
   * Update the renderer when the cairo surface/context changes
   *
   * @param cairo The new cairo graphics state
   * @param m_surface The new cairo surface
   */
  void update_renderer(cairo_t *cairo, cairo_surface_t *m_surface);

private:
  void draw_rectangle_path(point2d start, point2d end, bool fill_flag);

  void draw_arc_path(point2d center,
      double radius,
      double start_angle,
      double extent_angle,
      double stretch_factor,
      bool fill_flag);

  // Pre-clipping function
  bool rectangle_off_screen(rectangle rect);

  // Current coordinate system (World is the default)
  t_coordinate_system current_coordinate_system = WORLD;

  // A non-owning pointer to a cairo graphics context.
  cairo_t *m_cairo;

#ifdef EZGL_USE_X11
  // The x11 drawable
  Drawable x11_drawable;

  // The x11 display
  Display *x11_display = nullptr;

  // The x11 context
  GC x11_context;

  // Transparency flag, if set, cairo will be used
  bool transparency_flag = false;
#endif

  transform_fn m_transform;

  //A non-owning pointer to camera object
  camera *m_camera;

  // the rotation angle variable used in rotating text
  double rotation_angle;

  // Current horizontal text justification
  text_just horiz_text_just = text_just::center;

  // Current vertical text justification
  text_just vert_text_just = text_just::center;

  // Current line width
  int current_line_width = 1;

  // Current line cap
  line_cap current_line_cap = line_cap::butt;

  // Current line dash
  line_dash current_line_dash = line_dash::none;

  // Current color
  color current_color = {0, 0, 0, 255};
};
}

#endif //EZGL_GRAPHICS_HPP
