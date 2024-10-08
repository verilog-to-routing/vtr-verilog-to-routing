/**
 * @file net_cost_handler.h
 * @brief This file contains the declaration of NetCostHandler class used to update placement cost when a new move is proposed/committed.
 * For more details on the overall algorithm, refer to the comment at the top of the net_cost_handler.cpp
 */

#pragma once

#include "place_delay_model.h"
#include "timing_place.h"
#include "move_transactions.h"
#include "place_util.h"
#include "vtr_ndoffsetmatrix.h"

#include <functional>

class PlacerState;

/**
 * @brief The method used to calculate placement cost
 * @details For comp_cost. NORMAL means use the method that generates updatable bounding boxes for speed.
 * CHECK means compute all bounding boxes from scratch using a very simple routine to allow checks
 * of the other costs.
 * NORMAL: Compute cost efficiently using incremental techniques.
 * CHECK: Brute-force cost computation; useful to validate the more complex incremental cost update code.
 */
enum class e_cost_methods {
    NORMAL,
    CHECK
};


class NetCostHandler {
  public:
    NetCostHandler() = delete;
    NetCostHandler(const NetCostHandler&) = delete;
    NetCostHandler& operator=(const NetCostHandler&) = delete;
    NetCostHandler(NetCostHandler&&) = delete;
    NetCostHandler& operator=(NetCostHandler&&) = delete;

    /**
     * @brief Initializes a NetCostHandler object, which contains temporary swap data structures needed to determine which nets
     * are affected by a move and data needed per net about where their terminals are in order to quickly (incrementally) update
     * their wirelength costs. These data structures are (layer_)ts_bb_edge_new, (layer_)ts_bb_coord_new, ts_layer_sink_pin_count,
     * and ts_nets_to_update.
     * @param num_nets Number of nets in the netlist used by the placement engine (currently clustered netlist)
     * @param cube_bb True if the 3D bounding box should be used, false otherwise.
     * @param place_cost_exp It is an exponent to which you take the average inverse channel
     */
    NetCostHandler(const t_placer_opts& placer_opts, PlacerState& placer_state, size_t num_nets, bool cube_bb);

    /**
     * @brief Finds the bb cost from scratch.
     * Done only when the placement has been radically changed
     * (i.e. after initial placement). Otherwise find the cost
     * change incrementally. If method check is NORMAL, we find
     * bounding boxes that are updatable for the larger nets.
     * If method is CHECK, all bounding boxes are found via the
     * non_updateable_bb routine, to provide a cost which can be
     * used to check the correctness of the other routine.
     * @param method The method used to calculate placement cost.
     * @return The bounding box cost of the placement.
     */
    double comp_bb_cost(e_cost_methods method);

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
    * @return The number of affected nets.
    */
    void find_affected_nets_and_update_costs(const PlaceDelayModel* delay_model,
                                             const PlacerCriticalities* criticalities,
                                             t_pl_blocks_to_be_moved& blocks_affected,
                                             double& bb_delta_c,
                                             double& timing_delta_c);

    /**
     * @brief Reset the net cost function flags (proposed_net_cost and bb_updated_before)
     */
    void reset_move_nets();

    /**
     * @brief Update net cost data structures (in placer context and net_cost in .cpp file)
     * and reset flags (proposed_net_cost and bb_updated_before).
     * @param num_nets_affected The number of nets affected by the move.
     * It is used to determine the index up to which elements in ts_nets_to_update are valid.
     */
    void update_move_nets();

    /**
     * @brief Re-calculates different terms of the cost function (wire-length, timing, NoC)
     * and update "costs" accordingly. It is important to note that in this function bounding box
     * and connection delays are not calculated from scratch. However, it iterates over all nets
     * and connections and updates their costs by a complete summation, rather than incrementally.
     * @param noc_opts Contains NoC cost weighting factors.
     * @param delay_model Placement delay model. Used to compute timing cost.
     * @param criticalities Contains the clustered netlist connection criticalities.
     * Used to computed timing cost .
     * @param costs passed by reference and computed by this routine (i.e. returned by reference)
     */
    void recompute_costs_from_scratch(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities* criticalities,
                                      t_placer_costs& costs);

