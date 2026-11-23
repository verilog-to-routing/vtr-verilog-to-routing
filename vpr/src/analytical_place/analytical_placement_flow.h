#pragma once
/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Methods for running the Analytical Placement flow.
 */

// Forward declarations
struct t_vpr_setup;

/**
 * @brief Run the Analaytical Placement flow.
 *
 * If only global placement was requested (APOpts.stop_after_global_placement),
 * the flow stops right after global placement; no clustering or placement is
 * produced.
 *
 *  @param vpr_setup    The setup options provided by the user.
 */
void run_analytical_placement_flow(t_vpr_setup& vpr_setup);
