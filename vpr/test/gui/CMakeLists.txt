# VPR GUI Tests — Qt-based GUI flow verification
#
# Layer 3: Catch2 unit/integration tests (test_vpr_gui binary)
# Layer 1: Headless smoke tests (bash script)
# Layer 5: Visual regression tests (bash + Python SSIM)

# Enable CTest for this directory.

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml Svg Test)

add_executable(test_vpr_gui
    test_gui_main.cpp
    test_widget_hierarchy.cpp
    test_gui_flow.cpp
    test_mouse_events.cpp
    test_draw_state.cpp
    test_renderer.cpp
    test_ui_contract.cpp        # GUI-T-018 — main.ui ↔ ui_setup.cpp contract
    test_search_null_guard.cpp  # GUI-T-019 — search subsystem null-guard symmetry
    test_runstage_fixture.cpp   # GUI-T-017 — VPR run-stage Catch2 fixture
    test_ezgl_app_fixture.cpp   # GUI-T-017 — Layer-4 ezgl::application + main.ui fixture
    test_stage_gating.cpp       # GUI-T-008 — stage-aware control gating
    test_zoom_select.cpp        # GUI-T-013 — canvas zoom-select drag (rubber-band)
    test_block_hit_test.cpp     # GUI-T-014 — block hit-test on canvas click
    test_search_dispatch.cpp    # GUI-T-001 — search dispatcher behaviour
    test_save_graphics.cpp      # GUI-T-003 — SaveGraphics dialog & writer
    test_rr_click_highlight.cpp # GUI-T-005 — RR click highlight + toggle
    test_keyboard_nav.cpp       # GUI-T-015 — keyboard navigation in toolbar
    test_breakpoint_debug.cpp   # GUI-T-002 — debugger / breakpoint subsystem
    test_manual_moves.cpp       # GUI-T-007 — manual moves form & cost summary
    test_visual_render_smoke.cpp # GUI-T-006 — Layer-5 visual render smoke (draw_rr/noc)
    test_buttons_and_camera.cpp # GUI-T-020 — buttons.cpp + ezgl::control camera helpers
    test_toggle_callbacks.cpp   # GUI-T-021 — draw_toggle_functions.cpp callback contracts
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vpr.qrc
)

# GUI-T-017 — compile-time bench / arch resolution. Hardcoded relative to
# the source tree so the fixture has no working-directory dependency
# (it chdirs into a per-fixture temp dir at construction time).
target_compile_definitions(test_vpr_gui PRIVATE
    VPR_TEST_ARCH_XML="${CMAKE_SOURCE_DIR}/vtr_flow/arch/timing/k6_N10_40nm.xml"
    VPR_TEST_BLIF_AND_LATCH="${CMAKE_SOURCE_DIR}/vtr_flow/benchmarks/microbenchmarks/and_latch.blif"
)

target_link_libraries(test_vpr_gui PRIVATE
    libvpr
    ezgl
    Catch2::Catch2
    Qt6::Test
    Qt6::Widgets
)

target_include_directories(test_vpr_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/draw
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
    LABELS "gui;unit;layer3"
)

