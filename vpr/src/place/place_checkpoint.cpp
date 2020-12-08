#include "place_checkpoint.h"

float t_placement_checkpoint::get_cp_cpd() { return cpd; }
double t_placement_checkpoint::get_cp_bb_cost() { return costs.bb_cost; }
bool t_placement_checkpoint::cp_is_valid() { return valid; }

void t_placement_checkpoint::save_placement(const t_placer_costs& COSTS, const float& CPD) {
    auto& place_ctx = g_vpr_ctx.placement();
    block_locs = place_ctx.block_locs;
    valid = true;
    cpd = CPD;
    costs = COSTS;
}

t_placer_costs t_placement_checkpoint::restore_placement() {
    auto& mutable_place_ctx = g_vpr_ctx.mutable_placement();
    mutable_place_ctx.block_locs = block_locs;
    load_grid_blocks_from_block_locs();
    return costs;
}

void save_placement_checkpoint_if_needed(t_placement_checkpoint& placement_checkpoint, std::shared_ptr<SetupTimingInfo> timing_info, t_placer_costs& costs, float CPD) {

    if (placement_checkpoint.cp_is_valid() == false || (timing_info->least_slack_critical_path().delay() < placement_checkpoint.get_cp_cpd() && costs.bb_cost <= placement_checkpoint.get_cp_bb_cost())) {
        placement_checkpoint.save_placement(costs, CPD);
        VTR_LOG("Checkpoint saved: bb_costs=%g, TD costs=%g, CPD=%7.3f (ns) \n", costs.bb_cost, costs.timing_cost, 1e9 * CPD);
    }
}
