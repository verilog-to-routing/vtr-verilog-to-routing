#include "../src/base/partition_region.h"
#include "catch.hpp"

#include "vpr_constraints.h"
#include "partition.h"
#include "region.h"

/**
 * This file contains unit tests that check the functionality of all classes related to vpr constraints. These classes include
 * VprConstraints, Region, PartitionRegions, and Partition.
 */

//Test Region class accessors and mutators
TEST_CASE("Region", "[vpr]") {
    Region r1;

    r1.set_region_rect(1, 2, 3, 4);
    r1.set_sub_tile(2);

    vtr::Rect<int> rect;
    rect = r1.get_region_rect();

    REQUIRE(rect.xmin() == 1);
    REQUIRE(rect.ymin() == 2);
    REQUIRE(rect.xmax() == 3);
    REQUIRE(rect.ymax() == 4);
    REQUIRE(r1.get_sub_tile() == 2);

    //checking that default constructor creates an empty rectangle (-1,-1,-1,-1)
    Region def_region;
    bool is_def_empty = false;

    vtr::Rect<int> def_rect = def_region.get_region_rect();
    is_def_empty = def_rect.empty();
    REQUIRE(is_def_empty == true);
    REQUIRE(def_rect.xmin() == -1);
}

//Test PartitionRegion class accessors and mutators
TEST_CASE("PartitionRegion", "[vpr]") {
    Region r1;

    r1.set_region_rect(2, 3, 6, 7);
    r1.set_sub_tile(3);

    PartitionRegion pr1;

    pr1.add_to_part_region(r1);

    std::vector<Region> pr_regions = pr1.get_partition_region();
    REQUIRE(pr_regions[0].get_sub_tile() == 3);
    vtr::Rect<int> rect;
    rect = pr_regions[0].get_region_rect();
    REQUIRE(rect.xmin() == 2);
    REQUIRE(rect.ymin() == 3);
    REQUIRE(rect.xmax() == 6);
    REQUIRE(rect.ymax() == 7);
}

//Test Partition class accessors and mutators
TEST_CASE("Partition", "[vpr]") {
    Partition part;

    part.set_name("part");
    REQUIRE(part.get_name() == "part");

    //create region and partitionregions objects to test functions of the Partition class
    Region r1;
    r1.set_region_rect(2, 3, 7, 8);
    r1.set_sub_tile(3);

    PartitionRegion part_reg;
    part_reg.add_to_part_region(r1);

    part.set_part_region(part_reg);
    PartitionRegion part_reg_2 = part.get_part_region();
    std::vector<Region> regions = part_reg_2.get_partition_region();

    REQUIRE(regions[0].get_sub_tile() == 3);
    vtr::Rect<int> rect;
    rect = regions[0].get_region_rect();
    REQUIRE(rect.xmin() == 2);
    REQUIRE(rect.ymin() == 3);
    REQUIRE(rect.xmax() == 7);
    REQUIRE(rect.ymax() == 8);
}

//Test VprConstraints class accessors and mutators
TEST_CASE("VprConstraints", "[vpr]") {
    PartitionId part_id(0);
    PartitionId part_id_2(1);
    AtomBlockId atom_id(6);
    AtomBlockId atom_id_2(7);
    AtomBlockId atom_id_3(8);
    AtomBlockId atom_id_4(9);

    VprConstraints vprcon;

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

    region1.set_region_rect(1, 2, 3, 5);
    region2.set_region_rect(2, 3, 4, 6);

    Region int_reg;

    int_reg = intersection(region1, region2);
    vtr::Rect<int> rect = int_reg.get_region_rect();

    REQUIRE(rect.xmin() == 2);
    REQUIRE(rect.ymin() == 3);
    REQUIRE(rect.xmax() == 3);
    REQUIRE(rect.ymax() == 5);

    //Test full overlap
    Region region3;
    Region region4;

    region3.set_region_rect(5, 1, 8, 6);
    region4.set_region_rect(6, 3, 8, 6);

    Region int_reg_2;

    int_reg_2 = intersection(region3, region4);
    vtr::Rect<int> rect_2 = int_reg_2.get_region_rect();

    REQUIRE(rect_2.xmin() == 6);
    REQUIRE(rect_2.ymin() == 3);
    REQUIRE(rect_2.xmax() == 8);
    REQUIRE(rect_2.ymax() == 6);

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
    vtr::Rect<int> rect_5 = int_reg_5.get_region_rect();
    REQUIRE(rect_5.xmin() == 2);
    REQUIRE(rect_5.ymin() == 3);
    REQUIRE(rect_5.xmax() == 3);
    REQUIRE(rect_5.ymax() == 5);
}

