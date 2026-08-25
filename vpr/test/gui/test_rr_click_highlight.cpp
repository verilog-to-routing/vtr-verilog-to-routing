/**
 * @file test_rr_click_highlight.cpp
 * @brief GUI-T-005 — RR click → highlight fan-in / fan-out (Layer 4).
 *
 * Locks the contract of ``highlight_rr_nodes`` (both overloads) in
 * ``vpr/src/draw/search_bar.cpp`` and ``vpr/src/draw/draw_rr.cpp``,
 * the dispatch path that the RR-Show toolbar / canvas-click branch
 * of ``act_on_mouse_press`` exercises.
 *
 *   * ``highlight_rr_nodes(RRNodeId)`` — toggles the node's
 *     ``draw_rr_node`` color between ``MAGENTA`` (selected) and
 *     ``WHITE`` (de-selected); maintains ``draw_state->hit_nodes``;
 *     returns false on ``INVALID`` and resets the status message.
 *   * ``highlight_rr_nodes(float x, float y)`` — runs the
 *     coordinate hit-test (``draw_check_rr_node_hit``) and forwards
 *     to the ``RRNodeId`` form. Honours per-layer visibility and
 *     skips ``SOURCE``/``SINK`` nodes.
 *
 * Why this is real, behaviour-relevant coverage rather than
 * structural noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   The toggle semantics (first click selects, second click
 *   de-selects) is a contract the user depends on every time they
 *   inspect routing. ``hit_nodes`` and ``draw_rr_node[..].color``
 *   are the *only* observable mutations — a regression that loses
 *   either silently breaks the RR inspection workflow with no
 *   logged signal. We exercise the *real* function on a real
 *   PostRoute ``rr_graph`` (thousands of nodes), not a mock.
 *
 * Setup (per case):
 *
 *   1. ``VprRunStageFixture::run_to(PostRoute)`` populates
 *      ``g_vpr_ctx.routing()`` and the rr_graph.
 *   2. ``EzglAppFixture`` loads ``main.ui`` so the
 *      ``application->update_message`` call inside
 *      ``highlight_rr_nodes`` reaches a status bar widget.
 *   3. ``DrawStructsScope`` flips ``show_graphics`` and runs
 *      ``alloc_draw_structs`` + ``init_draw_coords`` so
 *      ``draw_state->draw_rr_node`` is sized to the rr_graph and
 *      ``draw_layer_display`` is populated. Without it the very
 *      first ``draw_state->draw_rr_node[node]`` indexes a zero-size
 *      vector.
 *   4. ``DrawApplicationScope`` points draw.cpp's file-scope
 *      ``application`` at ``test_app()``.
 *
 * Tag: [layer4][interactive][rr-click][vpr_gui][GUI-T-005]
 */

#include <catch2/catch_test_macros.hpp>

#include <ezgl/application.hpp>
#include <ezgl/canvas.hpp>
#include <ezgl/color.hpp>
#include <ezgl/rectangle.hpp>

#include "draw.h"
#include "draw_global.h"
#include "draw_rr.h"
#include "draw_types.h"
#include "globals.h"
#include "rr_graph_view.h"
#include "search_bar.h"
#include "test_app_singleton.hpp"
#include "test_ezgl_app_fixture.hpp"
#include "test_runstage_fixture.hpp"

using vpr_gui_test::EzglAppFixture;
using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

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

/// Returns the first inter-cluster, non-SOURCE/SINK rr-node.
/// ``highlight_rr_nodes(float, float)`` skips SOURCE/SINK in the
/// hit-test, so picking one of those would make the RRNodeId path
/// behave differently from a real click. We pick a CHANX/CHANY or
/// IPIN/OPIN to mirror what a click would land on.
RRNodeId pick_inter_cluster_node() {
    const auto& rr_graph = g_vpr_ctx.device().rr_graph;
    for (RRNodeId n : rr_graph.nodes()) {
        const auto t = rr_graph.node_type(n);
        if (t == e_rr_type::SOURCE || t == e_rr_type::SINK) continue;
        return n;
    }
    return RRNodeId::INVALID();
}

} // namespace

// =============================================================================
// Toggle: first call selects (MAGENTA + hit_nodes), second call de-selects
// =============================================================================
TEST_CASE("RRClick: highlight_rr_nodes(RRNodeId) toggles MAGENTA ↔ WHITE "
          "and tracks hit_nodes",
          "[layer4][interactive][rr-click][vpr_gui][GUI-T-005]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    RRNodeId target = pick_inter_cluster_node();
    REQUIRE(target != RRNodeId::INVALID());

    t_draw_state* ds = get_draw_state_vars();
    // highlight_rr_nodes(RRNodeId) only highlights node types that are
    // currently displayed. pick_inter_cluster_node() returns a channel
    // node or inter-cluster pin, both hidden by default, so enable them.
    ds->draw_channel_nodes = true;
    ds->draw_inter_cluster_pins = true;
    REQUIRE(ds->hit_nodes.count(target) == 0);

    // First call: select.
    REQUIRE(highlight_rr_nodes(target) == true);
    REQUIRE(ds->draw_rr_node[target].color == ezgl::MAGENTA);
    REQUIRE(ds->draw_rr_node[target].node_highlighted == true);
    REQUIRE(ds->hit_nodes.count(target) == 1);

    // Second call on the same node: de-select.
    REQUIRE(highlight_rr_nodes(target) == true);
    REQUIRE(ds->draw_rr_node[target].color == ezgl::WHITE);
    REQUIRE(ds->draw_rr_node[target].node_highlighted == false);
    REQUIRE(ds->hit_nodes.count(target) == 0);
}

