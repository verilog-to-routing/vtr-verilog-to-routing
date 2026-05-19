#include "catch2/catch_test_macros.hpp"

#include "device_grid.h"
#include "switchblock_scatter_gather_common_utils.h"
#include "vtr_ndmatrix.h"

namespace {

char g_test_tile_name[] = "test_tile";

/// Minimal grid for tests that only need width/height (match_sb_xy uses those dimensions only).
DeviceGrid make_test_device_grid(int width, int height) {
    auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, (size_t)width, (size_t)height});

    static t_physical_tile_type tile_type;
    tile_type.name = g_test_tile_name;
    tile_type.width = 1;
    tile_type.height = 1;
    tile_type.capacity = 1;
    tile_type.sub_tiles.emplace_back();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            test_grid[0][x][y].type = &tile_type;
            test_grid[0][x][y].width_offset = 0;
            test_grid[0][x][y].height_offset = 0;
        }
    }

    t_grid_def grid_def;
    grid_def.name = "test_device_for_match_sb_xy";
    grid_def.layers.resize(1);
    return DeviceGrid(grid_def, std::move(test_grid));
}

} // namespace

TEST_CASE("match_sb_xy exact and partial coordinates", "[vpr][switchblock][match_sb_xy]") {
    DeviceGrid grid = make_test_device_grid(16, 16);
    t_physical_tile_loc loc;
    loc.layer_num = 0;

    t_specified_loc spec;

    SECTION("exact (x, y)") {
        spec.x = 3;
        spec.y = 4;
        loc.x = 3;
        loc.y = 4;
        REQUIRE(match_sb_xy(grid, loc, spec));

        loc.y = 5;
        REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
    }

    SECTION("fixed x, all rows (y omitted in architecture)") {
        spec.x = 5;
        spec.y = -1;
        loc.x = 5;
        loc.y = 0;
        REQUIRE(match_sb_xy(grid, loc, spec));
        loc.y = 15;
        REQUIRE(match_sb_xy(grid, loc, spec));

        loc.x = 4;
        loc.y = 0;
        REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
    }

    SECTION("fixed y, all columns (x omitted in architecture)") {
        spec.x = -1;
        spec.y = 7;
        loc.x = 0;
        loc.y = 7;
        REQUIRE(match_sb_xy(grid, loc, spec));
        loc.x = 15;
        REQUIRE(match_sb_xy(grid, loc, spec));

        loc.x = 0;
        loc.y = 6;
        REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
    }
}

TEST_CASE("match_sb_xy single rectangle region (no repeat)", "[vpr][switchblock][match_sb_xy]") {
    // Mirrors a <region> with inclusive bounds and default incrx/incry of 1 (see architecture reference).
    DeviceGrid grid = make_test_device_grid(32, 32);
    t_physical_tile_loc loc;
    loc.layer_num = 0;

    t_specified_loc spec;
    spec.x = -1;
    spec.y = -1;
    spec.reg_x.start = 1;
    spec.reg_x.end = 5;
    spec.reg_x.repeat = 0;
    spec.reg_x.incr = 1;
    spec.reg_y.start = 1;
    spec.reg_y.end = 4;
    spec.reg_y.repeat = 0;
    spec.reg_y.incr = 1;

    loc.x = 3;
    loc.y = 2;
    REQUIRE(match_sb_xy(grid, loc, spec));

    loc.x = 0;
    loc.y = 2;
    REQUIRE_FALSE(match_sb_xy(grid, loc, spec));

    loc.x = 5;
    loc.y = 4;
    REQUIRE(match_sb_xy(grid, loc, spec));

    loc.x = 3;
    loc.y = 5;
    REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
}

TEST_CASE("match_sb_xy region with incrx stride", "[vpr][switchblock][match_sb_xy]") {
    // Every second column within the rectangle (incrx=2), as in the reference <region> increment example.
    DeviceGrid grid = make_test_device_grid(16, 16);
    t_physical_tile_loc loc;
    loc.layer_num = 0;

    t_specified_loc spec;
    spec.x = -1;
    spec.y = -1;
    spec.reg_x.start = 1;
    spec.reg_x.end = 5;
    spec.reg_x.repeat = 0;
    spec.reg_x.incr = 2;
    spec.reg_y.start = 1;
    spec.reg_y.end = 4;
    spec.reg_y.repeat = 0;
    spec.reg_y.incr = 1;

    loc.y = 2;
    loc.x = 1;
    REQUIRE(match_sb_xy(grid, loc, spec));
    loc.x = 2;
    REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
    loc.x = 3;
    REQUIRE(match_sb_xy(grid, loc, spec));
}

TEST_CASE("match_sb_xy repeated region tile (repeatx/repeaty)", "[vpr][switchblock][match_sb_xy]") {
    // Same pattern as reference: startx=1 endx=2 starty=1 endy=4 repeatx=3 repeaty=5.
    DeviceGrid grid = make_test_device_grid(32, 32);
    t_physical_tile_loc loc;
    loc.layer_num = 0;

    t_specified_loc spec;
    spec.x = -1;
    spec.y = -1;
    spec.reg_x.start = 1;
    spec.reg_x.end = 2;
    spec.reg_x.repeat = 3;
    spec.reg_x.incr = 1;
    spec.reg_y.start = 1;
    spec.reg_y.end = 4;
    spec.reg_y.repeat = 5;
    spec.reg_y.incr = 1;

    loc.x = 1;
    loc.y = 1;
    REQUIRE(match_sb_xy(grid, loc, spec));

    loc.x = 4;
    loc.y = 1;
    REQUIRE(match_sb_xy(grid, loc, spec));

    loc.x = 3;
    loc.y = 1;
    REQUIRE_FALSE(match_sb_xy(grid, loc, spec));

    loc.x = 1;
    loc.y = 6;
    REQUIRE(match_sb_xy(grid, loc, spec));

    loc.x = 1;
    loc.y = 5;
    REQUIRE_FALSE(match_sb_xy(grid, loc, spec));
}

TEST_CASE("match_sb_xy clamps region end to grid", "[vpr][switchblock][match_sb_xy]") {
    DeviceGrid grid = make_test_device_grid(10, 10);
    t_physical_tile_loc loc;
    loc.layer_num = 0;

    t_specified_loc spec;
    spec.x = -1;
    spec.y = -1;
    spec.reg_x.start = 8;
    spec.reg_x.end = 100;
    spec.reg_x.repeat = 0;
    spec.reg_x.incr = 1;
    spec.reg_y.start = 0;
    spec.reg_y.end = 2;
    spec.reg_y.repeat = 0;
    spec.reg_y.incr = 1;

    loc.x = 9;
    loc.y = 1;
    REQUIRE(match_sb_xy(grid, loc, spec));
}
