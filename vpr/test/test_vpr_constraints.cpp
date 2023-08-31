#include "../src/base/partition_region.h"

#include "catch2/catch_test_macros.hpp"

#include "vpr_api.h"
#include "globals.h"
#include "user_place_constraints.h"
#include "partition.h"
#include "region.h"
#include "place_constraints.h"

/**
 * This file contains unit tests that check the functionality of all classes related to vpr constraints. These classes include
 * UserPlaceConstraints, Region, PartitionRegions, and Partition.
 */

//Test Region class accessors and mutators
TEST_CASE("Region", "[vpr]") {
    Region r1;

    r1.set_region_rect({1, 2, 3, 4, 5});
    r1.set_sub_tile(2);

    const auto r1_coord = r1.get_region_rect();

    REQUIRE(r1_coord.xmin == 1);
    REQUIRE(r1_coord.ymin == 2);
    REQUIRE(r1_coord.xmax == 3);
    REQUIRE(r1_coord.ymax == 4);
    REQUIRE(r1_coord.layer_num == 5);
    REQUIRE(r1.get_sub_tile() == 2);

    //checking that default constructor creates an empty rectangle (999, 999,-1,-1)
    Region def_region;
    bool is_def_empty = false;

    const auto def_coord = def_region.get_region_rect();
    is_def_empty = def_region.empty();
    REQUIRE(is_def_empty == true);
    REQUIRE(def_coord.xmin == 999);
    REQUIRE(def_coord.layer_num == -1);
    REQUIRE(def_region.get_sub_tile() == -1);
}

//Test PartitionRegion class accessors and mutators
TEST_CASE("PartitionRegion", "[vpr]") {
    Region r1;

    r1.set_region_rect({2, 3, 6, 7, 0});
    r1.set_sub_tile(3);

    PartitionRegion pr1;

    pr1.add_to_part_region(r1);

    std::vector<Region> pr_regions = pr1.get_partition_region();
    REQUIRE(pr_regions[0].get_sub_tile() == 3);

    const auto pr_reg_coord = pr_regions[0].get_region_rect();
    REQUIRE(pr_reg_coord.layer_num == 0);
    REQUIRE(pr_reg_coord.xmin == 2);
    REQUIRE(pr_reg_coord.ymin == 3);
    REQUIRE(pr_reg_coord.xmax == 6);
    REQUIRE(pr_reg_coord.ymax == 7);
}

//Test Partition class accessors and mutators
TEST_CASE("Partition", "[vpr]") {
    Partition part;

    part.set_name("part");
    REQUIRE(part.get_name() == "part");

    //create region and partitionregions objects to test functions of the Partition class
    Region r1;
    r1.set_region_rect({2, 3, 7, 8, 0});
    r1.set_sub_tile(3);

    PartitionRegion part_reg;
    part_reg.add_to_part_region(r1);

    part.set_part_region(part_reg);
    PartitionRegion part_reg_2 = part.get_part_region();
    std::vector<Region> regions = part_reg_2.get_partition_region();

    REQUIRE(regions[0].get_sub_tile() == 3);

    const auto pr_reg_coord = regions[0].get_region_rect();
    REQUIRE(pr_reg_coord.layer_num == 0);
    REQUIRE(pr_reg_coord.xmin == 2);
    REQUIRE(pr_reg_coord.ymin == 3);
    REQUIRE(pr_reg_coord.xmax == 7);
    REQUIRE(pr_reg_coord.ymax == 8);
}

