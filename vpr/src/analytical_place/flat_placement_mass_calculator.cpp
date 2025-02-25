/**
 * @file
 * @author  Alex Singer
 * @date    February 2024
 * @brief   Implementation of the mass calculator used in the AP flow.
 */

#include "flat_placement_mass_calculator.h"
#include <vector>
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "globals.h"
#include "logic_types.h"
#include "physical_types.h"
#include "prepack.h"
#include "primitive_vector.h"
#include "vtr_log.h"

/**
 * @brief Get the scalar mass of the given model (primitive type).
 *
 * A model with a higher mass will take up more space in its bin which may force
 * more spreading of that type of primitive.
 *
 * TODO: This will be made more complicated later. Models may be weighted based
 *       on some factors.
 */
static float get_model_mass(const t_model* model) {
    // Currently, all models have a mass of one.
    (void)model;
    return 1.f;
}

// This method is being forward-declared due to the double recursion below.
// Eventually this should be made into a non-recursive algorithm for performance,
// however this is not in a performance critical part of the code.
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type);

/**
 * @brief Get the amount of primitives this mode can contain.
 *
 * This is part of a double recursion, since a mode contains primitives which
 * themselves have modes.
 */
static PrimitiveVector calc_mode_capacity(const t_mode& mode) {
    // Accumulate the capacities of all the pbs in this mode.
    PrimitiveVector capacity;
    for (int pb_child_idx = 0; pb_child_idx < mode.num_pb_type_children; pb_child_idx++) {
        const t_pb_type& pb_type = mode.pb_type_children[pb_child_idx];
        PrimitiveVector pb_capacity = calc_pb_type_capacity(&pb_type);
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
static PrimitiveVector calc_pb_type_capacity(const t_pb_type* pb_type) {
    // Since a pb cannot be multiple modes at the same time, we do not
    // accumulate the capacities of the mode. Instead we need to "mix" the two
    // capacities as if the pb could choose either one.
    PrimitiveVector capacity;
    // If this is a leaf / primitive, create the base PrimitiveVector capacity.
    if (pb_type->num_modes == 0) {
        const t_model* model = pb_type->model;
        VTR_ASSERT(model != nullptr);
        VTR_ASSERT_DEBUG(model->index >= 0);
        capacity.add_val_to_dim(get_model_mass(model), model->index);
        return capacity;
    }
    // For now, we simply mix the capacities of modes by taking the max of each
    // dimension of the capcities. This provides an upper-bound on the amount of
    // primitives this pb can contain.
    for (int mode = 0; mode < pb_type->num_modes; mode++) {
        PrimitiveVector mode_capacity = calc_mode_capacity(pb_type->modes[mode]);
        capacity = PrimitiveVector::max(capacity, mode_capacity);
    }
    return capacity;
}

/**
 * @brief Calculate the cpacity of the given logical block type.
 */
static PrimitiveVector calc_logical_block_type_capacity(
            const t_logical_block_type& logical_block_type) {
    // If this logical block is empty, it cannot contain any primitives.
    if (logical_block_type.is_empty())
        return PrimitiveVector();
    // The primitive capacity of a logical block is the primitive capacity of
    // its root pb.
    return calc_pb_type_capacity(logical_block_type.pb_type);
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
static PrimitiveVector calc_physical_tile_type_capacity(
            const t_physical_tile_type& tile_type,
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
        const t_model* model = atom_netlist.block_model(atom_blk_id);
        VTR_ASSERT_DEBUG(model->index >= 0);
        mass.add_val_to_dim(get_model_mass(model), model->index);
    }
    return mass;
}

/**
 * @brief Debug printing method to print the capacities of all logical blocks
 *        and physical tile types.
 */
static void print_capacities(const std::vector<PrimitiveVector>& logical_block_type_capacities,
                             const std::vector<PrimitiveVector>& physical_tile_type_capacities,
                             const std::vector<t_logical_block_type>& logical_block_types,
                             const std::vector<t_physical_tile_type>& physical_tile_types) {
    // Get a linear list of all models.
    // TODO: I do not like using the global context here, but these models
    //       should be stable in VTR. If they were stored better, we may be
    //       able to pass them in.
    std::vector<t_model*> all_models;
    t_model* curr_model = g_vpr_ctx.device().arch->models;
    while (curr_model != nullptr) {
        if (curr_model->index >= (int)all_models.size())
            all_models.resize(curr_model->index + 1);
        all_models[curr_model->index] = curr_model;
        curr_model = curr_model->next;
    }
    curr_model = g_vpr_ctx.device().arch->model_library;
    while (curr_model != nullptr) {
        if (curr_model->index >= (int)all_models.size())
            all_models.resize(curr_model->index + 1);
        all_models[curr_model->index] = curr_model;
        curr_model = curr_model->next;
    }
    // Print the capacities.
    VTR_LOG("Logical Block Type Capacities:\n");
    VTR_LOG("------------------------------\n");
    VTR_LOG("name\t");
    for (t_model* model : all_models) {
        VTR_LOG("%s\t", model->name);
    }
    VTR_LOG("\n");
    for (const t_logical_block_type& block_type : logical_block_types) {
        const PrimitiveVector& capacity = logical_block_type_capacities[block_type.index];
        VTR_LOG("%s\t", block_type.name.c_str());
        for (t_model* model : all_models) {
            VTR_LOG("%.2f\t", capacity.get_dim_val(model->index));
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
    VTR_LOG("Physical Tile Type Capacities:\n");
    VTR_LOG("------------------------------\n");
    VTR_LOG("name\t");
    for (t_model* model : all_models) {
        VTR_LOG("%s\t", model->name);
    }
    VTR_LOG("\n");
    for (const t_physical_tile_type& tile_type : physical_tile_types) {
        const PrimitiveVector& capacity = physical_tile_type_capacities[tile_type.index];
        VTR_LOG("%s\t", tile_type.name.c_str());
        for (t_model* model : all_models) {
            VTR_LOG("%.2f\t", capacity.get_dim_val(model->index));
        }
        VTR_LOG("\n");
    }
    VTR_LOG("\n");
}

FlatPlacementMassCalculator::FlatPlacementMassCalculator(
        const APNetlist& ap_netlist,
        const Prepacker& prepacker,
        const AtomNetlist& atom_netlist,
        const std::vector<t_logical_block_type>& logical_block_types,
        const std::vector<t_physical_tile_type>& physical_tile_types,
        int log_verbosity)
            : physical_tile_type_capacity_(physical_tile_types.size())
            , logical_block_type_capacity_(logical_block_types.size())
            , block_mass_(ap_netlist.blocks().size())
            , log_verbosity_(log_verbosity) {

    // Precompute the capacity of each logical block type.
    for (const t_logical_block_type& logical_block_type : logical_block_types) {
        logical_block_type_capacity_[logical_block_type.index] = calc_logical_block_type_capacity(logical_block_type);
    }

    // Precompute the capacity of each physical tile type.
    for (const t_physical_tile_type& physical_tile_type : physical_tile_types) {
        physical_tile_type_capacity_[physical_tile_type.index] = calc_physical_tile_type_capacity(physical_tile_type, logical_block_type_capacity_);
    }

    // Precompute the mass of each block in the APNetlist
    VTR_LOGV(log_verbosity_ >= 10, "Pre-computing the block masses...\n");
    for (APBlockId ap_block_id : ap_netlist.blocks()) {
        block_mass_[ap_block_id] = calc_block_mass(ap_block_id,
                                                   ap_netlist,
                                                   prepacker,
                                                   atom_netlist);
    }
    VTR_LOGV(log_verbosity_ >= 10, "Finished pre-computing the block masses.\n");

    // Print the precomputed block capacities. This can be helpful for debugging.
    if (log_verbosity_ > 1) {
        print_capacities(logical_block_type_capacity_,
                         physical_tile_type_capacity_,
                         logical_block_types,
                         physical_tile_types);
    }
}

