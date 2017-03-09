#include "graphics_types.h"

#include "math.h"

using namespace std;

// Predefined colours
const vector<t_color> t_color::predef_colors = {
    t_color(0xFF, 0xFF, 0xFF), // "white"
    t_color(0x00, 0x00, 0x00), // "black"
    t_color(0x8C, 0x8C, 0x8C), // "grey55"
    t_color(0xBF, 0xBF, 0xBF), // "grey75"
    t_color(0xFF, 0x00, 0x00), // "red"
    t_color(0xFF, 0xA5, 0x00), // "orange"
    t_color(0xFF, 0xFF, 0x00), // "yellow"
    t_color(0x00, 0xFF, 0x00), // "green"
    t_color(0x00, 0xFF, 0xFF), // "cyan"
    t_color(0x00, 0x00, 0xFF), // "blue"
    t_color(0xA0, 0x20, 0xF0), // "purple"
    t_color(0xFF, 0xC0, 0xCB), // "pink"
    t_color(0xFF, 0xB6, 0xC1), // "lightpink"
    t_color(0x00, 0x64, 0x00), // "darkgreen"
    t_color(0xFF, 0x00, 0xFF), // "magenta"
    t_color(0xFF, 0xE4, 0xC4), // "bisque"
    t_color(0x87, 0xCE, 0xFA), // "lightskyblue"
    t_color(0xD8, 0xBF, 0xD8), // "thistle"
    t_color(0xDD, 0xA0, 0xDD), // "plum"
    t_color(0xF0, 0xE6, 0x8C), // "khaki"
    t_color(0xFF, 0x7F, 0x50), // "coral"
    t_color(0x40, 0xE0, 0xD0), // "turquoise"
    t_color(0x93, 0x70, 0xDB), // "mediumpurple"
    t_color(0x48, 0x3D, 0x8B), // "darkslateblue"
    t_color(0xBD, 0xB7, 0x6B), // "darkkhaki"
    t_color(0x44, 0x44, 0xFF), // "lightmediumblue"
    t_color(0x8B, 0x45, 0x13), // "saddlebrown"
    t_color(0xB2, 0x22, 0x22), // "firebrick"
    t_color(0x32, 0xCD, 0x32) // "limegreen"
};


/****************** begin definition of data structure members *********************/

/******************************************
 * begin t_point function definitions *
 ******************************************/

void t_point::offset(float _x, float _y) {
    x += _x;
    y += _y;
}

t_point t_point::operator-(const t_point& rhs) const {
    t_point result = *this;
    result -= rhs;
    return result;
}

t_point t_point::operator+(const t_point& rhs) const {
    t_point result = *this;
    result += rhs;
    return result;
}

t_point t_point::operator*(float rhs) const {
    t_point result = *this;
    result *= rhs;
    return result;
}

t_point& t_point::operator+=(const t_point& rhs) {
    this->x += rhs.x;
    this->y += rhs.y;
    return *this;
}

t_point& t_point::operator-=(const t_point& rhs) {
    this->x -= rhs.x;
    this->y -= rhs.y;
    return *this;
}

t_point& t_point::operator*=(float rhs) {
    this->x *= rhs;
    this->y *= rhs;
    return *this;
}

t_point& t_point::operator=(const t_point& src) {
    this->x = src.x;
    this->y = src.y;
    return *this;
}

t_point::t_point() : x(0), y(0) {
}

t_point::t_point(const t_point& src) :
x(src.x), y(src.y) {
}

t_point::t_point(float _x, float _y) : x(_x), y(_y) {
}

/******************************************
 * begin t_bound_box function definitions *
 ******************************************/

const float& t_bound_box::left() const {
    return bottom_left().x;
}

const float& t_bound_box::right() const {
    return top_right().x;
}

const float& t_bound_box::bottom() const {
    return bottom_left().y;
}

const float& t_bound_box::top() const {
    return top_right().y;
}

const t_point& t_bound_box::bottom_left() const {
    return bottomleft;
}

const t_point& t_bound_box::top_right() const {
    return topright;
}

float& t_bound_box::left() {
    return bottom_left().x;
}

float& t_bound_box::right() {
    return top_right().x;
}

float& t_bound_box::bottom() {
    return bottom_left().y;
}

