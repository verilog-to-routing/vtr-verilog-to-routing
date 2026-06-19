/**
 * @file test_widget_hierarchy.cpp
 * @brief Functional tests for the widget hierarchy produced by loading
 *        main.ui through ezgl::MainWindow.
 *
 * Covers individual widget type construction, property parsing, two-pass
 * popover loading, and error handling for invalid/missing files. The
 * underlying parser is QtGladeLoader today; the planned switch to a
 * Qt Designer .ui loader will be transparent to these tests because
 * they only address ezgl::MainWindow's public surface.
 *
 * Tag: [layer3][vpr_gui]
 */

#include <catch2/catch_test_macros.hpp>

#ifndef VPR_MAIN_UI_PATH
#define VPR_MAIN_UI_PATH ":/ezgl/main.ui"
#endif

#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QBoxLayout>

#include <ezgl/main_window.hpp>
#include "test_gui_helpers.hpp"

// ---------------------------------------------------------------------------
// Basic loading
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: loadFile returns QMainWindow for valid file", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);
}

TEST_CASE("WidgetHierarchy: loadFile returns null for nonexistent file", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw("/nonexistent/path/to/missing.ui");
    CHECK(mw.window() == nullptr);
}

TEST_CASE("WidgetHierarchy: loadFile returns null for empty path", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(QString{});
    CHECK(mw.window() == nullptr);
}

// ---------------------------------------------------------------------------
// Widget type construction via main.ui
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: builds GtkWindow as QMainWindow", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // MainWindow should be a QMainWindow
    REQUIRE(qobject_cast<QMainWindow*>(win) != nullptr);
    CHECK(win->windowTitle() == "VPR: Versatile Place and Route for FPGAs");
}

TEST_CASE("WidgetHierarchy: builds GtkButton as QPushButton", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* btn = findWidgetByName<QPushButton>("Search");
    REQUIRE(btn != nullptr);
    CHECK(btn->text() == "Search");
}

TEST_CASE("WidgetHierarchy: builds GtkLabel as QLabel", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* label = findWidgetByName<QLabel>("LayerLabel");
    REQUIRE(label != nullptr);
    CHECK(label->text() == "View: ");
}

TEST_CASE("WidgetHierarchy: builds GtkComboBoxText as QComboBox with items", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* combo = findWidgetByName<QComboBox>("SearchType");
    REQUIRE(combo != nullptr);
    CHECK(combo->count() == 5);
    CHECK(combo->currentIndex() == 0);

    // Verify item text
    CHECK(combo->itemText(0) == "Block ID");
    CHECK(combo->itemText(1) == "Block Name");
    CHECK(combo->itemText(2) == "Net ID");
    CHECK(combo->itemText(3) == "Net Name");
    CHECK(combo->itemText(4) == "RR Node ID");
}

TEST_CASE("WidgetHierarchy: builds GtkCheckButton as QCheckBox with active state", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // blockOutline has active=True
    auto* bo = findWidgetByName<QCheckBox>("blockOutline");
    REQUIRE(bo != nullptr);
    CHECK(bo->isChecked());

    // drawPartitions has no active property → unchecked
    auto* dp = findWidgetByName<QCheckBox>("drawPartitions");
    REQUIRE(dp != nullptr);
    CHECK_FALSE(dp->isChecked());
}

TEST_CASE("WidgetHierarchy: builds GtkSpinButton as QSpinBox", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* spin = findWidgetByName<QSpinBox>("ToggleBlkInternals");
    REQUIRE(spin != nullptr);
}

TEST_CASE("WidgetHierarchy: builds GtkEntry as QLineEdit", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* entry = findWidgetByName<QLineEdit>("TextInput");
    REQUIRE(entry != nullptr);
}

