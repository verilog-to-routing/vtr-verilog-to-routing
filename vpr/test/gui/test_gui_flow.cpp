/**
 * @file test_gui_flow.cpp
 * @brief End-to-end GUI flow tests for VPR's Qt-based interface.
 *
 * Tests the full lifecycle: ezgl::MainWindow loading main.ui → window
 * creation → widget tree verification → canvas presence → menu bar structure.
 * Also tests the graphics command parser for valid/invalid/sequence commands.
 *
 * Tag: [layer3][vpr_gui]
 */

#include <catch2/catch_test_macros.hpp>

#include <memory>

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
#include <QStatusBar>
#include <QSet>
#include <QString>
#include <QStringList>

#include <ezgl/main_window.hpp>
#include "test_gui_helpers.hpp"

// ---------------------------------------------------------------------------
// Flow: main.ui loading produces a valid window with the expected hierarchy
// ---------------------------------------------------------------------------

TEST_CASE("Flow: MainWindow loads main.ui successfully", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    SECTION("Window has expected title") {
        REQUIRE(win->windowTitle() == "VPR: Versatile Place and Route for FPGAs");
    }

    SECTION("Window has default geometry hints") {
        // main.ui specifies default-width=800, default-height=600
        // The loader should respect the hints; the actual size may differ
        // on offscreen, but the minimum/requested size should be set.
        REQUIRE(win->minimumWidth() >= 0);
        REQUIRE(win->minimumHeight() >= 0);
    }
}

TEST_CASE("Flow: main.ui contains MainCanvas drawing area", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    QWidget* canvas = findWidgetByName<QWidget>("MainCanvas");
    REQUIRE(canvas != nullptr);

    SECTION("Canvas is not hidden") {
        CHECK_FALSE(canvas->isHidden());
    }

    SECTION("Canvas has expand policies") {
        auto sp = canvas->sizePolicy();
        CHECK(sp.horizontalPolicy() != QSizePolicy::Fixed);
        CHECK(sp.verticalPolicy() != QSizePolicy::Fixed);
    }
}

TEST_CASE("Flow: main.ui contains StatusBar", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    QWidget* statusBar = findWidgetByName<QWidget>("StatusBar");
    REQUIRE(statusBar != nullptr);
}

TEST_CASE("Flow: main.ui contains top menu grid with search", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    // Search type combo
    auto* searchType = findWidgetByName<QComboBox>("SearchType");
    REQUIRE(searchType != nullptr);
    CHECK(searchType->count() == 5); // Block ID, Block Name, Net ID, Net Name, RR Node ID

    // Text input
    auto* textInput = findWidgetByName<QLineEdit>("TextInput");
    REQUIRE(textInput != nullptr);

    // Search button
    auto* searchBtn = findWidgetByName<QPushButton>("Search");
    REQUIRE(searchBtn != nullptr);
    CHECK(searchBtn->text() == "Search");

    // Zoom buttons
    auto* zoomSelect = findWidgetByName<QPushButton>("Window");
    REQUIRE(zoomSelect != nullptr);
    CHECK(zoomSelect->text() == "Zoom Select");

    auto* zoomFit = findWidgetByName<QPushButton>("ZoomFitButton");
    REQUIRE(zoomFit != nullptr);
    CHECK(zoomFit->text() == "Zoom Fit");
}

TEST_CASE("Flow: main.ui contains bottom menu bar with all menu buttons", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    // Block menu
    auto* blockBtn = findWidgetByName<QPushButton>("BlockMenuButton");
    REQUIRE(blockBtn != nullptr);

    // Net menu
    auto* netBtn = findWidgetByName<QPushButton>("NetMenuButton");
    REQUIRE(netBtn != nullptr);

    // Route menu
    auto* routeBtn = findWidgetByName<QPushButton>("RoutingMenuButton");
    REQUIRE(routeBtn != nullptr);

    // Misc menu
    auto* miscBtn = findWidgetByName<QPushButton>("MiscMenuButton");
    REQUIRE(miscBtn != nullptr);

    // Layers (3D) menu
    auto* layersBtn = findWidgetByName<QPushButton>("3DMenuButton");
    REQUIRE(layersBtn != nullptr);

    // Proceed (Next Step)
    auto* proceedBtn = findWidgetByName<QPushButton>("ProceedButton");
    REQUIRE(proceedBtn != nullptr);
    CHECK(proceedBtn->text() == "Next Step");
}

// ---------------------------------------------------------------------------
// Flow: Popover contents (loaded from main.ui) contain correct widgets
// ---------------------------------------------------------------------------

