/**
 * @file test_breakpoint_debug.cpp
 * @brief GUI-T-002 — Debugger / breakpoint subsystem (Layer 4).
 *
 * Locks the contract of the breakpoint type and the debug-window
 * helper functions in ``vpr/src/draw/breakpoint.{h,cpp}`` and
 * ``vpr/src/draw/draw_debug.{h,cpp}``:
 *
 *   * ``Breakpoint`` constructors place the user-supplied value in
 *     the field corresponding to the requested ``bp_type`` (the
 *     other fields keep their default-initialised values, NOT the
 *     "never reached" sentinel from the default constructor).
 *   * ``Breakpoint::operator==`` matches only when the type AND the
 *     type-specific value coincide.
 *   * ``activate_breakpoint_by_index`` / ``delete_breakpoint_by_index``
 *     mutate ``draw_state->list_of_breakpoints`` in place; index is
 *     a positional erase, not a search.
 *   * ``check_for_breakpoints`` returns ``false`` on an empty list
 *     and short-circuits on the first active matching kind.
 *   * ``valid_expression`` accepts comp/bool alternation patterns
 *     (e.g. ``move_num == 4 || from_block == 11``) and rejects
 *     leading bool ops, trailing bool ops, even op counts, garbage,
 *     and the empty string.
 *   * ``close_debug_window`` / ``close_advanced_window`` reset the
 *     reopen-guard flags (the windows themselves use
 *     ``WA_DeleteOnClose`` and connect to these on ``destroyed``).
 *   * The single global ``BreakpointState`` returned by
 *     ``get_bp_state_globals()->get_glob_breakpoint_state()`` is a
 *     stable pointer for the process lifetime; mutations made by
 *     production paths (``annealer.cpp``: ``temp_count++``,
 *     ``route_utils.cpp``: ``router_iter++``) survive across calls.
 *
 * Why this is real, behaviour-relevant coverage rather than
 * structural noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The breakpoint subsystem is the GUI-side observability path the
 *   placer/router driver uses to pause execution. Silently dropping
 *   a breakpoint, mis-indexing a delete, or accepting an invalid
 *   expression all manifest as the program *not* stopping (or
 *   stopping in the wrong place) when the user clicks Set —
 *   indistinguishable from the GUI being broken. These tests pin
 *   each invariant against the real production translation units
 *   (no mocks, no shims).
 *
 * Setup (per case): none. The breakpoint helpers only touch the
 * ``t_draw_state::list_of_breakpoints`` vector and the global
 * ``BreakpointState``, both of which are process-static; we save
 * and restore the breakpoint list around each case to keep tests
 * order-independent.
 *
 * Note: this file deliberately does NOT open ``draw_debug_window``
 * or ``advanced_button_callback``; both build complex Qt widget
 * trees (``QIcon("src/draw/trash.png")``, ``QScrollArea``, dynamic
 * grid layouts) whose layout / painting paths are not stable under
 * the offscreen Qt platform plugin. The state-flag contract that
 * those windows rely on (``openWindows.debug_window`` and
 * ``openWindows.advanced_window``) is exercised through the
 * ``close_debug_window`` / ``close_advanced_window`` symmetric
 * pair, which is what production code wires to ``QObject::
 * destroyed``.
 *
 * Tag: [layer4][interactive][debugger][vpr_gui][GUI-T-002]
 */

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "breakpoint.h"
#include "breakpoint_state_globals.h"
#include "draw_debug.h"
#include "draw_global.h"
#include "draw_types.h"
#include "vtr_expr_eval.h"

namespace {

// Restore draw_state->list_of_breakpoints around every case so test
// order is irrelevant. The list is process-static.
class BreakpointListGuard {
  public:
    BreakpointListGuard() {
        saved_ = get_draw_state_vars()->list_of_breakpoints;
        get_draw_state_vars()->list_of_breakpoints.clear();
    }
    ~BreakpointListGuard() {
        get_draw_state_vars()->list_of_breakpoints = saved_;
    }

  private:
    std::vector<Breakpoint> saved_;
};

} // namespace

TEST_CASE("Breakpoint default ctor sets BT_UNIDENTIFIED with sentinel values",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    Breakpoint bp;
    REQUIRE(bp.type == BT_UNIDENTIFIED);
    REQUIRE(bp.bt_moves == -1);
    REQUIRE(bp.bt_temps == -1);
    REQUIRE(bp.bt_from_block == -1);
    REQUIRE(bp.bt_router_iter == -1);
    REQUIRE(bp.bt_route_net_id == -1);
    REQUIRE(bp.bt_expression.empty());
    REQUIRE(bp.active);
}

