#include "breakpoint.h"
#include "draw_global.h"

#include <iostream>

//global variables
int moveCount = 0;
int tempCount = 0;
current_information current_info_b;

//accessors
std::string breakpoint::get_type() {
    return type;
}

int breakpoint::get_moves_num() {
    return moves;
}

int breakpoint::get_temps_num() {
    return temps;
}

int breakpoint::get_block_id() {
    return id;
}

std::string breakpoint::get_expression() {
    return expression;
}

//gets current information from place.cpp
void get_current_info_b(current_information ci) {
    get_current_info_e(ci);
    current_info_b = ci;
}

//checks if there are any move breakpoints
bool check_for_moves_breakpoints(int moves_to_proceed) {
    bool stop;
    if (moves_to_proceed >= 1) {
        if (moveCount == moves_to_proceed) {
            moveCount = 0;
            std::cout << "\nStopped at move_num += " << std::to_string(moves_to_proceed) << "\n";
            print_current_info();
            return true;
        } else if (moveCount < moves_to_proceed) {
            moveCount++;
            return false;
        }
    }
}

//check for temperature breakpoint
bool check_for_temperature_breakpoints(int temps_to_proceed) {
    bool stop;
    if (temps_to_proceed >= 1) {
        if (tempCount == temps_to_proceed) {
            tempCount = 0;
            std::cout << "\nStopped at temp_num += " << std::to_string(temps_to_proceed) << "\n";
            print_current_info();
            return true;
        } else if (tempCount < temps_to_proceed) {
            tempCount++;
            return false;
        }
    }
}

//check for block breakpoint
bool check_for_block_breakpoints(ClusterBlockId current_blockId, int user_blockId) {
    bool stop;
    ClusterBlockId bId(user_blockId);
    if (bId == current_blockId) {
        std::cout << "\nStopped at from_block == " << std::to_string(user_blockId) << "\n";
        print_current_info();
        stop = true;
    } else
        stop = false;
    return stop;
}

//check for expression breakpoint
bool check_for_expression_breakpoints(std::string expression) {
    FormulaParser fp;
    t_formula_data dummy;
    is_a_breakpoint(true);
    int result = fp.parse_formula(expression, dummy);
    is_a_breakpoint(false);
    if (result == 1) {
        std::cout << "\nStopped at" << expression << "\n";
        print_current_info();
    }

    return result;
}

//checks for all types of breakpoints
bool check_for_breakpoints(ClusterBlockId blockId) {
    //goes through the breakpoints vector
    t_draw_state* draw_state = get_draw_state_vars();
    for (size_t i = 0; i < draw_state->list_of_breakpoints.size(); i++) {
        if (draw_state->list_of_breakpoints[i].type.compare("moves") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_moves_breakpoints(draw_state->list_of_breakpoints[i].moves);
        if (draw_state->list_of_breakpoints[i].type.compare("blockId") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_block_breakpoints(blockId, draw_state->list_of_breakpoints[i].id);
        if (draw_state->list_of_breakpoints[i].type.compare("expression") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_expression_breakpoints(draw_state->list_of_breakpoints[i].expression);
        if (draw_state->list_of_breakpoints[i].type.compare("temps") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_temperature_breakpoints(draw_state->list_of_breakpoints[i].temps);
    }
}

//activates or deactivates a breakpoint given its index
void activate_breakpoint_by_index(int index, bool enabled) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (enabled)
        draw_state->list_of_breakpoints[index].active = true;
    else
        draw_state->list_of_breakpoints[index].active = false;
}

//deletes a breakpoint from the breakpoints vector given its index
void delete_breakpoint_by_index(int index) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->list_of_breakpoints.erase(draw_state->list_of_breakpoints.begin() + index);
}

//auto deletes a breakpoint if reached (currently not called anywhere)
void delete_reached_breakpoint(std::string type, int bp_info) {
    //find the breakpoint
    t_draw_state* draw_state = get_draw_state_vars();
    breakpoint temp = breakpoint(type, bp_info);
    std::vector<breakpoint>::iterator it = std::find(draw_state->list_of_breakpoints.begin(), draw_state->list_of_breakpoints.end(), temp);

    //delete the found breakpoint
    if (it != draw_state->list_of_breakpoints.end())
        draw_state->list_of_breakpoints.erase(it);
}

void print_current_info() {
    std::cout << "\nmove_num: " << current_info_b.moveNumber << "\ntemp_num: " << current_info_b.temperature << "\nnet_id: " << current_info_b.netNumber << "\nblock_id: " << current_info_b.blockNumber << "\n----------------------------\n";
}