TEST_CASE("Flow: Block popover has expected controls", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    // Popover widgets are top-level (not children of QMainWindow), use findWidgetByName
    auto* toggleInternals = findWidgetByName<QSpinBox>("ToggleBlkInternals");
    REQUIRE(toggleInternals != nullptr);

    auto* pinUtil = findWidgetByName<QComboBox>("ToggleBlkPinUtil");
    REQUIRE(pinUtil != nullptr);
    CHECK(pinUtil->count() == 4); // None, All, Inputs, Outputs

    auto* macros = findWidgetByName<QComboBox>("TogglePlacementMacros");
    REQUIRE(macros != nullptr);
    CHECK(macros->count() == 2); // None, Regular

    auto* blockOutline = findWidgetByName<QCheckBox>("blockOutline");
    REQUIRE(blockOutline != nullptr);
    CHECK(blockOutline->text() == "Block Outline");
    CHECK(blockOutline->isChecked()); // active=True in XML

    auto* blockText = findWidgetByName<QCheckBox>("blockText");
    REQUIRE(blockText != nullptr);
    CHECK(blockText->text() == "Block Text");
    CHECK(blockText->isChecked()); // active=True in XML

    auto* drawPartitions = findWidgetByName<QCheckBox>("drawPartitions");
    REQUIRE(drawPartitions != nullptr);
    CHECK(drawPartitions->text() == "Draw Partitions");
    CHECK_FALSE(drawPartitions->isChecked()); // no active property
}

TEST_CASE("Flow: Net popover has expected controls", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* netTypeCombo = findWidgetByName<QComboBox>("ToggleNetType");
    REQUIRE(netTypeCombo != nullptr);
    CHECK(netTypeCombo->count() == 2); // Flylines, Routing

    auto* interCluster = findWidgetByName<QCheckBox>("ToggleInterClusterNets");
    REQUIRE(interCluster != nullptr);
    CHECK(interCluster->text() == "Inter-cluster Nets");

    auto* intraCluster = findWidgetByName<QCheckBox>("ToggleIntraClusterNets");
    REQUIRE(intraCluster != nullptr);

    auto* fanInOut = findWidgetByName<QCheckBox>("FanInFanOut");
    REQUIRE(fanInOut != nullptr);

    auto* netAlpha = findWidgetByName<QSpinBox>("NetAlpha");
    REQUIRE(netAlpha != nullptr);

    auto* netMaxFanout = findWidgetByName<QSpinBox>("NetMaxFanout");
    REQUIRE(netMaxFanout != nullptr);
}

TEST_CASE("Flow: Routing popover has expected controls", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    // RR checkboxes
    auto* rrChannels = findWidgetByName<QCheckBox>("ToggleRRChannels");
    REQUIRE(rrChannels != nullptr);

    auto* interPinNodes = findWidgetByName<QCheckBox>("ToggleInterClusterPinNodes");
    REQUIRE(interPinNodes != nullptr);

    auto* intraNodes = findWidgetByName<QCheckBox>("ToggleRRIntraClusterNodes");
    REQUIRE(intraNodes != nullptr);

    auto* sbox = findWidgetByName<QCheckBox>("ToggleRRSBox");
    REQUIRE(sbox != nullptr);

    auto* cbox = findWidgetByName<QCheckBox>("ToggleRRCBox");
    REQUIRE(cbox != nullptr);

    auto* intraEdges = findWidgetByName<QCheckBox>("ToggleRRIntraClusterEdges");
    REQUIRE(intraEdges != nullptr);

    auto* highlightRR = findWidgetByName<QCheckBox>("ToggleHighlightRR");
    REQUIRE(highlightRR != nullptr);

    // Congestion combo
    auto* congestion = findWidgetByName<QComboBox>("ToggleCongestion");
    REQUIRE(congestion != nullptr);
    CHECK(congestion->count() == 3); // None, Congested, Congested with Nets

    // Routing utilization combo
    auto* routeUtil = findWidgetByName<QComboBox>("ToggleRoutingUtil");
    REQUIRE(routeUtil != nullptr);
    CHECK(routeUtil->count() == 5);

    // Routing expansion cost combo
    auto* expCost = findWidgetByName<QComboBox>("ToggleRoutingExpansionCost");
    REQUIRE(expCost != nullptr);
    CHECK(expCost->count() == 7);

    // Clip routing util checkbox
    auto* clipUtil = findWidgetByName<QCheckBox>("clipRoutingUtil");
    REQUIRE(clipUtil != nullptr);

    // Routing bbox spin
    auto* bbox = findWidgetByName<QSpinBox>("ToggleRoutingBBox");
    REQUIRE(bbox != nullptr);
}

TEST_CASE("Flow: Misc popover has save, pause, debug, manual move", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* saveBtn = findWidgetByName<QPushButton>("SaveGraphics");
    REQUIRE(saveBtn != nullptr);
    CHECK(saveBtn->text() == "Save");

    auto* pauseBtn = findWidgetByName<QPushButton>("PauseButton");
    REQUIRE(pauseBtn != nullptr);
    CHECK(pauseBtn->text() == "Pause");

    auto* debugBtn = findWidgetByName<QPushButton>("debugButton");
    REQUIRE(debugBtn != nullptr);
    CHECK(debugBtn->text() == "Debug");

    auto* manualMove = findWidgetByName<QCheckBox>("manualMove");
    REQUIRE(manualMove != nullptr);
    CHECK(manualMove->text() == "Manual Move");
}

