#include "arch_util.h"
#include "catch2/catch_test_macros.hpp"

#include "compressed_grid.h"
#include "globals.h"
#include "physical_types.h"

namespace {

void set_type_tile_to_empty(const int x, const int y, vtr::NdMatrix<t_grid_tile, 3>& grid) {
    t_physical_tile_type_ptr type = grid[0][x][y].type;
    const int width_offset = grid[0][x][y].width_offset;
    const int height_offset = grid[0][x][y].height_offset;
    const int x_anchor = x - width_offset;
    const int y_anchor = y - height_offset;

    for (int i = x_anchor; i < x_anchor + type->width; i++) {
        for (int j = y_anchor; j < y_anchor + type->height; j++) {
            if (grid[0][i][j].type == type && grid[0][i][j].width_offset == i - x_anchor && grid[0][i][j].height_offset == j - y_anchor) {
                grid[0][i][j].type = g_vpr_ctx.device().EMPTY_PHYSICAL_TILE_TYPE;
                grid[0][i][j].width_offset = 0;
                grid[0][i][j].height_offset = 0;
            }
        }
    }
}

void set_tile_type_at_loc(const int x_anchor, const int y_anchor, vtr::NdMatrix<t_grid_tile, 3>& grid, const t_physical_tile_type& tile_type) {

    for (int i = x_anchor; i < x_anchor + tile_type.width; i++) {
        for (int j = y_anchor; j < y_anchor + tile_type.height; j++) {
            if (grid[0][i][j].type != g_vpr_ctx.device().EMPTY_PHYSICAL_TILE_TYPE) {
                set_type_tile_to_empty(i, j, grid);
            }
            grid[0][i][j].type = &tile_type;
            grid[0][i][j].width_offset = i - x_anchor;
            grid[0][i][j].height_offset = j - y_anchor;
        }
    }
}

TEST_CASE("test_compressed_grid", "[vpr_compressed_grid]") {
    // test device grid name
    std::string device_grid_name = "test";

    // creating a reference for the empty tile name and router name
    char empty_tile_name[] = "empty";
    char io_tile_name[] = "io";
    char small_tile_name[] = "small";
    char tall_tile_name[] = "tall";
    char large_tile_name[] = "large";

    // device grid parameters
    const int test_grid_width = 100;
    const int test_grid_height = 100;

    // create the test device grid (10x10)
    auto test_grid = vtr::NdMatrix<t_grid_tile, 3>({1, test_grid_width, test_grid_height});

    auto& logical_block_types = g_vpr_ctx.mutable_device().logical_block_types;
    logical_block_types.clear();

    t_physical_tile_type empty_tile;
    empty_tile.name = empty_tile_name;
    empty_tile.height = 1;
    empty_tile.width = 1;
    empty_tile.sub_tiles.emplace_back();

    t_logical_block_type EMPTY_LOGICAL_BLOCK_TYPE = get_empty_logical_type();
    EMPTY_LOGICAL_BLOCK_TYPE.index = 0;
    EMPTY_LOGICAL_BLOCK_TYPE.equivalent_tiles.push_back(&empty_tile);
    logical_block_types.push_back(EMPTY_LOGICAL_BLOCK_TYPE);

    g_vpr_ctx.mutable_device().EMPTY_PHYSICAL_TILE_TYPE = &empty_tile;

    empty_tile.sub_tiles.back().index = 0;
    empty_tile.sub_tiles.back().equivalent_sites.push_back(&EMPTY_LOGICAL_BLOCK_TYPE);

    // create an io physical tile and assign its parameters
    t_physical_tile_type io_tile;
    io_tile.name = io_tile_name;
    io_tile.height = 1;
    io_tile.width = 1;
    io_tile.sub_tiles.emplace_back();

    t_logical_block_type io_logical_type;
    io_logical_type.index = 1;
    io_logical_type.equivalent_tiles.push_back(&io_tile);
    logical_block_types.push_back(io_logical_type);

    io_tile.sub_tiles.back().index = 0;
    io_tile.sub_tiles.back().equivalent_sites.push_back(&io_logical_type);

    // create a small tile and assign its parameters
    t_physical_tile_type small_tile;
    small_tile.name = small_tile_name;
    small_tile.height = 1;
    small_tile.width = 1;
    small_tile.sub_tiles.emplace_back();

    t_logical_block_type small_logical_type;
    small_logical_type.index = 2;
    small_logical_type.equivalent_tiles.push_back(&small_tile);
    logical_block_types.push_back(small_logical_type);

    small_tile.sub_tiles.back().index = 0;
    small_tile.sub_tiles.back().equivalent_sites.push_back(&small_logical_type);

    // create a small tile and assign its parameters
    t_physical_tile_type tall_tile;
    tall_tile.name = tall_tile_name;
    tall_tile.height = 4;
    tall_tile.width = 1;
    tall_tile.sub_tiles.emplace_back();

    t_logical_block_type tall_logical_type;
    tall_logical_type.index = 3;
    tall_logical_type.equivalent_tiles.push_back(&tall_tile);
    logical_block_types.push_back(tall_logical_type);

    tall_tile.sub_tiles.back().index = 0;
    tall_tile.sub_tiles.back().equivalent_sites.push_back(&tall_logical_type);

    t_physical_tile_type large_tile;
    large_tile.name = large_tile_name;
    large_tile.height = 3;
    large_tile.width = 3;
    large_tile.sub_tiles.emplace_back();

    t_logical_block_type large_logical_type;
    large_logical_type.index = 4;
    large_logical_type.equivalent_tiles.push_back(&large_tile);
    logical_block_types.push_back(large_logical_type);

    large_tile.sub_tiles.back().index = 0;
    large_tile.sub_tiles.back().equivalent_sites.push_back(&large_logical_type);

    for (int x = 0; x < test_grid_width; x++) {
        for (int y = 0; y < test_grid_height; y++) {
            test_grid[0][x][y].type = &io_tile;
            test_grid[0][x][y].height_offset = 0;
            test_grid[0][x][y].width_offset = 0;
        }
    }

    for (int x = 1; x < test_grid_width - 1; x++) {
        for (int y = 1; y < test_grid_height - 1; y++) {
            set_tile_type_at_loc(x, y, test_grid, small_tile);
        }
    }

    for (int x = 7; x < test_grid_width - 7; x += 10) {
        for (int y = 5; y < test_grid_height - 5; y += 5) {
            set_tile_type_at_loc(x, y, test_grid, tall_tile);
        }
    }

    for (int x = 8; x < test_grid_width - 8; x += 17) {
        for (int y = 7; y < test_grid_height - 6; y += 13) {
            set_tile_type_at_loc(x, y, test_grid, large_tile);
        }
    }

    auto& grid = g_vpr_ctx.mutable_device().grid;
    std::vector<std::vector<int>> dummy_cuts0, dummy_cuts1;
    grid = DeviceGrid("test_device_grid", test_grid, std::move(dummy_cuts0), std::move(dummy_cuts1));

    std::vector<t_compressed_block_grid> compressed_grids = create_compressed_block_grids();

    SECTION("Check compressed grid sizes") {
        REQUIRE(compressed_grids[io_logical_type.index].compressed_to_grid_x[0].size() == 100);
        REQUIRE(compressed_grids[io_logical_type.index].compressed_to_grid_y[0].size() == 100);

        REQUIRE(compressed_grids[small_logical_type.index].compressed_to_grid_x[0].size() == 98);
        REQUIRE(compressed_grids[small_logical_type.index].compressed_to_grid_y[0].size() == 98);

        REQUIRE(compressed_grids[tall_logical_type.index].compressed_to_grid_x[0].size() == 9);
        REQUIRE(compressed_grids[tall_logical_type.index].compressed_to_grid_y[0].size() == 18);

        REQUIRE(compressed_grids[large_logical_type.index].compressed_to_grid_x[0].size() == 5);
        REQUIRE(compressed_grids[large_logical_type.index].compressed_to_grid_y[0].size() == 7);
    }

    SECTION("Exact mapped locations in the compressed grids") {
        t_physical_tile_loc comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({25, 33, 0});
        t_physical_tile_loc grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{25, 33, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({76, 7, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{76, 7, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({59, 85, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{59, 85, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({7, 5, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{7, 5, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({77, 40, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{77, 40, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({37, 85, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{37, 85, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx({2, 3, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{2, 3, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx({17, 3, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{17, 3, 0});
    }

    SECTION("Round to the nearest mapped location in the compressed grid") {
        t_physical_tile_loc comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({25, 33, 0});
        t_physical_tile_loc grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{25, 33, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({99, 10, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{76, 7, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx({51, 79, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{59, 85, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({1, 6, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{7, 5, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({81, 38, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{77, 40, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx({34, 83, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{37, 85, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx({0, 0, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{1, 1, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx({99, 99, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{98, 98, 0});
    }

    SECTION("Round down to the closest mapped location in the compressed grid") {
        t_physical_tile_loc comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({25, 33, 0});
        t_physical_tile_loc grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{25, 33, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({99, 10, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{76, 7, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({51, 79, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{42, 72, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({1, 6, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{7, 5, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({81, 38, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{77, 35, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({34, 83, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{27, 80, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({0, 0, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{1, 1, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx_round_down({99, 99, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{98, 98, 0});
    }

    SECTION("Round up to the closest mapped location in the compressed grid") {
        t_physical_tile_loc comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({25, 33, 0});
        t_physical_tile_loc grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{25, 33, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({99, 10, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{76, 20, 0});

        comp_loc = compressed_grids[large_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({51, 79, 0});
        grid_loc = compressed_grids[large_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{59, 85, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({1, 6, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{7, 10, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({81, 38, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{87, 40, 0});

        comp_loc = compressed_grids[tall_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({34, 83, 0});
        grid_loc = compressed_grids[tall_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{37, 85, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({0, 0, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{1, 1, 0});

        comp_loc = compressed_grids[small_logical_type.index].grid_loc_to_compressed_loc_approx_round_up({99, 99, 0});
        grid_loc = compressed_grids[small_logical_type.index].compressed_loc_to_grid_loc(comp_loc);
        REQUIRE(grid_loc == t_physical_tile_loc{98, 98, 0});
    }

    logical_block_types.clear();
}

} // namespace
