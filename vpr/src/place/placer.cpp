
#include "placer.h"

#include <utility>

#include "FlatPlacementInfo.h"
#include "place_macro.h"
#include "vtr_time.h"
#include "draw.h"
#include "read_place.h"
#include "analytic_placer.h"
#include "initial_placement.h"
#include "load_flat_place.h"
#include "concrete_timing_info.h"
#include "verify_placement.h"
#include "place_timing_update.h"
#include "annealer.h"
#include "RL_agent_util.h"
#include "place_checkpoint.h"
#include "tatum/echo_writer.hpp"

Placer::Placer(const Netlist<>& net_list,
               const PlaceMacros& place_macros,
               const t_placer_opts& placer_opts,
               const t_analysis_opts& analysis_opts,
               const t_noc_opts& noc_opts,
               const IntraLbPbPinLookup& pb_gpin_lookup,
               const ClusteredPinAtomPinsLookup& netlist_pin_lookup,
               const FlatPlacementInfo& flat_placement_info,
               std::shared_ptr<PlaceDelayModel> place_delay_model,
               bool cube_bb,
               bool is_flat,
               bool quiet)
    : place_macros_(place_macros)
    , placer_opts_(placer_opts)
    , analysis_opts_(analysis_opts)
    , noc_opts_(noc_opts)
    , pb_gpin_lookup_(pb_gpin_lookup)
    , netlist_pin_lookup_(netlist_pin_lookup)
    , costs_(placer_opts.place_algorithm, noc_opts.noc)
    , placer_state_(placer_opts.place_algorithm.is_timing_driven(), cube_bb)
    , rng_(placer_opts.seed)
    , net_cost_handler_(placer_opts, placer_state_, cube_bb)
    , place_delay_model_(std::move(place_delay_model))
    , log_printer_(*this, quiet)
    , is_flat_(is_flat) {
    const auto& cluster_ctx = g_vpr_ctx.clustering();

    pre_place_timing_stats_ = g_vpr_ctx.timing().stats;

    init_placement_context(placer_state_.mutable_blk_loc_registry());

    // create a NoC cost handler if NoC optimization is enabled
    if (noc_opts.noc) {
        noc_cost_handler_.emplace(placer_state_.block_locs());
    }

    /* To make sure the importance of NoC-related cost terms compared to
     * BB and timing cost is determine only through NoC placement weighting factor,
     * we normalize NoC-related cost weighting factors so that they add up to 1.
     * With this normalization, NoC-related cost weighting factors only determine
     * the relative importance of NoC cost terms with respect to each other, while
     * the importance of total NoC cost to conventional placement cost is determined
     * by NoC placement weighting factor.
     */
    if (noc_opts.noc) {
        normalize_noc_cost_weighting_factor(const_cast<t_noc_opts&>(noc_opts));
    }

    BlkLocRegistry& blk_loc_registry = placer_state_.mutable_blk_loc_registry();
    initial_placement(placer_opts, placer_opts.constraints_file.c_str(),
                      noc_opts, blk_loc_registry, place_macros_, noc_cost_handler_,
                      flat_placement_info, rng_);

    // After initial placement, if a flat placement is being reconstructed,
    // print flat placement reconstruction info.
    if (flat_placement_info.valid) {
        log_flat_placement_reconstruction_info(flat_placement_info,
                                               blk_loc_registry.block_locs(),
                                               g_vpr_ctx.clustering().atoms_lookup,
                                               g_vpr_ctx.atom().lookup,
                                               g_vpr_ctx.atom().nlist,
                                               g_vpr_ctx.clustering().clb_nlist);
    }

    const int move_lim = (int)(placer_opts.anneal_sched.inner_num * pow(net_list.blocks().size(), 1.3333));
    //create the move generator based on the chosen placement strategy
    auto [move_generator, move_generator2] = create_move_generators(placer_state_, placer_opts, move_lim, noc_opts.noc_centroid_weight, rng_);

    if (!placer_opts.write_initial_place_file.empty()) {
        print_place(nullptr, nullptr, placer_opts.write_initial_place_file.c_str(), placer_state_.block_locs());
    }

#ifdef ENABLE_ANALYTIC_PLACE
    /*
     * Cluster-level Analytic Placer:
     *  Passes in the initial_placement via vpr_context, and passes its placement back via locations marked on
     *  both the clb_netlist and the gird.
     *  Most of anneal is disabled later by setting initial temperature to 0 and only further optimizes in quench
     */
    if (placer_opts.enable_analytic_placer) {
        AnalyticPlacer{blk_loc_registry, place_macros_}.ap_place();
    }

#endif /* ENABLE_ANALYTIC_PLACE */

    // Update physical pin values
   for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
       blk_loc_registry.place_sync_external_block_connections(block_id);
   }

   if (!quiet) {
#ifndef NO_GRAPHICS
       if (noc_cost_handler_.has_value()) {
           get_draw_state_vars()->set_noc_link_bandwidth_usages_ref(noc_cost_handler_->get_link_bandwidth_usages());
       }
#endif

       // width_fac gives the width of the widest channel
       const int width_fac = placer_opts.place_chan_width;
       init_draw_coords((float)width_fac, placer_state_.blk_loc_registry());
   }

   // Gets initial cost and loads bounding boxes.
   costs_.bb_cost = net_cost_handler_.comp_bb_cost(e_cost_methods::NORMAL).first;
   costs_.bb_cost_norm = 1 / costs_.bb_cost;

   if (placer_opts.place_algorithm.is_timing_driven()) {
       alloc_and_init_timing_objects_(net_list, analysis_opts);
   } else {
       VTR_ASSERT(placer_opts.place_algorithm == e_place_algorithm::BOUNDING_BOX_PLACE);
       // Timing cost and normalization factors are not used
       constexpr double INVALID_COST = std::numeric_limits<double>::quiet_NaN();
       costs_.timing_cost = INVALID_COST;
       costs_.timing_cost_norm = INVALID_COST;
   }

   if (noc_opts.noc) {
       VTR_ASSERT(noc_cost_handler_.has_value());

       // get the costs associated with the NoC
       costs_.noc_cost_terms.aggregate_bandwidth = noc_cost_handler_->comp_noc_aggregate_bandwidth_cost();
       std::tie(costs_.noc_cost_terms.latency, costs_.noc_cost_terms.latency_overrun) = noc_cost_handler_->comp_noc_latency_cost();
       costs_.noc_cost_terms.congestion = noc_cost_handler_->comp_noc_congestion_cost();

       // initialize all the noc normalization factors
       noc_cost_handler_->update_noc_normalization_factors(costs_);
   }

   // set the starting total placement cost
   costs_.cost = costs_.get_total_cost(placer_opts, noc_opts);

   // Sanity check that initial placement is legal
   check_place_();

   log_printer_.print_initial_placement_stats();

   annealer_ = std::make_unique<PlacementAnnealer>(placer_opts_, placer_state_, place_macros_, costs_, net_cost_handler_, noc_cost_handler_,
                                                   noc_opts_, rng_, std::move(move_generator), std::move(move_generator2), place_delay_model_.get(),
                                                   placer_criticalities_.get(), placer_setup_slacks_.get(), timing_info_.get(), pin_timing_invalidator_.get(),
                                                   move_lim);
}

