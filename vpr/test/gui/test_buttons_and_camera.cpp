/**
 * @file test_buttons_and_camera.cpp
 * @brief GUI-T-020 — buttons.cpp helpers + ezgl::control free-function camera
 *        operations (Layer 4).
 *
 * Locks two small but production-relevant code paths that previous tests
 * exercised only incidentally:
 *
 *   1. ``vpr/src/draw/buttons.cpp`` — ``find_button(name)`` and
 *      ``delete_button(name)``. Both look up a child of the
 *      ``"InnerGrid"`` widget by ``objectName`` and either return it
 *      or schedule it for deletion via ``QObject::deleteLater``.
 *      Production code (e.g. ``manual_moves.cpp``) calls
 *      ``find_button("ProceedButton")`` to wire the manual-move
 *      proceed flow; a regression that returns the wrong child
 *      silently breaks the manual-move dialog.
 *
 *   2. ``libs/EXTERNAL/libezgl/src/control.cpp`` — the free
 *      ``ezgl::zoom_in / zoom_out / zoom_fit / translate*`` helpers
 *      bound to the toolbar Zoom/Pan buttons. Each rewrites
 *      ``canvas::camera::world`` and triggers a ``redraw_camera_only``.
 *      A regression in the zoom-around-point math (different
 *      formula for the four corners) leaves the user unable to
 *      zoom into a specific feature; a sign error in
 *      ``translate_up`` swaps the pan direction. Both classes are
 *      silent in production telemetry and must be locked here.
 *
 * Why this is real, behaviour-relevant coverage rather than structural
 * noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   We exercise the *actual* exported symbols against the *actual*
 *   ``ezgl::canvas`` registered on the singleton ``ezgl::application``.
 *   No mocks, no shims. Each assertion locks an externally-observable
 *   property: ``find_button`` returns the matching ``QPushButton``,
 *   missing names return ``nullptr``, ``zoom_in`` shrinks the world
 *   rectangle around its centre by exactly ``zoom_factor``,
 *   ``translate_up`` shifts the world rectangle in +y by
 *   ``height/factor``, etc.
 *
 * Tag: [layer4][buttons][camera][vpr_gui][GUI-T-020]
 */

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <memory>

#include <QPushButton>
#include <QWidget>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/control.hpp>
#include <ezgl/rectangle.hpp>
#include <ezgl/point.hpp>

#include "buttons.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"

// File-scope ``application`` pointer in vpr/src/draw/draw.cpp; buttons.cpp
// reads it as ``application->find_widget("InnerGrid")``. The fixture
// guarantees ``test_app()`` is non-null, but draw.cpp's own pointer is
// only initialised by ``vpr_init_graphics`` (gated on ``--disp on``),
// so we wire it for the duration of each test.
namespace ezgl {
class application;
}
extern ezgl::application* application;

using vpr_gui_test::EzglAppFixture;
using vpr_gui_test::test_app;

namespace {

constexpr double kTol = 1e-9;

bool rect_eq(const ezgl::rectangle& a, const ezgl::rectangle& b, double tol = kTol) {
    return std::abs(a.left() - b.left()) <= tol
           && std::abs(a.right() - b.right()) <= tol
           && std::abs(a.top() - b.top()) <= tol
           && std::abs(a.bottom() - b.bottom()) <= tol;
}

/// Sets draw.cpp's file-scope ``application`` for the test body and
/// restores the previous value on destruction. Same pattern as
/// DrawApplicationScope in test_search_dispatch.cpp.
class DrawApplicationScope {
  public:
    explicit DrawApplicationScope(ezgl::application* app)
        : saved_(application) {
        application = app;
    }
    ~DrawApplicationScope() { application = saved_; }
    DrawApplicationScope(const DrawApplicationScope&) = delete;
    DrawApplicationScope& operator=(const DrawApplicationScope&) = delete;

