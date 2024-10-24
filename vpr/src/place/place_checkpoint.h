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

class NocCostHandler;

/**
 * @brief Data structure that stores the placement state and saves it as a checkpoint.
 *
 * The placement checkpoints are very useful to solve the problem of critical 
 * delay oscillations, especially very late in the annealer.
 *   @param block_locs_ saves the location of each block
 *   @param cpd_ Saves the critical path delay of the current checkpoint
 *   @param valid_ a flag to show whether the current checkpoint is initialized or not
 *   @param costs_ The weighted average of the wiring cost and the timing cost.
 */
class t_placement_checkpoint {
  private:
    vtr::vector_map<ClusterBlockId, t_block_loc> block_locs_;
    float cpd_;
    bool valid_ = false;
    t_placer_costs costs_;

  public:
    /**
     * @brief Saves the given block locations and their corresponding placement cost and CPD
     * @param block_locs The block locations to be saved.
     * @param placement_costs Different cost terms associated with the given placement.
     * @param critical_path_delay The critical path delay associated with the given placement.
     */
    void save_placement(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                        const t_placer_costs& placement_costs,
                        const float critical_path_delay);

    /**
     * @brief Restores the placement solution saved in the checkpoint and update the placement context accordingly
     * @param block_locs To be filled with the saved placement.
     * @param grid_blocks To be filled with grid location to clustered block mapping of the saved placement.
     * @return Different cost terms associated with the saved placement.
     */
    t_placer_costs restore_placement(vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                     GridBlock& grid_blocks);

    //return the critical path delay of the saved checkpoint
    float get_cp_cpd() const;

    //return the WL cost of the saved checkpoint
    double get_cp_bb_cost() const;

    //return true if the checkpoint is valid
    bool cp_is_valid() const;
};

//save placement checkpoint if checkpointing is enabled and checkpoint conditions occurred
void save_placement_checkpoint_if_needed(const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs,
                                         t_placement_checkpoint& placement_checkpoint,
                                         const std::shared_ptr<SetupTimingInfo>& timing_info,
                                         t_placer_costs& costs,
                                         float cpd);

//restore the checkpoint if it's better than the latest placement solution
void restore_best_placement(PlacerState& placer_state,
                            t_placement_checkpoint& placement_checkpoint,
                            std::shared_ptr<SetupTimingInfo>& timing_info,
                            t_placer_costs& costs,
                            std::unique_ptr<PlacerCriticalities>& placer_criticalities,
                            std::unique_ptr<PlacerSetupSlacks>& placer_setup_slacks,
                            std::unique_ptr<PlaceDelayModel>& place_delay_model,
                            std::unique_ptr<NetPinTimingInvalidator>& pin_timing_invalidator,
                            PlaceCritParams crit_params,
                            std::optional<NocCostHandler>& noc_cost_handler);
#endif
