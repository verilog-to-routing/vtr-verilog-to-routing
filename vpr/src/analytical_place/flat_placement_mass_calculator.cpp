/**
 * @file
 * @author  Alex Singer
 * @date    February 2024
 * @brief   Implementation of the mass calculator used in the AP flow.
 */

#include "flat_placement_mass_calculator.h"
#include <vector>
#include "ap_mass_report.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "logic_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "primitive_dim_manager.h"
#include "primitive_vector.h"
#include "primitive_vector_fwd.h"
#include "vtr_log.h"
#include "vtr_vector.h"

/**
 * @brief Get the scalar mass of the given model (primitive type).
 *
 * A model with a higher mass will take up more space in its bin which may force
 * more spreading of that type of primitive.
 *
 * TODO: This will be made more complicated later. Models may be weighted based
 *       on some factors.
 */
static float get_model_mass(LogicalModelId model_id) {
    // Currently, all models have a mass of one.
    (void)model_id;
    return 1.f;
}

// This method is being forward-declared due to the double recursion below.
// Eventually this should be made into a non-recursive algorithm for performance,
// however this is not in a performance critical part of the code.
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type,
                                             const PrimitiveDimManager& dim_manager);

/**
 * @brief Get the amount of primitives this mode can contain.
 *
 * This is part of a double recursion, since a mode contains primitives which
 * themselves have modes.
 */
static PrimitiveVector calc_mode_capacity(const t_mode& mode,
                                          const PrimitiveDimManager& dim_manager) {
    // Accumulate the capacities of all the pbs in this mode.
    PrimitiveVector capacity;
    for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
        const t_pb_type& pb_type = mode.pb_type_children[pb_child_idx];
        PrimitiveVector pb_capacity = calc_pb_type_capacity(&pb_type, dim_manager);
        // A mode may contain multiple pbs of the same type, multiply the
        // capacity.
        pb_capacity *= pb_type.num_pb;
        capacity += pb_capacity;
    }
    return capacity;
}

/**
 * @brief Get the amount of primitives this pb can contain.
 *
 * This is the other part of the double recursion. A pb may have multiple modes.
 * Modes are made of pbs.
 */
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type,
                                             const PrimitiveDimManager& dim_manager) {
    // Since a pb cannot be multiple modes at the same time, we do not
    // accumulate the capacities of the mode. Instead we need to "mix" the two
    // capacities as if the pb could choose either one.
    PrimitiveVector capacity;
    // If this is a leaf / primitive, create the base PrimitiveVector capacity.
    if (pb_type->is_primitive()) {
        LogicalModelId model_id = pb_type->model_id;
        VTR_ASSERT(model_id.is_valid());
        PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
        VTR_ASSERT(dim.is_valid());
        capacity.add_val_to_dim(get_model_mass(model_id), dim);
        return capacity;
    }
    // For now, we simply mix the capacities of modes by taking the max of each
    // dimension of the capcities. This provides an upper-bound on the amount of
    // primitives this pb can contain.
    for (int mode = 0; mode < pb_type->num_modes; mode++) {
        PrimitiveVector mode_capacity = calc_mode_capacity(pb_type->modes[mode], dim_manager);
        capacity = PrimitiveVector::max(capacity, mode_capacity);
    }
    return capacity;
}

/**
 * @brief Calculate the cpacity of the given logical block type.
 */
static PrimitiveVector calc_logical_block_type_capacity(const t_logical_block_type& logical_block_type,
                                                        const PrimitiveDimManager& dim_manager) {
    // If this logical block is empty, it cannot contain any primitives.
    if (logical_block_type.is_empty())
        return PrimitiveVector();
    // The primitive capacity of a logical block is the primitive capacity of
    // its root pb.
    return calc_pb_type_capacity(logical_block_type.pb_type, dim_manager);
}

/**
 * @brief Get the primitive capacity of the given sub_tile.
 *
 * Sub_tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param sub_tile                         The sub_tile to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static PrimitiveVector calc_sub_tile_capacity(const t_sub_tile& sub_tile,
                                              const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Similar to getting the primitive capacity of the pb, sub_tiles have many
    // equivalent sites, but it can only be one of them at a time. Need to "mix"
    // the capacities of the different sites this sub_tile may be.
    PrimitiveVector capacity;
    for (t_logical_block_type_ptr block_type : sub_tile.equivalent_sites) {
        const PrimitiveVector& block_capacity = logical_block_type_capacities[block_type->index];
        // Currently, we take the max of each primitive dimension as an upper
        // bound on the capacity of the sub_tile.
        capacity = PrimitiveVector::max(capacity, block_capacity);
    }
    return capacity;
}

/**
 * @brief Get the primitive capacity of a tile of the given type.
 *
 * Tiles may reuse logical blocks between one another, therefore this method
 * requires that the capacities of all of the logical blocks have been
 * pre-calculated and stored in the given vector.
 *
 *  @param tile_type                        The tile type to get the capacity of.
 *  @param logical_block_type_capacities    The capacities of all logical block
 *                                          types.
 */
