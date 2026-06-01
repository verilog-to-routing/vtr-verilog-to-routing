#include "catch2/catch_test_macros.hpp"

#include "device_grid.h"
#include "interposer_types.h"
#include "physical_types.h"
#include "vpr_error.h"
#include "vtr_assert.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct TestTileSet {
    t_physical_tile_type clb;
    t_physical_tile_type io;
    t_physical_tile_type empty;
    t_physical_tile_type dsp;
    std::vector<t_physical_tile_type_ptr> physical_tile_types;

    TestTileSet() {
        clb.name = "clb";
        clb.width = 1;
        clb.height = 1;
        clb.capacity = 1;
        clb.sub_tiles.emplace_back();

        io.name = "io";
        io.width = 1;
        io.height = 1;
        io.capacity = 1;
        io.sub_tiles.emplace_back();

        empty.name = "empty";
        empty.width = 1;
        empty.height = 1;
        empty.capacity = 0;
        empty.sub_tiles.emplace_back();

        dsp.name = "dsp";
        dsp.width = 2;
        dsp.height = 2;
        dsp.capacity = 1;
        dsp.sub_tiles.emplace_back();

        physical_tile_types = {&clb, &io, &empty, &dsp};
    }
};

/// Place a tile at (x_anchor, y_anchor) and fill its width/height footprint with offsets.
static void set_tile_at(vtr::NdMatrix<t_grid_tile, 3>& grid,
                 int layer,
                 int x_anchor,
                 int y_anchor,
                 t_physical_tile_type_ptr type) {
    for (int x = x_anchor; x < x_anchor + type->width; ++x) {
        for (int y = y_anchor; y < y_anchor + type->height; ++y) {
            grid[layer][x][y].type = type;
            grid[layer][x][y].width_offset = x - x_anchor;
            grid[layer][x][y].height_offset = y - y_anchor;
        }
    }
}

void fill_layer_with_tile(vtr::NdMatrix<t_grid_tile, 3>& grid,
                          int layer,
                          size_t width,
                          size_t height,
                          t_physical_tile_type_ptr type) {
    VTR_ASSERT(type->width == 1);
    VTR_ASSERT(type->height == 1);
    for (size_t x = 0; x < width; ++x) {
        for (size_t y = 0; y < height; ++y) {
            grid[layer][x][y].type = type;
            grid[layer][x][y].width_offset = 0;
            grid[layer][x][y].height_offset = 0;
        }
    }
}

DeviceGrid make_device_grid(const std::string& name,
                            vtr::NdMatrix<t_grid_tile, 3> grid,
                            std::vector<t_layer_def> layer_defs = {}) {
    t_grid_def grid_def;
    grid_def.name = name;
    grid_def.grid_type = e_grid_def_type::FIXED;
    grid_def.width = grid.dim_size(1);
    grid_def.height = grid.dim_size(2);
    grid_def.layers = std::move(layer_defs);
    if (grid_def.layers.empty()) {
        grid_def.layers.resize(grid.dim_size(0));
    }
    return DeviceGrid(grid_def, std::move(grid));
}

std::filesystem::path temp_grid_path(const std::string& test_name) {
    return std::filesystem::temp_directory_path() / ("device_grid_io_" + test_name + ".grid");
}

void require_grids_equivalent(const DeviceGrid& original, const DeviceGrid& loaded) {
    REQUIRE(original.name() == loaded.name());
    REQUIRE(original.get_num_layers() == loaded.get_num_layers());
    REQUIRE(original.width() == loaded.width());
    REQUIRE(original.height() == loaded.height());
    REQUIRE(original.has_interposer_cuts() == loaded.has_interposer_cuts());
    REQUIRE(original.get_die_count() == loaded.get_die_count());

    REQUIRE(original.get_horizontal_interposer_cuts() == loaded.get_horizontal_interposer_cuts());
    REQUIRE(original.get_vertical_interposer_cuts() == loaded.get_vertical_interposer_cuts());

    for (const t_physical_tile_loc& loc : original.all_locations()) {
        REQUIRE(original.get_physical_type(loc) == loaded.get_physical_type(loc));
        REQUIRE(original.get_width_offset(loc) == loaded.get_width_offset(loc));
        REQUIRE(original.get_height_offset(loc) == loaded.get_height_offset(loc));
        REQUIRE(original.get_loc_die_id(loc) == loaded.get_loc_die_id(loc));
    }

    for (const t_die_region& die_region : original.all_die_regions()) {
        REQUIRE(original.get_die_region_id(die_region) == loaded.get_die_region_id(die_region));
    }
}