//Test UserPlaceConstraints class accessors and mutators
TEST_CASE("UserPlaceConstraints", "[vpr]") {
    PartitionId part_id(0);
    PartitionId part_id_2(1);
    AtomBlockId atom_id(6);
    AtomBlockId atom_id_2(7);
    AtomBlockId atom_id_3(8);
    AtomBlockId atom_id_4(9);

    UserPlaceConstraints vprcon;

    vprcon.add_constrained_atom(atom_id, part_id);
    vprcon.add_constrained_atom(atom_id_2, part_id);
    vprcon.add_constrained_atom(atom_id_3, part_id_2);

    REQUIRE(vprcon.get_atom_partition(atom_id) == part_id);
    REQUIRE(vprcon.get_atom_partition(atom_id_2) == part_id);
    REQUIRE(vprcon.get_atom_partition(atom_id_3) == part_id_2);
    REQUIRE(vprcon.get_atom_partition(atom_id_4) == PartitionId::INVALID());

    vprcon.add_constrained_atom(atom_id_3, part_id);
    REQUIRE(vprcon.get_atom_partition(atom_id_3) == part_id);

    Partition part;
    part.set_name("part_name");

    vprcon.add_partition(part);

    Partition got_part;
    got_part = vprcon.get_partition(part_id);
    REQUIRE(got_part.get_name() == "part_name");

    std::vector<AtomBlockId> partition_atoms;

    partition_atoms = vprcon.get_part_atoms(part_id);
    REQUIRE(partition_atoms.size() == 3);
}

//Test intersection function for Regions
TEST_CASE("RegionIntersect", "[vpr]") {
    //Test partial intersection
    Region region1;
    Region region2;

    region1.set_region_rect({1, 2, 3, 5, 0});
    region2.set_region_rect({2, 3, 4, 6, 0});

    Region int_reg;

    int_reg = intersection(region1, region2);
    auto intersect_reg_coord = int_reg.get_region_rect();

    REQUIRE(intersect_reg_coord.layer_num == 0);
    REQUIRE(intersect_reg_coord.xmin == 2);
    REQUIRE(intersect_reg_coord.ymin == 3);
    REQUIRE(intersect_reg_coord.xmax == 3);
    REQUIRE(intersect_reg_coord.ymax == 5);

    //Test full overlap
    Region region3;
    Region region4;

    region3.set_region_rect({5, 1, 8, 6, 0});
    region4.set_region_rect({6, 3, 8, 6, 0});

    Region int_reg_2;

    int_reg_2 = intersection(region3, region4);
    intersect_reg_coord = int_reg_2.get_region_rect();

    REQUIRE(intersect_reg_coord.layer_num == 0);
    REQUIRE(intersect_reg_coord.xmin == 6);
    REQUIRE(intersect_reg_coord.ymin == 3);
    REQUIRE(intersect_reg_coord.xmax == 8);
    REQUIRE(intersect_reg_coord.ymax == 6);

    //Test no intersection (rectangles don't overlap, intersect region will be returned empty)

    Region int_reg_3;

    int_reg_3 = intersection(region1, region3);

    REQUIRE(int_reg_3.empty() == TRUE);

    //Test no intersection (rectangles overlap but different subtiles are specified, intersect region will be returned empty)
    region1.set_sub_tile(5);
    region2.set_sub_tile(3);

    Region int_reg_4;
    int_reg_4 = intersection(region1, region2);

    REQUIRE(int_reg_4.empty() == TRUE);

    //Test intersection where subtiles are the same and equal to something other than the INVALID value
    region1.set_sub_tile(6);
    region2.set_sub_tile(6);

    Region int_reg_5;
    int_reg_5 = intersection(region1, region2);
    const auto reg_5_coord = int_reg_5.get_region_rect();
    REQUIRE(reg_5_coord.layer_num == 0);
    REQUIRE(reg_5_coord.xmin == 2);
    REQUIRE(reg_5_coord.ymin == 3);
    REQUIRE(reg_5_coord.xmax == 3);
    REQUIRE(reg_5_coord.ymax == 5);
}

//The following six test cases test the intersection function for PartitionRegions
//2x1 regions, 2 overlap
TEST_CASE("PartRegionIntersect", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect({0,
                        0,
                        1,
                        1,
                        0});

    r2.set_region_rect({1,
                        1,
                        2,
                        2,
                        0});

    r3.set_region_rect({0,
                        0,
                        2,
                        2,
                        0});

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    vtr::Rect<int> int_rect(0, 0, 1, 1);
    vtr::Rect<int> int_rect_2(1, 1, 2, 2);

    const auto first_reg_coord = regions[0].get_region_rect();
    const auto second_reg_coord = regions[1].get_region_rect();
    REQUIRE(vtr::Rect<int>(first_reg_coord.xmin, first_reg_coord.ymin, first_reg_coord.xmax, first_reg_coord.ymax) == int_rect);
    REQUIRE(vtr::Rect<int>(second_reg_coord.xmin, second_reg_coord.ymin, second_reg_coord.xmax, second_reg_coord.ymax) == int_rect_2);
    REQUIRE(first_reg_coord.layer_num == 0);
    REQUIRE(second_reg_coord.layer_num == 0);
}

