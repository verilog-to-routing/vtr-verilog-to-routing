/**
 * @file test_save_graphics.cpp
 * @brief GUI-T-003 — SaveGraphics dialog & writer (Layer 4).
 *
 * Locks the contract of the three public entry points in
 * ``vpr/src/draw/save_graphics.cpp``:
 *
 *   * ``save_graphics(ext, file_name)``        — strips a leading
 *     '.' from ``ext``; dispatches to
 *     ``canvas::print_pdf`` / ``print_png`` / ``print_svg`` when
 *     ``ext`` is ``pdf``/``png``/``svg``; pops a warning dialog
 *     (no file written) on any other extension.
 *   * ``save_graphics_from_button(dialog)``    — reads the text-
 *     entry + combo-box widgets named in
 *     ``save_graphics_dialog_box``; calls ``save_graphics`` with
 *     their current values.
 *   * ``save_graphics_dialog_box(widget, app)``— constructs the
 *     modal save dialog (``"Save Graphics Contents"``) with a
 *     line-edit, combo box, and Save/Cancel button-box.
 *
 * Why this is real, behaviour-relevant coverage rather than
 * structural noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The user pushes one button and gets a file with the chosen
 *   extension, or no file plus a warning. A regression in the
 *   extension-strip step, the file-format dispatch, or the dialog
 *   widget naming silently breaks export — there is no telemetry
 *   that surfaces it. We exercise the *real* function with a real
 *   ``ezgl::canvas`` + ``QDialog``, written into a real per-case
 *   tmp directory whose contents we assert are non-empty after a
 *   successful save and absent after an invalid-extension call.
 *
 * Setup (per case):
 *
 *   * ``EzglAppFixture`` provides ``main.ui`` so the warning
 *     dialog's ``find_widget(MainWindow)`` lookup succeeds.
 *   * A canvas is registered with a no-op draw callback so
 *     ``render_to_image`` has a backend (the lazy-init in
 *     ``canvas::render_to_image`` falls back from rhi to
 *     immediate via ``probe_rhi`` under offscreen Qt).
 *   * ``DrawApplicationScope`` points draw.cpp's file-scope
 *     ``application`` at ``test_app()`` so ``save_graphics``
 *     reaches a live app.
 *   * Each case creates / cleans its own
 *     ``/tmp/vpr_gui_t003_<pid>_<n>`` directory.
 *
 * Tag: [layer4][interactive][save-graphics][vpr_gui][GUI-T-003]
 */

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <string>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QWidget>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/rectangle.hpp>

#include "save_graphics.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"

using vpr_gui_test::EzglAppFixture;

namespace ezgl {
class application;
}
extern ezgl::application* application;

namespace {

namespace fs = std::filesystem;

class DrawApplicationScope {
  public:
    explicit DrawApplicationScope(ezgl::application* app)
        : saved_(application) { application = app; }
    ~DrawApplicationScope() { application = saved_; }
    DrawApplicationScope(const DrawApplicationScope&) = delete;
    DrawApplicationScope& operator=(const DrawApplicationScope&) = delete;

  private:
    ezgl::application* saved_ = nullptr;
};

ezgl::canvas* ensure_main_canvas(ezgl::application* app) {
    const std::string id = app->get_main_canvas_id();
    ezgl::canvas* cnv = app->get_canvas(id);
    if (!cnv) {
        // Non-null draw callback: immediate_backend::render_to_image
        // dereferences m_draw_callback during print_*. A no-op is
        // sufficient — we only need a writable QImage surface.
        cnv = app->add_canvas(
            id,
            /*draw_callback=*/[](ezgl::renderer*) {},
            ezgl::rectangle({0, 0}, 256, 256),
            ezgl::WHITE);
    }
    return cnv;
}

class TmpDir {
  public:
    TmpDir() {
        static std::atomic<unsigned> seq{0};
        path_ = fs::temp_directory_path()
                / ("vpr_gui_t003_" + std::to_string(::getpid()) + "_"
                   + std::to_string(seq.fetch_add(1)));
        fs::create_directories(path_);
    }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    TmpDir(const TmpDir&) = delete;
    TmpDir& operator=(const TmpDir&) = delete;

    std::string file(const std::string& base) const {
        return (path_ / base).string();
    }
    const fs::path& path() const { return path_; }

  private:
    fs::path path_;
};

// On GitHub Actions runners (and other minimal Linux containers) the
// offscreen Qt platform plug-in cannot create a QRhiGles2 context
// because Mesa's libgl1-mesa-dri lacks a working GLX/EGL backend. The
// failure manifests as ``QRhiGles2: Failed to create context`` followed
// by a SIGSEGV inside ``canvas::render_to_image`` — not a throw, so it
// cannot be caught. We therefore skip the tests that exercise the real
// PNG/PDF/SVG writer when running under CI. Locally (developer
// workstations and lab machines with a full Mesa install) the tests
// still execute end-to-end. Tracked alongside DEF-016 / GUI-T-006.
bool ci_render_unavailable() {
    const char* ci = std::getenv("CI");
    return ci && std::string(ci) == "true";
}

} // namespace

// =============================================================================
// Positive: png / pdf / svg dispatch each write a non-empty file
// =============================================================================
TEST_CASE("SaveGraphics: png/pdf/svg extensions each write a non-empty file",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    if (ci_render_unavailable()) {
        SKIP("CI offscreen Mesa lacks a working GL context for QRhiGles2");
    }
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());
    TmpDir tmp;

    for (const std::string ext : {"png", "pdf", "svg"}) {
        const std::string base = tmp.file("vpr_t003_" + ext);
        // ``save_graphics`` appends "." + extension to file_name internally,
        // so we pass the bare base.
        save_graphics(ext, base);

        const fs::path expected = base + "." + ext;
        INFO("ext=" << ext << " expected=" << expected.string());
        REQUIRE(fs::exists(expected));
        REQUIRE(fs::file_size(expected) > 0);
    }
}

