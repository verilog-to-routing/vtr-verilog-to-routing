#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <vector>
#include <string>

#include "move_transactions.h"
#include "vtr_expr_eval.h"

/**This class holds the definiton of type Breakpoint as well as al realted functions. Breakpoints can be set through the GUI anytime during placement or routing. Breakpoints can also be activated, deactivated, and deleted. Each breakpoint has a type (BT_MOVE_NUM, BT_TEMP_NUM, BT_FROM_BLOCK, BT_ROUTER_ITER, BT_ROUTE_NET_ID, BT_EXPRESSION) and holds the values for corresponding to its type, as well as a boolean variable to activate and deactivate a breakpoint. It should be noted that each breakpoint can only have one type and hold one value corresponding to that type. More complicated breakpoints are set using an expression. (e.g move_num > 3 && block_id == 11) 
 * Breakpoints can be create using 3 constructors, the default contructor that doesn't identify the type and sets a breakopint that is never reached, a constructor that takes in the type and an int value, and lastly a constructor that takes in the type and the sting that holds the expression. (e.g Breakpoint(BT_MOVE_NUM, 4) or Breakpoint(BT_EXPRESSION, "move_num += 3")) The == operator has also been provided which returns true when two breakpoints have the same type, and the same value corresponding to the type.**/

typedef enum breakpoint_types {
    BT_MOVE_NUM,
    BT_TEMP_NUM,
    BT_FROM_BLOCK,
    BT_ROUTER_ITER,
    BT_ROUTE_NET_ID,
    BT_EXPRESSION,
    BT_UNIDENTIFIED
} bp_type;

class Breakpoint {
  public:
    bp_type type;
    int bt_moves = 0;          ///breaks after proceeding the indicated number of moves in placement
    int bt_temps = 0;          ///breaks after proceeding the indicated number of temperature changes in placement
    int bt_from_block = -1;    ///breaks after the first block moved in a placement perturbation matches the block id indicated by user
    int bt_router_iter = 0;    ///breaks when reaching the indicated router iteration
    int bt_route_net_id = -1;  ///breaks after the indicated net is routed
    std::string bt_expression; ///breaks when expression returns true after parsing
    bool active = true;        ///indicates whether a breakpoint is active or not

    //constructors
    //sets a breakpoint that is never reached
    Breakpoint() {
        type = BT_UNIDENTIFIED;
        bt_moves = -1;
        bt_temps = -1;
        bt_from_block = -1;
        bt_router_iter = -1;
        bt_route_net_id = -1;
        bt_expression = "";
        active = true;
    }

    //sets the type of the breakpoint based on the indicated bp_type. the constructor then assigns the corresponding value e.g Breakpoint (BT_MOVE_NUM, 4) sets breakpoint type to BT_MOVE_NUM and the bt_moves value to 4
    Breakpoint(bp_type ty, int breakpoint_value) {
        type = ty;
        if (ty == BT_MOVE_NUM)
            bt_moves = breakpoint_value;
        else if (ty == BT_TEMP_NUM)
            bt_temps = breakpoint_value;
        else if (ty == BT_FROM_BLOCK)
            bt_from_block = breakpoint_value;
        else if (ty == BT_ROUTER_ITER)
            bt_router_iter = breakpoint_value;
        else if (ty == BT_ROUTE_NET_ID)
            bt_route_net_id = breakpoint_value;
    }

    //sets the breakpoint type to BT_EXPRESSION and sets the string member bt_expression to the expresssion the user inputted. e.g Breakpoint(BT_EXPRESSION, "move_num==3")
    //the user can set breakpoints, during routing or placement, using an expression with the available variables and operators
    Breakpoint(bp_type ty, std::string expr) {
        type = ty;
        bt_expression = expr;
    }

    //overridden operators
    bool operator==(const Breakpoint& bp) {
        if (bp.type != this->type)
            return false;
        else {
            if (bp.type == BT_MOVE_NUM && bp.bt_moves == this->bt_moves)
                return true;
            else if (bp.type == BT_TEMP_NUM && bp.bt_temps == this->bt_temps)
                return true;
            else if (bp.type == BT_FROM_BLOCK && bp.bt_from_block == this->bt_from_block)
                return true;
            else if (bp.type == BT_ROUTER_ITER && bp.bt_router_iter == this->bt_router_iter)
                return true;
            else if (bp.type == BT_ROUTE_NET_ID && bp.bt_route_net_id == this->bt_route_net_id)
                return true;
            else if (bp.type == BT_EXPRESSION && bp.bt_expression.compare(this->bt_expression) == 0)
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
void send_current_info_b(BreakpointState ci);

//returns current BreakpointState in the breakpoint class
BreakpointState get_current_info_b();

//prints current BreakpointState information to terminal when breakpoint is reached
void print_current_info(bool in_placer);

#endif /* BREAKPOINT_H */
