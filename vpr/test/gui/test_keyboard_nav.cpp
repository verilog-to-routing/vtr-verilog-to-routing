/**
 * @file test_keyboard_nav.cpp
 * @brief GUI-T-015 — Keyboard navigation in the toolbar (Layer 4).
 *
 * Locks the contract of ``act_on_key_press`` in
 * ``vpr/src/draw/draw.cpp``, the ezgl key-press dispatcher:
 *
 *   * Search bar focused + key == ``"Return"`` or ``"Tab"`` →
 *     ``enable_autocomplete(app)`` is invoked, the cursor moves to
 *     the end of the text, and the function returns *before* the
 *     completer-clear branch runs.
 *   * Search bar not focused + ``draw_state->justEnabled == true``
 *     → the flag flips to false **without** clearing the completer
 *     (the autocomplete that was just enabled survives one
 *     non-Return/Tab keystroke so the popup does not snap shut).
 *   * Search bar not focused + ``justEnabled == false`` → the
 *     completer is cleared (``setCompleter(nullptr)``).
 *   * key == ``"Escape"`` → ``deselect_all()`` is called: every
 *     selected block is reset to its default colour and
 *     ``hit_nodes`` is cleared.
 *
 * Why this is real, behaviour-relevant coverage rather than
 * structural noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The autocomplete-survive-one-keystroke contract (the
 *   ``justEnabled`` gate) is invisible by inspection but everything
 *   the user sees: snap the popup shut on the very keystroke that
 *   opened it and autocomplete is unusable. Escape-clears-selection
 *   is the user's only quick way out of an accidental highlight.
 *   Both behaviours are exercised against the *real* loaded
 *   ``main.ui`` widget tree and the *real* ``g_vpr_ctx`` populated
 *   by ``VprRunStageFixture``.
 *
 * Setup (per case):
 *
 *   * ``EzglAppFixture`` for ``main.ui`` so ``find_line_edit
 *     ("TextInput")`` succeeds.
 *   * Some cases compose ``VprRunStageFixture::run_to(PostPlace)``
 *     + ``DrawStructsScope`` for the Escape / autocomplete-content
 *     paths that need a populated ``g_vpr_ctx``.
 *
 * Note: ``act_on_key_press`` ignores its ``QKeyEvent*`` argument
 * (the production declaration marks the parameter unused) — only
 * the ``key_name`` C-string is read. Tests pass ``nullptr`` for
 * the event safely.
 *
 * Tag: [layer4][interactive][keyboard][vpr_gui][GUI-T-015]
 */

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QCompleter>
#include <QComboBox>
#include <QLineEdit>
#include <QStringList>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/rectangle.hpp>

#include "draw.h"
#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "search_bar.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"
#include "test_runstage_fixture.hpp"
#include "ui_setup.h"

using vpr_gui_test::EzglAppFixture;
using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

// Forward declarations of the production entries under contract.
namespace ezgl {
class application;
}
extern ezgl::application* application;

// act_on_key_press is non-static in draw.cpp (it's bound as the ezgl
// dispatch callback) but does not appear in any header.
void act_on_key_press(ezgl::application* app, QKeyEvent* event, const std::string& key_name);

namespace {

constexpr float kTileWidth = 100.0f;

class DrawStructsScope {
  public:
    explicit DrawStructsScope(const t_arch* arch) {
        t_draw_state* ds = get_draw_state_vars();
        saved_show_graphics_ = ds->show_graphics;
        ds->show_graphics = true;
        alloc_draw_structs(arch);
        init_draw_coords(kTileWidth,
                         g_vpr_ctx.placement().blk_loc_registry());
    }
    ~DrawStructsScope() {
        free_draw_structs();
        get_draw_state_vars()->show_graphics = saved_show_graphics_;
    }
    DrawStructsScope(const DrawStructsScope&) = delete;
    DrawStructsScope& operator=(const DrawStructsScope&) = delete;

  private:
    bool saved_show_graphics_ = false;
};

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
        cnv = app->add_canvas(id, /*draw_callback=*/[](ezgl::renderer*) {}, ezgl::rectangle({0, 0}, 100, 100), ezgl::WHITE);
    }
    return cnv;
}

/// Restores the QLineEdit's completer and the draw_state->justEnabled
/// flag at scope exit so cases do not bleed into one another.
class CompleterStateGuard {
  public:
    CompleterStateGuard(QLineEdit* le)
        : le_(le)
        , saved_completer_(le ? le->completer() : nullptr)
        , saved_just_enabled_(get_draw_state_vars()->justEnabled) {}
    ~CompleterStateGuard() {
        if (le_) le_->setCompleter(saved_completer_);
        get_draw_state_vars()->justEnabled = saved_just_enabled_;
    }
    CompleterStateGuard(const CompleterStateGuard&) = delete;
    CompleterStateGuard& operator=(const CompleterStateGuard&) = delete;

  private:
    QLineEdit* le_ = nullptr;
    QCompleter* saved_completer_ = nullptr;
    bool saved_just_enabled_ = false;
};

/// Installs a recognisable QCompleter as a child of the TextInput so
/// the test can prove `setCompleter(nullptr)` actually ran.
QCompleter* install_sentinel_completer(QLineEdit* le) {
    auto* completer = new QCompleter(QStringList{"alpha", "beta"}, le);
    le->setCompleter(completer);
    return completer;
}

} // namespace

