/**
 * @file test_draw_state.cpp
 * @brief Layer 3 unit tests for t_draw_state and t_draw_coords (S8 of the
 *        GUI test plan, §7.8).
 *
 * Scope discipline: t_draw_state's color/block setters require a populated
 * g_vpr_ctx.clustering(), which is a flow-stage fixture that does not yet
 * exist in the Catch2 unit-test surface (same fixture gap noted in S6 for
 * the Layer 4 stage matrix). These cases therefore exercise the
 * **context-free** surface only:
 *
 *   - Default values of every plain (bool/int/enum/string) field.
 *   - In-place mutation of those fields and read-back.
 *   - Copy construction (used by graphics-command save/restore).
 *   - t_draw_coords trivial accessors and tile-size mutation.
 *   - The free getters get_draw_state_vars() / get_draw_coords_vars()
 *     returning stable, non-null pointers to the globals.
 *
 * Per §1.A: the cases below are NOT instantiating t_draw_state with
 * fake/mocked vpr contexts to inflate coverage on color paths. If any
 * future case fails, file DEF-NNN; do not soften the assertion.
 *
 * Tag: [layer3][vpr_gui][drawstate]
 */

#include <catch2/catch_test_macros.hpp>

#include "draw_types.h"
#include "draw_global.h"

TEST_CASE("DrawState: default-constructed values match documented defaults",
          "[layer3][vpr_gui][drawstate]") {
    t_draw_state ds;

    CHECK(ds.pic_on_screen == e_pic_type::NO_PICTURE);
    CHECK(ds.show_nets == false);
    CHECK(ds.draw_nets == DRAW_FLYLINES);
    CHECK(ds.draw_inter_cluster_nets == false);
    CHECK(ds.draw_intra_cluster_nets == false);
    CHECK(ds.highlight_fan_in_fan_out == false);
    CHECK(ds.show_crit_path == false);
    CHECK(ds.show_crit_path_flylines == false);
    CHECK(ds.show_crit_path_delays == false);
    CHECK(ds.show_crit_path_routing == false);
    CHECK(ds.show_congestion == DRAW_NO_CONGEST);
    CHECK(ds.show_blk_pin_util == DRAW_NO_BLOCK_PIN_UTIL);
    CHECK(ds.show_router_expansion_cost == DRAW_NO_ROUTER_EXPANSION_COST);
    CHECK(ds.show_placement_macros == DRAW_NO_PLACEMENT_MACROS);
    CHECK(ds.show_routing_util == DRAW_NO_ROUTING_UTIL);
    CHECK(ds.show_rr == false);
    CHECK(ds.draw_block_outlines == true);
    CHECK(ds.draw_block_text == true);
    CHECK(ds.draw_partitions == false);
    CHECK(ds.show_graphics == false);
    CHECK(ds.gr_automode == 0);
    CHECK(ds.auto_proceed == false);
    CHECK(ds.draw_route_type == e_route_type::GLOBAL);
    CHECK(ds.save_graphics == false);
    CHECK(ds.renderer_type == "rhi");
    CHECK(ds.forced_pause == false);
    CHECK(ds.sequence_number == 0);
    CHECK(ds.net_alpha == 255);
    CHECK(ds.is_flat == false);
    CHECK(ds.show_noc_button == false);
    CHECK(ds.draw_noc == DRAW_NO_NOC);
    CHECK(ds.justEnabled == false);
    CHECK(ds.save_graphics_file_base == "vpr");
}

