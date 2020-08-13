#include "breakpoint.h"
#include "draw_global.h"

#include <iostream>

//global variables
int tempCount = 0;
current_information current_info_b;

//gets current information from place.cpp
void send_current_info_b(current_information ci) {
    get_current_info_e(ci);
    current_info_b = ci;
}

//for accessing get_current_info_b. specifically in place.cpp
current_information get_current_info_b() {
    return current_info_b;
}

//checks if there are any move breakpoints
bool check_for_moves_breakpoints(int moves_to_proceed) {
    std::string expression = "move_num += " + std::to_string(moves_to_proceed);
    return check_for_expression_breakpoints(expression, true);
}

//check for temperature breakpoint
bool check_for_temperature_breakpoints(int temps_to_proceed) {
    std::string expression = "temp_count += " + std::to_string(temps_to_proceed);
    return check_for_expression_breakpoints(expression, true);
}

//check for block breakpoint
bool check_for_block_breakpoints(int user_blockId) {
    std::string expression = "from_block == " + std::to_string(user_blockId);
    return check_for_expression_breakpoints(expression, true);
}

//checks for router iteration breakpoint
bool check_for_router_iter_breakpoints(int routerIter) {
    std::string expression = "router_iter == " + std::to_string(routerIter);
    return check_for_expression_breakpoints(expression, false);
}

//checks for router net id breakpoint
bool check_for_route_net_id_iter_breakpoints(int net_id) {
    std::string expression = "route_net_id == " + std::to_string(net_id);
    return check_for_expression_breakpoints(expression, false);
}

//checks for expression breakpoint
bool check_for_expression_breakpoints(std::string expression, bool in_placer) {
    vtr::
        FormulaParser fp;
    vtr::t_formula_data dummy;
    is_a_breakpoint(true);
    int result = fp.parse_formula(expression, dummy);
    is_a_breakpoint(false);
    if (result == 1) {
        std::cout << "\nStopped at " << expression << "\n";
        current_info_b.bp_description = expression;
        print_current_info(in_placer);
    }

    return result;
}

//checks for all types of breakpoints
bool check_for_breakpoints(bool in_placer) {
    //goes through the breakpoints vector
    t_draw_state* draw_state = get_draw_state_vars();
    for (size_t i = 0; i < draw_state->list_of_breakpoints.size(); i++) {
        if (draw_state->list_of_breakpoints[i].type.compare("bt_moves") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_moves_breakpoints(draw_state->list_of_breakpoints[i].bt_moves);
        else if (draw_state->list_of_breakpoints[i].type.compare("bt_from_block") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_block_breakpoints(draw_state->list_of_breakpoints[i].bt_from_block);
        else if (draw_state->list_of_breakpoints[i].type.compare("bt_temps") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_temperature_breakpoints(draw_state->list_of_breakpoints[i].bt_temps);
        else if (draw_state->list_of_breakpoints[i].type.compare("bt_router_iter") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_router_iter_breakpoints(draw_state->list_of_breakpoints[i].bt_router_iter);
        else if (draw_state->list_of_breakpoints[i].type.compare("bt_route_net_id") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_route_net_id_iter_breakpoints(draw_state->list_of_breakpoints[i].bt_route_net_id);
        else if (draw_state->list_of_breakpoints[i].type.compare("bt_expression") == 0 && draw_state->list_of_breakpoints[i].active)
            return check_for_expression_breakpoints(draw_state->list_of_breakpoints[i].bt_expression, in_placer);
    }
    return false;
}

//activates or deactivates a breakpoint given its index
void activate_breakpoint_by_index(int index, bool enabled) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->list_of_breakpoints[index].active = enabled;
}

//deletes a breakpoint from the breakpoints vector given its index
void delete_breakpoint_by_index(int index) {
    t_draw_state* draw_state = get_draw_state_vars();
    draw_state->list_of_breakpoints.erase(draw_state->list_of_breakpoints.begin() + index);
}

//prints placement and router info to the terminal
void print_current_info(bool in_placer) {
    if(in_placer)
        std::cout << "\nmove_num: " << current_info_b.move_num << "\ntemp_count: " << current_info_b.temp_count << "\nin_blocks_affected: " << get_current_info_e_ba() << "\nblock_id: " << current_info_b.from_block << "\n----------------------------\n";
    else 
        std::cout<<"\nrouter_iter: " << current_info_b.router_iter << "\nnet_id: " << current_info_b.net_id << "\n----------------------------\n";
}
