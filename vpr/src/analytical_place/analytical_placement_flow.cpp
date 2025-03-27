/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Implementation of the Analytical Placement flow.
 */

#include "analytical_placement_flow.h"
#include <memory>
#include "analytical_solver.h"
#include "ap_netlist.h"
#include "atom_netlist.h"
#include "detailed_placer.h"
#include "full_legalizer.h"
#include "gen_ap_netlist_from_atoms.h"
#include "global_placer.h"
#include "globals.h"
#include "partial_legalizer.h"
#include "partial_placement.h"
#include "prepack.h"
#include "user_place_constraints.h"
#include "vpr_context.h"
#include "vpr_types.h"
#include "vtr_assert.h"
#include "vtr_time.h"

/**
 * @brief A helper method to log statistics on the APNetlist.
 */
static void print_ap_netlist_stats(const APNetlist& netlist) {
    // Get the number of moveable and fixed blocks
    size_t num_moveable_blocks = 0;
    size_t num_fixed_blocks = 0;
    for (APBlockId blk_id : netlist.blocks()) {
        if (netlist.block_mobility(blk_id) == APBlockMobility::MOVEABLE)
            num_moveable_blocks++;
        else
            num_fixed_blocks++;
    }
    // Get the fanout information of nets
    size_t highest_fanout = 0;
    float average_fanout = 0.f;
    for (APNetId net_id : netlist.nets()) {
        size_t net_fanout = netlist.net_pins(net_id).size();
        if (net_fanout > highest_fanout)
            highest_fanout = net_fanout;
        average_fanout += static_cast<float>(net_fanout);
    }
    average_fanout /= static_cast<float>(netlist.nets().size());
    // Print the statistics
    VTR_LOG("Analytical Placement Netlist Statistics:\n");
    VTR_LOG("\tBlocks: %zu\n", netlist.blocks().size());
    VTR_LOG("\t\tMoveable Blocks: %zu\n", num_moveable_blocks);
    VTR_LOG("\t\tFixed Blocks: %zu\n", num_fixed_blocks);
    VTR_LOG("\tNets: %zu\n", netlist.nets().size());
    VTR_LOG("\t\tAverage Fanout: %.2f\n", average_fanout);
    VTR_LOG("\t\tHighest Fanout: %zu\n", highest_fanout);
    VTR_LOG("\tPins: %zu\n", netlist.pins().size());
    VTR_LOG("\n");
}

/**
 * @brief Passes the flat placement information to a provided partial placement.
 *
 *  @param flat_placement_info    The flat placement information to be read.
 *  @param ap_netlist             The APNetlist that used to iterate over its blocks.
 *  @param prepacker              The Prepacker to get molecule of blocks in the ap_netlist.
 *  @param p_placement            The partial placement to be updated which is assumend
 * to be generated on ap_netlist or have the same blocks.
 */
static void convert_flat_to_partial_placement(const FlatPlacementInfo& flat_placement_info, const APNetlist& ap_netlist, const Prepacker& prepacker, PartialPlacement& p_placement) {
    for (APBlockId ap_blk_id : ap_netlist.blocks()) {
        // Get the molecule that AP block represents
        PackMoleculeId mol_id = ap_netlist.block_molecule(ap_blk_id);
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        // Get location of a valid atom in the molecule and verify that
        // all atoms of the molecule share same placement information.
        float atom_loc_x, atom_loc_y, atom_loc_layer;
        int atom_loc_sub_tile;
        bool found_valid_atom = false;
        for (AtomBlockId atom_blk_id : mol.atom_block_ids) {
            if (!atom_blk_id.is_valid())
                continue;
            float current_loc_x = flat_placement_info.blk_x_pos[atom_blk_id];
            float current_loc_y = flat_placement_info.blk_y_pos[atom_blk_id];
            float current_loc_layer = flat_placement_info.blk_layer[atom_blk_id];
            int current_loc_sub_tile = flat_placement_info.blk_sub_tile[atom_blk_id];
            if (found_valid_atom) {
                if (current_loc_x != atom_loc_x || current_loc_y != atom_loc_y || current_loc_layer != atom_loc_layer || current_loc_sub_tile != atom_loc_sub_tile)
                    VPR_FATAL_ERROR(VPR_ERROR_AP,
                                    "Molecule of ID %zu contains atom %s (ID: %zu) with a location (%g, %g, layer: %g, subtile: %d) "
                                    "that conflicts the location of other atoms in this molecule of (%g, %g, layer: %g, subtile: %d).",
                                    mol_id, g_vpr_ctx.atom().netlist().block_name(atom_blk_id).c_str(), atom_blk_id,
                                    current_loc_x, current_loc_y, current_loc_layer, current_loc_sub_tile,
                                    atom_loc_x, atom_loc_y, atom_loc_layer, atom_loc_sub_tile);
            } else {
                atom_loc_x = current_loc_x;
                atom_loc_y = current_loc_y;
                atom_loc_layer = current_loc_layer;
                atom_loc_sub_tile = current_loc_sub_tile;
                found_valid_atom = true;
            }
        }
        // Ensure that there is a valid atom in the molecule to pass its location.
        VTR_ASSERT_MSG(found_valid_atom, "Each molecule must contain at least one valid atom");
        // Pass the placement information
        p_placement.block_x_locs[ap_blk_id] = atom_loc_x;
        p_placement.block_y_locs[ap_blk_id] = atom_loc_y;
        p_placement.block_layer_nums[ap_blk_id] = atom_loc_layer;
        p_placement.block_sub_tiles[ap_blk_id] = atom_loc_sub_tile;
    }
}