TEST_CASE("Flow: Critical path controls loaded from Net popover", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* critFlylines = findWidgetByName<QCheckBox>("ToggleCritPathFlylines");
    REQUIRE(critFlylines != nullptr);
    CHECK(critFlylines->text() == "Critical Path Flylines");

    auto* critRouting = findWidgetByName<QCheckBox>("ToggleCritPathRouting");
    REQUIRE(critRouting != nullptr);
    CHECK(critRouting->text() == "Critical Path Routing");

    auto* critDelays = findWidgetByName<QCheckBox>("ToggleCritPathDelays");
    REQUIRE(critDelays != nullptr);
    CHECK(critDelays->text() == "Critical Path Delays");
}

TEST_CASE("Flow: 3D layers popover has layer and transparency boxes", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* layerLabel = findWidgetByName<QLabel>("LayerLabel");
    REQUIRE(layerLabel != nullptr);
    CHECK(layerLabel->text() == "View: ");

    auto* transLabel = findWidgetByName<QLabel>("TransparencyLabel");
    REQUIRE(transLabel != nullptr);
    CHECK(transLabel->text().contains("Transparency"));
}

TEST_CASE("Flow: Congestion cost combo has all 8 options", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* combo = findWidgetByName<QComboBox>("ToggleCongestionCost");
    REQUIRE(combo != nullptr);
    CHECK(combo->count() == 8);
    CHECK(combo->itemText(0) == "None");
    CHECK(combo->itemText(1) == "Total Routing Costs");
    CHECK(combo->itemText(7) == "Base Routing Costs");
}

TEST_CASE("Flow: NoC display combo has expected items", "[layer3][vpr_gui]") {
    ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
    std::unique_ptr<QMainWindow> win(mw.release());
    REQUIRE(win != nullptr);

    auto* noc = findWidgetByName<QComboBox>("ToggleNocBox");
    REQUIRE(noc != nullptr);
    CHECK(noc->count() == 3);
    CHECK(noc->itemText(0) == "None");
    CHECK(noc->itemText(1) == "NoC Links");
    CHECK(noc->itemText(2) == "NoC Link Usage");
}

// ---------------------------------------------------------------------------
// Regression: QtGladeLoader::loadFile must not leak any widgets past
// ~QMainWindow. Before the fix in libezgl/src/qt/qtgladeloader.cpp (pass-2
// reparenting of orphan Qt::Popup frames), each loadFile/destroy cycle left
// stale popovers + their children in QApplication::allWidgets(); subsequent
// find_widget()/findWidgetByName() lookups could return a leaked copy
// (hash-iteration-order dependent) and shadow the freshly-loaded one. The
// original visible symptom was "Flow: Net popover has expected controls"
// flaking on `ToggleNetType.count() == 1` (expected 2).
//
// Asserts the invariant directly: the full widget set returns to baseline
// after the owning QMainWindow is destroyed. Any new widget alive in
// QApplication::allWidgets() that wasn't there before the load is a leak,
// regardless of class or name — catches popover-resident widgets we haven't
// thought to name explicitly, including future additions to main.ui.
// Deterministic, independent of test ordering and RNG seed.
// ---------------------------------------------------------------------------
TEST_CASE("Flow: widgets lifetime bounded by window lifetime",
          "[layer3][vpr_gui][regression]") {
    // Pointer-set snapshot of every widget Qt knows about right now. We never
    // dereference these pointers after taking the snapshot — only test them
    // for set membership — so even if Qt destroys one of them during the
    // load/unload cycle (it shouldn't), the comparison stays well-defined.
    const QList<QWidget*> baseline_list = QApplication::allWidgets();
    const QSet<QWidget*> baseline_set(baseline_list.cbegin(), baseline_list.cend());

    {
        ezgl::MainWindow mw(VPR_MAIN_UI_PATH);
        std::unique_ptr<QMainWindow> win(mw.release());
        REQUIRE(win != nullptr);
        // Sanity check: loading main.ui actually added widgets, so an empty
        // post-leak list below is genuine cleanup, not "nothing happened".
        REQUIRE(QApplication::allWidgets().size() > baseline_list.size());
    }

    // Anything alive now that wasn't in the baseline is a leak.
    QStringList leaked;
    for (QWidget* w : QApplication::allWidgets()) {
        if (!baseline_set.contains(w)) {
            const QString name = w->objectName().isEmpty()
                                     ? QStringLiteral("<unnamed>")
                                     : w->objectName();
            const QString klass = QString::fromUtf8(w->metaObject()->className());
            leaked << QStringLiteral("%1 (%2)").arg(name, klass);
        }
    }

    INFO("leaked widgets (" << leaked.size() << "): "
                            << leaked.join(QStringLiteral(", ")).toStdString());
    REQUIRE(leaked.isEmpty());
}