void Placer::alloc_and_init_timing_objects_(const Netlist<>& net_list,
                                            const t_analysis_opts& analysis_opts) {
   const auto& atom_ctx = g_vpr_ctx.atom();
   const auto& cluster_ctx = g_vpr_ctx.clustering();
   const auto& timing_ctx = g_vpr_ctx.timing();
   const auto& p_timing_ctx = placer_state_.timing();

   // Update the point-to-point delays from the initial placement
   comp_td_connection_delays(place_delay_model_.get(), placer_state_);

   // Initialize timing analysis
   placement_delay_calc_ = std::make_shared<PlacementDelayCalculator>(atom_ctx.nlist,
                                                                      atom_ctx.lookup,
                                                                      p_timing_ctx.connection_delay,
                                                                      is_flat_);
   placement_delay_calc_->set_tsu_margin_relative(placer_opts_.tsu_rel_margin);
   placement_delay_calc_->set_tsu_margin_absolute(placer_opts_.tsu_abs_margin);

   timing_info_ = make_setup_timing_info(placement_delay_calc_, placer_opts_.timing_update_type);

   placer_setup_slacks_ = std::make_unique<PlacerSetupSlacks>(cluster_ctx.clb_nlist,
                                                              netlist_pin_lookup_,
                                                              timing_info_);

   placer_criticalities_ = std::make_unique<PlacerCriticalities>(cluster_ctx.clb_nlist,
                                                                 netlist_pin_lookup_,
                                                                 timing_info_);

   pin_timing_invalidator_ = make_net_pin_timing_invalidator(placer_opts_.timing_update_type,
                                                             net_list,
                                                             netlist_pin_lookup_,
                                                             atom_ctx.nlist,
                                                             atom_ctx.lookup,
                                                             timing_info_,
                                                             is_flat_);

   // First time compute timing and costs, compute from scratch
   PlaceCritParams crit_params;
   crit_params.crit_exponent = placer_opts_.td_place_exp_first;
   crit_params.crit_limit = placer_opts_.place_crit_limit;

   initialize_timing_info(crit_params, place_delay_model_.get(), placer_criticalities_.get(),
                          placer_setup_slacks_.get(), pin_timing_invalidator_.get(),
                          timing_info_.get(), &costs_, placer_state_);

   critical_path_ = timing_info_->least_slack_critical_path();

   // Write out the initial timing echo file
   if (isEchoFileEnabled(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH)) {
       tatum::write_echo(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH),
                         *timing_ctx.graph, *timing_ctx.constraints,
                         *placement_delay_calc_, timing_info_->analyzer());

       tatum::NodeId debug_tnode = id_or_pin_name_to_tnode(analysis_opts.echo_dot_timing_graph_node);

       write_setup_timing_graph_dot(getEchoFileName(E_ECHO_INITIAL_PLACEMENT_TIMING_GRAPH) + std::string(".dot"),
                                    *timing_info_, debug_tnode);
   }

   costs_.timing_cost_norm = 1 / costs_.timing_cost;
}

