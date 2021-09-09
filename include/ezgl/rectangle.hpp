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

#ifndef EZGL_RECTANGLE_HPP
#define EZGL_RECTANGLE_HPP

#include "ezgl/point.hpp"

#include <algorithm>

namespace ezgl {

/**
 * Represents a rectangle as two diagonally opposite points.
 */
class rectangle {
public:
  /**
   * Default constructor: Create a zero-sized rectangle at {0,0}.
   */
  rectangle() : m_first({0, 0}), m_second({0, 0})
  {
  }

  /**
   * Create a rectangle from two diagonally opposite points.
   */
  rectangle(point2d origin_pt, point2d top_right_pt) : m_first(origin_pt), m_second(top_right_pt)
  {
  }

  /**
   * Create a rectangle with a given width and height.
   */
  rectangle(point2d origin_pt, double rec_width, double rec_height) : m_first(origin_pt), m_second(origin_pt)
  {
    m_second.x += rec_width;
    m_second.y += rec_height;
  }

  /**
   * The minimum x-coordinate.
   */
  double left() const
  {
    return std::min(m_first.x, m_second.x);
  }

  /**
   * The maximum x-coordinate.
   */
  double right() const
  {
    return std::max(m_first.x, m_second.x);
  }

  /**
   * The minimum y-coordinate.
   */
  double bottom() const
  {
    return std::min(m_first.y, m_second.y);
  }

  /**
   * The maximum y-coordinate.
   */
  double top() const
  {
    return std::max(m_first.y, m_second.y);
  }

  /**
   * The minimum x-coordinate and the minimum y-coordinate.
   */
  point2d bottom_left() const
  {
    return {left(), bottom()};
  }

  /**
   * The minimum x-coordinate and the maximum y-coordinate.
   */
  point2d top_left() const
  {
    return {left(), top()};
  }

  /**
   * The maximum x-coordinate and the minimum y-coordinate.
   */
  point2d bottom_right() const
  {
    return {right(), bottom()};
  }

  /**
   * The maximum x-coordinate and the maximum y-coordinate.
   */
  point2d top_right() const
  {
    return {right(), top()};
  }

  /**
   * Test if the x and y values are within the rectangle.
   */
  bool contains(double x, double y) const
  {
    if(x < left() || right() < x || y < bottom() || top() < y) {
      return false;
    }

    return true;
  }

  /**
   * Test if the x and y values are within the rectangle.
   */
  bool contains(point2d point) const
  {
    return contains(point.x, point.y);
  }

  /**
   * The width of the rectangle.
   */
  double width() const
  {
    return right() - left();
  }

  /**
   * The height of the rectangle.
   */
  double height() const
  {
    return top() - bottom();
  }

  /**
   *
   * The area of the rectangle.
   */
  double area() const
  {
    return width() * height();
  }

  /**
   * The center of the rectangle in the x plane.
   */
  double center_x() const
  {
    return (right() + left()) * 0.5;
  }

  /**
   * The center of the rectangle in the y plane.
   */
  double center_y() const
  {
    return (top() + bottom()) * 0.5;
  }

  /**
   * The center of the recangle.
   */
  point2d center() const
  {
    return {center_x(), center_y()};
  }

  /**
   * Test for equality.
   */
  bool operator==(const rectangle &rhs) const
  {
    return m_first == rhs.m_first && m_second == rhs.m_second;
  }

  /**
   * Test for inequality.
   */
  bool operator!=(const rectangle &rhs) const
  {
    return !(rhs == *this);
  }

  /**
   * translate the rectangle by positive offsets.
   */
  friend rectangle &operator+=(rectangle &lhs, point2d const &rhs)
  {
    lhs.m_first += rhs;
    lhs.m_second += rhs;

    return lhs;
  }

  /**
   * translate the rectangle by negative offsets.
   */
  friend rectangle &operator-=(rectangle &lhs, point2d const &rhs)
  {
    lhs.m_first -= rhs;
    lhs.m_second -= rhs;

    return lhs;
  }

  /**
   * Create a new rectangle that is translated (negative offsets).
   */
  friend rectangle operator-(rectangle &lhs, point2d const &rhs)
  {
    return rectangle(lhs.m_first - rhs, lhs.m_second - rhs);
  }

  /**
   * Create a new rectangle that is translated (positive offsets).
   */
  friend rectangle operator+(rectangle &lhs, point2d const &rhs)
  {
    return rectangle(lhs.m_first + rhs, lhs.m_second + rhs);
  }

  /** The first point of the rectangle */
  point2d m_first;

  /** The second point of the rectangle */
  point2d m_second;
};
}

#endif //EZGL_RECTANGLE_HPP
