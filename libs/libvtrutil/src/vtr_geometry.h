#ifndef VTR_GEOMETRY_H
#define VTR_GEOMETRY_H
#include "vtr_range.h"
#include "vtr_assert.h"

#include <cstdio> // vtr_geometry.tpp uses printf()

#include <vector>
#include <tuple>
#include <limits>
#include <type_traits>

/**
 * @file
 * @brief   This file include differents different geometry classes
 */

namespace vtr {

/*
 * Forward declarations
 */
template<class T>
class Point;

template<class T>
class Rect;

template<class T>
class Line;

template<class T>
class RectUnion;

template<class T>
bool operator==(Point<T> lhs, Point<T> rhs);
template<class T>
bool operator!=(Point<T> lhs, Point<T> rhs);
template<class T>
bool operator<(Point<T> lhs, Point<T> rhs);

template<class T>
bool operator==(const Rect<T>& lhs, const Rect<T>& rhs);
template<class T>
bool operator!=(const Rect<T>& lhs, const Rect<T>& rhs);

template<class T>
bool operator==(const RectUnion<T>& lhs, const RectUnion<T>& rhs);
template<class T>
bool operator!=(const RectUnion<T>& lhs, const RectUnion<T>& rhs);
/*
 * Class Definitions
 */

/**
 * @brief A point in 2D space
 *
 * This class represents a point in 2D space. Hence, it holds both
 * x and y components of the point. 
 */
template<class T>
class Point {
  public: //Constructors
    Point(T x_val, T y_val) noexcept;

  public: //Accessors
    ///@brief x coordinate
    T x() const;

    ///@brief y coordinate
    T y() const;

    ///@brief == operator
    friend bool operator== <>(Point<T> lhs, Point<T> rhs);

    ///@brief != operator
    friend bool operator!= <>(Point<T> lhs, Point<T> rhs);

    ///@brief < operator
    friend bool operator< <>(Point<T> lhs, Point<T> rhs);

  public: //Mutators
    ///@brief Set x and y values
    void set(T x_val, T y_val);

    ///@brief set x value
    void set_x(T x_val);

    ///@brief set y value
    void set_y(T y_val);

    ///@brief Swap x and y values
    void swap();

  private:
    T x_;
    T y_;
};

/**
 * @brief A 2D rectangle
 *
 * This class represents a 2D rectangle. It can be created with 
 * its 4 points or using the bottom left and the top rights ones only
 */
template<class T>
class Rect {
  public: //Constructors
    ///@brief default constructor
    Rect();

    ///@brief construct using 4 vertex
    Rect(T left_val, T bottom_val, T right_val, T top_val);

    ///@brief construct using the bottom left and the top right vertex
    Rect(Point<T> bottom_left_val, Point<T> top_right_val);

    /**
     * @brief Constructs a rectangle that only contains the given point
     *
     * Rect(p1).contains(p2) => p1 == p2
     * It is only enabled for integral types, because making this work for floating point types would be difficult and brittle.
     * The following line only enables the constructor if std::is_integral<T>::value == true
     */
    template<typename U = T, typename std::enable_if<std::is_integral<U>::value>::type...>
    Rect(Point<U> point);

  public: //Accessors
    ///@brief xmin coordinate
    T xmin() const;

    ///@brief xmax coordinate
    T xmax() const;

    ///@brief ymin coodrinate
    T ymin() const;

    ///@brief ymax coordinate
    T ymax() const;

    ///@brief Return the bottom left point
    Point<T> bottom_left() const;

    ///@brief Return the top right point
    Point<T> top_right() const;

    ///@brief Return the rectangle width
    T width() const;

    ///@brief Return the rectangle height
    T height() const;

    ///@brief Returns true if the point is fully contained within the rectangle (excluding the top-right edges)
    bool contains(Point<T> point) const;

    ///@brief Returns true if the point is strictly contained within the region (excluding all edges)
    bool strictly_contains(Point<T> point) const;

    ///@brief Returns true if the point is coincident with the rectangle (including the top-right edges)
    bool coincident(Point<T> point) const;

    ///@brief Returns true if other is contained within the rectangle (including all edges)
    bool contains(const Rect<T>& other) const;

    /**
     * @brief Checks whether the rectangle is empty
     *
     * Returns true if no points are contained in the rectangle
     * rect.empty() => not exists p. rect.contains(p)
     * This also implies either the width or height is 0.
     */
    bool empty() const;

