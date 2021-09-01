/*
 * @file 	manual_moves.h
 * @author	Paula Perdomo
 * @date 	2021-07-19
 * @brief 	Contains the function prototypes needed for manual moves feature.
 *
 * Includes the data structures and gtk function for manual moves. The Manual Move Generator class is defined  manual_move_generator.h/cpp.
 */

#ifndef MANUAL_MOVES_H
#define MANUAL_MOVES_H

/** This file contains all functions for manual moves **/
#ifndef NO_GRAPHICS

#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"
#    include "manual_move_generator.h"

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
 *
 * GUI writes to:
 * blockID: Stores the block ID of the block requested to move by the user. This block is the from block in the move generator.
 * x_pos: Stores the x position of the block requested to move by the user. 
 * y_pos: Stores the y position of the block requested to move by the user. 
 * subtile: Stores the subtile of the block requested to move by the user.
 * valid input: Stores whether the manual move is valid with respect to the user's input in the UI. 
 * user_move_outcome: The user determines to acceopt or reject the manual move. 
 *
 * Placer writes to:
 * delta_cost: Stores the delta cost of the manual move requested by the user.
 * delta_timing: Stores the delta timing of the manual move requested by the user. 
 * delta_bounding_box: Stores the delta bounding box cost of the manual move requested by the user. 
 * to_location: Stores the x position, y position and subtile position requested by the user in a t_pl_loc variable. 
 * placer_move_outcome: The placer determines if the manual move requested by the user is valid. 
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
 * @brief ManualMovesState struct
 *
 * Contains a ManualMovesInfo variable to store manual move data, boolean values to check the state of the windows needed and gtk manual move widow variable.
 * ManualMovesInfo: Stores the manual move information from the user's input and the placer's/user's outcome to the move.
 * manual_move_window_is_open: Stores whether the manual move window is open or not. 
 * user_highlighted_block: Stores whether user highlighted block in UI instead of entering the block ID manually. 
 * manual_move_window: GtkWindow for the manual move. In this window the user inputs the block ID and to position of the block to move. 
 */

struct ManualMovesState {
    ManualMovesInfo manual_move_info;
    bool manual_move_window_is_open = false;
    bool user_highlighted_block = false;
    bool manual_move_enabled = false;
    GtkWidget* manual_move_window;
};

/** manual moves functions **/

/**
 * @brief Gets the state of the manual moves togle button and assigns it to the manual_move_enabled in the ManualMovesState struct.
 *
 * @return True if the toggle button is active, false otherwise.
 */
bool manual_move_is_selected();

/**
 * @brief Draws the manual move window.
 *
 * Window prompts the user for input: block id/name used as the from block in the move generator, x position, y position, and subtile position.
 * @param block_id: The block id is passed in if the user decides to highlight the block in the UI. If the user decides to manually input the block ID in the manual move window, the string will be empty and the block ID will later be assigned to ManualMovesState struct.
 */
void draw_manual_moves_window(std::string block_id);

/**
 * @brief Evaluates if the user input is valid and allowed.
 *
 * Sets the members from the ManualMovesState manual_moves_state variable to their respective values (block id and locations).
 * @param GtkWidget* widget: Passed in for gtk callback functions (Needed due to the GTK function protoype, in GTK documentation).
 * @param GtkWidget* grid: The grid is used to extract the user input from the manual move window, to later assign to the ManualMovesState variable. 
 */
void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid);

/**
 * @brief In -detail checking of the user's input.
 *
 * Checks if the user input is between the grid's dimensions, block comptaibility, if the block requested to move is valid, if the block is fixed, and if the curent location of the block is different from the location requested by the user.
 * @param block_id: The ID of the block to move used as the from block in the move generator).
 * @param to: Location of where the user wants to move the block.
 *
 * @return True if all conditions are met, false otherwise.
 */
bool is_manual_move_legal(ClusterBlockId block_id, t_pl_loc to);

/**
 * @brief Draws the cost summary dialog.
 *
 * Window displays the delta cost, delta timing, delta bounding box timing, annealing decision to the user. Waits for the user to ACCEPT/REJECT the manual move.
 */
void manual_move_cost_summary_dialog();

/**
 * @brief Highlights new block location
 *
 * Highlights block in the new location if the manual move is active and the user accepted the manual move.
 */
void manual_move_highlight_new_block_location();

/**
 * @brief Disables the manual_move_window_is_open boolean and destroys the window
 */
void close_manual_moves_window();

/**
 * @brief Checks if the given string is a number
 *
 * @return True if the string only contains numbers, false otherwise.
 */
bool string_is_a_number(std::string block_id);

/**
 * @brief Updates the ManualMovesState variable members.
 *
 * @param d_cost: Delta cost for cost summary dialog function.
 * @param d_timing: Delta timing for cost summary dialog function.
 * @param d_bounding_box: Delta bounding box for cost summary dialog function.
 * @param move_outcome: Move result from placement for cost summary dialog function.
 * 
 * Helper function used in place.cpp. The ManualMovesState variable are updated and the manual_move_cost_summary_dialog is called to display the cost members to the user in the UI and waits for the user to either ACCPET/REJECT the manual move. 
 */
e_move_result pl_do_manual_move(double d_cost, double d_timing, double d_bounding_box, e_move_result& move_outcome);

e_create_move manual_move_display_and_propose(ManualMoveGenerator& manual_move_generator, t_pl_blocks_to_be_moved& blocks_affected, e_move_type& move_type, float rlim, const t_placer_opts& placer_opts, const PlacerCriticalities* criticalities);

#endif /*NO_GRAPHICS*/

#endif /* MANUAL_MOVES_H */
