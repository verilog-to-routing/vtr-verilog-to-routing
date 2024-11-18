#include "place_log_util.h"

#include "vtr_log.h"
#include "annealer.h"
#include "place_util.h"
#include "PostClusterDelayCalculator.h"
#include "tatum/TimingReporter.hpp"
#include "VprTimingGraphResolver.h"
#include "timing_info.h"
#include "placer.h"
#include "draw.h"
#include "read_place.h"
#include "tatum/echo_writer.hpp"

PlacementLogPrinter::PlacementLogPrinter(const Placer& placer)
    : placer_(placer)
    , msg_(vtr::bufsize) {}

void PlacementLogPrinter::print_place_status_header() const {
    const bool noc_enabled = placer_.noc_opts().noc;

    VTR_LOG("\n");
    if (!noc_enabled) {
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
        VTR_LOG(
            "Tnum   Time       T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha\n");
        VTR_LOG(
            "      (sec)                                          (ns)       (ns)     (ns)                                                 \n");
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------\n");
    } else {
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------ -------- -------- ---------  ---------\n");
        VTR_LOG(
            "Tnum   Time       T Av Cost Av BB Cost Av TD Cost     CPD       sTNS     sWNS Ac Rate Std Dev  R lim Crit Exp Tot Moves  Alpha Agg. BW  Agg. Lat Lat Over. NoC Cong.\n");
        VTR_LOG(
            "      (sec)                                          (ns)       (ns)     (ns)                                                   (bps)     (ns)     (ns)             \n");
        VTR_LOG(
            "---- ------ ------- ------- ---------- ---------- ------- ---------- -------- ------- ------- ------ -------- --------- ------ -------- -------- --------- ---------\n");
    }
}

void PlacementLogPrinter::print_place_status(float elapsed_sec) const {
    const PlacementAnnealer& annealer = placer_.annealer();
    const t_annealing_state& annealing_state = annealer.get_annealing_state();
    const auto& [swap_stats, move_type_stats, placer_stats] = annealer.get_stats();
    const int tot_moves = annealer.get_total_iteration();
    const t_placer_costs& costs = placer_.costs();
    std::shared_ptr<const SetupTimingInfo> timing_info = placer_.timing_info();

    const bool noc_enabled = placer_.noc_opts().noc;
    const NocCostTerms& noc_cost_terms = placer_.costs().noc_cost_terms;

    const bool is_timing_driven = placer_.placer_opts().place_algorithm.is_timing_driven();
    const float cpd = is_timing_driven ? placer_.critical_path().delay() : std::numeric_limits<float>::quiet_NaN();
    const float sTNS = is_timing_driven ? placer_.timing_info()->setup_total_negative_slack() : std::numeric_limits<float>::quiet_NaN();
    const float sWNS = is_timing_driven ? placer_.timing_info()->setup_worst_negative_slack() : std::numeric_limits<float>::quiet_NaN();

    VTR_LOG(
        "%4zu %6.1f %7.1e "
        "%7.3f %10.2f %-10.5g "
        "%7.3f % 10.3g % 8.3f "
        "%7.3f %7.4f %6.1f %8.2f",
        annealing_state.num_temps, elapsed_sec, annealing_state.t,
        placer_stats.av_cost, placer_stats.av_bb_cost, placer_stats.av_timing_cost,
        1e9 * cpd, 1e9 * sTNS, 1e9 * sWNS,
        placer_stats.success_rate, placer_stats.std_dev, annealing_state.rlim, annealing_state.crit_exponent);

    pretty_print_uint(" ", tot_moves, 9, 3);

    VTR_LOG(" %6.3f", annealing_state.alpha);

    if (noc_enabled) {
        VTR_LOG(
            " %7.2e %7.2e"
            " %8.2e %8.2f",
            noc_cost_terms.aggregate_bandwidth, noc_cost_terms.latency,
            noc_cost_terms.latency_overrun, noc_cost_terms.congestion);
    }

    VTR_LOG("\n");
    fflush(stdout);

   sprintf(msg_.data(), "Cost: %g  BB Cost %g  TD Cost %g  Temperature: %g",
           costs.cost, costs.bb_cost, costs.timing_cost, annealing_state.t);

   update_screen(ScreenUpdatePriority::MINOR, msg_.data(), PLACEMENT, timing_info);
}

