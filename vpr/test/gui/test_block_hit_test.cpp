/**
 * @file test_block_hit_test.cpp
 * @brief GUI-T-014 — Block hit-test on canvas click (Layer 4).
 *
 * Locks the contract of ``get_cluster_block_id_from_xy_loc(x, y)`` in
 * ``vpr/src/draw/draw.cpp``, the public hit-test entry point that the
 * left-click branch of ``act_on_mouse_press`` calls when the user
 * clicks on the canvas outside of zoom-select mode. Production:
 *
 *   * Click within a placed block's bounding box → returns that
 *     block's ``ClusterBlockId``.
 *   * Click on empty white-space / outside any block → returns
 *     ``ClusterBlockId::INVALID()``; ``highlight_blocks`` then
 *     early-exits without mutating selection (no spurious selection).
 *   * The hit-test honours per-layer visibility — clicks on a hidden
 *     layer must not surface a block from that layer.
 *
 * Why this is real, behaviour-relevant coverage rather than structural
 * noise (per §4 of :ref:`vpr_gui_test_plan`):
 *
 *   ``get_cluster_block_id_from_xy_loc`` traverses the device grid in
 *   reverse-layer order, calls ``get_absolute_clb_bbox`` for every
 *   placed block, and tests ``rectangle::contains(x, y)``. Any
 *   regression in either the traversal order, the bbox math, or the
 *   layer-visibility short-circuit is silent in production telemetry —
 *   the user just sees the wrong block highlighted (or none) and has
 *   no way to surface it as a bug. We exercise the *real* function
 *   with a real placed netlist (``and_latch.blif`` on
 *   ``k6_N10_40nm.xml``, packed + placed in-process by
 *   ``VprRunStageFixture``), not a mock.
 *
 * Setup (per-case, ~300ms each on top of the ~2s VPR run-stage):
 *
 *   1. ``VprRunStageFixture::run_to(Stage::PostPlace)`` populates
 *      ``g_vpr_ctx.placement()`` with real block locations.
 *   2. We force ``draw_state->show_graphics = true`` so
 *      ``init_draw_coords`` no longer takes its ``--disp off``
 *      early-return; then call ``alloc_draw_structs(arch)`` and
 *      ``init_draw_coords(tile_width, blk_loc_registry)`` exactly
 *      the way ``vpr_create_device`` would have under ``--disp on``.
 *      The drawing globals are restored / freed in a teardown
 *      helper at the end of each case.
 *
 * Tag: [layer4][hit-test][vpr_gui][GUI-T-014]
 */

#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <ezgl/rectangle.hpp>
#include <ezgl/point.hpp>

#include "draw.h"
#include "draw_global.h"
#include "draw_types.h"
#include "globals.h"
#include "test_runstage_fixture.hpp"
#include "vpr_types.h"

using vpr_gui_test::Stage;
using vpr_gui_test::VprRunStageFixture;