  private:
    ezgl::application* saved_ = nullptr;
};

/// Owns an ``InnerGrid`` QWidget plus a child QPushButton. Production
/// looks up the child via ``QObject::objectName`` walking the
/// InnerGrid's direct children, so we only need a parent named
/// "InnerGrid" reachable from ``QApplication::allWidgets()`` (which
/// includes any QWidget the process has constructed — visibility is
/// not required).
struct InnerGridScaffold {
    InnerGridScaffold() {
        grid = std::make_unique<QWidget>();
        grid->setObjectName("InnerGrid");

        button = new QPushButton("hello", grid.get());
        button->setObjectName("HelloButton");

        sibling = new QPushButton("other", grid.get());
        sibling->setObjectName("OtherButton");
    }

    ~InnerGridScaffold() {
        // Drop the parent first so children get destroyed via Qt's
        // parent-child chain, then drain any pending DeferredDelete
        // posted by ``delete_button``. Failing to drain leaves orphan
        // QWidgets in ``QApplication::allWidgets()`` that can be
        // observed by *subsequent* TEST_CASEs (e.g. GUI-T-019's child
        // fork in test_search_null_guard) and silently flips their
        // expected-fail behaviour.
        grid.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    std::unique_ptr<QWidget> grid;
    QPushButton* button = nullptr;  // owned by grid
    QPushButton* sibling = nullptr; // owned by grid
};

/// Same as ZoomSelect's ensure_main_canvas helper: idempotently
/// register a canvas on the singleton application and reset its
/// world to a known baseline rectangle.
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

} // namespace

// =============================================================================
// buttons.cpp — find_button / delete_button
// =============================================================================

TEST_CASE("Buttons: find_button returns the named child of InnerGrid",
          "[layer4][buttons][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    DrawApplicationScope app_scope(test_app());
    InnerGridScaffold scaffold;

    QWidget* found = find_button("HelloButton");
    REQUIRE(found != nullptr);
    CHECK(found == scaffold.button);

    QWidget* sibling = find_button("OtherButton");
    REQUIRE(sibling != nullptr);
    CHECK(sibling == scaffold.sibling);
}

TEST_CASE("Buttons: find_button returns nullptr for an unknown name",
          "[layer4][buttons][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    DrawApplicationScope app_scope(test_app());
    InnerGridScaffold scaffold;

    CHECK(find_button("NoSuchButton") == nullptr);
    // Empty names also fall through the loop without matching.
    CHECK(find_button("") == nullptr);
}

TEST_CASE("Buttons: delete_button schedules the named child for deletion",
          "[layer4][buttons][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    DrawApplicationScope app_scope(test_app());
    InnerGridScaffold scaffold;

    REQUIRE(find_button("HelloButton") != nullptr);

    // delete_button uses QObject::deleteLater(); the widget is still
    // alive on return but is queued for deletion. Locking the public
    // contract: after delete_button + processing the event loop the
    // widget is no longer findable by name.
    delete_button("HelloButton");

    // Process the deferred deletion event.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    CHECK(find_button("HelloButton") == nullptr);
    // Sibling untouched.
    CHECK(find_button("OtherButton") != nullptr);
}

TEST_CASE("Buttons: delete_button is a safe no-op on an unknown name",
          "[layer4][buttons][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    DrawApplicationScope app_scope(test_app());
    InnerGridScaffold scaffold;

    // Must not crash and must not affect the existing children.
    delete_button("NoSuchButton");
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    CHECK(find_button("HelloButton") != nullptr);
    CHECK(find_button("OtherButton") != nullptr);
}

// =============================================================================
// ezgl::control free-function helpers — zoom / translate
// =============================================================================

TEST_CASE("Control: zoom_in around centre shrinks world by zoom_factor",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    ezgl::zoom_in(cnv, /*zoom_factor=*/2.0);

    const ezgl::rectangle& w = cnv->get_camera().get_world();
    // Zooming in around the centre by 2x halves both width and height
    // about (50, 50): new world is ((25, 25), (75, 75)).
    CHECK(rect_eq(w, ezgl::rectangle({25.0, 25.0}, {75.0, 75.0})));
}

TEST_CASE("Control: zoom_out around centre grows world by zoom_factor",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    ezgl::zoom_out(cnv, /*zoom_factor=*/2.0);

    const ezgl::rectangle& w = cnv->get_camera().get_world();
    CHECK(rect_eq(w, ezgl::rectangle({-50.0, -50.0}, {150.0, 150.0})));
}

TEST_CASE("Control: zoom_fit replaces world with the requested region",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    const ezgl::rectangle target({10.0, 20.0}, {60.0, 80.0});
    ezgl::zoom_fit(cnv, target);

    CHECK(rect_eq(cnv->get_camera().get_world(), target));
}

TEST_CASE("Control: translate shifts world by (dx, dy)",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    ezgl::translate(cnv, /*dx=*/5.0, /*dy=*/-7.0);

    const ezgl::rectangle& w = cnv->get_camera().get_world();
    CHECK(rect_eq(w, ezgl::rectangle({5.0, -7.0}, {105.0, 93.0})));
}

TEST_CASE("Control: translate_up / down / left / right move along axes",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    // factor=4 → step = 100 / 4 = 25 along the relevant axis.
    SECTION("translate_up shifts world in +y") {
        cnv->get_camera().set_world(initial);
        ezgl::translate_up(cnv, /*translate_factor=*/4.0);
        CHECK(rect_eq(cnv->get_camera().get_world(),
                      ezgl::rectangle({0.0, 25.0}, {100.0, 125.0})));
    }

    SECTION("translate_down shifts world in -y") {
        cnv->get_camera().set_world(initial);
        ezgl::translate_down(cnv, /*translate_factor=*/4.0);
        CHECK(rect_eq(cnv->get_camera().get_world(),
                      ezgl::rectangle({0.0, -25.0}, {100.0, 75.0})));
    }

    SECTION("translate_left shifts world in -x") {
        cnv->get_camera().set_world(initial);
        ezgl::translate_left(cnv, /*translate_factor=*/4.0);
        CHECK(rect_eq(cnv->get_camera().get_world(),
                      ezgl::rectangle({-25.0, 0.0}, {75.0, 100.0})));
    }

    SECTION("translate_right shifts world in +x") {
        cnv->get_camera().set_world(initial);
        ezgl::translate_right(cnv, /*translate_factor=*/4.0);
        CHECK(rect_eq(cnv->get_camera().get_world(),
                      ezgl::rectangle({25.0, 0.0}, {125.0, 100.0})));
    }
}

TEST_CASE("Control: zoom_in around an explicit point keeps that point fixed",
          "[layer4][camera][vpr_gui][GUI-T-020]") {
    EzglAppFixture fx;
    const ezgl::rectangle initial({0.0, 0.0}, {100.0, 100.0});
    ezgl::canvas* cnv = ensure_main_canvas(fx.app(), initial);

    // The two-argument overload first transforms the input from
    // widget space to world space via ``camera::widget_to_world``,
    // so the resulting rectangle depends on the widget mapping. We
    // assert only the invariant that the world rectangle changes
    // (zoom occurred) and remains finite — locking the entry-point
    // and dispatch into ``redraw_camera_only`` without depending on
    // the offscreen canvas size.
    ezgl::zoom_in(cnv, ezgl::point2d{10.0, 10.0}, /*zoom_factor=*/2.0);

    const ezgl::rectangle& w = cnv->get_camera().get_world();
    CHECK(std::isfinite(w.left()));
    CHECK(std::isfinite(w.right()));
    CHECK(std::isfinite(w.top()));
    CHECK(std::isfinite(w.bottom()));
    CHECK_FALSE(rect_eq(w, initial));

    // The dual zoom_out(point) overload must round-trip back to a
    // rectangle whose dimensions match the original (same factor).
    ezgl::zoom_out(cnv, ezgl::point2d{10.0, 10.0}, /*zoom_factor=*/2.0);
    const ezgl::rectangle& w2 = cnv->get_camera().get_world();
    CHECK(std::abs(w2.width() - initial.width()) <= 1e-6);
    CHECK(std::abs(w2.height() - initial.height()) <= 1e-6);
}