void PlacementLogPrinter::print_resources_utilization() const {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& block_locs = placer_.placer_state().block_locs();

    size_t max_block_name = 0;
    size_t max_tile_name = 0;

    //Record the resource requirement
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    std::map<t_logical_block_type_ptr, std::map<t_physical_tile_type_ptr, size_t>> num_placed_instances;

    for (ClusterBlockId blk_id : cluster_ctx.clb_nlist.blocks()) {
        const t_pl_loc& loc = block_locs[blk_id].loc;

        t_physical_tile_type_ptr physical_tile = device_ctx.grid.get_physical_type({loc.x, loc.y, loc.layer});
        t_logical_block_type_ptr logical_block = cluster_ctx.clb_nlist.block_type(blk_id);

        num_type_instances[logical_block]++;
        num_placed_instances[logical_block][physical_tile]++;

        max_block_name = std::max(max_block_name, logical_block->name.length());
        max_tile_name = std::max(max_tile_name, physical_tile->name.length());
    }

    VTR_LOG("\n");
    VTR_LOG("Placement resource usage:\n");
    for (const auto [logical_block_type_ptr, _] : num_type_instances) {
        for (const auto [physical_tile_type_ptr, num_instances] : num_placed_instances[logical_block_type_ptr]) {
            VTR_LOG("  %-*s implemented as %-*s: %d\n", max_block_name,
                    logical_block_type_ptr->name.c_str(), max_tile_name,
                    physical_tile_type_ptr->name.c_str(), num_instances);
        }
    }
    VTR_LOG("\n");
}

