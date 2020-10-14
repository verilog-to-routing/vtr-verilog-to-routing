#include "catch.hpp"

#include "vpr_constraints.h"
#include "partition.h"
#include "partition_regions.h"
#include "region.h"

TEST_CASE("RegionAccessors", "[vpr]") {
    Region r1;

    r1.set_region_rect(1, 2, 3, 4);
    r1.set_sub_tile(2);

    REQUIRE(r1.get_xmin() == 1);
    REQUIRE(r1.get_ymin() == 2);
    REQUIRE(r1.get_xmax() == 3);
    REQUIRE(r1.get_ymax() == 4);
    REQUIRE(r1.get_sub_tile() == 2);
}

TEST_CASE("PartitionRegionsAccessors", "[vpr]") {
    Region r1;

    r1.set_region_rect(0, 0, 1, 1);
    r1.set_sub_tile(3);

    PartitionRegions pr1;

    pr1.add_to_part_regions(r1);

    std::vector<Region> pr_regions = pr1.get_partition_regions();
    REQUIRE(pr_regions[0].get_sub_tile() == 3);
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

    //create region and partitionregions objects to test accessors of the Partitions class
    Region r1;
    r1.set_region_rect(0, 0, 1, 1);
    r1.set_sub_tile(3);

    PartitionRegions part_reg;
    part_reg.add_to_part_regions(r1);

    part.set_part_regions(part_reg);
    PartitionRegions part_reg_2 = part.get_part_regions();
    std::vector<Region> regions = part_reg_2.get_partition_regions();

    REQUIRE(regions[0].get_sub_tile() == 3);
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
    Region region1;
    Region region2;

    region1.set_region_rect(0, 0, 2, 2);
    region2.set_region_rect(1, 1, 2, 2);

    Region int_reg;

    int_reg = region1.regions_intersection(region2);

    REQUIRE(int_reg.get_xmin() == 1);
    REQUIRE(int_reg.get_ymin() == 1);
    REQUIRE(int_reg.get_xmax() == 2);
    REQUIRE(int_reg.get_ymax() == 2);
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

    r1.set_region_rect(0, 1, 0, 1); //xmin = xmax = 0, ymin = ymax =1
    r1.set_sub_tile(3);             //set the region to specific x, y, subtile to lock down

    bool is_r1_locked = false;

    is_r1_locked = r1.locked();

    REQUIRE(is_r1_locked == true);
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
