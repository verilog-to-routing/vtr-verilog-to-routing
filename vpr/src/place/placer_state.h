/**
 * @file placer_state.h
 * @brief Contains placer state/data structures referenced by various source files in vpr/src/place.
 * A PlacerState object contains the placement state which is subject to change during the placement stage.
 * During the placement stage, one or multiple local PlacerState objects are created. At the end of the placement stage,
 * one of these object is copied to global placement context (PlacementContext). The PlacementContext,
 * which is declared in vpr_context.h, contains the placement solution. The PlacementContext should not be used before
 * the end of the placement stage.
 */

#pragma once
#include "vpr_context.h"
#include "vpr_net_pins_matrix.h"
#include "vpr_types.h"
#include "timing_place.h"

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
     */
    PlacerTimingContext(bool placement_is_timing_driven);

    /**
     * @brief Update the connection_timing_cost values from the temporary
     *        values for all connections that have/haven't changed.
     *
     * All the connections have already been gathered by blocks_affected.affected_pins
     * after running the routine find_affected_nets_and_update_costs() in try_swap().
     */
    void commit_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

    /**
     * @brief Reverts modifications to proposed_connection_delay and proposed_connection_timing_cost
     * based on the move proposed in blocks_affected
     */
    void revert_td_cost(const t_pl_blocks_to_be_moved& blocks_affected);

    /**
     * @brief Net connection delays based on the committed block positions.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    NetPinsMatrix<float> connection_delay;

    /**
     * @brief Net connection delays based on the proposed block positions.
     *
     * Only contains meaningful values for connections affected by the proposed
     * move. Otherwise, INVALID_DELAY.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> proposed_connection_delay;

    /**
     * @brief Net connection setup slacks based on most recently updated timing graph.
     *
     * Updated with commit_setup_slacks() routine.
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
     * @brief Costs for proposed block positions. Only for connection
     *        affected by the proposed move. Otherwise, INVALID_DELAY.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<double> proposed_connection_timing_cost;

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
 * @brief Placement Move generators data
 */
struct PlacerMoveContext : public Context {
  public:
    PlacerMoveContext() = delete;
    explicit PlacerMoveContext(bool cube_bb);

  public:
    // [0..cluster_ctx.clb_nlist.nets().size()-1]. Store the number of blocks on each of a net's bounding box (to allow efficient updates)
    vtr::vector<ClusterNetId, t_bb> bb_num_on_edges;

    // [0..cluster_ctx.clb_nlist.nets().size()-1]. Store the bounding box coordinates of a net's bounding box
    vtr::vector<ClusterNetId, t_bb> bb_coords;

    // [0..cluster_ctx.clb_nlist.nets().size()-1]. Store the number of blocks on each of a net's bounding box (to allow efficient updates)
    vtr::vector<ClusterNetId, std::vector<t_2D_bb>> layer_bb_num_on_edges;

    // [0..cluster_ctx.clb_nlist.nets().size()-1]. Store the bounding box coordinates of a net's bounding box
    vtr::vector<ClusterNetId, std::vector<t_2D_bb>> layer_bb_coords;

    // [0..cluster_ctx.clb_nlist.nets().size()-1]. Store the number of blocks on each layer ()
    vtr::Matrix<int> num_sink_pin_layer;

    // The first range limit calculated by the annealer
    float first_rlim;

    // Scratch vectors that are used by different directed moves for temporary calculations
    // These vectors will grow up with the net size as it is mostly used to save coords of the net pins or net bb edges
    // Given that placement moves involve operations on each coordinate independently, we chose to 
    // utilize a Struct of Arrays (SoA) rather than an Array of Struct (AoS).
    std::vector<int> X_coord;
    std::vector<int> Y_coord;
    std::vector<int> layer_coord;

    // Container to save the highly critical pins (higher than a timing criticality limit set by commandline option)
    std::vector<std::pair<ClusterNetId, int>> highly_crit_pins;
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
    PlacerState(bool placement_is_timing_driven, bool cube_bb);

  public:
    inline const PlacerTimingContext& timing() const { return timing_; }
    inline PlacerTimingContext& mutable_timing() { return timing_; }

    inline const PlacerRuntimeContext& runtime() const { return runtime_; }
    inline PlacerRuntimeContext& mutable_runtime() { return runtime_; }

    inline const PlacerMoveContext& move() const { return move_; }
    inline PlacerMoveContext& mutable_move() { return move_; }

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
    PlacerMoveContext move_;

    /**
     * @brief Contains: 1) The location where each clustered block is placed at.
     *                  2) Which clustered blocks are located at a given location
     *                  3) The mapping between the clustered block pins and physical tile pins.
     */
    BlkLocRegistry blk_loc_registry_;
};
