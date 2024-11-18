#include <cstdio>
#include <cmath>
#include <memory>
#include <chrono>
#include <optional>

#include "NetPinTimingInvalidator.h"
#include "clustered_netlist.h"
#include "device_grid.h"
#include "verify_placement.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_time.h"
#include "vtr_math.h"

#include "vpr_types.h"
#include "vpr_error.h"
#include "vpr_utils.h"

#include "globals.h"
#include "place.h"
#include "annealer.h"
#include "read_place.h"
#include "draw.h"
#include "timing_place.h"
#include "read_xml_arch_file.h"
#include "echo_files.h"
#include "histogram.h"
#include "place_util.h"
#include "analytic_placer.h"
#include "initial_placement.h"
#include "place_delay_model.h"
#include "place_timing_update.h"
#include "move_transactions.h"
#include "move_utils.h"
#include "buttons.h"

#include "PlacementDelayCalculator.h"
#include "VprTimingGraphResolver.h"
#include "timing_util.h"
#include "timing_info.h"
#include "concrete_timing_info.h"
#include "tatum/echo_writer.hpp"
#include "tatum/TimingReporter.hpp"

#include "RL_agent_util.h"
#include "place_checkpoint.h"

#include "clustered_netlist_utils.h"

#include "noc_place_utils.h"

#include "net_cost_handler.h"
#include "placer_state.h"
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

static int count_connections();

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat,
                                               const BlkLocRegistry& blk_loc_registry);

/**
 * @brief Copies the placement location variables into the global placement context.
 * @param blk_loc_registry The placement location variables to be copied.
 */
static void copy_locs_to_global_state(const BlkLocRegistry& blk_loc_registry);

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
    const auto& atom_ctx = g_vpr_ctx.atom();
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& timing_ctx = g_vpr_ctx.timing();
    auto pre_place_timing_stats = timing_ctx.stats;

    char msg[vtr::bufsize];

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

    int move_lim = (int)(placer_opts.anneal_sched.inner_num * pow(net_list.blocks().size(), 1.3333));

    auto& place_ctx = g_vpr_ctx.mutable_placement();
    place_ctx.lock_loc_vars();
    place_ctx.compressed_block_grids = create_compressed_block_grids();

    Placer placer(net_list, placer_opts, analysis_opts, noc_opts, directs, place_delay_model, cube_bb);

#ifndef NO_GRAPHICS
    if (placer.noc_cost_handler_.has_value()) {
        get_draw_state_vars()->set_noc_link_bandwidth_usages_ref(placer.noc_cost_handler_->get_link_bandwidth_usages());
    }
#endif

    const int width_fac = placer_opts.place_chan_width;
    init_draw_coords((float)width_fac, placer.placer_state_.blk_loc_registry());

    sprintf(msg,
            "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);

    // Draw the initial placement
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place", 0,
                                               0);
        VTR_LOG("Saving initial placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str(), blk_loc_registry.block_locs());
    }

    //Some stats
    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", swap_stats.num_ts_called);
    blocks_affected.move_abortion_logger.report_aborted_moves();

    if (placer_opts.place_algorithm.is_timing_driven()) {
        //Final timing estimate
        VTR_ASSERT(timing_info);

        critical_path = timing_info->least_slack_critical_path();

        if (isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints,
                              *placement_delay_calc, timing_info->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH) + std::string(".dot"),
                                         *timing_info, debug_tnode);
        }

        generate_post_place_timing_reports(placer_opts, analysis_opts, *timing_info,
                                           *placement_delay_calc, is_flat, blk_loc_registry);

        // Print critical path delay metrics
        VTR_LOG("\n");
        print_setup_timing_summary(*timing_ctx.constraints,
                                   *timing_info->setup_analyzer(), "Placement estimated ", "");
    }

    sprintf(msg,
            "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, width_fac);
    VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, \n", costs.cost,
            costs.bb_cost, costs.timing_cost);
    // print the noc costs info
    if (noc_opts.noc) {
        VTR_ASSERT(noc_cost_handler.has_value());
        noc_cost_handler->print_noc_costs("\nNoC Placement Costs", costs, noc_opts);

#ifdef ENABLE_NOC_SAT_ROUTING
        if (costs.noc_cost_terms.congestion > 0.0) {
            VTR_LOG("NoC routing configuration is congested. Invoking the SAT NoC router.\n");
            invoke_sat_router(costs, noc_opts, placer_opts.seed);
        }
#endif //ENABLE_NOC_SAT_ROUTING
    }

    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, timing_info);

    free_placement_structs();

    copy_locs_to_global_state(blk_loc_registry);
}

/*only count non-global connections */
static int count_connections() {
    auto& cluster_ctx = g_vpr_ctx.clustering();

    int count = 0;

    for (ClusterNetId net_id : cluster_ctx.clb_nlist.nets()) {
        if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
            continue;
        }

        count += cluster_ctx.clb_nlist.net_sinks(net_id).size();
    }

    return count;
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

/* Frees the major structures needed by the placer (and not needed       *
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

static void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
                                               const t_analysis_opts& analysis_opts,
                                               const SetupTimingInfo& timing_info,
                                               const PlacementDelayCalculator& delay_calc,
                                               bool is_flat,
                                               const BlkLocRegistry& blk_loc_registry) {
    const auto& timing_ctx = g_vpr_ctx.timing();
    const auto& atom_ctx = g_vpr_ctx.atom();

    VprTimingGraphResolver resolver(atom_ctx.nlist, atom_ctx.lookup, *timing_ctx.graph,
                                    delay_calc, is_flat, blk_loc_registry);
    resolver.set_detail_level(analysis_opts.timing_report_detail);

    tatum::TimingReporter timing_reporter(resolver, *timing_ctx.graph,
                                          *timing_ctx.constraints);

    timing_reporter.report_timing_setup(
        placer_opts.post_place_timing_report_file,
        *timing_info.setup_analyzer(), analysis_opts.timing_report_npaths);
}

#if 0
static void update_screen_debug();

//Performs a major (i.e. interactive) placement screen update.
//This function with no arguments is useful for calling from a debugger to
//look at the intermediate implemetnation state.
static void update_screen_debug() {
    update_screen(ScreenUpdatePriority::MAJOR, "DEBUG", PLACEMENT, nullptr);
}
#endif

static void copy_locs_to_global_state(const BlkLocRegistry& blk_loc_registry) {
    auto& place_ctx = g_vpr_ctx.mutable_placement();

    // the placement location variables should be unlocked before being accessed
    place_ctx.unlock_loc_vars();

    // copy the local location variables into the global state
    auto& global_blk_loc_registry = place_ctx.mutable_blk_loc_registry();
    global_blk_loc_registry = blk_loc_registry;

#ifndef NO_GRAPHICS
    // update the graphics' reference to placement location variables
    get_draw_state_vars()->set_graphics_blk_loc_registry_ref(global_blk_loc_registry);
#endif
}