void Placer::check_place_() {
   const ClusteredNetlist& clb_nlist = g_vpr_ctx.clustering().clb_nlist;
   const DeviceGrid& device_grid = g_vpr_ctx.device().grid;
   const auto& cluster_constraints = g_vpr_ctx.floorplanning().cluster_constraints;

   int error = 0;

   // Verify the placement invariants independent to the placement flow.
   error += verify_placement(placer_state_.blk_loc_registry(),
                             place_macros_,
                             clb_nlist,
                             device_grid,
                             cluster_constraints);

   error += check_placement_costs_();

   if (noc_opts_.noc) {
       // check the NoC costs during placement if the user is using the NoC supported flow
       error += noc_cost_handler_->check_noc_placement_costs(costs_, PL_INCREMENTAL_COST_TOLERANCE, noc_opts_);
       // make sure NoC routing configuration does not create any cycles in CDG
       error += (int)noc_cost_handler_->noc_routing_has_cycle();
   }

   if (error == 0) {
       VTR_LOGV(!log_printer_.quiet(),
                "\nCompleted placement consistency check successfully.\n");

   } else {
       VPR_ERROR(VPR_ERROR_PLACE,
                 "\nCompleted placement consistency check, %d errors found.\n"
                 "Aborting program.\n",
                 error);
   }
}

int Placer::check_placement_costs_() {
   int error = 0;
   double timing_cost_check;

   const auto [bb_cost_check, expected_wirelength] = net_cost_handler_.comp_bb_cost(e_cost_methods::CHECK);
   VTR_LOGV(!log_printer_.quiet(),
            "\nBB estimate of min-dist (placement) wire length: %.0f\n", expected_wirelength);


   if (fabs(bb_cost_check - costs_.bb_cost) > costs_.bb_cost * PL_INCREMENTAL_COST_TOLERANCE) {
       VTR_LOG_ERROR(
           "bb_cost_check: %g and bb_cost: %g differ in check_place.\n",
           bb_cost_check, costs_.bb_cost);
       error++;
   }

   if (placer_opts_.place_algorithm.is_timing_driven()) {
       comp_td_costs(place_delay_model_.get(), *placer_criticalities_, placer_state_, &timing_cost_check);
       if (fabs(timing_cost_check - costs_.timing_cost) > costs_.timing_cost * PL_INCREMENTAL_COST_TOLERANCE) {
           VTR_LOG_ERROR(
               "timing_cost_check: %g and timing_cost: %g differ in check_place.\n",
               timing_cost_check, costs_.timing_cost);
           error++;
       }
   }
   return error;
}

