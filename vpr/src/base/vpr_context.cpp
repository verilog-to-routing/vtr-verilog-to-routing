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

/**
 * @brief determine the type of the bounding box used by the placer to predict
 * the wirelength.
 *
 * @param place_bb_mode The bounding box mode passed by the CLI
 * @param rr_graph The routing resource graph
 */
static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph);

void FloorplanningContext::update_floorplanning_context_post_pack() {
    // Initialize the cluster_constraints using the constraints loaded from the
    // user and clustering generated from packing.
    load_cluster_constraints();
}

void FloorplanningContext::update_floorplanning_context_pre_place(
                                    const PlaceMacros& place_macros) {
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

    // The compressed cluster constraints are loaded in alloc_and_laod_compressed
    // cluster_constraints and are not used outside of placement.
    vtr::release_memory(compressed_cluster_constraints);
}

void PlacementContext::init_placement_context(
                                    const t_placer_opts& placer_opts,
                                    const std::vector<t_direct_inf>& directs) {
    const AtomContext& atom_ctx = g_vpr_ctx.atom();
    const ClusteringContext& cluster_ctx = g_vpr_ctx.clustering();
    const DeviceContext& device_ctx = g_vpr_ctx.device();

    cube_bb = is_cube_bb(placer_opts.place_bounding_box_mode, device_ctx.rr_graph);

    compressed_block_grids = create_compressed_block_grids();

    // Alloc and load the placement macros.
    place_macros = std::make_unique<PlaceMacros>(directs,
                                                 device_ctx.physical_tile_types,
                                                 cluster_ctx.clb_nlist,
                                                 atom_ctx.netlist(),
                                                 atom_ctx.lookup());
}

static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph) {
    bool cube_bb;
    const int number_layers = g_vpr_ctx.device().grid.get_num_layers();

    if (place_bb_mode == e_place_bounding_box_mode::AUTO_BB) {
        // If the auto_bb is used, we analyze the RR graph to see whether is there any inter-layer connection that is not
        // originated from OPIN. If there is any, cube BB is chosen, otherwise, per-layer bb is chosen.
        if (number_layers > 1 && inter_layer_connections_limited_to_opin(rr_graph)) {
            cube_bb = false;
        } else {
            cube_bb = true;
        }
    } else if (place_bb_mode == e_place_bounding_box_mode::CUBE_BB) {
        // The user has specifically asked for CUBE_BB
        cube_bb = true;
    } else {
        // The user has specifically asked for PER_LAYER_BB
        VTR_ASSERT_SAFE(place_bb_mode == e_place_bounding_box_mode::PER_LAYER_BB);
        cube_bb = false;
    }

    return cube_bb;
}

void PlacementContext::clean_placement_context_post_place() {
    // The compressed block grids are currently only used during placement.
    vtr::release_memory(compressed_block_grids);
}

