#pragma once
/**
 * @brief Which main UI form the GUI tests load.
 *
 * Defaults to the form VPR ships (see draw.cpp), so the tests exercise the
 * production configuration. Override from CMake to point a target at a
 * different form:
 *
 *   target_compile_definitions(<target> PRIVATE VPR_MAIN_UI_PATH="/ezgl/other.ui")
 */

#include <ezgl/main_window.hpp>

#include <QString>

#include <optional>

#ifndef VPR_MAIN_UI_PATH
#define VPR_MAIN_UI_PATH ":/ezgl/main.ui"
#endif

namespace vpr_gui_test {

/// Load the main UI form this build targets.
inline ezgl::MainWindow load_main_ui() {
    return ezgl::MainWindow(QString::fromLatin1(VPR_MAIN_UI_PATH), std::nullopt);
}

/// Load an arbitrary path. Used by the tests that exercise the loader's
/// failure paths.
inline ezgl::MainWindow load_ui(const QString& path) {
    return ezgl::MainWindow(path, std::nullopt);
}

} // namespace vpr_gui_test