void PlacementLogPrinter::print_placement_swaps_stats() const {
    const PlacementAnnealer& annealer = placer_.annealer();
    const auto& [swap_stats, move_type_stats, placer_stats] = annealer.get_stats();
    const t_annealing_state& annealing_state = annealer.get_annealing_state();

    size_t total_swap_attempts = swap_stats.num_swap_rejected + swap_stats.num_swap_accepted + swap_stats.num_swap_aborted;
    VTR_ASSERT(total_swap_attempts > 0);

    size_t num_swap_print_digits = ceil(log10(total_swap_attempts));
    float reject_rate = (float)swap_stats.num_swap_rejected / total_swap_attempts;
    float accept_rate = (float)swap_stats.num_swap_accepted / total_swap_attempts;
    float abort_rate = (float)swap_stats.num_swap_aborted / total_swap_attempts;
    VTR_LOG("Placement number of temperatures: %d\n", annealing_state.num_temps);
    VTR_LOG("Placement total # of swap attempts: %*d\n", num_swap_print_digits,
            total_swap_attempts);
    VTR_LOG("\tSwaps accepted: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_accepted, 100 * accept_rate);
    VTR_LOG("\tSwaps rejected: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_rejected, 100 * reject_rate);
    VTR_LOG("\tSwaps aborted: %*d (%4.1f %%)\n", num_swap_print_digits,
            swap_stats.num_swap_aborted, 100 * abort_rate);
}
void PlacementLogPrinter::print_initial_placement_stats() const {
    const t_placer_costs& costs = placer_.costs();
    const t_noc_opts& noc_opts = placer_.noc_opts();
    const t_placer_opts& placer_opts = placer_.placer_opts();
    const tatum::TimingPathInfo& critical_path = placer_.critical_path();
    const std::optional<NocCostHandler>& noc_cost_handler = placer_.noc_cost_handler();
    std::shared_ptr<const SetupTimingInfo> timing_info = placer_.timing_info();
    const PlacerState& placer_state = placer_.placer_state();

    VTR_LOG("Initial placement cost: %g bb_cost: %g td_cost: %g\n",
        costs.cost, costs.bb_cost, costs.timing_cost);

    if (noc_opts.noc) {
        VTR_ASSERT(noc_cost_handler.has_value());
        noc_cost_handler->print_noc_costs("Initial NoC Placement Costs", costs, noc_opts);
    }

    if (placer_opts.place_algorithm.is_timing_driven()) {
        VTR_LOG("Initial placement estimated Critical Path Delay (CPD): %g ns\n",
                1e9 * critical_path.delay());
        VTR_LOG("Initial placement estimated setup Total Negative Slack (sTNS): %g ns\n",
                1e9 * timing_info->setup_total_negative_slack());
        VTR_LOG("Initial placement estimated setup Worst Negative Slack (sWNS): %g ns\n",
                1e9 * timing_info->setup_worst_negative_slack());
        VTR_LOG("\n");
        VTR_LOG("Initial placement estimated setup slack histogram:\n");
        print_histogram(create_setup_slack_histogram(*timing_info->setup_analyzer()));
    }

    const BlkLocRegistry& blk_loc_registry = placer_state.blk_loc_registry();
    size_t num_macro_members = 0;
    for (const t_pl_macro& macro : blk_loc_registry.place_macros().macros()) {
        num_macro_members += macro.members.size();
    }
    VTR_LOG("Placement contains %zu placement macros involving %zu blocks (average macro size %f)\n",
            blk_loc_registry.place_macros().macros().size(), num_macro_members,
            float(num_macro_members) / blk_loc_registry.place_macros().macros().size());
    VTR_LOG("\n");

    sprintf(msg_.data(),
            "Initial Placement.  Cost: %g  BB Cost: %g  TD Cost %g \t Channel Factor: %d",
            costs.cost, costs.bb_cost, costs.timing_cost, placer_opts.place_chan_width);

    // Draw the initial placement
    update_screen(ScreenUpdatePriority::MAJOR, msg_.data(), PLACEMENT, timing_info);

    if (placer_opts.placement_saves_per_temperature >= 1) {
        std::string filename = vtr::string_fmt("placement_%03d_%03d.place", 0, 0);
        VTR_LOG("Saving initial placement to file: %s\n", filename.c_str());
        print_place(nullptr, nullptr, filename.c_str(), blk_loc_registry.block_locs());
    }
}

void PlacementLogPrinter::print_post_placement_stats() const {
    const auto& timing_ctx = g_vpr_ctx.timing();
    const PlacementAnnealer& annealer = placer_.annealer();
    const auto& [swap_stats, move_type_stats, placer_stats] = annealer.get_stats();

    VTR_LOG("\n");
    VTR_LOG("Swaps called: %d\n", swap_stats.num_ts_called);
    annealer.get_move_abortion_logger().report_aborted_moves();

    if (placer_.placer_opts_.place_algorithm.is_timing_driven()) {
        //Final timing estimate
        VTR_ASSERT(placer_.timing_info_);

        if (isEchoFileEnabled(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH)) {
            tatum::write_echo(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH),
                              *timing_ctx.graph, *timing_ctx.constraints,
                              *placer_.placement_delay_calc_, placer_.timing_info_->analyzer());

            tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(placer_.analysis_opts_.echo_dot_timing_graph_node);
            write_setup_timing_graph_dot(getEchoFileName(E_ECHO_FINAL_PLACEMENT_TIMING_GRAPH) + std::string(".dot"),
                                         *placer_.timing_info_, debug_tnode);
        }

        generate_post_place_timing_reports(placer_.placer_opts_, placer_.analysis_opts_, *placer_.timing_info_,
                                           *placer_.placement_delay_calc_, /*is_flat=*/false, placer_.placer_state_.blk_loc_registry());

        // Print critical path delay metrics
        VTR_LOG("\n");
        print_setup_timing_summary(*timing_ctx.constraints,
                                   *placer_.timing_info_->setup_analyzer(), "Placement estimated ", "");
    }

    char msg[vtr::bufsize];
    sprintf(msg,
            "Placement. Cost: %g  bb_cost: %g td_cost: %g Channel Factor: %d",
            placer_.costs_.cost, placer_.costs_.bb_cost, placer_.costs_.timing_cost, placer_.placer_opts_.place_chan_width);
    VTR_LOG("Placement cost: %g, bb_cost: %g, td_cost: %g, \n", placer_.costs_.cost,
            placer_.costs_.bb_cost, placer_.costs_.timing_cost);
    update_screen(ScreenUpdatePriority::MAJOR, msg, PLACEMENT, placer_.timing_info_);

    // print the noc costs info
    if (placer_.noc_opts_.noc) {
        VTR_ASSERT(placer_.noc_cost_handler_.has_value());
        placer_.noc_cost_handler_->print_noc_costs("\nNoC Placement Costs", placer_.costs_, placer_.noc_opts_);

        // TODO: move this to an appropriate file
#ifdef ENABLE_NOC_SAT_ROUTING
        if (costs.noc_cost_terms.congestion > 0.0) {
            VTR_LOG("NoC routing configuration is congested. Invoking the SAT NoC router.\n");
            invoke_sat_router(costs, noc_opts, placer_opts.seed);
        }
#endif //ENABLE_NOC_SAT_ROUTING
    }
}

void generate_post_place_timing_reports(const t_placer_opts& placer_opts,
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

