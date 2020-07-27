#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <vector>
#include <string>

#include "move_transactions.h"
#include "vtr_expr_eval.h"

class breakpoint {
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
    breakpoint() {
        bt_moves = 0;
        bt_temps = 0;
        bt_from_block = -1;
        bt_router_iter = 0;
        bt_route_net_id = -1;
        active = true;
    }

    breakpoint(std::string ty, int mtb) {
        type = ty;
        if (ty.compare("bt_moves") == 0)
            bt_moves = mtb;
        else if (ty.compare("bt_temps") == 0)
            bt_temps = mtb;
        else if (ty.compare("bt_from_block") == 0)
            bt_from_block = mtb;
        else if (ty.compare("bt_router_iter") == 0)
            bt_router_iter = mtb;
        else if (ty.compare("bt_route_net_id") == 0)
            bt_route_net_id = mtb;
    }

    breakpoint(std::string ty, std::string expr) {
        type = ty;
        bt_expression = expr;
    }

    //overridden operators
    bool operator==(const breakpoint& bp) {
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

//functions
bool check_for_breakpoints();
bool check_for_block_breakpoints(int user_blockId);
bool check_for_temperature_breakpoints(int temps_to_proceed);
bool check_for_moves_breakpoints(int moves_to_proceed);
bool check_for_router_iter_breakpoints(int routerIter);
bool check_for_route_net_id_iter_breakpoints(int net_id);
bool check_for_expression_breakpoints(std::string expression);
void delete_reached_breakpoint(std::string type, int bp_info);
void delete_breakpoint_by_index(int index);
void activate_breakpoint_by_index(int index, bool enabled);
void send_current_info_b(current_information ci);
current_information get_current_info_b();
void print_current_info();

#endif /* BREAKPOINT_H */
