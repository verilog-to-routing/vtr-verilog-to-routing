#ifndef VTR_GEOMETRY_H
#define VTR_GEOMETRY_H
#include "vtr_range.h"
#include "vtr_assert.h"

#include <vector>
#include <tuple>
#include <limits>
#include <type_traits>

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

//A point in 2D space
template<class T>
class Point {
  public: //Constructors
    Point(T x_val, T y_val) noexcept;

  public: //Accessors
    //Coordinates
    T x() const;
    T y() const;

    friend bool operator== <>(Point<T> lhs, Point<T> rhs);
    friend bool operator!= <>(Point<T> lhs, Point<T> rhs);
    friend bool operator< <>(Point<T> lhs, Point<T> rhs);

  public: //Mutators
    /* Set x and y values */
    void set(T x_val, T y_val);
    void set_x(T x_val);
    void set_y(T y_val);
    /* Swap x and y values */
    void swap();

  private:
    T x_;
    T y_;
};

//A 2D rectangle
template<class T>
class Rect {
  public: //Constructors
    Rect();
    Rect(T left_val, T bottom_val, T right_val, T top_val);
    Rect(Point<T> bottom_left_val, Point<T> top_right_val);

    //Constructs a rectangle that only contains the given point
    //  Rect(p1).contains(p2) => p1 == p2
    //It is only enabled for integral types, because making this work for floating point types would be difficult and brittle.
    //The following line only enables the constructor if std::is_integral<T>::value == true
    template<typename U = T, typename std::enable_if<std::is_integral<U>::value>::type...>
    Rect(Point<U> point);

  public: //Accessors
    //Co-ordinates
    T xmin() const;
    T xmax() const;
    T ymin() const;
    T ymax() const;

    Point<T> bottom_left() const;
    Point<T> top_right() const;

    T width() const;
    T height() const;

    //Returns true if the point is fully contained within the rectangle (excluding the top-right edges)
    bool contains(Point<T> point) const;

    //Returns true if the point is strictly contained within the region (excluding all edges)
    bool strictly_contains(Point<T> point) const;

    //Returns true if the point is coincident with the rectangle (including the top-right edges)
    bool coincident(Point<T> point) const;

    //Returns true if other is contained within the rectangle (including all edges)
    bool contains(const Rect<T>& other) const;

    //Returns true if no points are contained in the rectangle
    //  rect.empty() => not exists p. rect.contains(p)
    //  This also implies either the width or height is 0.
    bool empty() const;

    friend bool operator== <>(const Rect<T>& lhs, const Rect<T>& rhs);
    friend bool operator!= <>(const Rect<T>& lhs, const Rect<T>& rhs);

  public: //Mutators
    //Co-ordinates
    void set_xmin(T xmin_val);
    void set_ymin(T ymin_val);
    void set_xmax(T xmax_val);
    void set_ymax(T ymax_val);

    //Equivalent to `*this = bounding_box(*this, other)`
    Rect<T>& expand_bounding_box(const Rect<T>& other);

  private:
    Point<T> bottom_left_;
    Point<T> top_right_;
};

//Return the smallest rectangle containing both given rectangles
//Note that this isn't a union and the resulting rectangle may include points not in either given rectangle
template<class T>
Rect<T> bounding_box(const Rect<T>& lhs, const Rect<T>& rhs);

//Sample on a uniformly spaced grid within a rectangle
//  sample(vtr::Rect(l, h), 0, 0, M) == l
//  sample(vtr::Rect(l, h), M, M, M) == h
//To avoid the edges, use `sample(r, x+1, y+1, N+1) for x, y, in 0..N-1
//Only defined for integral types
template<typename T, typename std::enable_if<std::is_integral<T>::value>::type...>
Point<T> sample(const vtr::Rect<T>& r, T x, T y, T d);

// clamps v to be between low (lo) and high (hi), inclusive.
template<class T>
static constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return std::min(std::max(v, lo), hi);
}

//A 2D line
template<class T>
class Line {
  public: //Types
    typedef typename std::vector<Point<T>>::const_iterator point_iter;
    typedef vtr::Range<point_iter> point_range;

  public: //Constructors
    Line(std::vector<Point<T>> line_points);

  public: //Accessors
    //Returns the bounding box
    Rect<T> bounding_box() const;

    //Returns a range of constituent points
    point_range points() const;

  private:
    std::vector<Point<T>> points_;
};

//A union of 2d rectangles
template<class T>
class RectUnion {
  public: //Types
    typedef typename std::vector<Rect<T>>::const_iterator rect_iter;
    typedef vtr::Range<rect_iter> rect_range;

  public: //Constructors
    //Construct from a set of rectangles
    RectUnion(std::vector<Rect<T>> rects);

  public: //Accessors
    //Returns the bounding box of all rectangles in the union
    Rect<T> bounding_box() const;

    //Returns true if the point is fully contained within the region (excluding top-right edges)
    bool contains(Point<T> point) const;

    //Returns true if the point is strictly contained within the region (excluding all edges)
    bool strictly_contains(Point<T> point) const;

    //Returns true if the point is coincident with the region (including the top-right edges)
    bool coincident(Point<T> point) const;

    //Returns a range of all constituent rectangles
    rect_range rects() const;

    //Checks whether two RectUnions have identical representations
    // Note: does not check whether the representations they are equivalent
    friend bool operator== <>(const RectUnion<T>& lhs, const RectUnion<T>& rhs);
    friend bool operator!= <>(const RectUnion<T>& lhs, const RectUnion<T>& rhs);

  private:
    //Note that a union of rectanges may have holes and may not be contiguous
    std::vector<Rect<T>> rects_;
};

} // namespace vtr

#include "vtr_geometry.tpp"
#endif
