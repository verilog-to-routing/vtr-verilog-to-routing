#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"
#include "move_transactions.h"
#include "compressed_grid.h"

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum e_move_result {
    REJECTED,
    ACCEPTED,
    ABORTED
};

enum class e_create_move {
    VALID, //Move successful and legal
    ABORT, //Unable to perform move
};

/** This struct holds all necessary values to set a manual move. Some of the membres such as block_id, to_x, to_y, and to_subtile are obtained from the user, whereas the delta costs are calculated by the placer**/
struct ManualMoveInfo {
    int block_id;                      ///block id of the block the user wants to move
    int to_x = -1;                     //the x value of the to location entered by the user
    int to_y = -1;                     //the y value of the to location entered by the user
    int to_subtile = 0;                //the subtile value of the to location entered by the user
    double delta_c = 0;                //the delta cost of the move calculated by the placer
    double bb_delta_c = 0;             //the delta bounding box cost calculated by the placer
    double timing_delta_c = 0;         //the telta timing cost calculated by the placer
    e_move_result user_move_outcome;   //whether the user decides to reject/accept the move
    e_move_result placer_move_outcome; //whether the placer decides to reject/accept the move
    bool valid_input;                  //whether the user input was valid
};

//Records a reasons for an aborted move
void log_move_abort(std::string reason);

//Prints a breif report about aborted move reasons and counts
void report_aborted_moves();

e_create_move create_move(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result find_affected_blocks(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result record_single_block_swap(t_pl_blocks_to_be_moved& blocks_affected, ClusterBlockId b_from, t_pl_loc to);

e_block_move_result record_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, t_pl_offset swap_offset);
e_block_move_result record_macro_macro_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro_from, int& imember_from, const int imacro_to, ClusterBlockId blk_to, t_pl_offset swap_offset);

e_block_move_result record_macro_move(t_pl_blocks_to_be_moved& blocks_affected,
                                      std::vector<ClusterBlockId>& displaced_blocks,
                                      const int imacro,
                                      t_pl_offset swap_offset);
e_block_move_result identify_macro_self_swap_affected_macros(std::vector<int>& macros, const int imacro, t_pl_offset swap_offset);
e_block_move_result record_macro_self_swaps(t_pl_blocks_to_be_moved& blocks_affected, const int imacro, t_pl_offset swap_offset);

bool is_legal_swap_to_location(ClusterBlockId blk, t_pl_loc to);

std::set<t_pl_loc> determine_locations_emptied_by_move(t_pl_blocks_to_be_moved& blocks_affected);

ClusterBlockId pick_from_block();

bool find_to_loc_uniform(t_logical_block_type_ptr type,
                         float rlim,
                         const t_pl_loc from,
                         t_pl_loc& to);
std::string e_move_result_to_string(e_move_result result);
#endif
