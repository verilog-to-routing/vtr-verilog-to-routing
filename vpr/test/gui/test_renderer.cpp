/**
 * @file test_renderer.cpp
 * @brief Layer 3 unit tests for ezgl primitive value types (S8 of the
 *        GUI test plan, §7.8).
 *
 * Scope discipline: the ezgl renderer hierarchy (immediate_renderer,
 * deferred_renderer, rhi_renderer) is composed of pure-virtual classes
 * that require an active QPainter or QRhi surface to instantiate. Layer 5
 * already exercises these end-to-end (SSIM compare against goldens) and
 * Layer 1 exercises them via headless `--graphics_commands save_graphics`.
 *
 * The unit-testable surface left for Layer 3 is the ezgl **value types**:
 * `color`, `point2d`, `rectangle`. These are constexpr-friendly value
 * objects with no graphics-context dependencies — exactly the right shape
 * for Catch2 unit cases. Coverage of the renderer subclass implementations
 * stays the responsibility of Layer 1/5, where it already exists.
 *
 * Per §1.A: nothing here mocks a rendering surface to inflate Layer 3
 * numbers. If a value-type assertion fails, file DEF-NNN; do not soften.
 *
 * Tag: [layer3][vpr_gui][renderer]
 */

#include <catch2/catch_test_macros.hpp>

#include <ezgl/color.hpp>
#include <ezgl/point.hpp>
#include <ezgl/rectangle.hpp>

using ezgl::color;
using ezgl::point2d;
using ezgl::rectangle;

// ---------------------------------------------------------------------------
// color
// ---------------------------------------------------------------------------

TEST_CASE("color: default constructor produces opaque black",
          "[layer3][vpr_gui][renderer]") {
    constexpr color c;
    CHECK(c.red == 0);
    CHECK(c.green == 0);
    CHECK(c.blue == 0);
    CHECK(c.alpha == 255);
}

TEST_CASE("color: 4-arg constructor stores RGBA exactly",
          "[layer3][vpr_gui][renderer]") {
    constexpr color c(10, 20, 30, 40);
    CHECK(c.red == 10);
    CHECK(c.green == 20);
    CHECK(c.blue == 30);
    CHECK(c.alpha == 40);
}

TEST_CASE("color: 3-arg constructor defaults alpha to 255",
          "[layer3][vpr_gui][renderer]") {
    constexpr color c(255, 128, 0);
    CHECK(c.red == 255);
    CHECK(c.green == 128);
    CHECK(c.blue == 0);
    CHECK(c.alpha == 255);
}

TEST_CASE("color: equality and inequality operators",
          "[layer3][vpr_gui][renderer]") {
    color a(1, 2, 3, 4);
    color b(1, 2, 3, 4);
    color c(1, 2, 3, 5); // different alpha
    color d(0, 2, 3, 4); // different red

    CHECK(a == b);
    CHECK(a != c);
    CHECK(a != d);
    CHECK_FALSE(a != b);
    CHECK_FALSE(a == c);
}

// ---------------------------------------------------------------------------
// point2d
// ---------------------------------------------------------------------------

TEST_CASE("point2d: default constructor produces origin",
          "[layer3][vpr_gui][renderer]") {
    point2d p;
    CHECK(p.x == 0.0);
    CHECK(p.y == 0.0);
}

TEST_CASE("point2d: 2-arg constructor stores coordinates",
          "[layer3][vpr_gui][renderer]") {
    point2d p(3.5, -2.25);
    CHECK(p.x == 3.5);
    CHECK(p.y == -2.25);
}

TEST_CASE("point2d: equality reflects coordinates exactly",
          "[layer3][vpr_gui][renderer]") {
    point2d a(1.0, 2.0);
    point2d b(1.0, 2.0);
    point2d c(1.0, 2.5);
    CHECK(a == b);
    CHECK(a != c);
}

TEST_CASE("point2d: addition and subtraction",
          "[layer3][vpr_gui][renderer]") {
    point2d a(1.0, 2.0);
    point2d b(3.0, 4.0);
    point2d sum = a + b;
    point2d diff = b - a;
    CHECK(sum == point2d(4.0, 6.0));
    CHECK(diff == point2d(2.0, 2.0));
}

