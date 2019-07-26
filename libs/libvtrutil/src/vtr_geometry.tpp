namespace vtr {
/*
 * Point
 */

template<class T>
Point<T>::Point(T x_val, T y_val) noexcept
    : x_(x_val)
    , y_(y_val) {
    //pass
}

template<class T>
T Point<T>::x() const {
    return x_;
}

template<class T>
T Point<T>::y() const {
    return y_;
}

template<class T>
bool operator==(Point<T> lhs, Point<T> rhs) {
    return lhs.x() == rhs.x()
           && lhs.y() == rhs.y();
}

template<class T>
bool operator!=(Point<T> lhs, Point<T> rhs) {
    return !(lhs == rhs);
}

template<class T>
bool operator<(Point<T> lhs, Point<T> rhs) {
    return std::make_tuple(lhs.x(), lhs.y()) < std::make_tuple(rhs.x(), rhs.y());
}

/*
 * Rect
 */
template<class T>
Rect<T>::Rect(T left_val, T bottom_val, T right_val, T top_val)
    : Rect<T>(Point<T>(left_val, bottom_val), Point<T>(right_val, top_val)) {
    //pass
}

template<class T>
Rect<T>::Rect(Point<T> bottom_left_val, Point<T> top_right_val)
    : bottom_left_(bottom_left_val)
    , top_right_(top_right_val) {
    //pass
}

template<class T>
T Rect<T>::xmin() const {
    return bottom_left_.x();
}

template<class T>
T Rect<T>::xmax() const {
    return top_right_.x();
}

template<class T>
T Rect<T>::ymin() const {
    return bottom_left_.y();
}

template<class T>
T Rect<T>::ymax() const {
    return top_right_.y();
}

template<class T>
Point<T> Rect<T>::bottom_left() const {
    return bottom_left_;
}

template<class T>
Point<T> Rect<T>::top_right() const {
    return top_right_;
}

template<class T>
T Rect<T>::width() const {
    return xmax() - xmin();
}

template<class T>
T Rect<T>::height() const {
    return ymax() - ymin();
}

template<class T>
bool Rect<T>::contains(Point<T> point) const {
    //Up-to but not including right or top edges
    return point.x() >= xmin() && point.x() < xmax()
           && point.y() >= ymin() && point.y() < ymax();
}

template<class T>
bool Rect<T>::strictly_contains(Point<T> point) const {
    //Excluding edges
    return point.x() > xmin() && point.x() < xmax()
           && point.y() > ymin() && point.y() < ymax();
}

template<class T>
bool Rect<T>::coincident(Point<T> point) const {
    //Including right or top edges
    return point.x() >= xmin() && point.x() <= xmax()
           && point.y() >= ymin() && point.y() <= ymax();
}

template<class T>
bool operator==(const Rect<T>& lhs, const Rect<T>& rhs) {
    return lhs.bottom_left() == rhs.bottom_left()
           && lhs.top_right() == rhs.top_right();
}

template<class T>
bool operator!=(const Rect<T>& lhs, const Rect<T>& rhs) {
    return !(lhs == rhs);
}

/*
 * Line
 */
template<class T>
Line<T>::Line(std::vector<Point<T>> line_points)
    : points_(line_points) {
    //pass
}

template<class T>
Rect<T> Line<T>::bounding_box() const {
    T xmin = std::numeric_limits<T>::max();
    T ymin = std::numeric_limits<T>::max();
    T xmax = std::numeric_limits<T>::min();
    T ymax = std::numeric_limits<T>::min();

    for (const auto& point : points()) {
        xmin = std::min(xmin, point.x());
        ymin = std::min(ymin, point.y());
        xmax = std::max(xmax, point.x());
        ymax = std::max(ymax, point.y());
    }

    return Rect<T>(xmin, ymin, xmax, ymax);
}

template<class T>
typename Line<T>::point_range Line<T>::points() const {
    return vtr::make_range(points_.begin(), points_.end());
}

/*
 * RectUnion
 */
template<class T>
RectUnion<T>::RectUnion(std::vector<Rect<T>> rectangles)
    : rects_(rectangles) {
    //pass
}

template<class T>
Rect<T> RectUnion<T>::bounding_box() const {
    T xmin = std::numeric_limits<T>::max();
    T ymin = std::numeric_limits<T>::max();
    T xmax = std::numeric_limits<T>::min();
    T ymax = std::numeric_limits<T>::min();

    for (const auto& rect : rects_) {
        xmin = std::min(xmin, rect.xmin());
        ymin = std::min(ymin, rect.ymin());
        xmax = std::max(xmax, rect.xmax());
        ymax = std::max(ymax, rect.ymax());
    }

    return Rect<T>(xmin, ymin, xmax, ymax);
}

template<class T>
bool RectUnion<T>::contains(Point<T> point) const {
    for (const auto& rect : rects()) {
        if (rect.contains(point)) {
            return true;
        }
    }
    return false;
}

template<class T>
bool RectUnion<T>::strictly_contains(Point<T> point) const {
    for (const auto& rect : rects()) {
        if (rect.strictly_contains(point)) {
            return true;
        }
    }
    return false;
}

template<class T>
bool RectUnion<T>::coincident(Point<T> point) const {
    for (const auto& rect : rects()) {
        if (rect.coincident(point)) {
            return true;
        }
    }
    return false;
}

template<class T>
typename RectUnion<T>::rect_range RectUnion<T>::rects() const {
    return vtr::make_range(rects_.begin(), rects_.end());
}

template<class T>
bool operator==(const RectUnion<T>& lhs, const RectUnion<T>& rhs) {
    //Currently checks for an identical *representation* (not whether the
    //representations are equivalent)

    if (lhs.rects_.size() != rhs.rects_.size()) {
        return false;
    }

    for (size_t i = 0; i < lhs.rects_.size(); ++i) {
        if (lhs.rects_[i] != rhs.rects_[i]) {
            return false;
        }
    }

    return true;
}

template<class T>
bool operator!=(const RectUnion<T>& lhs, const RectUnion<T>& rhs) {
    return !(lhs == rhs);
}
} // namespace vtr
