/**
 * @file
 * @author  Alex Singer
 * @date    February 2024
 * @brief   Mass calculation for AP blocks and logical/physical block/tile types
 */

#pragma once

#include <vector>
#include "ap_netlist_fwd.h"
#include "primitive_vector.h"
#include "vtr_assert.h"
#include "vtr_vector.h"

class AtomNetlist;
class Prepacker;
struct t_logical_block_type;
struct t_physical_tile_type;

/**
 * @brief A calculator class which computes the M-dimensional mass of AP blocks
 *        and the capacity of tiles.
 *
 * Each atom in the Atom Netlist represents some model which may be implemented
 * on the FPGA. The FPGA architecture has M models. Since an AP block may
 * represent multiple atoms, its "mass" (the amount of "space" this block takes
 * up) must be M-dimensional. For example, a LUT+FF molecule would have a mass
 * of <1, 1> if LUTs and FFs were the only models in the architecture; while a
 * single LUT would have a mass of <1, 0>.
 *
 * This class handles how the mass of the AP blocks are calculated.
 *
 * This class also handles how much capacity each tile in the FGPA has. This
 * capacity is an approximation of how much M-dimensional mass that the tile
 * can hold. Since tiles may have multiple modes, the actual capacity of the
 * tiles change depending on what is in the tile. This class simplifies this
 * to assume that the capacity does not change and approximates the theoretical
 * mass that the tile should aim to hold.
 */
class FlatPlacementMassCalculator {
public:
    /**
     * @brief Construct the mass calculator.
     *
     *  @param ap_netlist
     *      The netlist of AP blocks to compute the mass of. The mass of each
     *      block is precomputed in the constructor and loaded cheaply later.
     *  @param prepacker
     *      The prepacker used to pack atoms into the molecules. The AP netlist
     *      contains molecules; this class is needed to get information on these
     *      molecules.
     *  @param atom_netlist
     *      The netlist of atoms for the circuit.
     *  @param logical_block_types
     *      A list of all logical block types that can be implemented in the
     *      device. The capacity of each logical block type is precomputed to
     *      help compute the capacity of physical_tile_types.
     *  @param physical_tile_types
     *      A list of all physical_tile_types that exist on the FGPA. The
     *      capacity of each physical tile is precomputed in the constructor to
     *      be loaded cheaply later.
     *  @param log_verbosity
     *      The verbosity of log messages in the mass calculator.
     */
    FlatPlacementMassCalculator(const APNetlist& ap_netlist,
                                const Prepacker& prepacker,
                                const AtomNetlist& atom_netlist,
                                const std::vector<t_logical_block_type>& logical_block_types,
                                const std::vector<t_physical_tile_type>& physical_tile_types,
                                int log_verbosity);

    /**
     * @brief Get the M-dimensional capacity of the given physical tile type.
     *
     * This is an approximation based on the description of the tile in the
     * architecture.
     */
    inline const PrimitiveVector& get_physical_tile_type_capacity(size_t physical_tile_type_index) const {
        VTR_ASSERT(physical_tile_type_index < physical_tile_type_capacity_.size());
        return physical_tile_type_capacity_[physical_tile_type_index];
    }

    /**
     * @brief Get the M-dimensional capacity of the given logical block type.
     */
    inline const PrimitiveVector& get_logical_block_type_capacity(size_t logical_block_type_index) const {
        VTR_ASSERT(logical_block_type_index < logical_block_type_capacity_.size());
        return logical_block_type_capacity_[logical_block_type_index];
    }

    /**
     * @brief Get the M-dimensional mass of the given AP block.
     */
    inline const PrimitiveVector& get_block_mass(APBlockId blk_id) const {
        VTR_ASSERT(blk_id.is_valid());
        return block_mass_[blk_id];
    }

private:
    /// @brief The capacity of each physical tile type, indexed by the index
    ///        of the physical_tile_type.
    std::vector<PrimitiveVector> physical_tile_type_capacity_;

    /// @brief The capacity of each logical block type, indexed by the index
    ///        of the logical block type.
    std::vector<PrimitiveVector> logical_block_type_capacity_;

    /// @brief The mass of each block in the AP netlist.
    vtr::vector<APBlockId, PrimitiveVector> block_mass_;

    /// @brief The verbosity of log messages in the mass calculator.
    const int log_verbosity_;
};

