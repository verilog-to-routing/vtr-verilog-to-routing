/**
 * @file test_ui_contract.cpp
 * @brief GUI-T-018 — main.ui ↔ ui_setup.cpp widget-contract test (Layer 3).
 *
 * Closes the gap recorded in
 * doc/src/dev/vpr_gui_complete_test_plan.rst §12.2 ("the current suite touches
 * the *loading* surface, not the *behaviour* surface"): for every widget that
 * `vpr/src/draw/ui_setup.cpp`, `vpr/src/draw/draw.cpp`, `search_bar.cpp`, and
 * `draw_toggle_functions.cpp` look up at runtime via
 *   ezgl::application::find_push_button / find_check_box / find_combo_box
 *                       / find_spin_box / find_switch_button / find_line_edit
 * the loaded `main.ui` widget tree must contain a top-level widget with the
 * matching `objectName` AND of the correct Qt subclass.
 *
 * Why this is real, behaviour-relevant coverage rather than structural noise:
 *
 *   ui_setup.cpp would fail at runtime — silently for the non-fatal lookups,
 *   noisily on a dereference for the rest — if anyone renames or retypes a
 *   widget in main.ui. There is currently no compile-time, link-time, or
 *   test-time check binding the two together. main.ui is parsed at runtime
 *   by ezgl::MainWindow, and the C++ side casts via the qobject
 *   meta-system in find_* — both sides see "string -> QWidget" only.
 *
 * Per §4 of the test plan, this test follows the bug-discovery discipline:
 *   * No placeholders. Every spec entry asserts a real lookup that
 *     production code performs.
 *   * On failure the test does NOT swallow the discrepancy — a missing or
 *     mistyped widget is a real defect that gets a DEF-NNN entry and a
 *     paired GitHub issue before any code change.
 *
 * Spec table maintenance: one row per find_* call site in production code.
 * The companion audit grep (doc/src/dev/vpr_gui_complete_test_plan.rst §12.4)
 * is the source of truth; keep this table aligned with it.
 *
 * Tag: [layer3][ui-contract][vpr_gui][GUI-T-018]
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <memory>
#include <string>
#include <string_view>

#ifndef VPR_MAIN_UI_PATH
#define VPR_MAIN_UI_PATH ":/ezgl/main.ui"
#endif

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include <ezgl/main_window.hpp>
#include <ezgl/qt/switchbutton.hpp>

#include "test_gui_helpers.hpp"

namespace {

enum class WidgetKind {
    PushButton,
    CheckBox,
    ComboBox,
    SpinBox,
    SwitchButton,
    LineEdit,
};

struct WidgetSpec {
    const char* object_name;
    WidgetKind kind;
    const char* found_by; // production source file:line where the lookup happens
};

// ---------------------------------------------------------------------------
// Spec table — one row per find_* call site in production code.
//
// Audit (date-stamped, refreshed when a new find_* lands):
//   grep -nE 'find_(push_button|check_box|combo_box|spin_box|switch_button|line_edit)' vpr/src/draw/*.cpp
//
// `manualMove` is intentionally excluded: it is created at runtime by
// manual_moves.cpp and is not a static main.ui widget.
// ---------------------------------------------------------------------------
constexpr WidgetSpec kSpecs[] = {
    // --- ui_setup.cpp basic_button_setup ---
    {"Window", WidgetKind::PushButton, "ui_setup.cpp:50"},
    {"Search", WidgetKind::PushButton, "ui_setup.cpp:56"},
    {"SaveGraphics", WidgetKind::PushButton, "ui_setup.cpp:63"},
    {"SearchType", WidgetKind::ComboBox, "ui_setup.cpp:69"},

    // --- ui_setup.cpp net_button_setup ---
    {"ToggleNets", WidgetKind::SwitchButton, "ui_setup.cpp:85"},
    {"ToggleNetType", WidgetKind::ComboBox, "ui_setup.cpp:91"},
    {"NetAlpha", WidgetKind::SpinBox, "ui_setup.cpp:103"},
    {"NetMaxFanout", WidgetKind::SpinBox, "ui_setup.cpp:111"},
    {"ToggleInterClusterNets", WidgetKind::CheckBox, "ui_setup.cpp:97"},
    {"ToggleIntraClusterNets", WidgetKind::CheckBox, "ui_setup.cpp:99"},
    {"FanInFanOut", WidgetKind::CheckBox, "ui_setup.cpp:101"},

    // --- ui_setup.cpp block_button_setup ---
    {"ToggleBlkInternals", WidgetKind::SpinBox, "ui_setup.cpp:131"},
    {"ToggleBlkPinUtil", WidgetKind::ComboBox, "ui_setup.cpp:139"},
    {"TogglePlacementMacros", WidgetKind::ComboBox, "ui_setup.cpp:145"},
    {"ToggleNocBox", WidgetKind::ComboBox, "ui_setup.cpp:155"},

    // --- ui_setup.cpp routing_button_setup ---
    {"ToggleRR", WidgetKind::SwitchButton, "ui_setup.cpp:174"},
    {"ToggleCongestion", WidgetKind::ComboBox, "ui_setup.cpp:190"},
    {"ToggleCongestionCost", WidgetKind::ComboBox, "ui_setup.cpp:196"},
    {"ToggleRoutingBBox", WidgetKind::SpinBox, "ui_setup.cpp:202"},
    {"ToggleRoutingExpansionCost", WidgetKind::ComboBox, "ui_setup.cpp:211"},
    {"ToggleRoutingUtil", WidgetKind::ComboBox, "ui_setup.cpp:217"},

    // --- ui_setup.cpp critical_path_button_setup ---
    {"ToggleCritPath", WidgetKind::SwitchButton, "ui_setup.cpp:324"},

    // --- ui_setup.cpp autocomplete_setup ---
    {"TextInput", WidgetKind::LineEdit, "ui_setup.cpp:370"},

    // --- draw.cpp default_setup (callback wiring) ---
    {"ProceedButton", WidgetKind::PushButton, "draw.cpp:1159"},
    {"ZoomFitButton", WidgetKind::PushButton, "draw.cpp:1165"},
    {"PauseButton", WidgetKind::PushButton, "draw.cpp:1171"},
    {"blockOutline", WidgetKind::CheckBox, "draw.cpp:1177"},
    {"blockText", WidgetKind::CheckBox, "draw.cpp:1183"},
    {"clipRoutingUtil", WidgetKind::CheckBox, "draw.cpp:1189"},
    {"debugButton", WidgetKind::PushButton, "draw.cpp:1195"},
    {"drawPartitions", WidgetKind::CheckBox, "draw.cpp:1201"},

    // --- search_bar.cpp ---
    // (find_combo_box("SearchType") and find_line_edit("TextInput") already
    //  covered above; listing additional consumers keeps the contract
    //  traceable.)
};

// Cast helper that returns nullptr unless the discovered widget matches the
// expected Qt subtype. Two failure modes get distinct messages:
//   * "name not found" — widget tree has no widget with that objectName.
//   * "type mismatch"  — widget exists but is not the expected Qt subclass.
struct LookupResult {
    bool name_found = false;
    bool type_match = false;
    QWidget* raw = nullptr;
};

LookupResult lookup(const WidgetSpec& s) {
    LookupResult r{};
    r.raw = findWidgetByName<QWidget>(s.object_name);
    r.name_found = (r.raw != nullptr);
    if (!r.name_found) return r;
    switch (s.kind) {
        case WidgetKind::PushButton:
            r.type_match = (qobject_cast<QPushButton*>(r.raw) != nullptr);
            break;
        case WidgetKind::CheckBox:
            r.type_match = (qobject_cast<QCheckBox*>(r.raw) != nullptr);
            break;
        case WidgetKind::ComboBox:
            r.type_match = (qobject_cast<QComboBox*>(r.raw) != nullptr);
            break;
        case WidgetKind::SpinBox:
            r.type_match = (qobject_cast<QSpinBox*>(r.raw) != nullptr);
            break;
        case WidgetKind::SwitchButton:
            r.type_match = (qobject_cast<SwitchButton*>(r.raw) != nullptr);
            break;
        case WidgetKind::LineEdit:
            r.type_match = (qobject_cast<QLineEdit*>(r.raw) != nullptr);
            break;
        default:
            r.type_match = false;
            break;
    }
    return r;
}

const char* kind_name(WidgetKind k) {
    switch (k) {
        case WidgetKind::PushButton:
            return "QPushButton";
        case WidgetKind::CheckBox:
            return "QCheckBox";
        case WidgetKind::ComboBox:
            return "QComboBox";
        case WidgetKind::SpinBox:
            return "QSpinBox";
        case WidgetKind::SwitchButton:
            return "SwitchButton";
        case WidgetKind::LineEdit:
            return "QLineEdit";
        default:
            return "<unknown>";
    }
    return "<unknown>";
}

// ---------------------------------------------------------------------------
// Combo-content contract — for the menu combos whose item count and order
// drive UI semantics, lock both. Reordering the items in main.ui without
// updating the C++-side enum mappings would be a silent regression; this
// table is the gate that catches it.
// ---------------------------------------------------------------------------
struct ComboContentSpec {
    const char* object_name;
    int expected_count;
    const char* note;
};

constexpr ComboContentSpec kComboContent[] = {
    // SearchType drives the five-branch dispatch in search_bar.cpp; the
    // index-to-branch mapping is currentIndex()-based, so the count and
    // order are part of the contract.
    {"SearchType", 5, "Block ID / Block Name / Net ID / Net Name / RR Node ID"},
    // ToggleNetType: 0 -> Flylines, 1 -> Routing.
    {"ToggleNetType", 2, "Flylines / Routing"},
    // ToggleBlkPinUtil: 0 -> None, 1 -> All, 2 -> Inputs, 3 -> Outputs.
    {"ToggleBlkPinUtil", 4, "None / All / Inputs / Outputs"},
    // TogglePlacementMacros: 0 -> None, 1 -> Regular.
    {"TogglePlacementMacros", 2, "None / Regular"},
};

} // namespace

// ---------------------------------------------------------------------------
// Per-widget contract case (parameterised over kSpecs)
// ---------------------------------------------------------------------------

TEST_CASE("UiContract: every find_*() target exists in main.ui with correct Qt type",
          "[layer3][ui-contract][vpr_gui][GUI-T-018]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);
    win->show();

    auto idx = GENERATE(range(size_t{0}, std::size(kSpecs)));
    const WidgetSpec& s = kSpecs[idx];

    INFO("Spec[" << idx << "]: name='" << s.object_name
                 << "' kind=" << kind_name(s.kind)
                 << " found_by=" << s.found_by);

    LookupResult r = lookup(s);

    // First diagnostic: did the widget tree contain a widget with that name
    // at all? A miss here means main.ui dropped or renamed the widget.
    REQUIRE(r.name_found);

    // Second diagnostic: did it match the expected Qt subclass? A miss here
    // means main.ui retyped the widget (e.g. switched a SwitchButton to a
    // QCheckBox), which find_*() in production would either silently return
    // nullptr from or hit a qobject_cast assertion downstream.
    REQUIRE(r.type_match);
}

// ---------------------------------------------------------------------------
// Combo-content contracts — count is part of the dispatch contract.
// ---------------------------------------------------------------------------

TEST_CASE("UiContract: menu combos have the contracted item counts",
          "[layer3][ui-contract][vpr_gui][GUI-T-018]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);
    win->show();

    auto idx = GENERATE(range(size_t{0}, std::size(kComboContent)));
    const ComboContentSpec& c = kComboContent[idx];

    INFO("Combo[" << idx << "]: name='" << c.object_name
                  << "' expected_count=" << c.expected_count
                  << " (" << c.note << ")");

    auto* combo = findWidgetByName<QComboBox>(c.object_name);
    REQUIRE(combo != nullptr);
    REQUIRE(combo->count() == c.expected_count);
}

// ---------------------------------------------------------------------------
// SearchType ordering — locks the index-to-branch mapping that
// search_bar.cpp::search_and_highlight depends on. A reordering in main.ui
// would route every search to the wrong branch.
// ---------------------------------------------------------------------------

TEST_CASE("UiContract: SearchType combo entries appear in the contracted order",
          "[layer3][ui-contract][vpr_gui][GUI-T-018]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);
    win->show();

    auto* combo = findWidgetByName<QComboBox>("SearchType");
    REQUIRE(combo != nullptr);
    REQUIRE(combo->count() == 5);

    // The strings are matched against the if/else-if chain in
    // search_bar.cpp::search_and_highlight. Updating either side without
    // the other is a silent dispatch regression.
    CHECK(combo->itemText(0) == QString("Block ID"));
    CHECK(combo->itemText(1) == QString("Block Name"));
    CHECK(combo->itemText(2) == QString("Net ID"));
    CHECK(combo->itemText(3) == QString("Net Name"));
    CHECK(combo->itemText(4) == QString("RR Node ID"));
}

// ---------------------------------------------------------------------------
// MainCanvas / StatusBar — referenced by ezgl::application's draw loop and
// by VPR's status-message updates. Already implicitly required by other
// tests; we make the requirement explicit here so a renaming surfaces with
// a UiContract failure rather than a downstream crash.
// ---------------------------------------------------------------------------

TEST_CASE("UiContract: top-level canvas and status surfaces are present",
          "[layer3][ui-contract][vpr_gui][GUI-T-018]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);
    win->show();

    auto* canvas = findWidgetByName<QWidget>("MainCanvas");
    REQUIRE(canvas != nullptr);
    CHECK_FALSE(canvas->isHidden());

    auto* status = findWidgetByName<QWidget>("StatusBar");
    REQUIRE(status != nullptr);
}
