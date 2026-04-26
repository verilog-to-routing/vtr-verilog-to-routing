/**
 * @file test_qtgladeloader.cpp
 * @brief Functional tests for QtGladeLoader — Glade XML → Qt widget tree.
 *
 * Tests individual widget type construction, property parsing, two-pass
 * popover loading, and error handling for invalid/missing files.
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

#include <ezgl/qt/qtgladeloader.hpp>
#include "test_gui_helpers.hpp"

// ---------------------------------------------------------------------------
// Basic loading
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: loadFile returns QMainWindow for valid file", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);
    delete win;
}

TEST_CASE("QtGladeLoader: loadFile returns null for nonexistent file", "[layer3][vpr_gui][!shouldfail]") {
    // KNOWN ISSUE: QtGladeLoader::loadFile() calls std::exit(1) on file-open
    // failure instead of returning nullptr. This test documents the expected
    // behavior once the loader is fixed. See defect log DEF-002.
    SKIP("QtGladeLoader calls std::exit(1) on missing file — cannot test without process death");
}

TEST_CASE("QtGladeLoader: loadFile returns null for empty path", "[layer3][vpr_gui][!shouldfail]") {
    // Same issue as above — std::exit(1) on empty path.
    SKIP("QtGladeLoader calls std::exit(1) on empty path — cannot test without process death");
}

// ---------------------------------------------------------------------------
// Widget type construction via main.ui
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: builds GtkWindow as QMainWindow", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    // MainWindow should be a QMainWindow
    REQUIRE(qobject_cast<QMainWindow*>(win) != nullptr);
    CHECK(win->windowTitle() == "VPR: Versatile Place and Route for FPGAs");

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkButton as QPushButton", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* btn = findWidgetByName<QPushButton>("Search");
    REQUIRE(btn != nullptr);
    CHECK(btn->text() == "Search");

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkLabel as QLabel", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* label = findWidgetByName<QLabel>("LayerLabel");
    REQUIRE(label != nullptr);
    CHECK(label->text() == "View: ");

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkComboBoxText as QComboBox with items", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
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

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkCheckButton as QCheckBox with active state", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    // blockOutline has active=True
    auto* bo = findWidgetByName<QCheckBox>("blockOutline");
    REQUIRE(bo != nullptr);
    CHECK(bo->isChecked());

    // drawPartitions has no active property → unchecked
    auto* dp = findWidgetByName<QCheckBox>("drawPartitions");
    REQUIRE(dp != nullptr);
    CHECK_FALSE(dp->isChecked());

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkSpinButton as QSpinBox", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* spin = findWidgetByName<QSpinBox>("ToggleBlkInternals");
    REQUIRE(spin != nullptr);

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkEntry as QLineEdit", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* entry = findWidgetByName<QLineEdit>("TextInput");
    REQUIRE(entry != nullptr);

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkGrid as QWidget with QGridLayout", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* outerGrid = findWidgetByName<QWidget>("OuterGrid");
    REQUIRE(outerGrid != nullptr);
    auto* gridLayout = qobject_cast<QGridLayout*>(outerGrid->layout());
    REQUIRE(gridLayout != nullptr);

    delete win;
}

TEST_CASE("QtGladeLoader: builds GtkDrawingArea as QWidget", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    auto* canvas = findWidgetByName<QWidget>("MainCanvas");
    REQUIRE(canvas != nullptr);

    delete win;
}

// ---------------------------------------------------------------------------
// Two-pass popover loading
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: popovers are loaded and accessible as children", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
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

    delete win;
}

// ---------------------------------------------------------------------------
// Sensitive (enabled) property
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: sensitive=False disables widget", "[layer3][vpr_gui][!mayfail]") {
    // KNOWN LIMITATION: QtGladeLoader::applyCommonProperties() does not
    // handle the "sensitive" Glade property. See defect log DEF-001.
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    // ToggleNetType has sensitive=False in XML
    auto* netType = findWidgetByName<QComboBox>("ToggleNetType");
    REQUIRE(netType != nullptr);
    CHECK_FALSE(netType->isEnabled());

    // ToggleInterClusterNets has sensitive=False
    auto* interCluster = findWidgetByName<QCheckBox>("ToggleInterClusterNets");
    REQUIRE(interCluster != nullptr);
    CHECK_FALSE(interCluster->isEnabled());

    delete win;
}

// ---------------------------------------------------------------------------
// Visibility property
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: visible=True widgets are not hidden", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win != nullptr);

    // In offscreen mode, isVisible() returns false unless the window is shown.
    // Instead, verify the widget is not explicitly hidden.
    auto* searchBtn = findWidgetByName<QPushButton>("Search");
    REQUIRE(searchBtn != nullptr);
    CHECK_FALSE(searchBtn->isHidden());

    delete win;
}

// ---------------------------------------------------------------------------
// Menu button hierarchy
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: GtkMenuButton built as QPushButton with popover association", "[layer3][vpr_gui]") {
    QtGladeLoader loader;
    QMainWindow* win = loader.loadFile(VPR_MAIN_UI_PATH);
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

    delete win;
}

// ---------------------------------------------------------------------------
// Loader can be reused for multiple loads
// ---------------------------------------------------------------------------

TEST_CASE("QtGladeLoader: can load the same file multiple times", "[layer3][vpr_gui]") {
    QtGladeLoader loader;

    QMainWindow* win1 = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win1 != nullptr);

    QMainWindow* win2 = loader.loadFile(VPR_MAIN_UI_PATH);
    REQUIRE(win2 != nullptr);

    CHECK(win1 != win2); // Each load produces a new window

    delete win1;
    delete win2;
}
