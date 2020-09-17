#include "breakpoint.h"
#include "draw_global.h"

#include <iostream>

#ifndef NO_GRAPHICS
//if the user adds a "proceed move" breakpoint using the entry field in the UI, this function converts it to the equivalent expression and calls the expression evaluator. Returns true if a breakpoint is encountered
//the way the proceed moves breakpoint works is that it proceeds the indicated number of moves from where the placer currently is i.e if at move 3 and proceed 4 ends up at move 7
bool check_for_moves_breakpoints(int moves_to_proceed) {
    std::string expression = "move_num += " + std::to_string(moves_to_proceed);
    return check_for_expression_breakpoints(expression, true);
}

//if the user adds a "proceed temperature" breakpoint using the entry field in the UI, this function converts it to the equivalent expression and calls the expression evaluator. Returns true if a breakpoint is encountered
//the way the proceed temps breakpoint works is that it proceeds the indicated number of temperature changes from where the placer currently is
bool check_for_temperature_breakpoints(int temps_to_proceed) {
    std::string expression = "temp_count += " + std::to_string(temps_to_proceed);
    return check_for_expression_breakpoints(expression, true);
}

//if the user adds a "from_block" breakpoint using the entry field in the UI, this function converts it to the equivalent expression and calls the expression evaluator. Returns true if a breakpoint is encountered
//the way the from_block breakpoint works is that it stops when the first block moved in a perturbation matched the block indicated by the user
bool check_for_block_breakpoints(int user_blockId) {
    std::string expression = "from_block == " + std::to_string(user_blockId);
    return check_for_expression_breakpoints(expression, true);
}

//if the user adds a "router_iter" breakpoint using the entry field in the UI, this function converts it to the equivalent expression and calls the expression evaluator. Returns true if a breakpoint is encountered
//the way the router_iter breakpoint works is that it stops after the indicated number of router iterations have been made
bool check_for_router_iter_breakpoints(int routerIter) {
    std::string expression = "router_iter == " + std::to_string(routerIter);
    return check_for_expression_breakpoints(expression, false);
}

//if the user adds a "route_net_id" breakpoint using the entry field in the UI, this function converts it to the equivalent expression and calls the expression evaluator. Returns true if a breakpoint is encountered
//the way the route_net_id breakpoint works is that it stops after the indicated net was moved in the router
bool check_for_route_net_id_iter_breakpoints(int net_id) {
    std::string expression = "route_net_id == " + std::to_string(net_id);
    return check_for_expression_breakpoints(expression, false);
}

//sends the expression indicated by the user to the expression evaluator and after parsing, checks if any breakpoints were reached. returns true if breakpoint encountered
bool check_for_expression_breakpoints(std::string expression, bool in_placer) {
    vtr::FormulaParser fp;
    vtr::t_formula_data dummy;
    int result = fp.parse_formula(expression, dummy, true);
    if (result == 1) {
        std::cout << "\nStopped at " << expression << "\n";
        get_bp_state_globals()->get_glob_breakpoint_state()->bp_description = expression;
        print_current_info(in_placer);
    }

    return result;
}

//checks for all types of breakpoints
bool check_for_breakpoints(bool in_placer) {
    //goes through the breakpoints vector
    t_draw_state* draw_state = get_draw_state_vars();
    for (size_t i = 0; i < draw_state->list_of_breakpoints.size(); i++) {
        if (draw_state->list_of_breakpoints[i].type == BT_MOVE_NUM && draw_state->list_of_breakpoints[i].active)
            return check_for_moves_breakpoints(draw_state->list_of_breakpoints[i].bt_moves);
        else if (draw_state->list_of_breakpoints[i].type == BT_FROM_BLOCK && draw_state->list_of_breakpoints[i].active)
            return check_for_block_breakpoints(draw_state->list_of_breakpoints[i].bt_from_block);
        else if (draw_state->list_of_breakpoints[i].type == BT_TEMP_NUM && draw_state->list_of_breakpoints[i].active)
            return check_for_temperature_breakpoints(draw_state->list_of_breakpoints[i].bt_temps);
        else if (draw_state->list_of_breakpoints[i].type == BT_ROUTER_ITER && draw_state->list_of_breakpoints[i].active)
            return check_for_router_iter_breakpoints(draw_state->list_of_breakpoints[i].bt_router_iter);
        else if (draw_state->list_of_breakpoints[i].type == BT_ROUTE_NET_ID && draw_state->list_of_breakpoints[i].active)
            return check_for_route_net_id_iter_breakpoints(draw_state->list_of_breakpoints[i].bt_route_net_id);
        else if (draw_state->list_of_breakpoints[i].type == BT_EXPRESSION && draw_state->list_of_breakpoints[i].active)
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

//prints the current placer information to the terminal
void print_current_info(bool in_placer) {
    if (in_placer)
        std::cout << "\nmove_num: " << get_bp_state_globals()->get_glob_breakpoint_state()->move_num << "\ntemp_count: " << get_bp_state_globals()->get_glob_breakpoint_state()->temp_count << "\nin_blocks_affected: " << get_bp_state_globals()->get_glob_breakpoint_state()->block_affected << "\nfrom_block: " << get_bp_state_globals()->get_glob_breakpoint_state()->from_block << "\n----------------------------\n";
    else
        std::cout << "\nrouter_iter: " << get_bp_state_globals()->get_glob_breakpoint_state()->router_iter << "\nnet_id: " << get_bp_state_globals()->get_glob_breakpoint_state()->route_net_id << "\n----------------------------\n";
}
#endif