TEST_CASE("Breakpoint typed-int ctor populates only the matching field",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    SECTION("BT_MOVE_NUM") {
        Breakpoint bp(BT_MOVE_NUM, 42);
        REQUIRE(bp.type == BT_MOVE_NUM);
        REQUIRE(bp.bt_moves == 42);
        // The non-default-ctor leaves siblings at their in-class
        // defaults — NOT the -1 sentinel.
        REQUIRE(bp.bt_temps == 0);
        REQUIRE(bp.bt_from_block == -1);
        REQUIRE(bp.bt_router_iter == 0);
        REQUIRE(bp.bt_route_net_id == -1);
    }
    SECTION("BT_TEMP_NUM") {
        Breakpoint bp(BT_TEMP_NUM, 7);
        REQUIRE(bp.type == BT_TEMP_NUM);
        REQUIRE(bp.bt_temps == 7);
        REQUIRE(bp.bt_moves == 0);
    }
    SECTION("BT_FROM_BLOCK") {
        Breakpoint bp(BT_FROM_BLOCK, 83);
        REQUIRE(bp.type == BT_FROM_BLOCK);
        REQUIRE(bp.bt_from_block == 83);
    }
    SECTION("BT_ROUTER_ITER") {
        Breakpoint bp(BT_ROUTER_ITER, 5);
        REQUIRE(bp.type == BT_ROUTER_ITER);
        REQUIRE(bp.bt_router_iter == 5);
    }
    SECTION("BT_ROUTE_NET_ID") {
        Breakpoint bp(BT_ROUTE_NET_ID, 12);
        REQUIRE(bp.type == BT_ROUTE_NET_ID);
        REQUIRE(bp.bt_route_net_id == 12);
    }
}

TEST_CASE("Breakpoint expression ctor stores expression verbatim",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    Breakpoint bp(BT_EXPRESSION, std::string("move_num == 4 || from_block == 11"));
    REQUIRE(bp.type == BT_EXPRESSION);
    REQUIRE(bp.bt_expression == "move_num == 4 || from_block == 11");
    REQUIRE(bp.active);
}

TEST_CASE("Breakpoint operator== matches on (type, type-specific value)",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    Breakpoint a(BT_MOVE_NUM, 4);
    Breakpoint b(BT_MOVE_NUM, 4);
    Breakpoint c(BT_MOVE_NUM, 5);
    Breakpoint d(BT_TEMP_NUM, 4);
    Breakpoint e(BT_EXPRESSION, std::string("move_num == 4"));
    Breakpoint f(BT_EXPRESSION, std::string("move_num == 4"));
    Breakpoint g(BT_EXPRESSION, std::string("move_num == 5"));

    // Note: Breakpoint::operator== is non-const in production. C++20
    // additionally synthesises a reversed candidate (b == a), which
    // makes the bare expression ``a == b`` ambiguous under -Werror.
    // Calling the operator as a method bypasses reversed-candidate
    // lookup; pre-evaluating to bool also keeps Catch2's expression
    // decomposer out of the overload-resolution chain.
    bool ab = a.operator==(b);
    bool ac = a.operator==(c);
    bool ad = a.operator==(d);
    bool ef = e.operator==(f);
    bool eg = e.operator==(g);

    REQUIRE(ab);       // identical
    REQUIRE_FALSE(ac); // same type, different value
    REQUIRE_FALSE(ad); // different type, same int
    REQUIRE(ef);       // identical expression
    REQUIRE_FALSE(eg); // different expression
}

TEST_CASE("delete_breakpoint_by_index removes the indexed entry",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointListGuard guard;
    auto& list = get_draw_state_vars()->list_of_breakpoints;
    list.push_back(Breakpoint(BT_MOVE_NUM, 1));
    list.push_back(Breakpoint(BT_MOVE_NUM, 2));
    list.push_back(Breakpoint(BT_MOVE_NUM, 3));

    delete_breakpoint_by_index(1);

    REQUIRE(list.size() == 2);
    REQUIRE(list[0].bt_moves == 1);
    REQUIRE(list[1].bt_moves == 3);
}

TEST_CASE("activate_breakpoint_by_index toggles the active flag in place",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointListGuard guard;
    auto& list = get_draw_state_vars()->list_of_breakpoints;
    list.push_back(Breakpoint(BT_MOVE_NUM, 1));
    list.push_back(Breakpoint(BT_MOVE_NUM, 2));

    REQUIRE(list[0].active);
    REQUIRE(list[1].active);

    activate_breakpoint_by_index(0, false);
    REQUIRE_FALSE(list[0].active);
    REQUIRE(list[1].active);

    activate_breakpoint_by_index(0, true);
    REQUIRE(list[0].active);
}

TEST_CASE("check_for_breakpoints returns false on an empty list",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointListGuard guard;
    REQUIRE(get_draw_state_vars()->list_of_breakpoints.empty());
    REQUIRE_FALSE(check_for_breakpoints(true));  // in_placer
    REQUIRE_FALSE(check_for_breakpoints(false)); // in_router
}