# ---------------------------------------------------------------------------
# Layer 4 — Interactive (mouse/keyboard) tests
# Same Catch2 binary, filtered by tag so we get a separate ctest entry with a
# distinct label triple. S6 of vpr_gui_test_implementation_plan.
# ---------------------------------------------------------------------------
add_test(
    NAME test_vpr_gui_interactive
    COMMAND test_vpr_gui --colour-mode ansi "[layer4]"
)
set_tests_properties(test_vpr_gui_interactive PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "gui;interactive;layer4"
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

# ---------------------------------------------------------------------------
# Aggregate make targets (S2 of vpr_gui_test_implementation_plan)
#
# Label-based selection — every GUI test carries LABELS "gui;<kind>;<layerN>"
# so a future S6 layer-4 test joins the aggregate automatically.
# WORKING_DIRECTORY must be the top-level build dir so ctest sees the
# whole registry; CMAKE_CURRENT_BINARY_DIR would only see this subdir.
# ---------------------------------------------------------------------------
add_custom_target(gui-tests
    COMMENT "Running VPR Qt GUI test suite (all layers, label=gui)"
    COMMAND ${CMAKE_CTEST_COMMAND}
            -L gui --output-on-failure --no-tests=error
    DEPENDS vpr test_vpr_gui
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(gui-tests-unit
    COMMENT "Running VPR Qt GUI Layer 3 (Catch2) tests only"
    COMMAND ${CMAKE_CTEST_COMMAND} -L layer3 --output-on-failure
    DEPENDS test_vpr_gui
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(gui-tests-smoke
    COMMENT "Running VPR Qt GUI Layer 1 (headless smoke) tests only"
    COMMAND ${CMAKE_CTEST_COMMAND} -L layer1 --output-on-failure
    DEPENDS vpr
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(gui-tests-visual
    COMMENT "Running VPR Qt GUI Layer 5 (visual regression) tests only"
    COMMAND ${CMAKE_CTEST_COMMAND} -L layer5 --output-on-failure
    DEPENDS vpr
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(gui-tests-interactive
    COMMENT "Running VPR Qt GUI Layer 4 (interactive) tests only"
    COMMAND ${CMAKE_CTEST_COMMAND} -L layer4 --output-on-failure
    DEPENDS test_vpr_gui
    USES_TERMINAL
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ---------------------------------------------------------------------------
# S7 — GUI code-coverage report (measure only; no fail-under gate)
#
# Plan reference: doc/src/dev/vpr_gui_test_implementation_plan.rst §7.7
#
# Activated by `-DVTR_ENABLE_COVERAGE=on`, which the top-level CMakeLists.txt
# turns into `-g -ftest-coverage -fprofile-arcs` for the whole tree (see
# CMakeLists.txt:344-352). The target itself does NOT add per-file coverage
# flags — it relies on the global flag set so that libvpr, ezgl, and the
# test binary all emit .gcno/.gcda consistently.
#
# v1.1 of the plan deliberately omits any --fail-under-* flag here; that
# gate is added by S8 once the test corpus is rich enough to clear it.
# ---------------------------------------------------------------------------
if(VTR_ENABLE_COVERAGE)
    if(APPLE)
        # macOS clang emits LLVM-format coverage; gcovr needs the gcc-style
        # shim that `xcrun llvm-cov gcov` provides. Quote-protect the two-
        # word command so gcovr forwards it intact to subprocess.
        set(GCOV_EXECUTABLE "xcrun llvm-cov gcov")
    else()
        find_program(GCOV_EXECUTABLE gcov REQUIRED)
    endif()

    find_program(GCOVR_EXECUTABLE gcovr REQUIRED)

    # Wrapper script: runs the GUI ctest aggregate but always exits 0 so
    # the downstream gcovr step still runs. S7 is measurement-only — see
    # the comment block below the add_custom_target call.
    set(_gui_cov_wrapper "${CMAKE_CURRENT_BINARY_DIR}/run_gui_tests_for_coverage.sh")
    file(WRITE  "${_gui_cov_wrapper}" "#!/bin/sh\n")
    file(APPEND "${_gui_cov_wrapper}"
         "\"${CMAKE_CTEST_COMMAND}\" -L gui --output-on-failure\n")
    file(APPEND "${_gui_cov_wrapper}" "exit 0\n")
    execute_process(COMMAND chmod +x "${_gui_cov_wrapper}")

    add_custom_target(gui-coverage
        COMMENT "Generating GUI code coverage report (gcovr → coverage_gui.{html,xml,txt})"
        COMMAND sh "${_gui_cov_wrapper}"
        COMMAND ${GCOVR_EXECUTABLE} -r ${CMAKE_SOURCE_DIR}
                --gcov-executable "${GCOV_EXECUTABLE}"
                --html-details ${CMAKE_BINARY_DIR}/coverage_gui.html
                --xml ${CMAKE_BINARY_DIR}/coverage_gui.xml
                --txt ${CMAKE_BINARY_DIR}/coverage_gui.txt
                # S8 regression gate (see DEF-014 in the discovered-defects
                # log for the rationale: the plan §7.8 80%/70% target is
                # unreachable from Layer 3 alone on the current renderer
                # architecture, so we ship a regression-only gate set
                # ~5pp below the measured baseline of 40.85% / 27.23%).
                --fail-under-line 35
                --fail-under-branch 22
                -f ${CMAKE_SOURCE_DIR}/vpr/src/draw
                -f ${CMAKE_SOURCE_DIR}/vpr/src/base/read_options.cpp
                -f ${CMAKE_SOURCE_DIR}/libs/EXTERNAL/libezgl/src
                -e ".*moc_.*"
                -e ".*qrc_.*"
                -e ".*ui_.*"
                ${CMAKE_BINARY_DIR}
        DEPENDS test_vpr_gui vpr
        USES_TERMINAL
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