  private:
    ///@brief Specifies whether the bounding box is computed using cube method or per-layer method.
    bool cube_bb_ = false;
    ///@brief A reference to the placer's state to be updated by this object.
    PlacerState& placer_state_;
    ///@brief Contains some parameter that determine how the placement cost is computed.
    const t_placer_opts& placer_opts_;
    ///@brief Points to the proper method for computing the bounding box cost from scratch.
    std::function<double(e_cost_methods method)> comp_bb_cost_functor_;
    ///@brief Points to the proper method for updating the bounding box of a net.
    std::function<void(ClusterNetId net_id, t_physical_tile_loc pin_old_loc, t_physical_tile_loc pin_new_loc, bool is_driver)> update_bb_functor_;
    ///@brief Points to the proper method for getting the bounding box cost of a net
    std::function<double(ClusterNetId)> get_net_bb_cost_functor_;
    ///@brief Points to the proper method for getting the non-updatable bounding box of a net
    std::function<void(const ClusterNetId net)> get_non_updatable_bb_functor_;

    /**
     * @brief for the states of the bounding box.
     */
    enum class NetUpdateState {
        NOT_UPDATED_YET,
        UPDATED_ONCE,
        GOT_FROM_SCRATCH
    };

    /**
     * @brief The wire length estimation is based on the bounding box of the net. In the case of the 2D architecture,
     * we use a 3D BB with the z-dimension (layer) set to 1. In the case of 3D architecture, there 2 types of bounding box:
     * 3D and per-layer. The type is determined at the beginning of the placement and stored in the placement context.
     * If the bonding box is of the type 3D, ts_bb_coord_new and ts_bb_edge_new are used. Otherwise, layer_ts_bb_edge_new and
     * layer_ts_bb_coord_new are used.
     */
    /* [0...cluster_ctx.clb_nlist.nets().size()-1] -> 3D bounding box*/
    vtr::vector<ClusterNetId, t_bb> ts_bb_coord_new_, ts_bb_edge_new_;
    /* [0...cluster_ctx.clb_nlist.nets().size()-1][0...num_layers-1] -> 2D bonding box on a layer*/
    vtr::vector<ClusterNetId, std::vector<t_2D_bb>> layer_ts_bb_coord_new_, layer_ts_bb_edge_new_;
    /* [0...cluster_ctx.clb_nlist.nets().size()-1][0...num_layers-1] -> number of sink pins on a layer*/
    vtr::Matrix<int> ts_layer_sink_pin_count_;
    /* [0...num_affected_nets] -> net_id of the affected nets */
    std::vector<ClusterNetId> ts_nets_to_update_;


    /**
     * @brief In each of these vectors, there is one entry per cluster level net:
     * [0...cluster_ctx.clb_nlist.nets().size()-1].
     * net_cost and proposed_net_cost: Cost of a net, and a temporary cost of a net used during move assessment.
     * We also use negative cost values in proposed_net_cost as a flag to indicate that
     * the cost of a net has not yet been updated.
     * bb_update_status: Flag array to indicate whether the specific bounding box has been updated
     * in this particular swap or not. If it has been updated before, the code
     * must use the updated data, instead of the out-of-date data passed into the
     * subroutine, particularly used in try_swap(). The value NOT_UPDATED_YET
     * indicates that the net has not been updated before, UPDATED_ONCE indicated
     * that the net has been updated once, if it is going to be updated again, the
     * values from the previous update must be used. GOT_FROM_SCRATCH is only
     * applicable for nets larger than SMALL_NETS and it indicates that the
     * particular bounding box is not incrementally updated, and hence the
     * bounding box is got from scratch, so the bounding box would definitely be
     * right, DO NOT update again.
     */
    vtr::vector<ClusterNetId, double> net_cost_;
    vtr::vector<ClusterNetId, double> proposed_net_cost_;
    vtr::vector<ClusterNetId, NetUpdateState> bb_update_status_;

