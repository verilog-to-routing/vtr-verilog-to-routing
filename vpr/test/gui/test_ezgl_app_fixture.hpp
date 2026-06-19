/**
 * @file test_ezgl_app_fixture.hpp
 * @brief GUI-T-017 (Layer-4 substrate) — ezgl::application + main.ui fixture.
 *
 * Layer-4 tests in vpr/test/gui drive functions in vpr/src/draw that
 * take an ``ezgl::application*`` and look up named widgets via
 * ``app->find_widget(...)``, which iterates ``QApplication::allWidgets()``.
 * Two preconditions must therefore hold inside such a test body:
 *
 *   1. A single ``ezgl::application`` instance exists for the process
 *      (constructed in test_gui_main.cpp; see test_app_singleton.hpp).
 *   2. The widgets named in production code (``ToggleCritPathRouting``,
 *      ``ToggleNetType``, etc.) are loaded into the Qt widget tree.
 *
 * EzglAppFixture loads ``main.ui`` via ``ezgl::MainWindow`` in its
 * constructor, holds the resulting ``QMainWindow`` in a unique_ptr,
 * and tears it down in the destructor. Because Qt registers every
 * constructed QWidget with QApplication's global widget set,
 * ``find_widget`` succeeds on every name in the loaded tree for the
 * duration of the test case.
 *
 * The fixture also takes a snapshot of ``t_draw_state`` at
 * construction and restores it at destruction so behavioural tests
 * that mutate stage state cannot bleed into one another.
 *
 * Construction does NOT depend on ``g_vpr_ctx`` and does NOT run any
 * VPR flow stage. Tests that require ``g_vpr_ctx`` snapshots compose
 * EzglAppFixture with ``VprRunStageFixture`` (see
 * test_runstage_fixture.hpp).
 */
#pragma once

#include <memory>

#include <QMainWindow>

namespace ezgl {
class application;
}

namespace vpr_gui_test {

class EzglAppFixture {
  public:
    EzglAppFixture();
    ~EzglAppFixture();

    EzglAppFixture(const EzglAppFixture&) = delete;
    EzglAppFixture& operator=(const EzglAppFixture&) = delete;
    EzglAppFixture(EzglAppFixture&&) = delete;
    EzglAppFixture& operator=(EzglAppFixture&&) = delete;

    /// The process-global ezgl::application; never null inside the fixture.
    ezgl::application* app() const { return app_; }

    /// The loaded main.ui QMainWindow; widgets are findable via
    /// ``app()->find_widget(...)`` while this fixture is alive.
    QMainWindow* main_window() const { return main_window_.get(); }

  private:
    void snapshot_draw_state();
    void restore_draw_state();

    ezgl::application* app_ = nullptr;
    std::unique_ptr<QMainWindow> main_window_ = nullptr;

    // Opaque storage for the saved t_draw_state. Sized generously so we
    // do not include draw_types.h in this header (which would pull in the
    // entire vpr/src/draw transitive web).
    struct DrawStateSnapshot;
    std::unique_ptr<DrawStateSnapshot> snapshot_;
};

} // namespace vpr_gui_test
