#ifndef VPR_CENTROID_MOVE_GEN_H
#define VPR_CENTROID_MOVE_GEN_H

#include "move_generator.h"

/**
 * @file
 * @author M. Elgammal
 * @brief Centroid move type
 *
 * This move picks a random block and moves it (swapping with what's there if necessary) to the zero force location
 * It calculates forces/weighs acting on the block based on its connections to other blocks.
 *
 * The computed centroid location can be optionally biased towards the location of NoC routers
 * that are reachable from the selected block. A NoC router is reachable from a block if one can
 * start from the block and reach the NoC router only by traversing low fanout nets. All the blocks
 * (including NoC routers) that can reach a NoC router form a NoC group.
 *
 * Returns its choices by filling in affected_blocks.
 */
class CentroidMoveGenerator : public MoveGenerator {
  public:
    /**
     * The move generator created by calling this constructor only considers
     * netlist connectivity for computing the centroid location.
     *
     * @param placer_state A mutable reference to the placement state which will
     * be stored in this object.
     * @param reward_function Specifies the reward function to update q-tables
     * of the RL agent.
     */
    CentroidMoveGenerator(PlacerState& placer_state,
                          e_reward_function reward_function);

    /**
     * The move generator created by calling this constructor considers both
     * netlist connectivity and NoC reachability for computing the centroid.
     * The constructor also forms NoC groups by finding connected components
     * in the graph representing the clustered netlist. When finding connected
     * components, none of the nets whose fanout is larger than high_fanout_net
     * are traversed.
     * @param placer_state A mutable reference to the placement state which will
     * be stored in this object.
     * @param noc_attraction_weight Specifies how much the computed centroid
     * is adjusted towards the location of NoC routers in the same NoC group as
     * the clustered block to be moved.
     * @param high_fanout_net All nets with a fanout larger than this number are
     * ignored when forming NoC groups.
     */
    CentroidMoveGenerator(PlacerState& placer_state,
                          e_reward_function reward_function,
                          float noc_attraction_weight,
                          size_t high_fanout_net);


    /**
     * Returns all NoC routers that are in the NoC group with a given ID.
     * @param noc_grp_id The NoC group ID whose NoC routers are requested.
     * @return The clustered block ID of all NoC routers in the given NoC group.
     */
    const std::vector<ClusterBlockId>& get_noc_group_routers(NocGroupId noc_grp_id);

    /**
     * Returns the NoC group ID of clustered block.
     * @param blk_id The clustered block whose NoC group ID is requested.
     * @return The NoC group ID of the given clustered block or INVALID if
     * the given clustered block does not belong to any NoC groups.
     */
    NocGroupId get_cluster_noc_group(ClusterBlockId blk_id);

  private:
    e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected,
                               t_propose_action& proposed_action,
                               float rlim,
                               const t_placer_opts& placer_opts,
                               const PlacerCriticalities* /*criticalities*/) override;

    /**
     * @brief Calculates the exact centroid location
     *
     * This function is very useful in centroid and weightedCentroid moves as it calculates
     * the centroid location. It returns the calculated location in centroid.
     *
     * When NoC attraction is enabled, the computed centroid is slightly adjusted towards the location
     * of NoC routers that are in the same NoC group b_from.
     *
     * @param b_from The block Id of the moving block
     * @param timing_weights Determines whether to calculate centroid or
     * weighted centroid location. If true, use the timing weights (weighted centroid).
     * @param criticalities A pointer to the placer criticalities which is used when
     * calculating weighted centroid (send a NULL pointer in case of centroid)
     * @param noc_attraction_enabled Indicates whether the computed centroid location
     * should be adjusted towards NoC routers in the NoC group of the given block.
     * @param noc_attraction_weight When NoC attraction is enabled, this weight
     * specifies to which extent the computed centroid should be adjusted. A value
     * in range [0, 1] is expected.
     *
     * @return The calculated location is returned in centroid parameter that is sent by reference
     */
    t_pl_loc calculate_centroid_loc_(ClusterBlockId b_from,
                                     const PlacerCriticalities* criticalities);

  protected:
    bool weighted_;

  private:
    /** A value in range [0, 1] that specifies how much the centroid location
     * computation is biased towards the associated NoC routers*/
    float noc_attraction_weight_;

    /** Indicates whether the centroid calculation is NoC-biased.*/
    bool noc_attraction_enabled_;

    /** Stores the ids of all non-router clustered blocks for each NoC group*/
    vtr::vector<NocGroupId, std::vector<ClusterBlockId>> noc_group_clusters_;

    /** Stores NoC routers in each NoC group*/
    vtr::vector<NocGroupId, std::vector<ClusterBlockId>> noc_group_routers_;

    /** Specifies the NoC group that each block belongs to. A block cannot belong to more
     * than one NoC because this means those NoC groups can reach each other and form
     * a single NoC group. We use NocGroupId::INVALID to show that a block does not belong
     * to any NoC groups. This happens when a block is not reachable from any NoC router.
     * */
    vtr::vector<ClusterBlockId, NocGroupId> cluster_to_noc_grp_;

    /** Specifies the NoC group for each NoC router*/
    std::map<ClusterBlockId, NocGroupId> noc_router_to_noc_group_;

    /**
     * @brief This function forms NoC groups by finding connected components
     * in the graph representing the clustered netlist. When finding connected
     * components, none of the nets whose fanout is larger than high_fanout_net
     * are traversed.
     * @param high_fanout_net All nets with a fanout larger than this number are
     * ignored when forming NoC groups.
     */
    void initialize_noc_groups(size_t high_fanout_net);
};

#endif
