/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Unit tests for the PartialPlacement object
 *
 * Very quick functionality checks to make sure that the methods inside of the
 * PartialPlacement object are working as expected.
 */

#include "catch2/catch_test_macros.hpp"

#include "ap_netlist.h"
#include "partial_placement.h"
#include "vpr_types.h"

namespace {

TEST_CASE("test_ap_partial_placement_verify", "[vpr_ap]") {
    // Create a test netlist object.
    APNetlist test_netlist("test_netlist");
    // Create a few molecules.
    t_pack_molecule mol_a;
    t_pack_molecule mol_b;
    t_pack_molecule mol_c;
    // Create blocks for these molecules.
    APBlockId block_id_a = test_netlist.create_block("BlockA", &mol_a);
    APBlockId block_id_b = test_netlist.create_block("BlockB", &mol_b);
    APBlockId block_id_c = test_netlist.create_block("BlockC", &mol_c);
    // Fix BlockC.
    APFixedBlockLoc fixed_block_loc;
    fixed_block_loc.x = 12;
    fixed_block_loc.y = 42;
    fixed_block_loc.layer_num = 2;
    fixed_block_loc.sub_tile = 1;
    test_netlist.set_block_loc(block_id_c, fixed_block_loc);

    // Create the PartialPlacement object.
    PartialPlacement test_placement(test_netlist);

    SECTION("Test constructor places fixed blocks correctly") {
        REQUIRE(test_placement.block_x_locs[block_id_c] == fixed_block_loc.x);
        REQUIRE(test_placement.block_y_locs[block_id_c] == fixed_block_loc.y);
        REQUIRE(test_placement.block_layer_nums[block_id_c] == fixed_block_loc.layer_num);
        REQUIRE(test_placement.block_sub_tiles[block_id_c] == fixed_block_loc.sub_tile);
    }

    // Place the blocks
    //  Place BlockA
    test_placement.block_x_locs[block_id_a] = 0;
    test_placement.block_y_locs[block_id_a] = 0;
    test_placement.block_layer_nums[block_id_a] = 0;
    test_placement.block_sub_tiles[block_id_a] = 0;
    //  Place BlockB
    test_placement.block_x_locs[block_id_b] = 0;
    test_placement.block_y_locs[block_id_b] = 0;
    test_placement.block_layer_nums[block_id_b] = 0;
    test_placement.block_sub_tiles[block_id_b] = 0;

    SECTION("Test verify returns true when the placement is valid") {
        // NOTE: Using a very large device.
        REQUIRE(test_placement.verify(test_netlist, 100, 100, 100));
        // Picking sizes that just fit.
        REQUIRE(test_placement.verify(test_netlist, 13, 100, 100));
        REQUIRE(test_placement.verify(test_netlist, 100, 43, 100));
        REQUIRE(test_placement.verify(test_netlist, 100, 100, 3));
        REQUIRE(test_placement.verify(test_netlist, 13, 43, 3));
    }

    SECTION("Test verify methods all return true when verify returns true") {
        REQUIRE(test_placement.verify_locs(test_netlist, 100, 100));
        REQUIRE(test_placement.verify_layer_nums(test_netlist, 100));
        REQUIRE(test_placement.verify_sub_tiles(test_netlist));
    }

    SECTION("Test verify returns false when blocks are outside grid") {
        // NOTE: Picking device sizes that are just small enough that BlockC
        //       is off the device.
        REQUIRE(!test_placement.verify_locs(test_netlist, 100, 1));
        REQUIRE(!test_placement.verify_locs(test_netlist, 1, 100));
        REQUIRE(!test_placement.verify_layer_nums(test_netlist, 1));
        // Make sure that the verify method also catches these cases
        REQUIRE(!test_placement.verify(test_netlist, 100, 1, 100));
        REQUIRE(!test_placement.verify(test_netlist, 1, 100, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 1));
        // Move BlockA off the grid into the negative direction
        test_placement.block_x_locs[block_id_a] = -1;
        REQUIRE(!test_placement.verify_locs(test_netlist, 100, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_x_locs[block_id_a] = 0;
        test_placement.block_y_locs[block_id_a] = -1;
        REQUIRE(!test_placement.verify_locs(test_netlist, 100, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_y_locs[block_id_a] = 0;
        test_placement.block_layer_nums[block_id_a] = -1;
        REQUIRE(!test_placement.verify_layer_nums(test_netlist, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_layer_nums[block_id_a] = 0;
        test_placement.block_sub_tiles[block_id_a] = -1;
        REQUIRE(!test_placement.verify_sub_tiles(test_netlist));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_sub_tiles[block_id_a] = 0;
        // Make sure everything is valid again
        REQUIRE(test_placement.verify(test_netlist, 100, 100, 100));
    }

    SECTION("Test verify returns false when fixed blocks are moved") {
        // Move BlockC's x-coordinate
        test_placement.block_x_locs[block_id_c] = 0;
        REQUIRE(!test_placement.verify_locs(test_netlist, 100, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_x_locs[block_id_c] = fixed_block_loc.x;
        // Move BlockC's y-coordinate
        test_placement.block_y_locs[block_id_c] = 0;
        REQUIRE(!test_placement.verify_locs(test_netlist, 100, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_y_locs[block_id_c] = fixed_block_loc.y;
        // Move BlockC's layer num
        test_placement.block_layer_nums[block_id_c] = 0;
        REQUIRE(!test_placement.verify_layer_nums(test_netlist, 100));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_layer_nums[block_id_c] = fixed_block_loc.layer_num;
        // Move BlockC's sub tile
        test_placement.block_sub_tiles[block_id_c] = 0;
        REQUIRE(!test_placement.verify_sub_tiles(test_netlist));
        REQUIRE(!test_placement.verify(test_netlist, 100, 100, 100));
        test_placement.block_sub_tiles[block_id_c] = fixed_block_loc.sub_tile;
        // Make sure everything was put back correctly.
        REQUIRE(test_placement.verify(test_netlist, 100, 100, 100));
    }
}

} // namespace