static PrimitiveVector calc_physical_tile_type_capacity(const t_physical_tile_type& tile_type,
                                                        const std::vector<PrimitiveVector>& logical_block_type_capacities) {
    // Accumulate the capacities of all the sub_tiles in the given tile type.
    PrimitiveVector capacity;
    for (const t_sub_tile& sub_tile : tile_type.sub_tiles) {
        PrimitiveVector sub_tile_capacity = calc_sub_tile_capacity(sub_tile, logical_block_type_capacities);
        // A tile may contain many sub_tiles of the same type. Multiply by the
        // number of sub_tiles of this type.
        sub_tile_capacity *= sub_tile.capacity.total();
        capacity += sub_tile_capacity;
    }
    return capacity;
}

/**
 * @brief Get the primitive mass of the given block.
 *
 * This returns an M-dimensional vector with each entry indicating the mass of
 * that primitive type in this block. M is the number of unique models
 * (primitive types) in the architecture.
 */
static PrimitiveVector calc_block_mass(APBlockId blk_id,
                                       const PrimitiveDimManager& dim_manager,
                                       const APNetlist& netlist,
                                       const Prepacker& prepacker,
                                       const AtomNetlist& atom_netlist) {
    PrimitiveVector mass;
    PackMoleculeId mol_id = netlist.block_molecule(blk_id);
    const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
    for (AtomBlockId atom_blk_id : mol.atom_block_ids) {
        // See issue #2791, some of the atom_block_ids may be invalid. They can
        // safely be ignored.
        if (!atom_blk_id.is_valid())
            continue;
        LogicalModelId model_id = atom_netlist.block_model(atom_blk_id);
        VTR_ASSERT(model_id.is_valid());
        PrimitiveVectorDim dim = dim_manager.get_model_dim(model_id);
        VTR_ASSERT(dim.is_valid());
        mass.add_val_to_dim(get_model_mass(model_id), dim);
    }
    return mass;
}

/**
 * @brief Initialize the dim manager such that every model in the architecture
 *        has a valid dimension in the primitive vector.
 */
static void initialize_dim_manager(PrimitiveDimManager& dim_manager,
                                   const LogicalModels& models,
                                   const AtomNetlist& atom_netlist) {
    // Set the mapping between model IDs and Primitive Vector IDs

    // Count the number of occurences of each model in the netlist.
    vtr::vector<LogicalModelId, unsigned> num_model_occurence(models.all_models().size(), 0);
    for (AtomBlockId blk_id : atom_netlist.blocks()) {
        num_model_occurence[atom_netlist.block_model(blk_id)]++;
    }

    // Create a list of models, sorted by their frequency in the netlist.
    // By sorting by frequency, we make the early dimensions more common,
    // which can reduce the overall size of the sparse vector.
    // NOTE: We use stable sort here to keep the order of models the same
    //       as what the user provided in the arch file in the event of a tie.
    std::vector<LogicalModelId> logical_models(models.all_models().begin(), models.all_models().end());
    std::stable_sort(logical_models.begin(), logical_models.end(), [&](LogicalModelId a, LogicalModelId b) {
        return num_model_occurence[a] > num_model_occurence[b];
    });

    // Create a primitive vector dim for each model.
    for (LogicalModelId model_id : logical_models) {
        dim_manager.create_dim(model_id, models.model_name(model_id));
    }
}

FlatPlacementMassCalculator::FlatPlacementMassCalculator(const APNetlist& ap_netlist,
                                                         const Prepacker& prepacker,
                                                         const AtomNetlist& atom_netlist,
                                                         const std::vector<t_logical_block_type>& logical_block_types,
                                                         const std::vector<t_physical_tile_type>& physical_tile_types,
                                                         const LogicalModels& models,
                                                         int log_verbosity)
    : physical_tile_type_capacity_(physical_tile_types.size())
    , logical_block_type_capacity_(logical_block_types.size())
    , block_mass_(ap_netlist.blocks().size())
    , log_verbosity_(log_verbosity) {

    // Initialize the mapping between model IDs and Primitive Vector dims
    initialize_dim_manager(primitive_dim_manager_,
                           models,
                           atom_netlist);

    // Precompute the capacity of each logical block type.
    for (const t_logical_block_type& logical_block_type : logical_block_types) {
        logical_block_type_capacity_[logical_block_type.index] = calc_logical_block_type_capacity(logical_block_type, primitive_dim_manager_);
    }

    // Precompute the capacity of each physical tile type.
    for (const t_physical_tile_type& physical_tile_type : physical_tile_types) {
        physical_tile_type_capacity_[physical_tile_type.index] = calc_physical_tile_type_capacity(physical_tile_type, logical_block_type_capacity_);
    }

    // Precompute the mass of each block in the APNetlist
    VTR_LOGV(log_verbosity_ >= 10, "Pre-computing the block masses...\n");
    for (APBlockId ap_block_id : ap_netlist.blocks()) {
        block_mass_[ap_block_id] = calc_block_mass(ap_block_id,
                                                   primitive_dim_manager_,
                                                   ap_netlist,
                                                   prepacker,
                                                   atom_netlist);
    }
    VTR_LOGV(log_verbosity_ >= 10, "Finished pre-computing the block masses.\n");
}

void FlatPlacementMassCalculator::generate_mass_report(const APNetlist& ap_netlist) const {
    generate_ap_mass_report(logical_block_type_capacity_,
                            physical_tile_type_capacity_,
                            block_mass_,
                            primitive_dim_manager_,
                            ap_netlist);
}
