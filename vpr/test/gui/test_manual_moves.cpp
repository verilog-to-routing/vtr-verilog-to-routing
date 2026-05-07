/**
 * @file test_manual_moves.cpp
 * @brief GUI-T-007 — Manual moves form & cost-summary contract (Layer 4).
 *
 * Locks the contract of ``vpr/src/draw/manual_moves.{h,cpp}``:
 *
 *   * ``ManualMovesInfo`` and ``ManualMovesState`` zero-initialise
 *     into the documented defaults — ``blockID = -1``,
 *     ``valid_input = true``, all window flags ``false``,
 *     ``placer_move_outcome == ABORTED``, etc. The placer reads
 *     these fields between user actions, so the defaults double as
 *     "no manual move pending".
 *   * ``string_is_a_number`` recognises numeric block IDs ("123")
 *     and rejects names ("clb_42"), letting ``calculate_cost_callback``
 *     decide between ``atoi`` and ``clb_nlist.find_block``.
 *   * ``close_manual_moves_window`` flips
 *     ``manual_moves_state.manual_move_window_is_open`` to false
 *     and is safe to call when the window was never opened (the
 *     production code wires it to ``QObject::destroyed`` which can
 *     fire from teardown paths).
 *   * ``manual_move_is_selected`` reads the ``manualMove`` checkbox
 *     from the loaded ``main.ui``. With the checkbox in its default
 *     (unchecked) state, the function returns false **and** writes
 *     false into ``manual_moves_state.manual_move_enabled``.
 *   * ``is_manual_move_legal`` rejects: invalid block IDs,
 *     out-of-grid (x, y) destinations, and the no-op case where
 *     the requested destination equals the block's current
 *     ``(x, y, sub_tile)``. Each rejection synchronously opens an
 *     ``invalid_breakpoint_entry_window`` (the EzglAppFixture
 *     guarantees the resulting top-level QWidgets are reaped).
 *
 * Why this is real, behaviour-relevant coverage rather than
 * structural noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The manual-moves form is the only GUI-driven write path into
 *   the placer. A bug here either pretends to move a block but
 *   silently drops the request (defaults wrong, validity check
 *   inverted) or proposes a swap that crashes the annealer
 *   (out-of-grid coordinates accepted). These tests pin each
 *   reject path against the real ``g_vpr_ctx`` produced by
 *   ``VprRunStageFixture::PostPlace`` so the grid bounds, block
 *   types, and current locations are the real placement.
 *
 * Setup:
 *
 *   * Pure-utility cases (defaults, ``string_is_a_number``,
 *     ``close_manual_moves_window``) take no fixture.
 *   * ``manual_move_is_selected`` requires ``EzglAppFixture`` so
 *     ``application->find_check_box("manualMove")`` succeeds.
 *   * ``is_manual_move_legal`` cases compose ``EzglAppFixture``
 *     (so the rejection-popup ``QWidget`` is reaped) plus
 *     ``VprRunStageFixture::run_to(PostPlace)`` (so
 *     ``g_vpr_ctx.placement().blk_loc_registry()`` is populated
 *     — both ``grid_blocks`` and ``block_locs`` are read inside
 *     ``is_manual_move_legal``).
 *
 * Note: this file deliberately does NOT exercise
 * ``manual_move_cost_summary_dialog`` or ``pl_do_manual_move``;
 * both call ``QDialog::exec()`` which spins a modal event loop
 * until the user clicks Accept / Reject. Driving that under the
 * offscreen Qt platform plugin would either hang or require a
 * staged ``QTimer`` injection that is much more brittle than the
 * contract it would lock.
 *
 * Tag: [layer4][interactive][manual-moves][vpr_gui][GUI-T-007]
 */

#include <catch2/catch_test_macros.hpp>

#include <string>

#include <QCheckBox>

#include <ezgl/application.hpp>

#include "draw_global.h"
#include "draw_types.h"
#include "manual_moves.h"
#include "move_utils.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"
#include "test_runstage_fixture.hpp"
#include "vpr_types.h"

using vpr_gui_test::EzglAppFixture;
using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

namespace {

// Save/restore manual_moves_state around every case so test order is
// irrelevant. The state lives on the process-static t_draw_state.
class ManualMovesStateGuard {
  public:
    ManualMovesStateGuard() {
        saved_ = get_draw_state_vars()->manual_moves_state;
    }
    ~ManualMovesStateGuard() {
        get_draw_state_vars()->manual_moves_state = saved_;
    }

  private:
    ManualMovesState saved_;
};

} // namespace

TEST_CASE("ManualMovesInfo zero-init defaults match the documented contract",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    ManualMovesInfo info;
    REQUIRE(info.blockID == -1);
    REQUIRE(info.x_pos == -1);
    REQUIRE(info.y_pos == -1);
    REQUIRE(info.subtile == 0);
    REQUIRE(info.layer == 0);
    REQUIRE(info.delta_cost == 0.0);
    REQUIRE(info.delta_timing == 0.0);
    REQUIRE(info.delta_bounding_box == 0.0);
    REQUIRE(info.valid_input);
    REQUIRE(info.placer_move_outcome == e_move_result::ABORTED);
    REQUIRE(info.user_move_outcome == e_move_result::ABORTED);
}

