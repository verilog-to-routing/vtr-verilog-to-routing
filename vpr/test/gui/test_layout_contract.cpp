/**
 * @brief Layout-geometry contract for the main UI form (Layer 3).
 *
 * The rest of the GUI suite asserts *widget* properties -- types, names, text,
 * item lists, checked state. None of it looks at how those widgets are placed,
 * so a form could satisfy every existing assertion while laying out wrongly:
 * a canvas in the wrong grid cell, a panel that no longer stretches, a switch
 * that became elastic. This file closes that gap.
 *
 * The expectations below were read off the form's rendered tree at the time
 * the UI was converted from Glade, so they also pin the geometry against
 * accidental drift when the form is edited.
 *
 * Tag: [layer3][layout-contract][vpr_gui]
 */

#include <catch2/catch_test_macros.hpp>

#include "test_main_ui.hpp"

#include <QBoxLayout>
#include <QGridLayout>
#include <QLayout>
#include <QMainWindow>
#include <QSizePolicy>
#include <QStatusBar>
#include <QWidget>

#include <vector>

namespace {

struct GridSlot {
    int row = -1;
    int column = -1;
    int row_span = 0;
    int column_span = 0;
};

/// Where `child` sits in its parent's QGridLayout. row stays -1 when the
/// parent has no grid layout or the child is not in it.
GridSlot slot_of(QWidget* child) {
    GridSlot slot;
    QWidget* parent = child ? child->parentWidget() : nullptr;
    auto* grid = parent ? qobject_cast<QGridLayout*>(parent->layout()) : nullptr;
    if (!grid) {
        return slot;
    }
    const int index = grid->indexOf(child);
    if (index < 0) {
        return slot;
    }
    grid->getItemPosition(index, &slot.row, &slot.column, &slot.row_span, &slot.column_span);
    return slot;
}

/// Popover content is laid out with a 4px inset; everything else ezgl builds
/// is flush. Identified by walking up to a Qt::Popup ancestor rather than by
/// name, so it holds for both formats.
bool inside_popup(const QWidget* w) {
    for (const QWidget* p = w; p != nullptr; p = p->parentWidget()) {
        // Compare against the masked window type, not a bitwise AND:
        // Qt::Popup is Qt::Window|0x8, so `flags & Qt::Popup` is non-zero for
        // any top-level window and would match everything.
        if ((p->windowFlags() & Qt::WindowType_Mask) == Qt::Popup) {
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("LayoutContract: OuterGrid stretches the canvas rows and its only column",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* outer = win->findChild<QWidget*>("OuterGrid");
    REQUIRE(outer != nullptr);

    auto* grid = qobject_cast<QGridLayout*>(outer->layout());
    REQUIRE(grid != nullptr);

    // The two menu bars keep their natural height; the canvas rows absorb
    // everything left over.
    CHECK(grid->rowStretch(0) == 0);
    CHECK(grid->rowStretch(1) == 0);
    CHECK(grid->rowStretch(2) == 1);
    CHECK(grid->columnStretch(0) == 1);
}

TEST_CASE("LayoutContract: MainCanvas expands and occupies the canvas rows",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* canvas = win->findChild<QWidget*>("MainCanvas");
    REQUIRE(canvas != nullptr);

    // Without both of these the drawing area stops filling the window.
    CHECK(canvas->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding);
    CHECK(canvas->sizePolicy().verticalPolicy() == QSizePolicy::Expanding);

    const GridSlot slot = slot_of(canvas);
    CHECK(slot.row == 2);
    CHECK(slot.column == 0);
    CHECK(slot.row_span == 3);
    CHECK(slot.column_span == 1);
}

TEST_CASE("LayoutContract: the menu bars sit above the canvas in column 0",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* top = win->findChild<QWidget*>("MenuGridTop");
    auto* bottom = win->findChild<QWidget*>("MenuGridBottom");
    REQUIRE(top != nullptr);
    REQUIRE(bottom != nullptr);

    const GridSlot top_slot = slot_of(top);
    const GridSlot bottom_slot = slot_of(bottom);
    CHECK(top_slot.row == 0);
    CHECK(top_slot.column == 0);
    CHECK(bottom_slot.row == 1);
    CHECK(bottom_slot.column == 0);
}

TEST_CASE("LayoutContract: ezgl layouts are flush except inside popovers",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // ezgl::applyLayoutDefaults zeroes margins and spacing on every layout it
    // builds. A stray default (Qt's is 9px/6px) shows up as the whole panel
    // drifting, which is easy to miss by eye and trivial to assert.
    int checked = 0;
    for (QWidget* w : win->findChildren<QWidget*>()) {
        QLayout* layout = w->layout();
        if (!layout || inside_popup(w)) {
            continue;
        }
        // Skip layouts Qt builds for itself rather than ones ezgl creates:
        // QMainWindow's own layout, and QStatusBar's internal QHBoxLayout
        // (Qt's default 6px spacing, never passed through
        // ezgl::applyLayoutDefaults).
        if (qobject_cast<QMainWindow*>(w) || qobject_cast<QStatusBar*>(w)) {
            continue;
        }
        const QMargins margins = layout->contentsMargins();
        INFO("layout of widget: " << w->objectName().toStdString());
        CHECK(margins.left() == 0);
        CHECK(margins.top() == 0);
        CHECK(margins.right() == 0);
        CHECK(margins.bottom() == 0);
        CHECK(layout->spacing() == 0);
        ++checked;
    }
    CHECK(checked > 0);
}

TEST_CASE("LayoutContract: switches keep their own fixed size policy",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // SwitchButton sets Fixed/Fixed for itself. The form declares each switch
    // as a placeholder widget that ezgl::UiLoader swaps out, and an early
    // version of that swap copied the placeholder's default Preferred policy
    // over the real one -- every switch silently became stretchable.
    const std::vector<const char*> switches = {
        "ToggleNets", "ToggleRR", "ToggleCritPath", "ProceedByStep"};

    for (const char* name : switches) {
        auto* sw = win->findChild<QWidget*>(name);
        INFO("switch: " << name);
        REQUIRE(sw != nullptr);
        CHECK(sw->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed);
        CHECK(sw->sizePolicy().verticalPolicy() == QSizePolicy::Fixed);
    }
}

TEST_CASE("LayoutContract: menu buttons share the bottom bar width",
          "[layer3][layout-contract][vpr_gui]") {
    auto mw = vpr_gui_test::load_main_ui();
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // These stretch to fill the bar instead of hugging their labels.
    const std::vector<const char*> menu_buttons = {
        "BlockMenuButton", "NetMenuButton", "RoutingMenuButton",
        "MiscMenuButton", "3DMenuButton"};

    for (const char* name : menu_buttons) {
        auto* button = win->findChild<QWidget*>(name);
        INFO("menu button: " << name);
        REQUIRE(button != nullptr);
        CHECK(button->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding);
    }
}
