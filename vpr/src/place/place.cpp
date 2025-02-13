
#include <memory>

#include "FlatPlacementInfo.h"
#include "place_macro.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "vpr_types.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "annealer.h"
#include "echo_files.h"
#include "PlacementDelayModelCreator.h"

#include "placer.h"

/********************* Static subroutines local to place.c *******************/
#ifdef VERBOSE
void print_clb_placement(const char* fname);
#endif

/**
 * @brief determine the type of the bounding box used by the placer to predict the wirelength
 *
 * @param place_bb_mode The bounding box mode passed by the CLI
 * @param rr_graph The routing resource graph
 */
static bool is_cube_bb(const e_place_bounding_box_mode place_bb_mode,
                       const RRGraphView& rr_graph);

/*****************************************************************************/
void try_place(const Netlist<>& net_list,
               const PlaceMacros& place_macros,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               const std::vector<t_direct_inf>& directs,
               const FlatPlacementInfo& flat_placement_info,
               bool is_flat) {

    /* Currently, the functions that require is_flat as their parameter and are called during placement should
     * receive is_flat as false. For example, if the RR graph of router lookahead is built here, it should be as
     * if is_flat is false, even if is_flat is set to true from the command line.
     */
    VTR_ASSERT(!is_flat);
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& atom_ctx = g_vpr_ctx.atom();

    /* Placement delay model is independent of the placement and can be shared across
     * multiple placers if we are performing parallel annealing.
     * So, it is created and initialized once. */
    std::shared_ptr<PlaceDelayModel> place_delay_model;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = PlacementDelayModelCreator::create_delay_model(placer_opts,
                                                                           router_opts,
                                                                           net_list,
                                                                           det_routing_arch,
                                                                           segment_inf,
                                                                           chan_width_dist,
                                                                           directs,
                                                                           is_flat);

        if (isEchoFileEnabled(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL)) {
            place_delay_model->dump_echo(getEchoFileName(E_ECHO_PLACEMENT_DELTA_DELAY_MODEL));
        }
    }

    g_vpr_ctx.mutable_placement().cube_bb = is_cube_bb(placer_opts.place_bounding_box_mode, device_ctx.rr_graph);
    const bool cube_bb = g_vpr_ctx.placement().cube_bb;

    VTR_LOG("\n");
    VTR_LOG("Bounding box mode is %s\n", (cube_bb ? "Cube" : "Per-layer"));
    VTR_LOG("\n");

    auto& place_ctx = g_vpr_ctx.mutable_placement();

    /* Make the global instance of BlkLocRegistry inaccessible through the getter methods of the
     * placement context. This is done to make sure that the placement stage only accesses its
     * own local instances of BlkLocRegistry.
     */
    place_ctx.lock_loc_vars();
    place_ctx.compressed_block_grids = create_compressed_block_grids();

    /* Start measuring placement time. The measured execution time will be printed
     * when this object goes out of scope at the end of this function.
     */
    vtr::ScopedStartFinishTimer placement_timer("Placement");

    // Enables fast look-up pb graph pins from block pin indices
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.logical_block_types);
    // Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.nlist, pb_gpin_lookup);

    Placer placer(net_list, place_macros, placer_opts, analysis_opts, noc_opts, pb_gpin_lookup, netlist_pin_lookup,
                  flat_placement_info, place_delay_model, cube_bb, is_flat, /*quiet=*/false);

    placer.place();

    vtr::release_memory(place_ctx.compressed_block_grids);

    /* The placer object has its own copy of block locations and doesn't update
     * the global context directly. We need to copy its internal data structures
     * to the global placement context before it goes out of scope.
     */
    placer.copy_locs_to_global_state(place_ctx);
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

#ifdef VERBOSE
void print_clb_placement(const char* fname) {
    /* Prints out the clb placements to a file.  */
    FILE* fp;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& place_ctx = g_vpr_ctx.placement();

    fp = vtr::fopen(fname, "w");
    fprintf(fp, "Complex block placements:\n\n");

    fprintf(fp, "Block #\tName\t(X, Y, Z).\n");
    for (auto i : cluster_ctx.clb_nlist.blocks()) {
        fprintf(fp, "#%d\t%s\t(%d, %d, %d).\n", i, cluster_ctx.clb_nlist.block_name(i).c_str(), place_ctx.block_locs[i].loc.x, place_ctx.block_locs[i].loc.y, place_ctx.block_locs[i].loc.sub_tile);
    }

    fclose(fp);
}
#endif

#if 0
static void update_screen_debug();

//Performs a major (i.e. interactive) placement screen update.
//This function with no arguments is useful for calling from a debugger to
//look at the intermediate implemetnation state.
static void update_screen_debug() {
    update_screen(ScreenUpdatePriority::MAJOR, "DEBUG", PLACEMENT, nullptr);
}
#endif