TEST_CASE("point2d: compound assignment += accumulates",
          "[layer3][vpr_gui][renderer]") {
    point2d a(1.0, 1.0);
    a += point2d(2.0, 3.0);
    CHECK(a == point2d(3.0, 4.0));
}

// ---------------------------------------------------------------------------
// rectangle
// ---------------------------------------------------------------------------

TEST_CASE("rectangle: default constructor is zero-area at origin",
          "[layer3][vpr_gui][renderer]") {
    rectangle r;
    CHECK(r.left() == 0.0);
    CHECK(r.right() == 0.0);
    CHECK(r.top() == 0.0);
    CHECK(r.bottom() == 0.0);
    CHECK(r.width() == 0.0);
    CHECK(r.height() == 0.0);
    CHECK(r.area() == 0.0);
}

TEST_CASE("rectangle: 2-point constructor handles ordered corners",
          "[layer3][vpr_gui][renderer]") {
    rectangle r({1.0, 2.0}, {5.0, 8.0});
    CHECK(r.left() == 1.0);
    CHECK(r.right() == 5.0);
    CHECK(r.bottom() == 2.0);
    CHECK(r.top() == 8.0);
    CHECK(r.width() == 4.0);
    CHECK(r.height() == 6.0);
    CHECK(r.area() == 24.0);
}

TEST_CASE("rectangle: 2-point constructor normalises reversed corners",
          "[layer3][vpr_gui][renderer]") {
    // top-right specified first — left/right/top/bottom must still be the
    // *bounding* min/max, not the raw corner values.
    rectangle r({5.0, 8.0}, {1.0, 2.0});
    CHECK(r.left() == 1.0);
    CHECK(r.right() == 5.0);
    CHECK(r.bottom() == 2.0);
    CHECK(r.top() == 8.0);
    CHECK(r.width() == 4.0);
    CHECK(r.height() == 6.0);
}

TEST_CASE("rectangle: width/height constructor",
          "[layer3][vpr_gui][renderer]") {
    rectangle r({0.0, 0.0}, /*width=*/10.0, /*height=*/5.0);
    CHECK(r.width() == 10.0);
    CHECK(r.height() == 5.0);
    CHECK(r.area() == 50.0);
    CHECK(r.bottom_left() == point2d(0.0, 0.0));
    CHECK(r.top_right() == point2d(10.0, 5.0));
}

TEST_CASE("rectangle: corner accessors are consistent",
          "[layer3][vpr_gui][renderer]") {
    rectangle r({1.0, 2.0}, {5.0, 8.0});
    CHECK(r.bottom_left() == point2d(1.0, 2.0));
    CHECK(r.bottom_right() == point2d(5.0, 2.0));
    CHECK(r.top_left() == point2d(1.0, 8.0));
    CHECK(r.top_right() == point2d(5.0, 8.0));
}

TEST_CASE("rectangle: center_x / center_y / center",
          "[layer3][vpr_gui][renderer]") {
    rectangle r({0.0, 0.0}, {10.0, 20.0});
    CHECK(r.center_x() == 5.0);
    CHECK(r.center_y() == 10.0);
    CHECK(r.center() == point2d(5.0, 10.0));
}

TEST_CASE("rectangle: contains() tests boundary and interior",
          "[layer3][vpr_gui][renderer]") {
    rectangle r({0.0, 0.0}, {10.0, 10.0});
    CHECK(r.contains(5.0, 5.0));
    CHECK(r.contains(0.0, 0.0));   // bottom-left included
    CHECK(r.contains(10.0, 10.0)); // top-right included
    CHECK_FALSE(r.contains(-0.1, 5.0));
    CHECK_FALSE(r.contains(5.0, 10.1));
    CHECK(r.contains(point2d(5.0, 5.0)));
    CHECK_FALSE(r.contains(point2d(20.0, 5.0)));
}

TEST_CASE("rectangle: equality and inequality",
          "[layer3][vpr_gui][renderer]") {
    rectangle a({1.0, 2.0}, {5.0, 8.0});
    rectangle b({1.0, 2.0}, {5.0, 8.0});
    rectangle c({1.0, 2.0}, {5.0, 9.0});
    CHECK(a == b);
    CHECK(a != c);
}
