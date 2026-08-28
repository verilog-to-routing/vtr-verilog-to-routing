#pragma once
/**
 * @file net_cost_handler.h
 * @brief This file contains the declaration of NetCostHandler class used to update placement cost when a new move is proposed/committed.
 * For more details on the overall algorithm, refer to the comment at the top of the net_cost_handler.cpp
 */

#include "place_delay_model.h"
#include "move_transactions.h"
#include "place_util.h"
#include "vtr_prefix_sum.h"

#include <cstdint>
#include <limits>
#include <utility>

class PlacerState;
class PlacerCriticalities;

/**
 * @brief To get the wirelength cost/est, BB perimeter is multiplied by a factor to approximately correct for the half-perimeter
 * bounding box wirelength's underestimate of wiring for nets with fanout greater than 2.
 * @return Multiplicative wirelength correction factor
 */
double wirelength_crossing_count(size_t fanout);

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

/// @brief Per-net placement cost components (wirelength/BB, interposer, and congestion terms).
struct t_net_cost_terms {
    double bb_cost = 0.;
    double interposer_cost = 0.;
    double interposer_cong_cost = 0.;
    double cong_cost = 0.;
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
     * their wirelength costs.
     *
     * @param placer_state Contains information about block locations and net bounding boxes.
     * @param place_algorithm The placement algorithm in use (e.g. bounding-box only vs timing-driven).
     * @param congestion_chan_util_threshold Floor on estimated average routing-channel utilization within a net's bounding
     *                                       box for the routing congestion term (`cong_cost`): for each of the horizontal and
     *                                       vertical channel directions, only utilization above this adds to that net's congestion
     *                                       penalty.
     */
    NetCostHandler(PlacerState& placer_state,
                   t_place_algorithm place_algorithm,
                   double congestion_chan_util_threshold);

