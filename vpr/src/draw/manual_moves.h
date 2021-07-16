#ifndef MANUAL_MOVES_H
#define MANUAL_MOVES_H

/** This file contains all functions for manual moves **/

#ifndef NO_GRAPHICS

#    include "draw_global.h"
#    include "draw_global.h"
#    include "move_utils.h"
#    include "ezgl/application.hpp"
#    include "ezgl/graphics.hpp"

#    include <cstdio>
#    include <cfloat>
#    include <cstring>
#    include <cmath>
#    include <algorithm>
#    include <iostream>

#endif /*NO_GRAPHICS*/

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
    GtkWidget* manual_move_window;
    bool mm_window_is_open = false;
    bool user_highlighted_block = false;
    bool manual_move_flag = false;
};

#ifndef NO_GRAPHICS

/** manual moves functions **/
void draw_manual_moves_window(std::string block_id);
void close_manual_moves_window();
void calculate_cost_callback(GtkWidget* /*widget*/, GtkWidget* grid);
bool checking_legality_conditions(ClusterBlockId block_id, t_pl_loc to);
void get_manual_move_flag();
void cost_summary_dialog();
void highlight_new_block_location(bool manual_move_flag);

#endif /*NO_GRAPHICS */

ManualMovesGlobals* get_manual_moves_global();
void update_manual_move_costs(double d_cost, double d_timing, double d_bounding_box, e_move_result& move_outcome);
bool string_is_a_number(std::string block_id);

#endif /* MANUAL_MOVES_H */