//2x1 regions, 1 overlap
TEST_CASE("PartRegionIntersect2", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect({0, 0, 2, 2, 0});
    r2.set_region_rect({4, 4, 6, 6, 0});
    r3.set_region_rect({0, 0, 2, 2, 0});

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();
    vtr::Rect<int> int_rect(0, 0, 2, 2);
    REQUIRE(regions.size() == 1);
    const auto first_reg_coord = regions[0].get_region_rect();
    REQUIRE(vtr::Rect<int>(first_reg_coord.xmin, first_reg_coord.ymin, first_reg_coord.xmax, first_reg_coord.ymax) == int_rect);
    REQUIRE(first_reg_coord.layer_num == 0);
}

//2x2 regions, no overlaps
TEST_CASE("PartRegionIntersect3", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect({1, 2, 3, 5, 0});
    r1.set_sub_tile(2);

    r2.set_region_rect({4, 2, 6, 4, 0});

    r3.set_region_rect({4, 5, 5, 7, 0});

    r4.set_region_rect({1, 2, 3, 5, 0});
    r4.set_sub_tile(4);

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);
    pr2.add_to_part_region(r4);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    REQUIRE(regions.size() == 0);
}

//2x2 regions, 1 overlap
TEST_CASE("PartRegionIntersect4", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect({1, 2, 3, 5, 0});
    r1.set_sub_tile(2);

    r2.set_region_rect({4, 2, 6, 4, 0});

    r3.set_region_rect({4, 5, 5, 7, 0});

    r4.set_region_rect({1, 2, 3, 4, 0});
    r4.set_sub_tile(2);

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);
    pr2.add_to_part_region(r4);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    vtr::Rect<int> intersect(1, 2, 3, 4);

    REQUIRE(regions.size() == 1);
    const auto first_reg_coord = regions[0].get_region_rect();
    REQUIRE(first_reg_coord.layer_num == 0);
    REQUIRE(first_reg_coord.get_rect() == intersect);
    REQUIRE(regions[0].get_sub_tile() == 2);
}

//2x2 regions, 2 overlap
TEST_CASE("PartRegionIntersect5", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect({1, 5, 5, 7, 0});

    r2.set_region_rect({6, 3, 8, 5, 0});

    r3.set_region_rect({2, 6, 4, 9, 0});

    r4.set_region_rect({6, 4, 8, 7, 0});

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);
    pr2.add_to_part_region(r4);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    vtr::Rect<int> int_r1r3(2, 6, 4, 7);
    vtr::Rect<int> int_r2r4(6, 4, 8, 5);

    REQUIRE(regions.size() == 2);
    const auto first_reg_coord = regions[0].get_region_rect();
    const auto second_reg_coord = regions[1].get_region_rect();

    REQUIRE(first_reg_coord.layer_num == 0);
    REQUIRE(second_reg_coord.layer_num == 0);
    REQUIRE(first_reg_coord.get_rect() == int_r1r3);
    REQUIRE(second_reg_coord.get_rect() == int_r2r4);
}

