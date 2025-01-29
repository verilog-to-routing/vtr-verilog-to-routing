
#include "multi_placer.h"

#include "annealer.h"
#include "vtr_math.h"
#include "RL_agent_util.h"

#include <ranges>
#include <future>

MultiPlacer::MultiPlacer(const Netlist<>& net_list,
                         const t_placer_opts& placer_opts,
                         const t_analysis_opts& analysis_opts,
                         const t_noc_opts& noc_opts,
                         const IntraLbPbPinLookup& pb_gpin_lookup,
                         const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                         const std::vector<t_direct_inf>& directs,
                         const FlatPlacementInfo& flat_placement_info,
                         std::shared_ptr<PlaceDelayModel> place_delay_model,
                         bool cube_bb,
                         bool is_flat)
    : num_annealers_(placer_opts.multi_placer_num_annealers)
    , placer_opts_(placer_opts.multi_placer_num_annealers, placer_opts)
    , stop_condition_barrier_(placer_opts.multi_placer_num_annealers) {
    VTR_ASSERT(placer_opts.multi_placer_enabled);
    VTR_ASSERT(num_annealers_ >= 2);

    for (int i = 0; i < num_annealers_; i++) {
        placer_opts_[i].seed = placer_opts.seed + i;
    }

    placers_.resize(num_annealers_);

    std::vector<std::future<void>> futures;
    futures.reserve(num_annealers_ - 1);

    for (int t = 1; t < num_annealers_; ++t) {
        futures.emplace_back(std::async(std::launch::async, [&, t]() {
            placers_[t] = std::make_unique<Placer>(net_list,
                                                   placer_opts_[t],
                                                   analysis_opts,
                                                   noc_opts,
                                                   pb_gpin_lookup,
                                                   netlist_pin_lookup,
                                                   directs,
                                                   flat_placement_info,
                                                   place_delay_model,
                                                   cube_bb,
                                                   is_flat,
                                                   /*quiet=*/true);
        }));
    }

    placers_[0] = std::make_unique<Placer>(net_list,
                                           placer_opts_[0],
                                           analysis_opts,
                                           noc_opts,
                                           pb_gpin_lookup,
                                           netlist_pin_lookup,
                                           directs,
                                           flat_placement_info,
                                           place_delay_model,
                                           cube_bb,
                                           is_flat,
                                           /*quiet=*/true);

    // Wait for all threads to complete
    for (auto& future : futures) {
        future.get();
    }
}

void MultiPlacer::place() {
    std::vector<std::future<void>> futures;
    futures.reserve(num_annealers_ - 1);

    for (int t = 1; t < num_annealers_; ++t) {

        // Launch each Placer's place() method in a separate thread
        futures.push_back(std::async(std::launch::async, [t, this]() {
            placers_[t]->place();
        }));
    }

    placers_[0]->place();

    for (auto& future : futures) {
        future.get();
    }
}

void MultiPlacer::copy_locs_to_global_state(PlacementContext& place_ctx) {
    std::vector<double> critical_delays(num_annealers_, 0.);
    std::vector<double> wirelengths(num_annealers_, 0.);
    for (size_t i = 0; const auto& placer : placers_) {
        critical_delays[i] = placer->critical_path_.delay();
        auto [_, estimated_wl] = placer->net_cost_handler_.comp_bb_cost(e_cost_methods::CHECK);
        wirelengths[i] = estimated_wl;

        i++;
    }

    double cpd_geomean = vtr::geomean(critical_delays);
    double wl_geomean = vtr::geomean(wirelengths);

    const double selection_timing_tradeoff = placer_opts_[0].multi_placer_selection_timing_tradeoff;
    std::vector<double> costs(num_annealers_, 0.);
    for (int i = 0; i < num_annealers_; i++) {
        costs[i] = selection_timing_tradeoff * critical_delays[i] / cpd_geomean;
        costs[i] += (1.0 - selection_timing_tradeoff) * wirelengths[i] / wl_geomean;
    }

    auto min_cost_idx = std::distance(costs.begin(), std::ranges::min_element(costs));

    placers_[min_cost_idx]->copy_locs_to_global_state(place_ctx);
}