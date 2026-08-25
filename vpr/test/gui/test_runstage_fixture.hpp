/**
 * @file test_runstage_fixture.hpp
 * @brief GUI-T-017 — VPR run-stage Catch2 fixture.
 *
 * Closes the structural blocker recorded in
 * doc/src/dev/vpr_gui_complete_test_plan.rst §12.3:
 *
 *   "Wave 2 cannot ship behavioural assertions without a Catch2 fixture
 *    that runs VPR through pack/place/route and exposes ``g_vpr_ctx`` to
 *    the test process."
 *
 * Architecture:
 *
 *   * Approach C from the planning analysis: a single in-process VPR
 *     flow per test case, stopped at the requested ``Stage``. State is
 *     read directly from ``g_vpr_ctx`` while the fixture is alive; the
 *     destructor calls ``vpr_free_all`` to leave the singleton clean
 *     for the next case (Catch2 runs cases sequentially by default).
 *
 *   * Compile-time benchmark / arch resolution via
 *     ``VPR_TEST_ARCH_XML`` and ``VPR_TEST_BLIF_AND_LATCH`` (set in
 *     vpr/test/gui/CMakeLists.txt). No working-directory dependency.
 *
 *   * Working directory isolation: the constructor ``chdir``\ s into a
 *     freshly-created temp directory so VPR's stdout / log / output
 *     artefacts do not pollute the build tree. The destructor restores
 *     the previous cwd and removes the temp directory.
 *
 *   * Channel width is pinned to a known-routable value for
 *     ``and_latch.blif`` so ``vpr_route_flow`` always succeeds without
 *     entering min-W binary search (which adds 10x runtime).
 *
 * Per §4 of the test plan this header is *infrastructure* — sentinel
 * cases below assert that the fixture itself works (``run_to`` produces
 * a non-empty cluster netlist, etc.). Behavioural Wave-2 tickets
 * (T-001, T-008, T-013, T-014) ride this fixture in follow-up
 * commits and assert their own contracts.
 *
 * Tag: [layer3][fixture][vpr_gui][GUI-T-017]
 */

#pragma once

#include <filesystem>
#include <string>

// VPR headers (libvpr is linked into test_vpr_gui — see
// vpr/test/gui/CMakeLists.txt). Already on the include path because
// ``test_vpr_gui`` adds ``vpr/src/draw`` and inherits the libvpr
// transitive include set.
#include "vpr_api.h"
#include "vpr_types.h"

namespace vpr_gui_test {

// Stages at which the flow can be paused. Each subsequent stage runs
// every step of the previous stages.
enum class Stage {
    PostInit,  ///< after vpr_init only — options + arch + setup parsed
    PostPack,  ///< after vpr_pack_flow (clb_nlist populated)
    PostPlace, ///< after vpr_create_device + vpr_place_flow
    PostRoute, ///< after vpr_route_flow (full P&R, t_draw_coords valid)
};

/**
 * @brief Runs the VPR flow up to a chosen stage, exposes g_vpr_ctx.
 *
 * Owns the t_vpr_setup / t_arch / t_options state for one flow run.
 * Single-use: construct, call ``run_to(stage)`` exactly once, read
 * ``g_vpr_ctx`` from the test body, let the destructor tear down.
 *
 * Thread-safety: none. Catch2 sequential test execution + the
 * process-wide ``g_vpr_ctx`` singleton mean only one fixture instance
 * may be alive at any time. A second concurrent instance would
 * scribble over the first's cluster / placement / routing data.
 */
class VprRunStageFixture {
  public:
    VprRunStageFixture();
    ~VprRunStageFixture();

    // Non-copyable, non-movable: owns process-wide state.
    VprRunStageFixture(const VprRunStageFixture&) = delete;
    VprRunStageFixture& operator=(const VprRunStageFixture&) = delete;
    VprRunStageFixture(VprRunStageFixture&&) = delete;
    VprRunStageFixture& operator=(VprRunStageFixture&&) = delete;

    /// Run the flow up to and including ``stage``. Throws on flow
    /// failure (a flow failure is a real defect — Wave 2 tickets are
    /// not expected to drive failing flows).
    void run_to(Stage stage);

    /// Stage actually reached (set by run_to).
    Stage reached() const { return reached_; }

    /// Mutable access for the rare case where a test needs to tweak
    /// vpr_setup before running. Most cases should not touch this.
    t_vpr_setup& vpr_setup() { return vpr_setup_; }
    t_arch& arch() { return arch_; }

    /// Path to the per-fixture temp work directory (where VPR's output
    /// files land). Useful when a test needs to inspect a *.net / *.place /
    /// *.route artefact.
    const std::filesystem::path& work_dir() const { return work_dir_; }

  private:
    // Owned VPR state (mirrors the t_vpr_setup pattern in test_vpr.cpp).
    t_options options_{};
    t_vpr_setup vpr_setup_{};
    t_arch arch_{};

    Stage reached_ = Stage::PostInit;
    bool initialized_ = false;
    bool freed_ = false;

    std::filesystem::path prev_cwd_;
    std::filesystem::path work_dir_;

    /// Build the argv that mirrors a typical Layer-1 smoke command line
    /// for and_latch.blif on k6_N10_40nm.xml at a routable channel
    /// width. Returned vector owns the storage; the caller passes
    /// .data() to vpr_init.
    static std::vector<const char*> build_argv();

    void init_logging_once();
    void create_workdir_and_chdir();
    void teardown_workdir();
};

} // namespace vpr_gui_test