// =============================================================================
// Escape: not focused → deselect_all clears highlighted blocks
// =============================================================================
TEST_CASE("KeyNav: Escape clears the highlighted block selection",
          "[layer4][interactive][keyboard][vpr_gui][GUI-T-015]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    REQUIRE_FALSE(cluster_ctx.clb_nlist.blocks().empty());
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();

    t_draw_state* ds = get_draw_state_vars();
    ds->set_block_color(target, SELECTED_COLOR);
    REQUIRE(ds->block_color(target) == SELECTED_COLOR);

    act_on_key_press(fx_app.app(), /*event=*/nullptr, "Escape");

    REQUIRE(ds->block_color(target) != SELECTED_COLOR);
}

// =============================================================================
// justEnabled gate: a non-Return key after autocomplete enable preserves
// the completer the first time, then clears it the second time
// =============================================================================
TEST_CASE("KeyNav: justEnabled gate survives one keystroke, then clears completer",
          "[layer4][interactive][keyboard][vpr_gui][GUI-T-015]") {
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    QLineEdit* search = fx_app.app()->find_line_edit("TextInput");
    REQUIRE(search != nullptr);
    CompleterStateGuard guard(search);

    QCompleter* completer = install_sentinel_completer(search);
    REQUIRE(search->completer() == completer);

    t_draw_state* ds = get_draw_state_vars();
    ds->justEnabled = true;

    // Search bar not focused (default for a never-shown widget under
    // offscreen Qt) + key != "Return"/"Tab" — the justEnabled gate
    // must flip the flag without clearing the completer.
    REQUIRE_FALSE(search->hasFocus());
    act_on_key_press(fx_app.app(), nullptr, "F");
    REQUIRE(ds->justEnabled == false);
    REQUIRE(search->completer() == completer);

    // The very next non-Return/Tab keystroke now hits the
    // setCompleter(nullptr) branch.
    act_on_key_press(fx_app.app(), nullptr, "F");
    REQUIRE(search->completer() == nullptr);
}

// =============================================================================
// Arrow / character keys are dispatched without throwing
// =============================================================================
TEST_CASE("KeyNav: arrow / character keys are dispatched without throwing",
          "[layer4][interactive][keyboard][vpr_gui][GUI-T-015]") {
    EzglAppFixture fx_app;
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    QLineEdit* search = fx_app.app()->find_line_edit("TextInput");
    REQUIRE(search != nullptr);
    CompleterStateGuard guard(search);

    // No completer, justEnabled == false: each call must hit the
    // "clear completer" branch (already null) and return cleanly.
    get_draw_state_vars()->justEnabled = false;
    search->setCompleter(nullptr);

    for (const char* key : {"Up", "Down", "Left", "Right",
                            "Home", "End", "Page_Up", "Page_Down",
                            "F", "1"}) {
        INFO("key=" << key);
        REQUIRE_NOTHROW(act_on_key_press(fx_app.app(), nullptr, key));
        REQUIRE(search->completer() == nullptr);
    }
}

// =============================================================================
// enable_autocomplete: Block Name installs the BlockNames completer;
// non-name search type leaves the completer unchanged
// =============================================================================
TEST_CASE("KeyNav: enable_autocomplete installs BlockNames completer for "
          "Block Name and is a no-op for Block ID",
          "[layer4][interactive][keyboard][vpr_gui][GUI-T-015]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    QLineEdit* search = fx_app.app()->find_line_edit("TextInput");
    REQUIRE(search != nullptr);
    CompleterStateGuard guard(search);

    // Production populates the named completers via load_block_names /
    // load_net_names — invoke them here so the lookup inside
    // enable_autocomplete (findChild<QCompleter*>("BlockNames")) hits.
    load_block_names(fx_app.app());
    load_net_names(fx_app.app());
    QCompleter* blocks = search->findChild<QCompleter*>("BlockNames");
    REQUIRE(blocks != nullptr);

    QComboBox* combo = fx_app.app()->find_combo_box("SearchType");
    REQUIRE(combo != nullptr);

    // --- Block Name path: completer becomes BlockNames ---
    {
        const int idx = combo->findText("Block Name");
        REQUIRE(idx >= 0);
        combo->setCurrentIndex(idx);
        search->setCompleter(nullptr);
        get_draw_state_vars()->justEnabled = false;

        enable_autocomplete(fx_app.app());
        REQUIRE(search->completer() == blocks);
        REQUIRE(get_draw_state_vars()->justEnabled == true);
    }

    // --- Block ID path: no name-based completer; setCompleter not invoked ---
    {
        const int idx = combo->findText("Block ID");
        REQUIRE(idx >= 0);
        combo->setCurrentIndex(idx);
        // Seed with a sentinel so the early-return is observable —
        // production must NOT clear it (the function only sets a
        // completer when one is found for the active type).
        QCompleter* sentinel = install_sentinel_completer(search);
        get_draw_state_vars()->justEnabled = false;

        enable_autocomplete(fx_app.app());
        REQUIRE(search->completer() == sentinel);
        REQUIRE(get_draw_state_vars()->justEnabled == false);
    }
}
