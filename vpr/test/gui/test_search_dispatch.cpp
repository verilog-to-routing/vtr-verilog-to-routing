/**
 * @file test_search_dispatch.cpp
 * @brief GUI-T-001 — Search dispatcher behaviour (Layer 4).
 *
 * Locks the contract of ``search_and_highlight`` in
 * ``vpr/src/draw/search_bar.cpp``, the slot bound to the toolbar
 * "Search" button. The function reads two widgets from ``main.ui``
 * (``SearchType`` combo + ``TextInput`` line-edit) and dispatches to
 * one of the highlight primitives:
 *
 *   * ``Block ID``   → ``highlight_cluster_block(ClusterBlockId)``
 *   * ``Block Name`` → atom lookup → ``highlight_atom_block`` or
 *                      ``highlight_cluster_block``
 *   * ``Net ID``     → ``highlight_nets(ClusterNetId)`` (no-op until
 *                      routing exists)
 *   * ``Net Name``   → ``highlight_nets`` after netlist lookup
 *   * ``RR Node ID`` → ``highlight_rr_nodes`` + ``auto_zoom_rr_node``
 *
 * Why this is real, behaviour-relevant coverage rather than structural
 * noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The dispatcher fans out into five distinct production code paths,
 *   each of which mutates ``t_draw_state`` in observable ways
 *   (``block_color``, ``net_color``). A regression in any branch —
 *   wrong validity check, wrong cast, swapped lookup — shows up only
 *   as the user staring at the canvas and seeing the wrong thing
 *   highlighted (or nothing). We exercise the *real* function with
 *   the *real* loaded ``main.ui`` widgets and the *real* placed
 *   netlist (``and_latch.blif`` on ``k6_N10_40nm.xml``), not a mock.
 *
 * Setup (per case):
 *
 *   1. ``EzglAppFixture`` loads ``main.ui`` so ``SearchType`` /
 *      ``TextInput`` / ``MainWindow`` are findable.
 *   2. ``VprRunStageFixture::run_to(stage)`` populates
 *      ``g_vpr_ctx`` (PostPlace for block paths; PostRoute for the
 *      Net ID + routing-present case).
 *   3. ``DrawStructsScope`` sets ``draw_state->show_graphics = true``
 *      and runs ``alloc_draw_structs`` + ``init_draw_coords`` so
 *      ``draw_state->net_color`` /
 *      ``get_graphics_blk_loc_registry_ref`` are populated.
 *   4. ``DrawApplicationScope`` points draw.cpp's file-scope
 *      ``application`` at ``test_app()`` so the highlight primitives'
 *      ``application->update_message`` /
 *      ``application->refresh_drawing`` calls reach a live app.
 *   5. ``ensure_main_canvas`` registers a canvas so
 *      ``refresh_drawing`` does not deref a null pointer.
 *
 * Tag: [layer4][interactive][search][vpr_gui][GUI-T-001]
 */

#include <catch2/catch_test_macros.hpp>

#include <QComboBox>
#include <QLineEdit>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/rectangle.hpp>

#include "draw.h"
#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "intra_logic_block.h"
#include "search_bar.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"
#include "test_runstage_fixture.hpp"

using vpr_gui_test::EzglAppFixture;
using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

// File-scope application pointer in vpr/src/draw/draw.cpp; the highlight
// primitives reach into it for ``update_message`` and ``refresh_drawing``.
namespace ezgl {
class application;
}
extern ezgl::application* application;

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

/// Sets draw.cpp's file-scope ``application`` pointer for the duration
/// of the test, restoring it on destruction. Without this the highlight
/// primitives crash on ``application->update_message`` because the test
/// process never runs ``init_graphics_state`` (it is gated on
/// ``--disp on`` / ``--save_graphics`` in ``vpr_init_graphics``).
class DrawApplicationScope {
  public:
    explicit DrawApplicationScope(ezgl::application* app)
        : saved_(application) {
        application = app;
    }
    ~DrawApplicationScope() { application = saved_; }
    DrawApplicationScope(const DrawApplicationScope&) = delete;
    DrawApplicationScope& operator=(const DrawApplicationScope&) = delete;

  private:
    ezgl::application* saved_ = nullptr;
};

/// Registers a main canvas so ``application::refresh_drawing()`` (called
/// inside ``highlight_cluster_block`` and ``highlight_nets``) does not
/// deref a null ``canvas*``. Mirrors the helper in test_zoom_select.cpp.
ezgl::canvas* ensure_main_canvas(ezgl::application* app) {
    const std::string id = app->get_main_canvas_id();
    ezgl::canvas* cnv = app->get_canvas(id);
    if (!cnv) {
        // No-op lambda rather than nullptr: the canvas map is process-global,
        // and a null draw_callback would persist into later tests (e.g.
        // save-graphics) whose canvas::render_to_image path calls the
        // callback unconditionally and would segfault on the null call.
        cnv = app->add_canvas(id, /*draw_callback=*/[](ezgl::renderer*) {}, ezgl::rectangle({0, 0}, 100, 100), ezgl::WHITE);
    }
    return cnv;
}

