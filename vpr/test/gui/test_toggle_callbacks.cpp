/**
 * @file test_toggle_callbacks.cpp
 * @brief GUI-T-021 — draw_toggle_functions.cpp callback contracts (Layer 4).
 *
 * Locks the production callback functions wired to the View / Toggle
 * widgets in main.ui. Each callback is a free function in
 * ``vpr/src/draw/draw_toggle_functions.cpp`` whose contract is:
 *
 *   1. Read the current Qt widget state (``QComboBox::currentText``,
 *      ``QSpinBox::value``, or the ``bool state`` argument coming from
 *      ``SwitchButton::toggled``).
 *   2. Translate that into the corresponding ``draw_state`` enum /
 *      counter (``draw_state->show_nets``, ``show_congestion``,
 *      ``show_routing_costs``, ``show_routing_util``,
 *      ``show_blk_pin_util``, ``show_blk_internal``,
 *      ``show_placement_macros``, ``show_router_expansion_cost``,
 *      ``draw_noc``).
 *   3. Call ``app->refresh_drawing()`` so the canvas redraws.
 *
 * A regression in the QString → enum mapping silently flips what the
 * user sees on the canvas (e.g. "Routing Util with Value" rendering
 * "Routing Util with Formula" overlays). This file pins each branch
 * of the mapping against the *real* exported callbacks; no mocks.
 *
 * Why this is real, behaviour-relevant coverage rather than structural
 * noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The toggle callbacks are the only seam between the View-menu Qt
 *   widgets and the renderer's ``draw_state`` enums. Production has no
 *   intermediate hook points — every regression here is a silent
 *   render-mode flip that no other layer can catch. We exercise the
 *   actual exported symbols with real ``QComboBox`` / ``QSpinBox``
 *   instances populated with the same item strings ``ui_setup.cpp``
 *   uses, against the live ``ezgl::application`` from EzglAppFixture
 *   so the trailing ``find_widget`` and ``refresh_drawing`` calls run
 *   end-to-end.
 *
 * Tag: [layer4][toggle][callbacks][vpr_gui][GUI-T-021]
 */

#include <catch2/catch_test_macros.hpp>

#include <QComboBox>
#include <QSpinBox>
#include <QString>

#include <ezgl/application.hpp>

#include "draw_global.h"
#include "draw_toggle_functions.h"
#include "draw_types.h"
#include "test_ezgl_app_fixture.hpp"

using vpr_gui_test::EzglAppFixture;

namespace {

/// Helper: seed an existing QComboBox with one item whose text is ``s``
/// and select it as currentText. QComboBox is non-copyable so the box
/// itself must be constructed by the caller.
void seed_combo(QComboBox& box, const char* s) {
    box.clear();
    box.addItem(QString::fromUtf8(s));
    box.setCurrentIndex(0);
}

/// Helper: seed an existing QSpinBox with a wide range and a value.
void seed_spin(QSpinBox& sb, int v, int lo = -100, int hi = 100) {
    sb.setRange(lo, hi);
    sb.setValue(v);
}

} // namespace

TEST_CASE("ToggleCbk: toggle_draw_nets_cbk maps Routing/Flylines",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    {
        QComboBox box;
        seed_combo(box, "Routing");
        toggle_draw_nets_cbk(&box, fx.app());
        REQUIRE(ds->draw_nets == DRAW_ROUTED_NETS);
    }

    {
        QComboBox box;
        seed_combo(box, "Flylines");
        toggle_draw_nets_cbk(&box, fx.app());
        REQUIRE(ds->draw_nets == DRAW_FLYLINES);
    }
}

TEST_CASE("ToggleCbk: toggle_cong_cbk maps None/Congested/CongestedWithNets",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    {
        QComboBox box;
        seed_combo(box, "None");
        toggle_cong_cbk(&box, fx.app());
        REQUIRE(ds->show_congestion == DRAW_NO_CONGEST);
    }
    {
        QComboBox box;
        seed_combo(box, "Congested");
        toggle_cong_cbk(&box, fx.app());
        REQUIRE(ds->show_congestion == DRAW_CONGESTED);
    }
    {
        QComboBox box;
        seed_combo(box, "Congested with Nets");
        toggle_cong_cbk(&box, fx.app());
        REQUIRE(ds->show_congestion == DRAW_CONGESTED_WITH_NETS);
    }
}

TEST_CASE("ToggleCbk: toggle_cong_cost_cbk maps every routing-cost label",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    struct Case {
        const char* label;
        e_draw_routing_costs expected;
    };
    const Case cases[] = {
        {"None", DRAW_NO_ROUTING_COSTS},
        {"Total Routing Costs", DRAW_TOTAL_ROUTING_COSTS},
        {"Log Total Routing Costs", DRAW_LOG_TOTAL_ROUTING_COSTS},
        {"Acc Routing Costs", DRAW_ACC_ROUTING_COSTS},
        {"Log Acc Routing Costs", DRAW_LOG_ACC_ROUTING_COSTS},
        {"Pres Routing Costs", DRAW_PRES_ROUTING_COSTS},
        {"Log Pres Routing Costs", DRAW_LOG_PRES_ROUTING_COSTS},
        {"Base Routing Costs", DRAW_BASE_ROUTING_COSTS},
    };

    for (const auto& c : cases) {
        QComboBox box;
        seed_combo(box, c.label);
        toggle_cong_cost_cbk(&box, fx.app());
        REQUIRE(ds->show_routing_costs == c.expected);
    }
}

