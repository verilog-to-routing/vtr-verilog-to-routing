#include "place_checkpoint.h"
#include "noc_place_utils.h"
#include "placer_state.h"
#include "grid_block.h"

float t_placement_checkpoint::get_cp_cpd() const { return cpd_; }

double t_placement_checkpoint::get_cp_bb_cost() const { return costs_.bb_cost; }

bool t_placement_checkpoint::cp_is_valid() const { return valid_; }

void t_placement_checkpoint::save_placement(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                            const t_placer_costs& placement_costs,
                                            const float critical_path_delay) {
    block_locs_ = block_locs;
    valid_ = true;
    cpd_ = critical_path_delay;
    costs_ = placement_costs;
}

t_placer_costs t_placement_checkpoint::restore_placement(vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                                         GridBlock& grid_blocks) {
    block_locs = block_locs_;
    grid_blocks.load_from_block_locs(block_locs);
    return costs_;
}

void save_placement_checkpoint_if_needed(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                         t_placement_checkpoint& placement_checkpoint,
                                         const std::shared_ptr<SetupTimingInfo>& timing_info,
                                         t_placer_costs& costs,
                                         float cpd) {
    if (!placement_checkpoint.cp_is_valid() || (timing_info->least_slack_critical_path().delay() < placement_checkpoint.get_cp_cpd() && costs.bb_cost <= placement_checkpoint.get_cp_bb_cost())) {
        placement_checkpoint.save_placement(block_locs, costs, cpd);
        VTR_LOG("Checkpoint saved: bb_costs=%g, TD costs=%g, CPD=%7.3f (ns) \n", costs.bb_cost, costs.timing_cost, 1e9 * cpd);
    }
}

void restore_best_placement(PlacerState& placer_state,
                            t_placement_checkpoint& placement_checkpoint,
                            std::shared_ptr<SetupTimingInfo>& timing_info,
                            t_placer_costs& costs,
                            std::unique_ptr<PlacerCriticalities>& placer_criticalities,
                            std::unique_ptr<PlacerSetupSlacks>& placer_setup_slacks,
                            std::unique_ptr<PlaceDelayModel>& place_delay_model,
                            std::unique_ptr<NetPinTimingInvalidator>& pin_timing_invalidator,
                            PlaceCritParams crit_params,
                            std::optional<NocCostHandler>& noc_cost_handler) {
    /* The (valid) checkpoint is restored if the following conditions are met:
     * 1) The checkpoint has a lower critical path delay.
     * 2) The checkpoint's wire-length cost is either better than the current solution,
     * or at least is not more than 5% worse than the current solution.
     */
    if (placement_checkpoint.cp_is_valid() && timing_info->least_slack_critical_path().delay() > placement_checkpoint.get_cp_cpd() && costs.bb_cost * 1.05 > placement_checkpoint.get_cp_bb_cost()) {
        //restore the latest placement checkpoint

        costs = placement_checkpoint.restore_placement(placer_state.mutable_block_locs(), placer_state.mutable_grid_blocks());

        //recompute timing from scratch
        placer_criticalities.get()->set_recompute_required();
        placer_setup_slacks.get()->set_recompute_required();
        comp_td_connection_delays(place_delay_model.get(), placer_state);
        perform_full_timing_update(crit_params,
                                   place_delay_model.get(),
                                   placer_criticalities.get(),
                                   placer_setup_slacks.get(),
                                   pin_timing_invalidator.get(),
                                   timing_info.get(),
                                   &costs,
                                   placer_state);

        /* If NoC is enabled, re-compute NoC costs and re-initialize NoC internal data structures.
         * If some routers have different locations than the last placement, NoC-related costs and
         * internal data structures that are used to keep track of each flow's cost are no longer valid,
         * and need to be re-computed from scratch.
         */
        if (noc_cost_handler.has_value()) {
            VTR_ASSERT(noc_cost_handler->points_to_same_block_locs(placer_state.block_locs()));
            noc_cost_handler->reinitialize_noc_routing(costs, {});
        }

        VTR_LOG("\nCheckpoint restored\n");
    }
}