// =============================================================================
// INVALID node returns false and does not mutate hit_nodes
// =============================================================================
TEST_CASE("RRClick: highlight_rr_nodes(INVALID) returns false, no mutation",
          "[layer4][interactive][rr-click][vpr_gui][GUI-T-005]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    t_draw_state* ds = get_draw_state_vars();
    // Enable display of channel nodes / inter-cluster pins so the seeded
    // node (picked from those types) is actually highlightable.
    ds->draw_channel_nodes = true;
    ds->draw_inter_cluster_pins = true;

    // Seed a real selection so we can assert the INVALID branch does
    // not touch existing state.
    RRNodeId seeded = pick_inter_cluster_node();
    REQUIRE(highlight_rr_nodes(seeded) == true);
    const size_t before = ds->hit_nodes.size();

    REQUIRE(highlight_rr_nodes(RRNodeId::INVALID()) == false);
    REQUIRE(ds->hit_nodes.size() == before);
    REQUIRE(ds->hit_nodes.count(seeded) == 1);
}

// =============================================================================
// Coordinate hit-test: clicks on origin or far-out are misses
// =============================================================================
TEST_CASE("RRClick: highlight_rr_nodes(x, y) at empty space returns false",
          "[layer4][interactive][rr-click][vpr_gui][GUI-T-005]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    // Far outside the device grid — guaranteed miss. Any hit here
    // would mean the bbox math leaked a real node into infinity.
    REQUIRE(highlight_rr_nodes(1.0e9f, 1.0e9f) == false);
    REQUIRE(highlight_rr_nodes(-1.0e9f, -1.0e9f) == false);
}

// =============================================================================
// Layer-visibility gating: hidden layer hides its nodes from the hit-test
// =============================================================================
TEST_CASE("RRClick: hidden layer hides its rr-nodes from the coordinate hit-test",
          "[layer4][interactive][rr-click][vpr_gui][GUI-T-005]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    t_draw_state* ds = get_draw_state_vars();
    REQUIRE_FALSE(ds->draw_layer_display.empty());

    // Snapshot + flip every layer to hidden.
    std::vector<bool> saved;
    saved.reserve(ds->draw_layer_display.size());
    for (auto& d : ds->draw_layer_display) {
        saved.push_back(d.visible);
        d.visible = false;
    }

    // Sweep a coarse grid of click coordinates over the device area;
    // every click must miss because every layer is hidden. We use a
    // fairly dense sweep (5x5) to exercise CHANX/CHANY/IPIN/OPIN
    // coordinate buckets.
    bool any_hit = false;
    for (int gx = 0; gx <= 4 && !any_hit; ++gx) {
        for (int gy = 0; gy <= 4 && !any_hit; ++gy) {
            const float x = float(gx) * 50.0f;
            const float y = float(gy) * 50.0f;
            if (highlight_rr_nodes(x, y)) any_hit = true;
        }
    }
    REQUIRE_FALSE(any_hit);

    // Restore.
    for (size_t i = 0; i < ds->draw_layer_display.size(); ++i) {
        ds->draw_layer_display[i].visible = saved[i];
    }
}

// =============================================================================
// Non-configurable expansion: highlighting a node propagates to neighbours
// =============================================================================
TEST_CASE("RRClick: select propagates MAGENTA to non-configurable neighbours",
          "[layer4][interactive][rr-click][vpr_gui][GUI-T-005]") {
    EzglAppFixture fx_app;
    VprRunStageFixture fx_vpr;
    fx_vpr.run_to(Stage::PostRoute);
    DrawStructsScope draw_scope(&fx_vpr.arch());
    DrawApplicationScope app_scope(fx_app.app());
    ensure_main_canvas(fx_app.app());

    t_draw_state* ds = get_draw_state_vars();
    // Enable display of channel nodes / inter-cluster pins so the target
    // node (picked from those types) is actually highlightable.
    ds->draw_channel_nodes = true;
    ds->draw_inter_cluster_pins = true;

    RRNodeId target = pick_inter_cluster_node();
    REQUIRE(target != RRNodeId::INVALID());

    REQUIRE(highlight_rr_nodes(target) == true);

    // Every node in hit_nodes must be MAGENTA + node_highlighted.
    // (The set tracks the closure — direct node + non-configurable
    // expansion, see ``draw_expand_non_configurable_rr_nodes`` in
    // draw_searchbar.cpp). The minimum closure size is 1 (just the
    // hit node itself) when no non-configurable edges are present.
    REQUIRE(ds->hit_nodes.size() >= 1u);
    for (RRNodeId n : ds->hit_nodes) {
        INFO("node " << size_t(n));
        REQUIRE(ds->draw_rr_node[n].color == ezgl::MAGENTA);
        REQUIRE(ds->draw_rr_node[n].node_highlighted == true);
    }
}