// =============================================================================
// Positive: leading-dot extension is stripped ("png" and ".png" equivalent)
// =============================================================================
TEST_CASE("SaveGraphics: leading-dot extension is stripped before dispatch",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    if (ci_render_unavailable()) {
        SKIP("CI offscreen Mesa lacks a working GL context for QRhiGles2");
    }
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());
    TmpDir tmp;

    const std::string base = tmp.file("vpr_t003_dotted");
    save_graphics(".png", base);
    REQUIRE(fs::exists(base + ".png"));
    REQUIRE(fs::file_size(base + ".png") > 0);
}

// =============================================================================
// Negative: invalid extension writes no file (warning_dialog branch)
// =============================================================================
TEST_CASE("SaveGraphics: invalid extension writes no file",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());
    TmpDir tmp;

    const std::string base = tmp.file("vpr_t003_bogus");
    save_graphics("xyz", base);

    // No file with any of the four extensions production knows about
    // should have been created, nor one with the invalid ext itself.
    for (const std::string ext : {"xyz", "png", "pdf", "svg"}) {
        const fs::path candidate = base + "." + ext;
        INFO("must-not-exist=" << candidate.string());
        REQUIRE_FALSE(fs::exists(candidate));
    }
    // The directory itself must still exist (dir was not deleted by
    // the warning path).
    REQUIRE(fs::is_directory(tmp.path()));
}

// =============================================================================
// save_graphics_from_button: dispatches using widgets named by the dialog
// =============================================================================
TEST_CASE("SaveGraphics: save_graphics_from_button reads named widgets and writes",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    if (ci_render_unavailable()) {
        SKIP("CI offscreen Mesa lacks a working GL context for QRhiGles2");
    }
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());
    TmpDir tmp;

    const std::string base = tmp.file("vpr_t003_button");

    // Build a synthetic dialog with the production widget names, mirror
    // of save_graphics_dialog_box. Owned by us so we control teardown.
    QDialog dialog;
    auto* text_entry = new QLineEdit(&dialog);
    auto* combo_box = new QComboBox(&dialog);
    text_entry->setObjectName("file_name_text_entry");
    combo_box->setObjectName("file_name_combo_box");
    combo_box->addItem("pdf");
    combo_box->addItem("png");
    combo_box->addItem("svg");

    text_entry->setText(QString::fromStdString(base));
    combo_box->setCurrentIndex(1); // png

    save_graphics_from_button(&dialog);

    REQUIRE(fs::exists(base + ".png"));
    REQUIRE(fs::file_size(base + ".png") > 0);
}

// =============================================================================
// save_graphics_from_button: missing widgets is a no-op (defensive guard)
// =============================================================================
TEST_CASE("SaveGraphics: save_graphics_from_button is a no-op when widgets missing",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());
    TmpDir tmp;

    QDialog dialog; // no named children
    save_graphics_from_button(&dialog);

    // Nothing was created; if the guard regressed, an early QLineEdit
    // null deref would crash this case.
    REQUIRE(fs::is_empty(tmp.path()));
}

// =============================================================================
// save_graphics_dialog_box: constructs a dialog with the contracted shape
// =============================================================================
TEST_CASE("SaveGraphics: dialog_box builds a Save/Cancel dialog with named widgets",
          "[layer4][interactive][save-graphics][vpr_gui][GUI-T-003]") {
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    save_graphics_dialog_box(/*widget=*/nullptr, fx_app.app());

    // Find the dialog by title — it is parented to MainWindow and
    // shown via show() (modeless). We don't accept/reject it (the
    // accept slot would dispatch save_graphics which we already
    // covered above); instead we close + delete it deterministically.
    QDialog* dialog = nullptr;
    for (QWidget* w : QApplication::allWidgets()) {
        if (auto* d = qobject_cast<QDialog*>(w)) {
            if (d->windowTitle() == "Save Graphics Contents") {
                dialog = d;
                break;
            }
        }
    }
    REQUIRE(dialog != nullptr);

    QLineEdit* text_entry = dialog->findChild<QLineEdit*>("file_name_text_entry");
    QComboBox* combo_box = dialog->findChild<QComboBox*>("file_name_combo_box");
    REQUIRE(text_entry != nullptr);
    REQUIRE(combo_box != nullptr);

    // Production defaults locked: "vpr_graphics" and combo at index 0
    // (= "pdf"). Combo holds exactly the three supported formats.
    REQUIRE(text_entry->text() == "vpr_graphics");
    REQUIRE(combo_box->count() == 3);
    REQUIRE(combo_box->itemText(0) == "pdf");
    REQUIRE(combo_box->itemText(1) == "png");
    REQUIRE(combo_box->itemText(2) == "svg");
    REQUIRE(combo_box->currentIndex() == 0);

    // The Save/Cancel button-box must exist (the connect() lines bind
    // to its accepted/rejected signals).
    QDialogButtonBox* bbox = dialog->findChild<QDialogButtonBox*>();
    REQUIRE(bbox != nullptr);
    REQUIRE(bbox->button(QDialogButtonBox::Save) != nullptr);
    REQUIRE(bbox->button(QDialogButtonBox::Cancel) != nullptr);

    // WA_DeleteOnClose was set; close() releases the dialog without
    // bleeding into subsequent cases.
    dialog->close();
}
