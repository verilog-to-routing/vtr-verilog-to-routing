#ifndef PLACE_CHECKPOINT_H
#define PLACE_CHECKPOINT_H

#include "vtr_util.h"
#include "vpr_types.h"
#include "vtr_vector_map.h"
#include "place_util.h"
#include "globals.h"
#include "timing_info.h"

#include "place_delay_model.h"
#include "place_timing_update.h"
//Placement checkpoint
/**
 * @brief Data structure that stores the placement state and saves it as a checkpoint.
 *
 * The placement checkpoints are very useful to solve the problem of critical 
 * delay oscillations, expecially very late in the annealer. 
 *
 *   @param cost The weighted average of the wiring cost and the timing cost.
 *   @param block_locs saves the location of each block
 *   @param cpd Saves the critical path delay of the current checkpoint
 *   @param valid a flag to show whether the current checkpoint is initialized or not
 */
class t_placement_checkpoint {
  private:
    vtr::vector_map<ClusterBlockId, t_block_loc> block_locs;
    float cpd;
    bool valid = false;
    t_placer_costs costs;

  public:
    //save the block locations from placement context with the current placement cost and cpd
    void save_placement(const t_placer_costs& placement_costs, const float& critical_path_delay);

    //restore the placement solution saved in the checkpoint and update the placement context accordingly
    t_placer_costs restore_placement();

    //return the critical path delay of the saved checkpoint
    float get_cp_cpd();

    //return the WL cost of the saved checkpoint
    double get_cp_bb_cost();

    //return true if the checkpoint is valid
    bool cp_is_valid();
};

//save placement checkpoint if checkpointing is enabled and checkpoint conditions occured
void save_placement_checkpoint_if_needed(t_placement_checkpoint& placement_checkpoint, std::shared_ptr<SetupTimingInfo> timing_info, t_placer_costs& costs, float cpd);

//restore the checkpoint if it's better than the latest placement solution
void restore_best_placement(t_placement_checkpoint& placement_checkpoint, std::shared_ptr<SetupTimingInfo>& timing_info, t_placer_costs& costs, std::unique_ptr<PlacerCriticalities>& placer_criticalities, std::unique_ptr<PlacerSetupSlacks>& placer_setup_slacks, std::unique_ptr<PlaceDelayModel>& place_delay_model, std::unique_ptr<ClusteredPinTimingInvalidator>& pin_timing_invalidator, PlaceCritParams crit_params);
#endif
