#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2026
 * @brief   Methods for estimating the quality of a global placement solution.
 */

#include <limits>
#include <memory>

struct PartialPlacement;
class APNetlist;
class DeviceGrid;
class FlatPlacementArcDelayEstimator;
class PreClusterDelayCalculator;

/**
 * @brief The estimated post-routing timing of a flat placement.
 */
struct t_ap_timing_estimate {
    /// @brief The estimated critical path delay in seconds.
    float cpd = std::numeric_limits<float>::quiet_NaN();
    /// @brief The estimated setup total negative slack in seconds.
    float stns = std::numeric_limits<float>::quiet_NaN();
    /// @brief The estimated setup worst negative slack in seconds.
    float swns = std::numeric_limits<float>::quiet_NaN();
};

/**
 * @brief Estimate the post-routing wire usage of the given flat placement.
 *
 * The estimate is computed on the flat (pre-clustering) placement, so nets
 * which are expected to be absorbed into a single tile by clustering do not
 * contribute. Each remaining net contributes its tile-level bounding box
 * half-perimeter, weighted by the placer's crossing count for the estimated
 * number of tiles that the net connects. An empirical correction factor is
 * then applied to account for the estimate consistently undershooting the
 * routed wirelength (mainly due to short nets using longer wire segments).
 *
 * If the wire usage estimate echo file is enabled, the estimate of every net
 * (and the intermediate values used to compute it) is written to it. This can
 * be compared net-by-net against the routed net wire usage echo file, joined
 * on the net name.
 *
 *  @param p_placement  The flat placement to estimate the wire usage of.
 *  @param netlist      The AP netlist that the placement is over.
 *  @param device_grid  The device grid that the placement is over.
 *
 *  @return The estimated wire usage, in units of tile-length wire segments.
 *          This is an approximation of the routed wirelength.
 */
double estimate_post_routing_wire_usage(const PartialPlacement& p_placement,
                                        const APNetlist& netlist,
                                        const DeviceGrid& device_grid);

/**
 * @brief Estimate the post-routing setup timing of the given flat placement.
 *
 * The delays of all timing arcs are re-estimated from the flat placement and
 * stored in the given delay calculator, then a full setup timing analysis is
 * performed with a new timing analyzer which uses that delay calculator.
 *
 * If the timing graph echo file for this estimate is enabled, the timing graph
 * (including the estimated delay of every edge) is written to it. This can be
 * compared edge-by-edge against the post-routing analysis timing graph echo
 * file, since both are over the same atom-level timing graph.
 *
 * NOTE: The arc delays in the given delay calculator are restored to their
 *       previous values before returning, so this does not affect any timing
 *       info which shares this delay calculator (for example, the one owned by
 *       the pre-cluster timing manager).
 *
 *  @param p_placement          The flat placement to estimate the timing of.
 *  @param arc_delay_estimator  The estimator used to compute arc delays.
 *  @param delay_calc           The delay calculator used for timing analysis.
 *
 *  @return The estimated post-routing timing.
 */
t_ap_timing_estimate estimate_post_routing_timing(const PartialPlacement& p_placement,
                                                  const FlatPlacementArcDelayEstimator& arc_delay_estimator,
                                                  std::shared_ptr<PreClusterDelayCalculator> delay_calc);