DeviceGrid round_trip_grid(const DeviceGrid& grid, const std::vector<t_physical_tile_type_ptr>& tile_types, const std::string& test_name) {
    const std::filesystem::path filepath = temp_grid_path(test_name);
    grid.write_grid_file(filepath.string());
    return DeviceGrid(filepath.string(), tile_types);
}

t_layer_def make_layer_with_cuts(const std::vector<std::pair<e_interposer_cut_type, std::string>>& cuts) {
    t_layer_def layer_def;
    for (const auto& [dim, loc] : cuts) {
        t_interposer_cut_inf cut_inf;
        cut_inf.dim = dim;
        cut_inf.loc = loc;
        layer_def.interposer_cuts.push_back(cut_inf);
    }
    return layer_def;
}

} // namespace

TEST_CASE("DeviceGrid 2D round-trip", "[device_grid_io]") {
    TestTileSet tiles;

    const size_t grid_width = 10;
    const size_t grid_height = 10;
    auto grid_matrix = vtr::NdMatrix<t_grid_tile, 3>({1, grid_width, grid_height});
    fill_layer_with_tile(grid_matrix, 0, grid_width, grid_height, &tiles.clb);
    set_tile_at(grid_matrix, 0, 0, 0, &tiles.io);
    set_tile_at(grid_matrix, 0, 9, 0, &tiles.io);
    set_tile_at(grid_matrix, 0, 0, 9, &tiles.io);
    set_tile_at(grid_matrix, 0, 9, 9, &tiles.io);
    set_tile_at(grid_matrix, 0, 1, 1, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 4, 4, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 6, 1, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 1, 6, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 6, 6, &tiles.dsp);

    DeviceGrid original = make_device_grid("grid_2d", std::move(grid_matrix));
    DeviceGrid loaded = round_trip_grid(original, tiles.physical_tile_types, "2d");

    require_grids_equivalent(original, loaded);

    REQUIRE(loaded.num_instances(&tiles.io, 0) == 4);
    REQUIRE(loaded.num_instances(&tiles.dsp, 0) == 5);
    REQUIRE(loaded.num_instances(&tiles.clb, 0) == 76);
    REQUIRE(loaded.get_num_layers() == 1);
    REQUIRE_FALSE(loaded.has_interposer_cuts());
    REQUIRE(loaded.get_die_count() == 1);
}

TEST_CASE("DeviceGrid 3D round-trip without interposer cuts", "[device_grid_io]") {
    TestTileSet tiles;

    const size_t grid_width = 8;
    const size_t grid_height = 8;
    const size_t num_layers = 2;
    auto grid_matrix = vtr::NdMatrix<t_grid_tile, 3>({num_layers, grid_width, grid_height});

    // Layer 0: CLB fabric, IO corners, multiple 2x2 DSP blocks.
    fill_layer_with_tile(grid_matrix, 0, grid_width, grid_height, &tiles.clb);
    set_tile_at(grid_matrix, 0, 0, 0, &tiles.io);
    set_tile_at(grid_matrix, 0, 7, 0, &tiles.io);
    set_tile_at(grid_matrix, 0, 0, 7, &tiles.io);
    set_tile_at(grid_matrix, 0, 7, 7, &tiles.io);
    set_tile_at(grid_matrix, 0, 1, 1, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 4, 4, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 5, 1, &tiles.dsp);
    set_tile_at(grid_matrix, 0, 1, 5, &tiles.dsp);

    // Layer 1: IO fabric, central CLB cluster, DSP blocks away from corners.
    fill_layer_with_tile(grid_matrix, 1, grid_width, grid_height, &tiles.io);
    for (int x = 2; x <= 5; ++x) {
        for (int y = 2; y <= 5; ++y) {
            set_tile_at(grid_matrix, 1, x, y, &tiles.clb);
        }
    }
    set_tile_at(grid_matrix, 1, 6, 6, &tiles.dsp);
    set_tile_at(grid_matrix, 1, 4, 6, &tiles.dsp);
    set_tile_at(grid_matrix, 1, 6, 1, &tiles.dsp);

    std::vector<t_layer_def> layer_defs(num_layers);
    DeviceGrid original = make_device_grid("grid_3d", std::move(grid_matrix), std::move(layer_defs));
    DeviceGrid loaded = round_trip_grid(original, tiles.physical_tile_types, "3d");

    require_grids_equivalent(original, loaded);

    REQUIRE(loaded.get_num_layers() == 2);
    REQUIRE_FALSE(loaded.has_interposer_cuts());
    REQUIRE(loaded.get_die_count() == 2);
    REQUIRE(loaded.num_instances(&tiles.clb, 0) == 44);
    REQUIRE(loaded.num_instances(&tiles.io, 0) == 4);
    REQUIRE(loaded.num_instances(&tiles.dsp, 0) == 4);
    REQUIRE(loaded.num_instances(&tiles.clb, 1) == 16);
    REQUIRE(loaded.num_instances(&tiles.io, 1) == 36);
    REQUIRE(loaded.num_instances(&tiles.dsp, 1) == 3);
}

