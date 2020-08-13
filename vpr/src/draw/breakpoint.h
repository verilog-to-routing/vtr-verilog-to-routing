#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <vector>
#include <string>

#include "move_transactions.h"
#include "vtr_expr_eval.h"

class Breakpoint {
  public:
    //types include: bt_moves, bt_temps, bt_from_block, bt_router_iter, bt_route_net_id, bt_expression
    std::string type;
    int bt_moves = 0;
    int bt_temps = 0;
    int bt_from_block = -1;
    int bt_router_iter = 0;
    int bt_route_net_id = -1;
    bool active = true;
    std::string bt_expression;

    //constructors
    Breakpoint() {
        bt_moves = 0;
        bt_temps = 0;
        bt_from_block = -1;
        bt_router_iter = 0;
        bt_route_net_id = -1;
        active = true;
    }

    //sets move_num/from_block/temp_num/router_iter/net_id type of breakpoint based on the type indicated by string ty. Also sets the corresponding breakpoint value indicated by int breakpoint_value
    Breakpoint(std::string ty, int breakpoint_value) {
        type = ty;
        if (ty.compare("bt_moves") == 0)
            bt_moves = breakpoint_value;
        else if (ty.compare("bt_temps") == 0)
            bt_temps = breakpoint_value;
        else if (ty.compare("bt_from_block") == 0)
            bt_from_block = breakpoint_value;
        else if (ty.compare("bt_router_iter") == 0)
            bt_router_iter = breakpoint_value;
        else if (ty.compare("bt_route_net_id") == 0)
            bt_route_net_id = breakpoint_value;
    }

    Breakpoint(std::string ty, std::string expr) {
        type = ty;
        bt_expression = expr;
    }

    //overridden operators
    bool operator==(const Breakpoint& bp) {
        if (bp.type.compare(this->type) != 0)
            return false;
        else {
            if (bp.type.compare("bt_moves") == 0 && bp.bt_moves == this->bt_moves)
                return true;
            else if (bp.type.compare("bt_temps") == 0 && bp.bt_temps == this->bt_temps)
                return true;
            else if (bp.type.compare("bt_from_block") == 0 && bp.bt_from_block == this->bt_from_block)
                return true;
            else if (bp.type.compare("bt_router_iter") == 0 && bp.bt_router_iter == this->bt_router_iter)
                return true;
            else if (bp.type.compare("bt_route_net_id") == 0 && bp.bt_route_net_id == this->bt_route_net_id)
                return true;
            else if (bp.type.compare("bt_expression") == 0 && bp.bt_expression.compare(this->bt_expression) == 0)
                return true;
            else
                return false;
        }
    }
};

//breakpoint realted functions to be called from draw.cpp, place.cpp and route_timing.cpp

//checks for all types of breakpoint and returns true if a breakpoint is encountered
bool check_for_breakpoints(bool in_placer);

//get user input from the gtk entries in the debug window and creates equivalent expression and checks if the breakpoint is encountered
bool check_for_block_breakpoints(int user_blockId);
bool check_for_temperature_breakpoints(int temps_to_proceed);
bool check_for_moves_breakpoints(int moves_to_proceed);
bool check_for_router_iter_breakpoints(int routerIter);
bool check_for_route_net_id_iter_breakpoints(int net_id);

//using expr_eval functions parses breakpoint indicated by expression (through gtk entry in the debugger window) and returns true if breakpoint is encountered
bool check_for_expression_breakpoints(std::string expression, bool in_placer);


//deletes indicated breakpoint from the breakpoint list
void delete_breakpoint_by_index(int index);

//activates and deactivates indicated breakpoint
void activate_breakpoint_by_index(int index, bool enabled);

//sends the current BreakpointState to expre_eval.cpp
void send_current_info_b(current_information ci);

//returns current BreakpointState in the breakpoint class
current_information get_current_info_b();

//prints current BreakpointState information to terminal when breakpoint is reached
void print_current_info(bool in_placer);

#endif /* BREAKPOINT_H */
