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

//Mutators
template<class T>
void Point<T>::set(T x_val, T y_val) {
    x_ = x_val;
    y_ = y_val;
}

template<class T>
void Point<T>::set_x(T x_val) {
    x_ = x_val;
}

template<class T>
void Point<T>::set_y(T y_val) {
    y_ = y_val;
}

template<class T>
void Point<T>::swap() {
    std::swap(x_, y_);
}

/*
 * Rect
 */
template<class T>
Rect<T>::Rect()
    : Rect<T>(Point<T>(0, 0), Point<T>(0, 0)) {
    //pass
}

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

//Only defined for integral types
template<class T>
template<typename U, typename std::enable_if<std::is_integral<U>::value>::type...>
Rect<T>::Rect(Point<U> point)
    : bottom_left_(point)
    , top_right_(point.x() + 1,
                 point.y() + 1) {
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
bool Rect<T>::contains(const Rect<T>& other) const {
    //Including all edges
    return other.xmin() >= xmin() && other.xmax() <= xmax()
           && other.ymin() >= ymin() && other.ymax() <= ymax();
}

template<class T>
bool Rect<T>::empty() const {
    return xmax() <= xmin() || ymax() <= ymin();
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

template<class T>
Rect<T> bounding_box(const Rect<T>& lhs, const Rect<T>& rhs) {
    return Rect<T>(std::min(lhs.xmin(), rhs.xmin()),
                   std::min(lhs.ymin(), rhs.ymin()),
                   std::max(lhs.xmax(), rhs.xmax()),
                   std::max(lhs.ymax(), rhs.ymax()));
}

template<class T>
Rect<T> intersection(const Rect<T>& lhs, const Rect<T>& rhs) {
    return Rect<T>(std::max(lhs.xmin(), rhs.xmin()),
                   std::max(lhs.ymin(), rhs.ymin()),
                   std::min(lhs.xmax(), rhs.xmax()),
                   std::min(lhs.ymax(), rhs.ymax()));
}
template<class T>
static void print_rect(FILE* fp, const Rect<T> rect) {
    fprintf(fp, "\txmin: %d\n", rect.xmin());
    fprintf(fp, "\tymin: %d\n", rect.ymin());
    fprintf(fp, "\txmax: %d\n", rect.xmax());
    fprintf(fp, "\tymax: %d\n", rect.ymax());
}
//Only defined for integral types
template<typename T, typename std::enable_if<std::is_integral<T>::value>::type...>
Point<T> sample(const vtr::Rect<T>& r, T x, T y, T d) {
    VTR_ASSERT(d > 0 && x <= d && y <= d && !r.empty());
    return Point<T>((r.xmin() * (d - x) + r.xmax() * x + d / 2) / d,
                    (r.ymin() * (d - y) + r.ymax() * y + d / 2) / d);
}

template<class T>
void Rect<T>::set_xmin(T xmin_val) {
    bottom_left_.set_x(xmin_val);
}

template<class T>
void Rect<T>::set_ymin(T ymin_val) {
    bottom_left_.set_y(ymin_val);
}

template<class T>
void Rect<T>::set_xmax(T xmax_val) {
    top_right_.set_x(xmax_val);
}

template<class T>
void Rect<T>::set_ymax(T ymax_val) {
    top_right_.set_y(ymax_val);
}

template<class T>
Rect<T>& Rect<T>::expand_bounding_box(const Rect<T>& other) {
    *this = bounding_box(*this, other);
    return *this;
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
