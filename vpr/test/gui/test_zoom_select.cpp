/**
 * @file test_zoom_select.cpp
 * @brief GUI-T-013 — Canvas zoom-select drag (rubber-band).
 *
 * Locks the contract of the two-click zoom-select state machine in
 * ``vpr/src/draw/draw.cpp``:
 *
 *   * ``toggle_window_mode()`` arms zoom-select (sets ``window_mode``).
 *   * ``act_on_mouse_press()`` left-button:
 *       (a) first click while ``window_mode`` collects ``point_1``,
 *           seeds ``window_preview_cursor`` to the same point, sets
 *           ``window_point_1_collected = true``;
 *       (b) second click computes the new camera world rectangle
 *           preserving the canvas aspect ratio, calls
 *           ``canvas::get_camera().set_world(new_world)``, then resets
 *           ``window_mode`` and ``window_point_1_collected`` to false.
 *   * ``act_on_mouse_move()`` while ``window_point_1_collected``
 *     updates ``window_preview_cursor`` (used to render the dashed
 *     preview rectangle) without touching the anchor.
 *   * Non-left-button presses must not advance the state machine.
 *   * ``zoom_fit(canvas, region)`` restores ``camera.get_world() ==
 *     region``.
 *
 * Why this is real, behaviour-relevant coverage rather than structural
 * noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The zoom-select state machine is process-global state spread across
 *   four file-scope variables in draw.cpp. Production has no
 *   intermediate hook points — every regression here either
 *   (a) breaks the user's first attempt at ZoomOnSelection (silent — the
 *       second click does nothing) or
 *   (b) computes a wrong new_world (wrong aspect or wrong corner — the
 *       camera leaps to a confusing rectangle).
 *   Both classes are silent in production telemetry and must be locked
 *   here. We exercise the *real* dispatch path (act_on_mouse_press
 *   from libvpr), not a mock, with a real ezgl::canvas registered on
 *   the singleton application so the second-click branch runs end-to-
 *   end including ``set_world``.
 *
 * Fixture composition:
 *   * ``EzglAppFixture`` provides the ezgl::application singleton + a
 *     loaded main.ui (so ``app->update_message`` and ``find_widget``
 *     calls inside the production handlers behave normally).
 *   * The local ``ZoomSelectFixture`` registers a ``MainCanvas`` on
 *     the application (one-shot per process — ezgl::application's
 *     m_canvases is a permanent map) and snapshots / restores the
 *     four file-scope state globals around each TEST_CASE so case
 *     ordering cannot leak state.
 *
 * Tag: [layer4][zoom-select][vpr_gui][GUI-T-013]
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include <QMouseEvent>
#include <QPointF>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/control.hpp>
#include <ezgl/graphics.hpp>
#include <ezgl/rectangle.hpp>
#include <ezgl/point.hpp>

#include "draw_global.h"
#include "draw_types.h"
#include "test_ezgl_app_fixture.hpp"
#include "vpr_types.h" // e_pic_type

// File-local globals in vpr/src/draw/draw.cpp — declared without `static`
// so they have external linkage and are addressable from this TU through
// libvpr. This is the established pattern (see test_mouse_events.cpp).
extern bool window_mode;
extern bool window_point_1_collected;
extern ezgl::point2d point_1;
extern ezgl::point2d window_preview_cursor;

// Production handlers; same external-linkage pattern.
extern void act_on_mouse_press(ezgl::application* app, QMouseEvent* event, double x, double y);
extern void act_on_mouse_move(ezgl::application* app, QMouseEvent* event, double x, double y);
extern void toggle_window_mode(QWidget* widget, ezgl::application* app);

using vpr_gui_test::EzglAppFixture;

namespace {

constexpr double kTol = 1e-9;

bool point_eq(const ezgl::point2d& a, const ezgl::point2d& b, double tol = kTol) {
    return std::abs(a.x - b.x) <= tol && std::abs(a.y - b.y) <= tol;
}

bool rect_eq(const ezgl::rectangle& a, const ezgl::rectangle& b, double tol = kTol) {
    return std::abs(a.left() - b.left()) <= tol
           && std::abs(a.right() - b.right()) <= tol
           && std::abs(a.top() - b.top()) <= tol
           && std::abs(a.bottom() - b.bottom()) <= tol;
}

/// RAII guard that snapshots all four zoom-select file-scope globals
/// at construction and restores them at destruction. Required because
/// the state is process-wide and Catch2 cases run sequentially in a
/// single process — without the guard a failed case mid-flight would
/// leak ``window_mode == true`` into the next case.
class ZoomSelectStateGuard {
  public:
    ZoomSelectStateGuard()
        : saved_mode_(window_mode)
        , saved_collected_(window_point_1_collected)
        , saved_p1_(point_1)
        , saved_cursor_(window_preview_cursor) {
        // Start from a known-clean baseline regardless of how the
        // previous case left things.
        window_mode = false;
        window_point_1_collected = false;
        point_1 = {0.0, 0.0};
        window_preview_cursor = {0.0, 0.0};
    }
    ~ZoomSelectStateGuard() {
        window_mode = saved_mode_;
        window_point_1_collected = saved_collected_;
        point_1 = saved_p1_;
        window_preview_cursor = saved_cursor_;
    }
    ZoomSelectStateGuard(const ZoomSelectStateGuard&) = delete;
    ZoomSelectStateGuard& operator=(const ZoomSelectStateGuard&) = delete;

  private:
    bool saved_mode_;
    bool saved_collected_;
    ezgl::point2d saved_p1_;
    ezgl::point2d saved_cursor_;
};

/// Ensures a canvas with the application's main canvas id ("MainCanvas")
/// is registered. Idempotent: ezgl::application::add_canvas warns and
/// no-ops on duplicate keys, and our canvas pointer remains valid for
/// the lifetime of the application (which is the test process). Resets
/// the camera world to a known rectangle so each case starts from the
/// same numerical baseline.
ezgl::canvas* ensure_main_canvas(ezgl::application* app,
                                 const ezgl::rectangle& initial_world) {
    const std::string id = app->get_main_canvas_id();
    ezgl::canvas* cnv = app->get_canvas(id);
    if (!cnv) {
        // No-op lambda rather than nullptr: the canvas map is process-global,
        // so a null draw_callback persists into later tests (e.g.
        // save-graphics) whose canvas::render_to_image calls the callback
        // unconditionally and segfaults on the null call.
        cnv = app->add_canvas(id, /*draw_callback=*/[](ezgl::renderer*) {}, initial_world, ezgl::WHITE);
    }
    cnv->get_camera().set_world(initial_world);
    return cnv;
}

