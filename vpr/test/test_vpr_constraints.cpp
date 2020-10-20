#include "catch.hpp"

#include "vpr_constraints.h"
#include "partition.h"
#include "partition_regions.h"
#include "region.h"

/**
 * This file contains unit tests that check the functionality of all classes related to vpr constraints. These classes include
 * VprConstraints, Region, PartitionRegions, and Partition.
 */

TEST_CASE("RegionAccessors", "[vpr]") {
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

TEST_CASE("PartitionRegionsAccessors", "[vpr]") {
    Region r1;

    r1.set_region_rect(2, 3, 6, 7);
    r1.set_sub_tile(3);

    PartitionRegions pr1;

    pr1.add_to_part_regions(r1);

    std::vector<Region> pr_regions = pr1.get_partition_regions();
    REQUIRE(pr_regions[0].get_sub_tile() == 3);
    vtr::Rect<int> rect;
    rect = pr_regions[0].get_region_rect();
    REQUIRE(rect.xmin() == 2);
    REQUIRE(rect.ymin() == 3);
    REQUIRE(rect.xmax() == 6);
    REQUIRE(rect.ymax() == 7);
}

TEST_CASE("PartitionAccessors", "[vpr]") {
    Partition part;

    part.set_name("part");
    REQUIRE(part.get_name() == "part");

    PartitionId part_id(6);
    part.set_partition_id(part_id);
    REQUIRE(part.get_partition_id() == part_id);

    AtomBlockId atom_1(3);
    AtomBlockId atom_2(5);
    part.add_to_atoms(atom_1);
    part.add_to_atoms(atom_2);
    std::vector<AtomBlockId> atoms = part.get_atoms();
    REQUIRE(atoms[0] == atom_1);
    REQUIRE(atoms[1] == atom_2);
    REQUIRE(part.contains_atom(atom_1) == true);
    REQUIRE(part.contains_atom(atom_2) == true);

    //create region and partitionregions objects to test accessors of the Partition class
    Region r1;
    r1.set_region_rect(2, 3, 7, 8);
    r1.set_sub_tile(3);

    PartitionRegions part_reg;
    part_reg.add_to_part_regions(r1);

    part.set_part_regions(part_reg);
    PartitionRegions part_reg_2 = part.get_part_regions();
    std::vector<Region> regions = part_reg_2.get_partition_regions();

    REQUIRE(regions[0].get_sub_tile() == 3);
    vtr::Rect<int> rect;
    rect = regions[0].get_region_rect();
    REQUIRE(rect.xmin() == 2);
    REQUIRE(rect.ymin() == 3);
    REQUIRE(rect.xmax() == 7);
    REQUIRE(rect.ymax() == 8);
}

TEST_CASE("VprConstraintsAccessors", "[vpr]") {
    PartitionId part_id(0);
    PartitionId part_id_2(1);
    AtomBlockId atom_id(6);
    AtomBlockId atom_id_2(7);
    AtomBlockId atom_id_3(8);

    VprConstraints vprcon;

    vprcon.add_constrained_atom(atom_id, part_id);
    vprcon.add_constrained_atom(atom_id_2, part_id);
    vprcon.add_constrained_atom(atom_id_3, part_id_2);

    REQUIRE(vprcon.get_atom_partition(atom_id) == part_id);
    REQUIRE(vprcon.get_atom_partition(atom_id_2) == part_id);
    REQUIRE(vprcon.get_atom_partition(atom_id_3) == part_id_2);

    Partition part;
    part.set_partition_id(part_id);
    part.set_name("part_name");
    part.add_to_atoms(atom_id);
    part.add_to_atoms(atom_id_2);

    vprcon.add_partition(part);
    vtr::vector<PartitionId, Partition> parts = vprcon.get_partitions();
    REQUIRE(parts[part_id].get_name() == "part_name");
}

TEST_CASE("RegionIntersect", "[vpr]") {
    //Test partial intersection
    Region region1;
    Region region2;

    region1.set_region_rect(1, 2, 3, 5);
    region2.set_region_rect(2, 3, 4, 6);

    Region int_reg;

    int_reg = region1.regions_intersection(region2);
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

    int_reg_2 = region3.regions_intersection(region4);
    vtr::Rect<int> rect_2 = int_reg_2.get_region_rect();

    REQUIRE(rect_2.xmin() == 6);
    REQUIRE(rect_2.ymin() == 3);
    REQUIRE(rect_2.xmax() == 8);
    REQUIRE(rect_2.ymax() == 6);

    //Test no intersection (rect is empty)

    Region int_reg_3;

    int_reg_3 = region1.regions_intersection(region3);
    vtr::Rect<int> rect_3 = int_reg_3.get_region_rect();

    REQUIRE(rect_3.empty() == TRUE);
}

TEST_CASE("PartRegionIntersect", "[vpr]") {
    PartitionRegions pr1;
    PartitionRegions pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect(0, 0, 1, 1);
    r2.set_region_rect(1, 1, 2, 2);
    r3.set_region_rect(0, 0, 2, 2);

    pr1.add_to_part_regions(r1);
    pr1.add_to_part_regions(r2);
    pr2.add_to_part_regions(r3);

    PartitionRegions int_pr;

    int_pr = pr1.get_intersection(pr2);
    std::vector<Region> regions = int_pr.get_partition_regions();

    REQUIRE(regions[0].get_xmin() == 0);
    REQUIRE(regions[0].get_ymin() == 0);
    REQUIRE(regions[0].get_xmax() == 1);
    REQUIRE(regions[0].get_ymax() == 1);

    REQUIRE(regions[1].get_xmin() == 1);
    REQUIRE(regions[1].get_ymin() == 1);
    REQUIRE(regions[1].get_xmax() == 2);
    REQUIRE(regions[1].get_ymax() == 2);
}

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

TEST_CASE("PartRegionIntersect2", "[vpr]") {
    PartitionRegions pr1;
    PartitionRegions pr2;

    Region r1;
    Region r2;
    Region r3;

    r1.set_region_rect(0, 0, 2, 2);
    r2.set_region_rect(4, 4, 6, 6);
    r3.set_region_rect(0, 0, 2, 2);

    pr1.add_to_part_regions(r1);
    pr1.add_to_part_regions(r2);
    pr2.add_to_part_regions(r3);

    PartitionRegions int_pr;

    int_pr = pr1.get_intersection(pr2);
    std::vector<Region> regions = int_pr.get_partition_regions();

    REQUIRE(regions.size() == 1);
    REQUIRE(regions[0].get_xmin() == 0);
    REQUIRE(regions[0].get_ymin() == 0);
    REQUIRE(regions[0].get_xmax() == 2);
    REQUIRE(regions[0].get_ymax() == 2);
}