float& t_bound_box::top() {
    return top_right().y;
}

t_point& t_bound_box::bottom_left() {
    return bottomleft;
}

t_point& t_bound_box::top_right() {
    return topright;
}

float t_bound_box::get_xcenter() const {
    return (right() + left()) / 2;
}

float t_bound_box::get_ycenter() const {
    return (top() + bottom()) / 2;
}

t_point t_bound_box::get_center() const {
    return t_point(get_xcenter(), get_ycenter());
}

float t_bound_box::get_width() const {
    return right() - left();
}

float t_bound_box::get_height() const {
    return top() - bottom();
}

void t_bound_box::offset(const t_point& relative_to) {
    this->bottomleft += relative_to;
    this->topright += relative_to;
}

void t_bound_box::offset(float by_x, float by_y) {
    this->bottomleft.offset(by_x, by_y);
    this->topright.offset(by_x, by_y);
}

bool t_bound_box::intersects(const t_point& test_pt) const {
    return intersects(test_pt.x, test_pt.y);
}

bool t_bound_box::intersects(float x, float y) const {
    if (x < left() || right() < x || y < bottom() || top() < y) {
        return false;
    } else {
        return true;
    }
}

float t_bound_box::area() const {
    return fabs(get_width() * get_height());
}

t_bound_box t_bound_box::operator+(const t_point& rhs) const {
    t_bound_box result = *this;
    result.offset(rhs);
    return result;
}

t_bound_box t_bound_box::operator-(const t_point& rhs) const {
    t_bound_box result = *this;
    result.offset(t_point(-rhs.x, -rhs.y));
    return result;
}

t_bound_box& t_bound_box::operator+=(const t_point& rhs) {
    this->offset(rhs);
    return *this;
}

t_bound_box& t_bound_box::operator-=(const t_point& rhs) {
    this->offset(t_point(-rhs.x, -rhs.y));
    return *this;
}

t_bound_box& t_bound_box::operator=(const t_bound_box& src) {
    this->bottom_left() = src.bottom_left();
    this->top_right() = src.top_right();
    return *this;
}

t_bound_box::t_bound_box() :
bottomleft(), topright() {
}

t_bound_box::t_bound_box(const t_bound_box& src) :
bottomleft(src.bottom_left()), topright(src.top_right()) {
}

t_bound_box::t_bound_box(float _left, float _bottom, float _right, float _top) :
bottomleft(_left, _bottom), topright(_right, _top) {
}

t_bound_box::t_bound_box(const t_point& _bottomleft, const t_point& _topright) :
bottomleft(_bottomleft), topright(_topright) {
}

t_bound_box::t_bound_box(const t_point& _bottomleft, float width, float height) :
bottomleft(_bottomleft), topright(_bottomleft) {
    topright.offset(width, height);
}

/******************************************
 * begin t_color function definitions *
 ******************************************/

t_color::t_color(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a) :
        red(r),
        green(g),
        blue(b),
        alpha(a)
{}

t_color::t_color(const t_color& src) :
        red(src.red),
        green(src.green),
        blue(src.blue),
        alpha(src.alpha)
{}

t_color::t_color() :
        red(0),
        green(0),
        blue(0),
        alpha(255)
{}

t_color::t_color(color_types src) {
    *this = src;
}

bool t_color::operator==(const t_color& rhs) const {
    return red == rhs.red
        && green == rhs.green
        && blue == rhs.blue
        && alpha == rhs.alpha;
}

bool t_color::operator!=(const t_color& rhs) const {
    return !(*this == rhs);
}


#ifndef NO_GRAPHICS

color_types t_color::operator=(color_types color_enum) {
    *this = predef_colors[color_enum];
    return color_enum;
}

bool t_color::operator==(color_types rhs) const {
    const t_color& test = predef_colors[rhs];
    if (test == *this) {
        return true;
    } else {
        return false;
    }
}

#else /* WITHOUT GRAPHICS */

color_types t_color::operator=(color_types color_enum) {
    *this = t_color(0, 0, 0);
    return BLACK;
}

bool t_color::operator==(color_types rhs) const {
    return false;
}

#endif /* NO_GRAPHICS */

bool t_color::operator!=(color_types rhs) const {
    return !(*this == rhs);
}