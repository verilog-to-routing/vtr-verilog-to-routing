/**
 * @file test_app_singleton.hpp
 * @brief Process-global ezgl::application accessor for the GUI test binary.
 *
 * Qt only permits a single QApplication instance per process, and
 * ezgl::application IS-A QApplication. Layer-4 tests that drive
 * code paths in vpr/src/draw require an ezgl::application*; we
 * therefore construct one in test_gui_main.cpp (rather than a bare
 * QApplication) and expose it via this single accessor.
 *
 * Layer-3 tests that only need static QApplication helpers
 * (e.g. QApplication::allWidgets()) continue to work unchanged.
 */
#pragma once

#include <ezgl/application.hpp>

namespace vpr_gui_test {

/// Returns the process-wide ezgl::application instance constructed in main().
/// Never null inside a Catch2 TEST_CASE body.
ezgl::application* test_app();

} // namespace vpr_gui_test
