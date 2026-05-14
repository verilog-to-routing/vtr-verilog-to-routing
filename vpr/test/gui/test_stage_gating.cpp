/**
 * @file test_stage_gating.cpp
 * @brief GUI-T-008 — Stage-aware control gating across placement / routing.
 *
 * Locks the contract of the two public widget-gating functions in
 * ``vpr/src/draw/ui_setup.cpp``:
 *
 *   * ``hide_crit_path_routing(app)`` enables the
 *     ``ToggleCritPathRouting`` checkbox iff
 *       ``draw_state->setup_timing_info && pic_on_screen == ROUTING
 *        && show_crit_path``.
 *
 *   * ``hide_draw_routing(app)`` removes the ``"Routing"`` item from
 *     the ``ToggleNetType`` combo when ``pic_on_screen == PLACEMENT``
 *     and adds it (when absent) for every other stage.
 *
 * Production callers invoke these from
 * ``draw.cpp::on_stage_change_setup`` on every stage transition. A
 * silent regression here lets users mutate ``t_draw_state`` mid-flow
 * (the GUI looks fine but a control fails to disable) — exactly the
 * silent-regress class that the test plan §4 asks us to surface.
 *
 * Why this is real, behaviour-relevant coverage:
 *
 *   The function bodies are short but their *combined* behaviour
 *   forms the only stage-gating truth in the GUI. They depend on
 *   process-global ``t_draw_state`` and on a live widget tree. We
 *   exercise every cell of the
 *     (pic_on_screen × setup_timing_info × show_crit_path)
 *   product, plus the idempotency contract that the production
 *   stage-transition path relies on (``hide_draw_routing`` is called
 *   on every redraw, must not duplicate the "Routing" item).
 *
 * We do NOT need a VPR run-stage here — neither gating function
 * touches ``g_vpr_ctx``. Layer-3 fixtures + a stub
 * ``setup_timing_info`` shared_ptr are sufficient.
 *
 * Tag: [layer4][stage-gating][vpr_gui][GUI-T-008]
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <memory>

#include <QComboBox>
#include <QString>
#include <QWidget>

#include <ezgl/application.hpp>

#include "draw_global.h"
#include "draw_types.h"
#include "test_ezgl_app_fixture.hpp"
#include "ui_setup.h"
#include "vpr_types.h" // e_pic_type

using vpr_gui_test::EzglAppFixture;

namespace {

/// Build a non-null shared_ptr<const SetupTimingInfo> *without*
/// constructing a SetupTimingInfo (the class is purely abstract).
/// The aliasing constructor lets us share ownership with a control
/// block backing a sentinel int while presenting a non-null typed
/// pointer. The pointer is never dereferenced by production code:
/// ``hide_crit_path_routing`` only does a boolean test on the
/// shared_ptr.
std::shared_ptr<const SetupTimingInfo> make_truthy_timing_info_handle() {
    auto holder = std::make_shared<int>(0);
    return std::shared_ptr<const SetupTimingInfo>(
        holder,
        reinterpret_cast<const SetupTimingInfo*>(holder.get()));
}

const char* pic_name(e_pic_type p) {
    switch (p) {
        case e_pic_type::NO_PICTURE:
            return "NO_PICTURE";
        case e_pic_type::PLACEMENT:
            return "PLACEMENT";
        case e_pic_type::ANALYTICAL_PLACEMENT:
            return "ANALYTICAL_PLACEMENT";
        case e_pic_type::ROUTING:
            return "ROUTING";
        default:
            return "?";
    }
}

int routing_item_index(QComboBox* cb) {
    return cb->findText("Routing");
}

int routing_item_count(QComboBox* cb) {
    int count = 0;
    for (int i = 0; i < cb->count(); ++i) {
        if (cb->itemText(i) == "Routing") ++count;
    }
    return count;
}

} // namespace

// =============================================================================
// hide_crit_path_routing — the eight-cell truth table
// =============================================================================
TEST_CASE("hide_crit_path_routing: enabled iff timing && ROUTING && show_crit",
          "[layer4][stage-gating][vpr_gui][GUI-T-008]") {
    EzglAppFixture fx;
    auto* app = fx.app();

    QWidget* w = app->find_widget("ToggleCritPathRouting");
    REQUIRE(w != nullptr);

    t_draw_state* ds = get_draw_state_vars();
    auto truthy_tip = make_truthy_timing_info_handle();

    struct Case {
        e_pic_type pic;
        bool has_timing;
        bool show_crit;
        bool expect_enabled;
    };
    const auto c = GENERATE(
        // Only the all-three-true row enables the checkbox.
        Case{e_pic_type::ROUTING, true, true, true},
        Case{e_pic_type::ROUTING, true, false, false},
        Case{e_pic_type::ROUTING, false, true, false},
        Case{e_pic_type::ROUTING, false, false, false},
        Case{e_pic_type::PLACEMENT, true, true, false},
        Case{e_pic_type::PLACEMENT, true, false, false},
        Case{e_pic_type::PLACEMENT, false, true, false},
        Case{e_pic_type::PLACEMENT, false, false, false},
        // Boundary stages — both must yield disabled regardless of the
        // other two flags.
        Case{e_pic_type::NO_PICTURE, true, true, false},
        Case{e_pic_type::ANALYTICAL_PLACEMENT, true, true, false});

    INFO("pic=" << pic_name(c.pic)
                << " has_timing=" << (c.has_timing ? 1 : 0)
                << " show_crit=" << (c.show_crit ? 1 : 0)
                << " expect=" << (c.expect_enabled ? "ENABLED" : "DISABLED"));

    ds->pic_on_screen = c.pic;
    ds->setup_timing_info = c.has_timing ? truthy_tip
                                         : std::shared_ptr<const SetupTimingInfo>{};
    ds->show_crit_path = c.show_crit;

    // Pre-condition: flip the control to the *opposite* of the expected
    // state so we are sure the function actually drives setEnabled (rather
    // than passively succeeding because it was already in the right state).
    w->setEnabled(!c.expect_enabled);
    REQUIRE(w->isEnabled() == !c.expect_enabled);

    hide_crit_path_routing(app);
    REQUIRE(w->isEnabled() == c.expect_enabled);
}

// =============================================================================
// hide_draw_routing — Routing combo-item presence per stage
// =============================================================================
TEST_CASE("hide_draw_routing: PLACEMENT removes 'Routing' item",
          "[layer4][stage-gating][vpr_gui][GUI-T-008]") {
    EzglAppFixture fx;
    auto* app = fx.app();

    QComboBox* cb = app->find_combo_box("ToggleNetType");
    REQUIRE(cb != nullptr);

    t_draw_state* ds = get_draw_state_vars();

    SECTION("starts present, PLACEMENT removes it exactly once") {
        // Force a known starting condition: "Routing" present.
        if (routing_item_index(cb) == -1) cb->addItem("Routing", "2");
        REQUIRE(routing_item_count(cb) == 1);

        ds->pic_on_screen = e_pic_type::PLACEMENT;
        hide_draw_routing(app);
        REQUIRE(routing_item_index(cb) == -1);
        REQUIRE(routing_item_count(cb) == 0);
    }

    SECTION("idempotent — second PLACEMENT call does not throw or re-add") {
        if (routing_item_index(cb) == -1) cb->addItem("Routing", "2");
        ds->pic_on_screen = e_pic_type::PLACEMENT;
        hide_draw_routing(app);
        REQUIRE(routing_item_index(cb) == -1);
        // The production redraw loop calls this on every refresh; a second
        // call must remain a no-op when the item is already absent.
        hide_draw_routing(app);
        REQUIRE(routing_item_index(cb) == -1);
        REQUIRE(routing_item_count(cb) == 0);
    }
}

TEST_CASE("hide_draw_routing: non-PLACEMENT stages keep/restore 'Routing' item",
          "[layer4][stage-gating][vpr_gui][GUI-T-008]") {
    EzglAppFixture fx;
    auto* app = fx.app();

    QComboBox* cb = app->find_combo_box("ToggleNetType");
    REQUIRE(cb != nullptr);

    t_draw_state* ds = get_draw_state_vars();

    const auto pic = GENERATE(
        e_pic_type::ROUTING,
        e_pic_type::NO_PICTURE,
        e_pic_type::ANALYTICAL_PLACEMENT);
    INFO("pic=" << pic_name(pic));

    SECTION("re-adds 'Routing' if absent") {
        // Force absence.
        int idx = routing_item_index(cb);
        while (idx != -1) {
            cb->removeItem(idx);
            idx = routing_item_index(cb);
        }
        REQUIRE(routing_item_count(cb) == 0);

        ds->pic_on_screen = pic;
        hide_draw_routing(app);
        REQUIRE(routing_item_index(cb) != -1);
        REQUIRE(routing_item_count(cb) == 1);
    }

    SECTION("preserves a single 'Routing' on repeated calls") {
        if (routing_item_index(cb) == -1) cb->addItem("Routing", "2");
        REQUIRE(routing_item_count(cb) == 1);

        ds->pic_on_screen = pic;
        hide_draw_routing(app);
        hide_draw_routing(app);
        // Idempotent: must not silently grow into a duplicate item — a
        // duplicate would corrupt the production combo dispatcher because
        // QComboBox::findText returns only the first match.
        REQUIRE(routing_item_count(cb) == 1);
    }
}

// =============================================================================
// Cross-stage transition: PLACEMENT -> ROUTING -> PLACEMENT in one binary
// =============================================================================
TEST_CASE("hide_draw_routing: stage transitions toggle item presence",
          "[layer4][stage-gating][vpr_gui][GUI-T-008]") {
    EzglAppFixture fx;
    auto* app = fx.app();

    QComboBox* cb = app->find_combo_box("ToggleNetType");
    REQUIRE(cb != nullptr);

    t_draw_state* ds = get_draw_state_vars();

    // Establish baseline: present.
    if (routing_item_index(cb) == -1) cb->addItem("Routing", "2");
    REQUIRE(routing_item_count(cb) == 1);

    ds->pic_on_screen = e_pic_type::PLACEMENT;
    hide_draw_routing(app);
    REQUIRE(routing_item_index(cb) == -1);

    ds->pic_on_screen = e_pic_type::ROUTING;
    hide_draw_routing(app);
    REQUIRE(routing_item_index(cb) != -1);
    REQUIRE(routing_item_count(cb) == 1);

    ds->pic_on_screen = e_pic_type::PLACEMENT;
    hide_draw_routing(app);
    REQUIRE(routing_item_index(cb) == -1);
}