/**
 * @brief If a flat placement is provided, skips the Global Placer and
 * converts it to a partial placement. Otherwise, runs the Global Placer.
 */
static PartialPlacement run_global_placer(const t_ap_opts& ap_opts,
                                          const AtomNetlist& atom_nlist,
                                          const APNetlist& ap_netlist,
                                          const Prepacker& prepacker,
                                          const DeviceContext& device_ctx) {
    if (g_vpr_ctx.atom().flat_placement_info().valid) {
        VTR_LOG("Flat Placement is provided in the AP flow, skipping the Global Placement.\n");
        PartialPlacement p_placement(ap_netlist);
        convert_flat_to_partial_placement(g_vpr_ctx.atom().flat_placement_info(),
                                          ap_netlist,
                                          prepacker,
                                          p_placement);
        return p_placement;
    } else {
        // Run the Global Placer
        std::unique_ptr<GlobalPlacer> global_placer = make_global_placer(ap_opts.analytical_solver_type,
                                                                         ap_opts.partial_legalizer_type,
                                                                         ap_netlist,
                                                                         prepacker,
                                                                         atom_nlist,
                                                                         device_ctx.grid,
                                                                         device_ctx.logical_block_types,
                                                                         device_ctx.physical_tile_types,
                                                                         ap_opts.log_verbosity);
        return global_placer->place();
    }
}

void run_analytical_placement_flow(t_vpr_setup& vpr_setup) {
    // Start an overall timer for the Analytical Placement flow.
    vtr::ScopedStartFinishTimer timer("Analytical Placement");

    // The global state used/modified by this flow.
    const AtomNetlist& atom_nlist = g_vpr_ctx.atom().netlist();
    const DeviceContext& device_ctx = g_vpr_ctx.device();
    const UserPlaceConstraints& constraints = g_vpr_ctx.floorplanning().constraints;

    // Run the prepacker
    const Prepacker prepacker(atom_nlist, device_ctx.logical_block_types);

    // Create the ap netlist from the atom netlist using the result from the
    // prepacker.
    APNetlist ap_netlist = gen_ap_netlist_from_atoms(atom_nlist,
                                                     prepacker,
                                                     constraints);
    print_ap_netlist_stats(ap_netlist);

    // Run the Global Placer.
    const t_ap_opts& ap_opts = vpr_setup.APOpts;
    PartialPlacement p_placement = run_global_placer(ap_opts,
                                                     atom_nlist,
                                                     ap_netlist,
                                                     prepacker,
                                                     device_ctx);

    // Verify that the partial placement is valid before running the full
    // legalizer.
    const size_t device_width = device_ctx.grid.width();
    const size_t device_height = device_ctx.grid.height();
    VTR_ASSERT(p_placement.verify(ap_netlist,
                                  device_width,
                                  device_height,
                                  device_ctx.grid.get_num_layers()));

    // Run the Full Legalizer.
    std::unique_ptr<FullLegalizer> full_legalizer = make_full_legalizer(ap_opts.full_legalizer_type,
                                                                        ap_netlist,
                                                                        atom_nlist,
                                                                        prepacker,
                                                                        vpr_setup,
                                                                        *device_ctx.arch,
                                                                        device_ctx.grid);
    full_legalizer->legalize(p_placement);

    // Run the Detailed Placer.
    std::unique_ptr<DetailedPlacer> detailed_placer = make_detailed_placer(ap_opts.detailed_placer_type,
                                                                           g_vpr_ctx.placement().blk_loc_registry(),
                                                                           atom_nlist,
                                                                           g_vpr_ctx.clustering().clb_nlist,
                                                                           vpr_setup,
                                                                           *device_ctx.arch);
    detailed_placer->optimize_placement();

    // Clean up some of the global variables that will no longer be used outside
    // of this flow.
    g_vpr_ctx.mutable_placement().clean_placement_context_post_place();
    g_vpr_ctx.mutable_floorplanning().clean_floorplanning_context_post_place();
}
