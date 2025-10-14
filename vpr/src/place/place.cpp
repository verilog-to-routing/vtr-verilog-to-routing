
#include <memory>

#include "flat_placement_types.h"
#include "initial_placement.h"
#include "noc_place_utils.h"
#include "pack.h"
#include "vpr_context.h"
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

/*****************************************************************************/
void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               const t_chan_width_dist& chan_width_dist,
               t_det_routing_arch& det_routing_arch,
               const std::vector<t_segment_inf>& segment_inf,
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
    auto& mutable_placement = g_vpr_ctx.mutable_placement();
    auto& mutable_floorplanning = g_vpr_ctx.mutable_floorplanning();

    // Initialize the variables in the placement context.
    mutable_placement.init_placement_context(placer_opts, directs);

    // Update the floorplanning constraints with the macro information from the
    // placement context.
    mutable_floorplanning.update_floorplanning_context_pre_place(*mutable_placement.place_macros);

    VTR_LOG("\n");
    VTR_LOG("Bounding box mode is %s\n", (mutable_placement.cube_bb ? "Cube" : "Per-layer"));
    VTR_LOG("\n");

    /* To make sure the importance of NoC-related cost terms compared to
     * BB and timing cost is determine only through NoC placement weighting factor,
     * we normalize NoC-related cost weighting factors so that they add up to 1.
     * With this normalization, NoC-related cost weighting factors only determine
     * the relative importance of NoC cost terms with respect to each other, while
     * the importance of total NoC cost to conventional placement cost is determined
     * by NoC placement weighting factor.
     * FIXME: This should not be modifying the NoC Opts here, this normalization
     *        should occur when these Opts are loaded in.
     */
    if (noc_opts.noc) {
        normalize_noc_cost_weighting_factor(const_cast<t_noc_opts&>(noc_opts));
    }

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

    /* Make the global instance of BlkLocRegistry inaccessible through the getter methods of the
     * placement context. This is done to make sure that the placement stage only accesses its
     * own local instances of BlkLocRegistry.
     */
    mutable_placement.lock_loc_vars();

    /* Start measuring placement time. The measured execution time will be printed
     * when this object goes out of scope at the end of this function.
     */
    vtr::ScopedStartFinishTimer placement_timer("Placement");

    // Enables fast look-up pb graph pins from block pin indices
    IntraLbPbPinLookup pb_gpin_lookup(device_ctx.logical_block_types);
    // Enables fast look-up of atom pins connect to CLB pins
    ClusteredPinAtomPinsLookup netlist_pin_lookup(cluster_ctx.clb_nlist, atom_ctx.netlist(), pb_gpin_lookup);

    Placer placer(net_list, {}, placer_opts, analysis_opts, noc_opts, pb_gpin_lookup, netlist_pin_lookup,
                  flat_placement_info, place_delay_model, placer_opts.place_auto_init_t_scale,
                  mutable_placement.cube_bb, is_flat, /*quiet=*/false);

    placer.place();

    /* The placer object has its own copy of block locations and doesn't update
     * the global context directly. We need to copy its internal data structures
     * to the global placement context before it goes out of scope.
     */
    placer.update_global_state();

    // Clean the variables in the placement context. This will deallocate memory
    // used by variables which were allocated in the placement context and are
    // never used outside of placement.
    mutable_placement.clean_placement_context_post_place();
    mutable_floorplanning.clean_floorplanning_context_post_place();
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
    update_screen(ScreenUpdatePriority::MAJOR, "DEBUG", e_pic_type::PLACEMENT, nullptr);
}
#endif
