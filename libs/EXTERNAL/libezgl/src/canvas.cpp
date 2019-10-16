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

#include "ezgl/canvas.hpp"

#include "ezgl/graphics.hpp"

#include <gtk/gtk.h>

#include <cassert>
#include <cmath>
#include <functional>

namespace ezgl {

static cairo_surface_t *create_surface(GtkWidget *widget)
{
  GdkWindow *parent_window = gtk_widget_get_window(widget);
  int const width = gtk_widget_get_allocated_width(widget);
  int const height = gtk_widget_get_allocated_height(widget);

  // Cairo image surfaces are more efficient than normal Cairo surfaces
  // However, you cannot use X11 functions to draw on image surfaces
#ifdef EZGL_USE_X11
  return gdk_window_create_similar_surface(parent_window, CAIRO_CONTENT_COLOR_ALPHA, width, height);
#else
  return gdk_window_create_similar_image_surface(
      parent_window, CAIRO_FORMAT_ARGB32, width, height, 0);
#endif
}

static cairo_t *create_context(cairo_surface_t *p_surface)
{
  cairo_t *context = cairo_create(p_surface);

  // Set the antialiasing mode of the rasterizer used for drawing shapes
  // Set to CAIRO_ANTIALIAS_NONE for maximum speed
  // See https://www.cairographics.org/manual/cairo-cairo-t.html#cairo-antialias-t
  cairo_set_antialias(context, CAIRO_ANTIALIAS_NONE);

  return context;
}

bool canvas::print_pdf(const char *file_name, int output_width, int output_height)
{
  cairo_surface_t *pdf_surface;
  cairo_t *context;
  int surface_width = 0;
  int surface_height = 0;
  
  // create pdf surface based on canvas size
  if(output_width == 0 && output_height == 0){
    surface_width = gtk_widget_get_allocated_width(m_drawing_area);
    surface_height = gtk_widget_get_allocated_height(m_drawing_area);
  }else{
      surface_width = output_width;
      surface_height = output_height;
  }
  pdf_surface = cairo_pdf_surface_create(file_name, surface_width, surface_height);

  if(pdf_surface == NULL)
    return false; // failed to create due to errors such as out of memory
  context = create_context(pdf_surface);

  // draw on the newly created pdf surface & context
  cairo_set_source_rgb(context, m_background_color.red / 255.0, m_background_color.green / 255.0,
      m_background_color.blue / 255.0);
  cairo_paint(context);

  using namespace std::placeholders;
  camera pdf_cam = m_camera;
  pdf_cam.update_widget(surface_width, surface_height);
  renderer g(context, std::bind(&camera::world_to_screen, pdf_cam, _1), &pdf_cam, pdf_surface);
  m_draw_callback(&g);

  // free surface & context
  cairo_surface_destroy(pdf_surface);
  cairo_destroy(context);

  return true;
}

bool canvas::print_svg(const char *file_name, int output_width, int output_height)
{
  cairo_surface_t *svg_surface;
  cairo_t *context;
  int surface_width = 0;
  int surface_height = 0;
  
  // create pdf surface based on canvas size
  if(output_width == 0 && output_height == 0){
    surface_width = gtk_widget_get_allocated_width(m_drawing_area);
    surface_height = gtk_widget_get_allocated_height(m_drawing_area);
  }else{
      surface_width = output_width;
      surface_height = output_height;
  }
  svg_surface = cairo_svg_surface_create(file_name, surface_width, surface_height);

  if(svg_surface == NULL)
    return false; // failed to create due to errors such as out of memory
  context = create_context(svg_surface);

  // draw on the newly created svg surface & context
  cairo_set_source_rgb(context, m_background_color.red / 255.0, m_background_color.green / 255.0,
      m_background_color.blue / 255.0);
  cairo_paint(context);

  using namespace std::placeholders;
  camera svg_cam = m_camera;
  svg_cam.update_widget(surface_width, surface_height);
  renderer g(context, std::bind(&camera::world_to_screen, svg_cam, _1), &svg_cam, svg_surface);
  m_draw_callback(&g);

  // free surface & context
  cairo_surface_destroy(svg_surface);
  cairo_destroy(context);

  return true;
}

bool canvas::print_png(const char *file_name, int output_width, int output_height)
{
  cairo_surface_t *png_surface;
  cairo_t *context;
  int surface_width = 0;
  int surface_height = 0;
  
  // create pdf surface based on canvas size
  if(output_width == 0 && output_height == 0){
    surface_width = gtk_widget_get_allocated_width(m_drawing_area);
    surface_height = gtk_widget_get_allocated_height(m_drawing_area);
  }else{
      surface_width = output_width;
      surface_height = output_height;
  }
  png_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, surface_width, surface_height);

  if(png_surface == NULL)
    return false; // failed to create due to errors such as out of memory
  context = create_context(png_surface);

  // draw on the newly created png surface & context
  cairo_set_source_rgb(context, m_background_color.red / 255.0, m_background_color.green / 255.0,
      m_background_color.blue / 255.0);
  cairo_paint(context);

