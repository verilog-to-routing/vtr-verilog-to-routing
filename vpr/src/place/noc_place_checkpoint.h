#ifndef VTR_ROUTERPLACEMENTCHECKPOINT_H
#define VTR_ROUTERPLACEMENTCHECKPOINT_H

class NocCostHandler;

/**
 * @brief NoC router placement checkpoint
 *
 * This class stores a checkpoint only for NoC router placement.
 * If a checkpoint for all block types is needed, refer to place_checkpoint.h file.
 *
 * The initial placement for NoC routers is done before conventional blocks. Therefore,
 * t_placement_checkpoint could not be used to store a checkpoint as t_placement_checkpoint
 * assumes all blocks are placed.
 *
 * This class should only be used during initial NoC placement as it does not update
 * bounding box and timing costs.
 */

#include "vpr_types.h"
#include "place_util.h"

/**
 * @brief A NoC router placement checkpoint
 *
 * The class stores a NoC router placement and its corresponding cost.
 * The checkpoint can be restored to replace the current placement.
 */
class NoCPlacementCheckpoint {
  public:
    /**
     * @brief Default constructor initializes private member variables.
     */
    explicit NoCPlacementCheckpoint(NocCostHandler& noc_cost_handler);
    NoCPlacementCheckpoint(const NoCPlacementCheckpoint& other) = delete;
    NoCPlacementCheckpoint& operator=(const NoCPlacementCheckpoint& other) = delete;

    /**
     * @brief Saves the current NoC router placement as a checkpoint
     *
     *  @param cost The placement cost associated with the current placement
     *  @param block_locs Stores where each clustered block (including NoC routers) is placed at.
     */
    void save_checkpoint(double cost, const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs);

    /**
     * @brief Loads the save checkpoint into global placement data structues.
     *
     *  @param costs Used to load NoC related costs for the checkpoint
     *  @param blk_loc_registry To be updated with the save checkpoint for NoC router locations.
     */
    void restore_checkpoint(t_placer_costs& costs,
                            BlkLocRegistry& blk_loc_registry);

    /**
     * @brief Indicates whether the object is empty or it has already stored a
     * checkpoint.
     *
     * @return bool True if there is a save checkpoint.
     */
    bool is_valid() const;

    /**
     * @brief Return the cost associated with the checkpoint
     *
     * @return double Saved checkpoint's cost
     */
    double get_cost() const;

  private:
    std::unordered_map<ClusterBlockId, t_pl_loc> router_locations_;
    NocCostHandler& noc_cost_handler_;
    bool valid_ = false;
    double cost_;
};

#endif //VTR_ROUTERPLACEMENTCHECKPOINT_H