    /**
     * @brief Finds the bb cost and congestion cost from scratch.
     * @details Done only when the placement has been radically changed
     * (i.e. after initial placement). Otherwise, find the cost
     * change incrementally. If method check is NORMAL, we find
     * bounding boxes that are updatable for the larger nets.
     * If method is CHECK, all bounding boxes are found via the
     * non_updateable_bb routine, to provide a cost which can be
     * used to check the correctness of the other routine.
     * @param method The method used to calculate placement cost.
     * @return (cost terms, estimated wirelength)
     *
     * @note Cost terms are: bb_cost, interposer_cost, interposer_cong_cost, cong_cost.
     * @note The returned estimated wirelength is valid only when method == CHECK
     */
    std::pair<t_net_cost_terms, double> comp_bb_cong_cost(e_cost_methods method);

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
                                             t_net_cost_terms& cost_terms_delta,
                                             double& timing_delta_c);

    /**
     * @brief Reset the net cost function flags (proposed_net_cost and bb_updated_before)
     */
    void reset_move_nets();

    /**
     * @brief Update net cost data structures (in placer context and net_cost in .cpp file)
     * and reset flags (proposed_net_cost and bb_updated_before).
     * It is used to determine the index up to which elements in ts_nets_to_update are valid.
     */
    void update_move_nets();

    /**
     * @brief Re-calculates different terms of the cost function (wire-length, timing, NoC)
     * and update "costs" accordingly. It is important to note that in this function bounding box
     * and connection delays are not calculated from scratch. However, it iterates over all nets
     * and connections and updates their costs by a complete summation, rather than incrementally.
     * @param delay_model Placement delay model. Used to compute timing cost.
     * @param criticalities Contains the clustered netlist connection criticalities.
     * Used to computed timing cost .
     * @param costs passed by reference and computed by this routine (i.e. returned by reference)
     */
    void recompute_costs_from_scratch(const PlaceDelayModel* delay_model,
                                      const PlacerCriticalities* criticalities,
                                      t_placer_costs& costs);

    ///@brief Get the total wirelength estimate of all nets.
    double get_total_wirelength_estimate() const;

    /// @brief Returns the number of nets whose bounding box spans more than one layer.
    int get_num_nets_spanning_multiple_layers() const;

    ///@brief Get the nets affected by the most recent proposed move.
    /// Valid only after find_affected_nets_and_update_costs() has been called for the
    /// current proposed move (e.g. within try_swap()).
    const std::vector<ClusterNetId>& affected_nets() const;

    /**
     * @brief Estimates routing channel utilization and computes the congestion cost
     * for each net.
     * @param compute_congestion_cost Indicates whether computing congestion cost is needed.
     *
     * For each net, distributes estimated wirelength across its bounding box
     * and accumulates demand for different routing channels. Normalizes by channel widths
     * (e.g. a value of 0.5 means 50% of the wiring in a channel is expected to be used).
     *
     * @note This method assumes that net bounding boxes are already computed.
     *
     * @return Total congestion cost if requested.
     */
    double estimate_routing_chan_util(bool compute_congestion_cost = true);

    /**
     * @brief Returns the estimated routing channel usage for each location in the grid.
     *        The channel usage estimates are computed in estimate_routing_chan_util().
     */
    const ChannelMetric<vtr::NdMatrix<double, 3>>& get_chan_util() const;

  private:
    /// Indicates whether congestion cost modeling is enabled.
    bool congestion_modeling_started_;
    /// Determines whether the FPGA has multiple dies (layers)
    bool is_multi_layer_;
    /// A reference to the placer's state to be updated by this object.
    PlacerState& placer_state_;

    /// Contains some parameters that determine how the placement cost is computed.
    t_place_algorithm place_algorithm_;
    double congestion_chan_util_threshold_;

    /**
     * @brief for the states of the bounding box.
     */
    enum class NetUpdateState {
        NOT_UPDATED_YET,
        UPDATED_ONCE,
        GOT_FROM_SCRATCH
    };

    /**
     * @brief Committed bounding box state of one net.
     *
     * The coordinates, edge counts and cost of a net are packed together so that visiting
     * a net during a move touches one contiguous region of memory rather than several
     * separate num_nets-sized arrays.
     */
    struct t_net_bb_info {
        /// Bounding box coordinates. For 2D architectures layer_min == layer_max == 0.
        t_bb coords;
        /// Number of blocks on each edge of the bounding box.
        /// Only maintained for nets with at least SMALL_NET sinks.
        t_bb num_on_edges;
        /// Wirelength (bounding box) cost of the net. Negative means not computed yet.
        double cost = -1.;
    };

    /**
     * @brief Proposed bounding box state of one net affected by the move under evaluation.
     *
     * These entries live in a dense scratch array indexed by the position of the net in
     * ts_nets_to_update_ (its slot). If the move is accepted, the proposed state is copied
     * into net_bb_ and net_cong_.
     */
    struct t_ts_net_info {
        /// Proposed bounding box coordinates
        t_bb coords;
        /// Proposed number of blocks on each edge of the bounding box
        t_bb num_on_edges;
        /// Proposed wirelength cost
        double proposed_cost = 0.;
        /// Proposed congestion cost. Only valid when congestion modeling is enabled.
        double proposed_cong_cost = 0.;
        /// Proposed average CHANX and CHANY utilization within the bounding box.
        /// Only valid when congestion modeling is enabled.
        std::pair<float, float> avg_chan_util = {0.f, 0.f};
        /// How far the bounding box of this net has been updated for the current move.
        /// NOT_UPDATED_YET: the committed bounding box is still the reference.
        /// UPDATED_ONCE: the proposed bounding box has been updated incrementally at least once
        /// and must be used as the reference for later pins of the same net.
        /// GOT_FROM_SCRATCH: the proposed bounding box was recomputed from scratch and is final.
        NetUpdateState update_status = NetUpdateState::NOT_UPDATED_YET;
    };

    /**
     * @brief Committed congestion state of one net.
     *
     * The congestion cost of a net is based on the extent to which its average routing
     * channel utilization exceeds congestion_chan_util_threshold_. Only the excess portion
     * contributes to the cost.
     */
    struct t_net_cong_info {
        /// Average CHANX and CHANY utilization within the net's bounding box
        std::pair<float, float> avg_chan_util = {0.f, 0.f};
        /// Congestion cost of the net
        double cost = -1.;
    };

    /// Slot value meaning that a net is not affected by the current move.
    static constexpr uint32_t NO_TS_SLOT = std::numeric_limits<uint32_t>::max();

    /// Committed bounding box state of every net.
    /// [0..cluster_ctx.clb_nlist.nets().size()-1]
    vtr::vector<ClusterNetId, t_net_bb_info> net_bb_;

    /// Committed congestion state of every net.
    /// Empty until congestion modeling is enabled by estimate_routing_chan_util().
    vtr::vector<ClusterNetId, t_net_cong_info> net_cong_;

    /// Nets affected by the move under evaluation, in the order they were recorded.
    /// The position of a net in this vector is its slot in ts_net_info_.
    std::vector<ClusterNetId> ts_nets_to_update_;

    /// Proposed state of the affected nets, indexed by slot.
    /// Only the first ts_nets_to_update_.size() entries are valid.
    std::vector<t_ts_net_info> ts_net_info_;

    /// Slot of each net in ts_net_info_, or NO_TS_SLOT if the net is not affected by the current move.
    /// [0..cluster_ctx.clb_nlist.nets().size()-1]
    vtr::vector<ClusterNetId, uint32_t> net_ts_slot_;

    /**
     * @brief Matrices below are used to precompute the inverse of the average
     * number of tracks per channel between [subhigh] and [sublow].  Access
     * them as chan?_place_cost_fac(subhigh, sublow).  They are used to
     * speed up the computation of the cost function that takes the length
     * of the net bounding box in each dimension, divided by the average
     * number of tracks in that direction; for other cost functions they
     * will never be used.
     */
    ChannelMetric<vtr::PrefixSum1D<int>> acc_chan_width_;

    /**
     * @brief Estimated routing usage per channel segment,
     *        indexed by [layer][x][y]. Values represent normalized wire demand
     *        contribution from all nets distributed over their bounding boxes.
     */
    ChannelMetric<vtr::NdMatrix<double, 3>> chan_util_;

    /**
     * @brief Accumulated (prefix sum) channel utilization in each direction (x/y),
     *        on the base layer. Enables fast computation of average utilization
     *        over a net’s bounding box during congestion cost estimation.
     */
    ChannelMetric<vtr::PrefixSum2D<double>> acc_chan_util_;

    /**
     * @brief The matrix below is used to calculate a chanz_place_cost_fac based on the average channel width in 
     * the cross-die-layer direction over a 2D (x,y) region. We don't assume the inter-die connectivity is the same at all (x,y) locations, so we
     * can't compute the full chanz_place_cost_fac for all possible (xlow,ylow)(xhigh,yhigh) without a 4D array, which would
     * be too big: O(n^2) in circuit size. Instead, we compute a prefix sum that stores the number of inter-die connections per layer from
     * (x=0,y=0) to (x,y). Given this, we can compute the average number of inter-die connections over a (xlow,ylow) to (xhigh,yhigh) 
     * region in O(1) (by adding and subtracting 4 entries)
     */
    vtr::PrefixSum2D<int> acc_tile_num_inter_die_conn_; // [0..grid_width-1][0..grid_height-1]

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
     * @brief Update the bounding box of the net connected to pin_id. Also,
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
     * @brief Accumulates the placement cost deltas for all nets affected by the proposed move.
     * @param cost_terms_delta Updated with the delta in bb (wirelength) cost, and (when enabled) the deltas in
     * congestion, interposer crossing, and interposer congestion costs.
     */
    void set_bb_delta_cost_(t_net_cost_terms& cost_terms_delta);

    /**
     * @brief Allocates and loads the chanx_place_cost_fac and chany_place_cost_fac arrays with the inverse of
     * the average number of tracks per channel between [subhigh] and [sublow].
     *
     * @details This is only useful for the cost function that takes the length of the net bounding box in each
     * dimension divided by the average number of tracks in that direction. For other cost functions, you don't
     * have to bother calling this routine; when using the cost function described above, however, you must always
     * call this routine before you do any placement cost determination.
     */
    void alloc_and_load_chan_w_factors_for_place_cost_();

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
     * @brief Returns the proposed state of a net that has already been recorded
     * as affected by the current move (see record_affected_net_()).
     * @param net_id ID of an affected net
     */
    t_ts_net_info& ts_info_(ClusterNetId net_id) {
        VTR_ASSERT_SAFE(net_ts_slot_[net_id] != NO_TS_SLOT);
        return ts_net_info_[net_ts_slot_[net_id]];
    }

    const t_ts_net_info& ts_info_(ClusterNetId net_id) const {
        VTR_ASSERT_SAFE(net_ts_slot_[net_id] != NO_TS_SLOT);
        return ts_net_info_[net_ts_slot_[net_id]];
    }

    /**
     * @brief Calculate the 3D bounding box of "net_id" from scratch (based on the block locations
     * stored in placer_state_.blk_loc_registry). Does not compute the number of blocks on each edge.
     * @param net_id ID of the net for which the bounding box is requested
     * @param use_ts Specifies whether the proposed (`ts`) bounding box is updated or the committed one.
     * When use_ts is true, the net must already have been recorded by record_affected_net_().
     */
    void get_non_updatable_bb_(ClusterNetId net_id, bool use_ts);

    /**
     * @brief Calculate the 3D BB of a large net from scratch and update its coordinates and number of blocks on each edge.
     * @details This routine finds the bounding box of each net from scratch (i.e. from only the block location information).
     * It updates both the coordinate and number of pins on each edge information. It should only be called when the bounding box
     * information is not valid.
     * @param net_id ID of the net which the moving pin belongs to
     * @param use_ts Specifies whether the proposed (`ts`) bounding box is updated or the committed one.
     * When use_ts is true, the net must already have been recorded by record_affected_net_().
     */
    void get_bb_from_scratch_(ClusterNetId net_id, bool use_ts);

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
     */
    void update_bb_(ClusterNetId net_id,
                    t_physical_tile_loc pin_old_loc,
                    t_physical_tile_loc pin_new_loc);

    /**
     * @brief If "net" is not already recorded as affected by the current move, append it to
     * ts_nets_to_update_ and assign it a slot in ts_net_info_.
     * @param net ID of a net affected by a move
     */
    void record_affected_net_(const ClusterNetId net);

    /**
     * @brief To mitigate round-off errors, every once in a while, the costs of nets are summed up from scratch.
     *        This function is called to do that for bb and congestion cost.
     *        It doesn't calculate the BBs or channel usage estimate from scratch,
     *        it would only add the costs again.
     * @return Total cost terms summed across all nets (bb cost, and any enabled congestion/interposer terms).
     */
    t_net_cost_terms recompute_bb_cong_cost_();

    /**
     * @brief Given the 3D BB, calculate the wire-length cost of the net
     * @param net_id ID of the net whose cost is requested. Used to look up the net's fanout.
     * @param bb The (committed or proposed) bounding box of the net.
     * @return Wirelength cost of the net
     */
    double get_net_bb_cost_(ClusterNetId net_id, const t_bb& bb) const;

    /**
     * @brief Calculate the congestion cost of a net from the average channel
     * utilization within its bounding box.
     * @param avg_chan_util Average CHANX and CHANY utilization within the net's bounding box.
     * @return Congestion cost of the net
     */
    double get_net_cong_cost_(const std::pair<float, float>& avg_chan_util) const;

    /**
     * @brief Computes the inverse of average channel width for horizontal and
     * vertical channels within a bounding box.
     * @param bb The bounding box for which the inverse of average channel width
     * within the bounding box is computed.
     * @return std::pair<double, double>
     *         first  -> The inverse of average channel width for horizontal channels.
     *         second -> The inverse of average channel width for vertical channels.
     */
    std::pair<double, double> get_chanxy_cost_fac_(const t_bb& bb) const;

    /**
     * @brief Calculate the chanz cost factor based on the inverse of the average number of inter-die connections 
     * in the given bounding box. This cost factor increases the placement cost for blocks that require inter-layer 
     * connections in areas with, on average, fewer inter-die connections. If inter-die connections are evenly 
     * distributed across tiles, the cost factor will be the same for all bounding boxes, but it will still 
     * weight z-directed vs. x- and y-directed connections appropriately.
     *
     * @param bb Bounding box of the net which chanz cost factor is to be calculated
     * @return ChanZ cost factor
     */
    float get_chanz_cost_factor_(const t_bb& bb) const;

    /**
     * @brief Given the 3D BB, calculate the wire-length estimate of the net
     * @param net_id ID of the net which wirelength estimate is requested
     * @return Wirelength estimate of the net
     */
    double get_net_wirelength_estimate_(ClusterNetId net_id) const;

    // Bounding-box getters
  public:
    inline const t_bb& bb_num_on_edges(ClusterNetId net_id) const { return net_bb_[net_id].num_on_edges; }

    inline const t_bb& bb_coords(ClusterNetId net_id) const { return net_bb_[net_id].coords; }

    /// @brief Returns the net's bounding box, either the proposed (`ts`) one or the committed one.
    /// The proposed one is only available for nets affected by the current move.
    inline const t_bb& bb_coords(ClusterNetId net_id, bool use_ts) const { return use_ts ? ts_info_(net_id).coords : net_bb_[net_id].coords; }
};