namespace {

constexpr float kTileWidth = 100.0f;

/// Sets up draw_coords + draw_state for the *current* g_vpr_ctx and
/// tears them down on destruction. Composes on top of
/// ``VprRunStageFixture::run_to(PostPlace)``: the run-stage fixture
/// owns g_vpr_ctx; this helper owns the drawing globals.
class DrawStructsScope {
  public:
    explicit DrawStructsScope(const t_arch* arch) {
        // Snapshot the show_graphics flag — production sets it via
        // init_graphics_state from VPR's command-line flags. Our
        // fixture runs with --disp off, so we must flip it on so
        // init_draw_coords does not take its early-return path.
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

/// Returns the absolute bbox of a placed block, computed via the
/// production accessor on draw_coords. Mirrors what
/// ``highlight_blocks`` does internally.
ezgl::rectangle bbox_of(ClusterBlockId blk) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    return get_draw_coords_vars()->get_absolute_clb_bbox(
        blk, cluster_ctx.clb_nlist.block_type(blk));
}

} // namespace

// =============================================================================
// Positive: every placed block is hit-testable at its bbox center
// =============================================================================
TEST_CASE("HitTest: every placed block is found at its bbox center",
          "[layer4][hit-test][vpr_gui][GUI-T-014]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx.arch());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    REQUIRE(cluster_ctx.clb_nlist.blocks().size() >= 3u);

    for (ClusterBlockId blk : cluster_ctx.clb_nlist.blocks()) {
        const ezgl::rectangle r = bbox_of(blk);
        // Sanity: bbox must be non-degenerate; otherwise the test
        // below would pass vacuously.
        REQUIRE(r.width() > 0.0);
        REQUIRE(r.height() > 0.0);

        const ezgl::point2d center{(r.left() + r.right()) * 0.5,
                                   (r.bottom() + r.top()) * 0.5};

        const ClusterBlockId hit =
            get_cluster_block_id_from_xy_loc(center.x, center.y);
        INFO("block=" << size_t(blk)
                      << " bbox=(" << r.left() << "," << r.bottom()
                      << ")-(" << r.right() << "," << r.top()
                      << ") center=(" << center.x << "," << center.y << ")"
                      << " hit=" << size_t(hit));
        // The hit-test prioritises higher layers, so on a
        // multi-layer arch the click could resolve to a block on a
        // different layer that overlaps in (x,y). On the 2-D
        // ``k6_N10_40nm.xml`` baseline used by the fixture there is
        // only one layer, so the hit must be the same block.
        REQUIRE(hit == blk);
    }
}

// =============================================================================
// Negative: clicking on empty / out-of-grid space returns INVALID
// =============================================================================
TEST_CASE("HitTest: clicks far outside the grid return INVALID "
          "(no spurious selection)",
          "[layer4][hit-test][vpr_gui][GUI-T-014]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx.arch());

    // Negative-x and negative-y are guaranteed-out for the grid, which
    // starts at (0, 0). Very large positive coords are guaranteed-out
    // for the small ``k6_N10_40nm`` device used by the fixture.
    for (auto p : std::vector<ezgl::point2d>{
             {-1000.0, -1000.0},
             {-1.0, -1.0},
             {1.0e9, 1.0e9},
             {1.0e9, -1.0e9}}) {
        INFO("click at (" << p.x << "," << p.y << ")");
        const ClusterBlockId hit =
            get_cluster_block_id_from_xy_loc(p.x, p.y);
        REQUIRE(hit == ClusterBlockId::INVALID());
    }
}

// =============================================================================
// Negative: hidden layer is invisible to the hit-test
// =============================================================================
TEST_CASE("HitTest: hidden layer hides its blocks from the hit-test",
          "[layer4][hit-test][vpr_gui][GUI-T-014]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx.arch());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    t_draw_state* ds = get_draw_state_vars();

    // Pick the first placed block and confirm it is hit-testable as a
    // baseline before flipping the layer to hidden.
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();
    const ezgl::rectangle r = bbox_of(target);
    const ezgl::point2d center{(r.left() + r.right()) * 0.5,
                               (r.bottom() + r.top()) * 0.5};

    REQUIRE(get_cluster_block_id_from_xy_loc(center.x, center.y) == target);

    // Flip every layer to hidden; the hit-test must now return
    // INVALID — the click cannot resolve to a block on any layer.
    REQUIRE_FALSE(ds->draw_layer_display.empty());
    std::vector<bool> saved_visible;
    saved_visible.reserve(ds->draw_layer_display.size());
    for (auto& d : ds->draw_layer_display) {
        saved_visible.push_back(d.visible);
        d.visible = false;
    }

    REQUIRE(get_cluster_block_id_from_xy_loc(center.x, center.y)
            == ClusterBlockId::INVALID());

    // Restore so subsequent assertions and DrawStructsScope teardown
    // see the original state.
    for (size_t i = 0; i < ds->draw_layer_display.size(); ++i) {
        ds->draw_layer_display[i].visible = saved_visible[i];
    }
    REQUIRE(get_cluster_block_id_from_xy_loc(center.x, center.y) == target);
}

// =============================================================================
// Boundary: clicks on the bbox edges resolve consistently
// =============================================================================
TEST_CASE("HitTest: clicks on a bbox interior corner hit the block",
          "[layer4][hit-test][vpr_gui][GUI-T-014]") {
    VprRunStageFixture fx;
    fx.run_to(Stage::PostPlace);
    DrawStructsScope draw_scope(&fx.arch());

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    ClusterBlockId target = *cluster_ctx.clb_nlist.blocks().begin();
    const ezgl::rectangle r = bbox_of(target);

    // Sample four well-inside-the-bbox points (a ε from each corner)
    // — guaranteed to be inside the half-open ``rectangle::contains``
    // test that production uses, regardless of which side of the
    // bbox is closed.
    const double eps = 0.25 * std::min(r.width(), r.height());
    REQUIRE(eps > 0.0);
    const std::vector<ezgl::point2d> samples{
        {r.left() + eps, r.bottom() + eps},
        {r.right() - eps, r.bottom() + eps},
        {r.left() + eps, r.top() - eps},
        {r.right() - eps, r.top() - eps},
    };
    for (const auto& p : samples) {
        INFO("interior sample at (" << p.x << "," << p.y << ")");
        REQUIRE(get_cluster_block_id_from_xy_loc(p.x, p.y) == target);
    }
}