void Placer::place() {
   const auto& timing_ctx = g_vpr_ctx.timing();
   const auto& cluster_ctx = g_vpr_ctx.clustering();


   bool skip_anneal = false;
#ifdef ENABLE_ANALYTIC_PLACE
   // Cluster-level analytic placer: when enabled, skip most of the annealing and go straight to quench
   if (placer_opts_.enable_analytic_placer) {
       skip_anneal = true;
   }
#endif

   if (!skip_anneal) {
       // Table header
       log_printer_.print_place_status_header();

       // Outer loop of the simulated annealing begins
       do {
           vtr::Timer temperature_timer;

           annealer_->outer_loop_update_timing_info();

           if (placer_opts_.place_algorithm.is_timing_driven()) {
               critical_path_ = timing_info_->least_slack_critical_path();

               // see if we should save the current placement solution as a checkpoint
               if (placer_opts_.place_checkpointing && annealer_->get_agent_state() == e_agent_state::LATE_IN_THE_ANNEAL) {
                   save_placement_checkpoint_if_needed(placer_state_.mutable_block_locs(),
                                                       placement_checkpoint_,
                                                       timing_info_, costs_, critical_path_.delay());
               }
           }

           // do a complete inner loop iteration
           annealer_->placement_inner_loop();

           log_printer_.print_place_status(temperature_timer.elapsed_sec());

           // Outer loop of the simulated annealing ends
       } while (annealer_->outer_loop_update_state());
   } //skip_anneal ends

    // Start Quench
    annealer_->start_quench();

    pre_quench_timing_stats_ = timing_ctx.stats;
    { // Quench
       vtr::ScopedFinishTimer temperature_timer("Placement Quench");

       annealer_->outer_loop_update_timing_info();

       /* Run inner loop again with temperature = 0 so as to accept only swaps
        * which reduce the cost of the placement */
       annealer_->placement_inner_loop();

       if (placer_opts_.place_quench_algorithm.is_timing_driven()) {
           critical_path_ = timing_info_->least_slack_critical_path();
       }

       log_printer_.print_place_status(temperature_timer.elapsed_sec());
    }
    post_quench_timing_stats_ = timing_ctx.stats;

    // Final timing analysis
    const t_annealing_state& annealing_state = annealer_->get_annealing_state();
    PlaceCritParams crit_params;
    crit_params.crit_exponent = annealing_state.crit_exponent;
    crit_params.crit_limit = placer_opts_.place_crit_limit;

    if (placer_opts_.place_algorithm.is_timing_driven()) {
       perform_full_timing_update(crit_params, place_delay_model_.get(), placer_criticalities_.get(),
                                  placer_setup_slacks_.get(), pin_timing_invalidator_.get(),
                                  timing_info_.get(), &costs_, placer_state_);

       critical_path_ = timing_info_->least_slack_critical_path();

       VTR_LOGV(!log_printer_.quiet(),
                "post-quench CPD = %g (ns) \n", 1e9 * critical_path_.delay());
    }

    // See if our latest checkpoint is better than the current placement solution
    if (placer_opts_.place_checkpointing) {
       restore_best_placement(placer_state_,
                              placement_checkpoint_, timing_info_, costs_,
                              placer_criticalities_, placer_setup_slacks_, place_delay_model_,
                              pin_timing_invalidator_, crit_params, noc_cost_handler_);
    }

    if (placer_opts_.placement_saves_per_temperature >= 1) {
       std::string filename = vtr::string_fmt("placement_%03d_%03d.place",
                                              annealing_state.num_temps + 1, 0);
       VTR_LOGV(!log_printer_.quiet(), "Saving final placement to file: %s\n", filename.c_str());
       print_place(nullptr, nullptr, filename.c_str(), placer_state_.mutable_block_locs());
    }

    // Update physical pin values
    for (const ClusterBlockId block_id : cluster_ctx.clb_nlist.blocks()) {
       placer_state_.mutable_blk_loc_registry().place_sync_external_block_connections(block_id);
    }

    check_place_();

    log_printer_.print_post_placement_stats();
}

void Placer::copy_locs_to_global_state(PlacementContext& place_ctx) {
    // the placement location variables should be unlocked before being accessed
    place_ctx.unlock_loc_vars();

    // copy the local location variables into the global state
    auto& global_blk_loc_registry = place_ctx.mutable_blk_loc_registry();
    global_blk_loc_registry = placer_state_.blk_loc_registry();

#ifndef NO_GRAPHICS
    // update the graphics' reference to placement location variables
    get_draw_state_vars()->set_graphics_blk_loc_registry_ref(global_blk_loc_registry);
#endif
}
