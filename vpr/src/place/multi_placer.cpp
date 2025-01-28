
#include "multi_placer.h"

#include "annealer.h"
#include "vtr_math.h"
#include "RL_agent_util.h"

#include <ranges>
#include <future>

static std::vector<double> create_geometric_vector(double number, int n, double l, double h);

static std::vector<double> create_geometric_vector(double number, int n, double l, double h) {
    std::vector<double> result;

    double smallest = number / l;
    double largest = number * h;
    double ratio = std::pow(largest / smallest, 1.0 / (n - 1));

    result.reserve(n);
    for (int i = 0; i < n; ++i) {
        result.push_back(smallest * std::pow(ratio, i));
    }

    return result;
}

MultiPlacer::MultiPlacer(int num_parallel_annealers,
                         const Netlist<>& net_list,
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
    : num_annealers_(num_parallel_annealers)
    , placer_opts_(num_parallel_annealers, placer_opts)
    , stop_condition_barrier_(num_parallel_annealers)
    , stop_flag_(false) {
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

    std::vector<double> initial_temperatures(num_annealers_, 0.);
    for (int i = 0; i < num_annealers_; i++) {
        initial_temperatures[i] = placers_[i]->annealer_->annealing_state().temperature();
    }
    const double mean_initial_temperature = vtr::geomean(initial_temperatures);

    // TODO: make these arguments
    const double lowest_temperature_scale_factor = 1.5;
    const double highest_temperature_scale_factor = 1.5;

    std::vector<double> modified_initial_temperatures = create_geometric_vector(mean_initial_temperature,
                                                                                num_annealers_,
                                                                                highest_temperature_scale_factor,
                                                                                lowest_temperature_scale_factor);

    for (int i = 0; i < num_annealers_; i++) {
        placers_[i]->annealer_->mutable_annealing_state().set_temperature((float)modified_initial_temperatures[i]);
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

    for (size_t i = 0; const auto& placer : placers_) {
        double cpd = placer->critical_path_.delay();
        auto [_, estimated_wl] = placer->net_cost_handler_.comp_bb_cost(e_cost_methods::CHECK);

        std::cout << "Placer " << i << ": Estimated wirelength: " << estimated_wl << " , CPD: " << cpd * 1.0e9 << std::endl;

        i++;
    }
}