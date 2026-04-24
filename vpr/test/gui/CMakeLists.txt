# VPR GUI Tests — Qt-based GUI flow verification
#
# Layer 3: Catch2 unit/integration tests (test_vpr_gui binary)
# Layer 1: Headless smoke tests (bash script)
# Layer 5: Visual regression tests (bash + Python SSIM)

# Enable CTest for this directory (top-level enable_testing() is disabled
# during QT_MIGRATION_TMP; this ensures our tests are still registered).
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Svg Test)

add_executable(test_vpr_gui
    test_gui_main.cpp
    test_qtgladeloader.cpp
    test_gui_flow.cpp
)

target_link_libraries(test_vpr_gui PRIVATE
    libvpr
    ezgl
    Catch2::Catch2
    Qt6::Test
)

target_include_directories(test_vpr_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/qt
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/draw
)

# Pass VPR main.ui path for QtGladeLoader tests
target_compile_definitions(test_vpr_gui PRIVATE
    VPR_MAIN_UI_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../main.ui"
)

set_target_properties(test_vpr_gui PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_test(
    NAME test_vpr_gui
    COMMAND test_vpr_gui --colour-mode ansi
)
set_tests_properties(test_vpr_gui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

# ---------------------------------------------------------------------------
# Layer 1 — Headless Smoke Tests
# ---------------------------------------------------------------------------
set(VPR_BINARY $<TARGET_FILE:vpr>)
set(ARCH_DIR "${CMAKE_SOURCE_DIR}/vtr_flow/arch/timing")
set(BENCH_DIR "${CMAKE_SOURCE_DIR}/vtr_flow/benchmarks/microbenchmarks")

add_test(
    NAME test_vpr_gui_smoke
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_headless_smoke.sh
            "${VPR_BINARY}" "${ARCH_DIR}" "${BENCH_DIR}"
)
set_tests_properties(test_vpr_gui_smoke PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1"
    TIMEOUT 300
    LABELS "gui;smoke;layer1"
)

# ---------------------------------------------------------------------------
# Layer 5 — Visual Regression Tests
# ---------------------------------------------------------------------------
add_test(
    NAME test_vpr_gui_visual
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_visual_regression.sh
            "${VPR_BINARY}" "${ARCH_DIR}" "${BENCH_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/golden" "0.999"
)
set_tests_properties(test_vpr_gui_visual PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1"
    TIMEOUT 600
    LABELS "gui;visual;layer5"
    SKIP_RETURN_CODE 2
)