QMouseEvent make_press(Qt::MouseButton button) {
    return QMouseEvent(QEvent::MouseButtonPress,
                       QPointF(0, 0), QPointF(0, 0),
                       button, button,
                       Qt::NoModifier);
}

} // namespace

// =============================================================================
// State-machine micro-tests (no canvas dispatch required)
// =============================================================================

TEST_CASE("ZoomSelect: toggle_window_mode arms zoom-select",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;

    REQUIRE_FALSE(window_mode);
    toggle_window_mode(/*widget=*/nullptr, fx.app());
    REQUIRE(window_mode);
    REQUIRE_FALSE(window_point_1_collected); // toggling does not collect a point
}

TEST_CASE("ZoomSelect: first left-press collects anchor and seeds preview",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;
    ensure_main_canvas(fx.app(), ezgl::rectangle({0, 0}, 100, 100));

    window_mode = true;
    QMouseEvent left = make_press(Qt::LeftButton);

    act_on_mouse_press(fx.app(), &left, 10.0, 20.0);

    CHECK(window_point_1_collected);
    CHECK(point_eq(point_1, {10.0, 20.0}));
    // Production seeds window_preview_cursor at the anchor so the dashed
    // preview rectangle has zero area until the next mouse-move arrives
    // (otherwise a stale cursor from a previous zoom-select operation
    // would briefly produce a wrong-sized rectangle).
    CHECK(point_eq(window_preview_cursor, {10.0, 20.0}));
    // First click leaves window_mode armed — only the second click
    // disarms it.
    CHECK(window_mode);
}

TEST_CASE("ZoomSelect: mouse-move during preview updates only the cursor",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;
    ensure_main_canvas(fx.app(), ezgl::rectangle({0, 0}, 100, 100));

    window_mode = true;
    QMouseEvent left = make_press(Qt::LeftButton);
    act_on_mouse_press(fx.app(), &left, 10.0, 20.0);
    REQUIRE(window_point_1_collected);
    REQUIRE(point_eq(point_1, {10.0, 20.0}));

    act_on_mouse_move(fx.app(), /*event=*/nullptr, 50.0, 60.0);

    CHECK(window_point_1_collected);                      // unchanged
    CHECK(point_eq(point_1, {10.0, 20.0}));               // anchor preserved
    CHECK(point_eq(window_preview_cursor, {50.0, 60.0})); // cursor advanced
}

