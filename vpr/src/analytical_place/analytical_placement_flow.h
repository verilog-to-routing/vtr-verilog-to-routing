/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Methods for running the Analytical Placement flow.
 */

#pragma once

// Forward declarations
struct t_vpr_setup;
struct PartialPlacement;
class FlatPlacementInfo;
class APNetlist;

/**
 * @brief Passes the flat placement information to a provided partial placement.
 *
 *  @param flat_placement_info    The flat placement information to be read.
 *  @param ap_netlist             The APNetlist that used to iterate over its blocks.
 *  @param p_placement            The partial placement to be updated which is assumend
 * to be generated on ap_netlist or have same blocks.
 */
void convert_flat_to_partial_placement(const FlatPlacementInfo& flat_placement_info, const APNetlist& ap_netlist, PartialPlacement& p_placement);

/**
 * @brief Run the Analaytical Placement flow.
 *
 *  @param vpr_setup    The setup options provided by the user.
 */
void run_analytical_placement_flow(t_vpr_setup& vpr_setup);

