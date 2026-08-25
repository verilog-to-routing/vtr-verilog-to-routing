/**
 * @file test_gui_main.cpp
 * @brief Catch2 entry point for the VPR GUI test binary.
 *
 * Constructs a single ezgl::application (a QApplication subclass) so
 * that Layer-4 tests can drive vpr/src/draw APIs that require an
 * ezgl::application*. Qt forbids more than one QApplication per
 * process, so this is the canonical place to instantiate it.
 *
 * The application's UI XML is *not* loaded here. Tests that need a
 * loaded widget tree construct an ezgl::MainWindow directly (Layer 3)
 * or use the EzglAppFixture (Layer 4).
 */

#include <catch2/catch_session.hpp>
#include <ezgl/application.hpp>

#include "test_app_singleton.hpp"

namespace {
ezgl::application* g_test_app = nullptr;
}

namespace vpr_gui_test {
ezgl::application* test_app() { return g_test_app; }
} // namespace vpr_gui_test

int main(int argc, char* argv[]) {
    // Settings are required by the constructor but neither the UI
    // resource nor the canvas/window identifiers are consumed unless
    // application::run() is called. The test binary never calls run().
    ezgl::application::settings settings(
        /*m_resource=*/"/ezgl/main.ui",
        /*w_identifier=*/"MainWindow",
        /*c_identifier=*/"MainCanvas",
        /*a_identifier=*/"ezgl.app.test",
        /*s_callbacks=*/nullptr);

    ezgl::application app(settings, argc, argv);
    g_test_app = &app;

    int rc = Catch::Session().run(argc, argv);
    g_test_app = nullptr;
    return rc;
}
