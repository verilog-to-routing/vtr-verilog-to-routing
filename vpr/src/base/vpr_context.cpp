/**
 * @file
 * @author  Alex Singer
 * @date    February 2025
 * @brief   Implementation of the more complicated context member functions.
 *          These are methods used to initialize / clean the contexts.
 */

#include "vpr_context.h"
#include <memory>
#include "compressed_grid.h"
#include "globals.h"
#include "physical_types.h"
#include "place_constraints.h"
#include "place_macro.h"
#include "vpr_types.h"
#include "vtr_memory.h"

void FloorplanningContext::update_floorplanning_context_post_pack() {
    // Initialize the cluster_constraints using the constraints loaded from the
    // user and clustering generated from packing.
    load_cluster_constraints();
}

void FloorplanningContext::update_floorplanning_context_pre_place(const PlaceMacros& place_macros) {
    // Go through cluster blocks to calculate the tightest placement
    // floorplan constraint for each constrained block.
    propagate_place_constraints(place_macros);

    // Compute and store compressed floorplanning constraints.
    alloc_and_load_compressed_cluster_constraints();
}

void FloorplanningContext::clean_floorplanning_context_post_place() {
    // The cluster constraints are loaded in propagate_place_constraints and are
    // not used outside of placement.
    vtr::release_memory(cluster_constraints);

    // The compressed cluster constraints are loaded in alloc_and_load_compressed
    // cluster_constraints and are not used outside of placement.
    vtr::release_memory(compressed_cluster_constraints);
}

void PlacementContext::init_placement_context(const std::vector<t_direct_inf>& directs) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    compressed_block_grids = create_compressed_block_grids();

    // Alloc and load the placement macros.
    place_macros = std::make_unique<PlaceMacros>(directs,
                                                 device_ctx.physical_tile_types,
                                                 cluster_ctx.clb_nlist,
                                                 atom_ctx.netlist(),
                                                 atom_ctx.lookup());
}

void PlacementContext::clean_placement_context_post_place() {
    // The compressed block grids are currently only used during placement.
    vtr::release_memory(compressed_block_grids);
}
