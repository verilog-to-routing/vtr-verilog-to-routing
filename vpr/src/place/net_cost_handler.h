#pragma once
#include "place_delay_model.h"
#include "timing_place.h"
#include "move_transactions.h"
#include "place_util.h"

class PlacerContext;

/**
 * @brief The error tolerance due to round off for the total cost computation.
 * When we check it from scratch vs. incrementally. 0.01 means that there is a 1% error tolerance.
 */
constexpr double ERROR_TOL = .01;

/**
 * @brief The method used to calculate placement cost
 * @details For comp_cost.  NORMAL means use the method that generates updatable bounding boxes for speed.
 * CHECK means compute all bounding boxes from scratch using a very simple routine to allow checks
 * of the other costs.
 * NORMAL: Compute cost efficiently using incremental techniques.
 * CHECK: Brute-force cost computation; useful to validate the more complex incremental cost update code.
 */
enum class e_cost_methods {
    NORMAL,
    CHECK
};

/**
 * @brief Finds the bb cost from scratch (based on 3D BB).
 * Done only when the placement has been radically changed
 * (i.e. after initial placement). Otherwise find the cost
 * change incrementally. If method check is NORMAL, we find
 * bounding boxes that are updatable for the larger nets.
 * If method is CHECK, all bounding boxes are found via the
 * non_updateable_bb routine, to provide a cost which can be
 * used to check the correctness of the other routine.
 * @param method
 * @return The bounding box cost of the placement, computed by the 3D method.
 */
double comp_bb_cost(e_cost_methods method);

/**
 * @brief Finds the bb cost from scratch (based on per-layer BB).
 * Done only when the placement has been radically changed
 * (i.e. after initial placement). Otherwise find the cost change
 * incrementally.  If method check is NORMAL, we find bounding boxes
 * that are updateable for the larger nets.  If method is CHECK, all
 * bounding boxes are found via the non_updateable_bb routine, to provide
 * a cost which can be used to check the correctness of the other routine.
 * @param method
 * @return The placement bounding box cost, computed by the per layer method.
 */
double comp_layer_bb_cost(e_cost_methods method);

/**
 * @brief Reset the net cost function flags (proposed_net_cost and bb_updated_before)
 * @param num_nets_affected
 */
void reset_move_nets();

/**
 * @brief re-calculates different terms of the cost function (wire-length, timing, NoC) and update "costs" accordingly. It is important to note that
 * in this function bounding box and connection delays are not calculated from scratch. However, it iterates over all nets and connections and updates
 * their costs by a complete summation, rather than incrementally.
 * @param placer_opts
 * @param noc_opts
 * @param delay_model
 * @param criticalities
 * @param costs passed by reference and computed by this routine (i.e. returned by reference)
 */
void recompute_costs_from_scratch(const t_placer_opts& placer_opts,
                                  const t_noc_opts& noc_opts,
                                  const PlaceDelayModel* delay_model,
                                  const PlacerCriticalities* criticalities,
                                  t_placer_costs* costs);

/**
 * @brief Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac
 * arrays with the inverse of the average number of tracks per channel
 * between [subhigh] and [sublow].
 * @param place_cost_exp It is an exponent to which you take the average inverse channel
 * capacity; a higher value would favour wider channels more over narrower channels during placement (usually we use 1).
 */
void alloc_and_load_chan_w_factors_for_place_cost(float place_cost_exp);

/**
 * @brief Frees the chanx_place_cost_fac and chany_place_cost_fac arrays.
 */
void free_chan_w_factors_for_place_cost();

/**
 * @brief Resize net_cost, proposed_net_cost, and  bb_updated_before data structures to accommodate all nets.
 * @param num_nets Number of nets in the netlist (clustered currently) that the placement engine uses.
 */
void init_place_move_structs(size_t num_nets);

/**
 * @brief Free net_cost, proposed_net_cost, and  bb_updated_before data structures.
 */
void free_place_move_structs();



/**
 * @brief Free (layer_)ts_bb_edge_new, (layer_)ts_bb_coord_new, ts_layer_sink_pin_count, and ts_nets_to_update data structures.
 */
void free_try_swap_net_cost_structs();

void set_net_handlers_placer_ctx(PlacerContext& placer_ctx);


class NetCostHandler {
  public:
    NetCostHandler() = delete;
    NetCostHandler(const NetCostHandler&) = delete;
    NetCostHandler(NetCostHandler&&) = delete;
    NetCostHandler& operator=(const NetCostHandler&) = delete;
    NetCostHandler& operator=(NetCostHandler&&) = delete;

    /**
    * @brief Resize temporary swap data structures needed to determine which nets are affected by a move and data needed per net
    * about where their terminals are in order to quickly (incrementally) update their wirelength costs. These data structures are
    * (layer_)ts_bb_edge_new, (layer_)ts_bb_coord_new, ts_layer_sink_pin_count, and ts_nets_to_update.
    * @param num_nets Number of nets in the netlist used by the placement engine (currently clustered netlist)
    * @param cube_bb True if the 3D bounding box should be used, false otherwise.
    */
    NetCostHandler(size_t num_nets, bool cube_bb);

