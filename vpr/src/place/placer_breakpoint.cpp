#include "placer_breakpoint.h"

#ifdef VTR_ENABLE_DEBUG_LOGGING

//map of the available move types and their corresponding type number
std::map<int, std::string> available_move_types = {
    {0, "Uniform"}};

#    ifndef NO_GRAPHICS
//transforms the vector moved_blocks to a vector of ints and adds it in glob_breakpoint_state
void transform_blocks_affected(t_pl_blocks_to_be_moved blocksAffected) {
    get_bp_state_globals()->get_glob_breakpoint_state()->blocks_affected_by_move.clear();
    for (size_t i = 0; i < blocksAffected.moved_blocks.size(); i++) {
        //size_t conversion is required since block_num is of type ClusterBlockId and can't be cast to an int. And this vector has to be of type int to be recognized in expr_eval class

        get_bp_state_globals()->get_glob_breakpoint_state()->blocks_affected_by_move.push_back(size_t(blocksAffected.moved_blocks[i].block_num));
    }
}

void stop_placement_and_check_breakpoints(t_pl_blocks_to_be_moved& blocks_affected, e_move_result move_outcome, double delta_c, double bb_delta_c, double timing_delta_c) {
    t_draw_state* draw_state = get_draw_state_vars();
    if (draw_state->list_of_breakpoints.size() != 0) {
        //update current information
        transform_blocks_affected(blocks_affected);
        get_bp_state_globals()->get_glob_breakpoint_state()->move_num++;
        get_bp_state_globals()->get_glob_breakpoint_state()->from_block = size_t(blocks_affected.moved_blocks[0].block_num);

        //check for breakpoints
        set_placer_breakpoint_reached(check_for_breakpoints(true)); // the passed flag is true as we are in the placer
        if (placer_breakpoint_reached())
            breakpoint_info_window(get_bp_state_globals()->get_glob_breakpoint_state()->bp_description, *get_bp_state_globals()->get_glob_breakpoint_state(), true);
    } else
        set_placer_breakpoint_reached(false);

    if (placer_breakpoint_reached() && draw_state->show_graphics) {
        std::string msg = available_move_types[0];
        if (move_outcome == 0)
            msg += vtr::string_fmt(", Rejected");
        else if (move_outcome == 1)
            msg += vtr::string_fmt(", Accepted");
        else
            msg += vtr::string_fmt(", Aborted");

        msg += vtr::string_fmt(", Delta_cost: %1.6f (bb_delta_cost= %1.5f , timing_delta_c= %6.1e)", delta_c, bb_delta_c, timing_delta_c);

        highlight_moved_block_and_its_terminals(blocks_affected);
        update_screen(ScreenUpdatePriority::MAJOR, msg.c_str(), PLACEMENT, nullptr);
    }
}

#    endif //NO_GRAPHICS

#endif //VTR_ENABLE_DEBUG_LOGGING