/// Drives the SearchType combo to a chosen production-supported value
/// and the TextInput line-edit to a chosen string, both addressed by
/// objectName the same way ``search_and_highlight`` does. Returns the
/// (possibly null) widgets so callers can REQUIRE on them.
struct SearchInputs {
    QComboBox* combo;
    QLineEdit* text_entry;
};

SearchInputs set_search(ezgl::application* app,
                        const QString& search_type,
                        const QString& text) {
    QComboBox* combo = app->find_combo_box("SearchType");
    QLineEdit* entry = app->find_line_edit("TextInput");
    REQUIRE(combo != nullptr);
    REQUIRE(entry != nullptr);

    // The combo was loaded from main.ui with the production item list
    // (Block ID, Block Name, Net ID, Net Name, RR Node ID). findText
    // returns -1 if our target is not in the list — fail loudly so a
    // future main.ui rename surfaces here, not in a confusing later
    // assertion.
    const int idx = combo->findText(search_type);
    REQUIRE(idx >= 0);
    combo->setCurrentIndex(idx);
    entry->setText(text);
    return SearchInputs{combo, entry};
}

/// Returns true iff some block in the clustering currently carries
/// ``SELECTED_COLOR`` in ``draw_state->block_color``.
bool any_block_selected() {
    t_draw_state* ds = get_draw_state_vars();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    for (ClusterBlockId blk : cluster_ctx.clb_nlist.blocks()) {
        if (ds->block_color(blk) == SELECTED_COLOR) {
            return true;
        }
    }
    return false;
}

} // namespace

// =============================================================================
// Block ID — valid path: dispatch highlights the targeted cluster block
// =============================================================================
TEST_CASE("Search: Block ID with a valid id selects that cluster block",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    REQUIRE(!cluster_ctx.clb_nlist.blocks().empty());
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();

    set_search(fx_app.app(), "Block ID", QString::number(size_t(target)));

    REQUIRE_FALSE(any_block_selected()); // pre-condition

    search_and_highlight(/*widget=*/nullptr, fx_app.app());

    // Same observable states as the Block Name path: either
    // block_color SELECTED or a sub-pb selected inside `target`.
    const bool block_selected =
        get_draw_state_vars()->block_color(target) == SELECTED_COLOR;
    const auto& sel = get_selected_sub_block_info();
    const bool sub_selected =
        sel.has_selection() && sel.get_containing_block() == target;
    INFO("target=" << size_t(target)
                   << " block_selected=" << block_selected
                   << " sub_selected=" << sub_selected);
    REQUIRE((block_selected || sub_selected));
}

// =============================================================================
// Block ID — invalid path: dispatch deselects, no spurious selection
// =============================================================================
TEST_CASE("Search: Block ID with an out-of-range id deselects, "
          "leaves no block selected",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    // First do a successful select so the de-select side of the
    // dispatcher (``deselect_all`` at the top of search_and_highlight)
    // has a non-trivial state to clear.
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();
    set_search(fx_app.app(), "Block ID", QString::number(size_t(target)));
    search_and_highlight(nullptr, fx_app.app());
    // Either a block or a sub-pb is now selected.
    REQUIRE((any_block_selected()
             || get_selected_sub_block_info().has_selection()));

    // Now the failing search; the dispatcher must reach the
    // ``warning_dialog_box("Invalid Block ID")`` branch and leave the
    // draw_state in a clean (no SELECTED, no sub-block) state.
    set_search(fx_app.app(), "Block ID", "999999");
    search_and_highlight(nullptr, fx_app.app());
    REQUIRE_FALSE(any_block_selected());
    REQUIRE_FALSE(get_selected_sub_block_info().has_selection());
}

