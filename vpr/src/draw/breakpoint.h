#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <vector>
#include <string>

#include "move_transactions.h"
#include "expr_eval.h"

class breakpoint {
  public:
    //types include: moves, temps, blockId, expression
    std::string type;
    int moves = 0;
    int temps = 0;
    int id = -1;
    bool active = true;
    std::string expression;

    //constructors
    breakpoint() {
        moves = 0;
        temps = 0;
        id = -1;
        active = true;
    }

    breakpoint(std::string ty, int mtb) {
        type = ty;
        if (ty.compare("moves") == 0)
            moves = mtb;
        else if (ty.compare("temps") == 0)
            temps = mtb;
        else if (ty.compare("blockId") == 0)
            id = mtb;
    }

    breakpoint(std::string ty, std::string expr) {
        type = ty;
        expression = expr;
    }

    //overridden operators
    bool operator==(const breakpoint& bp) {
        if (bp.type.compare(this->type) != 0)
            return false;
        else {
            if (bp.type.compare("moves") == 0 && bp.moves == this->moves)
                return true;
            else if (bp.type.compare("temps") == 0 && bp.temps == this->temps)
                return true;
            else if (bp.type.compare("id") == 0 && bp.id == this->id)
                return true;
            else if (bp.type.compare("expression") == 0 && bp.expression.compare(this->expression) == 0)
                return true;
            else
                return false;
        }
    }

    //accessors
    std::string get_type();
    int get_moves_num();
    int get_temps_num();
    int get_block_id();
    std::string get_expression();
};

//functions
bool check_for_breakpoints(ClusterBlockId blockId, bool temp_check);
bool check_for_block_breakpoints(ClusterBlockId current_blockId, int user_blockId);
bool check_for_temperature_breakpoints(int temps_to_proceed, bool temp_check);
bool check_for_moves_breakpoints(int moves_to_proceed);
bool check_for_expression_breakpoints(std::string expression);
void delete_reached_breakpoint(std::string type, int bp_info);
void delete_breakpoint_by_index(int index);
void activate_breakpoint_by_index(int index, bool enabled);
void get_current_info_b(current_information ci);
void print_current_info();

#endif /* BREAKPOINT_H */
