#include <memory>

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_time.h"
#include "vpr_types.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "annealer.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "histogram.h"
#include "place_delay_model.h"
#include "move_utils.h"
#include "buttons.h"

#include "VprTimingGraphResolver.h"
#include "tatum/TimingReporter.hpp"

#include "RL_agent_util.h"
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

static void free_placement_structs();

static void run_placement(int seed_offset, std::unique_ptr<Placer>& placer,
                          const Netlist<>& net_list,
                          const t_placer_opts& placer_opts,
                          const t_analysis_opts& analysis_opts,
                          const t_noc_opts& noc_opts,
                          const std::vector<t_direct_inf>& directs,
                          const std::shared_ptr<PlaceDelayModel>& place_delay_model,
                          bool cube_bb) {
    t_placer_opts placer_opts_copy = placer_opts;
    placer_opts_copy.seed += seed_offset;

    placer = std::make_unique<Placer>(net_list, placer_opts_copy, analysis_opts, noc_opts, directs, place_delay_model, cube_bb, /*is_flat=*/false, /*quiet=*/true);
    placer->place();
}

/*****************************************************************************/
void try_place(const Netlist<>& net_list,
               const t_placer_opts& placer_opts,
               const t_router_opts& router_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               t_chan_width_dist chan_width_dist,
               t_det_routing_arch* det_routing_arch,
               std::vector<t_segment_inf>& segment_inf,
               const std::vector<t_direct_inf>& directs,
               bool is_flat) {
    /* Does almost all the work of placing a circuit.  Width_fac gives the   *
     * width of the widest channel.  Place_cost_exp says what exponent the   *
     * width should be taken to when calculating costs.  This allows a       *
     * greater bias for anisotropic architectures.                           */

    /* Currently, the functions that require is_flat as their parameter and are called during placement should
     * receive is_flat as false. For example, if the RR graph of router lookahead is built here, it should be as
     * if is_flat is false, even if is_flat is set to true from the command line.
     */
    VTR_ASSERT(!is_flat);
    const auto& device_ctx = g_vpr_ctx.device();

    /* Placement delay model is independent of the placement and can be shared across
     * multiple placers. So, it is created and initialized once. */
    std::shared_ptr<PlaceDelayModel> place_delay_model;

    if (placer_opts.place_algorithm.is_timing_driven()) {
        /*do this before the initial placement to avoid messing up the initial placement */
        place_delay_model = alloc_lookups_and_delay_model(net_list,
                                                          chan_width_dist,
                                                          placer_opts,
                                                          router_opts,
                                                          det_routing_arch,
                                                          segment_inf,
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
    place_ctx.lock_loc_vars();
    place_ctx.compressed_block_grids = create_compressed_block_grids();

    std::vector<std::unique_ptr<Placer>> placers(2);

    std::thread place_thread(run_placement, 0, std::ref(placers[0]),
                             std::ref(net_list),
                             std::ref(placer_opts),
                             std::ref(analysis_opts),
                             std::ref(noc_opts),
                             std::ref(directs),
                             std::ref(place_delay_model),
                             cube_bb);

    run_placement(1, placers[1], net_list, placer_opts, analysis_opts, noc_opts, directs, place_delay_model, cube_bb);

    place_thread.join();

    for (const auto& placer : placers) {
        const t_placer_costs& costs = placer->costs();
        std::cout << "Cost " << costs.bb_cost << "  " << costs.timing_cost << std::endl;
    }

//    Placer placer(net_list, placer_opts, analysis_opts, noc_opts, directs, place_delay_model, cube_bb, is_flat, /*quiet=*/false);
//
//    placer.place();

    free_placement_structs();

    placers[0]->copy_locs_to_global_state();
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

/* Frees the major structures needed by the placer (and not needed
 * elsewhere).   */
static void free_placement_structs() {
    auto& place_ctx = g_vpr_ctx.mutable_placement();
    vtr::release_memory(place_ctx.compressed_block_grids);
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