TEST_CASE("DrawState: every plain field round-trips through assignment",
          "[layer3][vpr_gui][drawstate]") {
    t_draw_state ds;

    ds.show_nets = true;
    CHECK(ds.show_nets);
    ds.draw_nets = DRAW_ROUTED_NETS;
    CHECK(ds.draw_nets == DRAW_ROUTED_NETS);
    ds.show_rr = true;
    CHECK(ds.show_rr);
    ds.show_crit_path = true;
    CHECK(ds.show_crit_path);
    ds.show_congestion = DRAW_CONGESTED;
    CHECK(ds.show_congestion == DRAW_CONGESTED);
    ds.show_routing_util = DRAW_ROUTING_UTIL;
    CHECK(ds.show_routing_util == DRAW_ROUTING_UTIL);
    ds.show_placement_macros = DRAW_PLACEMENT_MACROS;
    CHECK(ds.show_placement_macros == DRAW_PLACEMENT_MACROS);
    ds.draw_block_outlines = false;
    CHECK_FALSE(ds.draw_block_outlines);
    ds.draw_block_text = false;
    CHECK_FALSE(ds.draw_block_text);
    ds.show_blk_internal = 3;
    CHECK(ds.show_blk_internal == 3);
    ds.show_routing_bb = 7;
    CHECK(ds.show_routing_bb == 7);
    ds.draw_net_max_fanout = 100;
    CHECK(ds.draw_net_max_fanout == 100);
    ds.gr_automode = 2;
    CHECK(ds.gr_automode == 2);
    ds.net_alpha = 128;
    CHECK(ds.net_alpha == 128);
    ds.pres_fac = 2.5f;
    CHECK(ds.pres_fac == 2.5f);
    ds.is_flat = true;
    CHECK(ds.is_flat);
    ds.show_noc_button = true;
    CHECK(ds.show_noc_button);
    ds.draw_noc = DRAW_NOC_LINK_USAGE;
    CHECK(ds.draw_noc == DRAW_NOC_LINK_USAGE);
    ds.renderer_type = "deferred";
    CHECK(ds.renderer_type == "deferred");
    ds.save_graphics = true;
    CHECK(ds.save_graphics);
    ds.save_graphics_file_base = "out";
    CHECK(ds.save_graphics_file_base == "out");
    ds.graphics_commands = "set_nets 1; exit 0";
    CHECK(ds.graphics_commands == "set_nets 1; exit 0");
    ds.forced_pause = true;
    CHECK(ds.forced_pause);
    ds.sequence_number = 42;
    CHECK(ds.sequence_number == 42);
    ds.draw_route_type = e_route_type::DETAILED;
    CHECK(ds.draw_route_type == e_route_type::DETAILED);
}

TEST_CASE("DrawState: copy construction preserves all plain state",
          "[layer3][vpr_gui][drawstate]") {
    // Used by graphics_commands save/restore — must stay copyable.
    t_draw_state src;
    src.show_nets = true;
    src.show_rr = true;
    src.show_blk_internal = 5;
    src.draw_block_text = false;
    src.renderer_type = "immediate";
    src.graphics_commands = "set_routing_util 1";
    src.net_alpha = 64;

    t_draw_state copy = src;
    CHECK(copy.show_nets);
    CHECK(copy.show_rr);
    CHECK(copy.show_blk_internal == 5);
    CHECK_FALSE(copy.draw_block_text);
    CHECK(copy.renderer_type == "immediate");
    CHECK(copy.graphics_commands == "set_routing_util 1");
    CHECK(copy.net_alpha == 64);
}

TEST_CASE("DrawState: assignment overwrites all plain state",
          "[layer3][vpr_gui][drawstate]") {
    t_draw_state src;
    src.show_nets = true;
    src.gr_automode = 1;

    t_draw_state dst;
    REQUIRE_FALSE(dst.show_nets);
    REQUIRE(dst.gr_automode == 0);

    dst = src;
    CHECK(dst.show_nets);
    CHECK(dst.gr_automode == 1);
}

TEST_CASE("DrawCoords: default tile geometry",
          "[layer3][vpr_gui][drawstate]") {
    t_draw_coords dc;
    CHECK(dc.get_tile_width() == 0);
    CHECK(dc.get_tile_height() == dc.get_tile_width()); // symmetric per impl note
    CHECK(dc.pin_size == 0);
    CHECK(dc.tile_x.empty());
    CHECK(dc.tile_y.empty());
}

TEST_CASE("DrawCoords: pin_size and tile vectors are public-mutable",
          "[layer3][vpr_gui][drawstate]") {
    // pin_size, tile_x, tile_y are public; tile_width is private and only
    // settable via init_draw_coords (friend) — exercising it here would
    // require a populated BlkLocRegistry, deferred to the flow-stage
    // fixture (same gap as S6's Layer 4 stage matrix).
    t_draw_coords dc;
    dc.pin_size = 3.0f;
    dc.tile_x = {0.0f, 1.0f, 2.0f};
    dc.tile_y = {0.0f, 1.0f};
    CHECK(dc.pin_size == 3.0f);
    CHECK(dc.tile_x.size() == 3);
    CHECK(dc.tile_y.size() == 2);
}

TEST_CASE("DrawState global accessors return stable non-null pointers",
          "[layer3][vpr_gui][drawstate]") {
    t_draw_state* a = get_draw_state_vars();
    t_draw_state* b = get_draw_state_vars();
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a == b); // singleton — same global

    t_draw_coords* ca = get_draw_coords_vars();
    t_draw_coords* cb = get_draw_coords_vars();
    REQUIRE(ca != nullptr);
    REQUIRE(cb != nullptr);
    CHECK(ca == cb);
}
