#ifndef NO_GRAPHICS

#    include "breakpoint.h"

#    include <iostream>

//global variables
int moveCount = 0;
int tempCount = 0;
std::vector<breakpoint> list_of_breakpoints;

breakpoint::breakpoint() {
        moves = 0;
        temps = 0;
        id = -1;
}

breakpoint::breakpoint(std::string ty, int mtb) {
        type = ty;

        if(ty.compare("moves")==0)
            moves = mtb;
        else if (ty.compare("temps")==0)
            temps = mtb;
        else if (ty.compare("blockId")==0)
            id = mtb;
}

breakpoint::breakpoint(std::string ty, std::string expr) {
        type = ty;
        expression = expr;
}

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

std::vector<breakpoint> get_list_of_breakpoints() {
    return list_of_breakpoints;
}

void breakpoint::check_for_moves_breakpoints(int moves_to_proceed, bool* f_placer_debug) {

    //check for move breakpoint
    if(moves_to_proceed>=1){
        if(moveCount==moves_to_proceed){
            *f_placer_debug = true;
            moveCount=0;
        }
        else if(moveCount<moves_to_proceed){
            *f_placer_debug = false;
            moveCount++;
            std::cout<<moveCount<<"\n";
        }
    }
}

void breakpoint::check_for_temperature_breakpoints(int temps_to_proceed, bool* f_placer_debug) {

    //check for temperature breakpoint
    if(temps_to_proceed>=1){
        if(tempCount == temps_to_proceed){
            *f_placer_debug = true;
            tempCount=0;
        }
        else if(tempCount<temps_to_proceed){
            *f_placer_debug = false;
            tempCount++;
            std::cout<<"tempcount: "<<tempCount<<"\n";
        }
    }
}

void breakpoint::check_for_block_breakpoints(ClusterBlockId current_blockId, int user_blockId, bool* f_placer_debug) {

    //check for block breakpoint
    ClusterBlockId bId(user_blockId);
    if(bId==current_blockId)
        *f_placer_debug = true;
    else 
        *f_placer_debug = false;
}

void breakpoint::check_for_expression_breakpoints(std::string expression, bool* f_placer_debug) {

}

void breakpoint::check_for_breakpoints(ClusterBlockId blockId, bool* f_placer_debug) {
    for(int i = 0; i<list_of_breakpoints.size(); i++) {
        if(list_of_breakpoints[i].type.compare("moves") == 0)
            check_for_moves_breakpoints(list_of_breakpoints[i].moves, f_placer_debug);
        else if(list_of_breakpoints[i].type.compare("temps") == 0)
            check_for_temperature_breakpoints(list_of_breakpoints[i].temps, f_placer_debug);
        else if(list_of_breakpoints[i].type.compare("blockId") == 0)
            check_for_block_breakpoints(blockId, list_of_breakpoints[i].id, f_placer_debug);
        else if(list_of_breakpoints[i].type.compare("expression") == 0)
            check_for_expression_breakpoints(list_of_breakpoints[i].expression, f_placer_debug);
    }
}

#endif // NO_GRAPHICS