    ///@brief == operator
    friend bool operator== <>(const Rect<T>& lhs, const Rect<T>& rhs);

    ///@brief != operator
    friend bool operator!= <>(const Rect<T>& lhs, const Rect<T>& rhs);

  public: //Mutators
    ///@brief set xmin to a point
    void set_xmin(T xmin_val);

    ///@brief set ymin to a point
    void set_ymin(T ymin_val);

    ///@brief set xmax to a point
    void set_xmax(T xmax_val);

    ///@brief set ymax to a point
    void set_ymax(T ymax_val);

    ///@brief Equivalent to `*this = bounding_box(*this, other)`
    Rect<T>& expand_bounding_box(const Rect<T>& other);

  private:
    Point<T> bottom_left_;
    Point<T> top_right_;
};

/**
 * @brief Return the smallest rectangle containing both given rectangles
 *
 * Note that this isn't a union and the resulting rectangle may include points not in either given rectangle
 */
template<class T>
Rect<T> bounding_box(const Rect<T>& lhs, const Rect<T>& rhs);

///@brief Return the intersection of two given rectangles
template<class T>
Rect<T> intersection(const Rect<T>& lhs, const Rect<T>& rhs);

//Prints a rectangle
template<class T>
static void print_rect(FILE* fp, const Rect<T> rect);

//Sample on a uniformly spaced grid within a rectangle
//  sample(vtr::Rect(l, h), 0, 0, M) == l
//  sample(vtr::Rect(l, h), M, M, M) == h
//To avoid the edges, use `sample(r, x+1, y+1, N+1) for x, y, in 0..N-1
//Only defined for integral types

/**
 * @brief Sample on a uniformly spaced grid within a rectangle
 *
 * sample(vtr::Rect(l, h), 0, 0, M) == l
 * sample(vtr::Rect(l, h), M, M, M) == h
 * To avoid the edges, use `sample(r, x+1, y+1, N+1) for x, y, in 0..N-1
 * Only defined for integral types
 */

template<typename T, typename std::enable_if<std::is_integral<T>::value>::type...>
Point<T> sample(const vtr::Rect<T>& r, T x, T y, T d);

///@brief clamps v to be between low (lo) and high (hi), inclusive.
template<class T>
static constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return std::min(std::max(v, lo), hi);
}

/**
 * @brief A 2D line
 *
 * It is constructed using a vector of the line points
 */
template<class T>
class Line {
  public: //Types
    typedef typename std::vector<Point<T>>::const_iterator point_iter;
    typedef vtr::Range<point_iter> point_range;

  public: //Constructors
    ///@brief contructor
    Line(std::vector<Point<T>> line_points);

  public: //Accessors
    ///@brief Returns the bounding box
    Rect<T> bounding_box() const;

    ///@brief Returns a range of constituent points
    point_range points() const;

  private:
    std::vector<Point<T>> points_;
};

///@brief A union of 2d rectangles
template<class T>
class RectUnion {
  public: //Types
    typedef typename std::vector<Rect<T>>::const_iterator rect_iter;
    typedef vtr::Range<rect_iter> rect_range;

  public: //Constructors
    ///@brief Construct from a set of rectangles
    RectUnion(std::vector<Rect<T>> rects);

  public: //Accessors
    ///@brief Returns the bounding box of all rectangles in the union
    Rect<T> bounding_box() const;

    ///@brief Returns true if the point is fully contained within the region (excluding top-right edges)
    bool contains(Point<T> point) const;

    ///@brief Returns true if the point is strictly contained within the region (excluding all edges)
    bool strictly_contains(Point<T> point) const;

    ///@brief Returns true if the point is coincident with the region (including the top-right edges)
    bool coincident(Point<T> point) const;

    ///@brief Returns a range of all constituent rectangles
    rect_range rects() const;

    /**
     * @brief Checks whether two RectUnions have identical representations
     *
     * Note: does not check whether the representations they are equivalent
     */
    friend bool operator== <>(const RectUnion<T>& lhs, const RectUnion<T>& rhs);

    ///@brief != operator
    friend bool operator!= <>(const RectUnion<T>& lhs, const RectUnion<T>& rhs);

  private:
    // Note that a union of rectanges may have holes and may not be contiguous
    std::vector<Rect<T>> rects_;
};

} // namespace vtr

#include "vtr_geometry.tpp"
#endif
