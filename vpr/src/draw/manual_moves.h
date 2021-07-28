/*
 * @file 	manual_moves.h
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the function prototypes needed for manual moves feature.
 *
 * Includes the data structures sed and gtk function for manual moves. The Manual Move Generator class is defined  manual_move_generator.h/cpp.
 */

#ifndef MANUAL_MOVES_H
#define MANUAL_MOVES_H

/** This file contains all functions for manual moves **/
#ifndef NO_GRAPHICS

#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

#    include "move_utils.h"
#    include <cstdio>
#    include <cfloat>
#    include <cstring>
#    include <cmath>
#    include <algorithm>
#    include <iostream>

/**
 * @brief ManualMovesInfo struct
 *
 * Contains information about the block, location, validity of user input, timing variables, and placer outcomes.
 */

struct ManualMovesInfo {
    int blockID = -1;
    int x_pos = -1;
    int y_pos = -1;
    int subtile = 0;
    double delta_cost = 0;
    double delta_timing = 0;
    double delta_bounding_box = 0;
    bool valid_input = true;
    t_pl_loc to_location;
    e_move_result placer_move_outcome = ABORTED;
    e_move_result user_move_outcome = ABORTED;
};

/**
 * @brief ManualMovesGlobals struct
 *
 * Contains a ManualMovesInfo variable to store manual move data, boolean values to check the state of the windows needed and gtk manual move widow variable.
 */

struct ManualMovesGlobals {
    ManualMovesInfo manual_move_info;
    bool mm_window_is_open = false;
    bool user_highlighted_block = false;
    bool manual_move_flag = false;
    GtkWidget* manual_move_window;
};

/** manual moves functions **/

/**
 * @brief Gets the state of the manual moves togle button and assigns it to the manual_move_flag in the ManualMovesGlobal struct.
 *
 * @return True if the toggle button is active, false otherwise.
 */
bool get_manual_move_flag();

/**
 * @brief Draws the manual move window.
 *
 * Window prompts the user for input: block id/name, s position, y position, and subtile position.
 * @param block_id: The block id selected/highlighted by the user.
 */
void draw_manual_moves_window(std::string block_id);

/**
 * @brief Evaluates if the user input is valid and allowed.
 *
 * Sets the members from the ManualMovesGlobals global variable to their respective values (block id and locations).
 * @param GtkWidget* widget: Needed for gtk functions.
 * @param GtkWidget* grid: Needed to extract members from the manual move window.
 */
void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid);

/**
 * @brief In -detail checking of the user's input.
 *
 * Checks if the user input is between the grid's dimensions, block comptaibility, if the block requested to move is valid, if the block is fixed, and if the curent location of the block is different from the location requested by the user.
 * @param block_id: The ID of the block to move.
 * @param to: Location of where the user wants to move the block.
 *
 * @return True if all conditions are met, false otherwise.
 */
bool checking_legality_conditions(ClusterBlockId block_id, t_pl_loc to);

/**
 * @brief Draws the cost summary dialog.
 *
 * Window displays the delta cost, delta timing, delta bounding box timing, annealing decision to the user. Waits for the user to ACCEPT/REJECT the manual move.
 */
void cost_summary_dialog();

/**
 * @brief Highlights new block location
 *
 * Highlights block in the new location if the manual move flag is active and the user accepted the manual move.
 */
void highlight_new_block_location();

/**
 * @brief Disables the mm_window_is_open boolean and destroys the window
 */
void close_manual_moves_window();

/**
 * @brief Gets the manual_moves_global variables in manual_move.cpp.
 *
 * @return The manual_moves_global variable.
 */
ManualMovesGlobals* get_manual_moves_global();

/**
 * @brief Updates the ManualMovesGlobals global veriable members.
 *
 * @param d_cost: Delta cost for cost summary dialog function.
 * @param d_timing: Delta timing for cost summary dialog function.
 * @param d_bounding_box: Delta bounding box for cost summary dialog function.
 * @param move_outcome: Move result from placement for cost summary dialog function.
 */
void update_manual_move_costs(double d_cost, double d_timing, double d_bounding_box, e_move_result& move_outcome);

/**
 * @brief Checks if the given string is a number
 *
 * @return True if the string only contains numbers, false otherwise.
 */
bool string_is_a_number(std::string block_id);

/**
 * @brief Returns the block requested by the user for manual moves.
 */
int return_block_id();

/**
 * @brief Returns the to location requested by the user for manual moves.
 */
t_pl_loc return_to_loc();

#endif /*NO_GRAPHICS*/

#endif /* MANUAL_MOVES_H */
