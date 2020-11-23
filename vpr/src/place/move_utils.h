#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"
#include "move_transactions.h"
#include "compressed_grid.h"

extern vtr::vector<ClusterNetId, t_bb> bb_coords, bb_num_on_edges;

/* Cut off for incremental bounding box updates.                          *
 * 4 is fastest -- I checked.                                             */
/* To turn off incremental bounding box updates, set this to a huge value */
#define SMALL_NET 4

/* This is for the placement swap routines. A swap attempt could be       *
 * rejected, accepted or aborted (due to the limitations placed on the    *
 * carry chain support at this point).                                    */
enum e_move_result {
    REJECTED,
    ACCEPTED,
    ABORTED
};

//This is to list all the abailable moves
enum class e_move_type {
    UNIFORM,
    MEDIAN,
    W_CENTROID,
    CENTROID,
    W_MEDIAN,
    CRIT_UNIFORM,
    FEASIBLE_REGION
};

enum class e_create_move {
    VALID, //Move successful and legal
    ABORT, //Unable to perform move
};

/**
 * @brief Stores a bounding box edge of a net with the timing
 *        criticality of the net terminal that caused this edge
 */
struct t_edge_cost {
    int edge;
    float criticality;
};

/**
 * @brief Stores the bounding box of a net in terms of the minimum and
 *        maximum coordinates of the blocks forming the net, clipped to
 *        the region: (1..device_ctx.grid.width()-2, 1..device_ctx.grid.height()-1)
 *        and the timing cost of the net terminal that caused each edge. 
 *        This is useful for some directed move generators.
 */
struct t_bb_cost {
    t_edge_cost xmin = {0, 0.0};
    t_edge_cost xmax = {0, 0.0};
    t_edge_cost ymin = {0, 0.0};
    t_edge_cost ymax = {0, 0.0};
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

// Accessor f_placer_breakpoint_reached
// return true when a placer breakpoint is reached
bool placer_breakpoint_reached();

// setter for f_placer_breakpoint_reached
// Should be setted in the breakpoint calculation algorithm
void set_placer_breakpoint_reached(bool);

bool find_to_loc_median(t_logical_block_type_ptr type, const t_bb* limit_coords, const t_pl_loc from, t_pl_loc& to);

bool find_to_loc_centroid(t_logical_block_type_ptr type,
                          float rlim,
                          const t_pl_loc from,
                          const t_pl_loc centeroid,
                          t_pl_loc& to,
                          int dm_rlim);

std::string move_type_to_string(e_move_type);

#endif
