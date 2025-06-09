#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    May 2025
 * @brief   Methods for generating mass reports. Mass reports contain information
 *          on the AP partial legalizer's internal representation of mass for
 *          primitives and tiles.
 */

#include <vector>
#include "ap_netlist_fwd.h"
#include "primitive_vector_fwd.h"
#include "vtr_vector.h"

// Forward declarations
class PrimitiveDimManager;

/**
 * @brief Generate a mass report for the given AP netlist with the given block
 *        masses and tile / logic block capacities.
 *
 *  @param logical_block_type_capacities
 *      The primitive-vector capacity of each logical block on the device.
 *  @param physical_tile_type_capacities
 *      The primitive-vector capacity of each tile on the device.
 *  @param block_mass
 *      The mass of each AP block in the AP netlist.
 *  @param ap_netlist
 */
void generate_ap_mass_report(const std::vector<PrimitiveVector>& logical_block_type_capacities,
                             const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                             const vtr::vector<APBlockId, PrimitiveVector>& block_mass,
                             const PrimitiveDimManager& dim_manager,
                             const APNetlist& ap_netlist);
