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

#ifndef EZGL_POINT_HPP
#define EZGL_POINT_HPP

namespace ezgl {

/**
 * Represents a two-dimensional point.
 */
class point2d {
public:
  /**
   * Create a point at the given x and y position.
   */
  point2d(double x_coord, double y_coord) : x(x_coord), y(y_coord)
  {
  }

  /**
   * Location of the x-coordinate.
   */
  double x = 0.0;

  /**
   * Location of the y-coordinate.
   */
  double y = 0.0;

  /**
   * Test for equality.
   */
  friend bool operator==(point2d const &lhs, point2d const &rhs)
  {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
  }

  /**
   * Test for inequality.
   */
  friend bool operator!=(point2d const &lhs, point2d const &rhs)
  {
    return !(lhs == rhs);
  }

  /**
   * Create a new point that is the sum of two points.
   */
  friend point2d operator+(point2d const &lhs, point2d const &rhs)
  {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
  }

  /**
   * Add one point to another point.
   */
  friend point2d &operator+=(point2d &lhs, point2d const &rhs)
  {
    lhs.x += rhs.x;
    lhs.y += rhs.y;

    return lhs;
  }

  /**
   * Create a new point that is the difference of two points.
   */
  friend point2d operator-(point2d const &lhs, point2d const &rhs)
  {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
  }

  /**
   * Subtract one point from another point.
   */
  friend point2d &operator-=(point2d &lhs, point2d const &rhs)
  {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;

    return lhs;
  }

  /**
   * Create a new point that is the product of two points.
   */
  friend point2d operator*(point2d const &lhs, point2d const &rhs)
  {
    return {lhs.x * rhs.x, lhs.y * rhs.y};
  }

  /**
   * Multiply one point with another point.
   */
  friend point2d &operator*=(point2d &lhs, point2d const &rhs)
  {
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;

    return lhs;
  }
};
}

#endif //EZGL_POINT_HPP
