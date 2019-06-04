#ifndef VTR_GEOMETRY_H
#define VTR_GEOMETRY_H
#include "vtr_range.h"
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

  private:
    T x_;
    T y_;
};

//A 2D rectangle
template<class T>
class Rect {
  public: //Constructors
    Rect(T left_val, T bottom_val, T right_val, T top_val);
    Rect(Point<T> bottom_left_val, Point<T> top_right_val);

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

    friend bool operator== <>(const Rect<T>& lhs, const Rect<T>& rhs);
    friend bool operator!= <>(const Rect<T>& lhs, const Rect<T>& rhs);

  private:
    Point<T> bottom_left_;
    Point<T> top_right_;
};

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
