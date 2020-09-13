/**
 * @file placer_context.h
 * @brief Contains placer context/data structures referenced by various
 *        source files in vpr/src/place.
 *
 * All the variables and data structures in this file can be accessed via
 * a single global variable: g_placer_ctx. (see placer_globals.h/.cpp).
 */

#pragma once
#include "vpr_context.h"
#include "vpr_net_pins_matrix.h"
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
    /**
     * @brief Net connection delays based on the committed block positions.
     *
     * Index ranges: [0..cluster_ctx.clb_nlist.nets().size()-1][1..num_pins-1]
     */
    ClbNetPinsMatrix<float> connection_delay;

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
     *
     *
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
class PlacerContext : public Context {
  public:
    const PlacerTimingContext& timing() const { return timing_; }
    PlacerTimingContext& mutable_timing() { return timing_; }

    const PlacerRuntimeContext& runtime() const { return runtime_; }
    PlacerRuntimeContext& mutable_runtime() { return runtime_; }

  private:
    PlacerTimingContext timing_;
    PlacerRuntimeContext runtime_;
};
