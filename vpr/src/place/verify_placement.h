/**
 * @file
 * @author  Alex Singer
 * @date    October 2024
 * @brief   Independent verify methods to check invariants that ensure that the
 *          given placement is valid and can be used with the rest of the VPR
 *          flow.
 *
 * This methods were orginally part of methods found in the place.cpp file and
 * the place_constraints.cpp files. Moved into here to put it all in one place.
 * Since these methods should be completely independent of the place flow, it
 * makes sense that they are in their own file.
 */

#pragma once

#include "vtr_vector.h"

// Forward declarations
class BlkLocRegistry;
class ClusterBlockId;
class ClusteredNetlist;
class DeviceGrid;
class PartitionRegion;
class PlaceMacros;
class VprContext;

/**
 * @brief Verify the placement of the clustered blocks on the device.
 *
 * This is verifier is independent to how the placement was performed and check
 * invariants that all placements must adhere to in order to be used in the
 * rest of the VPR flow. The assumption is that if a placement passes this
 * verifier it can be passed into routing without issue.
 *
 * By design, this verifier uses no global variables and tries to recompute
 * anything that it does not assume.
 *
 * Invariants (are checked by this function):
 *  - All clusters are placed.
 *  - All clusters are placed in a tile, sub-tile that it can exist in.
 *  - The cluster locations are consistent with the clusters in each block.
 *  - The placement macros are consistent such that all blocks are in the
 *    proper relative positions.
 *  - All clusters are placed in legal positions according to the floorplanning
 *    constraints.
 *
 * Assumptions (are not checked by this function):
 *  - The clustered netlist is valid.
 *  - The device grid is valid.
 *  - The cluster constraints are valid.
 *  - The macros are valid.
 *
 *  @param blk_loc_registry     A registry containing the current placement of
 *                              the clusters.
 *  @param clb_nlist            The clustered netlist being verified.
 *  @param device_grid          The device grid being verified over.
 *  @param cluster_constraints  The constrained regions that each cluster is
 *                              constrained to.
 *
 *  @return The number of errors found in the placement. Will also print error
 *          log messages for each error found.
 */
unsigned verify_placement(const BlkLocRegistry& blk_loc_registry,
                          const PlaceMacros& place_macros,
                          const ClusteredNetlist& clb_nlist,
                          const DeviceGrid& device_grid,
                          const vtr::vector<ClusterBlockId, PartitionRegion>& cluster_constraints);

/**
 * @brief Verifies the placement as it appears in the global context.
 *
 * This performs the verification in the method above, but performs it on the
 * global VPR context itself. This verifies that the actual placement being
 * used through the rest of the flow is valid.
 *
 * NOTE: The above method is in-case the user wishes to verify a temporary
 *       placement before updating the actual global VPR context.
 *
 *  @param ctx  The global VPR context variable found in g_vpr_ctx.
 */
unsigned verify_placement(const VprContext& ctx);

