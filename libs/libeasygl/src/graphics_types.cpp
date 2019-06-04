#include "graphics_types.h"

#include "math.h"

using namespace std;

// Predefined colours
const std::array<t_color,NUM_COLOR> t_color::predef_colors = {
    t_color(0xFF, 0xFF, 0xFF), // "White"
    t_color(0x00, 0x00, 0x00), // "Black"

    // Remaining HTML colors
    t_color(0xF0, 0xF8, 0xFF), // "AliceBlue"
    t_color(0xFA, 0xEB, 0xD7), // "AntiqueWhite"
    t_color(0x00, 0xFF, 0xFF), // "Aqua"
    t_color(0x7F, 0xFF, 0xD4), // "Aquamarine"
    t_color(0xF0, 0xFF, 0xFF), // "Azure"
    t_color(0xF5, 0xF5, 0xDC), // "Beige"
    t_color(0xFF, 0xE4, 0xC4), // "Bisque"
    t_color(0xFF, 0xEB, 0xCD), // "BlanchedAlmond"
    t_color(0x00, 0x00, 0xFF), // "Blue"
    t_color(0x8A, 0x2B, 0xE2), // "BlueViolet"
    t_color(0xA5, 0x2A, 0x2A), // "Brown"
    t_color(0xDE, 0xB8, 0x87), // "BurlyWood"
    t_color(0x5F, 0x9E, 0xA0), // "CadetBlue"
    t_color(0x7F, 0xFF, 0x00), // "Chartreuse"
    t_color(0xD2, 0x69, 0x1E), // "Chocolate"
    t_color(0xFF, 0x7F, 0x50), // "Coral"
    t_color(0x64, 0x95, 0xED), // "CornflowerBlue"
    t_color(0xFF, 0xF8, 0xDC), // "Cornsilk"
    t_color(0xDC, 0x14, 0x3C), // "Crimson"
    t_color(0x00, 0xFF, 0xFF), // "Cyan"
    t_color(0x00, 0x00, 0x8B), // "DarkBlue"
    t_color(0x00, 0x8B, 0x8B), // "DarkCyan"
    t_color(0xB8, 0x86, 0x0B), // "DarkGoldenRod"
    t_color(0xA9, 0xA9, 0xA9), // "DarkGray"
    t_color(0x00, 0x64, 0x00), // "DarkGreen"
    t_color(0xA9, 0xA9, 0xA9), // "DarkGrey"
    t_color(0xBD, 0xB7, 0x6B), // "DarkKhaki"
    t_color(0x8B, 0x00, 0x8B), // "DarkMagenta"
    t_color(0x55, 0x6B, 0x2F), // "DarkOliveGreen"
    t_color(0xFF, 0x8C, 0x00), // "DarkOrange"
    t_color(0x99, 0x32, 0xCC), // "DarkOrchid"
    t_color(0x8B, 0x00, 0x00), // "DarkRed"
    t_color(0xE9, 0x96, 0x7A), // "DarkSalmon"
    t_color(0x8F, 0xBC, 0x8F), // "DarkSeaGreen"
    t_color(0x48, 0x3D, 0x8B), // "DarkSlateBlue"
    t_color(0x2F, 0x4F, 0x4F), // "DarkSlateGray"
    t_color(0x2F, 0x4F, 0x4F), // "DarkSlateGrey"
    t_color(0x00, 0xCE, 0xD1), // "DarkTurquoise"
    t_color(0x94, 0x00, 0xD3), // "DarkViolet"
    t_color(0xFF, 0x14, 0x93), // "DeepPink"
    t_color(0x00, 0xBF, 0xFF), // "DeepSkyBlue"
    t_color(0x69, 0x69, 0x69), // "DimGray"
    t_color(0x69, 0x69, 0x69), // "DimGrey"
    t_color(0x1E, 0x90, 0xFF), // "DodgerBlue"
    t_color(0xB2, 0x22, 0x22), // "FireBrick"
    t_color(0xFF, 0xFA, 0xF0), // "FloralWhite"
    t_color(0x22, 0x8B, 0x22), // "ForestGreen"
    t_color(0xFF, 0x00, 0xFF), // "Fuchsia"
    t_color(0xDC, 0xDC, 0xDC), // "Gainsboro"
    t_color(0xF8, 0xF8, 0xFF), // "GhostWhite"
    t_color(0xFF, 0xD7, 0x00), // "Gold"
    t_color(0xDA, 0xA5, 0x20), // "GoldenRod"
    t_color(0x80, 0x80, 0x80), // "Gray"
    t_color(0x00, 0x80, 0x00), // "Green"
    t_color(0xAD, 0xFF, 0x2F), // "GreenYellow"
    t_color(0x80, 0x80, 0x80), // "Grey"
    t_color(0x8C, 0x8C, 0x8C), // "grey55"
    t_color(0xBF, 0xBF, 0xBF), // "grey75"
    t_color(0xF0, 0xFF, 0xF0), // "HoneyDew"
    t_color(0xFF, 0x69, 0xB4), // "HotPink"
    t_color(0xCD, 0x5C, 0x5C), // "IndianRed"
    t_color(0x4B, 0x00, 0x82), // "Indigo"
    t_color(0xFF, 0xFF, 0xF0), // "Ivory"
    t_color(0xF0, 0xE6, 0x8C), // "Khaki"
    t_color(0xE6, 0xE6, 0xFA), // "Lavender"
    t_color(0xFF, 0xF0, 0xF5), // "LavenderBlush"
    t_color(0x7C, 0xFC, 0x00), // "LawnGreen"
    t_color(0xFF, 0xFA, 0xCD), // "LemonChiffon"
    t_color(0xAD, 0xD8, 0xE6), // "LightBlue"
    t_color(0xF0, 0x80, 0x80), // "LightCoral"
    t_color(0xE0, 0xFF, 0xFF), // "LightCyan"
    t_color(0xFA, 0xFA, 0xD2), // "LightGoldenRodYellow"
    t_color(0xD3, 0xD3, 0xD3), // "LightGray"
    t_color(0x90, 0xEE, 0x90), // "LightGreen"
    t_color(0xD3, 0xD3, 0xD3), // "LightGrey"
    t_color(0x44, 0x44, 0xFF), // "lightmediumblue"
    t_color(0xFF, 0xB6, 0xC1), // "LightPink"
    t_color(0xFF, 0xA0, 0x7A), // "LightSalmon"
    t_color(0x20, 0xB2, 0xAA), // "LightSeaGreen"
    t_color(0x87, 0xCE, 0xFA), // "LightSkyBlue"
    t_color(0x77, 0x88, 0x99), // "LightSlateGray"
    t_color(0x77, 0x88, 0x99), // "LightSlateGrey"
    t_color(0xB0, 0xC4, 0xDE), // "LightSteelBlue"
    t_color(0xFF, 0xFF, 0xE0), // "LightYellow"
    t_color(0x00, 0xFF, 0x00), // "Lime"
    t_color(0x32, 0xCD, 0x32), // "LimeGreen"
    t_color(0xFA, 0xF0, 0xE6), // "Linen"
    t_color(0xFF, 0x00, 0xFF), // "Magenta"
    t_color(0x80, 0x00, 0x00), // "Maroon"
    t_color(0x66, 0xCD, 0xAA), // "MediumAquaMarine"
    t_color(0x00, 0x00, 0xCD), // "MediumBlue"
    t_color(0xBA, 0x55, 0xD3), // "MediumOrchid"
    t_color(0x93, 0x70, 0xDB), // "MediumPurple"
    t_color(0x3C, 0xB3, 0x71), // "MediumSeaGreen"
    t_color(0x7B, 0x68, 0xEE), // "MediumSlateBlue"
    t_color(0x00, 0xFA, 0x9A), // "MediumSpringGreen"
    t_color(0x48, 0xD1, 0xCC), // "MediumTurquoise"
    t_color(0xC7, 0x15, 0x85), // "MediumVioletRed"
    t_color(0x19, 0x19, 0x70), // "MidnightBlue"
    t_color(0xF5, 0xFF, 0xFA), // "MintCream"
    t_color(0xFF, 0xE4, 0xE1), // "MistyRose"
    t_color(0xFF, 0xE4, 0xB5), // "Moccasin"
    t_color(0xFF, 0xDE, 0xAD), // "NavajoWhite"
    t_color(0x00, 0x00, 0x80), // "Navy"
    t_color(0xFD, 0xF5, 0xE6), // "OldLace"
    t_color(0x80, 0x80, 0x00), // "Olive"
    t_color(0x6B, 0x8E, 0x23), // "OliveDrab"
    t_color(0xFF, 0xA5, 0x00), // "Orange"
    t_color(0xFF, 0x45, 0x00), // "OrangeRed"
    t_color(0xDA, 0x70, 0xD6), // "Orchid"
    t_color(0xEE, 0xE8, 0xAA), // "PaleGoldenRod"
    t_color(0x98, 0xFB, 0x98), // "PaleGreen"
    t_color(0xAF, 0xEE, 0xEE), // "PaleTurquoise"
    t_color(0xDB, 0x70, 0x93), // "PaleVioletRed"
    t_color(0xFF, 0xEF, 0xD5), // "PapayaWhip"
    t_color(0xFF, 0xDA, 0xB9), // "PeachPuff"
    t_color(0xCD, 0x85, 0x3F), // "Peru"
    t_color(0xFF, 0xC0, 0xCB), // "Pink"
    t_color(0xDD, 0xA0, 0xDD), // "Plum"
    t_color(0xB0, 0xE0, 0xE6), // "PowderBlue"
    t_color(0x80, 0x00, 0x80), // "Purple"
    t_color(0x66, 0x33, 0x99), // "RebeccaPurple"
    t_color(0xFF, 0x00, 0x00), // "Red"
    t_color(0xBC, 0x8F, 0x8F), // "RosyBrown"
    t_color(0x41, 0x69, 0xE1), // "RoyalBlue"
    t_color(0x8B, 0x45, 0x13), // "SaddleBrown"
    t_color(0xFA, 0x80, 0x72), // "Salmon"
    t_color(0xF4, 0xA4, 0x60), // "SandyBrown"
    t_color(0x2E, 0x8B, 0x57), // "SeaGreen"
    t_color(0xFF, 0xF5, 0xEE), // "SeaShell"
    t_color(0xA0, 0x52, 0x2D), // "Sienna"
    t_color(0xC0, 0xC0, 0xC0), // "Silver"
    t_color(0x87, 0xCE, 0xEB), // "SkyBlue"
    t_color(0x6A, 0x5A, 0xCD), // "SlateBlue"
    t_color(0x70, 0x80, 0x90), // "SlateGray"
    t_color(0x70, 0x80, 0x90), // "SlateGrey"
    t_color(0xFF, 0xFA, 0xFA), // "Snow"
    t_color(0x00, 0xFF, 0x7F), // "SpringGreen"
    t_color(0x46, 0x82, 0xB4), // "SteelBlue"
    t_color(0xD2, 0xB4, 0x8C), // "Tan"
    t_color(0x00, 0x80, 0x80), // "Teal"
    t_color(0xD8, 0xBF, 0xD8), // "Thistle"
    t_color(0xFF, 0x63, 0x47), // "Tomato"
    t_color(0x40, 0xE0, 0xD0), // "Turquoise"
    t_color(0xEE, 0x82, 0xEE), // "Violet"
    t_color(0xF5, 0xDE, 0xB3), // "Wheat"
    t_color(0xF5, 0xF5, 0xF5), // "WhiteSmoke"
    t_color(0xFF, 0xFF, 0x00), // "Yellow"
    t_color(0x9A, 0xCD, 0x32), // "YellowGreen"
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

t_point& t_point::operator=(const t_point&) = default;

t_point::t_point() = default;

t_point::t_point(const t_point&) = default;

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

color_types t_color::operator=(color_types /*color_enum*/) {
    *this = t_color(0, 0, 0);
    return BLACK;
}

bool t_color::operator==(color_types /*rhs*/) const {
    return false;
}

#endif /* NO_GRAPHICS */

bool t_color::operator!=(color_types rhs) const {
    return !(*this == rhs);
}
