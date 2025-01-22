
#include "parallel_temperer.h"

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
    : num_annealers_(num_parallel_annealers)
    , rng_(placer_opts.seed + 97983)
    , temperature_swap_barrier_(num_parallel_annealers, std::bind(&ParallelTemperer::try_annealer_swap, this))
    , stop_condition_barrier_(num_parallel_annealers)
    , stop_flag_(false) {
    VTR_ASSERT(num_annealers_ >= 2);

    placer_opts_.resize(num_annealers_, placer_opts);
    for (int i = 0; i < num_annealers_; i++) {
        placer_opts_[i].seed = placer_opts.seed + i;
    }

    placers_.resize(num_annealers_);

    std::vector<std::future<void>> futures;
    futures.reserve(num_annealers_);

    for (int t = 1; t < num_annealers_; ++t) {
        futures.emplace_back(std::async(std::launch::async, [&]() {
            placers_[t] = std::make_unique<Placer>(net_list,
                                                   placer_opts_[t],
                                                   analysis_opts,
                                                   noc_opts,
                                                   pb_gpin_lookup,
                                                   netlist_pin_lookup,
                                                   directs,
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

void ParallelTemperer::run_thread_(int worker_id) {
    Placer& placer = *placers_[worker_id];

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

        temperature_swap_barrier_.arrive_and_wait();

        bool continue_annealing = placer.annealer_->outer_loop_update_state();

        if (worker_id == num_annealers_ / 2) {
            stop_flag_.store(!continue_annealing);
        }

        stop_condition_barrier_.arrive_and_wait();

        if (stop_flag_.load()) {
            break;
        }
    }

    // Start Quench
    placer.annealer_->start_quench();

//    pre_quench_timing_stats_ = timing_ctx.stats;
    { // Quench
//        vtr::ScopedFinishTimer temperature_timer("Placement Quench");

        placer.annealer_->outer_loop_update_timing_info();

        /* Run inner loop again with temperature = 0 so as to accept only swaps
        * which reduce the cost of the placement */
        placer.annealer_->placement_inner_loop();

        if (placer.placer_opts_.place_quench_algorithm.is_timing_driven()) {
            placer.critical_path_ = placer.timing_info_->least_slack_critical_path();
        }

//        log_printer_.print_place_status(temperature_timer.elapsed_sec());
    }
//    post_quench_timing_stats_ = timing_ctx.stats;


    // Final timing analysis
    const t_annealing_state& annealing_state = placer.annealer_->annealing_state();
    PlaceCritParams crit_params {
        .crit_exponent = annealing_state.crit_exponent,
        .crit_limit = placer.placer_opts_.place_crit_limit
    };


    if (placer.placer_opts_.place_algorithm.is_timing_driven()) {
        perform_full_timing_update(crit_params, placer.place_delay_model_.get(), placer.placer_criticalities_.get(),
                                   placer.placer_setup_slacks_.get(), placer.pin_timing_invalidator_.get(),
                                   placer.timing_info_.get(), &placer.costs_, placer.placer_state_);

        placer.critical_path_ = placer.timing_info_->least_slack_critical_path();

//        VTR_LOG("post-quench CPD = %g (ns) \n",
//                1e9 * critical_path_.delay());
    }

    // See if our latest checkpoint is better than the current placement solution
    if (placer.placer_opts_.place_checkpointing) {
        restore_best_placement(placer.placer_state_,
                               placer.placement_checkpoint_, placer.timing_info_, placer.costs_,
                               placer.placer_criticalities_, placer.placer_setup_slacks_, placer.place_delay_model_,
                               placer.pin_timing_invalidator_, crit_params, placer.noc_cost_handler_);
    }

//    if (placer.placer_opts_.placement_saves_per_temperature >= 1) {
//        std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
//                                               annealing_state.num_temps + 1, 0);
//        VTR_LOG("Saving final placement to file: %s\n", filename.c_str());
//        print_place(nullptr, nullptr, filename.c_str(), placer.placer_state_.mutable_block_locs());
//    }

    const auto& cluster_ctx = g_vpr_ctx.clustering();
    // Update physical pin values
    for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
        placer.placer_state_.mutable_blk_loc_registry().place_sync_external_block_connections(block_id);
    }

    placer.check_place_();

//    log_printer_.print_post_placement_stats();
}

void ParallelTemperer::try_annealer_swap() {

    std::vector<double> bb_costs(num_annealers_);
    std::vector<double> timing_costs(num_annealers_);

    for (int i = 0; i < num_annealers_; i++) {
        const t_placer_costs& costs = placers_[i]->annealer_->costs();
        bb_costs[i] = costs.bb_cost;
        timing_costs[i] = costs.timing_cost;
    }

    const double mean_bb_cost = vtr::geomean(bb_costs);
    const double mean_timing_cost = vtr::geomean(timing_costs);

    const double bb_cost_norm = 1 / mean_bb_cost;
    // Prevent the norm factor from going to infinity
    const double timing_cost_norm = std::min(1. / mean_timing_cost, t_placer_costs::MAX_INV_TIMING_COST);

    const double timing_tradeoff = placer_opts_[0].timing_tradeoff;
    std::vector<double> energies(num_annealers_);
    for (int i = 0; i < num_annealers_; i++) {
        const t_placer_costs& costs = placers_[i]->annealer_->costs();
        energies[i] = (1. - timing_tradeoff) * (costs.bb_cost * bb_cost_norm) + (timing_tradeoff) * (costs.timing_cost * timing_cost_norm);
    }

    std::vector<int> sorted_temperature_indices(num_annealers_);
    std::iota(sorted_temperature_indices.begin(), sorted_temperature_indices.end(), 0);
    std::ranges::sort(sorted_temperature_indices, [this](int i1, int i2) noexcept{
        return placers_[i1]->annealer_->annealing_state().temperature() < placers_[i2]->annealer_->annealing_state().temperature();
    });


    std::vector<bool> swap_temperatures(num_annealers_ - 1, false);
    for (int i = 0; i < num_annealers_ - 1; i++) {
        int i1 = sorted_temperature_indices[i];
        double t1 = placers_[i1]->annealer_->annealing_state().temperature();
        double e1 = energies[i1];
        int i2 = sorted_temperature_indices[i + 1];
        double t2 = placers_[i2]->annealer_->annealing_state().temperature();
        double e2 = energies[i2];

        double p_exchange = std::min(1., std::exp( (e1 - e2) * ((1. / t1) - (1. / t2))  ));
        double prob = rng_.frand();

        swap_temperatures[i] = (prob < p_exchange);
    }

    VTR_ASSERT(swap_temperatures.size() >= 2);
    for (size_t i = 1; i < swap_temperatures.size(); ++i) {
        if (swap_temperatures[i] && swap_temperatures[i - 1]) {
            swap_temperatures[i] = false;
        }
    }

    VTR_ASSERT(swap_temperatures.size() == sorted_temperature_indices.size() - 1);
    for (size_t i = 0; i < swap_temperatures.size(); i++) {
        if (swap_temperatures[i]) {
            int i1 = sorted_temperature_indices[i];
            int i2 = sorted_temperature_indices[i + 1];
            float t1 = placers_[i1]->annealer_->annealing_state().temperature();
            float t2 = placers_[i2]->annealer_->annealing_state().temperature();
            placers_[i1]->annealer_->mutable_annealing_state().t = t2;
            placers_[i2]->annealer_->mutable_annealing_state().t = t1;
        }
    }
}