TEST_CASE("ZoomSelect: non-left-button press does not advance state",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;
    ensure_main_canvas(fx.app(), ezgl::rectangle({0, 0}, 100, 100));

    window_mode = true;
    QMouseEvent right = make_press(Qt::RightButton);
    QMouseEvent middle = make_press(Qt::MiddleButton);

    act_on_mouse_press(fx.app(), &right, 77.0, 88.0);
    act_on_mouse_press(fx.app(), &middle, 99.0, 11.0);

    CHECK_FALSE(window_point_1_collected);
    CHECK(point_eq(point_1, {0.0, 0.0}));
    CHECK(point_eq(window_preview_cursor, {0.0, 0.0}));
    CHECK(window_mode); // still armed; the user simply hasn't anchored yet
}

// =============================================================================
// Second-click commit — exercises the canvas dispatch path end-to-end
// =============================================================================

TEST_CASE("ZoomSelect: second click commits new camera world (square aspect)",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;

    // 100×100 starting world: window_ratio = 1.
    const ezgl::rectangle initial({0, 0}, 100.0, 100.0);
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);
    REQUIRE(rect_eq(cnv->get_camera().get_world(), initial));

    window_mode = true;
    QMouseEvent left = make_press(Qt::LeftButton);

    act_on_mouse_press(fx.app(), &left, 10.0, 10.0);
    REQUIRE(window_point_1_collected);

    act_on_mouse_press(fx.app(), &left, 40.0, 60.0);

    // Production formula:
    //   window_ratio = current.height/current.width = 1
    //   new_height   = |p1.y - p2.y| = 50
    //   new_width    = new_height / window_ratio = 50
    //   new_world    = {p1, {p1.x + new_width, p2.y}}
    //                = {(10,10), (10+50, 60)} = {(10,10), (60,60)}
    const ezgl::rectangle expected({10.0, 10.0}, {60.0, 60.0});
    CHECK(rect_eq(cnv->get_camera().get_world(), expected));

    // Flags must reset so a subsequent zoom-select must be re-armed by
    // toggle_window_mode (the user has to click the ZoomOnSelection
    // button again — see draw.cpp::act_on_mouse_press second branch).
    CHECK_FALSE(window_mode);
    CHECK_FALSE(window_point_1_collected);
}

TEST_CASE("ZoomSelect: second click preserves canvas aspect ratio (tall window)",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;

    // 100×200 starting world: window_ratio = 200/100 = 2.0. Selecting
    // a tall sliver should yield a new_window narrower than the user
    // dragged so aspect is preserved.
    const ezgl::rectangle initial({0, 0}, 100.0, 200.0);
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    window_mode = true;
    QMouseEvent left = make_press(Qt::LeftButton);
    act_on_mouse_press(fx.app(), &left, 10.0, 10.0);
    act_on_mouse_press(fx.app(), &left, 60.0, 90.0);

    // window_ratio = 2 ; new_height = 80 ; new_width = 80/2 = 40
    // new_world = {(10,10), (10+40, 90)} = {(10,10), (50,90)}
    const ezgl::rectangle expected({10.0, 10.0}, {50.0, 90.0});
    CHECK(rect_eq(cnv->get_camera().get_world(), expected));
    // Sanity: width/height ratio of new_world matches initial.
    const auto w = cnv->get_camera().get_world();
    CHECK(std::abs((w.height() / w.width())
                   - (initial.height() / initial.width()))
          <= 1e-9);
}

// =============================================================================
// Recovery — zoom_fit can always restore a known world
// =============================================================================

TEST_CASE("ZoomSelect: zoom_fit restores a known world rectangle",
          "[layer4][zoom-select][vpr_gui][GUI-T-013]") {
    EzglAppFixture fx;
    ZoomSelectStateGuard guard;

    const ezgl::rectangle initial({0, 0}, 100.0, 100.0);
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    // Drive the camera to a non-initial world via a zoom-select drag,
    // then assert zoom_fit recovers any caller-supplied region.
    window_mode = true;
    QMouseEvent left = make_press(Qt::LeftButton);
    act_on_mouse_press(fx.app(), &left, 10.0, 10.0);
    act_on_mouse_press(fx.app(), &left, 40.0, 60.0);
    REQUIRE_FALSE(rect_eq(cnv->get_camera().get_world(), initial));

    const ezgl::rectangle target({-5.0, -5.0}, 50.0, 50.0);
    ezgl::zoom_fit(cnv, target);
    CHECK(rect_eq(cnv->get_camera().get_world(), target));

    ezgl::zoom_fit(cnv, initial);
    CHECK(rect_eq(cnv->get_camera().get_world(), initial));
}