//2x2 regions, 4 overlap
TEST_CASE("PartRegionIntersect6", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect({2, 3, 4, 7, 0});

    r2.set_region_rect({5, 3, 7, 8, 0});

    r3.set_region_rect({2, 2, 7, 4, 0});

    r4.set_region_rect({2, 6, 7, 8, 0});

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);
    pr2.add_to_part_region(r4);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    vtr::Rect<int> int_r1r3(2, 3, 4, 4);
    vtr::Rect<int> int_r1r4(2, 6, 4, 7);
    vtr::Rect<int> int_r2r3(5, 3, 7, 4);
    vtr::Rect<int> int_r2r4(5, 6, 7, 8);

    REQUIRE(regions.size() == 4);
    REQUIRE(regions[0].get_region_rect().get_rect() == int_r1r3);
    REQUIRE(regions[1].get_region_rect().get_rect() == int_r1r4);
    REQUIRE(regions[2].get_region_rect().get_rect() == int_r2r3);
    REQUIRE(regions[3].get_region_rect().get_rect() == int_r2r4);

    REQUIRE(regions[0].get_region_rect().layer_num == 0);
    REQUIRE(regions[1].get_region_rect().layer_num == 0);
    REQUIRE(regions[2].get_region_rect().layer_num == 0);
    REQUIRE(regions[3].get_region_rect().layer_num == 0);
}

//Test calculation of macro constraints
/* Checks that the PartitionRegion of a macro member is updated properly according
 * to the head member's PartitionRegion that is passed in.
 */
TEST_CASE("MacroConstraints", "[vpr]") {
    t_pl_macro pl_macro;
    PartitionRegion head_pr;
    t_pl_offset offset(2, 1, 0);

    Region reg;
    reg.set_region_rect({5, 2, 9, 6, 0});

    head_pr.add_to_part_region(reg);

    Region grid_reg;
    grid_reg.set_region_rect({0, 0, 20, 20, 0});
    PartitionRegion grid_pr;
    grid_pr.add_to_part_region(grid_reg);

    PartitionRegion macro_pr = update_macro_member_pr(head_pr, offset, grid_pr, pl_macro);

    std::vector<Region> mac_regions = macro_pr.get_partition_region();

    const auto mac_first_reg_coord = mac_regions[0].get_region_rect();

    REQUIRE(mac_first_reg_coord.layer_num == 0);
    REQUIRE(mac_first_reg_coord.xmin == 7);
    REQUIRE(mac_first_reg_coord.ymin == 3);
    REQUIRE(mac_first_reg_coord.xmax == 11);
    REQUIRE(mac_first_reg_coord.ymax == 7);
}

#if 0
static constexpr const char kArchFile[] = "test_read_arch_metadata.xml";

// Test that place constraints are not changed during placement
TEST_CASE("PlaceConstraintsIntegrity", "[vpr]") {
    auto options = t_options();
    auto arch = t_arch();
    auto vpr_setup = t_vpr_setup();

    vpr_initialize_logging();

    // Command line arguments
    //
    // parameters description:
    //      - place_static_move_prob: Timing Feasible Region move type is always selected.
    //      - RL_agent_placement: disabled. This way the desired move type is selected.
    const char* argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--fix_clusters", "wire.constraints",
        "--place_static_move_prob", "0", "0", "0", "0", "0", "100", "0",
        "--RL_agent_placement", "off"};
    vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
             &options, &vpr_setup, &arch);

    vpr_pack_flow(vpr_setup, arch);
    vpr_create_device(vpr_setup, arch);
    vpr_place_flow(vpr_setup, arch);

    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    // Check if constraints are preserved

    // Input block
    ClusterBlockId input_blk_id = cluster_ctx.clb_nlist.find_block("di");
    int input_x = 2;
    int input_y = 1;
    int input_sub_tile = 5;

    auto input_loc = place_ctx.block_locs[input_blk_id].loc;

    REQUIRE(input_x == input_loc.x);
    REQUIRE(input_y == input_loc.y);
    REQUIRE(input_sub_tile == input_loc.sub_tile);

    // Output block
    ClusterBlockId output_blk_id = cluster_ctx.clb_nlist.find_block("out:do");
    int output_x = 2;
    int output_y = 1;
    int output_sub_tile = 1;

    auto output_loc = place_ctx.block_locs[output_blk_id].loc;

    REQUIRE(output_x == output_loc.x);
    REQUIRE(output_y == output_loc.y);
    REQUIRE(output_sub_tile == output_loc.sub_tile);

    vpr_free_all(arch, vpr_setup);
}
#endif
