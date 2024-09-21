/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   The declaration of the PartialPlacement object
 *
 * The partial placement object stores the placement of the APNetlist blocks
 * during different stages of the Analytical Placement flow. The Partial
 * Placement need not represent a legal placement; however, the placed blocks
 * will always be on the device and will respect fixed block locations.
 */

#pragma once

#include "ap_netlist.h"
#include "vtr_vector.h"

/**
 * @brief A partial placement during the Analytical Placement flow
 *
 * The goal of this class is to contain positional information about the blocks
 * that gets passed around during analytical placement.
 *
 * The placement of a given APBlock is given by the following values:
 *  - x_loc         The x-position of the APBlock on the device grid
 *  - y_loc         The y-position of the APBlock on the device grid
 *  - layer_num     The layer on the device grid this block is on
 *  - sub_tile      The sub-tile this APBlock wants to be a part of
 * TODO: Not sure if we want to keep the sub tile, but it may be useful for
 *       transferring information from the PartialLegalizer to the FullLegalizer.
 *
 * x_loc, y_loc, and layer_num are represented by doubles since APBlocks can be
 * placed in continuous space, rather than constrained to the integer device
 * grid of legal locations. These are doubles rather than floats since they
 * better fit the matrix solver we currently use.
 *
 * layer_num is represented by an int since it is not decided by an analytical
 * solver, but rather by the legalizers; so it does not need to be in continuous
 * space.
 *
 * This object assumes that the APNetlist object is static during the operation
 * of Analytical Placement (no blocks are added or removed).
 *
 * Note: The placement information was stored in this object as a struct of
 *       arrays instead of an array of structs since it is assumed that the
 *       main operations being performed on this placement will be analytical;
 *       thus the x positions and y positions will likely be stored and computed
 *       as separate continuous vectors.
 */
struct PartialPlacement {
    /// @brief The x locations of all the APBlocks
    vtr::vector<APBlockId, double> block_x_locs;
    /// @brief The y locations of all the APBlocks
    vtr::vector<APBlockId, double> block_y_locs;
    /// @brief The layers of the device of all the APBlocks
    vtr::vector<APBlockId, double> block_layer_nums;
    /// @brief The sub tiles of all the APBlocks
    vtr::vector<APBlockId, int> block_sub_tiles;

    // Remove the default constructor. Need to construct this using an APNetlist.
    PartialPlacement() = delete;

    /**
     * @brief Construct the partial placement
     *
     * Uses the APNetlist object to allocate space for the locations of all
     * APBlocks.
     *
     *  @param netlist  The APNetlist which contains the blocks to be placed.
     */
    PartialPlacement(const APNetlist& netlist)
            : block_x_locs(netlist.blocks().size(), -1.0),
              block_y_locs(netlist.blocks().size(), -1.0),
              block_layer_nums(netlist.blocks().size(), 0.0),
              block_sub_tiles(netlist.blocks().size(), 0) {
        // Note: All blocks are initialized to:
        //      x_loc = -1.0
        //      y_loc = -1.0
        //      layer_num = 0.0
        //      sub_tile = 0
        // Load the fixed block locations
        for (APBlockId blk_id : netlist.blocks()) {
            if (netlist.block_mobility(blk_id) != APBlockMobility::FIXED)
                continue;
            const APFixedBlockLoc &loc = netlist.block_loc(blk_id);
            if (loc.x != -1)
                block_x_locs[blk_id] = loc.x;
            if (loc.y != -1)
                block_y_locs[blk_id] = loc.y;
            if (loc.layer_num != -1)
                block_layer_nums[blk_id] = loc.layer_num;
            if (loc.sub_tile != -1)
                block_sub_tiles[blk_id] = loc.sub_tile;
        }
    }

    /**
     * @brief Verify the block_x_locs and block_y_locs vectors
     *
     * Currently ensures:
     *  - All blocks have locs
     *  - All blocks are on the device
     *  - All fixed locs are correct
     *
     *  @param netlist      The APNetlist used to generate this placement
     *  @param grid_width   The width of the device grid
     *  @param grid_height  The height of the device grid
     */
    bool verify_locs(const APNetlist& netlist,
                     size_t grid_width,
                     size_t grid_height);

    /**
     * @brief Verify the block_layer_nums vector
     *
     * Currently ensures:
     *  - All blocks have layer_nums
     *  - All blocks are on the device
     *  - All fixed layers are correct
     *
     *  @param netlist          The APNetlist used to generate this placement
     *  @param grid_num_layers  The number of layers in the device grid
     */
    bool verify_layer_nums(const APNetlist& netlist, size_t grid_num_layers);

    /**
     * @brief Verify the sub_tiles
     *
     * Currently ensures:
     *  - All sub_tiles are non-negative (implies unset)
     *
     *  @param netlist  The APNetlist used to generate this placement
     */
    bool verify_sub_tiles(const APNetlist& netlist);

    /**
     * @brief Verify the entire partial placement object
     *
     * Runs all of the verification checks above.
     *
     *  @param netlist          The APNetlist used to generate this placement
     *  @param grid_width       The width of the device grid
     *  @param grid_height      The height of the device grid
     *  @param grid_num_layers  The number of layers in the device grid
     */
    bool verify(const APNetlist& netlist,
                size_t grid_width,
                size_t grid_height,
                size_t grid_num_layers);
};

