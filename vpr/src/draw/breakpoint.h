#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#    include <vector>
#    include <string>

#    include "move_transactions.h"

#ifndef NO_GRAPHICS

class breakpoint {
    public:
    //types include: moves, temps, blockId, expression
    std::string type;
    int moves = 0;
    int temps = 0;
    int id = -1;
    std::string expression;

    //constructors
    breakpoint();
    breakpoint(std::string ty, int mtb);
    breakpoint(std::string ty, std::string expr);

    //destructor
    ~breakpoint();
    
    //accessors
    std::string get_type();
    int get_moves_num();
    int get_temps_num();
    int get_block_id();
    std::string get_expression();

    //methods
    void check_for_breakpoints(ClusterBlockId blockId, bool* f_placer_debug);
    void check_for_block_breakpoints(ClusterBlockId current_blockId, int user_blockId, bool* f_placer_debug);
    void check_for_temperature_breakpoints(int temps_to_proceed, bool* f_placer_debug);
    void check_for_moves_breakpoints(int moves_to_proceed, bool* f_placer_debug);
    void check_for_expression_breakpoints(std::string expression, bool* f_placer_debug);
};
std::vector<breakpoint> get_list_of_breakpoints();

#endif /* NO_GRAPHICS */

#endif /* BREAKPOINT_H */
