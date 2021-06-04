#ifndef MANUAL_MOVES_H
#define MANUAL_MOVES_H

/** This file contains all functions for manual moves **/
#ifndef NO_GRAPHICS

#    include "draw_global.h"
#    include "draw_global.h"
#    include "move_utils.h"
#    include "move_generator.h"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

#    include <cstdio>
#    include <cfloat>
#    include <cstring>
#    include <cmath>
#    include <algorithm>
#    include <sstream>
#    include <array>
#    include <iostream>

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

struct ManualMovesGlobals {
	ManualMovesInfo manual_move_info;
	bool mm_window_is_open = false;
	GtkWidget* manual_move_window;
	bool cost_window_is_open = false;
};

/** Manual Moves Generator, inherits from MoveGenerator **/
class ManualMoveGenerator: public MoveGenerator {
	public:
		//Evaluates if move is successful and legal or unable to do. 
		e_create_move propose_move(t_pl_blocks_to_be_moved& blocks_affected, float /*rlim*/);
};

//extern ManualMovesGlobals manual_moves_global;

/** manual moves functions **/
void draw_manual_moves_window(std::string block_id);
void close_manual_moves_window();
void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid);
bool string_is_a_number(std::string block_id);
void cost_summary();
bool find_to_loc_manual_move(t_logical_block_type_ptr type, t_pl_loc& to);
ManualMovesInfo* get_manual_move_info();
ManualMovesGlobals* get_manual_moves_global();
void accept_manual_move_window(GtkWidget* /*widget*/, GtkWidget* cost_window);
void reject_manual_move_window(GtkWidget* /*widget*/, GtkWidget* cost_window);



#endif /*NO_GRAPHICS */

#endif /* MANUAL_MOVES_H */
