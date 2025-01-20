
#include "parallel_temperer.h"

#include "annealer.h"
#include "vtr_math.h"
#include "RL_agent_util.h"

std::vector<double> create_geometric_vector(double number, int n, double l, double h) {
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

ParallelTemperer::ParallelTemperer(int num_parallel_annealers,
                                   const Netlist<>& net_list,
                                   const t_placer_opts& placer_opts,
                                   const t_analysis_opts& analysis_opts,
                                   const t_noc_opts& noc_opts,
                                   const IntraLbPbPinLookup& pb_gpin_lookup,
                                   const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
                                   const std::vector<t_direct_inf>& directs,
                                   std::shared_ptr<PlaceDelayModel> place_delay_model,
                                   bool cube_bb,
                                   bool is_flat)
    : num_annealers_(num_parallel_annealers) {
    VTR_ASSERT(num_annealers_ >= 2);

    placer_opts_.resize(num_annealers_, placer_opts);
    for (int i = 0; i < num_annealers_; i++) {
        placer_opts_[i].seed = placer_opts.seed + i;
    }

    placers_.reserve(num_annealers_);
    for (int i = 0; i < num_annealers_; i++) {
        placers_[i] = std::make_unique<Placer>(net_list,
                                               placer_opts,
                                               analysis_opts,
                                               noc_opts,
                                               pb_gpin_lookup,
                                               netlist_pin_lookup,
                                               directs,
                                               place_delay_model,
                                               cube_bb,
                                               is_flat,
                                               /*quiet=*/true);
    }

    std::vector<double> initial_temperatures(num_annealers_, 0.);
    for (int i = 0; i < num_annealers_; i++) {
        initial_temperatures[i] = placers_[i]->annealer_->annealing_state().temperature();
    }
    const double mean_initial_temperature = vtr::geomean(initial_temperatures);

    // TODO: make these arguments
    const double lowest_temperature_scale_factor = 10.0;
    const double highest_temperature_scale_factor = 10.0;

    std::vector<double> modified_initial_temperatures = create_geometric_vector(mean_initial_temperature,
                                                                                num_annealers_,
                                                                                highest_temperature_scale_factor,
                                                                                lowest_temperature_scale_factor);

    for (int i = 0; i < num_annealers_; i++) {
        placers_[i]->annealer_->mutable_annealing_state().set_temperature((float)modified_initial_temperatures[i]);
    }
}

void ParallelTemperer::run_thread_(int thread_id) {
    Placer& placer = *placers_[thread_id];

    // Outer loop of the simulated annealing begins
    while (true) {
        placer.annealer_->outer_loop_update_timing_info();

        if (placer.placer_opts_.place_algorithm.is_timing_driven()) {
            placer.critical_path_ = placer.timing_info_->least_slack_critical_path();

            // see if we should save the current placement solution as a checkpoint
            if (placer.placer_opts_.place_checkpointing && placer.annealer_->get_agent_state() == e_agent_state::LATE_IN_THE_ANNEAL) {
                save_placement_checkpoint_if_needed(placer.placer_state_.mutable_block_locs(),
                                                    placer.placement_checkpoint_,
                                                    placer.timing_info_, placer.costs_, placer.critical_path_.delay());
            }
        }

        // do a complete inner loop iteration
        placer.annealer_->placement_inner_loop();

        placer.annealer_->outer_loop_update_state();
        // Outer loop of the simulated annealing ends
    }
}