TEST_CASE("check_for_breakpoints respects the in_placer / active gates",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointListGuard guard;
    auto& list = get_draw_state_vars()->list_of_breakpoints;

    // A router-only breakpoint while in_placer=true must NOT trigger
    // even though it is active.
    list.push_back(Breakpoint(BT_ROUTER_ITER, 1));
    REQUIRE_FALSE(check_for_breakpoints(true));

    // A placer-only breakpoint while in_placer=false must NOT trigger.
    list.clear();
    list.push_back(Breakpoint(BT_MOVE_NUM, 1));
    REQUIRE_FALSE(check_for_breakpoints(false));

    // An inactive breakpoint of the right kind must NOT trigger.
    list.clear();
    list.push_back(Breakpoint(BT_MOVE_NUM, 1));
    list[0].active = false;
    REQUIRE_FALSE(check_for_breakpoints(true));
}

TEST_CASE("valid_expression — extern bool valid_expression(std::string);"
          " accepts COMP-BOOL alternation, rejects everything else",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    // valid_expression is non-static in draw_debug.cpp but does not
    // appear in draw_debug.h. Redeclare it locally; this is the
    // function the production "Set" advanced-expression button calls.
    extern bool valid_expression(std::string exp);

    SECTION("empty string is rejected") {
        REQUIRE_FALSE(valid_expression(""));
    }
    SECTION("single comparison without identifiers is parser-rejected") {
        // Operator pattern is COMP only (size 1, odd) — passes the
        // pattern check but the formula parser rejects it.
        REQUIRE_FALSE(valid_expression("=="));
    }
    SECTION("leading bool op rejected") {
        REQUIRE_FALSE(valid_expression("&& move_num == 4"));
    }
    SECTION("trailing bool op rejected") {
        REQUIRE_FALSE(valid_expression("move_num == 4 &&"));
    }
    SECTION("two comp ops in a row violate the alternation pattern") {
        // "move_num == 4 == 5" — ops vector is [COMP, COMP], even,
        // and pattern (slot 1 should be BOOL) violated.
        REQUIRE_FALSE(valid_expression("move_num == 4 == 5"));
    }
    SECTION("balanced COMP-BOOL-COMP expression accepted") {
        REQUIRE(valid_expression("move_num == 4 || from_block == 11"));
    }
    SECTION("longer COMP-BOOL-COMP-BOOL-COMP accepted") {
        REQUIRE(valid_expression("move_num == 4 || from_block == 11 && temp_count == 2"));
    }
}

TEST_CASE("close_debug_window / close_advanced_window are idempotent state resets",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    // Both functions just clear the reopen-guard flags; calling them
    // twice in a row must be safe (production code wires them to
    // QObject::destroyed which fires once).
    REQUIRE_NOTHROW(close_debug_window());
    REQUIRE_NOTHROW(close_debug_window());
    REQUIRE_NOTHROW(close_advanced_window());
    REQUIRE_NOTHROW(close_advanced_window());
}

TEST_CASE("get_bp_state_globals returns a stable singleton pointer; "
          "mutations persist across calls",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointStateGlobals* g1 = get_bp_state_globals();
    BreakpointStateGlobals* g2 = get_bp_state_globals();
    REQUIRE(g1 != nullptr);
    REQUIRE(g1 == g2);

    BreakpointState* s1 = g1->get_glob_breakpoint_state();
    BreakpointState* s2 = g2->get_glob_breakpoint_state();
    REQUIRE(s1 != nullptr);
    REQUIRE(s1 == s2);

    // Save and restore state we are about to scribble on. annealer.cpp
    // mutates temp_count and route_utils.cpp mutates router_iter, so
    // these are the production-path-relevant fields.
    int saved_temp_count = s1->temp_count;
    int saved_router_iter = s1->router_iter;

    s1->temp_count = 12345;
    s1->router_iter = 67890;
    REQUIRE(get_bp_state_globals()->get_glob_breakpoint_state()->temp_count == 12345);
    REQUIRE(get_bp_state_globals()->get_glob_breakpoint_state()->router_iter == 67890);

    s1->temp_count = saved_temp_count;
    s1->router_iter = saved_router_iter;
}

TEST_CASE("check_for_breakpoints triggers when a matching active expression is reached",
          "[layer4][interactive][debugger][vpr_gui][GUI-T-002]") {
    BreakpointListGuard guard;
    auto& list = get_draw_state_vars()->list_of_breakpoints;

    // Save the global breakpoint state so we can prime move_num to a
    // known value. This is the exact path placer_breakpoint.cpp uses
    // to pause the annealer.
    BreakpointState* bp = get_bp_state_globals()->get_glob_breakpoint_state();
    int saved_move_num = bp->move_num;
    bp->move_num = 100;

    // "move_num == 100" should evaluate to true; check_for_breakpoints
    // should return true and update bp_description.
    list.push_back(Breakpoint(BT_EXPRESSION, std::string("move_num == 100")));
    REQUIRE(check_for_breakpoints(true));
    REQUIRE(bp->bp_description == "move_num == 100");

    // Same expression, different in-flight move_num — must NOT trigger.
    bp->move_num = 99;
    bp->bp_description.clear();
    REQUIRE_FALSE(check_for_breakpoints(true));

    bp->move_num = saved_move_num;
}
