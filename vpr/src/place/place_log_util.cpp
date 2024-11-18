#include "place_log_util.h"

#include "vtr_log.h"
#include "annealer.h"
#include "place_util.h"
#include "PostClusterDelayCalculator.h"
#include "tatum/TimingReporter.hpp"
#include "VprTimingGraphResolver.h"
#include "timing_info.h"
#include "placer.h"

PlacementLogPrinter::PlacementLogPrinter(const Placer& placer)
    : placer_(placer) {}

void PlacementLogPrinter::print_place_status_header() const {
    const bool noc_enabled = placer_.noc_opts_.noc;

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
    const t_annealing_state& annealing_state = placer_.annealer_->get_annealing_state();
    const auto& [swap_stats, move_type_stats, placer_stats] = placer_.annealer_->get_stats();
    const int tot_moves = placer_.annealer_->get_total_iteration();
    const bool noc_enabled = placer_.noc_opts_.noc;
    const NocCostTerms& noc_cost_terms = placer_.costs_.noc_cost_terms;

    const bool is_timing_driven = placer_.placer_opts_.place_algorithm.is_timing_driven();
    const float cpd = is_timing_driven ? placer_.critical_path_.delay() : std::numeric_limits<float>::quiet_NaN();
    const float sTNS = is_timing_driven ? placer_.timing_info_->setup_total_negative_slack() : std::numeric_limits<float>::quiet_NaN();
    const float sWNS = is_timing_driven ? placer_.timing_info_->setup_worst_negative_slack() : std::numeric_limits<float>::quiet_NaN();

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
}

void PlacementLogPrinter::print_resources_utilization() const {
    const auto& cluster_ctx = g_vpr_ctx.clustering();
    const auto& device_ctx = g_vpr_ctx.device();
    const auto& block_locs = placer_.placer_state_.block_locs();

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
    const auto& [swap_stats, move_type_stats, placer_stats] = placer_.annealer_->get_stats();
    const t_annealing_state& annealing_state = placer_.annealer_->get_annealing_state();

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