TEST_CASE("DeviceGrid 2.5D interposer round-trip", "[device_grid_io]") {
    TestTileSet tiles;

    const size_t grid_width = 6;
    const size_t grid_height = 6;
    auto grid_matrix = vtr::NdMatrix<t_grid_tile, 3>({1, grid_width, grid_height});
    fill_layer_with_tile(grid_matrix, 0, grid_width, grid_height, &tiles.clb);

    std::vector<t_layer_def> layer_defs;
    layer_defs.push_back(make_layer_with_cuts({
        {e_interposer_cut_type::HORZ, "2"},
        {e_interposer_cut_type::VERT, "3"},
    }));

    DeviceGrid original = make_device_grid("grid_interposer", std::move(grid_matrix), std::move(layer_defs));
    DeviceGrid loaded = round_trip_grid(original, tiles.physical_tile_types, "interposer_25d");

    require_grids_equivalent(original, loaded);

    REQUIRE(loaded.has_interposer_cuts());
    REQUIRE(loaded.get_die_count() == 4);
    REQUIRE(loaded.get_horizontal_interposer_cuts()[0] == std::vector<int>{2});
    REQUIRE(loaded.get_vertical_interposer_cuts()[0] == std::vector<int>{3});

    const t_physical_tile_loc same_die_a{1, 1, 0};
    const t_physical_tile_loc same_die_b{2, 2, 0};
    const t_physical_tile_loc other_die{4, 4, 0};
    REQUIRE(loaded.are_locs_on_same_die(same_die_a, same_die_b));
    REQUIRE_FALSE(loaded.are_locs_on_same_die(same_die_a, other_die));
    REQUIRE(loaded.get_loc_die_id(same_die_a) == loaded.get_loc_die_id(same_die_b));
    REQUIRE(loaded.get_loc_die_id(same_die_a) != loaded.get_loc_die_id(other_die));
}

TEST_CASE("DeviceGrid 3D with interposer cuts round-trip", "[device_grid_io]") {
    TestTileSet tiles;

    const size_t grid_width = 4;
    const size_t grid_height = 4;
    auto grid_matrix = vtr::NdMatrix<t_grid_tile, 3>({2, grid_width, grid_height});
    fill_layer_with_tile(grid_matrix, 0, grid_width, grid_height, &tiles.clb);
    fill_layer_with_tile(grid_matrix, 1, grid_width, grid_height, &tiles.io);

    std::vector<t_layer_def> layer_defs;
    layer_defs.push_back(make_layer_with_cuts({
        {e_interposer_cut_type::HORZ, "1"},
        {e_interposer_cut_type::VERT, "2"},
    }));
    layer_defs.emplace_back();

    DeviceGrid original = make_device_grid("grid_3d_interposer", std::move(grid_matrix), std::move(layer_defs));
    DeviceGrid loaded = round_trip_grid(original, tiles.physical_tile_types, "3d_interposer");

    require_grids_equivalent(original, loaded);

    REQUIRE(loaded.get_num_layers() == 2);
    REQUIRE(loaded.has_interposer_cuts());
    REQUIRE(loaded.get_die_count() == 5);
    REQUIRE(loaded.get_horizontal_interposer_cuts()[0] == std::vector<int>{1});
    REQUIRE(loaded.get_vertical_interposer_cuts()[0] == std::vector<int>{2});
    REQUIRE(loaded.get_horizontal_interposer_cuts()[1].empty());
    REQUIRE(loaded.get_vertical_interposer_cuts()[1].empty());

    REQUIRE(loaded.get_loc_die_id({0, 0, 0}) != loaded.get_loc_die_id({3, 3, 0}));
    REQUIRE(loaded.get_loc_die_id({0, 0, 1}) == loaded.get_loc_die_id({3, 3, 1}));
}

TEST_CASE("DeviceGrid unknown tile name is fatal", "[device_grid_io]") {
    TestTileSet tiles;

    const std::filesystem::path filepath = temp_grid_path("unknown_tile");
    {
        std::ofstream out(filepath);
        out << "name: bad_grid\n";
        out << "layers: 1\n";
        out << "width: 1\n";
        out << "height: 1\n";
        out << "interposer_cuts layer=0 horz: vert:\n";
        out << "tiles layer=0\n";
        out << "  0 0 unknown_tile 0 0\n";
    }

    REQUIRE_THROWS_AS(DeviceGrid(filepath.string(), tiles.physical_tile_types), VprError);
}
