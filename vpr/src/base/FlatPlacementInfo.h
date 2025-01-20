/**
 * @file
 * @author  Alex Singer
 * @date    January 2025
 * @brief   Declaration of the FlatPlacementInfo object, which is used to store
 *          flat placement information used by the packer.
 */

#pragma once

#include "atom_netlist.h"
#include "vtr_vector.h"

/**
 * @brief Flat placement storage class.
 *
 * This stores placement information for each atom in the netlist. It contains
 * any information that may be used by the packer to better create clusters.
 */
class FlatPlacementInfo {
public:
    /// @brief Identifier for an undefined position.
    static constexpr float UNDEFINED_POS = -1.f;
    /// @brief Identifier for an undefined sub tile.
    static constexpr int UNDEFINED_SUB_TILE = -1;
    /// @brief Identifier for an undefined site idx.
    static constexpr int UNDEFINED_SITE_IDX = -1;

    // The following three floating point numbers describe the flat position of
    // an atom block. These are floats instead of integers to allow for flat
    // placements which are not quite legal (ok to be off-grid). This allows
    // the flat placement to encode information about where atom blocks would
    // want to go if they cannot be placed at the grid position they are at.
    // (for example, a block placed at (0.9, 0.9) wants to be at tile (0, 0),
    // but if thats not possible it would prefer (1, 1) over anything else.

    /// @brief The x-positions of each atom block. Is UNDEFINED_POS if undefined.
    vtr::vector<AtomBlockId, float> blk_x_pos;
    /// @brief The y-positions of each atom block. Is UNDEFINED_POS if undefined.
    vtr::vector<AtomBlockId, float> blk_y_pos;
    /// @brief The layer of each atom block. Is UNDEFINED_POS if undefined.
    vtr::vector<AtomBlockId, float> blk_layer;

    /// @brief The sub tile location of each atom block. Is UNDEFINED_SUB_TILE
    ///        if undefined.
    vtr::vector<AtomBlockId, int> blk_sub_tile;
    /// @brief The flat site idx of each atom block. This is an optional index
    ///        into a linearized list of primitive locations within a cluster-
    ///        level block. Is UNDEFINED_SITE_IDX if undefined.
    vtr::vector<AtomBlockId, int> blk_site_idx;

    /// @brief A flag to signify if this object has been constructed with data
    ///        or not. This makes it easier to detect if a flat placement exists
    ///        or not. Is true when a placement has been loaded into this
    ///        object, false otherwise.
    bool valid;

    /**
     * @brief Default constructor of this class.
     *
     * Initializes the data structure to invalid so it can be easily checked to
     * be uninitialized.
     */
    FlatPlacementInfo() : valid(false) {}

    /**
     * @brief Constructs the flat placement with undefined positions for each
     *        atom block in the atom netlist.
     *
     * The valid flag is set to true here, since this structure is now
     * initialized with data and can be used.
     *
     *  @param atom_netlist
     *              The netlist of atom blocks in the circuit.
     */
    FlatPlacementInfo(const AtomNetlist& atom_netlist)
        : blk_x_pos(atom_netlist.blocks().size(), UNDEFINED_POS),
          blk_y_pos(atom_netlist.blocks().size(), UNDEFINED_POS),
          blk_layer(atom_netlist.blocks().size(), UNDEFINED_POS),
          blk_sub_tile(atom_netlist.blocks().size(), UNDEFINED_SUB_TILE),
          blk_site_idx(atom_netlist.blocks().size(), UNDEFINED_SITE_IDX),
          valid(true) {}
};