    /**
    * @brief Find all the nets and pins affected by this swap and update costs.
    *
    * Find all the nets affected by this swap and update the bounding box (wiring)
    * costs. This cost function doesn't depend on the timing info.
    *
    * Find all the connections affected by this swap and update the timing cost.
    * For a connection to be affected, it not only needs to be on or driven by
    * a block, but it also needs to have its delay changed. Otherwise, it will
    * not be added to the affected_pins structure.
    *
    * For more, see update_td_delta_costs().
    *
    * The timing costs are calculated by getting the new connection delays,
    * multiplied by the connection criticalities returned by the timing
    * analyzer. These timing costs are stored in the proposed_* data structures.
    *
    * The change in the bounding box cost is stored in `bb_delta_c`.
    * The change in the timing cost is stored in `timing_delta_c`.
    * ts_nets_to_update is also extended with the latest net.
    *
    * @param place_algorithm
    * @param delay_model
    * @param criticalities
    * @param blocks_affected
    * @param bb_delta_c
    * @param timing_delta_c
    * @return The number of affected nets.
    */
    void find_affected_nets_and_update_costs(const t_place_algorithm& place_algorithm,
                                             const PlaceDelayModel* delay_model,
                                             const PlacerCriticalities* criticalities,
                                             t_pl_blocks_to_be_moved& blocks_affected,
                                             double& bb_delta_c,
                                             double& timing_delta_c);

    /**
    * @brief update net cost data structures (in placer context and net_cost in .cpp file) and reset flags (proposed_net_cost and bb_updated_before).
    * @param num_nets_affected The number of nets affected by the move. It is used to determine the index up to which elements in ts_nets_to_update are valid.
    */
    void update_move_nets();

  private:
    /**
     * @brief This class is used to hide control flows needed to distinguish 2d and 3d placement
     */
    class BBUpdater {
      public:
        BBUpdater() = delete;
        BBUpdater(const BBUpdater&) = delete;
        BBUpdater(BBUpdater&&) = delete;
        BBUpdater& operator=(const BBUpdater&) = delete;
        BBUpdater& operator=(BBUpdater&&) = delete;

        BBUpdater(size_t num_nets, bool cube_bb);

      private:
        bool cube_bb_ = false;

      public:

        void get_non_updatable_bb(const ClusterNetId net);

        void update_bb(ClusterNetId net_id, t_physical_tile_loc pin_old_loc, t_physical_tile_loc pin_new_loc, bool is_driver);

        double get_net_cost(const ClusterNetId net_id);

        void set_ts_bb_coord(const ClusterNetId net_id);

        void set_ts_edge(const ClusterNetId net_id);
    };

  private:
    BBUpdater bb_updater_;

  private:
    /**
    * @brief Update the bounding box (3D) of the net connected to blk_pin. The old and new locations of the pin are
    * stored in pl_moved_block. The updated bounding box will be stored in ts data structures. Do not update the net
    * cost here since it should only be updated once per net, not once per pin.
    */
    void update_net_bb_(const ClusterNetId net,
                        const ClusterBlockId blk,
                        const ClusterPinId blk_pin,
                        const t_pl_moved_block& pl_moved_block);

    /**
    * @brief Call suitable function based on the bounding box type to update the bounding box of the net connected to pin_id. Also,
    * call the function to update timing information if the placement algorithm is timing-driven.
    * @param place_algorithm Placement algorithm
    * @param delay_model Timing delay model used by placer
    * @param criticalities Connections timing criticalities
    * @param blk_id Block ID of that the moving pin belongs to.
    * @param pin_id Pin ID of the moving pin
    * @param moving_blk_inf Data structure that holds information, e.g., old location and new location, about all moving blocks
    * @param affected_pins Netlist pins which are affected, in terms placement cost, by the proposed move.
    * @param timing_delta_c Timing cost change based on the proposed move
    * @param is_src_moving Is the moving pin the source of a net.
    */
    void update_net_info_on_pin_move_(const t_place_algorithm& place_algorithm,
                                      const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities* criticalities,
                                      const ClusterBlockId blk_id,
                                      const ClusterPinId pin_id,
                                      const t_pl_moved_block& moving_blk_inf,
                                      std::vector<ClusterPinId>& affected_pins,
                                      double& timing_delta_c,
                                      bool is_src_moving);

    /**
    * @brief Calculates and returns the total bb (wirelength) cost change that would result from moving the blocks
    * indicated in the blocks_affected data structure.
    * @param bb_delta_c Cost difference after and before moving the block
    */
    void set_bb_delta_cost_(double& bb_delta_c);

};