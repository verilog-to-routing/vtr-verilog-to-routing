/**
 * @file test_ezgl_app_fixture.cpp
 * @brief See test_ezgl_app_fixture.hpp for the architectural rationale.
 */

#include "test_ezgl_app_fixture.hpp"

#include <QMainWindow>
#include <stdexcept>

#include <ezgl/application.hpp>
#include <ezgl/main_window.hpp>

#include "draw_global.h"
#include "draw_types.h"
#include "test_app_singleton.hpp"

namespace vpr_gui_test {

// Full t_draw_state copy. The struct is non-trivially-copyable but
// has a working default copy ctor / assignment, so a snapshot is one
// member-wise copy. We hide it behind a struct here so that the
// header does not need to include draw_types.h.
struct EzglAppFixture::DrawStateSnapshot {
    t_draw_state state;
};

EzglAppFixture::EzglAppFixture() {
    app_ = vpr_gui_test::test_app();
    if (!app_) {
        throw std::logic_error(
            "EzglAppFixture: vpr_gui_test::test_app() returned null. "
            "test_gui_main.cpp must construct an ezgl::application before "
            "Catch2 enters any TEST_CASE body.");
    }

    ezgl::MainWindow mw; // loads ":/ezgl/main.ui" by default
    main_window_.reset(mw.release());
    if (!main_window_) {
        throw std::runtime_error(
            "EzglAppFixture: ezgl::MainWindow failed to load main.ui — "
            "the compiled .qrc resource may be missing.");
    }

    snapshot_draw_state();
}

EzglAppFixture::~EzglAppFixture() {
    try {
        restore_draw_state();
    } catch (...) {
    }
    main_window_.reset();
}

void EzglAppFixture::snapshot_draw_state() {
    snapshot_ = std::make_unique<DrawStateSnapshot>();
    snapshot_->state = *get_draw_state_vars();
}

void EzglAppFixture::restore_draw_state() {
    if (snapshot_) {
        *get_draw_state_vars() = snapshot_->state;
    }
}

} // namespace vpr_gui_test