  using namespace std::placeholders;
  camera png_cam = m_camera;
  png_cam.update_widget(surface_width, surface_height);
  renderer g(context, std::bind(&camera::world_to_screen, png_cam, _1), &png_cam, png_surface);
  m_draw_callback(&g);

  // create png output file
  cairo_surface_write_to_png(png_surface, file_name);

  // free surface & context
  cairo_surface_destroy(png_surface);
  cairo_destroy(context);

  return true;
}

gboolean canvas::configure_event(GtkWidget *widget, GdkEventConfigure *, gpointer data)
{
  // User data should have been set during the signal connection.
  g_return_val_if_fail(data != nullptr, FALSE);

  auto ezgl_canvas = static_cast<canvas *>(data);
  auto &p_surface = ezgl_canvas->m_surface;
  auto &p_context = ezgl_canvas->m_context;

  if(p_surface != nullptr) {
    cairo_surface_destroy(p_surface);
  }

  if(p_context != nullptr) {
    cairo_destroy(p_context);
  }

  // Something has changed, recreate the surface.
  p_surface = create_surface(widget);

  // Recreate the context
  p_context = create_context(p_surface);

  // The camera needs to be updated before we start drawing again.
  ezgl_canvas->m_camera.update_widget(ezgl_canvas->width(), ezgl_canvas->height());

  // Draw to the newly created surface.
  ezgl_canvas->redraw();

  // Update the animation renderer
  if(ezgl_canvas->m_animation_renderer != nullptr)
    ezgl_canvas->m_animation_renderer->update_renderer(p_context, p_surface);

  g_info("canvas::configure_event has been handled.");
  return TRUE; // the configure event was handled
}

gboolean canvas::draw_surface(GtkWidget *, cairo_t *context, gpointer data)
{
  // Assume context and data are non-null.
  auto &p_surface = static_cast<canvas *>(data)->m_surface;

  // Assume surface is non-null.
  cairo_set_source_surface(context, p_surface, 0, 0);
  cairo_paint(context);

  return FALSE;
}

canvas::canvas(std::string canvas_id,
    draw_canvas_fn draw_callback,
    rectangle coordinate_system,
    color background_color)
    : m_canvas_id(std::move(canvas_id))
    , m_draw_callback(draw_callback)
    , m_camera(coordinate_system)
    , m_background_color(background_color)
{
}

canvas::~canvas()
{
  if(m_surface != nullptr) {
    cairo_surface_destroy(m_surface);
  }

  if(m_context != nullptr) {
    cairo_destroy(m_context);
  }

  if(m_animation_renderer != nullptr) {
    delete m_animation_renderer;
  }
}

int canvas::width() const
{
  return gtk_widget_get_allocated_width(m_drawing_area);
}

int canvas::height() const
{
  return gtk_widget_get_allocated_height(m_drawing_area);
}

void canvas::initialize(GtkWidget *drawing_area)
{
  g_return_if_fail(drawing_area != nullptr);

  m_drawing_area = drawing_area;
  m_surface = create_surface(m_drawing_area);
  m_context = create_context(m_surface);
  m_camera.update_widget(width(), height());

  // Draw to the newly created surface for the first time.
  redraw();

  // Connect to configure events in case our widget changes shape.
  g_signal_connect(m_drawing_area, "configure-event", G_CALLBACK(configure_event), this);
  // Connect to draw events so that we draw our surface to the drawing area.
  g_signal_connect(m_drawing_area, "draw", G_CALLBACK(draw_surface), this);

  // GtkDrawingArea objects need specific events enabled explicitly.
  gtk_widget_add_events(GTK_WIDGET(m_drawing_area), GDK_BUTTON_PRESS_MASK);
  gtk_widget_add_events(GTK_WIDGET(m_drawing_area), GDK_BUTTON_RELEASE_MASK);
  gtk_widget_add_events(GTK_WIDGET(m_drawing_area), GDK_POINTER_MOTION_MASK);
  gtk_widget_add_events(GTK_WIDGET(m_drawing_area), GDK_SCROLL_MASK);

  g_info("canvas::initialize successful.");
}

void canvas::redraw()
{
  // Clear the screen and set the background color
  cairo_set_source_rgb(m_context, m_background_color.red / 255.0, m_background_color.green / 255.0,
      m_background_color.blue / 255.0);
  cairo_paint(m_context);

  using namespace std::placeholders;
  renderer g(m_context, std::bind(&camera::world_to_screen, &m_camera, _1), &m_camera, m_surface);
  m_draw_callback(&g);

  gtk_widget_queue_draw(m_drawing_area);

  g_info("The canvas will be redrawn.");
}

renderer *canvas::create_animation_renderer()
{
  if(m_animation_renderer == nullptr) {
    using namespace std::placeholders;
    m_animation_renderer = new renderer(m_context, std::bind(&camera::world_to_screen, &m_camera, _1), &m_camera, m_surface);
  }

  return m_animation_renderer;
}
} // namespace ezgl
