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

#ifndef EZGL_CAMERA_HPP
#define EZGL_CAMERA_HPP

#include "ezgl/point.hpp"
#include "ezgl/rectangle.hpp"

namespace ezgl {

/**
 * Manages the transformations between coordinate systems.
 * Application code doesn't (and can't) call these functions; they are for ezgl internal use.
 *
 * The camera class manages transformations between a GTK widget, world, and "screen" coordinate system. A GTK widget
 * has dimensions that change based on the user, and its aspect ratio may not match the world coordinate system. The
 * camera maintains a "screen" within the widget that keeps the same aspect ratio as the world coordinate system,
 * regardless of the dimensions of the widget.
 *
 * A camera object can only be created by an ezgl::canvas object, who has the responsibility of updating the camera with
 * changes to the widget's dimensions. The only state that can be mutated outside the library is the camera's world
 * coordinate system.
 */
class camera {
public:
  /**
   * Convert a point in world coordinates to screen coordinates.
   */
  point2d world_to_screen(point2d world_coordinates) const;

  /**
   * Convert a point in widget coordinates to screen coordinates.
   */
  point2d widget_to_screen(point2d widget_coordinates) const;

  /**
   * Convert a point in widget coordinates to world coordinates.
   */
  point2d widget_to_world(point2d widget_coordinates) const;

  /**
   * Get the currently visible bounds of the world.
   */
  rectangle get_world() const
  {
    return m_world;
  }

  /**
   * Get the dimensions of the screen.
   */
  rectangle get_screen() const
  {
    return m_screen;
  }

  /**
   * Get the dimensions of the widget.
   */
  rectangle get_widget() const
  {
    return m_widget;
  }

  /**
   * Get the initial bounds of the world. Needed for zoom_fit
   */
  rectangle get_initial_world() const
  {
    return m_initial_world;
  }

  /**
   * Update the visible bounds of the world.
   *
   * Used in panning and zooming.
   */
  void set_world(rectangle new_world);

  /**
   * Reset the world coordinates
   *
   * Used by change_canvas_world_coordinates().
   */
  void reset_world(rectangle new_world);

  /**
   * Get the screen to world scaling factor.
   */
  point2d get_world_scale_factor() const
  {
    return m_screen_to_world;
  }

protected:
  // Only an ezgl::canvas can create a camera.
  friend class canvas;

  /**
   * Create a camera.
   *
   * @param bounds The initial bounds of the coordinate system.
   */
  explicit camera(rectangle bounds);

  /**
   * Update the dimensions of the widget.
   *
   * This will change the screen where the world is projected. The screen will maintain the aspect ratio of the world's
   * coordinate system while being centered within the screen.
   *
   * @see canvas::configure_event
   */
  void update_widget(int width, int height);

  /**
   * Update the scaling factors.
   */
  void update_scale_factors();

private:
  // The dimensions of the parent widget.
  rectangle m_widget = {{0, 0}, 1.0, 1.0};

  // The dimensions of the world (user-defined bounding box).
  rectangle m_world;

  // The dimensions of the screen, which may not match the widget.
  rectangle m_screen;

  // The dimensions of the initial world (user-defined bounding box). Needed for zoom_fit
  rectangle m_initial_world;

  // The x and y scaling factors.
  point2d m_world_to_widget = {1.0, 1.0};
  point2d m_widget_to_screen = {1.0, 1.0};
  point2d m_screen_to_world = {1.0, 1.0};
};
}

#endif //EZGL_CAMERA_HPP
