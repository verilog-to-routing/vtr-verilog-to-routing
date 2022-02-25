#ifndef VPR_MOVE_UTILS_H
#define VPR_MOVE_UTILS_H
#include "vpr_types.h"
#include "move_transactions.h"
#include "compressed_grid.h"

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

//This is to list all the available moves
enum class e_move_type {
    UNIFORM,
    MEDIAN,
    W_CENTROID,
    CENTROID,
    W_MEDIAN,
    CRIT_UNIFORM,
    FEASIBLE_REGION,
    MANUAL_MOVE,
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
    int edge;          //x or y coordinate of one edge of a bounding box
    float criticality; //the timing criticality of the net terminal that caused this edge
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

/**
 * @brief Stores the different range limiters
 *
 * original_rlim: the original range limit calculated by the anneal
 * first_rlim: the first range limit calculated based on the design and device size
 * dm_rlim: the range limit around the center calculated by a directed move
 */
struct t_range_limiters {
    float original_rlim;
    float first_rlim;
    float dm_rlim;
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
                         t_pl_loc& to,
                         ClusterBlockId b_from);

// Accessor f_placer_breakpoint_reached
// return true when a placer breakpoint is reached
bool placer_breakpoint_reached();

// setter for f_placer_breakpoint_reached
// Should be setted in the breakpoint calculation algorithm
void set_placer_breakpoint_reached(bool);

/**
 * @brief Find a legal swap to location for the given type in a specific region.
 *
 * This function finds a legal swap to location for the type "blk_type" in a region 
 * defined in "limit_coords". It returns the location it picks in "to_loc" that is passed by reference. It also returns true
 * if it was able to find a compatible location and false otherwise.
 * It is similar to find_to_loc_uniform but searching in a defined range instead of searching in a range around the current block location.
 *
 *  @param blk_type: the type of the moving block
 *  @param from_loc: the original location of the moving block
 *  @param limit_coords: the region where I can move the block to
 *  @param to_loc: the new location that the function picked for the block
 */
bool find_to_loc_median(t_logical_block_type_ptr blk_type, const t_pl_loc& from_loc, const t_bb* limit_coords, t_pl_loc& to_loc, ClusterBlockId b_from);

/**
 * @brief Find a legal swap to location for the given type in a range around a specific location.
 *
 * This function finds a legal swap to location for the type "blk_type" in a range around a specific location.
 * It returns the location it picks in "to_loc" that is passed by reference. It also returns true
 * if it was able to find a compatible location and false otherwise.
 * It is similar to find_to_loc_uniform but searching in a range around a defined location 
 * instead of searching in a range around the current block location.
 *
 *  @param blk_type: the type of the moving block
 *  @param from_loc: the original location of the moving block
 *  @param limit_coords: the region where I can move the block to
 *  @param to_loc: the new location that the function picked for the block
 */
bool find_to_loc_centroid(t_logical_block_type_ptr blk_type,
                          const t_pl_loc& from_loc,
                          const t_pl_loc& centeroid,
                          const t_range_limiters& range_limiters,
                          t_pl_loc& to_loc,
                          ClusterBlockId b_from);

std::string move_type_to_string(e_move_type);

/* find to loaction helper functions */
/**
 * @brief convert compressed location to normal location
 *
 * blk_type: the type of the moving block
 * cx: the x coordinate of the compressed location
 * cy: the y coordinate of the compressed location
 * loc: the uncompressed output location (returned in reference)
 */
void compressed_grid_to_loc(t_logical_block_type_ptr blk_type, int cx, int cy, t_pl_loc& loc);
/**
 * @brief find compressed location in a compressed range for a specific type
 * 
 * type: defines the moving block type
 * min_cx, max_cx: the minimum and maximum x coordinates of the range in the compressed grid
 * min_cy, max_cx: the minimum and maximum y coordinates of the range in the compressed grid
 * cx_from, cy_from: the x and y coordinates of the old location 
 * cx_to, cy_to: the x and y coordinates of the new location on the compressed grid
 * is_median: true if this is called from find_to_loc_median
 */
bool find_compatible_compressed_loc_in_range(t_logical_block_type_ptr type, int min_cx, int max_cx, int min_cy, int max_cy, int delta_cx, int cx_from, int cy_from, int& cx_to, int& cy_to, bool is_median);

/*
 * If the block to be moved (b_from) has a floorplan constraint, this routine changes the max and min coords
 * in the compressed grid (min_cx, min_cy, max_cx, max_cy) to make sure the range limit is within the floorplan constraint.
 *
 * Returns false if there is no intersection between the range limit and the floorplan constraint,
 * true otherwise.
 *
 *
 * If region size of the block's floorplan constraints is greater than 1, the block is constrained to more than one rectangular region.
 * In this case, we return true (i.e. the range limit intersects with
 * the floorplan constraints) to simplify the problem. This simplification can be done because
 * this routine is done for cpu time optimization, so we do not have to necessarily check each
 * complicated case to get correct functionality during place moves.
 *
 */
bool intersect_range_limit_with_floorplan_constraints(t_logical_block_type_ptr type, ClusterBlockId b_from, int& min_cx, int& min_cy, int& max_cx, int& max_cy, int& delta_cx);

std::string e_move_result_to_string(e_move_result move_outcome);

#endif
