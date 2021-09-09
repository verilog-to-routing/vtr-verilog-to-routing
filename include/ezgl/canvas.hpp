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

#ifndef EZGL_CANVAS_HPP
#define EZGL_CANVAS_HPP

#include "ezgl/camera.hpp"
#include "ezgl/rectangle.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/color.hpp"

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <gtk/gtk.h>

#include <string>

namespace ezgl {

/**** Functions in this class are for ezgl internal use; application code doesn't need to call them ****/

class renderer;

/**
 * The signature of a function that draws to an ezgl::canvas.
 */
using draw_canvas_fn = void (*)(renderer*);

/**
 * Responsible for creating, destroying, and maintaining the rendering context of a GtkWidget.
 *
 * Underneath, the class relies on a GtkDrawingArea as its GUI widget along with cairo to provide the rendering context.
 * The class connects to the relevant GTK signals, namely configure and draw events, to remain responsive.
 *
 * Each canvas is double-buffered. A draw callback (see: ezgl::draw_canvas_fn) is invoked each time the canvas needs to
 * be redrawn. This may be caused by the user (e.g., resizing the screen), but can also be forced by the programmer.
 */
class canvas {
public:
  /**
   * Destructor.
   */
  ~canvas();

  /**
   * Get the name (identifier) of the canvas.
   */
  char const *id() const
  {
    return m_canvas_id.c_str();
  }

  /**
   * Get the width of the canvas in pixels.
   */
  int width() const;

  /**
   * Get the height of the canvas in pixels.
   */
  int height() const;

  /**
   * Force the canvas to redraw itself.
   *
   * This will invoke the ezgl::draw_canvas_fn callback and queue a redraw of the GtkWidget.
   */
  void redraw();

  /**
   * Get an immutable reference to this canvas' camera.
   */
  camera const &get_camera() const
  {
    return m_camera;
  }

  /**
   * Get a mutable reference to this canvas' camera.
   */
  camera &get_camera()
  {
    return m_camera;
  }

  /**
   * Create an animation renderer that can be used to draw on top of the current canvas
   */
  renderer *create_animation_renderer();
  
  /**
   * print_pdf, print_svg, and print_png generate a PDF, SVG, or PNG output file showing 
   * all the graphical content of the current canvas. 
   * 
   * @param file_name   name of the output file
   * @return            returns true if the function has successfully generated the output file, otherwise
   *                    failed due to errors such as out of memory occurs. 
   */
  bool print_pdf(const char *file_name, int width = 0, int height = 0);
  bool print_svg(const char *file_name, int width = 0, int height = 0);
  bool print_png(const char *file_name, int width = 0, int height = 0);
  
  
protected:
  // Only the ezgl::application can create and initialize a canvas object.
  friend class application;

  /**
   * Create a canvas that can be drawn to.
   */
  canvas(std::string canvas_id, draw_canvas_fn draw_callback, rectangle coordinate_system, color background_color);

  /**
   * Lazy initialization of the canvas class.
   *
   * This function is required because GTK will not send activate/startup signals to an ezgl::application until control
   * of the program has been reliquished. The GUI is not built until ezgl::application receives an activate signal.
   */
  void initialize(GtkWidget *drawing_area);

private:
  // Name of the canvas in XML.
  std::string m_canvas_id;

  // The function to call when the widget needs to be redrawn.
  draw_canvas_fn m_draw_callback;

  // The transformations between the GUI and the world.
  camera m_camera;

  // The background color of the drawing area
  color m_background_color;

  // A non-owning pointer to the drawing area inside a GTK window.
  GtkWidget *m_drawing_area = nullptr;

  // The off-screen surface that can be drawn to.
  cairo_surface_t *m_surface = nullptr;

  // The off-screen cairo context that can be drawn to
  cairo_t *m_context = nullptr;

  // The animation renderer
  renderer *m_animation_renderer = nullptr;

private:
  // Called each time our drawing area widget has changed (e.g., in size).
  static gboolean configure_event(GtkWidget *widget, GdkEventConfigure *event, gpointer data);

  // Called each time we need to draw to our drawing area widget.
  static gboolean draw_surface(GtkWidget *widget, cairo_t *context, gpointer data);
};
}

#endif //EZGL_CANVAS_HPP