//The following six test cases test the intersection function for PartitionRegions
//2x1 regions, 2 overlap
TEST_CASE("PartRegionIntersect", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect(0, 0, 1, 1);
    r2.set_region_rect(1, 1, 2, 2);
    r3.set_region_rect(0, 0, 2, 2);

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();

    vtr::Rect<int> int_rect(0, 0, 1, 1);
    vtr::Rect<int> int_rect_2(1, 1, 2, 2);
    REQUIRE(regions[0].get_region_rect() == int_rect);
    REQUIRE(regions[1].get_region_rect() == int_rect_2);
}

//2x1 regions, 1 overlap
TEST_CASE("PartRegionIntersect2", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect(0, 0, 2, 2);
    r2.set_region_rect(4, 4, 6, 6);
    r3.set_region_rect(0, 0, 2, 2);

    pr1.add_to_part_region(r1);
    pr1.add_to_part_region(r2);
    pr2.add_to_part_region(r3);

    PartitionRegion int_pr;

    int_pr = intersection(pr1, pr2);
    std::vector<Region> regions = int_pr.get_partition_region();
    vtr::Rect<int> int_rect(0, 0, 2, 2);
    REQUIRE(regions.size() == 1);
    REQUIRE(regions[0].get_region_rect() == int_rect);
}

//2x2 regions, no overlaps
TEST_CASE("PartRegionIntersect3", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect(1, 2, 3, 5);
    r1.set_sub_tile(2);

    r2.set_region_rect(4, 2, 6, 4);

    r3.set_region_rect(4, 5, 5, 7);

    r4.set_region_rect(1, 2, 3, 5);
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

    r1.set_region_rect(1, 2, 3, 5);
    r1.set_sub_tile(2);

    r2.set_region_rect(4, 2, 6, 4);

    r3.set_region_rect(4, 5, 5, 7);

    r4.set_region_rect(1, 2, 3, 4);
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
    REQUIRE(regions[0].get_region_rect() == intersect);
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

    r1.set_region_rect(1, 5, 5, 7);

    r2.set_region_rect(6, 3, 8, 5);

    r3.set_region_rect(2, 6, 4, 9);

    r4.set_region_rect(6, 4, 8, 7);

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
    REQUIRE(regions[0].get_region_rect() == int_r1r3);
    REQUIRE(regions[1].get_region_rect() == int_r2r4);
}

//2x2 regions, 4 overlap
TEST_CASE("PartRegionIntersect6", "[vpr]") {
    PartitionRegion pr1;
    PartitionRegion pr2;

    Region r1;
    Region r2;
    Region r3;
    Region r4;

    r1.set_region_rect(2, 3, 4, 7);

    r2.set_region_rect(5, 3, 7, 8);

    r3.set_region_rect(2, 2, 7, 4);

    r4.set_region_rect(2, 6, 7, 8);

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
    REQUIRE(regions[0].get_region_rect() == int_r1r3);
    REQUIRE(regions[1].get_region_rect() == int_r1r4);
    REQUIRE(regions[2].get_region_rect() == int_r2r3);
    REQUIRE(regions[3].get_region_rect() == int_r2r4);
}

//Test the locked member function of the Region class
TEST_CASE("RegionLocked", "[vpr]") {
    Region r1;
    bool is_r1_locked = false;

    //set the region to a specific x, y, subtile location - region is locked
    r1.set_region_rect(2, 3, 2, 3); //point (2,3) to point (2,3) - locking to specific x, y location
    r1.set_sub_tile(3);             //locking down to subtile 3

    is_r1_locked = r1.locked();

    REQUIRE(is_r1_locked == true);

    //do not set region to specific x, y location - region is not locked even if a subtile is specified
    r1.set_region_rect(2, 3, 5, 6); //point (2,3) to point (5,6) - not locking to specific x, y location
    r1.set_sub_tile(3);             //locking down to subtile 3

    is_r1_locked = r1.locked();

    REQUIRE(is_r1_locked == false);

    //do not specify a subtile for the region - region is not locked even if it is set at specific x, y location
    Region r2;
    bool is_r2_locked = true;

    r2.set_region_rect(2, 3, 2, 3);

    is_r2_locked = r2.locked();

    REQUIRE(is_r2_locked == false);
}