    /**
     * @brief Matrices below are used to precompute the inverse of the average
     * number of tracks per channel between [subhigh] and [sublow].  Access
     * them as chan?_place_cost_fac(subhigh, sublow).  They are used to
     * speed up the computation of the cost function that takes the length
     * of the net bounding box in each dimension, divided by the average
     * number of tracks in that direction; for other cost functions they
     * will never be used.
     */
    vtr::NdOffsetMatrix<float, 2> chanx_place_cost_fac_; // [-1...device_ctx.grid.width()-1]
    vtr::NdOffsetMatrix<float, 2> chany_place_cost_fac_; // [-1...device_ctx.grid.height()-1]


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
    * @param delay_model Timing delay model used by placer
    * @param criticalities Connections timing criticalities
    * @param pin_id Pin ID of the moving pin
    * @param moving_blk_inf Data structure that holds information, e.g., old location and new location, about all moving blocks
    * @param affected_pins Netlist pins which are affected, in terms placement cost, by the proposed move.
    * @param timing_delta_c Timing cost change based on the proposed move
    * @param is_src_moving Is the moving pin the source of a net.
    */
    void update_net_info_on_pin_move_(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities* criticalities,
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

    /**
     * @brief Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac arrays with the inverse of
     * the average number of tracks per channel between [subhigh] and [sublow].
     *
     * @details This is only useful for the cost function that takes the length of the net bounding box in each
     * dimension divided by the average number of tracks in that direction. For other cost functions, you don't
     * have to bother calling this routine; when using the cost function described above, however, you must always
     * call this routine before you do any placement cost determination. The place_cost_exp factor specifies to
     * what power the width of the channel should be taken -- larger numbers make narrower channels more expensive.
     *
     * @param place_cost_exp It is an exponent to which you take the average inverse channel capacity;
     * a higher value would favour wider channels more over narrower channels during placement (usually we use 1).
     */
    void alloc_and_load_chan_w_factors_for_place_cost_(float place_cost_exp);

    /**
     * @brief Calculate the new connection delay and timing cost of all the
     * sink pins affected by moving a specific pin to a new location. Also
     * calculates the total change in the timing cost.
     * @param delay_model
     * @param criticalities
     * @param net
     * @param pin
     * @param affected_pins Updated by this routine to store the sink pins whose delays are changed due to moving the block
     * @param delta_timing_cost Computed by this routine and returned by reference.
     * @param is_src_moving True if "pin" is a sink pin and its driver is among the moving blocks
     */
    void update_td_delta_costs_(const PlaceDelayModel* delay_model,
                                const PlacerCriticalities& criticalities,
                                const ClusterNetId net,
                                const ClusterPinId pin,
                                std::vector<ClusterPinId>& affected_pins,
                                double& delta_timing_cost,
                                bool is_src_moving);

    /**
     * @brief Updates the bounding box coordinates of a net in the placer state
     * with coordinates stored in the `ts` bounding box coordinates container.
     * @param net_id ID of the net whose bounding box coordinates it to be updated.
     */
    void set_ts_bb_coord_(const ClusterNetId net_id);

    /**
     * @brief Updates the number of pins on each boundary of the bounding box for a net
     * in the placer state with the number of pins stores in the 'ts' num_on_edges containers.
     * @param net_id ID of the net whose number of pins on each BB edge is to be updated.
     */
    void set_ts_edge_(const ClusterNetId net_id);

    /**
     * @brief Calculate the 3D bounding box of "net_id" from scratch (based on the block locations
     * stored in placer_state_.blk_loc_registry) and store them in bb_coord_new
     * @param net_id ID of the net for which the bounding box is requested
     * @param use_ts Specifies whether the `ts` bounding box is updated or
     * the one stored in placer_state_
     */
    void get_non_updatable_cube_bb_(ClusterNetId net_id, bool use_ts);

    /**
     * @brief Calculate the per-layer bounding box of "net_id" from scratch (based on the block locations stored in place_ctx) and
     * store them in bb_coord_new
     * @param net_id ID of the net for which the bounding box is requested
     * @param use_ts Specifies whether the `ts` bounding box is updated or
     * the one stored in placer_state_
     */
    void get_non_updatable_per_layer_bb_(ClusterNetId net_id, bool use_ts);

    /**
     * @brief Calculate the 3D BB of a large net from scratch and update coord, edge, and num_sink_pin_layer data structures.
     * @details This routine finds the bounding box of each net from scratch (i.e. from only the block location information).
     * It updates both the coordinate and number of pins on each edge information. It should only be called when the bounding box
     * information is not valid.
     * @param net_id ID of the net which the moving pin belongs to
     * @param coords Bounding box coordinates of the net. It is calculated in this function
     * @param num_on_edges Net's number of blocks on the edges of the bounding box. It is calculated in this function.
     * @param num_sink_pin_layer Net's number of sinks on each layer, calculated in this function.
     */
    void get_bb_from_scratch_(ClusterNetId net_id,
                              t_bb& coords,
                              t_bb& num_on_edges,
                              vtr::NdMatrixProxy<int, 1> num_sink_pin_layer);

    /**
     * @brief Calculate the per-layer BB of a large net from scratch and update coord, edge, and num_sink_pin_layer data structures.
     * @details This routine finds the bounding box of each net from scratch when the bounding box is of type per-layer (i.e. from
     * only the block location information). It updates the coordinate, number of pins on each edge information, and the
     * number of sinks on each layer. It should only be called when the bounding box information is not valid.
     * @param net_id ID of the net which the moving pin belongs to
     * @param coords Bounding box coordinates of the net. It is calculated in this function
     * @param num_on_edges Net's number of blocks on the edges of the bounding box. It is calculated in this function.
     * @param num_sink_pin_layer Net's number of sinks on each layer, calculated in this function.
     */
    void get_layer_bb_from_scratch_(ClusterNetId net_id,
                                    std::vector<t_2D_bb>& num_on_edges,
                                    std::vector<t_2D_bb>& coords,
                                    vtr::NdMatrixProxy<int, 1> layer_pin_sink_count);

    /**
     * @brief Update the 3D bounding box of "net_id" incrementally based on the old and new locations of a pin on that net
     * @details Updates the bounding box of a net by storing its coordinates in the bb_coord_new data structure and the
     * number of blocks on each edge in the bb_edge_new data structure. This routine should only be called for large nets,
     * since it has some overhead relative to just doing a brute force bounding box calculation. The bounding box coordinate
     * and edge information for inet must be valid before this routine is called. Currently assumes channels on both sides of
     * the CLBs forming the edges of the bounding box can be used.  Essentially, I am assuming the pins always lie on the
     * outside of the bounding box. The x and y coordinates are the pin's x and y coordinates. IO blocks are considered to be one
     * cell in for simplicity.
     * @param net_id ID of the net which the moving pin belongs to
     * @param pin_old_loc The old location of the moving pin
     * @param pin_new_loc The new location of the moving pin
     * @param src_pin Is the moving pin driving the net
     */
    void update_bb_(ClusterNetId net_id,
                    t_physical_tile_loc pin_old_loc,
                    t_physical_tile_loc pin_new_loc,
                    bool src_pin);

    /**
     * @brief Update the per-layer bounding box of "net_id" incrementally based on the old and new locations of a pin on that net
     * @details Updates the bounding box of a net by storing its coordinates in the bb_coord_new data structure and
     * the number of blocks on each edge in the bb_edge_new data structure. This routine should only  be called for
     * large nets, since it has some overhead relative to just doing a brute force bounding box calculation.
     * The bounding box coordinate and edge information for inet must be valid before  this routine is called.
     * Currently assumes channels on both sides of the CLBs forming the   edges of the bounding box can be used.
     * Essentially, I am assuming the pins always lie on the outside of the bounding box. The x and y coordinates
     * are the pin's x and y coordinates. IO blocks are considered to be one cell in for simplicity.
     * @param bb_edge_new Number of blocks on the edges of the bounding box
     * @param bb_coord_new Coordinates of the bounding box
     * @param num_sink_pin_layer_new Number of sinks of the given net on each layer
     * @param pin_old_loc The old location of the moving pin
     * @param pin_new_loc The new location of the moving pin
     * @param is_output_pin Is the moving pin of the type output
     */
    void update_layer_bb_(ClusterNetId net_id,
                          t_physical_tile_loc pin_old_loc,
                          t_physical_tile_loc pin_new_loc,
                          bool is_output_pin);

    /**
     * @brief Update the data structure for large nets that keep track of
     * the number of blocks on each edge of the bounding box. If the moving block
     * is the only block on one of the edges, the bounding box is calculated from scratch.
     * Since this function is used for large nets, it updates the bounding box incrementally.
     * @param net_id ID of the net which the moving pin belongs to
     * @param bb_edge_new The new bb edge calculated by this function
     * @param bb_coord_new The new bb calculated by this function
     * @param bb_layer_pin_sink_count The updated number of net's sinks on each layer
     * @param old_num_block_on_edge The current known number of blocks of the net on bounding box edges
     * @param old_edge_coord The current known bounding box of the net
     * @param new_num_block_on_edge The new bb calculated by this function
     * @param new_edge_coord The new bb edge calculated by this function
     */
    inline void update_bb_edge_(ClusterNetId net_id,
                                std::vector<t_2D_bb>& bb_edge_new,
                                std::vector<t_2D_bb>& bb_coord_new,
                                vtr::NdMatrixProxy<int, 1> bb_layer_pin_sink_count,
                                const int& old_num_block_on_edge,
                                const int& old_edge_coord,
                                int& new_num_block_on_edge,
                                int& new_edge_coord);

    /**
    * @brief This function is called in update_layer_bb to update the net's bounding box incrementally if
    * the pin under consideration change layer.
     * @param net_id ID of the net which the moving pin belongs to
     * @param pin_old_loc Old location of the moving pin
     * @param pin_new_loc New location of the moving pin
     * @param curr_bb_edge The current known number of blocks of the net on bounding box edges
     * @param curr_bb_coord The current known bounding box of the net
     * @param bb_pin_sink_count_new The updated number of net's sinks on each layer
     * @param bb_edge_new The new bb edge calculated by this function
     * @param bb_coord_new The new bb calculated by this function
     */
    inline void update_bb_layer_changed_(ClusterNetId net_id,
                                         const t_physical_tile_loc& pin_old_loc,
                                         const t_physical_tile_loc& pin_new_loc,
                                         const std::vector<t_2D_bb>& curr_bb_edge,
                                         const std::vector<t_2D_bb>& curr_bb_coord,
                                         vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                         std::vector<t_2D_bb>& bb_edge_new,
                                         std::vector<t_2D_bb>& bb_coord_new);

    /**
     * @brief This function is called in update_layer_bb to update the net's bounding box incrementally if
     * the pin under consideration is not changing layer.
     * @param net_id ID of the net which the moving pin belongs to
     * @param pin_old_loc Old location of the moving pin
     * @param pin_new_loc New location of the moving pin
     * @param curr_bb_edge The current known number of blocks of the net on bounding box edges
     * @param curr_bb_coord The current known bounding box of the net
     * @param bb_pin_sink_count_new The updated number of net's sinks on each layer
     * @param bb_edge_new The new bb edge calculated by this function
     * @param bb_coord_new The new bb calculated by this function
     */
     inline void update_bb_same_layer_(ClusterNetId net_id,
                                             const t_physical_tile_loc& pin_old_loc,
                                             const t_physical_tile_loc& pin_new_loc,
                                             const std::vector<t_2D_bb>& curr_bb_edge,
                                             const std::vector<t_2D_bb>& curr_bb_coord,
                                             vtr::NdMatrixProxy<int, 1> bb_pin_sink_count_new,
                                             std::vector<t_2D_bb>& bb_edge_new,
                                             std::vector<t_2D_bb>& bb_coord_new);

     /**
      * @brief Computes the bounding box from scratch using 2D bounding boxes (per-layer mode)
      * @param method The method used to calculate placement cost. Specifies whether the cost is
      * computed from scratch or incrementally.
      * @return Computed bounding box cost.
      */
     double comp_per_layer_bb_cost_(e_cost_methods method);

     /**
      * @brief Computes the bounding box from scratch using 3D bounding boxes (cube mode)
      * @param method The method used to calculate placement cost. Specifies whether the cost is
      * computed from scratch or incrementally.
      * @return Computed bounding box cost.
      */
     double comp_cube_bb_cost_(e_cost_methods method);

     /**
      * @brief if "net" is not already stored as an affected net, add it in ts_nets_to_update.
      * @param net ID of a net affected by a move
      */
     void record_affected_net_(const ClusterNetId net);

     /**
      * @brief To mitigate round-off errors, every once in a while, the costs of nets are summed up from scratch.
      * This functions is called to do that for bb cost. It doesn't calculate the BBs from scratch, it would only add the costs again.
      * @return Total bb (wirelength) cost for the placement
      */
     double recompute_bb_cost_();

     /**
      * @brief Given the 3D BB, calculate the wire-length cost of the net
      * @param net_id ID of the net which cost is requested.
      * @param use_ts Specifies if the bounding box is retrieved from ts data structures
      * or move context.
      * @return Wirelength cost of the net
      */
     double get_net_cube_bb_cost_(ClusterNetId net_id, bool use_ts);

     /**
      * @brief Given the per-layer BB, calculate the wire-length cost of the net on each layer
      * and return the sum of the costs
      * @param net_id ID of the net which cost is requested. Currently unused
      * @param use_ts Specifies whether the 'ts` bounding box is used to compute the
      * cost or the one stored in placer_state_
      * @return Wirelength cost of the net
      */
     double get_net_per_layer_bb_cost_(ClusterNetId net_id, bool use_ts);

    /**
     * @brief Given the per-layer BB, calculate the wire-length estimate of the net on each layer
     * and return the sum of the lengths
     * @param net_id ID of the net which wirelength estimate is requested
     * @return Wirelength estimate of the net
     */
    double get_net_wirelength_from_layer_bb_(ClusterNetId net_id);

};
