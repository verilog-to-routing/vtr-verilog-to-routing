#include "placer_breakpoint.h"


//map of the available move types and their corresponding type number
std::map<int, std::string> available_move_types = {
    {0, "Uniform"}};

#ifndef NO_GRAPHICS
//transforms the vector moved_blocks to a vector of ints and adds it in glob_breakpoint_state
void transform_blocks_affected(const t_pl_blocks_to_be_moved& blocksAffected) {
    BreakpointState* bp_state = get_bp_state_globals()->get_glob_breakpoint_state();

    bp_state->blocks_affected_by_move.clear();
    for (const t_pl_moved_block& moved_block : blocksAffected.moved_blocks) {
        //size_t conversion is required since block_num is of type ClusterBlockId and can't be cast to an int. And this vector has to be of type int to be recognized in expr_eval class
        bp_state->blocks_affected_by_move.push_back(size_t(moved_block.block_num));
    }
}

void stop_placement_and_check_breakpoints(t_pl_blocks_to_be_moved& blocks_affected, e_move_result move_outcome,
                                          double delta_c, double bb_delta_c, double timing_delta_c) {
    t_draw_state* draw_state = get_draw_state_vars();
    BreakpointState* bp_state = get_bp_state_globals()->get_glob_breakpoint_state();

    if (!draw_state->list_of_breakpoints.empty()) {
        //update current information
        transform_blocks_affected(blocks_affected);
        bp_state->move_num++;
        bp_state->from_block = size_t(blocks_affected.moved_blocks[0].block_num);

        //check for breakpoints
        set_placer_breakpoint_reached(check_for_breakpoints(true)); // the passed flag is true as we are in the placer
        if (placer_breakpoint_reached()) {
            breakpoint_info_window(bp_state->bp_description, *bp_state, true);
        }
    } else {
        set_placer_breakpoint_reached(false);
    }

    if (placer_breakpoint_reached() && draw_state->show_graphics) {
        std::string msg = available_move_types[0];
        if (move_outcome == e_move_result::REJECTED) {
            msg += vtr::string_fmt(", Rejected");
        } else if (move_outcome == e_move_result::ACCEPTED) {
            msg += vtr::string_fmt(", Accepted");
        } else {
            msg += vtr::string_fmt(", Aborted");
        }

        msg += vtr::string_fmt(", Delta_cost: %1.6f (bb_delta_cost= %1.5f , timing_delta_c= %6.1e)", delta_c, bb_delta_c, timing_delta_c);

        highlight_moved_block_and_its_terminals(blocks_affected);
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), PLACEMENT, nullptr);
    }
}

#endif //NO_GRAPHICS
