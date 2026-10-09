#include "catch2/catch_test_macros.hpp"

#include "arch_util.h"
#include "clustered_netlist.h"
#include "compressed_grid.h"
#include "device_grid.h"
#include "globals.h"
#include "partition_region.h"
#include "physical_types.h"
#include "place_constraints.h"
#include "region.h"

namespace {

/**
 * @brief Builds a 20x20 single-layer device with an IO ring and 1x1 "small" tiles inside,
 *        and loads it with its compressed grids into the global contexts.
 *
 * The tile and logical block type objects are owned by the caller so that the
 * pointers stored in the grid stay valid for the duration of the test.
 */
void build_small_tile_device(t_physical_tile_type& empty_tile,
                             t_physical_tile_type& io_tile,
                             t_physical_tile_type& small_tile,
                             t_logical_block_type& empty_logical_type,
                             t_logical_block_type& io_logical_type,
                             t_logical_block_type& small_logical_type) {
    const int grid_width = 20;
    const int grid_height = 20;

    DeviceContext& device_ctx = g_vpr_ctx.mutable_device();
    std::vector<t_logical_block_type>& logical_block_types = device_ctx.logical_block_types;
    logical_block_types.clear();

    empty_tile.name = "empty";
    empty_tile.height = 1;
    empty_tile.width = 1;
    empty_tile.sub_tiles.emplace_back();
    empty_logical_type = get_empty_logical_type();
    empty_logical_type.index = 0;
    empty_logical_type.equivalent_tiles.push_back(&empty_tile);
    logical_block_types.push_back(empty_logical_type);
    device_ctx.EMPTY_PHYSICAL_TILE_TYPE = &empty_tile;
    empty_tile.sub_tiles.back().index = 0;
    empty_tile.sub_tiles.back().equivalent_sites.push_back(&empty_logical_type);

    io_tile.name = "io";
    io_tile.height = 1;
    io_tile.width = 1;
    io_tile.sub_tiles.emplace_back();
    io_logical_type.index = 1;
    io_logical_type.equivalent_tiles.push_back(&io_tile);
    logical_block_types.push_back(io_logical_type);
    io_tile.sub_tiles.back().index = 0;
    io_tile.sub_tiles.back().equivalent_sites.push_back(&io_logical_type);

    small_tile.name = "small";
    small_tile.height = 1;
    small_tile.width = 1;
    small_tile.sub_tiles.emplace_back();
    small_logical_type.index = 2;
    small_logical_type.equivalent_tiles.push_back(&small_tile);
    logical_block_types.push_back(small_logical_type);
    small_tile.sub_tiles.back().index = 0;
    small_tile.sub_tiles.back().equivalent_sites.push_back(&small_logical_type);

    vtr::NdMatrix<t_grid_tile, 3> tiles({1, (size_t)grid_width, (size_t)grid_height});
    for (int x = 0; x < grid_width; x++) {
        for (int y = 0; y < grid_height; y++) {
            bool ring = (x == 0 || y == 0 || x == grid_width - 1 || y == grid_height - 1);
            tiles[0][x][y].type = ring ? &io_tile : &small_tile;
            tiles[0][x][y].width_offset = 0;
            tiles[0][x][y].height_offset = 0;
        }
    }

    t_grid_def grid_def;
    grid_def.name = "test_device_grid";
    grid_def.layers.resize(1);
    device_ctx.grid = DeviceGrid(grid_def, std::move(tiles));

    g_vpr_ctx.mutable_placement().compressed_block_grids = create_compressed_block_grids();
}

} // namespace

TEST_CASE("compressed_cluster_constraints_keep_every_rectangle", "[vpr_place_constraints]") {
    t_physical_tile_type empty_tile;
    t_physical_tile_type io_tile;
    t_physical_tile_type small_tile;
    t_logical_block_type empty_logical_type;
    t_logical_block_type io_logical_type;
    t_logical_block_type small_logical_type;
    build_small_tile_device(empty_tile, io_tile, small_tile,
                            empty_logical_type, io_logical_type, small_logical_type);

    ClusteringContext& cluster_ctx = g_vpr_ctx.mutable_clustering();
    cluster_ctx.clb_nlist = ClusteredNetlist("test_netlist");
    t_logical_block_type_ptr small_type = &g_vpr_ctx.device().logical_block_types[2];
    ClusterBlockId two_rects = cluster_ctx.clb_nlist.create_block("two_rects", nullptr, small_type);
    ClusterBlockId one_rect = cluster_ctx.clb_nlist.create_block("one_rect", nullptr, small_type);
    ClusterBlockId unconstrained = cluster_ctx.clb_nlist.create_block("unconstrained", nullptr, small_type);

    // Two disjoint columns of small tiles, x 1..5 and x 12..18, both spanning y 1..18.
    // Small tiles start at grid x = 1, so compressed x = grid x - 1.
    FloorplanningContext& floorplanning_ctx = g_vpr_ctx.mutable_floorplanning();
    floorplanning_ctx.cluster_constraints.clear();
    floorplanning_ctx.cluster_constraints.resize(cluster_ctx.clb_nlist.blocks().size());
    PartitionRegion two;
    two.add_to_part_region(Region(1, 1, 5, 18, 0));
    two.add_to_part_region(Region(12, 1, 18, 18, 0));
    floorplanning_ctx.cluster_constraints[two_rects] = two;
    PartitionRegion one;
    one.add_to_part_region(Region(1, 1, 5, 18, 0));
    floorplanning_ctx.cluster_constraints[one_rect] = one;

    alloc_and_load_compressed_cluster_constraints();

    REQUIRE(floorplanning_ctx.compressed_cluster_constraints.size() == 1);
    const vtr::vector<ClusterBlockId, PartitionRegion>& compressed = floorplanning_ctx.compressed_cluster_constraints[0];

    SECTION("a partition with two rectangles keeps both, in order") {
        const std::vector<Region>& regions = compressed[two_rects].get_regions();
        REQUIRE(regions.size() == 2);
        const vtr::Rect<int>& first = regions[0].get_rect();
        REQUIRE(first.xmin() == 0);
        REQUIRE(first.xmax() == 4);
        REQUIRE(first.ymin() == 0);
        REQUIRE(first.ymax() == 17);
        const vtr::Rect<int>& second = regions[1].get_rect();
        REQUIRE(second.xmin() == 11);
        REQUIRE(second.xmax() == 17);
        REQUIRE(second.ymin() == 0);
        REQUIRE(second.ymax() == 17);
    }

    SECTION("a partition with one rectangle keeps one") {
        const std::vector<Region>& regions = compressed[one_rect].get_regions();
        REQUIRE(regions.size() == 1);
        REQUIRE(regions[0].get_rect().xmin() == 0);
        REQUIRE(regions[0].get_rect().xmax() == 4);
    }

    SECTION("an unconstrained block has no compressed partition") {
        REQUIRE(compressed[unconstrained].empty());
    }
}
