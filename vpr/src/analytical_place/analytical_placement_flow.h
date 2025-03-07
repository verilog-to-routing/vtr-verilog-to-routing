/**
 * @file
 * @author  Alex Singer
 * @date    September 2024
 * @brief   Methods for running the Analytical Placement flow.
 */

#pragma once

// Forward declarations
struct t_vpr_setup;

/**
 * @brief Run the Analaytical Placement flow.
 *
 *  @param vpr_setup    The setup options provided by the user.
 */
void run_analytical_placement_flow(t_vpr_setup& vpr_setup);

