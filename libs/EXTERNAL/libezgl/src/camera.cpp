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

#include "ezgl/camera.hpp"

#include <cmath>
#include <cstdio>

namespace ezgl {

static rectangle maintain_aspect_ratio(rectangle const &view, double widget_width, double widget_height)
{
  double const x_scale = widget_width / view.width();
  double const y_scale = widget_height / view.height();

  double x_start = 0.0;
  double y_start = 0.0;
  double new_width;
  double new_height;

  if(x_scale * view.height() > widget_height) {
    // Using x_scale causes the view to be larger than the widget's height.

    // Keep the same height as the widget.
    new_height = widget_height;
    // Scale the width to maintain the aspect ratio.
    new_width = view.width() * y_scale;
    // Keep the view in the center of the widget.
    x_start = 0.5 * std::fabs(widget_width - new_width);
  } else {
    // Using x_scale keeps the view within the widget's height.

    // Keep the width the same as the widget.
    new_width = widget_width;
    // Scale the height to maintain the aspect ratio.
    new_height = view.height() * x_scale;
    // Keep the view in the center of the widget.
    y_start = 0.5 * std::fabs(widget_height - new_height);
  }

  return {{x_start, y_start}, new_width, new_height};
}

camera::camera(rectangle bounds) : m_world(bounds), m_screen(bounds), m_initial_world(bounds)
{
}

point2d camera::widget_to_screen(point2d widget_coordinates) const
{
  point2d const screen_origin = {m_screen.left(), m_screen.bottom()};
  point2d screen_coordinates = widget_coordinates - screen_origin;

  return screen_coordinates;
}

point2d camera::widget_to_world(point2d widget_coordinates) const
{
  point2d const screen_coordinates = widget_to_screen(widget_coordinates);

  point2d world_coordinates = screen_coordinates * m_screen_to_world;

  world_coordinates.x += m_world.left();

  // GTK and cairo use a flipped y-axis.
  world_coordinates.y = (world_coordinates.y - m_world.top()) * -1.0;

  return world_coordinates;
}

/**
 * Some X11 implementations overflow with sufficiently large pixel
 * coordinates and start drawing strangely. We will clip all pixels
 * to lie in the range below.
 * TODO: We can also switch to cairo for large pixel coordinates
 */
#define MAXPIXEL 10000.0
#define MINPIXEL -10000.0

point2d camera::world_to_screen(point2d world_coordinates) const
{
  point2d const world_origin{m_world.left(), m_world.bottom()};
  point2d widget_coordinates = (world_coordinates - world_origin) * m_world_to_widget;

  // GTK and cairo use a flipped y-axis.
  widget_coordinates.y = (widget_coordinates.y - m_widget.top()) * -1.0;

  point2d screen_coordinates = widget_coordinates * m_widget_to_screen;

  point2d const screen_origin = {m_screen.left(), m_screen.bottom()};
  screen_coordinates = screen_coordinates + screen_origin;

  screen_coordinates.x = std::max(screen_coordinates.x, MINPIXEL);
  screen_coordinates.y = std::max(screen_coordinates.y, MINPIXEL);
  screen_coordinates.x = std::min(screen_coordinates.x, MAXPIXEL);
  screen_coordinates.y = std::min(screen_coordinates.y, MAXPIXEL);

  return screen_coordinates;
}

void camera::set_world(rectangle new_world)
{
  m_world = new_world;

  update_scale_factors();
}

void camera::reset_world(rectangle new_world)
{
  // Change the coordinates to the new bounds
  m_world = new_world;
  m_screen = new_world;
  m_initial_world = new_world;

  m_screen = maintain_aspect_ratio(m_screen, m_widget.width(), m_widget.height());
  update_scale_factors();
}

void camera::update_widget(int width, int height)
{
  m_widget = rectangle{{0, 0}, static_cast<double>(width), static_cast<double>(height)};

  m_screen = maintain_aspect_ratio(m_screen, m_widget.width(), m_widget.height());
  update_scale_factors();
}

void camera::update_scale_factors()
{
  m_widget_to_screen.x = m_screen.width() / m_widget.width();
  m_widget_to_screen.y = m_screen.height() / m_widget.height();

  m_world_to_widget.x = m_widget.width() / m_world.width();
  m_world_to_widget.y = m_widget.height() / m_world.height();

  m_screen_to_world.x = m_world.width() / m_screen.width();
  m_screen_to_world.y = m_world.height() / m_screen.height();
}
}