TEST_CASE("ManualMovesState zero-init defaults: every window flag is false",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    ManualMovesState state;
    REQUIRE_FALSE(state.manual_move_window_is_open);
    REQUIRE_FALSE(state.user_highlighted_block);
    REQUIRE_FALSE(state.manual_move_enabled);
    // Nested ManualMovesInfo defaults still hold.
    REQUIRE(state.manual_move_info.blockID == -1);
    REQUIRE(state.manual_move_info.valid_input);
}

TEST_CASE("string_is_a_number recognises pure-digit ids and rejects names",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    REQUIRE(string_is_a_number("0"));
    REQUIRE(string_is_a_number("42"));
    REQUIRE(string_is_a_number("123456789"));
    // Block names always contain non-digits.
    REQUIRE_FALSE(string_is_a_number("clb_42"));
    REQUIRE_FALSE(string_is_a_number("abc"));
    REQUIRE_FALSE(string_is_a_number("12a"));
    REQUIRE_FALSE(string_is_a_number("a12"));
    REQUIRE_FALSE(string_is_a_number(" 42")); // whitespace
    REQUIRE_FALSE(string_is_a_number("-1"));  // sign char
    // std::all_of of the empty range is vacuously true; this is what
    // the production callback sees if the user submits an empty
    // block-id field, so lock it down.
    REQUIRE(string_is_a_number(""));
}

TEST_CASE("close_manual_moves_window flips the open guard to false; idempotent",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    ManualMovesStateGuard guard;
    auto& state = get_draw_state_vars()->manual_moves_state;

    // Pretend a window is open (the production path that actually
    // assigns this is draw_manual_moves_window).
    state.manual_move_window_is_open = true;

    close_manual_moves_window();
    REQUIRE_FALSE(state.manual_move_window_is_open);

    // Calling again must remain safe (production wires this to
    // QObject::destroyed which may fire during teardown chains).
    REQUIRE_NOTHROW(close_manual_moves_window());
    REQUIRE_FALSE(state.manual_move_window_is_open);
}

TEST_CASE("manual_move_is_selected reads the manualMove checkbox and "
          "writes the result into manual_moves_state",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    EzglAppFixture app_fixture;
    ManualMovesStateGuard state_guard;

    auto& state = get_draw_state_vars()->manual_moves_state;
    // Prime the cached flag with the OPPOSITE of what we expect to
    // read — a true reader has to overwrite it.
    state.manual_move_enabled = true;

    QCheckBox* manual_moves =
        app_fixture.app()->find_check_box("manualMove");
    REQUIRE(manual_moves != nullptr);
    // The widget loaded from main.ui defaults to unchecked.
    REQUIRE_FALSE(manual_moves->isChecked());

    REQUIRE_FALSE(manual_move_is_selected());
    REQUIRE_FALSE(state.manual_move_enabled);

    // Flip the checkbox and re-read; the function must see the new
    // state and propagate it.
    manual_moves->setChecked(true);
    REQUIRE(manual_move_is_selected());
    REQUIRE(state.manual_move_enabled);

    // Reset for cleanliness (ManualMovesStateGuard restores
    // manual_moves_state but not the QCheckBox itself).
    manual_moves->setChecked(false);
}

TEST_CASE("is_manual_move_legal rejects an invalid block id (PostPlace)",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    EzglAppFixture app_fixture;
    VprRunStageFixture vpr_fixture;
    vpr_fixture.run_to(Stage::PostPlace);

    // Way past any real ClusterBlockId — clb_nlist.valid_block_id
    // returns false, the production helper opens an
    // invalid_breakpoint_entry_window and returns false.
    ClusterBlockId bad_block(999999);
    t_pl_loc to(1, 1, 0, 0);
    REQUIRE_FALSE(is_manual_move_legal(bad_block, to));
}

TEST_CASE("is_manual_move_legal rejects out-of-grid destinations (PostPlace)",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    EzglAppFixture app_fixture;
    VprRunStageFixture vpr_fixture;
    vpr_fixture.run_to(Stage::PostPlace);

    // Valid block id (block 0 always exists post-pack), invalid loc.
    ClusterBlockId block0(0);
    t_pl_loc to_neg(-1, 0, 0, 0);
    REQUIRE_FALSE(is_manual_move_legal(block0, to_neg));

    t_pl_loc to_huge(99999, 99999, 0, 0);
    REQUIRE_FALSE(is_manual_move_legal(block0, to_huge));
}

TEST_CASE("is_manual_move_legal rejects same-location no-op moves (PostPlace)",
          "[layer4][interactive][manual-moves][vpr_gui][GUI-T-007]") {
    EzglAppFixture app_fixture;
    VprRunStageFixture vpr_fixture;
    vpr_fixture.run_to(Stage::PostPlace);

    // Look up block 0's current location from the placement, then
    // ask is_manual_move_legal to "move" it there.
    t_draw_state* draw_state = get_draw_state_vars();
    const auto& block_locs =
        draw_state->get_graphics_blk_loc_registry_ref().block_locs();
    ClusterBlockId block0(0);
    t_pl_loc current = block_locs[block0].loc;

    // Even when the destination is on-grid and compatible, a no-op
    // move is rejected — same-location swaps are nonsensical for
    // the placer.
    REQUIRE_FALSE(is_manual_move_legal(block0, current));
}
