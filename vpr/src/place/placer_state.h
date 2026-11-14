#pragma once
/**
 * @file placer_state.h
 * @brief Contains placer state/data structures referenced by various source files in vpr/src/place.
 * A PlacerState object contains the placement state which is subject to change during the placement stage.
 * During the placement stage, one or multiple local PlacerState objects are created. At the end of the placement stage,
 * one of these object is copied to global placement context (PlacementContext). The PlacementContext,
 * which is declared in vpr_context.h, contains the placement solution. The PlacementContext should not be used before
 * the end of the placement stage.
 */

#include "vpr_context.h"
#include "vpr_net_pins_matrix.h"
#include "vpr_types.h"
#include "PlacerTimingCosts.h"

/**
 * @brief The proposed delay and timing cost of one connection affected by a move.
 *
 * While a move is in flight, PlacerTimingContext::proposed_connection_timing holds one entry
 * per affected connection and commit_td_cost() writes them into the committed values.
 * A commit record is a copy of that vector. It is captured with
 * extract_connection_commit_record() on the state that evaluated the move and
 * replayed on another state with apply_connection_commit_record().
 */
struct t_connection_commit_entry {
    /// The net of the connection
    ClusterNetId net_id;
    /// The sink pin index of the connection within its net
    int ipin;
    /// Delay of the connection at the proposed block locations
    float connection_delay;
    /// Criticality times delay of the connection at the proposed block locations
    double connection_timing_cost;
};

/**
 * @brief State relating to the timing driven data.
 *
 * These structures are used when the placer is using a timing driven
 * algorithm (placer_opts->place_algorithm.is_timing_driven() == true).
 *
 * Due to the nature of the type of connection_timing_cost, one must
 * use mutable_timing() to access it. For more, see PlacerTimingCosts.
 */
struct PlacerTimingContext : public Context {
    PlacerTimingContext() = delete;

    /**
     * @brief Allocate structures associated with timing driven placement
     * @param placement_is_timing_driven Specifies whether the placement is timing driven.
     * @param placement_uses_setup_slacks Specifies whether connection_setup_slack is allocated.
     */
    PlacerTimingContext(bool placement_is_timing_driven, bool placement_uses_setup_slacks);

    /// @brief Writes every entry of proposed_connection_timing into connection_delay and
    /// connection_timing_cost, then clears proposed_connection_timing.
    void commit_td_cost();

    /// @brief Discards proposed_connection_timing after a rejected move.
    void revert_td_cost();

    /// @brief Records the values commit_td_cost() would commit for the move in flight.
    /// @note Must be called before the move is committed or reverted on this state.
    void extract_connection_commit_record(std::vector<t_connection_commit_entry>& record) const;

    /**
     * @brief Writes an extracted commit record into the committed connection delays and timing costs.
     * @note This state must have no move in flight.
     */
    void apply_connection_commit_record(const std::vector<t_connection_commit_entry>& record);

    /**
     * @brief Net connection delays based on the committed block positions.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    NetPinsMatrix<float> connection_delay;

    /// @brief Proposed delay and timing cost of every connection whose delay
    /// changes under the move in flight.
    /// Same order as t_pl_blocks_to_be_moved::affected_pins. Empty when no move is in flight.
    std::vector<t_connection_commit_entry> proposed_connection_timing;

    /**
     * @brief Net connection setup slacks based on most recently updated timing graph.
     *
     * Updated with commit_setup_slacks() routine. Empty unless the placer uses setup slacks.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> connection_setup_slack;

    /**
     * @brief Net connection timing costs (i.e. criticality * delay)
     *        of committed block positions. See PlacerTimingCosts.
     */
    PlacerTimingCosts connection_timing_cost;

    /**
     * @brief Timing cost of nets (i.e. sum of criticality * delay for each net sink/connection).
     *
     * Like connection_timing_cost, but summed across net pins. Used to allow more
     * efficient recalculation of timing cost if only a sub-set of nets are changed
     * while maintaining numeric stability.
     *
     * Index range: [0..cluster_ctx.clb_nlist.nets().size()-1]
     */
    vtr::vector<ClusterNetId, double> net_timing_cost;

    static constexpr float INVALID_DELAY = std::numeric_limits<float>::quiet_NaN();
};

/**
 * @brief State relating to various runtime statistics.
 */
struct PlacerRuntimeContext : public Context {
    float f_update_td_costs_connections_elapsed_sec;
    float f_update_td_costs_nets_elapsed_sec;
    float f_update_td_costs_sum_nets_elapsed_sec;
    float f_update_td_costs_total_elapsed_sec;
};

/**
 * @brief This object encapsulates VPR placer's state.
 *
 * It is divided up into separate sub-contexts of logically related
 * data structures.
 *
 * Typical usage in the VPR placer would be to call the appropriate
 * accessor to get a reference to the context of interest, and then
 * operate on it.
 *
 * See the class VprContext in `vpr_context.h` for descriptions on
 * how to use this class due to similar implementation style.
 */
class PlacerState : public Context {
  public:
    PlacerState(bool placement_is_timing_driven, bool placement_uses_setup_slacks);

  public:
    inline const PlacerTimingContext& timing() const { return timing_; }
    inline PlacerTimingContext& mutable_timing() { return timing_; }

    inline const PlacerRuntimeContext& runtime() const { return runtime_; }
    inline PlacerRuntimeContext& mutable_runtime() { return runtime_; }

    inline const vtr::vector_map<ClusterBlockId, t_block_loc>& block_locs() const { return blk_loc_registry_.block_locs(); }
    inline vtr::vector_map<ClusterBlockId, t_block_loc>& mutable_block_locs() { return blk_loc_registry_.mutable_block_locs(); }

    inline const GridBlock& grid_blocks() const { return blk_loc_registry_.grid_blocks(); }
    inline GridBlock& mutable_grid_blocks() { return blk_loc_registry_.mutable_grid_blocks(); }

    inline const vtr::vector_map<ClusterPinId, int>& physical_pins() const { return blk_loc_registry_.physical_pins(); }
    inline vtr::vector_map<ClusterPinId, int>& mutable_physical_pins() { return blk_loc_registry_.mutable_physical_pins(); }

    inline const BlkLocRegistry& blk_loc_registry() const { return blk_loc_registry_; }
    inline BlkLocRegistry& mutable_blk_loc_registry() { return blk_loc_registry_; }

  private:
    PlacerTimingContext timing_;
    PlacerRuntimeContext runtime_;

    /**
     * @brief Contains: 1) The location where each clustered block is placed at.
     *                  2) Which clustered blocks are located at a given location
     *                  3) The mapping between the clustered block pins and physical tile pins.
     */
    BlkLocRegistry blk_loc_registry_;
};
