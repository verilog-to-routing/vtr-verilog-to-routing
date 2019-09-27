#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"
#include "move_transactions.h"


enum class e_create_move {
    VALID, //Move successful and legal
    ABORT, //Unable to perform move
};


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

#endif