TEST_CASE("WidgetHierarchy: builds GtkGrid as QWidget with QGridLayout", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* outerGrid = findWidgetByName<QWidget>("OuterGrid");
    REQUIRE(outerGrid != nullptr);
    auto* gridLayout = qobject_cast<QGridLayout*>(outerGrid->layout());
    REQUIRE(gridLayout != nullptr);
}

TEST_CASE("WidgetHierarchy: builds GtkDrawingArea as QWidget", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    auto* canvas = findWidgetByName<QWidget>("MainCanvas");
    REQUIRE(canvas != nullptr);
}

// ---------------------------------------------------------------------------
// Two-pass popover loading
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: popovers are loaded and accessible as children", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // Popovers should be loaded in first pass before the main window
    // Their children should be findable from the window

    // BlockPopover contents
    auto* blkPinUtil = findWidgetByName<QComboBox>("ToggleBlkPinUtil");
    REQUIRE(blkPinUtil != nullptr);

    // NetPopover contents
    auto* netType = findWidgetByName<QComboBox>("ToggleNetType");
    REQUIRE(netType != nullptr);

    // RoutingPopover contents
    auto* congestion = findWidgetByName<QComboBox>("ToggleCongestion");
    REQUIRE(congestion != nullptr);

    // MiscPopover contents
    auto* saveGfx = findWidgetByName<QPushButton>("SaveGraphics");
    REQUIRE(saveGfx != nullptr);
}

// ---------------------------------------------------------------------------
// Sensitive (enabled) property
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: sensitive=False disables widget", "[layer3][vpr_gui][!mayfail]") {
    // KNOWN LIMITATION: QtGladeLoader::applyCommonProperties() does not
    // handle the "sensitive" Glade property. See defect log DEF-001.
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // ToggleNetType has sensitive=False in XML
    auto* netType = findWidgetByName<QComboBox>("ToggleNetType");
    REQUIRE(netType != nullptr);
    CHECK_FALSE(netType->isEnabled());

    // ToggleInterClusterNets has sensitive=False
    auto* interCluster = findWidgetByName<QCheckBox>("ToggleInterClusterNets");
    REQUIRE(interCluster != nullptr);
    CHECK_FALSE(interCluster->isEnabled());
}

// ---------------------------------------------------------------------------
// Visibility property
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: visible=True widgets are not hidden", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // In offscreen mode, isVisible() returns false unless the window is shown.
    // Instead, verify the widget is not explicitly hidden.
    auto* searchBtn = findWidgetByName<QPushButton>("Search");
    REQUIRE(searchBtn != nullptr);
    CHECK_FALSE(searchBtn->isHidden());
}

// ---------------------------------------------------------------------------
// Menu button hierarchy
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: GtkMenuButton built as QPushButton with popover association", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    QMainWindow* win = mw.window();
    REQUIRE(win != nullptr);

    // GtkMenuButton children (GtkLabel) are consumed as button text, not
    // built as separate widgets. Verify via the button text instead.
    auto* blockBtn = findWidgetByName<QPushButton>("BlockMenuButton");
    REQUIRE(blockBtn != nullptr);
    CHECK(blockBtn->text() == "Block");

    auto* netBtn = findWidgetByName<QPushButton>("NetMenuButton");
    REQUIRE(netBtn != nullptr);
    CHECK(netBtn->text() == "Net");

    auto* miscBtn = findWidgetByName<QPushButton>("MiscMenuButton");
    REQUIRE(miscBtn != nullptr);
    CHECK(miscBtn->text() == "Misc.");
}

// ---------------------------------------------------------------------------
// Loader can be reused for multiple loads
// ---------------------------------------------------------------------------

TEST_CASE("WidgetHierarchy: can load the same file multiple times", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw1(VPR_MAIN_UI_PATH);
    REQUIRE(mw1.window() != nullptr);

    ezgl::MainWindow mw2(VPR_MAIN_UI_PATH);
    REQUIRE(mw2.window() != nullptr);

    CHECK(mw1.window() != mw2.window()); // Each load produces a new window
}
