/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Unit tests for the APNetlist class.
 *
 * These are very quick functionality checks to ensure that the APNetlist is
 * working as intended. These tests mainly focus on new features in the
 * APNetlist which are not in the base Netlist class.
 */
#include "catch2/catch_test_macros.hpp"

#include "ap_netlist.h"
#include "vpr_types.h"

namespace {

TEST_CASE("test_ap_netlist_data_storage", "[vpr_ap_netlist]") {
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

    SECTION("Test block_molecule returns the correct molecule after creation") {
        REQUIRE(test_netlist.block_molecule(block_id_a) == &mol_a);
        REQUIRE(test_netlist.block_molecule(block_id_b) == &mol_b);
        REQUIRE(test_netlist.block_molecule(block_id_c) == &mol_c);
    }

    // Delete block B to reorganize the blocks internally.
    test_netlist.remove_block(block_id_b);
    test_netlist.compress();
    // Get the new block ids (invalidated by compress).
    block_id_a = test_netlist.find_block("BlockA");
    block_id_b = APBlockId::INVALID();
    block_id_c = test_netlist.find_block("BlockC");

    SECTION("Test block_molecule returns the correct molecule after compression") {
        REQUIRE(test_netlist.block_molecule(block_id_a) == &mol_a);
        REQUIRE(test_netlist.block_molecule(block_id_c) == &mol_c);
    }

    // Create a new block, and fix its location.
    t_pack_molecule fixed_mol;
    APBlockId fixed_block_id = test_netlist.create_block("FixedBlock", &fixed_mol);
    APFixedBlockLoc fixed_block_loc;
    fixed_block_loc.x = 12;
    fixed_block_loc.y = 42;
    fixed_block_loc.layer_num = 2;
    fixed_block_loc.sub_tile = 1;
    test_netlist.set_block_loc(fixed_block_id, fixed_block_loc);

    SECTION("Test block_type returns the correct block type") {
        // Make sure the default block mobility is moveable.
        REQUIRE(test_netlist.block_mobility(block_id_a) == APBlockMobility::MOVEABLE);
        REQUIRE(test_netlist.block_mobility(block_id_c) == APBlockMobility::MOVEABLE);
        // Make sure the fixed block has a fixed mobility.
        REQUIRE(test_netlist.block_mobility(fixed_block_id) == APBlockMobility::FIXED);
    }

    SECTION("Test block_loc returns the correct location") {
        const APFixedBlockLoc& stored_loc = test_netlist.block_loc(fixed_block_id);
        REQUIRE(stored_loc.x == fixed_block_loc.x);
        REQUIRE(stored_loc.y == fixed_block_loc.y);
        REQUIRE(stored_loc.layer_num == fixed_block_loc.layer_num);
        REQUIRE(stored_loc.sub_tile == fixed_block_loc.sub_tile);
    }
}

} // namespace

