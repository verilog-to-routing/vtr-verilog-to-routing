#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2026
 * @brief   Methods for estimating the quality of a global placement solution.
 */

struct PartialPlacement;
class APNetlist;
class DeviceGrid;

/**
 * @brief Estimate the post-routing wire usage of the given flat placement.
 *
 * The estimate is computed on the flat (pre-clustering) placement, so nets
 * which are expected to be absorbed into a single tile by clustering do not
 * contribute. Each remaining net contributes its tile-level bounding box
 * half-perimeter, weighted by the placer's crossing count for the estimated
 * number of tiles that the net connects.
 *
 *  @param p_placement  The flat placement to estimate the wire usage of.
 *  @param netlist      The AP netlist that the placement is over.
 *  @param device_grid  The device grid that the placement is over.
 *
 *  @return The estimated wire usage, in units of tile-length wire segments
 *          weighted by crossing count. This is a relative quality metric and
 *          should not be compared directly to routed wirelength.
 */
double estimate_post_routing_wire_usage(const PartialPlacement& p_placement,
                                        const APNetlist& netlist,
                                        const DeviceGrid& device_grid);