TEST_CASE("ToggleCbk: toggle_router_util_cbk maps every util label",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    struct Case {
        const char* label;
        e_draw_routing_util expected;
    };
    const Case cases[] = {
        {"None", DRAW_NO_ROUTING_UTIL},
        {"Routing Util", DRAW_ROUTING_UTIL},
        {"Routing Util with Value", DRAW_ROUTING_UTIL_WITH_VALUE},
        {"Routing Util with Formula", DRAW_ROUTING_UTIL_WITH_FORMULA},
        {"Routing Util Over Blocks", DRAW_ROUTING_UTIL_OVER_BLOCKS},
    };

    for (const auto& c : cases) {
        QComboBox box;
        seed_combo(box, c.label);
        toggle_router_util_cbk(&box, fx.app());
        REQUIRE(ds->show_routing_util == c.expected);
    }
}

TEST_CASE("ToggleCbk: toggle_blk_pin_util_cbk maps All/Inputs/Outputs",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    {
        QComboBox box;
        seed_combo(box, "All");
        toggle_blk_pin_util_cbk(&box, fx.app());
        REQUIRE(ds->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_TOTAL);
    }
    {
        QComboBox box;
        seed_combo(box, "Inputs");
        toggle_blk_pin_util_cbk(&box, fx.app());
        REQUIRE(ds->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_INPUTS);
    }
    {
        QComboBox box;
        seed_combo(box, "Outputs");
        toggle_blk_pin_util_cbk(&box, fx.app());
        REQUIRE(ds->show_blk_pin_util == DRAW_BLOCK_PIN_UTIL_OUTPUTS);
    }
}

TEST_CASE("ToggleCbk: placement_macros_cbk None/On",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    {
        QComboBox box;
        seed_combo(box, "None");
        placement_macros_cbk(&box, fx.app());
        REQUIRE(ds->show_placement_macros == DRAW_NO_PLACEMENT_MACROS);
    }
    {
        QComboBox box;
        seed_combo(box, "On");
        placement_macros_cbk(&box, fx.app());
        REQUIRE(ds->show_placement_macros == DRAW_PLACEMENT_MACROS);
    }
}

TEST_CASE("ToggleCbk: toggle_blk_internal_cbk clamps at 0 and max_sub_blk_lvl",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    const int saved_max = ds->max_sub_blk_lvl;
    ds->max_sub_blk_lvl = 3;

    {
        QSpinBox sb;
        seed_spin(sb, -5);
        toggle_blk_internal_cbk(&sb, fx.app());
        REQUIRE(ds->show_blk_internal == 0);
    }
    {
        QSpinBox sb;
        seed_spin(sb, 2);
        toggle_blk_internal_cbk(&sb, fx.app());
        REQUIRE(ds->show_blk_internal == 2);
    }
    {
        QSpinBox sb;
        seed_spin(sb, 99);
        toggle_blk_internal_cbk(&sb, fx.app());
        REQUIRE(ds->show_blk_internal == ds->max_sub_blk_lvl);
    }

    ds->max_sub_blk_lvl = saved_max;
}

TEST_CASE("ToggleCbk: toggle_routing_bbox_cbk no-ops when route_bb is empty",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    // The early-return-on-empty branch is the only path safely
    // exercisable from a unit test (route_bb is empty until VPR has
    // routed a netlist). The canonical behaviour we lock is: do not
    // crash and do not mutate show_routing_bb.
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    const int saved = ds->show_routing_bb;
    QSpinBox sb;
    seed_spin(sb, 7);
    toggle_routing_bbox_cbk(&sb, fx.app());
    REQUIRE(ds->show_routing_bb == saved);
}

TEST_CASE("ToggleCbk: set_net_max_fanout_cbk and set_net_alpha_value_cbk "
          "propagate spinbox values to draw_state",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    {
        QSpinBox sb;
        seed_spin(sb, 42, 0, 1000);
        set_net_max_fanout_cbk(&sb, fx.app());
        REQUIRE(ds->draw_net_max_fanout == 42);
    }

    {
        // NetAlpha widget is a 0..255 slider; production mapping is
        // ``net_alpha = 255 - value`` (see set_net_alpha_value_cbk).
        QSpinBox sb;
        seed_spin(sb, 50, 0, 255);
        set_net_alpha_value_cbk(&sb, fx.app());
        REQUIRE(ds->net_alpha == 205);
    }
}

TEST_CASE("ToggleCbk: toggle_expansion_cost_cbk None resets to default",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    QComboBox box;
    seed_combo(box, "None");
    toggle_expansion_cost_cbk(&box, fx.app());
    REQUIRE(ds->show_router_expansion_cost == DRAW_NO_ROUTER_EXPANSION_COST);
}

TEST_CASE("ToggleCbk: toggle_noc_cbk None branch resets draw_noc",
          "[layer4][toggle][vpr_gui][GUI-T-021]") {
    EzglAppFixture fx;
    t_draw_state* ds = get_draw_state_vars();

    QComboBox box;
    seed_combo(box, "None");
    toggle_noc_cbk(&box, fx.app());
    REQUIRE(ds->draw_noc == DRAW_NO_NOC);
}