// =============================================================================
// Block Name — valid: name lookup resolves to a cluster block
// =============================================================================
TEST_CASE("Search: Block Name with a real cluster name selects a cluster block",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& atom_ctx = g_vpr_ctx.atom();
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();
    const std::string name = cluster_ctx.clb_nlist.block_name(target);
    REQUIRE_FALSE(name.empty());

    // Resolve the *expected* selected block the same way the
    // dispatcher does: prefer the atom→clb mapping when the atom
    // exists, otherwise fall back to the cluster netlist's
    // find_block. The cluster name and an atom of the same name can
    // map to different cluster blocks, so the fixture predicts the
    // dispatcher's choice rather than assuming both yield the same
    // ClusterBlockId.
    ClusterBlockId expected = ClusterBlockId::INVALID();
    AtomBlockId atom_blk = atom_ctx.netlist().find_block(name);
    if (atom_blk != AtomBlockId::INVALID()) {
        expected = atom_ctx.lookup().atom_clb(atom_blk);
    }
    if (expected == ClusterBlockId::INVALID()) {
        expected = cluster_ctx.clb_nlist.find_block(name);
    }
    REQUIRE(expected != ClusterBlockId::INVALID());

    set_search(fx_app.app(), "Block Name", QString::fromStdString(name));
    search_and_highlight(nullptr, fx_app.app());

    // The dispatcher's atom-first lookup ends in one of three observable
    // states, depending on whether highlight_sub_block found a sub-pb
    // at the (degenerate, default-constructed) click point:
    //   (a) highlight_atom_block returned true and selected a sub-pb
    //       in `expected` — block_color is left default.
    //   (b) highlight_atom_block returned false → fallback to
    //       highlight_cluster_block, which also calls highlight_sub_block
    //       and may select a sub-pb in `expected` — same as (a).
    //   (c) highlight_sub_block missed — draw_highlight_blocks_color
    //       paints `expected` with SELECTED_COLOR.
    // All three are valid resolutions of the search; the test contract
    // is that the *expected* cluster block was identified by the
    // dispatcher (sub-block containing-block matches OR block_color
    // SELECTED). A regression that loses the netlist lookup entirely
    // would leave both observables unset.
    const bool block_selected =
        get_draw_state_vars()->block_color(expected) == SELECTED_COLOR;
    const auto& sel = get_selected_sub_block_info();
    const bool sub_selected =
        sel.has_selection() && sel.get_containing_block() == expected;

    INFO("expected=" << size_t(expected)
                     << " block_selected=" << block_selected
                     << " sub_selected=" << sub_selected);
    REQUIRE((block_selected || sub_selected));
}

// =============================================================================
// Block Name — invalid: lookup misses both atom and cluster, no selection
// =============================================================================
TEST_CASE("Search: Block Name with an unknown name leaves no block selected",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    set_search(fx_app.app(), "Block Name",
               "__definitely_not_a_block_name_in_and_latch__");
    search_and_highlight(nullptr, fx_app.app());

    REQUIRE_FALSE(any_block_selected());
    REQUIRE_FALSE(get_selected_sub_block_info().has_selection());
}

// =============================================================================
// Net ID — pre-route: highlight_nets must early-return on empty route_trees
// =============================================================================
TEST_CASE("Search: Net ID before routing is a no-op "
          "(highlight_nets early-returns when route_trees is empty)",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    REQUIRE(!cluster_ctx.clb_nlist.nets().empty());
    ClusterNetId target = *cluster_ctx.clb_nlist.nets().begin();

    t_draw_state* ds = get_draw_state_vars();
    REQUIRE(g_vpr_ctx.routing().route_trees.empty()); // pre-condition: not routed

    // Snapshot net_color at the target before dispatch so we lock the
    // exact "no mutation" contract: ``highlight_nets(ClusterNetId)``
    // returns early on empty route_trees without writing net_color.
    const ezgl::color before = ds->net_color[ParentNetId(size_t(target))];

    set_search(fx_app.app(), "Net ID", QString::number(size_t(target)));
    search_and_highlight(nullptr, fx_app.app());

    REQUIRE(ds->net_color[ParentNetId(size_t(target))] == before);
    // And it certainly must not have been painted MAGENTA (the
    // post-route highlight colour) — that would be a leak of the
    // routed-net path into the unrouted state.
    REQUIRE_FALSE(ds->net_color[ParentNetId(size_t(target))] == ezgl::MAGENTA);
}

// =============================================================================
// Net ID — post-route: highlight_nets paints the chosen cluster net
// =============================================================================
TEST_CASE("Search: Net ID after routing colours that net MAGENTA",
          "[layer4][interactive][search][vpr_gui][GUI-T-001]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    REQUIRE_FALSE(g_vpr_ctx.routing().route_trees.empty());

    // Pick the first net that actually has a route tree — pre-routing
    // some nets may have been absorbed.
    ClusterNetId target = ClusterNetId::INVALID();
    for (ClusterNetId net : cluster_ctx.clb_nlist.nets()) {
        if (g_vpr_ctx.routing().route_trees[ParentNetId(size_t(net))].has_value()) {
            target = net;
            break;
        }
    }
    REQUIRE(target != ClusterNetId::INVALID());

    set_search(fx_app.app(), "Net ID", QString::number(size_t(target)));
    search_and_highlight(nullptr, fx_app.app());

    REQUIRE(get_draw_state_vars()->net_color[ParentNetId(size_t(target))]
            == ezgl::MAGENTA);
}
